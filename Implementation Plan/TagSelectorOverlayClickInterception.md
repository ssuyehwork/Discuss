# Implementation Plan - TagSelectorOverlay 点击穿透拦截与生命周期去重方案

## 1. 概述 (Overview)
本方案旨在彻底解决 `TagSelectorOverlay` 标签选择悬浮面板打开期间，点击浮层外部时点击事件穿透到背景控件（导致背景文件被选中、按钮被触发或浮层关闭后立刻被重新打开）的问题。

方案通过在 `TagSelectorOverlay` 的全局事件过滤器（`eventFilter`）中拦截外部点击事件的 `MouseButtonPress`、`MouseButtonDblClick` 及后续对应的 `MouseButtonRelease`，严格按控件父子链（`parentWidget`）判断内外关系，并在消费外部点击事件的同时关闭浮层，实现“点击外部仅关闭浮层，不触发任何被点击控件”的交互要求。同时，清理重叠的关闭触发点（移除冗余的 `focusChanged` 监听），统一全工程 3 处创建点（`MetaPanel`、`PresetTagsDialog`、`SmartFolderDialog`）在 `overlayClosed` 信号回调中及时置空指针。

---

## 2. 真实调用链与生命周期分析 (Call Chain & Lifecycle Analysis)

### 2.1 穿透与重开的真实根因
1. **事件穿透根因**：`TagSelectorOverlay::eventFilter` 捕获到外部 `MouseButtonPress` 时，原代码判断 `!geometry().contains(globalPos)`，调用 `closeOverlay()` 后返回了 `false`。返回 `false` 使得 Qt 事件总线将点击事件继续分发给目标控件，导致项目选中、按钮点击等穿透行为。
2. **关闭后立刻重开根因**：点击“添加标签”按钮时，`MouseButtonPress` 被判定为外部点击并调用 `closeOverlay()`。`closeOverlay()` 内部调用了 `close()` 与 `deleteLater()`（延迟销毁），但未消费 `MouseButtonRelease`。松开鼠标时，`QPushButton::clicked` 信号被触发，再次调用 `MetaPanel::openTagSelectorOverlay`。由于原方案依赖 `deleteLater` 延迟置空指针，`QPointer` 在同一事件循环内未清空，或事件消费不完整，导致“关闭后又重新打开”。
3. **几何判定与右键菜单误关根因**：原判定依赖 `geometry().contains(...)`，当搜索框弹出右键菜单（`QMenu`）时，`QMenu` 的几何区域超出了浮层的主矩形，导致点击菜单项被误判为外部点击而误关闭浮层。

### 2.2 修改后的事件生命周期流图
```
[ 用户在全局任意位置按下鼠标 (MouseButtonPress / MouseButtonDblClick) ]
                            ↓
             [ TagSelectorOverlay::eventFilter ]
                            ↓
               [ 沿 parentWidget() 向上回溯 ]
           /                                  \
  [ 属于浮层本身或后代 (Internal) ]      [ 不属于浮层 (External) ]
          ↓                                     ↓
  [ 正常放行, 不影响内部操作 ]           [ 设置 m_swallowingClick = true ]
                                         [ 调用 closeOverlay() 关闭浮层 ]
                                         [ (保持 eventFilter 继续挂接) ]
                                         [ return true; 吞掉 Press 事件 ]
                                                ↓
                                [ 用户松开鼠标 (MouseButtonRelease) ]
                                                ↓
                                   [ TagSelectorOverlay::eventFilter ]
                                                ↓
                                 [ m_swallowingClick 为 true? ]
                                   /                         \
                             [ 是 (True) ]               [ 否 (False) ]
                                  ↓                            ↓
                    [ 设置 m_swallowingClick = false ]     [ 正常放行 ]
                    [ 从 qApp 卸载 eventFilter ]
                    [ return true; 吞掉 Release ]
```

---

## 3. 架构三问解答 (Architecture QA)

