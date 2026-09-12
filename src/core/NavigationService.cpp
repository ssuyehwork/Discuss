#include "NavigationService.h"
#include "NavigationHistoryService.h"
#include "DeviceWatcher.h"
#include <QDir>
#include <QFileInfo>
#include <QUuid>
#include <QDateTime>

namespace QuarkMeta {

NavigationService& NavigationService::instance() {
    static NavigationService s_instance;
    return s_instance;
}

NavigationService::NavigationService(QObject* parent) : QObject(parent) {
    connect(&DeviceWatcher::instance(), &DeviceWatcher::driveUnmounted, this, [this](const QString& driveLetter) {
        if (currentUrl().contains(driveLetter, Qt::CaseInsensitive)) {
            navigateTo("computer://");
        }
    });
    createTab("computer://");
}

QString NavigationService::displayPathForUrl(const QString& url) {
    if (url == "computer://") return "此电脑";
    if (url == "trash://") return "回收站";
    if (url.contains("://")) return url;
    return QDir::toNativeSeparators(url);
}

QString NavigationService::titleForUrl(const QString& url) {
    if (url.isEmpty() || url == "computer://") return "此电脑";
    if (url == "trash://") return "回收站";
    if (url.contains("://")) return url;

    QString clean = QDir::cleanPath(url);
    QFileInfo fi(clean);
    QString fileName = fi.fileName();
    if (!fileName.isEmpty()) {
        return fileName;
    }
    return QDir::toNativeSeparators(clean);
}

int NavigationService::createTab(const QString& url) {
    NavTabSession tab;
    tab.id = QString::number(QUuid::createUuid().data1, 16) + QString::number(QDateTime::currentMSecsSinceEpoch());
    QString targetUrl = url.isEmpty() ? (m_activeTabIndex >= 0 && m_activeTabIndex < m_tabs.size() ? m_tabs[m_activeTabIndex].currentUrl : QString("computer://")) : url;
    tab.currentUrl = normalizeUrl(targetUrl);
    tab.title = titleForUrl(tab.currentUrl);
    tab.history.append(tab.currentUrl);
    tab.historyIndex = 0;

    m_tabs.append(tab);
    m_activeTabIndex = m_tabs.size() - 1;

    emit tabsUpdated();
    emit currentUrlChanged(currentUrl(), currentDisplayPath());
    emitNavState();

    return m_activeTabIndex;
}

void NavigationService::closeTab(int index) {
    if (index < 0 || index >= m_tabs.size()) return;

    if (m_tabs.size() == 1) {
        // 若只剩最后一个，重置为 computer://
        m_tabs[0].currentUrl = "computer://";
        m_tabs[0].title = titleForUrl("computer://");
        m_tabs[0].history.clear();
        m_tabs[0].history.append("computer://");
        m_tabs[0].historyIndex = 0;
        m_activeTabIndex = 0;
        emit tabsUpdated();
        emit currentUrlChanged(currentUrl(), currentDisplayPath());
        emitNavState();
        return;
    }

    m_tabs.removeAt(index);
    if (m_activeTabIndex >= m_tabs.size()) {
        m_activeTabIndex = m_tabs.size() - 1;
    } else if (m_activeTabIndex > index) {
        m_activeTabIndex--;
    }

    emit tabsUpdated();
    emit currentUrlChanged(currentUrl(), currentDisplayPath());
    emitNavState();
}

void NavigationService::switchTab(int index) {
    if (index < 0 || index >= m_tabs.size() || index == m_activeTabIndex) return;

    m_activeTabIndex = index;
    emit tabsUpdated();
    emit currentUrlChanged(currentUrl(), currentDisplayPath());
    emitNavState();
}

QString NavigationService::currentUrl() const {
    if (m_activeTabIndex >= 0 && m_activeTabIndex < m_tabs.size()) {
        return m_tabs[m_activeTabIndex].currentUrl;
    }
    return "computer://";
}

QString NavigationService::normalizeUrl(const QString& rawUrl) const {
    if (rawUrl.isEmpty()) return "computer://";

    QString url = rawUrl.trimmed();
    if (url.startsWith("file://", Qt::CaseInsensitive)) {
        url = url.mid(7);
    }

    if (url == "computer://" || url == "trash://" || url.contains("://")) {
        return url;
    }

    QString clean = QDir::fromNativeSeparators(QDir::cleanPath(url));
    if (clean.endsWith(':')) {
        clean += "/";
    }
    return clean;
}

QString NavigationService::currentDisplayPath() const {
    return displayPathForUrl(currentUrl());
}

bool NavigationService::isVirtualProtocol() const {
    return currentUrl().contains("://");
}

bool NavigationService::canGoBack() const {
    if (m_activeTabIndex < 0 || m_activeTabIndex >= m_tabs.size()) return false;
    return m_tabs[m_activeTabIndex].historyIndex > 0;
}

bool NavigationService::canGoForward() const {
    if (m_activeTabIndex < 0 || m_activeTabIndex >= m_tabs.size()) return false;
    const auto& tab = m_tabs[m_activeTabIndex];
    return tab.historyIndex < tab.history.size() - 1;
}

bool NavigationService::canGoUp() const {
    QString url = currentUrl();
    if (url.isEmpty() || url == "computer://" || url == "trash://") {
        return false;
    }
    return true;
}

void NavigationService::emitNavState() {
    emit navStateChanged(canGoBack(), canGoForward(), canGoUp());
}

void NavigationService::navigateTo(const QString& rawUrl, bool recordHistory) {
    if (rawUrl.isEmpty()) return;
    if (m_activeTabIndex < 0 || m_activeTabIndex >= m_tabs.size()) {
        createTab("computer://");
    }

    QString normalized = normalizeUrl(rawUrl);
    NavTabSession& activeTab = m_tabs[m_activeTabIndex];

    if (recordHistory) {
        if (activeTab.historyIndex < activeTab.history.size() - 1) {
            activeTab.history = activeTab.history.mid(0, activeTab.historyIndex + 1);
        }

        if (activeTab.history.isEmpty() || activeTab.history.last() != normalized) {
            activeTab.history.append(normalized);
            if (activeTab.history.size() > kMaxHistoryDepth) {
                activeTab.history.removeFirst();
            }
            activeTab.historyIndex = activeTab.history.size() - 1;
        }
    }

    activeTab.currentUrl = normalized;
    activeTab.title = titleForUrl(normalized);

    if (!isVirtualProtocol()) {
        NavigationHistoryService::instance().appendPath(normalized);
        NavigationHistoryService::recordRecentVisitedFolder(QDir::toNativeSeparators(normalized).toStdWString());
    }

    emit currentUrlChanged(activeTab.currentUrl, currentDisplayPath());
    emit tabsUpdated();
    emitNavState();
}

void NavigationService::goBack() {
    if (!canGoBack()) return;

    NavTabSession& activeTab = m_tabs[m_activeTabIndex];
    activeTab.historyIndex--;
    activeTab.currentUrl = activeTab.history[activeTab.historyIndex];
    activeTab.title = titleForUrl(activeTab.currentUrl);

    emit currentUrlChanged(activeTab.currentUrl, currentDisplayPath());
    emit tabsUpdated();
    emitNavState();
}

void NavigationService::goForward() {
    if (!canGoForward()) return;

    NavTabSession& activeTab = m_tabs[m_activeTabIndex];
    activeTab.historyIndex++;
    activeTab.currentUrl = activeTab.history[activeTab.historyIndex];
    activeTab.title = titleForUrl(activeTab.currentUrl);

    emit currentUrlChanged(activeTab.currentUrl, currentDisplayPath());
    emit tabsUpdated();
    emitNavState();
}

void NavigationService::goUp() {
    if (!canGoUp()) return;

    QDir dir(currentUrl());
    if (dir.isRoot()) {
        navigateTo("computer://");
        return;
    }

    if (dir.cdUp()) {
        navigateTo(dir.absolutePath());
    } else {
        navigateTo("computer://");
    }
}

void NavigationService::refresh() {
    QString cur = currentUrl();
    if (!cur.isEmpty()) {
        emit currentUrlChanged(cur, currentDisplayPath());
    }
}

} // namespace QuarkMeta
