# MultiPaneAndColumnViewFix Implementation Plan

## 1. Overview
本方案旨在解决多窗格（双窗格/多分栏）下出现的两处关键 Bug：
1. **列视图最后一列（刻意留白列）小于 230px 被挤压问题**：在分栏模式下，当窗格处于列视图（`ColumnView`）时，窗格最小宽度未进行动态约束，导致 `QSplitter` 挤压窗格至低于 460px，从而破坏了“1数据列(230px) + 最右侧刻意留白画布(230px)”的物理刚性红线。本方案为 `ContentPanel` 引入基于视图模式的动态最小宽度约束（列视图下锁定下限 460px），确保留白列绝对不低于 230px。
2. **右键菜单“关闭窗格”与顶部“双窗格分栏视图”按钮形同虚设问题**：
   - 顶部 `ContentHeaderWidget::splitViewRequested` 信号在 `ContentPanel` 中漏连，导致点击“双窗格分栏视图”按钮无响应。本方案补齐该信号与 `splitPane` 的槽函数连接；
   - 右键菜单“关闭窗格”在主窗格上触发时因信号未连接及关闭机制缺失而失灵。本方案在 `ContentPanel` 构造函数中将 `closePaneRequested` 统一绑定至 `closePane`，并在 `ContentPaneSplitManager::closePane` 中补齐主窗格关闭时自动由首个副窗格接管替换的关闭路由。

---

## 2. Modified Files List
1. `src/ui/ContentPanel.h`
2. `src/ui/ContentPanel.cpp`
3. `src/ui/controllers/ContentPaneSplitManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Changes to `src/ui/ContentPanel.h`

<<<<<<< SEARCH
    QSize minimumSizeHint() const override { return QSize(230, 100); }
=======
    QSize minimumSizeHint() const override { return QSize(m_currentViewMode == ColumnView ? 460 : 230, 100); }
>>>>>>> REPLACE

### 3.2 Changes to `src/ui/ContentPanel.cpp`

<<<<<<< SEARCH
    m_keyHandler = new ContentKeyHandler(this);

    initUi();
=======
    m_keyHandler = new ContentKeyHandler(this);

    connect(this, &ContentPanel::closePaneRequested, this, [this]() {
        closePane(this);
    });

    initUi();
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_headerWidget = new ContentHeaderWidget(this);
    m_headerWidget->setFilterState(m_currentFilter);

    connect(m_headerWidget, &ContentHeaderWidget::filterStateChanged, this, [this](const FilterState& state) {
=======
    m_headerWidget = new ContentHeaderWidget(this);
    m_headerWidget->setFilterState(m_currentFilter);

    connect(m_headerWidget, &ContentHeaderWidget::splitViewRequested, this, [this]() {
        if (isSplitMode()) {
            closeSecondaryPane();
        } else {
            splitPane(Qt::Horizontal);
        }
    });

    connect(m_headerWidget, &ContentHeaderWidget::filterStateChanged, this, [this](const FilterState& state) {
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_diskModel || m_diskModel->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }

    // 2. 消费 SelectionState 真理源同步恢复选区
    restoreSelections();
=======
    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_diskModel || m_diskModel->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }

    // 更新窗格及容器的动态最小宽度约束（列视图锁定 460px 保证 1列数据230px + 1列留白230px）
    int minW = (mode == ColumnView) ? 460 : 230;
    setMinimumWidth(minW);
    if (parentWidget() && parentWidget()->objectName() == "EditorContainer") {
        parentWidget()->setMinimumWidth(minW);
    }

    // 2. 消费 SelectionState 真理源同步恢复选区
    restoreSelections();
>>>>>>> REPLACE

### 3.3 Changes to `src/ui/controllers/ContentPaneSplitManager.cpp`

<<<<<<< SEARCH
        m_primaryPaneContainer->setMinimumWidth(230);
=======
        m_primaryPaneContainer->setMinimumWidth(m_panel->currentViewMode() == ContentPanel::ColumnView ? 460 : 230);
>>>>>>> REPLACE

<<<<<<< SEARCH
    QWidget* container = new QWidget(m_paneSplitter);
    container->setMinimumWidth(230);
=======
    QWidget* container = new QWidget(m_paneSplitter);
    container->setMinimumWidth(newPane->currentViewMode() == ContentPanel::ColumnView ? 460 : 230);
>>>>>>> REPLACE

<<<<<<< SEARCH
    int idx = m_panes.indexOf(pane);
    if (idx < 0) return;
=======
    int idx = m_panes.indexOf(pane);
    if (idx < 0) {
        // 如果请求关闭的是主窗格（rootPane / m_panel），且存在副窗格，则由首个副窗格接管主窗格内容并销毁副窗格
        if (pane == m_panel && !m_panes.isEmpty()) {
            ContentPanel* firstSecondary = m_panes.first();
            QString secondaryPath = firstSecondary->currentPath();
            m_panel->setViewMode(firstSecondary->currentViewMode());
            m_panel->loadDirectory(secondaryPath, firstSecondary->isRecursive());
            closePane(firstSecondary);
        }
        return;
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### 4.1 编译指令
使用 MSVC/CMake 在 Windows 环境下构建项目：
```cmd
cmake --build build --config Release
```

### 4.2 功能验证步骤
1. **验证双窗格分栏按钮**：
   - 启动应用，在内容面板顶部工具栏点击“双窗格分栏视图”按钮；
   - 确认能够正常创建并切出水平双分栏视图。再次点击确认能成功合并/关闭分栏。
2. **验证右键菜单“关闭窗格”**：
   - 开启分栏后，在主窗格（左侧）空白处右键点击“关闭窗格”，确认主窗格成功接管右侧路径并平滑关闭分栏；
   - 在副窗格（右侧）空白处右键点击“关闭窗格”，确认右侧副窗格平滑关闭。
3. **验证列视图留白列 230px 物理刚性下限**：
   - 在拖拽标签页或点击按钮创建双窗格后，将视图切换至“列视图”；
   - 尝试拖拽分栏 `QSplitter` 句柄拉窄窗格，确认窗格受到 460px 最小尺寸限制，无法被压缩到 460px 以下，最右侧刻意留白列严格保持 ≥230px 物理空间，不被挤扣偷窃。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API 复用**：复用了既有的 `m_splitManager->splitPane`、`closePane`、`closeSecondaryPane` 和 `setViewMode` 官方 API，未另起炉灶建立并行关闭/分栏管线。
- **死代码清理**：无冗余独立分支，去除了旧有未绑定的隐患逻辑。

---

## 6. Header API Signature Verification
1. `ContentHeaderWidget::splitViewRequested()`：`void splitViewRequested()`
2. `ContentPanel::closePaneRequested()`：`void closePaneRequested()`
3. `ContentPanel::closePane(ContentPanel* pane)`：`void closePane(ContentPanel* pane)`
4. `ContentPanel::splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString())`
5. `ContentPanel::currentViewMode()`：`ViewMode currentViewMode() const`

---

## 7. Header Inclusion Chain & Type Completeness Check
- 修改文件均使用已有类与信号/槽机制，未新增或删除 `#include` 头文件，不影响现有编译包含链。
