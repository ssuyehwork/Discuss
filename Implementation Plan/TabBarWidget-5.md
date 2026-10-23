# Implementation Plan - TabBar State Persistence

This plan adds full persistence for open tabs in `TabBarWidget` using `AppConfig`. All open tabs (URLs, titles, colors, icons, and active index) will be saved on application close and restored on startup.

## Overview
1. **TabBar Persistence Methods**:
   - In `TabBarWidget.h / .cpp`: Add `saveStateToConfig()` and `restoreStateFromConfig()`.
   - `saveStateToConfig()` serializes `m_tabs` (URL, title, color, iconKey) and `m_currentIndex` into a JSON string under `"TabBar/State"` in `AppConfig`.
   - `restoreStateFromConfig()` parses `"TabBar/State"`, rebuilds `m_tabs`, restores the active tab index, and emits `currentTabChanged`.
2. **MainWindow Close & Restore Hooks**:
   - In `MainWindow::showEvent`: Call `m_titleBarWidget->tabBar()->restoreStateFromConfig()` on startup.
   - In `MainWindow::closeEvent`: Call `m_titleBarWidget->tabBar()->saveStateToConfig()` on shutdown.

## Modified Files List
- `src/ui/TabBarWidget.h`
- `src/ui/TabBarWidget.cpp`
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/TabBarWidget.h`
<<<<<<< SEARCH
    void updateCurrentTabTitle(const QString& title, const QString& url);
    void openOrFocusTab(const QString& path);
=======
    void updateCurrentTabTitle(const QString& title, const QString& url);
    void openOrFocusTab(const QString& path);

    void saveStateToConfig();
    bool restoreStateFromConfig();
>>>>>>> REPLACE

### 2. `src/ui/TabBarWidget.cpp`
<<<<<<< SEARCH
#include "../meta/FavoriteDao.h"
=======
#include "../meta/FavoriteDao.h"
#include "../core/AppConfig.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabBarWidget::openOrFocusTab(const QString& rawPath) {
=======
void TabBarWidget::saveStateToConfig() {
    QJsonArray tabArray;
    for (const auto& tab : m_tabs) {
        QJsonObject obj;
        obj["title"] = tab.title;
        obj["url"] = tab.url;
        obj["color"] = tab.color;
        obj["iconKey"] = tab.iconKey;
        tabArray.append(obj);
    }

    QJsonObject stateObj;
    stateObj["tabs"] = tabArray;
    stateObj["currentIndex"] = m_currentIndex;

    QString jsonStr = QString::fromUtf8(QJsonDocument(stateObj).toJson(QJsonDocument::Compact));
    AppConfig::instance().setValue("TabBar/SavedState", jsonStr);
    AppConfig::instance().sync();
}

bool TabBarWidget::restoreStateFromConfig() {
    QString jsonStr = AppConfig::instance().getValue("TabBar/SavedState").toString();
    if (jsonStr.isEmpty()) return false;

    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isObject()) return false;

    QJsonObject stateObj = doc.object();
    QJsonArray tabArray = stateObj["tabs"].toArray();
    if (tabArray.isEmpty()) return false;

    m_tabs.clear();
    for (const auto& val : tabArray) {
        QJsonObject obj = val.toObject();
        TabInfo info;
        info.id = QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + QString::number(m_tabs.size());
        info.title = obj["title"].toString("此电脑");
        info.url = obj["url"].toString("computer://");
        info.color = obj["color"].toString();
        info.iconKey = obj["iconKey"].toString();
        info.active = false;
        m_tabs.append(info);
    }

    int savedIndex = stateObj["currentIndex"].toInt(0);
    if (savedIndex < 0 || savedIndex >= m_tabs.size()) {
        savedIndex = 0;
    }

    m_currentIndex = savedIndex;
    for (int i = 0; i < m_tabs.size(); ++i) {
        m_tabs[i].active = (i == m_currentIndex);
    }

    rebuildTabsUi();
    if (m_currentIndex >= 0 && m_currentIndex < m_tabs.size()) {
        emit currentTabChanged(m_currentIndex, m_tabs[m_currentIndex].url);
    }
    return true;
}

void TabBarWidget::openOrFocusTab(const QString& rawPath) {
>>>>>>> REPLACE

<<<<<<< SEARCH
        addTab(src.title, src.url, true);
    }
}
=======
        addTab(src.title, src.url, true);
    }
    saveStateToConfig();
}
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (indexChanged || forceNotify) {
        emit currentTabChanged(m_currentIndex, m_tabs[m_currentIndex].url);
    }
}
=======
    if (indexChanged || forceNotify) {
        emit currentTabChanged(m_currentIndex, m_tabs[m_currentIndex].url);
    }
    saveStateToConfig();
}
>>>>>>> REPLACE

### 3. `src/ui/MainWindow.cpp`
<<<<<<< SEARCH
#include "MainWindow.h"
#include "TitleBarWidget.h"
#include "NavBarWidget.h"
=======
#include "MainWindow.h"
#include "TitleBarWidget.h"
#include "TabBarWidget.h"
#include "NavBarWidget.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (m_navPanel) m_navPanel->deferredInit();

        QString lastPath = AppConfig::instance().getValue("MainWindow/LastPath", "computer://").toString();
        bool isValid = lastPath.contains("://") || QDir(lastPath).exists();
        NavigationService::instance().navigateTo(isValid ? lastPath : "computer://");
=======
        if (m_navPanel) m_navPanel->deferredInit();

        bool restored = false;
        if (m_titleBarWidget && m_titleBarWidget->tabBar()) {
            restored = m_titleBarWidget->tabBar()->restoreStateFromConfig();
        }

        if (!restored) {
            QString lastPath = AppConfig::instance().getValue("MainWindow/LastPath", "computer://").toString();
            bool isValid = lastPath.contains("://") || QDir(lastPath).exists();
            NavigationService::instance().navigateTo(isValid ? lastPath : "computer://");
        }
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (m_panelLayoutManager) {
        m_panelLayoutManager->saveLayoutState();
    }
    AppConfig::instance().sync();
=======
    if (m_panelLayoutManager) {
        m_panelLayoutManager->saveLayoutState();
    }
    if (m_titleBarWidget && m_titleBarWidget->tabBar()) {
        m_titleBarWidget->tabBar()->saveStateToConfig();
    }
    AppConfig::instance().sync();
>>>>>>> REPLACE

## Build & Verification Steps
1. Build application.
2. Open multiple tabs with different paths.
3. Close the application.
4. Launch the application again: verify all tabs and the active tab selection are 100% restored.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `AppConfig::instance().setValue(...)` and `AppConfig::instance().getValue(...)`.

## Header API Signature Verification
- `TabBarWidget::saveStateToConfig()`
- `TabBarWidget::restoreStateFromConfig()`
