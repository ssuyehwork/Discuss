# Implementation Plan: ContentPanel-18.md (Dual Pane Independent Browsing & MetaPanel Unified Selection)

## 1. Overview
This implementation plan establishes a functional dual-pane split browsing mechanism inside `ContentPanel` and wires selection signals from both the primary pane and the newly instantiated secondary `ContentPanel` (`m_secondaryContentPanel`) to the unified right-hand `MetaPanel` via `PanelMediator`.

When in split mode:
1. `ContentPanel::splitPane(...)` instantiates an actual child `ContentPanel` inside `m_secondaryPaneContainer` and loads `secondaryPath` (defaulting to `"computer://"` if unprovided).
2. `ContentPanel` emits `secondaryPaneCreated(ContentPanel* pane)` on creation and `secondaryPaneClosed()` on teardown.
3. `PanelMediator` binds `selectionChanged` from both primary and secondary panels to `MetaPanel` so selecting items in either pane updates metadata in real time.

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/PanelMediator.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.h`
Declare secondary pane lifecycle signals and child `ContentPanel` pointer.

```
<<<<<<< SEARCH
signals:
    void zoomLevelChanged(int level);
=======
signals:
    void secondaryPaneCreated(ContentPanel* pane);
    void secondaryPaneClosed();
    void zoomLevelChanged(int level);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QSplitter* m_paneSplitter = nullptr;
    QWidget* m_secondaryPaneContainer = nullptr;
    QWidget* m_dragOverlayWidget = nullptr;
=======
    QSplitter* m_paneSplitter = nullptr;
    QWidget* m_secondaryPaneContainer = nullptr;
    ContentPanel* m_secondaryContentPanel = nullptr;
    QWidget* m_dragOverlayWidget = nullptr;
