# QuarkMeta 路径导航与历史栈服务无脑实施方案 (NavigationService.md)

## 1. Overview（概述与解决的问题）

### 1.1 解决的问题
当前主窗口 `MainWindow` 承担了过多路径跳转与历史状态维护的职责（例如保存 `m_currentPath`、维护后退/前进历史列表 `m_history` / `m_historyIndex`、解析协议及判断物理与虚拟路径上级），导致 `MainWindow` 充斥大量底层算式与状态控制逻辑，严重违背单一职责原则（SRP）与 Clean Architecture 五层解耦规范。

### 1.2 重构目标
1. **新建 `NavigationService` 领域单例服务**：收拢全系统路径状态（`m_currentUrl`）、协议归一化解析（`file://`、`computer://`、`trash://`）、前进/后退双向历史栈状态机、上级路径智能解析及最近访问记录持久化。
2. **彻底净化 `MainWindow`**：物理删除 `MainWindow` 内部持有的路径历史成员变量与历史槽函数，主窗口退化为纯 UI 装配与信号监听壳体。
3. **`PanelMediator` 统一路由**：各面板与工具栏按钮通过 `NavigationService` 单向驱动，路径变更事件自动联动更新各子面板与清空搜索/筛选界面状态。

---

## 2. Modified Files List（影响文件清单）

1. `CMakeLists.txt`（注册新增源文件）
2. `src/core/NavigationService.h`（全新创建，定义导航领域服务）
3. `src/core/NavigationService.cpp`（全新创建，实现导航与历史栈服务）
4. `src/ui/MainWindow.h`（彻底移除路径变量与历史槽函数）
5. `src/ui/MainWindow.cpp`（重构工具栏绑定与路径状态转调）
6. `src/ui/PanelMediator.cpp`（统一连接 NavigationService 信号槽与状态重置）

---

## 3. Detailed Line-by-Line Changes（精准代码替换块）

### 3.1 `CMakeLists.txt` 注册新文件

<<<<<<< SEARCH
    src/core/NavigationHistoryService.cpp
    src/core/NavigationHistoryService.h
=======
    src/core/NavigationService.cpp
    src/core/NavigationService.h
    src/core/NavigationHistoryService.cpp
    src/core/NavigationHistoryService.h
>>>>>>> REPLACE

---

### 3.2 新建 `src/core/NavigationService.h`

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QList>

namespace QuarkMeta {

class NavigationService : public QObject {
    Q_OBJECT

public:
    static NavigationService& instance();

    // 核心导航调度接口
    void navigateTo(const QString& rawUrl, bool recordHistory = true);
    void goBack();
    void goForward();
    void goUp();
    void refresh();

    // 状态查询接口
    QString currentUrl() const { return m_currentUrl; }
    QString currentDisplayPath() const;
    bool isVirtualProtocol() const;
    bool canGoBack() const { return m_currentIndex > 0; }
    bool canGoForward() const { return m_currentIndex < m_history.size() - 1; }
    bool canGoUp() const;

signals:
    /**
     * @brief 全局统一路径变更信号 (驱动各子面板单向加载数据)
     * @param url 标准协议 URL (如 file://C:/Users 或 computer://)
     * @param displayPath 适合 UI 面包屑展示的文本 (如 C:\Users 或 此电脑)
     */
    void currentUrlChanged(const QString& url, const QString& displayPath);

    /**
     * @brief 导航可用性状态变动信号 (驱动前进/后退/上级按钮状态)
     */
    void navStateChanged(bool canBack, bool canForward, bool canUp);

private:
    explicit NavigationService(QObject* parent = nullptr);
    ~NavigationService() override = default;
    NavigationService(const NavigationService&) = delete;
    NavigationService& operator=(const NavigationService&) = delete;

    QString normalizeUrl(const QString& rawUrl) const;
    void emitNavState();

    QString m_currentUrl;
    QList<QString> m_history;
    int m_currentIndex = -1;
    static constexpr int kMaxHistoryDepth = 100;
};

} // namespace QuarkMeta
```

---

### 3.3 新建 `src/core/NavigationService.cpp`

```cpp
#include "NavigationService.h"
#include "NavigationHistoryService.h"
#include <QDir>
#include <QFileInfo>

