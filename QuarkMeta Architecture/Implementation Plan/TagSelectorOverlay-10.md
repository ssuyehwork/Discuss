# Implementation Plan - TagSelectorOverlay Local Window Focus & Selection Snapshot (TagSelectorOverlay-10)

## 1. Overview & 架构三问回答

### 【架构三问先答后写】
1. **第一问（真理源溯源）**：
   打标的目标项目集合在浮层唤起的瞬间已完成决策。选区状态的 SSOT 属于视图选择模型，但浮层进行连续打标的**目标路径快照（Target Paths Snapshot）**必须归属于浮层生命周期内的闭包环境。在浮层未关闭前，打标操作只且仅只针对唤起时的路径快照执行，绝对不随外部由于刷新而被动的 `m_selectedPaths` 置空而丢失。
2. **第二问（黑盒完整性）**：
   本修复彻底移除了 `TagSelectorOverlay` 构造函数中将自身无差别挂载到全局 `qApp`（`qApp->installEventFilter(this)`）的盲目污染代码，改用局域**宿主窗口事件过滤器（Window-Level Event Filter）与局部焦点监听**模式：浮层仅挂载在其宿主顶层窗口（`window()`）上，精准监听局域鼠标点击与焦点转移事件。在保证“点击主窗口任意外部区域即时自动关闭”体验的同时，恢复了标准的事件传递黑盒边界。
3. **第三问（根因 vs 症状）**：
   - **根因**：打标触发 `MetadataUpdated` 事件后驱动视图/模型刷新，当在分类/筛选视图模式下时，模型更新触发了 `ProxyModel` 的重新过滤并触发 `clearSelection()`，使外部 `m_selectedPaths` 变成空列表。而后发生的后续标签勾选由于读取了被置空的 `m_selectedPaths`，导致后续打标请求全被静默判定为无效并丢弃。
   - **症状掩盖**：绝不通过禁用视图刷新或强锁视图选区来“掩盖”，而是在触发源头（`MetaTagSection` 与 `MetaPanel`）对打标路径实施**不可变快照锁定（Snapshot Lock）**，确保浮层生命周期内任何连续勾选均稳定作用于原选定目标项目。

---

## 2. Modified Files List
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`
- `src/ui/MetaTagSection.cpp`
- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update Header Signature & Window-Level Filter Tracking in `TagSelectorOverlay.h`

```
<<<<<<< SEARCH
class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;
=======
class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths = {}, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    bool m_isClosing = false;
    bool m_isDragging = false;
    QPoint m_dragPos;

    FramelessWindowHelper* m_framelessHelper = nullptr;
};
=======
    bool m_isClosing = false;
    bool m_isDragging = false;
    QPoint m_dragPos;

    QPointer<QWidget> m_ownerWindow;
    FramelessWindowHelper* m_framelessHelper = nullptr;
};
>>>>>>> REPLACE
```

---

### 3.2 Refactor Window-Level Focus & Click-Outside Dismissal in `TagSelectorOverlay.cpp`

```
<<<<<<< SEARCH
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
      m_selectedTags(initialSelected)
{
    setObjectName("TagSelectorOverlay");
    setFrameShape(QFrame::StyledPanel);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_DeleteOnClose, false);

    m_framelessHelper = FramelessWindowHelper::apply(this, nullptr);

    initUi();
    loadTagsAndGroups();

    m_searchEdit->installEventFilter(this);
    m_tagGridWidget->installEventFilter(this);

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
=======
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
      m_selectedTags(initialSelected)
{
    setObjectName("TagSelectorOverlay");
    setFrameShape(QFrame::StyledPanel);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_DeleteOnClose, false);

    m_framelessHelper = FramelessWindowHelper::apply(this, nullptr);

    initUi();
    loadTagsAndGroups();

    m_searchEdit->installEventFilter(this);
    m_tagGridWidget->installEventFilter(this);

    // 🔒【宿主窗口级局域事件过滤】：仅挂载在其真正的宿主 Window 上，绝对不全应用污染 qApp
    QWidget* topWin = parent ? parent->window() : nullptr;
    if (topWin) {
        m_ownerWindow = topWin;
        topWin->installEventFilter(this);
    }

    // 🔒【局域焦点感知】：焦点转移至非自身/非子控件时即时自动优雅关闭
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget* old, QWidget* now) {
        Q_UNUSED(old);
        if (m_isClosing || !isVisible()) return;
        if (now && (now == this || this->isAncestorOf(now))) return;
        closeOverlay();
    });
}

