# Implementation Plan - TabBar & AddressBar Path Synchronization and Dynamic Folder Icon Color

This plan details the changes required to synchronize the active tab title and address bar breadcrumbs with navigation events (especially in `ColumnViewWidget`), and to render solid folder icons with dynamic color tags on `TabBarWidget`.

## Overview
1. **Address Bar & Tab Title Synchronization**:
   - `ColumnViewWidget` emitted `pathNavigated(path)` upon navigating folders in Column View, but `PanelMediator` failed to forward this to `NavigationService::instance().navigateTo(path)`.
   - Update `PanelMediator.cpp` so that `ColumnViewWidget::pathNavigated` triggers `NavigationService::instance().navigateTo(path)`.
   - Update `TabBarWidget::updateCurrentTabTitle` to set the folder name (or "此电脑" for `computer://`) and read any custom color tag via `MetadataManager::instance().getMeta(path.toStdWString())`.
2. **Dynamic Solid Folder SVG Icon**:
   - Update `TabBarWidget::updateTabsUiState` / `rebuildTabsUi` / `updateCurrentTabTitle` to fetch manual color from `MetadataManager`.
   - If a manual color is set (e.g. `#e74c3c`), render `UiHelper::getIcon("folder_filled", QColor(manualColor))` for the tab icon.
   - Otherwise, render `UiHelper::getIcon("folder_filled", active ? QColor("#EEEEEE") : QColor("#888888"))`.

## Modified Files List
- `src/ui/PanelMediator.cpp`
- `src/ui/TabBarWidget.h`
- `src/ui/TabBarWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/PanelMediator.cpp`
<<<<<<< SEARCH
        if (filterPanel && contentPanel->columnView()) {
            connect(contentPanel->columnView(), &ColumnViewWidget::pathNavigated, filterPanel, [filterPanel](const QString&) {
                filterPanel->clearAllFilters(false);
            });
        }
=======
        if (contentPanel->columnView()) {
            connect(contentPanel->columnView(), &ColumnViewWidget::pathNavigated, this, [filterPanel](const QString& path) {
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                NavigationService::instance().navigateTo(path);
            });
        }
>>>>>>> REPLACE

### 2. `src/ui/TabBarWidget.h`
<<<<<<< SEARCH
struct TabInfo {
    QString id;
    QString title;
    QString url;
    bool active = false;
};
=======
struct TabInfo {
    QString id;
    QString title;
    QString url;
    QString color;
    bool active = false;
};
>>>>>>> REPLACE

### 3. `src/ui/TabBarWidget.cpp`
<<<<<<< SEARCH
#include "TabBarWidget.h"
#include "UiHelper.h"
#include "StyleLibrary.h"

#include <QStyle>
#include <QDateTime>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
=======
#include "TabBarWidget.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include "../meta/MetadataManager.h"