>>>>>>> REPLACE
```

### 3.2 `src/ui/ContentPanel.cpp`
Instantiate `m_secondaryContentPanel`, emit creation/closure signals, and load target directory into the secondary pane.

```
<<<<<<< SEARCH
void ContentPanel::splitPane(Qt::Orientation orientation, const QString& secondaryPath) {
    if (m_isSplit && m_splitOrientation == orientation) return;

    m_splitOrientation = orientation;
    m_isSplit = true;

    if (!m_paneSplitter) {
        m_paneSplitter = new QSplitter(m_splitOrientation, this);
        m_paneSplitter->setHandleWidth(2);
        m_mainLayout->removeWidget(m_viewStack);
        m_paneSplitter->addWidget(m_viewStack);

        m_secondaryPaneContainer = new QWidget(m_paneSplitter);
        QVBoxLayout* secLayout = new QVBoxLayout(m_secondaryPaneContainer);
        secLayout->setContentsMargins(0, 0, 0, 0);

        m_paneSplitter->addWidget(m_secondaryPaneContainer);
        m_mainLayout->addWidget(m_paneSplitter, 1);
    } else {
        m_paneSplitter->setOrientation(m_splitOrientation);
        if (m_secondaryPaneContainer) {
            m_secondaryPaneContainer->show();
        }
    }

    QList<int> sizes;
    int total = (orientation == Qt::Horizontal) ? width() : height();
    sizes << total / 2 << total / 2;
    m_paneSplitter->setSizes(sizes);
}
=======
void ContentPanel::splitPane(Qt::Orientation orientation, const QString& secondaryPath) {
    if (m_isSplit && m_splitOrientation == orientation) {
        if (m_secondaryContentPanel && !secondaryPath.isEmpty()) {
            m_secondaryContentPanel->loadDirectory(secondaryPath);
        }
        return;
    }

    m_splitOrientation = orientation;
    m_isSplit = true;

    if (!m_paneSplitter) {
        m_paneSplitter = new QSplitter(m_splitOrientation, this);
        m_paneSplitter->setHandleWidth(2);
        m_mainLayout->removeWidget(m_viewStack);
        m_paneSplitter->addWidget(m_viewStack);

        m_secondaryPaneContainer = new QWidget(m_paneSplitter);
        QVBoxLayout* secLayout = new QVBoxLayout(m_secondaryPaneContainer);
        secLayout->setContentsMargins(0, 0, 0, 0);

        m_secondaryContentPanel = new ContentPanel(m_secondaryPaneContainer);
        secLayout->addWidget(m_secondaryContentPanel);

        m_paneSplitter->addWidget(m_secondaryPaneContainer);
        m_mainLayout->addWidget(m_paneSplitter, 1);

        emit secondaryPaneCreated(m_secondaryContentPanel);
    } else {
        m_paneSplitter->setOrientation(m_splitOrientation);
        if (m_secondaryPaneContainer) {
            m_secondaryPaneContainer->show();
        }
    }

    if (m_secondaryContentPanel) {
        m_secondaryContentPanel->loadDirectory(!secondaryPath.isEmpty() ? secondaryPath : "computer://");
    }

    QList<int> sizes;
    int total = (orientation == Qt::Horizontal) ? width() : height();
    sizes << total / 2 << total / 2;
    m_paneSplitter->setSizes(sizes);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::closeSecondaryPane() {
    if (!m_isSplit) return;

    m_isSplit = false;
    if (m_secondaryPaneContainer) {
        m_secondaryPaneContainer->hide();
    }
}
=======
void ContentPanel::closeSecondaryPane() {
    if (!m_isSplit) return;

    m_isSplit = false;
    if (m_secondaryPaneContainer) {
        m_secondaryPaneContainer->hide();
    }
    emit secondaryPaneClosed();
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/PanelMediator.cpp`
Wire selection changed signals from both primary and secondary content panels to the shared `MetaPanel`.

```
<<<<<<< SEARCH
        connect(contentPanel, &ContentPanel::selectionChanged, metaPanel, [contentPanel, metaPanel](const QStringList& paths) {
            if (paths.isEmpty()) {
                metaPanel->clearSelection();
                return;
            }
            if (paths.size() == 1) {
                metaPanel->inspectPath(paths.first());
            } else {
                metaPanel->inspectMultiplePaths(paths);
            }
        });
=======
        auto wireSelectionToMeta = [metaPanel](ContentPanel* panel) {
            connect(panel, &ContentPanel::selectionChanged, metaPanel, [panel, metaPanel](const QStringList& paths) {
                if (paths.isEmpty()) {
                    metaPanel->clearSelection();
                    return;
                }
                if (paths.size() == 1) {
                    metaPanel->inspectPath(paths.first());
                } else {
                    metaPanel->inspectMultiplePaths(paths);
                }
            });
        };

        wireSelectionToMeta(contentPanel);
        connect(contentPanel, &ContentPanel::secondaryPaneCreated, this, [wireSelectionToMeta](ContentPanel* pane) {
            wireSelectionToMeta(pane);
        });
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Run `cmake --build build` or `ninja -C build` to ensure clean C++ compilation without symbol or syntax errors.
2. Trigger pane split by dragging a tab or invoking split commands on `ContentPanel`.
3. Verify that `m_secondaryContentPanel` renders a fully functional file browser view with a visible `QSplitter` handle.
4. Select items in the left pane -> verify `MetaPanel` updates with selected file metadata.
5. Select items in the right pane -> verify `MetaPanel` updates with selected file metadata.
6. Trigger "关闭窗格" -> verify secondary pane hides and `secondaryPaneClosed()` signal emits cleanly.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **View Navigation SSOT**: Child panel navigation strictly uses `ContentPanel::loadDirectory(...)`.
- **Selection Dispatch SSOT**: Both panes reuse the standard `selectionChanged(QStringList)` signal and `PanelMediator` binding logic.

## 6. Header API Signature Verification
- `ContentPanel::secondaryPaneCreated(ContentPanel*)` -> Declared in `src/ui/ContentPanel.h`.
- `ContentPanel::secondaryPaneClosed()` -> Declared in `src/ui/ContentPanel.h`.
- `ContentPanel::loadDirectory(const QString&, bool)` -> Verified existing in `src/ui/ContentPanel.h`.
- `MetaPanel::inspectPath(const QString&)` -> Verified existing in `src/ui/MetaPanel.h`.
- `MetaPanel::inspectMultiplePaths(const QStringList&)` -> Verified existing in `src/ui/MetaPanel.h`.