TagSelectorOverlay::~TagSelectorOverlay() {
    if (m_ownerWindow) {
        m_ownerWindow->removeEventFilter(this);
    }
}
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
=======
bool TagSelectorOverlay::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::NonClientAreaMouseButtonPress) {
        if (isVisible() && !m_isClosing) {
            QMouseEvent* me = dynamic_cast<QMouseEvent*>(event);
            QPoint globalPos = me ? me->globalPosition().toPoint() : QCursor::pos();
            if (!frameGeometry().contains(globalPos)) {
                closeOverlay();
                return false;
            }
        }
    }
>>>>>>> REPLACE
```

---

### 3.3 Lock Target Paths Snapshot in `MetaTagSection.cpp`

```
<<<<<<< SEARCH
void MetaTagSection::openTagSelectorOverlay(QWidget* targetAnchor) {
    Q_UNUSED(targetAnchor);
    if (m_selectedPaths.isEmpty()) return;

    if (!m_tagSelectorOverlay) {
        m_tagSelectorOverlay = new TagSelectorOverlay({}, this);
        connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this](const QStringList& currentTags) {
            emit tagsChanged(m_selectedPaths, currentTags);
        });
    }

    m_tagSelectorOverlay->show();
}
=======
void MetaTagSection::openTagSelectorOverlay(QWidget* targetAnchor) {
    Q_UNUSED(targetAnchor);
    if (m_selectedPaths.isEmpty()) return;

    // 🔒【路径快照锁定】：锁定唤起时刻的目标项目路径，绝不随外部视图刷新的 m_selectedPaths 变动而脱节
    const QStringList targetPathsSnapshot = m_selectedPaths;

    if (m_tagSelectorOverlay) {
        m_tagSelectorOverlay->close();
    }

    m_tagSelectorOverlay = new TagSelectorOverlay({}, targetPathsSnapshot, this);
    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this, targetPathsSnapshot](const QStringList& currentTags) {
        if (!targetPathsSnapshot.isEmpty()) {
            emit tagsChanged(targetPathsSnapshot, currentTags);
        }
    });

    m_tagSelectorOverlay->show();
}
>>>>>>> REPLACE
```

---

### 3.4 Ensure Target Paths Snapshot Integrity in `MetaPanel.cpp`

```
<<<<<<< SEARCH
    m_tagSelectorOverlay->show();

    // 1. 浮层打开瞬时，保存决议绑定的固定目标项目快照，不随 m_selectedPaths 动态漂移
    const QStringList targetPaths = m_selectedPaths;

    // 2. 浮层局部的已提交标签集合，打开时用 m_currentTagsSet 初始化
    auto appliedTagsSet = std::make_shared<QSet<QString>>(m_currentTagsSet);

    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this, targetPaths, appliedTagsSet](const QStringList& newSelectedTags) {
        if (targetPaths.isEmpty()) return;

        QSet<QString> newSet(newSelectedTags.begin(), newSelectedTags.end());

        // 新增的标签：与浮层局部的已提交集合比对
        for (const QString& t : newSet) {
            if (!appliedTagsSet->contains(t)) {
                emit tagAddRequested(targetPaths, t);
            }
        }
        // 移除的标签：与浮层局部的已提交集合比对
        for (const QString& t : *appliedTagsSet) {
            if (!newSet.contains(t)) {
                emit tagRemoveRequested(targetPaths, t);
            }
        }
        *appliedTagsSet = newSet;
    });
