# Implementation Plan - TagSelectorOverlay-15

**本方案替代 `Skip upload/TagSelectorOverlay-14.md` 与 `Skip upload/TagSelectorOverlayClickInterception.md`**

- **Base Commit Hash**: `76c60907a65aeff77f6be72ded963c913d819e86`
- **Prerequisite Plans**: 无
- **Supersedes**: `Skip upload/TagSelectorOverlay-14.md`, `Skip upload/TagSelectorOverlayClickInterception.md`

---

## 第 1 关：起草前物理证据关 (Gate 1: Pre-Draft Evidence)

### 1.1 物理读取文件列表与确认
1. `src/ui/TagSelectorOverlay.h`：已通读全文（行 1-102），确认其继承自 `QFrame`，声明了 `isInternalWidget(QObject* obj) const`、`toggleTagSelection`、`changeEvent` 与 `eventFilter`。
2. `src/ui/TagSelectorOverlay.cpp`：已通读全文（行 1-415），确认了 `isInternalWidget` 的实现逻辑、`changeEvent` 的失活触发条件、`toggleTagSelection` 调用 `filterAndPopulate()` 的重建方式以及 `eventFilter` 的全局鼠标事件拦截流程。
3. `CMakeLists.txt`：已通读核验，确认 `src/ui/TagSelectorOverlay.h`（第 274 行）与 `src/ui/TagSelectorOverlay.cpp`（第 275 行）均已正确注册，本次修改无任何增删文件。

### 1.2 调用的外部成员函数原样签名
1. `UiHelper::getIcon(const QString& name, const QColor& color, int size)`：
   - 物理文件：`src/ui/UiHelper.h` 第 40 行：
     `static QIcon getIcon(const QString& name, const QColor& color = QColor(), int size = 16);`
2. `QGuiApplication::applicationState()`：
   - Qt 官方 API，返回当前全局应用状态枚举 `Qt::ApplicationState`。
3. `QWidget::windowHandle()`：
   - Qt 官方 API，返回底层关联的顶层 `QWindow*` 指针。

### 1.3 既有 API 检索日志（防另起炉灶）
- 检索：`filterAndPopulate()` 与 `toggleTagSelection()`，确认其为标签过滤与点选的核心通道。
- 确认：本次修改不新增任何另起炉灶的类或重复通道，完全在既有 `TagSelectorOverlay` 类内部完成逻辑闭环与自我纠偏。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与根因溯源
在上一版 `TagSelectorOverlayClickInterception.md` 落地后，出现了“只要在 `TagSelectorOverlay` 界面上点击任何位置（标签按钮、搜索框、滚动区、空白处）就会瞬间被误关闭”的严重故障。经排查，该故障由两层底层机制认知盲区导致：

1. **第一重致命根因：Qt 6 QPA 底层原生窗口事件分发误判**：
   在 Qt 6 / QPA 架构下，操作系统的底层原生鼠标按下事件（`WM_LBUTTONDOWN`）首先投递给顶层窗口的 `QWidgetWindow`（继承自 `QWindow`，非 `QWidget`）。挂载在 `qApp` 上的 `eventFilter` 捕获到的第一个事件目标就是该 `QWidgetWindow`。原代码中的 `isInternalWidget(obj)` 仅沿 `QWidget::parentWidget()` 回溯，当 `obj` 为 `QWidgetWindow` 时直接返回 `false`，导致在按下鼠标的第 0 毫秒就被当成“外部点击”，立刻触发 `closeOverlay()` 并吞噬事件。
2. **第二重致命根因：`Qt::Tool` 工具窗特性与全量销毁重建引发焦点归零**：
   `TagSelectorOverlay` 带有 `Qt::Tool` 窗口标志且附属于主窗，其 `isActiveWindow()` 常态为 `false`。原代码在 `toggleTagSelection` 中调用 `filterAndPopulate()`，把所有按钮全量 `deleteLater` 物理销毁后再重新 `new`。正在被点击的按钮瞬间蒸发，导致全局焦点归零，触发 `ActivationChange`；原 `changeEvent` 检测到 `!isActiveWindow()` 且焦点丢失，再次误触发 `closeOverlay()`。