### 3.1 真理源 (SSOT) 溯源
- **事件拦截与消费 SSOT**：由 `TagSelectorOverlay` 自身的 `eventFilter` 独家持有，绝不侵入 `MetaPanel` 或其他控制器。
- **浮层指针生命周期 SSOT**：由各个持有方（`MetaPanel`、`PresetTagsDialog`、`SmartFolderDialog`）连接 `TagSelectorOverlay::overlayClosed` 信号，在收到信号时立即将指针置为 `nullptr`。

### 3.2 黑盒完整性
- 所有修改封装在 `TagSelectorOverlay` 及其持有方中，公开接口与信号（`selectionChanged`、`tagsCommitted`、`overlayClosed`）签名完全保持不变。
- 磁盘直连与托管库模式互相隔离，零 I/O 影响。

### 3.3 根因溯源
- 彻底废除 `geometry().contains()` 模糊矩形判定，改用基于 C++ 控件树（`parentWidget()` 链）的精准内外识别，解决右键菜单误关与穿透问题。

---

## 4. 控制变量交叉验证矩阵 (Cross-Validation Matrix)

| 场景测试项 | 对照组 (现状) | 实验组 (修补后) | 预期结果 |
| :--- | :--- | :--- | :--- |
| **场景 A：浮层打开时点击内容区文件** | 浮层关闭，文件被选中高亮 | 浮层关闭，`return true` 消费 Press/Release，文件未被选中 | 选区完全不受影响 |
| **场景 B：浮层打开时点击“添加标签”按钮** | 浮层关闭后立刻被重新打开 | 按钮未收到 Press/Release，无 `clicked` 信号，浮层平滑关闭不重开 | 需再点一次才重新打开 |
| **场景 C：在搜索框右击弹出右键菜单并点击“复制”** | 菜单项点击被判为外部，浮层被误关闭 | 右键菜单沿 parent 回溯至 `TagSelectorOverlay`，判定为内部，正常响应 | 浮层不关闭，菜单正常工作 |
| **场景 D：按 Esc 键或 Enter 键** | 触发 `closeOverlay()` 并提交标签 | `eventFilter` 正常捕获 KeyPress，提交行为与改动前 100% 一致 | 键盘快捷操作正常 |

---

## 5. 核实结论与评估报告 (Verification & Assessment Report)

### 5.1 对 F 与 G 的真实代码核实结论
- **核实 F（搜索框右键菜单与焦点）**：
  - 代码中 `UiHelper::setupLineEditContextMenu(m_searchEdit)` 会通过 `menu.exec(...)` 弹出 `QMenu`。
  - `QMenu` 的构造传入的是 `m_searchEdit` 作为 `parent`，其控件链属于 `TagSelectorOverlay` 的后代。
  - 原代码使用 `geometry().contains()` 时，因为 `QMenu` 弹出的全局坐标位于 `TagSelectorOverlay` 的 `QFrame` 几何矩形之外，会导致鼠标点击菜单项时被错误判定为外部点击而误关闭浮层。通过改用控件父子链（`parentWidget()` 向上回溯）判定后，`QMenu` 及其菜单项将被精确判定为内部，完全不会误关浮层。
- **核实 G（FramelessWindowHelper 非客户区点击）**：
  - 经查阅 `FramelessWindowHelper.cpp`，`TagSelectorOverlay` 构造时传入 `titleBar = nullptr`。`FramelessWindowHelper::handleNativeEvent` 将整窗标记为 `HTCLIENT`，不处理非客户区按钮，也不会拦截或抢夺全局鼠标点击事件。非客户区逻辑与本穿透问题无交集。