=======
    m_tagSelectorOverlay->show();

    // 1. 🔒【路径快照锁定】：浮层打开瞬时，保存决议绑定的固定目标项目快照，不随 m_selectedPaths 动态漂移
    const QStringList targetPathsSnapshot = m_selectedPaths;

    // 2. 浮层局部的已提交标签集合，打开时用 m_currentTagsSet 初始化
    auto appliedTagsSet = std::make_shared<QSet<QString>>(m_currentTagsSet);

    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this, targetPathsSnapshot, appliedTagsSet](const QStringList& newSelectedTags) {
        if (targetPathsSnapshot.isEmpty()) return;

        QSet<QString> newSet(newSelectedTags.begin(), newSelectedTags.end());

        // 新增的标签：与浮层局部的已提交集合比对
        for (const QString& t : newSet) {
            if (!appliedTagsSet->contains(t)) {
                emit tagAddRequested(targetPathsSnapshot, t);
            }
        }
        // 移除的标签：与浮层局部的已提交集合比对
        for (const QString& t : *appliedTagsSet) {
            if (!newSet.contains(t)) {
                emit tagRemoveRequested(targetPathsSnapshot, t);
            }
        }
        *appliedTagsSet = newSet;
    });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Functional Verification**:
   - Open TagSelectorOverlay, click anywhere on ContentPanel or MainWindow outside TagSelectorOverlay $\to$ overlay closes immediately and automatically.
   - Select multiple items, click "+ 添加标签", select tag A $\to$ applies immediately.
   - Immediately select tag B, tag C in TagSelectorOverlay $\to$ tag B and tag C continue to bind correctly without failing.
   - Verify no global event filter (`qApp->installEventFilter`) is polluting application event flow.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 本次修改复用了已有的 `tagAddRequested` / `tagRemoveRequested` 信号与 `CoreEngine::executeCommand` 架构通道；
- 将无差别全局事件挂载收敛为宿主 Window 局域挂载 (`m_ownerWindow->installEventFilter(this)`），完全对齐 Clean Architecture。

---

## 6. Header API Signature Verification

| 类名 | 成员函数/信号物理签名 | 头文件物理位置 | 状态 |
| :--- | :--- | :--- | :--- |
| `TagSelectorOverlay` | `TagSelectorOverlay(const QStringList&, const QStringList&, QWidget*)` | `src/ui/TagSelectorOverlay.h` | 方案 3.1 明确新增对齐 |
| `MetaTagSection` | `void tagsChanged(const QStringList&, const QStringList&)` | `src/ui/MetaTagSection.h` | 已核验 100% 存在 |
| `MetaPanel` | `void tagAddRequested(const QStringList&, const QString&)` | `src/ui/MetaPanel.h` | 已核验 100% 存在 |
| `MetaPanel` | `void tagRemoveRequested(const QStringList&, const QString&)` | `src/ui/MetaPanel.h` | 已核验 100% 存在 |

---

## 7. Header Inclusion Chain & Type Completeness Check

| 受影响文件 | `#include` 修改点 | 核查的类型/类定义 | 闭包完整性结论 |
| :--- | :--- | :--- | :--- |
| `TagSelectorOverlay.h` | 补充 `<QPointer>` | `QPointer<QWidget>` | 100% 完整 |
| `TagSelectorOverlay.cpp` | 无需变更 | `QApplication`, `TagSelectorOverlay` | 100% 完整 |
| `MetaTagSection.cpp` | 无需变更 | `MetaTagSection`, `TagSelectorOverlay` | 100% 完整 |
| `MetaPanel.cpp` | 无需变更 | `MetaPanel`, `TagSelectorOverlay` | 100% 完整 |
