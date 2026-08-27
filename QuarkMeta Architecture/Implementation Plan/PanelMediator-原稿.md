# QuarkMeta 面板中介者深度解耦实施方案 (PanelMediator)

## 1. 目标与范围
- 重构 `PanelMediator`：彻底废除 `MainWindow*` 宿主依赖，重构构造函数直接接收 5 个子面板与地址栏指针，回归纯粹的“多面板信号槽事件路由器”。
- 彻底拔除友元侵入：从 `MainWindow.h` 中删除 `friend class PanelMediator` 与 `friend class GlobalShortcutController`。
- 消除 Model 裸指针穿透：废除在 Mediator 内部将 `contentPanel->model()` 强转为 `DiskItemModel*` 并下钻遍历私有容器的违规代码，改由 `ContentPanel` 与 `MetaPanel` 的标准化数据接口对接。
- 净化呈现与排版代码：将星级与颜色的 HTML 拼接、屏幕坐标居中计算从 `PanelMediator` 剥离，收敛回 `QuickLookWindow`。

---

## 2. 新增与改造核心代码实现

### 2.1 `src/ui/PanelMediator.h`
```cpp
#pragma once

#include <QObject>
#include <QPointer>

namespace QuarkMeta {

class NavPanel;
class FavoritePanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;
class AddressBar;

class PanelMediator : public QObject {
    Q_OBJECT

public:
    explicit PanelMediator(NavPanel* navPanel,
                           FavoritePanel* favoritePanel,
                           ContentPanel* contentPanel,
                           MetaPanel* metaPanel,
                           FilterPanel* filterPanel,
                           AddressBar* addressBar,
                           QObject* parent = nullptr);
    ~PanelMediator() override = default;

    /**
     * @brief 建立各独立面板之间的单向信号槽路由网络
     */
    void setupConnections();

private:
    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;
    QPointer<AddressBar> m_addressBar;

    QString m_currentQuickLookPath;
};

} // namespace QuarkMeta
```

