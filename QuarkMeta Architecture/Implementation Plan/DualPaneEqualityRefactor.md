# Dual Pane Equality Refactor Implementation Plan

## Overview
This plan refactors the dual-pane (split view) architecture in `ContentPanel` and `PanelMediator` to treat primary and secondary split panes as equal first-class citizens:
1. **Focus Visual Indication**: Adds dynamic active pane border/header visual highlighting (`activePane="true"` vs `activePane="false"`) so users can immediately distinguish which split pane currently holds user focus.
2. **Active Pane Routing for Navigation**: Modifies navigation callback routing in `PanelMediator` to route directory navigation from `NavPanel` (目录导航) and `FavoritePanel` (收藏栏) to `m_activeContentPanel` instead of hardcoding the primary pane.
3. **Synchronized View Mode Transitions**: Ensures view mode toggle actions (e.g., Grid, List, Column, Justified) emitted from titlebar/statusbar view buttons apply simultaneously to both active and peer split panes.

## Modified Files List
1. `src/ui/ContentPanel.h`
2. `src/ui/ContentPanel.cpp`
3. `src/ui/PanelMediator.cpp`
4. `resources/style.qss`

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`
Add `setActivePane(bool active)` method and active pane state tracking:

```
<<<<<<< SEARCH
    ContentPanel* secondaryContentPanel() const { return m_secondaryContentPanel; }
=======
    ContentPanel* secondaryContentPanel() const { return m_secondaryContentPanel; }
    void setActivePane(bool active);
    bool isActivePane() const { return m_isActivePane; }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    bool m_isSecondaryPane = false;
=======
    bool m_isSecondaryPane = false;
    bool m_isActivePane = true;
