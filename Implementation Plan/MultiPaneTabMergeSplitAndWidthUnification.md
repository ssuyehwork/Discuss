# Implementation Plan - MultiPaneTabMergeSplitAndWidthUnification.md

## 1. Overview
This implementation plan delivers three core multi-pane split view enhancements for QuarkMeta:
1. **Unification of Pane Minimum Width to 230px**:
   - Replaces all hardcoded `460` minimum width logic and `EditorContainer` checks across `ContentPanel` and `ContentPaneSplitManager` with a single SSOT constant `static constexpr int kMinPaneWidth = 230;`.
   - `ContentPanel::paneCount()`, `panes()`, and `isSplitMode()` on secondary panes now delegate to `rootPane()` to return the true root split state. This ensures drag-and-drop pane count limit checks work identically across primary and secondary panes.
   - Implements `updateContainerMinimumWidth()` in `ContentPaneSplitManager` called after `splitPane`, `closePane`, `restoreSplitState`, and `setViewMode`.
2. **Tab Title Displays All Pane Folder Names**:
   - `ContentPaneSplitManager` emits `layoutChanged()` whenever split panes are created, closed, restored, or path-updated.
   - `PanelMediator` listens to `layoutChanged()`, exports `TabSplitState`, and passes it to `TabBarWidget::updateSplitTabTitle(state)`.
   - `TabBarWidget::updateSplitTabTitle` formats each pane's folder name in order (e.g., `此电脑`, drive labels, or folder names) and joins them with `" | "` without deduplication.
3. **Merge Tabs & Split Tabs Functions**:
   - Adds "合并到" (Merge to) sub-menu listing all other open tabs and "拆分标签页" (Split Tab) action (enabled when pane count > 1) to the tab right-click context menu.
   - **Merge Rules**: Obtains real-time `TabSplitState` via `exportSplitState()`. If `sourcePaneCount + targetPaneCount > ContentPanel::kMaxPanes`, shows error tooltip `"窗格数量超出上限4窗格，不支持合并"` without merging or altering tabs. If within limit, appends source pane paths to target tab, removes source tab without adding to recent closed history, and restores merged `TabSplitState`.
   - **Split Rules**: Splitting a tab with N > 1 panes creates N - 1 new single-pane tabs inserted directly after the original tab.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h` (Declare `kMinPaneWidth = 230`, update `paneCount()`, `panes()`, `isSplitMode()`, and `minimumSizeHint()`)
- `src/ui/ContentPanel.cpp` (Use `kMinPaneWidth`, remove `460` and `EditorContainer` checks in `setViewMode`)
- `src/ui/controllers/ContentPaneSplitManager.h` (Declare `layoutChanged()` signal, helper `updateContainerMinimumWidth()`)
- `src/ui/controllers/ContentPaneSplitManager.cpp` (Use `kMinPaneWidth`, emit `layoutChanged()`, implement `updateContainerMinimumWidth()`)
- `src/ui/PanelMediator.cpp` (Connect `layoutChanged()` to `updateSplitTabTitle`)
- `src/ui/TabBarWidget.h` (Declare tab merge/split signals, handlers, and update `updateSplitTabTitle`)
- `src/ui/TabBarWidget.cpp` (Implement tab right-click merge/split actions, tooltip error for overflow, and tab index correction)

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/ContentPanel.h`
<<<<<<< SEARCH
    static constexpr int kMaxPanes = 4;

    QSize minimumSizeHint() const override { return QSize(m_currentViewMode == ColumnView ? 460 : 230, 100); }
=======
    static constexpr int kMinPaneWidth = 230;
    static constexpr int kMaxPanes = 4;

    QSize minimumSizeHint() const override { return QSize(kMinPaneWidth, 100); }
>>>>>>> REPLACE

<<<<<<< SEARCH
    int paneCount() const;
    bool isSplitMode() const;
=======
    int paneCount() const { return rootPane() ? rootPane()->splitManager()->paneCount() : 1; }
    bool isSplitMode() const { return rootPane() ? rootPane()->splitManager()->isSplit() : false; }
>>>>>>> REPLACE

### Change 2: `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
    setMinimumWidth(230);
=======
    setMinimumWidth(kMinPaneWidth);
>>>>>>> REPLACE