#include <QStyle>
#include <QDateTime>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QDir>
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabBarWidget::updateCurrentTabTitle(const QString& title, const QString& url) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;
    if (m_tabs[m_currentIndex].title == title && m_tabs[m_currentIndex].url == url) return;

    m_tabs[m_currentIndex].title = title.isEmpty() ? "此电脑" : title;
    m_tabs[m_currentIndex].url = url;

    if (m_currentIndex < m_tabWidgets.size()) {
        auto tabBtn = m_tabWidgets[m_currentIndex];
        tabBtn->setTabTitle(m_tabs[m_currentIndex].title);
        tabBtn->setTabIcon(UiHelper::getIcon(url.startsWith("computer://") ? "computer" : "folder_filled", QColor("#EEEEEE")));
    }
}
=======
void TabBarWidget::updateCurrentTabTitle(const QString& title, const QString& url) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    QString folderName = title;
    if (url == "computer://" || url.isEmpty()) {
        folderName = "此电脑";
    } else if (title.contains("/") || title.contains("\\")) {
        QString cleanPath = QDir::cleanPath(url);
        QFileInfo fi(cleanPath);
        folderName = fi.fileName();
        if (folderName.isEmpty()) {
            folderName = cleanPath; // E.g., drive root "C:"
        }
    }

    QString colorHex;
    if (!url.startsWith("computer://") && !url.isEmpty()) {
        auto meta = MetadataManager::instance().getMeta(url.toStdWString());
        colorHex = QString::fromStdWString(meta.manualColor);
    }

    if (m_tabs[m_currentIndex].title == folderName && m_tabs[m_currentIndex].url == url && m_tabs[m_currentIndex].color == colorHex) return;

    m_tabs[m_currentIndex].title = folderName.isEmpty() ? "此电脑" : folderName;
    m_tabs[m_currentIndex].url = url;
    m_tabs[m_currentIndex].color = colorHex;

    if (m_currentIndex < m_tabWidgets.size()) {
        auto tabBtn = m_tabWidgets[m_currentIndex];
        tabBtn->setTabTitle(m_tabs[m_currentIndex].title);
        QColor iconColor = !colorHex.isEmpty() ? QColor(colorHex) : QColor("#EEEEEE");
        tabBtn->setTabIcon(UiHelper::getIcon(url.startsWith("computer://") ? "computer" : "folder_filled", iconColor));
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabBarWidget::updateTabsUiState() {
    if (m_tabWidgets.size() != m_tabs.size()) {
        rebuildTabsUi();
        return;
    }

    for (int i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs[i];
        auto tabBtn = m_tabWidgets[i];
        tabBtn->setIndex(i);
        tabBtn->setTabTitle(tab.title);
        tabBtn->setTabIcon(UiHelper::getIcon(tab.url.startsWith("computer://") ? "computer" : "folder_filled", 
                                                tab.active ? QColor("#EEEEEE") : QColor("#888888")));
        tabBtn->setActive(tab.active);
    }
}
=======
void TabBarWidget::updateTabsUiState() {
    if (m_tabWidgets.size() != m_tabs.size()) {
        rebuildTabsUi();
        return;
    }

    for (int i = 0; i < m_tabs.size(); ++i) {
        auto& tab = m_tabs[i];
        if (!tab.url.startsWith("computer://") && !tab.url.isEmpty()) {
            auto meta = MetadataManager::instance().getMeta(tab.url.toStdWString());
            tab.color = QString::fromStdWString(meta.manualColor);
        } else {
            tab.color.clear();
        }

        auto tabBtn = m_tabWidgets[i];
        tabBtn->setIndex(i);
        tabBtn->setTabTitle(tab.title);

        QColor iconColor = !tab.color.isEmpty() ? QColor(tab.color) : (tab.active ? QColor("#EEEEEE") : QColor("#888888"));
        tabBtn->setTabIcon(UiHelper::getIcon(tab.url.startsWith("computer://") ? "computer" : "folder_filled", iconColor));
        tabBtn->setActive(tab.active);
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabBarWidget::rebuildTabsUi() {
    m_tabWidgets.clear();
    QLayoutItem* child;
    while ((child = m_tabsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    for (int i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs[i];
        TabItemButton* tabItem = new TabItemButton(i, this);
        tabItem->setTabTitle(tab.title);
        tabItem->setTabIcon(UiHelper::getIcon(tab.url.startsWith("computer://") ? "computer" : "folder_filled", 
                                                tab.active ? QColor("#EEEEEE") : QColor("#888888")));
        tabItem->setActive(tab.active);
=======
void TabBarWidget::rebuildTabsUi() {
    m_tabWidgets.clear();
    QLayoutItem* child;
    while ((child = m_tabsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    for (int i = 0; i < m_tabs.size(); ++i) {
        auto& tab = m_tabs[i];
        if (!tab.url.startsWith("computer://") && !tab.url.isEmpty()) {
            auto meta = MetadataManager::instance().getMeta(tab.url.toStdWString());
            tab.color = QString::fromStdWString(meta.manualColor);
        } else {
            tab.color.clear();
        }

        TabItemButton* tabItem = new TabItemButton(i, this);
        tabItem->setTabTitle(tab.title);
        QColor iconColor = !tab.color.isEmpty() ? QColor(tab.color) : (tab.active ? QColor("#EEEEEE") : QColor("#888888"));
        tabItem->setTabIcon(UiHelper::getIcon(tab.url.startsWith("computer://") ? "computer" : "folder_filled", iconColor));
        tabItem->setActive(tab.active);
>>>>>>> REPLACE

## Build & Verification Steps
1. Build the project using CMake & MSVC / Ninja:
   `cmake --build build --config Release`
2. Run application and verify:
   - Navigate in ColumnView or Click folders: observe real-time updates to address bar breadcrumbs and active tab title.
   - Assign a color tag to a folder: observe the solid folder icon on the tab turning into the assigned color tag.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `NavigationService::instance().navigateTo(path)` SSOT entry point.
- Reused `MetadataManager::instance().getMeta(...)` for folder color lookup.

## Header API Signature Verification
- `NavigationService::instance().navigateTo(const QString& rawUrl, bool recordHistory = true)`
- `MetadataManager::instance().getMeta(const std::wstring& path)`
- `UiHelper::getIcon(const QString& key, const QColor& color = QColor(), int size = 16)`