### 2.2 `src/ui/PanelMediator.cpp`
```cpp
#include "PanelMediator.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "AddressBar.h"
#include "QuickLookWindow.h"
#include "ToolTipOverlay.h"
#include "../core/NavigationService.h"
#include "../core/TrashService.h"
#include "../core/CoreEngine.h"
#include "../core/CentralEventHub.h"
#include "../core/VolumeOnlineManager.h"
#include "../core/ModelContract.h"
#include "../util/ShellHelper.h"
#include <QFileInfo>
#include <QCursor>

namespace QuarkMeta {

PanelMediator::PanelMediator(NavPanel* navPanel,
                             FavoritePanel* favoritePanel,
                             ContentPanel* contentPanel,
                             MetaPanel* metaPanel,
                             FilterPanel* filterPanel,
                             AddressBar* addressBar,
                             QObject* parent)
    : QObject(parent),
      m_navPanel(navPanel),
      m_favoritePanel(favoritePanel),
      m_contentPanel(contentPanel),
      m_metaPanel(metaPanel),
      m_filterPanel(filterPanel),
      m_addressBar(addressBar) {
}

void PanelMediator::setupConnections() {
    NavPanel* navPanel = m_navPanel;
    FavoritePanel* favoritePanel = m_favoritePanel;
    ContentPanel* contentPanel = m_contentPanel;
    MetaPanel* metaPanel = m_metaPanel;
    FilterPanel* filterPanel = m_filterPanel;
    AddressBar* addressBar = m_addressBar;

    // -------------------------------------------------------------
    // 1. 导航/收藏/内容/地址栏 触发跳转 -> NavigationService
    // -------------------------------------------------------------
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
        connect(favoritePanel, &FavoritePanel::requestLocateFile, this, [contentPanel](const QString& path) {
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

        // 收藏夹添加联动
        if (favoritePanel) {
            connect(contentPanel, &ContentPanel::requestAddFavorite, favoritePanel, [favoritePanel](const QStringList& paths) {
                for (const QString& p : paths) {
                    favoritePanel->addFavoriteItem(p);
                }
                favoritePanel->saveFavorites();
            });
        }
    }

    if (addressBar) {
        connect(addressBar, &AddressBar::pathChanged, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });
        connect(addressBar, &AddressBar::refreshRequested, &NavigationService::instance(), &NavigationService::refresh);
    }

    // -------------------------------------------------------------
    // 2. NavigationService 路径变更 -> 驱动各视图响应
    // -------------------------------------------------------------
    connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, this,
            [contentPanel, addressBar, navPanel](const QString& url, const QString& displayPath) {
        if (addressBar) addressBar->setPath(displayPath);
        if (navPanel) navPanel->selectPath(url);

        if (contentPanel) {
            if (url == "computer://") {
                contentPanel->loadDirectory("");
            } else if (url == "trash://") {
                contentPanel->loadCategory("trash");
            } else {
                contentPanel->loadDirectory(url);
            }
        }
    });

    connect(&VolumeOnlineManager::instance(), &VolumeOnlineManager::volumeStateChanged, this,
            [](const QString& driveLetter, bool isOnline) {
        if (!isOnline) {
            QString current = NavigationService::instance().currentUrl();
            if (current.contains(driveLetter + ":", Qt::CaseInsensitive)) {
                NavigationService::instance().navigateTo("computer://");
            }
        }
    });

    // -------------------------------------------------------------
    // 3. 内容面板选区改变 -> 元数据面板标准角色单向绑定
    // -------------------------------------------------------------
    if (contentPanel && metaPanel) {
        connect(contentPanel, &ContentPanel::selectionChanged, metaPanel, [contentPanel, metaPanel](const QStringList& paths) {
            metaPanel->setSelectedPaths(paths);
            if (paths.isEmpty()) {
                metaPanel->setImagePreview(QPixmap());
                metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
                metaPanel->setRating(0, false);
                metaPanel->setColor(L"", false);
                metaPanel->setTags(QStringList());
                metaPanel->setNote(L"");
                metaPanel->setURL(L"");
                metaPanel->setPalettes({});
            } else {
                QModelIndexList selectedIndices = contentPanel->getSelectedIndexes();
                QModelIndex idx = selectedIndices.isEmpty() ? QModelIndex() : selectedIndices.first();

                QString path = paths.first();
                QFileInfo fi(path);

                QString name = idx.isValid() ? idx.sibling(idx.row(), 0).data(Qt::DisplayRole).toString() : fi.fileName();
                QString type = idx.isValid() ? ((idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), 4).data(Qt::DisplayRole).toString() + " 文件") : (fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件");
                QString sizeStr = idx.isValid() ? idx.sibling(idx.row(), 5).data(Qt::DisplayRole).toString() : "-";
                QString mtimeStr = idx.isValid() ? idx.sibling(idx.row(), 6).data(Qt::DisplayRole).toString() : "-";

                metaPanel->updateInfo(
                    name, type, sizeStr, "-", mtimeStr, "-",
                    path, idx.data(EncryptedRole).toBool(), 0, 0
                );
                metaPanel->setRating(idx.data(RatingRole).toInt(), false);
                metaPanel->setColor(idx.data(ColorRole).toString().toStdWString(), false);
                metaPanel->setTags(idx.data(TagsRole).toStringList());
                
                QVariant decData = idx.data(Qt::DecorationRole);
                QPixmap previewPixmap;
                if (decData.canConvert<QIcon>()) {
                    previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                } else if (decData.canConvert<QPixmap>()) {
                    previewPixmap = decData.value<QPixmap>();
                }
                metaPanel->setImagePreview(previewPixmap);
            }
        });
    }

    // -------------------------------------------------------------
    // 4. 内容面板与 QuickLook 预览窗口联动
    // -------------------------------------------------------------
    if (contentPanel) {
        connect(contentPanel, &ContentPanel::requestQuickLook, this, [this](const QString& path) {
            m_currentQuickLookPath = path;
            QuickLookWindow::instance().previewFile(path);
        });
    }

    connect(&QuickLookWindow::instance(), &QuickLookWindow::prevRequested, this, [this, contentPanel]() {
        if (!contentPanel) return;
        QString prev = contentPanel->getAdjacentFilePath(m_currentQuickLookPath, -1);
        if (!prev.isEmpty()) {
            m_currentQuickLookPath = prev;
            QuickLookWindow::instance().previewFile(prev);
            contentPanel->selectAndScrollToPath(prev);
        }
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::nextRequested, this, [this, contentPanel]() {
        if (!contentPanel) return;
        QString next = contentPanel->getAdjacentFilePath(m_currentQuickLookPath, 1);
        if (!next.isEmpty()) {
            m_currentQuickLookPath = next;
            QuickLookWindow::instance().previewFile(next);
            contentPanel->selectAndScrollToPath(next);
        }
    });

    // 预览窗口内修改星级与颜色 -> 执行 AppCommand 派发
    connect(&QuickLookWindow::instance(), &QuickLookWindow::ratingRequested, this, [this, metaPanel](int rating) {
        if (m_currentQuickLookPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetRating;
        cmd.targetPaths << m_currentQuickLookPath;
        cmd.params["rating"] = rating;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setRating(rating);
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::colorRequested, this, [this, metaPanel](const QString& color) {
        if (m_currentQuickLookPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths << m_currentQuickLookPath;
        cmd.params["color"] = color;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setColor(color.toStdWString());
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::deleteRequested, this, [this, contentPanel](const QString& path) {
        if (path.isEmpty()) return;

        // 统一交由 TrashService 执行安全移入回收站
        if (TrashService::instance().moveToTrash({path})) {
            if (contentPanel) {
                QString next = contentPanel->getAdjacentFilePath(path, 1);
                if (!next.isEmpty()) {
                    m_currentQuickLookPath = next;
                    QuickLookWindow::instance().previewFile(next);
                } else {
                    QString prev = contentPanel->getAdjacentFilePath(path, -1);
                    if (!prev.isEmpty()) {
                        m_currentQuickLookPath = prev;
                        QuickLookWindow::instance().previewFile(prev);
                    } else {
                        QuickLookWindow::instance().closePreview();
                    }
                }
                contentPanel->refreshAll();
            }
        }
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::favoriteRequested, this, [favoritePanel](const QString& path) {
        if (!path.isEmpty() && favoritePanel) {
            favoritePanel->addFavoriteItem(path);
            favoritePanel->saveFavorites();
            ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, QColor("#2ecc71"));
        }
    });

    // -------------------------------------------------------------
    // 5. 统计与过滤联动
    // -------------------------------------------------------------
    if (contentPanel && filterPanel) {
        connect(contentPanel, &ContentPanel::directoryStatsReady, filterPanel, [filterPanel](const ScanStats& stats) {
            filterPanel->populateStats(stats);
            AppEvent ev;
            ev.type = AppEventType::FilterStateChanged;
            CentralEventHub::instance().publishEvent(ev);
        });

        connect(filterPanel, &FilterPanel::filterChanged, contentPanel, [contentPanel](const FilterState& state) {
            contentPanel->applyFilters(state);
        });
    }

    // -------------------------------------------------------------
    // 6. 元数据面板属性修改 -> 驱动 CoreEngine 与 ContentPanel 同步
    // -------------------------------------------------------------
    if (metaPanel && contentPanel) {
        connect(metaPanel, &MetaPanel::metadataChanged, contentPanel, [contentPanel](int rating, const std::wstring& color) {
            auto indexes = contentPanel->getSelectedIndexes();
            QStringList paths;
            for (const auto& idx : indexes) {
                QString path = idx.data(PathRole).toString(); 
                if (!path.isEmpty()) paths << path;
            }
            if (paths.isEmpty()) return;

            if (rating != -1) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetRating;
                cmd.targetPaths = paths;
                cmd.params["rating"] = rating;
                CoreEngine::instance().executeCommand(cmd);
            }
            if (color != L"__NO_CHANGE__") {
                AppCommand cmd;
                cmd.type = AppCommandType::SetColor;
                cmd.targetPaths = paths;
                cmd.params["color"] = QString::fromStdWString(color);
                CoreEngine::instance().executeCommand(cmd);
            }
        });

        connect(metaPanel, &MetaPanel::tagAddRequested, contentPanel, [contentPanel](const QStringList& paths, const QString& newTag) { 
            if (!paths.isEmpty() && !newTag.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::AddTag;
                cmd.targetPaths = paths;
                cmd.params["tag"] = newTag;
                CoreEngine::instance().executeCommand(cmd);
                for (const QString& p : paths) {
                    contentPanel->updateItemMetadata(p);
                }
            }
        }); 

        connect(metaPanel, &MetaPanel::tagRemoveRequested, contentPanel, [contentPanel](const QStringList& paths, const QString& removeTag) { 
            if (!paths.isEmpty() && !removeTag.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::RemoveTag;
                cmd.targetPaths = paths;
                cmd.params["tag"] = removeTag;
                CoreEngine::instance().executeCommand(cmd);
                for (const QString& p : paths) {
                    contentPanel->updateItemMetadata(p);
                }
            }
        }); 

        connect(metaPanel, &MetaPanel::tagsChanged, contentPanel, [contentPanel](const QStringList& paths, const QStringList&) {
            for (const QString& p : paths) {
                contentPanel->updateItemMetadata(p);
            }
        });

        if (filterPanel) {
            connect(metaPanel, &MetaPanel::searchByColor, filterPanel, [filterPanel](const QColor& color) {
                filterPanel->selectColor(color);
            });
        }

        connect(metaPanel, &MetaPanel::renameRequested, contentPanel, [contentPanel](const QString& oldPath, const QString& newPath) {
            if (ShellHelper::renameItem(oldPath, newPath)) {
                contentPanel->migrateModelCache(oldPath, newPath);
                contentPanel->refreshAll();
            } else {
                contentPanel->updateItemMetadata(oldPath);
            }
        });

        connect(metaPanel, &MetaPanel::noteEdited, this, [](const QStringList& paths, const QString& newNote) {
            if (!paths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetNote;
                cmd.targetPaths = paths;
                cmd.params["note"] = newNote;
                CoreEngine::instance().executeCommand(cmd);
            }
        });

        connect(metaPanel, &MetaPanel::linkEdited, this, [](const QStringList& paths, const QString& newLink) {
            if (!paths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetURL;
                cmd.targetPaths = paths;
                cmd.params["url"] = newLink;
                CoreEngine::instance().executeCommand(cmd);
            }
        });
    }

    // -------------------------------------------------------------
    // 7. 全局事件总线 CentralEventHub 增量通知响应
    // -------------------------------------------------------------
    connect(&CentralEventHub::instance(), &CentralEventHub::eventOccurred, this, [contentPanel](const QuarkMeta::AppEvent& event) {
        if (!contentPanel) return;

        if (event.type == QuarkMeta::AppEventType::MetadataUpdated) {
            if (!event.targetPath.isEmpty()) {
                contentPanel->updateItemMetadata(event.targetPath);
            } else if (!event.paths.isEmpty()) {
                for (const QString& p : event.paths) {
                    contentPanel->updateItemMetadata(p);
                }
            } else {
                contentPanel->refreshAll();
            }
        } else if (event.type == QuarkMeta::AppEventType::ItemsDeleted || 
                   event.type == QuarkMeta::AppEventType::ItemsRenamed) {
            contentPanel->refreshAll();
        }
    });
}

} // namespace QuarkMeta
```