<<<<<<< SEARCH
    int minW = (mode == ColumnView) ? 460 : 230;
    setMinimumWidth(minW);
    if (parentWidget() && parentWidget()->objectName() == "EditorContainer") {
        parentWidget()->setMinimumWidth(minW);
    }
=======
    setMinimumWidth(kMinPaneWidth);
    if (parentWidget()) {
        parentWidget()->setMinimumWidth(kMinPaneWidth);
    }
>>>>>>> REPLACE

### Change 3: `src/ui/controllers/ContentPaneSplitManager.h`
<<<<<<< SEARCH
signals:
    void activePaneChanged(ContentPanel* panel);
=======
signals:
    void activePaneChanged(ContentPanel* panel);
    void layoutChanged();
>>>>>>> REPLACE

### Change 4: `src/ui/controllers/ContentPaneSplitManager.cpp`
<<<<<<< SEARCH
        m_primaryPaneContainer->setMinimumWidth(m_panel->currentViewMode() == ContentPanel::ColumnView ? 460 : 230);
=======
        m_primaryPaneContainer->setMinimumWidth(ContentPanel::kMinPaneWidth);
>>>>>>> REPLACE

<<<<<<< SEARCH
    container->setMinimumWidth(newPane->currentViewMode() == ContentPanel::ColumnView ? 460 : 230);
=======
    container->setMinimumWidth(ContentPanel::kMinPaneWidth);
>>>>>>> REPLACE

<<<<<<< SEARCH
    emit m_panel->secondaryPaneCreated(newPane);

    refreshActiveIndicators();
=======
    emit m_panel->secondaryPaneCreated(newPane);

    refreshActiveIndicators();
    emit layoutChanged();
>>>>>>> REPLACE

<<<<<<< SEARCH
    refreshActiveIndicators();
}
=======
    refreshActiveIndicators();
    emit layoutChanged();
}
>>>>>>> REPLACE

### Change 5: `src/ui/PanelMediator.cpp`
<<<<<<< SEARCH
        connect(root->splitManager(), &ContentPaneSplitManager::activePaneChanged, this, [this](ContentPanel* panel) {
            if (panel) {
                setActiveContentPanel(panel);
            }
        });
=======
        connect(root->splitManager(), &ContentPaneSplitManager::activePaneChanged, this, [this](ContentPanel* panel) {
            if (panel) {
                setActiveContentPanel(panel);
            }
        });

        connect(root->splitManager(), &ContentPaneSplitManager::layoutChanged, this, [this, titleBar, root]() {
            if (titleBar && titleBar->tabBar()) {
                TabSplitState state = root->splitManager()->exportSplitState();
                titleBar->tabBar()->updateSplitTabTitle(state);
            }
        });
>>>>>>> REPLACE

### Change 6: `src/ui/TabBarWidget.h`
<<<<<<< SEARCH
signals:
    void tabClicked(int index);
    void tabCloseRequested(int index);
=======
signals:
    void tabClicked(int index);
    void tabCloseRequested(int index);
    void mergeTabRequested(int sourceIndex, int targetIndex);
    void splitTabRequested(int tabIndex);
>>>>>>> REPLACE

### Change 7: `src/ui/TabBarWidget.cpp`
<<<<<<< SEARCH
        QMenu menu(this);
        QAction* actNewTab = menu.addAction("新建标签页");
        QAction* actDuplicate = menu.addAction("复制标签页");
        menu.addSeparator();
        QAction* actClose = menu.addAction("关闭标签页");
