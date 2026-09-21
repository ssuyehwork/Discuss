# Implementation Plan - TabBar Persistence Bug Fix

This plan fixes the issue where persisted tab states in `AppConfig` were being overwritten by default initialization during application startup.

## Overview
1. **Prevent Overwriting Saved State During Initialization**:
   - Add `bool m_isInitializing = true;` flag to `TabBarWidget`.
   - In `saveStateToConfig()`, return immediately if `m_isInitializing` is true.
   - At the end of `restoreStateFromConfig()`, or if restoring fails, set `m_isInitializing = false` so that subsequent user interactions and path updates are saved.
2. **Persist State On Tab Navigation & Title Update**:
   - Call `saveStateToConfig()` inside `updateCurrentTabTitle()`, `addTab()`, and `closeTab()` so tab URLs and colors are persisted immediately upon navigation or tab changes.

## Modified Files List
- `src/ui/TabBarWidget.h`
- `src/ui/TabBarWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/TabBarWidget.h`
<<<<<<< SEARCH
    QList<TabItemButton*> m_tabWidgets;
    int m_currentIndex = -1;
};
=======
    QList<TabItemButton*> m_tabWidgets;
    int m_currentIndex = -1;
    bool m_isInitializing = true;
};
>>>>>>> REPLACE

### 2. `src/ui/TabBarWidget.cpp`
<<<<<<< SEARCH
    addTab(title, cleanTarget, true);
}
=======
    addTab(title, cleanTarget, true);
    saveStateToConfig();
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabBarWidget::addTab(const QString& title, const QString& url, bool switchToNew) {
    TabInfo info;
    info.id = QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + QString::number(m_tabs.size());
    info.title = title.isEmpty() ? "此电脑" : title;
    info.url = url.isEmpty() ? "computer://" : url;
    info.active = false;

    m_tabs.append(info);
    if (switchToNew || m_currentIndex == -1) {
        setCurrentIndex(m_tabs.size() - 1, true);
    } else {
        rebuildTabsUi();
    }
}

void TabBarWidget::closeTab(int index) {
    if (index < 0 || index >= m_tabs.size()) return;
    if (m_tabs.size() <= 1) {
        // 仅剩一个标签页时，重置为默认“此电脑”
        m_tabs[0].title = "此电脑";
        m_tabs[0].url = "computer://";
        updateTabsUiState();
        emit currentTabChanged(0, "computer://");
        return;
    }

    m_closedTabsHistory.append(m_tabs[index]);
    m_tabs.removeAt(index);
    if (index < m_currentIndex) {
        m_currentIndex--;
    } else if (m_currentIndex >= m_tabs.size()) {
        m_currentIndex = m_tabs.size() - 1;
    }
    setCurrentIndex(m_currentIndex, true);
    emit tabClosed(index);
}
=======
void TabBarWidget::addTab(const QString& title, const QString& url, bool switchToNew) {
    TabInfo info;
    info.id = QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + QString::number(m_tabs.size());
    info.title = title.isEmpty() ? "此电脑" : title;
    info.url = url.isEmpty() ? "computer://" : url;
    info.active = false;

    m_tabs.append(info);
    if (switchToNew || m_currentIndex == -1) {
        setCurrentIndex(m_tabs.size() - 1, true);
    } else {
        rebuildTabsUi();
    }
    saveStateToConfig();
}

void TabBarWidget::closeTab(int index) {
    if (index < 0 || index >= m_tabs.size()) return;
    if (m_tabs.size() <= 1) {
        // 仅剩一个标签页时，重置为默认“此电脑”
        m_tabs[0].title = "此电脑";
        m_tabs[0].url = "computer://";
        updateTabsUiState();
        emit currentTabChanged(0, "computer://");
        saveStateToConfig();
        return;
    }

    m_closedTabsHistory.append(m_tabs[index]);
    m_tabs.removeAt(index);
    if (index < m_currentIndex) {
        m_currentIndex--;
    } else if (m_currentIndex >= m_tabs.size()) {
        m_currentIndex = m_tabs.size() - 1;
    }
    setCurrentIndex(m_currentIndex, true);
    emit tabClosed(index);
    saveStateToConfig();
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabBarWidget::saveStateToConfig() {
    QJsonArray tabArray;
=======
void TabBarWidget::saveStateToConfig() {
    if (m_isInitializing) return;

    QJsonArray tabArray;
>>>>>>> REPLACE

<<<<<<< SEARCH
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
=======
bool TabBarWidget::restoreStateFromConfig() {
    m_isInitializing = true;
    QString jsonStr = AppConfig::instance().getValue("TabBar/SavedState").toString();
    if (jsonStr.isEmpty()) {
        m_isInitializing = false;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isObject()) {
        m_isInitializing = false;
        return false;
    }

    QJsonObject stateObj = doc.object();
    QJsonArray tabArray = stateObj["tabs"].toArray();
    if (tabArray.isEmpty()) {
        m_isInitializing = false;
        return false;
    }

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
    m_isInitializing = false;

    if (m_currentIndex >= 0 && m_currentIndex < m_tabs.size()) {
        emit currentTabChanged(m_currentIndex, m_tabs[m_currentIndex].url);
    }
    return true;
}
>>>>>>> REPLACE

<<<<<<< SEARCH
        QString iconKey = !m_tabs[m_currentIndex].iconKey.isEmpty() ? m_tabs[m_currentIndex].iconKey : (url.startsWith("computer://") ? "computer" : "folder_filled");
        tabBtn->setTabIcon(UiHelper::getIcon(iconKey, iconColor));
    }
}
=======
        QString iconKey = !m_tabs[m_currentIndex].iconKey.isEmpty() ? m_tabs[m_currentIndex].iconKey : (url.startsWith("computer://") ? "computer" : "folder_filled");
        tabBtn->setTabIcon(UiHelper::getIcon(iconKey, iconColor));
    }
    saveStateToConfig();
}
>>>>>>> REPLACE

## Build & Verification Steps
1. Build project.
2. Launch app, open multiple tabs with different paths.
3. Close app and relaunch: verify all tabs and current active index are fully restored.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reused existing `AppConfig::instance()` and `TabBarWidget::saveStateToConfig()`.

## Header API Signature Verification
- `TabBarWidget::saveStateToConfig()`
- `TabBarWidget::restoreStateFromConfig()`
