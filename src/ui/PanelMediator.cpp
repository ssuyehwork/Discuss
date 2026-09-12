#include "PanelMediator.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "AddressBar.h"
#include "SearchController.h"
#include "TitleBarWidget.h"
#include "PanelLayoutManager.h"
#include "AppShortcutController.h"
#include "QuickLookWindow.h"
#include "ToolTipOverlay.h"
#include "../core/NavigationService.h"
#include "../core/TrashService.h"
#include "../core/CoreEngine.h"
#include "../core/CentralEventHub.h"
#include "../core/VolumeOnlineManager.h"
#include "../core/ModelContract.h"
#include "../core/AppConfig.h"
#include "../util/ShellHelper.h"
#include "../meta/MetadataManager.h"
#include "UiHelper.h"
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDesktopServices>
#include <QCursor>

namespace QuarkMeta {

PanelMediator::PanelMediator(const PanelMediatorComponents& components, QObject* parent)
    : QObject(parent),
      m_navPanel(components.navPanel),
      m_favoritePanel(components.favoritePanel),
      m_contentPanel(components.contentPanel),
      m_metaPanel(components.metaPanel),
      m_filterPanel(components.filterPanel),
      m_addressBar(components.addressBar),
      m_searchController(components.searchController),
      m_titleBar(components.titleBar),
      m_layoutManager(components.layoutManager),
      m_shortcutController(components.shortcutController) {
}

void PanelMediator::setupConnections() {
    NavPanel* navPanel = m_navPanel;
    FavoritePanel* favoritePanel = m_favoritePanel;
    ContentPanel* contentPanel = m_contentPanel;
    MetaPanel* metaPanel = m_metaPanel;
    FilterPanel* filterPanel = m_filterPanel;
    AddressBar* addressBar = m_addressBar;
    SearchController* searchController = m_searchController;
    TitleBarWidget* titleBar = m_titleBar;
    PanelLayoutManager* layoutManager = m_layoutManager;
    AppShortcutController* shortcutController = m_shortcutController;

    // 0. TitleBar 与各组件的高阶编排及 UI 状态恢复/持久化
    if (titleBar) {
        if (layoutManager) {
            connect(titleBar, &TitleBarWidget::layoutMenuRequested, layoutManager, [layoutManager](const QPoint& pos) {
                layoutManager->showPanelContextMenu(pos);
            });
        }
        if (contentPanel) {
            connect(titleBar, &TitleBarWidget::viewModeRequested, contentPanel, [contentPanel](TitleBarWidget::ViewModeOption option) {
                ContentPanel::ViewMode targetMode = ContentPanel::GridView;
                if (option == TitleBarWidget::JustifiedViewMode) targetMode = ContentPanel::JustifiedViewMode;
                else if (option == TitleBarWidget::GridViewMode) targetMode = ContentPanel::GridView;
                else if (option == TitleBarWidget::ListViewMode) targetMode = ContentPanel::ListView;
                else if (option == TitleBarWidget::ColumnViewMode) targetMode = ContentPanel::ColumnView;

                contentPanel->setViewMode(targetMode);
            });

            connect(titleBar, &TitleBarWidget::createItemRequested, contentPanel, [contentPanel](const QString& type) {
                contentPanel->createNewItem(type);
            });

            // 缩放级别初始化与双向同步 + 持久化
            int initZoom = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt();
            int boundZoom = qBound(30, initZoom, 230);
            titleBar->setZoomLevel(boundZoom);
            contentPanel->setZoomLevel(boundZoom);

            connect(titleBar, &TitleBarWidget::zoomLevelChanged, this, [contentPanel](int value) {
                if (contentPanel) contentPanel->setZoomLevel(value);
                AppConfig::instance().setValue("UI/GridZoomLevel", value);
            });

            connect(contentPanel, &ContentPanel::zoomLevelChanged, this, [titleBar](int level) {
                if (titleBar) titleBar->setZoomLevel(level);
                AppConfig::instance().setValue("UI/GridZoomLevel", level);
            });
        }
    }

    // 快捷键沉浸模式切换下沉
    if (shortcutController && layoutManager) {
        connect(shortcutController, &AppShortcutController::toggleImmersiveRequested, layoutManager, [layoutManager]() {
            layoutManager->toggleImmersiveMode();
        });
    }

    // 搜索控制器与 ContentPanel 绑定及状态更新
    if (searchController) {
        if (contentPanel) {
            searchController->bindContentPanel(contentPanel);
        }
        connect(searchController, &SearchController::searchExecuted, this, [this]() {
            emit statusMessageRequested("搜索已完成");
        });
    }

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
                contentPanel->loadDirectory("");
            } else if (url == "trash://") {
                contentPanel->loadCategory("trash");
            } else {
                contentPanel->loadDirectory(url);
            }
        }
    });

    if (navPanel) {
        connect(navPanel, &NavPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(navPanel, &NavPanel::requestOpenTrash, &NavigationService::instance(), []() {
            NavigationService::instance().navigateTo("trash://");
        });

        if (favoritePanel) {
            connect(navPanel, &NavPanel::requestAddFavorite, favoritePanel, [favoritePanel](const QString& path) {
                favoritePanel->addFavoriteItem(path);
                favoritePanel->saveFavorites();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, QColor("#2ecc71"));
            });
            connect(navPanel, &NavPanel::requestRemoveFavorite, favoritePanel, [favoritePanel](const QString& path) {
                favoritePanel->removeFavoriteItem(path);
                favoritePanel->saveFavorites();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已从收藏夹移除", 1500, QColor("#e74c3c"));
            });
        }
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

        if (favoritePanel) {
            connect(contentPanel, &ContentPanel::requestAddFavorite, favoritePanel, [favoritePanel](const QStringList& paths) {
                for (const QString& p : paths) {
                    favoritePanel->addFavoriteItem(p);
                }
                favoritePanel->saveFavorites();
            });
            connect(contentPanel, &ContentPanel::requestRemoveFavorite, favoritePanel, [favoritePanel](const QStringList& paths) {
                for (const QString& p : paths) {
                    favoritePanel->removeFavoriteItem(p);
                }
                favoritePanel->saveFavorites();
            });
        }
    }

    connect(&VolumeOnlineManager::instance(), &VolumeOnlineManager::volumeStateChanged, this,
            [](const QString& driveLetter, bool isOnline) {
        if (!isOnline) {
            QString current = NavigationService::instance().currentUrl();
            if (current.contains(driveLetter + ":", Qt::CaseInsensitive)) {
                NavigationService::instance().navigateTo("computer://");
            }
        }
    });

    // 2. 内容面板选中项改变 -> 元数据面板 0 毫秒极速同步
    if (contentPanel && metaPanel) {
        // 监听卡片/列表上的就地修改，0 毫秒同步右侧 MetaPanel
        connect(contentPanel->model(), &QAbstractItemModel::dataChanged, metaPanel, 
                [contentPanel, metaPanel](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>& roles) {
            if (!roles.isEmpty() && !roles.contains(RatingRole) && !roles.contains(ColorRole) && !roles.contains(TagsRole) && !roles.contains(NoteRole) && !roles.contains(UrlRole)) {
                return;
            }

            QModelIndexList selected = contentPanel->getSelectedIndexes();
            if (selected.isEmpty()) return;

            QModelIndex currentSel = selected.first();
            QString selPath = currentSel.data(PathRole).toString();
            QString changedPath = topLeft.data(PathRole).toString();

            if (!selPath.isEmpty() && QString::compare(selPath, changedPath, Qt::CaseInsensitive) == 0) {
                if (roles.isEmpty() || roles.contains(RatingRole)) {
                    int newRating = currentSel.data(RatingRole).toInt();
                    metaPanel->setRating(newRating, false);
                }
                if (roles.isEmpty() || roles.contains(ColorRole)) {
                    QString newColor = currentSel.data(ColorRole).toString();
                    metaPanel->setColor(newColor, false);
                }
                if (roles.isEmpty() || roles.contains(TagsRole)) {
                    metaPanel->setTags(currentSel.data(TagsRole).toStringList());
                }
                if (roles.isEmpty() || roles.contains(NoteRole)) {
                    metaPanel->setNote(currentSel.data(NoteRole).toString());
                }
                if (roles.isEmpty() || roles.contains(UrlRole)) {
                    metaPanel->setURL(currentSel.data(UrlRole).toString());
                }
            }
        });

        connect(contentPanel, &ContentPanel::selectionChanged, metaPanel, [contentPanel, metaPanel](const QStringList& paths) {
            metaPanel->setSelectedPaths(paths);
            qDebug() << "[PanelMediator] selectionChanged triggered. Selected paths count:" << paths.size() << "paths:" << paths;
            if (paths.isEmpty()) {
                qDebug() << "[PanelMediator] Selection is empty. Clearing MetaPanel.";
                metaPanel->setImagePreview(QPixmap());
                metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
                metaPanel->setRating(0, false);
                metaPanel->setColor(QString(""), false);
                metaPanel->setTags(QStringList());
                metaPanel->setNote(QString(""));
                metaPanel->setURL(QString(""));
                metaPanel->setPalettes({});
            } else if (paths.size() == 1) {
                QModelIndexList selectedIndices = contentPanel->getSelectedIndexes();
                QModelIndex idx = selectedIndices.isEmpty() ? QModelIndex() : selectedIndices.first();

                QString path = paths.first();
                qDebug() << "[PanelMediator] Single item selected path:" << path << "idx.isValid:" << idx.isValid();
                QFileInfo fi(path);

                QString name = idx.isValid() ? idx.sibling(idx.row(), 0).data(Qt::DisplayRole).toString() : fi.fileName();
                QString type = idx.isValid() ? ((idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), 4).data(Qt::DisplayRole).toString() + " 文件") : (fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件");
                QString sizeStr = idx.isValid() ? idx.sibling(idx.row(), 5).data(Qt::DisplayRole).toString() : "-";
                QString mtimeStr = idx.isValid() ? idx.sibling(idx.row(), 6).data(Qt::DisplayRole).toString() : "-";
                bool encrypted = idx.isValid() ? idx.data(EncryptedRole).toBool() : false;

                metaPanel->updateInfo(
                    name, type, sizeStr, "-", mtimeStr, "-",
                    path, encrypted, 0, 0
                );

                // 🚀【双保险装载】：idx 有效走 Model，idx 无效通过 MetadataManager 兜底，扩展属性补充融合，绝不丢弃高级元数据
                auto meta = MetadataManager::instance().getMeta(path.toStdWString());
                QVector<QPair<QColor, float>> qPalettes;
                qPalettes.reserve(static_cast<int>(meta.palettes.size()));
                for (const auto& entry : meta.palettes) {
                    qPalettes.append(qMakePair(entry.color, entry.ratio));
                }

                if (idx.isValid()) {
                    int rating = idx.data(RatingRole).toInt();
                    QString color = idx.data(ColorRole).toString();
                    QStringList tags = idx.data(TagsRole).toStringList();
                    QString note = idx.data(NoteRole).toString();
                    QString url = idx.data(UrlRole).toString();

                    int finalRating = rating > 0 ? rating : meta.rating;
                    QString finalColor = !color.isEmpty() ? color : QString::fromStdWString(meta.manualColor);
                    QStringList finalTags = !tags.isEmpty() ? tags : meta.tags;
                    QString finalNote = !note.isEmpty() ? note : QString::fromStdWString(meta.note);
                    QString finalUrl = !url.isEmpty() ? url : QString::fromStdWString(meta.url);

                    qDebug() << "[PanelMediator] Merged Meta -> Rating:" << finalRating
                             << "Color:" << finalColor
                             << "Tags:" << finalTags
                             << "Note:" << finalNote
                             << "URL:" << finalUrl
                             << "Palettes count:" << qPalettes.size();

                    metaPanel->setRating(finalRating, false);
                    metaPanel->setColor(finalColor, false);
                    metaPanel->setTags(finalTags);
                    metaPanel->setNote(finalNote);
                    metaPanel->setURL(finalUrl);
                    metaPanel->setPalettes(qPalettes);

                    QVariant decData = idx.data(Qt::DecorationRole);
                    QPixmap previewPixmap;
                    if (decData.canConvert<QIcon>()) {
                        previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                    } else if (decData.canConvert<QPixmap>()) {
                        previewPixmap = decData.value<QPixmap>();
                    }
                    metaPanel->setImagePreview(previewPixmap);
                } else {
                    metaPanel->setRating(meta.rating, false);
                    metaPanel->setColor(QString::fromStdWString(meta.manualColor), false);
                    metaPanel->setTags(meta.tags);
                    metaPanel->setNote(QString::fromStdWString(meta.note));
                    metaPanel->setURL(QString::fromStdWString(meta.url));
                    metaPanel->setPalettes(qPalettes);
                    metaPanel->setImagePreview(QPixmap());
                }
            }
        });
    }

    // 3. 内容面板与 QuickLook 预览窗口联动 (🚀 闭环补齐内容同步)
    if (contentPanel) {
        connect(contentPanel, &ContentPanel::requestQuickLook, this, [this](const QString& path) {
            m_currentQuickLookPath = path;
            QuickLookWindow::instance().previewFile(path);
        });

        connect(contentPanel, &ContentPanel::fileActivated, this, [this](const QString& path) {
            AppCommand cmd;
            cmd.type = AppCommandType::RecordAccess;
            cmd.targetPaths << path;
            CoreEngine::instance().executeCommand(cmd);

            if (UiHelper::canPreviewFile(path)) {
                m_currentQuickLookPath = path;
                QuickLookWindow::instance().previewFile(path);
            } else {
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            }
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

    // QuickLook 改星级 -> 同步更新内容面板卡片
    connect(&QuickLookWindow::instance(), &QuickLookWindow::ratingRequested, this, [this, metaPanel, contentPanel](int rating) {
        if (m_currentQuickLookPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetRating;
        cmd.targetPaths << m_currentQuickLookPath;
        cmd.params["rating"] = rating;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setRating(rating, false);
        if (contentPanel) contentPanel->updateItemMetadata(m_currentQuickLookPath);
    });

    // QuickLook 改颜色 -> 同步更新内容面板卡片
    connect(&QuickLookWindow::instance(), &QuickLookWindow::colorRequested, this, [this, metaPanel, contentPanel](const QString& color) {
        if (m_currentQuickLookPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths << m_currentQuickLookPath;
        cmd.params["color"] = color;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setColor(color, false);
        if (contentPanel) contentPanel->updateItemMetadata(m_currentQuickLookPath);
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::deleteRequested, this, [this, contentPanel](const QString& path) {
        if (path.isEmpty()) return;

        if (TrashService::instance().moveToTrash({path}, contentPanel)) {
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
            if (favoritePanel->containsPath(path)) {
                favoritePanel->removeFavoriteItem(path);
                favoritePanel->saveFavorites();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已从收藏夹移除", 1500, QColor("#e74c3c"));
            } else {
                favoritePanel->addFavoriteItem(path);
                favoritePanel->saveFavorites();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, QColor("#2ecc71"));
            }
        }
    });

    // 4. 统计与过滤联动
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

    // 5. 地址栏路径跳转与刷新
    if (addressBar) {
        connect(addressBar, &AddressBar::pathChanged, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(addressBar, &AddressBar::refreshRequested, &NavigationService::instance(), &NavigationService::refresh);

        if (favoritePanel) {
            connect(addressBar, &AddressBar::requestAddFavorite, favoritePanel, [favoritePanel](const QString& path) {
                if (favoritePanel->containsPath(path)) {
                    favoritePanel->removeFavoriteItem(path);
                    favoritePanel->saveFavorites();
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "已从收藏夹移除", 1500, QColor("#e74c3c"));
                } else {
                    favoritePanel->addFavoriteItem(path);
                    favoritePanel->saveFavorites();
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, QColor("#2ecc71"));
                }
            });
            connect(addressBar, &AddressBar::requestRemoveFavorite, favoritePanel, [favoritePanel](const QString& path) {
                favoritePanel->removeFavoriteItem(path);
                favoritePanel->saveFavorites();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已从收藏夹移除", 1500, QColor("#e74c3c"));
            });
        }
    }

    // 6. 响应元数据面板解耦信号 -> 驱动 CoreEngine 与 ContentPanel 同步
    if (metaPanel && contentPanel) {
        connect(metaPanel, &MetaPanel::ratingChanged, contentPanel, [contentPanel](const QStringList& paths, int rating) {
            if (paths.isEmpty()) return;
            AppCommand cmd;
            cmd.type = AppCommandType::SetRating;
            cmd.targetPaths = paths;
            cmd.params["rating"] = rating;
            CoreEngine::instance().executeCommand(cmd);
            for (const QString& p : paths) {
                contentPanel->updateItemMetadata(p);
            }
            contentPanel->recalculateAndEmitStats();
        });

        connect(metaPanel, &MetaPanel::colorChanged, contentPanel, [contentPanel](const QStringList& paths, const QString& hexColor) {
            if (paths.isEmpty()) return;
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = paths;
            cmd.params["color"] = hexColor;
            CoreEngine::instance().executeCommand(cmd);
            for (const QString& p : paths) {
                contentPanel->updateItemMetadata(p);
            }
            contentPanel->recalculateAndEmitStats();
        });

        connect(metaPanel, &MetaPanel::primaryColorChanged, contentPanel, [contentPanel](const QString& path, const QColor& color) {
            if (path.isEmpty()) return;
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = {path};
            cmd.params["color"] = color.name(QColor::HexRgb);
            CoreEngine::instance().executeCommand(cmd);
            contentPanel->updateItemMetadata(path);
            contentPanel->recalculateAndEmitStats();
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
                contentPanel->recalculateAndEmitStats();
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
                contentPanel->recalculateAndEmitStats();
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

        connect(metaPanel, &MetaPanel::noteEdited, contentPanel, [contentPanel](const QStringList& paths, const QString& newNote) {
            if (!paths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetNote;
                cmd.targetPaths = paths;
                cmd.params["note"] = newNote;
                CoreEngine::instance().executeCommand(cmd);
                for (const QString& p : paths) {
                    contentPanel->updateItemMetadata(p);
                }
            }
        });

        connect(metaPanel, &MetaPanel::linkEdited, contentPanel, [contentPanel](const QStringList& paths, const QString& newLink) {
            if (!paths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetURL;
                cmd.targetPaths = paths;
                cmd.params["url"] = newLink;
                CoreEngine::instance().executeCommand(cmd);
                for (const QString& p : paths) {
                    contentPanel->updateItemMetadata(p);
                }
            }
        });
    }

    // 7. 全局事件总线 CentralEventHub 增量通知响应
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
            contentPanel->recalculateAndEmitStats();
        } else if (event.type == QuarkMeta::AppEventType::ItemsDeleted ||
                   event.type == QuarkMeta::AppEventType::ItemsRenamed) {
            contentPanel->refreshAll();
        }
    });
}

} // namespace QuarkMeta