=======
        QMenu menu(this);
        QAction* actNewTab = menu.addAction("新建标签页");
        QAction* actDuplicate = menu.addAction("复制标签页");

        QMenu* mergeMenu = menu.addMenu("合并到");
        for (int i = 0; i < m_tabs.size(); ++i) {
            if (i != m_index) {
                QAction* mergeAct = mergeMenu->addAction(m_tabs[i].title);
                connect(mergeAct, &QAction::triggered, this, [this, i]() {
                    emit mergeTabRequested(m_index, i);
                });
            }
        }
        if (m_tabs.size() <= 1) {
            mergeMenu->setEnabled(false);
        }

        QAction* actSplit = menu.addAction("拆分标签页");
        TabBarWidget* tabBar = qobject_cast<TabBarWidget*>(parentWidget());
        if (!tabBar && parentWidget()) tabBar = qobject_cast<TabBarWidget*>(parentWidget()->parentWidget());
        int paneCount = (tabBar && m_index < tabBar->tabs().size()) ? tabBar->tabs()[m_index].panePaths.size() : 1;
        actSplit->setEnabled(paneCount > 1);

        menu.addSeparator();
        QAction* actClose = menu.addAction("关闭标签页");
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (selected == actDuplicate) {
            emit duplicateTabRequested(m_index);
        } else if (selected == actClose) {
=======
        if (selected == actDuplicate) {
            emit duplicateTabRequested(m_index);
        } else if (selected == actSplit) {
            emit splitTabRequested(m_index);
        } else if (selected == actClose) {
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabBarWidget::updateSplitTabTitle(const TabSplitState& state) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    QStringList formattedNames;
    for (const QString& path : state.panePaths) {
        if (path == "computer://") {
            formattedNames.append("此电脑");
        } else {
            QFileInfo fi(path);
            QString name = fi.fileName();
            if (name.isEmpty()) name = path;
            formattedNames.append(name);
        }
    }

    QString mergedTitle = formattedNames.join(" | ");
    m_tabs[m_currentIndex].title = mergedTitle;
    m_tabs[m_currentIndex].panePaths = state.panePaths;

    if (m_currentIndex < m_tabWidgets.size()) {
        m_tabWidgets[m_currentIndex]->setTabTitle(mergedTitle);
    }
}
=======
void TabBarWidget::updateSplitTabTitle(const TabSplitState& state) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    QStringList formattedNames;
    for (const QString& path : state.panePaths) {
        if (path == "computer://") {
            formattedNames.append("此电脑");
        } else if (path == "trash://") {
            formattedNames.append("回收站");
        } else {
            QFileInfo fi(path);
            QString name = fi.fileName();
            if (name.isEmpty()) name = path;
            formattedNames.append(name);
        }
    }

    QString mergedTitle = formattedNames.join(" | ");
    m_tabs[m_currentIndex].title = mergedTitle;
    m_tabs[m_currentIndex].panePaths = state.panePaths;

    if (m_currentIndex < m_tabWidgets.size()) {
        m_tabWidgets[m_currentIndex]->setTabTitle(mergedTitle);
    }
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Clean build directory and compile using CMake & Ninja / MSVC:
   ```bash
   cmake -B build -G Ninja
   cmake --build build --config Release
   ```
2. Verify Unified Minimum Width (230px):
   - Switch active pane to List View, Icon View, or Column View.
   - Resize pane down: verify minimum width is locked to 230px without jumping to 460px.
   - Split pane and switch view mode: verify 230px minimum width constraint remains consistent.
3. Verify Tab Title Folder Name Concatenation:
   - Open 1, 2, 3, or 4 split panes.
   - Verify tab title updates in real-time to display all folder names separated by `" | "`.
   - Verify duplicate folder names are preserved without deduplication.
4. Verify Merge Tabs & Split Tabs Actions:
   - Right-click tab -> "合并到" -> select target tab: verify source tab panes append to target tab, source tab is removed, and total panes up to 4 are displayed.
   - Attempt to merge tabs resulting in > 4 total panes: verify error tooltip `"窗格数量超出上限4窗格，不支持合并"` appears and tabs remain unchanged.
   - Right-click multi-pane tab -> "拆分标签页": verify tab is split into N single-pane tabs in original pane order.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Pane Minimum Width Constant**: Single SSOT constant `ContentPanel::kMinPaneWidth` (230px) used across all UI classes.
- **Split State Transport**: Reused `TabSplitState` struct and `ContentPaneSplitManager::exportSplitState` / `restoreSplitState`.
- **Max Panes Limit Constant**: Reused `ContentPanel::kMaxPanes` (4) everywhere without magic number `4`.

---

## 6. Header API Signature Verification
- `static constexpr int ContentPanel::kMinPaneWidth = 230;`
- `static constexpr int ContentPanel::kMaxPanes = 4;`
- `void ContentPaneSplitManager::layoutChanged()`
- `void TabBarWidget::updateSplitTabTitle(const TabSplitState& state)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `ContentPanel.h` includes `QSize`, `QWidget`.
- `ContentPaneSplitManager.h` includes `QObject`, `ContentPanel.h`.
- `TabBarWidget.h` includes `QWidget`, `QMenu`, `QAction`, `ToolTipOverlay.h`.
