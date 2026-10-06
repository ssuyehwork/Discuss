# Implementation Plan - ContentPaneSplitManager-8.md

## 1. Overview
This implementation plan addresses cross-pane drag-and-drop refresh and multi-pane split view defects in QuarkMeta:
1. **Simultaneous Dual-Pane Drop Refresh Fix**: When dragging items between split panes (Primary -> Secondary OR Secondary -> Primary), `ContentFileOpsHandler` previously only invoked `refreshAll()` on the destination panel (`m_panel`). By retrieving all active split panes associated with `m_panel->panes()` and calling `pane->refreshAll()` on each panel upon `DiskIoService` completion, both source and target panels (and any additional split panes) refresh simultaneously in place upon releasing the left mouse button and completing the file operation.
2. **Multi-Pane List Mode Column Header Title Loss Fix**: `SectionProxyModel` wraps proxy and disk models in `ContentPanel`. Overriding `headerData(...)` in `SectionProxyModel` to proxy requests to `sourceModel()->headerData(...)` restores column header titles ("名称", "状态", "评分", "尺寸", "类型", "大小", "修改日期") in list view mode.
3. **Active Pane Focus Routing Fix**: `PanelMediator`'s `PaneActivationTracker::paneInteracted` handler now updates `m_activeContentPanel` and broadcasts `emit activeContentPanelChanged(m_activeContentPanel)`, ensuring status bar controls and peripheral widgets track the currently active split pane.

---

## 2. Modified Files List
- `src/ui/controllers/ContentFileOpsHandler.cpp` (Refresh all split panes in `weakPanel->panes()` upon drop operation completion)
- `src/ui/models/SectionProxyModel.h` (Declare `headerData` method override)
- `src/ui/models/SectionProxyModel.cpp` (Implement `headerData` proxy forwarding to `sourceModel()`)
- `src/ui/PanelMediator.cpp` (Update `m_activeContentPanel` and emit `activeContentPanelChanged` on pane interaction)

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/controllers/ContentFileOpsHandler.cpp`
<<<<<<< SEARCH
    QPointer<ContentPanel> weakPanel(m_panel);
    DiskIoService::instance().executeAsync(ioCtx, [weakPanel](bool success) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, success]() {
            if (weakPanel && success) {
                weakPanel->refreshAll();
            }
        });
    });
=======
    QPointer<ContentPanel> weakPanel(m_panel);
    DiskIoService::instance().executeAsync(ioCtx, [weakPanel](bool success) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, success]() {
            if (weakPanel && success) {
                const auto allPanes = weakPanel->panes();
                for (ContentPanel* pane : allPanes) {
                    if (pane) {
                        pane->refreshAll();
                    }
                }
            }
        });
    });
>>>>>>> REPLACE

### Change 2: `src/ui/models/SectionProxyModel.h`
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

### Change 3: `src/ui/models/SectionProxyModel.cpp`
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

### Change 4: `src/ui/PanelMediator.cpp`
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

---

## 4. Build & Verification Steps
1. Clean build directory and compile using CMake & Ninja / MSVC:
   ```bash
   cmake -B build -G Ninja
   cmake --build build --config Release
   ```
2. Verify cross-pane drop simultaneous refresh:
   - Open split pane view (Primary & Secondary panes).
   - Drag item from primary pane to secondary pane or from secondary pane to primary pane.
   - Upon releasing left mouse button and completing disk operation, verify both primary and secondary panes invoke `refreshAll()` simultaneously.
3. Verify list view header rendering:
   - Switch active pane to List View mode.
   - Verify column headers ("名称", "评分", "尺寸", "类型", "大小", "修改日期") display correct labels instead of `0`.
4. Verify active pane focus state:
   - Click back and forth between primary and secondary panes.
   - Verify `activeContentPanelChanged` updates status bar controls and active selection state cleanly.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **View Refresh Standard Channel**: Reused `ContentPanel::refreshAll()` as the mandatory SSOT entry point for post-drop and view synchronization. Iterating over `weakPanel->panes()` ensures all active split panes refresh via `refreshAll()` without manually re-triggering `loadDirectory`.
- **Active Pane Tracking**: Reused existing `PanelMediator::activeContentPanelChanged` signal and `m_activeContentPanel` without creating parallel state trackers.

---

## 6. Header API Signature Verification
- `QList<ContentPanel*> ContentPanel::panes() const`
- `void ContentPanel::refreshAll()`
- `QVariant SectionProxyModel::headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override`
- `void PanelMediator::activeContentPanelChanged(ContentPanel* panel)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `ContentFileOpsHandler.cpp` includes `ContentPanel.h`, which provides the `panes()` method and `refreshAll()` method signatures.
- `SectionProxyModel.h` inherits `QAbstractProxyModel`, providing `headerData` declaration.
- `PanelMediator.cpp` includes `ContentPanel.h` and `PaneActivationTracker.h`.