>>>>>>> REPLACE
```

### 2. `src/ui/ContentPanel.cpp`
Implement `setActivePane` to update active border properties on primary container (`m_primaryPaneContainer`) and secondary pane (`m_secondaryContentPanel`), and ensure split initialization synchronizes view mode:

```
<<<<<<< SEARCH
        m_paneSplitter->addWidget(m_secondaryPaneContainer);
        m_mainLayout->addWidget(m_paneSplitter, 1);

        connect(m_secondaryContentPanel, &ContentPanel::directorySelected, this, [this](const QString& path) {
=======
        m_paneSplitter->addWidget(m_secondaryPaneContainer);
        m_mainLayout->addWidget(m_paneSplitter, 1);

        // Sync initial view mode to secondary pane
        m_secondaryContentPanel->setViewMode(m_currentViewMode);

        connect(m_secondaryContentPanel, &ContentPanel::directorySelected, this, [this](const QString& path) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::closeSecondaryPane() {
=======
void ContentPanel::setActivePane(bool active) {
    if (m_isActivePane == active) return;
    m_isActivePane = active;

    if (m_isSplit && m_primaryPaneContainer) {
        m_primaryPaneContainer->setProperty("activePane", active ? "true" : "false");
        m_primaryPaneContainer->style()->unpolish(m_primaryPaneContainer);
        m_primaryPaneContainer->style()->polish(m_primaryPaneContainer);
    } else {
        setProperty("activePane", active ? "true" : "false");
        style()->unpolish(this);
        style()->polish(this);
    }

    if (m_headerWidget) {
        m_headerWidget->setActive(active);
    }
}

void ContentPanel::closeSecondaryPane() {
>>>>>>> REPLACE
```

### 3. `src/ui/PanelMediator.cpp`
Route navigation events to `m_activeContentPanel` and synchronize view mode transitions across both panes:

```
<<<<<<< SEARCH
            connect(titleBar, &TitleBarWidget::viewModeRequested, contentPanel, [contentPanel](TitleBarWidget::ViewModeOption option) {
                ContentPanel::ViewMode targetMode = ContentPanel::GridView;
                if (option == TitleBarWidget::JustifiedViewMode) targetMode = ContentPanel::JustifiedViewMode;
                else if (option == TitleBarWidget::GridViewMode) targetMode = ContentPanel::GridView;
                else if (option == TitleBarWidget::ListViewMode) targetMode = ContentPanel::ListView;
                else if (option == TitleBarWidget::ColumnViewMode) targetMode = ContentPanel::ColumnView;

                contentPanel->setViewMode(targetMode);
            });
=======
            connect(titleBar, &TitleBarWidget::viewModeRequested, contentPanel, [contentPanel](TitleBarWidget::ViewModeOption option) {
                ContentPanel::ViewMode targetMode = ContentPanel::GridView;
                if (option == TitleBarWidget::JustifiedViewMode) targetMode = ContentPanel::JustifiedViewMode;
                else if (option == TitleBarWidget::GridViewMode) targetMode = ContentPanel::GridView;
                else if (option == TitleBarWidget::ListViewMode) targetMode = ContentPanel::ListView;
                else if (option == TitleBarWidget::ColumnViewMode) targetMode = ContentPanel::ColumnView;

                contentPanel->setViewMode(targetMode);
                if (contentPanel->isSplitMode() && contentPanel->secondaryContentPanel()) {
                    contentPanel->secondaryContentPanel()->setViewMode(targetMode);
                }
            });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 1. 路径变更与导航驱动
    connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, this,
            [contentPanel, addressBar, navPanel, filterPanel, searchController](const QString& url, const QString& displayPath) {
        if (searchController && searchController->searchEdit()) {
            searchController->searchEdit()->blockSignals(true);
            searchController->searchEdit()->clear();
            searchController->searchEdit()->blockSignals(false);
        }
        if (contentPanel) {
            contentPanel->search("");
        }
        if (filterPanel) {
            filterPanel->clearAllFilters(false);
        }
        if (addressBar) addressBar->setPath(displayPath);
        if (navPanel) navPanel->selectPath(url == "computer://" ? "" : url);

        if (contentPanel) {
            if (url == "computer://") {
                contentPanel->loadDirectory("computer://");
            } else if (url == "trash://") {
                contentPanel->loadCategory("trash");
            } else {
                contentPanel->loadDirectory(url);
            }
        }
    });
=======
    // 1. 路径变更与导航驱动
    connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, this,
            [this, contentPanel, addressBar, navPanel, filterPanel, searchController](const QString& url, const QString& displayPath) {
        if (searchController && searchController->searchEdit()) {
            searchController->searchEdit()->blockSignals(true);
            searchController->searchEdit()->clear();
            searchController->searchEdit()->blockSignals(false);
        }
        ContentPanel* targetPanel = (m_activeContentPanel && m_activeContentPanel->isVisible()) ? m_activeContentPanel : contentPanel;
        if (targetPanel) {
            targetPanel->search("");
        }
        if (filterPanel) {
            filterPanel->clearAllFilters(false);
        }
        if (addressBar) addressBar->setPath(displayPath);
        if (navPanel) navPanel->selectPath(url == "computer://" ? "" : url);

        if (targetPanel) {
            if (url == "computer://") {
                targetPanel->loadDirectory("computer://");
            } else if (url == "trash://") {
                targetPanel->loadCategory("trash");
            } else {
                targetPanel->loadDirectory(url);
            }
        }
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        auto bindPanelActivation = [this, addressBar, filterPanel](ContentPanel* panel) {
            if (!panel) return;
            connect(panel, &ContentPanel::panelActivated, this, [this, panel, addressBar, filterPanel](ContentPanel* activePanel) {
                m_activeContentPanel = activePanel;
                if (addressBar) {
                    addressBar->setPath(activePanel->currentPath());
                }
            });
        };
=======
        auto bindPanelActivation = [this, contentPanel, addressBar, filterPanel](ContentPanel* panel) {
            if (!panel) return;
            connect(panel, &ContentPanel::panelActivated, this, [this, contentPanel, panel, addressBar, filterPanel](ContentPanel* activePanel) {
                m_activeContentPanel = activePanel;
                if (addressBar) {
                    addressBar->setPath(activePanel->currentPath());
                }
                if (contentPanel && contentPanel->isSplitMode()) {
                    contentPanel->setActivePane(activePanel == contentPanel);
                    if (contentPanel->secondaryContentPanel()) {
                        contentPanel->secondaryContentPanel()->setActivePane(activePanel == contentPanel->secondaryContentPanel());
                    }
                }
            });
        };
>>>>>>> REPLACE
```

### 4. `resources/style.qss`
Add styling for active pane highlight borders:

```
<<<<<<< SEARCH
#EditorContainer {
    background-color: #1e1e1e;
    border: 1px solid #333333;
    border-radius: 0px;
    color: #EEEEEE;
    margin: 0px;
    padding: 0px;
}
=======
#EditorContainer {
    background-color: #1e1e1e;
    border: 1px solid #333333;
    border-radius: 0px;
    color: #EEEEEE;
    margin: 0px;
    padding: 0px;
}

#EditorContainer[activePane="true"], ContentPanel[activePane="true"] {
    border: 1px solid #ff551c;
}

#EditorContainer[activePane="false"], ContentPanel[activePane="false"] {
    border: 1px solid #333333;
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify CMake configuration matches all sources.
2. Confirm focus border updates when clicking between primary and secondary split panes.
3. Confirm clicking sidebar folders (`NavPanel`/`FavoritePanel`) loads the path into the active split pane.
4. Confirm clicking view mode buttons in the titlebar/statusbar changes both primary and secondary panes simultaneously.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `ContentPanel::loadDirectory(url)` and `m_activeContentPanel` in `PanelMediator`.
- Reuses `setViewMode(targetMode)` on both primary and secondary split panels.

## Header API Signature Verification
- `ContentPanel::secondaryContentPanel() const`: `ContentPanel*`
- `ContentPanel::isSplitMode() const`: `bool`
- `ContentPanel::setActivePane(bool active)`: `void`
- `ContentPanel::setViewMode(ViewMode mode)`: `void`