namespace QuarkMeta {

NavigationService& NavigationService::instance() {
    static NavigationService s_instance;
    return s_instance;
}

NavigationService::NavigationService(QObject* parent) : QObject(parent) {}

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
    if (m_currentUrl == "computer://") return "此电脑";
    if (m_currentUrl == "trash://") return "回收站";
    if (m_currentUrl.contains("://")) return m_currentUrl;
    return QDir::toNativeSeparators(m_currentUrl);
}

bool NavigationService::isVirtualProtocol() const {
    return m_currentUrl.contains("://");
}

bool NavigationService::canGoUp() const {
    if (m_currentUrl.isEmpty() || m_currentUrl == "computer://" || m_currentUrl == "trash://") {
        return false;
    }
    return true;
}

void NavigationService::emitNavState() {
    emit navStateChanged(canGoBack(), canGoForward(), canGoUp());
}

void NavigationService::navigateTo(const QString& rawUrl, bool recordHistory) {
    if (rawUrl.isEmpty()) return;

    QString normalized = normalizeUrl(rawUrl);

    if (recordHistory) {
        if (m_currentIndex < m_history.size() - 1) {
            m_history = m_history.mid(0, m_currentIndex + 1);
        }

        if (m_history.isEmpty() || m_history.last() != normalized) {
            m_history.append(normalized);
            if (m_history.size() > kMaxHistoryDepth) {
                m_history.removeFirst();
            }
            m_currentIndex = m_history.size() - 1;
        }
    }

    m_currentUrl = normalized;

    if (!isVirtualProtocol()) {
        NavigationHistoryService::recordRecentVisitedFolder(QDir::toNativeSeparators(m_currentUrl).toStdWString());
    }

    emit currentUrlChanged(m_currentUrl, currentDisplayPath());
    emitNavState();
}

void NavigationService::goBack() {
    if (canGoBack()) {
        m_currentIndex--;
        m_currentUrl = m_history[m_currentIndex];
        emit currentUrlChanged(m_currentUrl, currentDisplayPath());
        emitNavState();
    }
}

void NavigationService::goForward() {
    if (canGoForward()) {
        m_currentIndex++;
        m_currentUrl = m_history[m_currentIndex];
        emit currentUrlChanged(m_currentUrl, currentDisplayPath());
        emitNavState();
    }
}

