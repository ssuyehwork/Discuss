# Implementation Plan - ContentPaneSplitManager-7.md

## 1. Overview
This implementation plan addresses critical multi-pane split view defects identified in QuarkMeta:
1. **Multi-Pane List Mode Column Header Loss Fix**: `SectionProxyModel` wraps `FilterProxyModel` and `DiskItemModel` in `ContentPanel` list view. Because `SectionProxyModel` did not override `headerData(...)`, Qt fell back to `QAbstractProxyModel::headerData`, returning invalid header text (`0` / empty) in list view mode. Implementing `headerData` proxy forwarding in `SectionProxyModel` restores full column header titles ("名称", "状态", "评分", "尺寸", "类型", "大小", "修改日期").
2. **Active Pane Focus Desync Fix**: `PanelMediator` installed `PaneActivationTracker` to catch pane click interactions, but failed to assign `m_activeContentPanel = panel;` or emit `activeContentPanelChanged(panel)`. Updating `m_activeContentPanel` and broadcasting `activeContentPanelChanged` ensures status bar controls and peripheral widgets track the focused active pane.
3. **Cross-Pane Drag & Drop In-Place Refresh Synchronization**: Connects cross-pane refresh logic through SSOT `ContentPanel::refreshAll()` on both source and destination panels upon completing file move/copy operations.

---

## 2. Modified Files List
- `src/ui/models/SectionProxyModel.h` (Override `headerData` method)
- `src/ui/models/SectionProxyModel.cpp` (Implement `headerData` proxy forwarding)
- `src/ui/PanelMediator.cpp` (Update `m_activeContentPanel` and emit `activeContentPanelChanged` on pane interaction)
- `src/ui/controllers/ContentPaneSplitManager.cpp` (Wire cross-pane update synchronization to SSOT `refreshAll()`)

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/models/SectionProxyModel.h`
<<<<<<< SEARCH
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
=======
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
>>>>>>> REPLACE

### Change 2: `src/ui/models/SectionProxyModel.cpp`
<<<<<<< SEARCH
int SectionProxyModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return sourceModel() ? sourceModel()->columnCount() : 0;
}

QVariant SectionProxyModel::data(const QModelIndex& index, int role) const {
=======
int SectionProxyModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return sourceModel() ? sourceModel()->columnCount() : 0;
}

QVariant SectionProxyModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (sourceModel()) {
        return sourceModel()->headerData(section, orientation, role);
    }
    return QAbstractProxyModel::headerData(section, orientation, role);
}

QVariant SectionProxyModel::data(const QModelIndex& index, int role) const {
>>>>>>> REPLACE

### Change 3: `src/ui/PanelMediator.cpp`
<<<<<<< SEARCH
    // 安装全应用窗格激活事件过滤器
    if (!m_activationTracker) {
        m_activationTracker = new PaneActivationTracker(this);
        qApp->installEventFilter(m_activationTracker);

        connect(m_activationTracker, &PaneActivationTracker::paneInteracted, this, [this](ContentPanel* panel) {
            if (panel && m_activeContentPanel != panel) {
                panel->setActivePane(true);
            }
        });
    }
=======
    // 安装全应用窗格激活事件过滤器
    if (!m_activationTracker) {
        m_activationTracker = new PaneActivationTracker(this);
        qApp->installEventFilter(m_activationTracker);

        connect(m_activationTracker, &PaneActivationTracker::paneInteracted, this, [this](ContentPanel* panel) {
            if (panel && m_activeContentPanel != panel) {
                if (m_activeContentPanel) {
                    m_activeContentPanel->setActivePane(false);
                }
                m_activeContentPanel = panel;
                m_activeContentPanel->setActivePane(true);
                emit activeContentPanelChanged(m_activeContentPanel);
            }
        });
    }
>>>>>>> REPLACE

### Change 4: `src/ui/controllers/ContentPaneSplitManager.cpp`
<<<<<<< SEARCH
void ContentPaneSplitManager::splitActivePane(Qt::Orientation orientation) {
    if (!m_mediator) return;
    ContentPanel* activePanel = m_mediator->activeContentPanel();
    if (!activePanel) return;

    if (m_panes.size() >= kMaxPaneCount) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已达到最大分栏数量限制", 1500, QColor("#378ADD"));
        return;
    }

    ContentPanel* newPanel = createSecondaryPane(activePanel->currentPath());
    if (!newPanel) return;

    // 绑定信号到 Mediator
    m_mediator->bindPanelEvents(newPanel);
=======
void ContentPaneSplitManager::splitActivePane(Qt::Orientation orientation) {
    if (!m_mediator) return;
    ContentPanel* activePanel = m_mediator->activeContentPanel();
    if (!activePanel) return;

    if (m_panes.size() >= kMaxPaneCount) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已达到最大分栏数量限制", 1500, QColor("#378ADD"));
        return;
    }

    ContentPanel* newPanel = createSecondaryPane(activePanel->currentPath());
    if (!newPanel) return;

    // 绑定信号到 Mediator
    m_mediator->bindPanelEvents(newPanel);

    // 双向广播数据变动/拖放刷新：确保跨分栏操作后两端均调用 ContentPanel::refreshAll()
    QPointer<ContentPanel> srcPanel(activePanel);
    QPointer<ContentPanel> dstPanel(newPanel);
    QObject::connect(newPanel, &ContentPanel::directorySelected, [srcPanel, dstPanel](const QString&) {
        if (srcPanel) srcPanel->refreshAll();
        if (dstPanel) dstPanel->refreshAll();
    });
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Clean build directory and compile using CMake & Ninja / MSVC:
   ```bash
   cmake -B build -G Ninja
   cmake --build build --config Release
   ```
2. Verify list view header rendering:
   - Toggle multi-pane split view mode.
   - Switch active pane view mode to List View.
   - Verify column headers ("名称", "评分", "尺寸", "类型", "大小", "修改日期") render with correct titles instead of `0`.
3. Verify pane activation:
   - Click on primary vs secondary split panes.
   - Verify status bar view mode buttons update to reflect active pane state.
4. Verify cross-pane drag-and-drop:
   - Drag files from secondary pane to primary pane folder.
   - Verify both panes execute in-place refresh (`refreshAll()`) without view layout state resetting.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **View Refresh Standard Channel**: Reused `ContentPanel::refreshAll()` as the mandatory SSOT entry point for post-drop and view synchronization. No local manual model reloading (`loadDirectory`) was introduced.
- **Active Pane Tracking**: Reused existing `PanelMediator::activeContentPanelChanged` signal without duplicating state mechanisms.

---

## 6. Header API Signature Verification
- `QVariant SectionProxyModel::headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override`
- `void PanelMediator::activeContentPanelChanged(ContentPanel* panel)`
- `void ContentPanel::refreshAll()`
- `void ContentPanel::setActivePane(bool active)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `SectionProxyModel.h` inherits `QAbstractProxyModel`, which provides `headerData` method declaration.
- `PanelMediator.cpp` includes `PaneActivationTracker.h` and `ContentPanel.h`.
- `ContentPaneSplitManager.cpp` includes `ContentPanel.h` and `QPointer`.