### 1.2 重构治理目标
1. 建立兼顾 `QWindow` 与 `QWidget` 的双层内外识别机制，确保自身窗口原生事件 100% 被判定为内部事件并正常放行。
2. 纠偏 `changeEvent` 的失活判定逻辑，仅在应用程序整体切换到后台（`Qt::ApplicationInactive`）时才允许失活离场，彻底杜绝内部焦点微观扰动误关。
3. 实现标签按钮“就地状态切换（In-Place State Mutation）”，点击时仅更新 `checked`、`property` 与图标，彻底消除全量销毁重建与焦点震荡。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/TagSelectorOverlay.h`（修改：新增私有方法 `updateTagButtonsSelection` 声明）
2. `src/ui/TagSelectorOverlay.cpp`（修改：完善 `isInternalWidget` 双层判定、重构 `toggleTagSelection` 就地更新、收敛 `changeEvent` 判定，补充 `<QWindow>` 与 `<QStyle>` 头文件包含）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 修改 `src/ui/TagSelectorOverlay.h`

```
<<<<<<< SEARCH
    void filterAndPopulate();
    void toggleTagSelection(const QString& tag);
    void recordRecentTag(const QString& tag);
=======
    void filterAndPopulate();
    void toggleTagSelection(const QString& tag);
    void updateTagButtonsSelection();
    void recordRecentTag(const QString& tag);
>>>>>>> REPLACE
```

### 3.2 修改 `src/ui/TagSelectorOverlay.cpp`

#### 替换块 1：引入必要头文件
```
<<<<<<< SEARCH
#include <QScreen>
#include <QScrollBar>

namespace QuarkMeta {
=======
#include <QScreen>
#include <QScrollBar>
#include <QWindow>
#include <QStyle>

namespace QuarkMeta {
>>>>>>> REPLACE
```

#### 替换块 2：完善 `isInternalWidget` 双层内外判定
```
<<<<<<< SEARCH
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
=======
bool TagSelectorOverlay::isInternalWidget(QObject* obj) const {
    if (!obj) return false;

    // 1. 原生窗口层 (Qt 6 QPA QWidgetWindow / QWindow 派发阶段)
    if (QWindow* win = qobject_cast<QWindow*>(obj)) {
        QWindow* overlayWin = const_cast<TagSelectorOverlay*>(this)->windowHandle();
        if (!overlayWin) return false;

        QWindow* currWin = win;
        while (currWin) {
            if (currWin == overlayWin) {
                return true;
            }
            currWin = currWin->parent() ? currWin->parent() : currWin->transientParent();
        }
        return false;
    }

    // 2. 控件树层 (QWidget 控件链向上回溯)
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
>>>>>>> REPLACE
```

#### 替换块 3：重构 `toggleTagSelection` 并实现就地状态更新
```
<<<<<<< SEARCH
void TagSelectorOverlay::toggleTagSelection(const QString& tag) {
    QString cleanTag = tag.trimmed();
    if (cleanTag.isEmpty()) return;

    // 纯内存维护：严禁在此执行任何 SQLite 写入或磁盘落盘
    if (m_selectedTags.contains(cleanTag)) {
        m_selectedTags.removeAll(cleanTag);
    } else {
        m_selectedTags.append(cleanTag);
    }

    filterAndPopulate();

    // 实时通知前端界面同步刷新标签胶囊 (零 I/O 纯内存)
    emit selectionChanged(m_selectedTags);
}
=======
void TagSelectorOverlay::toggleTagSelection(const QString& tag) {
    QString cleanTag = tag.trimmed();
    if (cleanTag.isEmpty()) return;

    // 纯内存维护：严禁在此执行任何 SQLite 写入或磁盘落盘
    if (m_selectedTags.contains(cleanTag)) {
        m_selectedTags.removeAll(cleanTag);
    } else {
        m_selectedTags.append(cleanTag);
    }

    // 就地刷新按钮选中状态，杜绝粗暴销毁重建导致焦点瞬间丢失与窗口失活误杀
    updateTagButtonsSelection();

    // 实时通知前端界面同步刷新标签胶囊 (零 I/O 纯内存)
    emit selectionChanged(m_selectedTags);
}

void TagSelectorOverlay::updateTagButtonsSelection() {
    auto updateLayout = [this](FlowLayout* layout, bool isRec) {
        if (!layout) return;
        for (int i = 0; i < layout->count(); ++i) {
            QLayoutItem* item = layout->itemAt(i);
            if (!item || !item->widget()) continue;
            QPushButton* btn = qobject_cast<QPushButton*>(item->widget());
            if (!btn) continue;

            const QString tag = btn->text();
            const bool isSelected = m_selectedTags.contains(tag);
            if (btn->isChecked() != isSelected) {
                btn->setChecked(isSelected);
            }
            btn->setProperty("selected", isSelected ? "true" : "false");
            btn->style()->unpolish(btn);
            btn->style()->polish(btn);

            if (isRec) {
                btn->setIcon(UiHelper::getIcon(isSelected ? "check" : "star_filled", isSelected ? QColor("#FFFFFF") : QColor("#378ADD"), 12));
            } else {
                btn->setIcon(UiHelper::getIcon(isSelected ? "check" : "clock", isSelected ? QColor("#FFFFFF") : QColor("#888888"), 12));
            }
        }
    };

    updateLayout(m_recFlowLayout, true);
    updateLayout(m_recentFlowLayout, false);
}
>>>>>>> REPLACE
```

#### 替换块 4：安全收敛 `changeEvent` 失活判定
```
<<<<<<< SEARCH
void TagSelectorOverlay::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange || event->type() == QEvent::WindowDeactivate) {
        if (!isActiveWindow() && !this->isAncestorOf(QApplication::focusWidget())) {
            closeOverlay();
        }
    }
    QFrame::changeEvent(event);
}
=======
void TagSelectorOverlay::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange) {
        QWidget* parentWin = parentWidget() ? parentWidget()->window() : nullptr;
        bool isAnyWindowActive = isActiveWindow() || (parentWin && parentWin->isActiveWindow());
        if (!isAnyWindowActive && QGuiApplication::applicationState() == Qt::ApplicationInactive) {
            closeOverlay();
        }
    }
    QFrame::changeEvent(event);
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建命令
```bash
cmake --build build --config Debug --target QuarkMeta -j8
```