---

## 3. `MainWindow.h` 与 `MainWindow.cpp` 友元拔除与装配改造

### 3.1 `MainWindow.h` 净化
- **彻底删除 `friend class PanelMediator;` 与 `friend class GlobalShortcutController;`**。
- `MainWindow` 内部仅持有 `PanelMediator* m_panelMediator = nullptr;`。

```cpp
// MainWindow.h 净化后头部：
#pragma once

#include <QMainWindow>
#include <QPointer>

namespace QuarkMeta {

class PanelMediator;
class PanelLayoutManager;
class GlobalShortcutController;
class TaskProgressController;
class SearchController; 
class NavPanel;
class FavoritePanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;
class AddressBar;
class TaskProgressToolBar;

class MainWindow : public QMainWindow {
    Q_OBJECT

    // 🚨 彻底拔除友元特权！
    // 严禁 friend class PanelMediator;
    // 严禁 friend class GlobalShortcutController;

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // ...
```

### 3.2 `MainWindow.cpp` 装配改造
- 在 `setupSplitters()` 中，将构建好的 5 个子面板与地址栏指针显式传给 `PanelMediator` 进行连接装配。

```cpp
// MainWindow.cpp 中 setupSplitters 关键区段：

void MainWindow::setupSplitters() {
    // ... [构建 navPanel, favoritePanel, contentPanel, metaPanel, filterPanel, panelLayoutManager 保持不变] ...

    // 🚀【纯净解耦构建】：PanelMediator 仅作为外部路由器装配，主窗口完全私有化！
    m_panelMediator = new PanelMediator(
        m_navPanel,
        m_favoritePanel,
        m_contentPanel,
        m_metaPanel,
        m_filterPanel,
        m_addressBar,
        this
    );
    m_panelMediator->setupConnections();

    // ... [其余装配保持不变] ...
}
```