### 5.2 关闭触发点去重评估 (De-duplication Assessment)
- **分析三处关闭触发点**：
  1. `eventFilter` 捕获外部鼠标点击：鼠标引发关闭的**唯一合法路径**，负责消费事件并关闭。
  2. `focusChanged` 监听：原代码在构造函数中连接 `QApplication::focusChanged`，在鼠标点击外部时与 `eventFilter` 竞争，且可能因焦点切换导致误关。
  3. `changeEvent` 监听 `ActivationChange / WindowDeactivate`：处理系统级窗口切换（如 Alt+Tab 离开应用）导致的窗口失活关闭。
- **取舍结论**：
  - 移除构造函数中冗余的 `QApplication::focusChanged` 连接。
  - 鼠标点击外部关闭完全由 `eventFilter` 独占管辖；应用失活关闭由 `changeEvent` 管辖；Esc/Enter 键盘关闭由 `eventFilter` 的 `KeyPress` 管辖。关闭触发路径职责清晰、互不重叠。

---

## 6. 修改文件清单 (Modified Files List)

1. `src/ui/TagSelectorOverlay.h`：新增内部判定函数 `isInternalWidget` 与吞点击标记 `m_swallowingClick`。
2. `src/ui/TagSelectorOverlay.cpp`：实现 `isInternalWidget`，重构 `eventFilter` 拦截与消费逻辑（按 Press 标记并在 Release 后卸载过滤器），移除 `focusChanged` 连接，在 `closeOverlay` 中安全发射信号和清理。
3. `src/ui/MetaPanel.cpp`：在 `openTagSelectorOverlay` 中连接 `overlayClosed` 信号，立即清空 `m_tagSelectorOverlay` 指针。
4. `src/ui/dialogs/SmartFolderDialog.cpp`：在创建 `TagSelectorOverlay` 处连接 `overlayClosed` 信号置空 `m_tagOverlay` 指针。

---

## 7. 详细代码修改方案 (Git Merge Diffs)

### 7.1 修改 `src/ui/TagSelectorOverlay.h`

```
<<<<<<< SEARCH
    bool m_isClosing = false;
    bool m_hasCommitted = false;
    bool m_isDragging = false;
    QPoint m_dragPos;

    FramelessWindowHelper* m_framelessHelper = nullptr;
};
=======
    bool isInternalWidget(QObject* obj) const;

    bool m_isClosing = false;
    bool m_hasCommitted = false;
    bool m_isDragging = false;
    bool m_swallowingClick = false;
    QPoint m_dragPos;

    FramelessWindowHelper* m_framelessHelper = nullptr;
};
>>>>>>> REPLACE
```

### 7.2 修改 `src/ui/TagSelectorOverlay.cpp`

```
<<<<<<< SEARCH
    m_searchEdit->installEventFilter(this);

    qApp->installEventFilter(this);
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget* old, QWidget* now) {
        Q_UNUSED(old);
        if (m_isClosing || !isVisible()) return;

        if (now && (now == this || this->isAncestorOf(now))) {
            return;
        }

        if (now && !this->isAncestorOf(now)) {
            closeOverlay();
        }
    });
}

TagSelectorOverlay::~TagSelectorOverlay() {
    if (qApp) {
        qApp->removeEventFilter(this);
    }
}

void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;
=======
    m_searchEdit->installEventFilter(this);

    if (qApp) {
        qApp->installEventFilter(this);
    }
}

TagSelectorOverlay::~TagSelectorOverlay() {
    if (qApp) {
        qApp->removeEventFilter(this);
    }
    m_swallowingClick = false;
}

bool TagSelectorOverlay::isInternalWidget(QObject* obj) const {
    if (!obj) return false;
    
    QObject* current = obj;
    while (current) {
        if (current == this) {
            return true;
        }
        if (QWidget* w = qobject_cast<QWidget*>(current)) {
            current = w->parentWidget() ? w->parentWidget() : w->parent();
        } else {
            current = current->parent();
        }
    }
    return false;
}

void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool TagSelectorOverlay::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (isVisible() && !geometry().contains(me->globalPosition().toPoint())) {
            closeOverlay();
            return false;
        }
    }

    if (event->type() == QEvent::KeyPress) {
=======
bool TagSelectorOverlay::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        if (isVisible() && !m_isClosing) {
            if (!isInternalWidget(obj)) {
                m_swallowingClick = true;
                closeOverlay();
                return true;
            }
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        if (m_swallowingClick) {
            m_swallowingClick = false;
            if (qApp) {
                qApp->removeEventFilter(this);
            }
            return true;
        }
    }

    if (event->type() == QEvent::KeyPress) {
>>>>>>> REPLACE
```