### 4.2 运行时验收标准（严格控制变量验证）
1. **测试场景 1（标签点选与取消）**：
   - 打开 `TagSelectorOverlay` 浮层；
   - 点击“推荐”中的任一标签（如 `main`），按钮瞬间变为选中状态（打勾图标、高亮样式），浮层保持开启，**绝不发生关闭**；
   - 再次点击该标签，按钮取消选中，浮层依然保持开启，属性面板胶囊实时同步增删；
   - 连续点选多个“最近使用”中的标签，界面流畅无任何闪烁，焦点稳定。
2. **测试场景 2（内部控件正常交互）**：
   - 点击顶部 `m_searchEdit` 搜索框，能够正常输入文字并过滤标签列表；
   - 在搜索框内右键呼出应用级右键菜单，点击复制/粘贴等菜单项，浮层绝不关闭；
   - 点击“推荐”与“最近使用”的折叠/展开标题栏按钮，对应区域平滑折叠/展开，浮层不关闭；
   - 点击滚动条并上下拖拽，滚动区正常滑动，浮层不关闭。
3. **测试场景 3（浮层背景拖拽与空白点击）**：
   - 在浮层顶部或边距等空白区域按住鼠标左键拖动，浮层跟随鼠标平滑移动；
   - 在空白区域单纯单击并松开鼠标，浮层保持开启，绝不被误判为外部点击而关闭。
4. **测试场景 4（外部点击拦截与离场）**：
   - 点击主界面上的文件项或背景空白区域，浮层平滑关闭，且该次点击被完整吞噬，背景文件不被误选中；
   - 按下 `Esc` 键，浮层正常关闭；
   - 按下 `Alt + Tab` 切换到外部第三方程序，浮层自动失活离场关闭。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **复用 `UiHelper::getIcon`**：保持标签图标样式与全局色彩（`#FFFFFF`、`#378ADD`、`#888888`）100% 统一。
2. **复用 Qt 样式重刷新标准机制**：通过 `btn->style()->unpolish(btn)` 与 `polish(btn)` 触发 QSS 伪类重绘，杜绝手写内联样式侵入。
3. **复用 `QGuiApplication::applicationState()`**：使用 Qt 官方跨平台权威应用生命周期状态，杜绝平台 Hack。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本 / 展示层 | 同步方向与契约 |
| :--- | :--- | :--- | :--- |
| **选中的标签集合** | `TagSelectorOverlay::m_selectedTags` | `QPushButton::isChecked`、`MetaPanel` 胶囊展示 | 内存实时单向同步；仅在离场关闭时发射 `tagsCommitted` 信号落盘 |
| **内部/外部判定事实** | `QWindow::windowHandle()` 与 `QWidget` 父子控件链 | `TagSelectorOverlay::eventFilter` 局部判定 | 实时由底层事件对象向上回溯计算，无持久化状态副本 |
| **应用全局激活状态** | `QGuiApplication::applicationState()` | `TagSelectorOverlay::changeEvent` 判定 | 由 Qt QPA 原生总线广播，只读查询 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部调用接口核查**：
   - `UiHelper::getIcon`：声明于 `src/ui/UiHelper.h` 第 40 行，`public static`，完全一致。