void NavigationService::goUp() {
    if (!canGoUp()) return;

    QDir dir(m_currentUrl);
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
    if (!m_currentUrl.isEmpty()) {
        emit currentUrlChanged(m_currentUrl, currentDisplayPath());
    }
}

} // namespace QuarkMeta
```

---

### 3.4 `src/ui/MainWindow.h` 修改

<<<<<<< SEARCH
private slots:
    void onPinToggled(bool checked);
    void onBackClicked();
    void onForwardClicked();
    void onUpClicked();
    void onStatusBarStatsUpdated(int fileCount, int folderCount, int totalCount);
    void onVolumeUnplugged(const QString& driveLetter);
=======
private slots:
    void onPinToggled(bool checked);
    void onStatusBarStatsUpdated(int fileCount, int folderCount, int totalCount);
    void onVolumeUnplugged(const QString& driveLetter);
>>>>>>> REPLACE

<<<<<<< SEARCH
    void initUi();
    void updateNavButtons();
    void updateStatusBar();
    void initDriveBar();

    // 2026-07-xx 导航协议常量
    static inline const QString kProtocolFile     = "file://";
    static inline const QString kProtocolSystem   = "system://";

    /**
     * @brief 2026-07-xx 按照 Plan-56：统一导航调度中心
     * 支持 file://, category://, system:// 等协议
     */
    void unifiedNavigateTo(const QString& url, bool record = true);
=======
    void initUi();
    void updateStatusBar();
    void initDriveBar();

    /**
     * @brief 统一导航调度向前兼容转调接口
     */
    void unifiedNavigateTo(const QString& url, bool record = true);
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 状态管理
    bool m_isPinned = false;
    bool m_isTagManagerMode = false;
    QString m_currentDataSource; // "category" or "nav"
    bool m_panelsInitialized = false; // 2026-04-12 状态锁：确保面板仅初始化一次
    QString m_currentPath;
    QStringList m_history;
    int m_historyIndex = -1;
=======
    // 状态管理
    bool m_isPinned = false;
    bool m_isTagManagerMode = false;
    QString m_currentDataSource; // "category" or "nav"
    bool m_panelsInitialized = false; // 2026-04-12 状态锁：确保面板仅初始化一次
>>>>>>> REPLACE

---

### 3.5 `src/ui/MainWindow.cpp` 修改

<<<<<<< SEARCH
#include "FramelessDialog.h"
#include "TagSelectorOverlay.h"
=======
#include "FramelessDialog.h"
#include "TagSelectorOverlay.h"
#include "../core/NavigationService.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
    QTimer::singleShot(200, [this]() {
        QString lastPath = AppConfig::instance().getValue("MainWindow/LastPath", "computer://").toString();
        bool isValid = lastPath.contains("://") || QDir(lastPath).exists();
        if (isValid) {
            unifiedNavigateTo(lastPath);
        } else {
            unifiedNavigateTo("computer://");
        }
    });
=======
    QTimer::singleShot(200, []() {
        QString lastPath = AppConfig::instance().getValue("MainWindow/LastPath", "computer://").toString();
        bool isValid = lastPath.contains("://") || QDir(lastPath).exists();
        NavigationService::instance().navigateTo(isValid ? lastPath : "computer://");
    });
>>>>>>> REPLACE

<<<<<<< SEARCH
void MainWindow::unifiedNavigateTo(const QString& url, bool record) {
    if (url.isEmpty()) return;

    if (m_searchController && m_searchController->searchEdit()) {
        m_searchController->searchEdit()->blockSignals(true);
        m_searchController->searchEdit()->clear();
        m_searchController->searchEdit()->blockSignals(false);
    }
    
    if (m_contentPanel) {
        m_contentPanel->search("");
    }

    if (m_filterPanel) m_filterPanel->clearAllFilters();

    if (record) {
        if (m_historyIndex < static_cast<int>(m_history.size()) - 1) {
            m_history = m_history.mid(0, m_historyIndex + 1);
        }
        if (m_history.isEmpty() || m_history.last() != url) {
            m_history.append(url);
            m_historyIndex = static_cast<int>(m_history.size()) - 1;
        }
    }

    QString normPath = url;
    if (url.startsWith("file://", Qt::CaseInsensitive)) {
        normPath = url.mid(7);
    }

    if (normPath == "computer://") {
        if (m_addressBar) m_addressBar->setPath("此电脑");
        if (m_contentPanel) m_contentPanel->loadDirectory("");
        if (m_navPanel) m_navPanel->selectPath("");
        m_currentPath = "computer://";
    } else {
        QString normPath = QDir::toNativeSeparators(path);
        if (m_addressBar) m_addressBar->setPath(normPath);
        if (m_contentPanel) m_contentPanel->loadDirectory(normPath);
        if (m_navPanel) m_navPanel->selectPath(normPath);
        m_currentPath = normPath;
        NavigationHistoryService::recordRecentVisitedFolder(normPath.toStdWString());
    }

    if (m_filterPanel && m_contentPanel) {
        m_filterPanel->setMirrorSource(false);
    }

    updateNavButtons();
    updateStatusBar();
}

void MainWindow::onBackClicked() {
    if (m_historyIndex > 0) {
        m_historyIndex--;
        unifiedNavigateTo(m_history[m_historyIndex], false);
    }
}

void MainWindow::onForwardClicked() {
    if (m_historyIndex < m_history.size() - 1) {
        m_historyIndex++;
        unifiedNavigateTo(m_history[m_historyIndex], false);
    }
}

void MainWindow::onUpClicked() {
    if (m_currentPath.contains("://") && m_currentPath != "computer://") {
        unifiedNavigateTo("computer://");
        return;
    }

    QDir dir(m_currentPath);
    if (dir.cdUp()) {
        unifiedNavigateTo(dir.absolutePath());
    }
}

void MainWindow::updateNavButtons() {
    m_btnBack->setEnabled(m_historyIndex > 0);
    m_btnForward->setEnabled(m_historyIndex < m_history.size() - 1);
    
    bool isLogic = m_currentPath.contains("://");
    bool atRoot = (m_currentPath == "computer://" || (!isLogic && QDir(m_currentPath).isRoot()));
    m_btnUp->setEnabled(!atRoot && !m_currentPath.isEmpty());
}

void MainWindow::onVolumeUnplugged(const QString& driveLetter) {
    bool isCurrentOnUnpluggedDrive = false;
    if (m_currentPath.contains(driveLetter + ":", Qt::CaseInsensitive)) {
        isCurrentOnUnpluggedDrive = true;
    }

    if (isCurrentOnUnpluggedDrive) {
        unifiedNavigateTo("computer://");
    }
}
=======
void MainWindow::unifiedNavigateTo(const QString& url, bool record) {
    NavigationService::instance().navigateTo(url, record);
}

void MainWindow::onVolumeUnplugged(const QString& driveLetter) {
    QString current = NavigationService::instance().currentUrl();
    if (current.contains(driveLetter + ":", Qt::CaseInsensitive)) {
        NavigationService::instance().navigateTo("computer://");
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
    AppConfig::instance().setValue("MainWindow/LastPath", m_currentPath);
=======
    AppConfig::instance().setValue("MainWindow/LastPath", NavigationService::instance().currentUrl());
>>>>>>> REPLACE

<<<<<<< SEARCH
        TagManagerDialog::showDialog(this, m_currentPath, false);
=======
        TagManagerDialog::showDialog(this, NavigationService::instance().currentUrl(), false);
>>>>>>> REPLACE

---

### 3.6 `src/ui/PanelMediator.cpp` 修改

<<<<<<< SEARCH
#include "../core/CoreEngine.h"
=======
#include "../core/CoreEngine.h"
#include "../core/NavigationService.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 1. 导航/收藏/内容面板 双击跳转 -> 统一导航中枢
    if (navPanel) {
        connect(navPanel, &NavPanel::directorySelected, m_mainWindow, [this](const QString& path) {
            m_mainWindow->unifiedNavigateTo(path);
        });

        connect(navPanel, &NavPanel::requestOpenTrash, m_mainWindow, [this, contentPanel, addressBar]() {
            if (contentPanel) {
                contentPanel->loadCategory("trash");
            }
            if (addressBar) {
                addressBar->setPath("trash://");
            }
            m_mainWindow->m_currentPath = "trash://";
            m_mainWindow->updateNavButtons();
            m_mainWindow->updateStatusBar();
        });
    }

    if (favoritePanel) {
        connect(favoritePanel, &FavoritePanel::directorySelected, m_mainWindow, [this](const QString& path) {
            m_mainWindow->unifiedNavigateTo(path);
        });

        connect(favoritePanel, &FavoritePanel::requestLocateFile, m_mainWindow, [this, contentPanel](const QString& path) {
            QFileInfo fi(path);
            if (contentPanel) {
                contentPanel->setPendingSelectName(fi.fileName(), false);
            }
            m_mainWindow->unifiedNavigateTo(fi.absolutePath());
        });
    }

    if (contentPanel) {
        connect(contentPanel, &ContentPanel::directorySelected, m_mainWindow, [this](const QString& path) {
            m_mainWindow->unifiedNavigateTo(path);
        });

        // 监听内容容器的右键添加至收藏夹信号
        connect(contentPanel, &ContentPanel::requestAddFavorite, m_mainWindow, [favoritePanel](const QStringList& paths) {
            if (favoritePanel) {
                for (const QString& p : paths) {
                    favoritePanel->addFavoriteItem(p);
                }
                favoritePanel->saveFavorites();
            }
        });
    }
=======
    // 1. 路径变更与导航驱动
    connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, m_mainWindow,
            [this, contentPanel, addressBar, navPanel, filterPanel](const QString& url, const QString& displayPath) {
        // A. 重置 UI 搜索词与筛选条件
        if (m_mainWindow->m_searchController && m_mainWindow->m_searchController->searchEdit()) {
            m_mainWindow->m_searchController->searchEdit()->blockSignals(true);
            m_mainWindow->m_searchController->searchEdit()->clear();
            m_mainWindow->m_searchController->searchEdit()->blockSignals(false);
        }
        if (contentPanel) {
            contentPanel->search("");
        }
        if (filterPanel) {
            filterPanel->clearAllFilters();
            filterPanel->setMirrorSource(false);
        }

        // B. 同步地址栏与侧边栏
        if (addressBar) addressBar->setPath(displayPath);
        if (navPanel) navPanel->selectPath(url == "computer://" ? "" : url);

        // C. 加载内容区数据
        if (contentPanel) {
            if (url == "computer://") {
                contentPanel->loadDirectory("");
            } else if (url == "trash://") {
                contentPanel->loadCategory("trash");
            } else {
                contentPanel->loadDirectory(url);
            }
        }

        m_mainWindow->updateStatusBar();
    });

    if (navPanel) {
        connect(navPanel, &NavPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(navPanel, &NavPanel::requestOpenTrash, &NavigationService::instance(), []() {
            NavigationService::instance().navigateTo("trash://");
        });
    }

    if (favoritePanel) {
        connect(favoritePanel, &FavoritePanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(favoritePanel, &FavoritePanel::requestLocateFile, m_mainWindow, [contentPanel](const QString& path) {
            QFileInfo fi(path);
            if (contentPanel) {
                contentPanel->setPendingSelectName(fi.fileName(), false);
            }
            NavigationService::instance().navigateTo(fi.absolutePath());
        });
    }

    if (contentPanel) {
        connect(contentPanel, &ContentPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(contentPanel, &ContentPanel::requestAddFavorite, m_mainWindow, [favoritePanel](const QStringList& paths) {
            if (favoritePanel) {
                for (const QString& p : paths) {
                    favoritePanel->addFavoriteItem(p);
                }
                favoritePanel->saveFavorites();
            }
        });
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 6. 地址栏路径跳转与刷新
    if (addressBar) {
        connect(addressBar, &AddressBar::pathChanged, m_mainWindow, [this](const QString& path) {
            m_mainWindow->unifiedNavigateTo(path);
        });

        connect(addressBar, &AddressBar::refreshRequested, m_mainWindow, [contentPanel]() {
            if (contentPanel) contentPanel->refreshAll();
        });
    }
=======
    // 6. 地址栏路径跳转与刷新
    if (addressBar) {
        connect(addressBar, &AddressBar::pathChanged, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(addressBar, &AddressBar::refreshRequested, &NavigationService::instance(), &NavigationService::refresh);
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 构建步骤
由于在 `CMakeLists.txt` 中引入了含有 `Q_OBJECT` 的全新类 `NavigationService`：
1. 确保在 CMake 配置中正常生成 MOC 编译文件：
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```

### 4.2 验证用例
1. **启动与持久化路径校验**：启动应用，校验上次退出的路径（如 `C:/Windows`）能否自动通过 `NavigationService` 加载。
2. **后退与前进校验**：依次导航 `C:/` -> `C:/Windows` -> `C:/Users`，点击“后退”按钮校验是否依次退回 `C:/Windows` 和 `C:/`，且“前进”按钮使能正常。
3. **上级目录边界校验**：处于 `C:/` 盘根目录时点击“上级”，校验是否准确返回“此电脑”（`computer://`），且此时“上级”按钮禁用。
4. **虚拟协议隔离校验**：从“此电脑”进入“回收站”，校验历史记录及地址栏显示是否为 `trash://`，且不向本地磁盘/数据库写入虚拟路径历史。
