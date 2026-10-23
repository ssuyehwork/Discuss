# PanelMediator Active Panel Navigation Routing Implementation Plan

## Overview
This plan modifies `PanelMediator` (`src/ui/PanelMediator.cpp`) to route directory navigation signals (`NavigationService::currentUrlChanged`) to the currently active content panel (`m_activeContentPanel`) instead of hardcoding navigation to the root/primary `contentPanel`.

## Modified Files List
1. `src/ui/PanelMediator.cpp`

## Detailed Line-by-Line Changes

### `src/ui/PanelMediator.cpp`
```cpp
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
            filterPanel->clearAllFilters();
            filterPanel->clearStats();
            filterPanel->setMirrorSource(false);
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
            filterPanel->clearAllFilters();
            filterPanel->clearStats();
            filterPanel->setMirrorSource(false);
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

## Header API Signature Verification
- `NavigationService::instance().currentUrlChanged`: `void(const QString& url, const QString& displayPath)`
- `PanelMediator::m_activeContentPanel`: `ContentPanel*`