2. **新增私有方法核查**：
   - `void updateTagButtonsSelection()`：
     - 声明：`src/ui/TagSelectorOverlay.h` 第 64 行（`private` 区域）；
     - 定义：`src/ui/TagSelectorOverlay.cpp`；
     - 调用点：仅在 `TagSelectorOverlay::toggleTagSelection` 内部被调用，访问权限完全受控。
3. **既有公有 API 签名绝对冻结**：
   - `TagSelectorOverlay` 构造函数、析构函数、`closeOverlay()` 以及所有 signals 签名 100% 保持只读未变，调用方无需任何适配。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `src/ui/TagSelectorOverlay.cpp` 中新增使用了 `QWindow` 与 `QStyle`。
2. 已在 `src/ui/TagSelectorOverlay.cpp` 显式添加 `#include <QWindow>` 与 `#include <QStyle>`，确保 `qobject_cast<QWindow*>`、`win->parent()`、`win->transientParent()` 及 `btn->style()` 均具备完整类型定义，杜绝任何不完整类型或间接依赖隐患。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 结论 | 位置与证据 |
| :---: | :--- | :---: | :--- |
| 1 | 严禁直接修改代码文件，实施方案放置于隔离目录 | **满足** | 方案创建于 `Implementation Plan/TagSelectorOverlay-15.md` |
| 2 | SEARCH 块原文在目标文件中真实存在且唯一匹配 | **满足** | 已逐块在 `TagSelectorOverlay.h` 与 `.cpp` 中比对行号与唯一性 |
| 3 | 方案调用的外部成员函数在 `.h` 中真实存在且签名一致 | **满足** | 已核实 `UiHelper::getIcon` 等函数签名 |
| 4 | 方案新增接口声明、定义与调用点 100% 一致 | **满足** | `updateTagButtonsSelection()` 在 `.h` 声明、`.cpp` 定义并调用 |
| 5 | 被包含头文件闭合完整，无不完整类型隐患 | **满足** | 显式包含了 `<QWindow>` 与 `<QStyle>` |
| 6 | CMakeLists.txt 全覆盖，增删文件同步注册 | **满足** | 本次不增删文件，既有文件已在 `CMakeLists.txt` 注册 |
| 7 | 跨类访问级别（public/private）合规 | **满足** | `updateTagButtonsSelection` 严格私有，无违规跨类访问 |
| 8 | 方案未触及方案范围外的文件 | **满足** | 仅涉及 `TagSelectorOverlay.h` 与 `TagSelectorOverlay.cpp` |
| 9 | 遵照 10 大必备章节结构要求 | **满足** | Chapter 1 至 Chapter 10 章节齐备无裁剪 |
| 10 | 遵循 Memories.md 第 18 章真理源设计规范 | **满足** | 严格贯彻双层判定、失活收敛与就地切换契约 |
| 11 | 严禁 Win32 Hack 与全局强抢焦点 | **满足** | 无任何 `SetForegroundWindow` 等非法平台级 Hack |
| 12 | 严禁内联硬编码样式，保持 QSS 框架统一 | **满足** | 仅使用 `setProperty("selected")` 配合 `polish/unpolish` |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 维度 | 原设计规范 / 数值 | 本方案设计 / 数值 | 判定结果 |
| :--- | :--- | :--- | :---: |
| **交互逻辑零篡改** | 点击标签切换选中状态并在面板实时显示胶囊 | 保持完全一致：点击即时切换状态并实时广播胶囊 | **100% 保持** |
| **浮层离场落盘逻辑** | 失焦关闭时单次原子提交落盘（`tagsCommitted`） | 保持完全一致：仅在关闭时单次落盘，点选过程零 I/O | **100% 保持** |
| **选中图标尺寸** | `12px` (`QSize(12, 12)`) | `12px` (`QSize(12, 12)`) | **100% 保持** |
| **推荐标签勾选颜色** | 选中 `#FFFFFF`，未选 `#378ADD` | 选中 `#FFFFFF`，未选 `#378ADD` | **100% 保持** |
| **最近使用勾选颜色** | 选中 `#FFFFFF`，未选 `#888888` | 选中 `#FFFFFF`，未选 `#888888` | **100% 保持** |
| **外部点击拦截行为** | 外部点击消费事件并关闭浮层，不穿透背景 | 保持完全一致，同时修复自身点击被误杀的核心缺陷 | **100% 保持** |