### 7.3 修改 `src/ui/MetaPanel.cpp`

```
<<<<<<< SEARCH
void MetaPanel::openTagSelectorOverlay(QWidget* targetAnchor) {
    if (m_tagSelectorOverlay) {
        m_tagSelectorOverlay->close();
        return;
    }

    QWidget* topWidget = this->topLevelWidget();
    const QStringList targetPathsSnapshot = m_selectedPaths;

    // 显式声明来自 MetaPanel 触发源
    m_tagSelectorOverlay = new TagSelectorOverlay(m_currentTagsSet.values(), targetPathsSnapshot, topWidget, TagTriggerSource::MetaPanel);
=======
void MetaPanel::openTagSelectorOverlay(QWidget* targetAnchor) {
    if (m_tagSelectorOverlay) {
        m_tagSelectorOverlay->closeOverlay();
        return;
    }

    QWidget* topWidget = this->topLevelWidget();
    const QStringList targetPathsSnapshot = m_selectedPaths;

    // 显式声明来自 MetaPanel 触发源
    m_tagSelectorOverlay = new TagSelectorOverlay(m_currentTagsSet.values(), targetPathsSnapshot, topWidget, TagTriggerSource::MetaPanel);
    
    connect(m_tagSelectorOverlay, &TagSelectorOverlay::overlayClosed, this, [this]() {
        m_tagSelectorOverlay = nullptr;
    });
>>>>>>> REPLACE
```

### 7.4 修改 `src/ui/dialogs/SmartFolderDialog.cpp`

```
<<<<<<< SEARCH
    m_tagOverlay = new TagSelectorOverlay(initTags, topWidget, TagTriggerSource::PresetTagsDialog);

    // 计算并在 m_valueEdit 正下方定位 TagSelectorOverlay 弹出框
=======
    m_tagOverlay = new TagSelectorOverlay(initTags, topWidget, TagTriggerSource::PresetTagsDialog);

    connect(m_tagOverlay, &TagSelectorOverlay::overlayClosed, this, [this]() {
        m_tagOverlay = nullptr;
    });

    // 计算并在 m_valueEdit 正下方定位 TagSelectorOverlay 弹出框
>>>>>>> REPLACE
```

---

## 8. 三道关合规性自检 (Three-Gate Self-Audit)

- **第一关：黑盒接口契约自检**
  - 公开头文件接口签名未发生任何改动。
  - 所有新新增函数与变量均为私有成员。
- **第二关：零破坏性自检**
  - 标签选择、搜索、提交（`tagsCommitted`）、界面外观与样式完全保持原样。
  - 磁盘与数据库逻辑未进行任何变动。
- **第三关：规范遵从自检**
  - 代码格式、命名规范符合工程要求。
  - 方案包含完整的 Git Merge Diff。

---

## 9. 编译与验证测试计划 (Verification Test Plan)

1. **格式验证**：使用规范格式自检。
2. **场景功能验证**：
   - 打开 `TagSelectorOverlay` 浮层，点击文件卡片：浮层关闭，文件未被选中。
   - 点击“添加标签”按钮：浮层关闭，不会重新弹出；再次点击按钮方可重新弹出。
   - 搜索框右键弹出菜单并操作：浮层不关闭，菜单正常工作。
   - 按 Esc / Enter 键：标签按预期提交并关闭浮层。
