#include "PanelMediator.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "ContentPanel.h"
#include "ColumnViewWidget.h"
#include "controllers/ContentPaneSplitManager.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "AddressBar.h"
#include "SearchController.h"
#include "TitleBarWidget.h"
#include "TabBarWidget.h"
#include "PanelLayoutManager.h"
#include "AppShortcutController.h"
#include "QuickLookWindow.h"
#include "ToolTipOverlay.h"
#include "Logger.h"
#include <QElapsedTimer>
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
        if (titleBar->tabBar()) {
            // 切出旧 Tab 前：快照导出旧 Tab 的完整分屏树
            connect(titleBar->tabBar(), &TabBarWidget::tabAboutToChange, this, [this, titleBar, contentPanel](int oldIndex) {
                ContentPanel* root = contentPanel ? contentPanel->rootPane() : nullptr;
                if (root && root->splitManager()) {
                    TabSplitState state = root->splitManager()->exportSplitState();
                    titleBar->tabBar()->setTabSplitState(oldIndex, state);
                }
            });

            // 切换到新 Tab：还原新 Tab 的专属分屏树并通知 NavigationService
            connect(titleBar->tabBar(), &TabBarWidget::currentTabChanged, this, [this, titleBar, contentPanel](int index, const QString& url) {
                ContentPanel* root = contentPanel ? contentPanel->rootPane() : nullptr;
                if (root && root->splitManager() && titleBar->tabBar()) {
                    TabSplitState state = titleBar->tabBar()->tabSplitState(index);
                    root->splitManager()->restoreSplitState(state);
                }
                NavigationService::instance().navigateTo(url);
            });

            connect(titleBar->tabBar(), &TabBarWidget::refreshRequested, this, []() {
                NavigationService::instance().refresh();
            });

            connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, this, [this, titleBar, contentPanel](const QString& url, const QString& displayPath) {
                Q_UNUSED(url);
                Q_UNUSED(displayPath);
                if (titleBar->tabBar()) {
                    ContentPanel* root = contentPanel ? contentPanel->rootPane() : nullptr;
                    if (root && root->splitManager()) {
                        TabSplitState state = root->splitManager()->exportSplitState();
                        titleBar->tabBar()->updateSplitTabTitle(state);
                    }
                }
            });
        }
        if (layoutManager) {
            connect(titleBar, &TitleBarWidget::layoutMenuRequested, layoutManager, [layoutManager](const QPoint& pos) {
                layoutManager->showPanelContextMenu(pos);
            });
        }
        if (contentPanel) {
            connect(titleBar, &TitleBarWidget::viewModeRequested, this, [this, contentPanel](TitleBarWidget::ViewModeOption option) {
                ContentPanel::ViewMode targetMode = ContentPanel::GridView;
                if (option == TitleBarWidget::JustifiedViewMode) targetMode = ContentPanel::JustifiedViewMode;
                else if (option == TitleBarWidget::GridViewMode) targetMode = ContentPanel::GridView;
                else if (option == TitleBarWidget::ListViewMode) targetMode = ContentPanel::ListView;
                else if (option == TitleBarWidget::ColumnViewMode) targetMode = ContentPanel::ColumnView;

                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) target->setViewMode(targetMode);
            });

            connect(titleBar, &TitleBarWidget::createItemRequested, this, [this, contentPanel](const QString& type) {
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) target->createNewItem(type);
            });

            // 缩放级别初始化与双向同步 + 持久化
            int initZoom = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt();
            int boundZoom = qBound(30, initZoom, 230);
            titleBar->setZoomLevel(boundZoom);
            contentPanel->setZoomLevel(boundZoom);

            connect(titleBar, &TitleBarWidget::zoomLevelChanged, this, [this, contentPanel](int value) {
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) target->setZoomLevel(value);
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
            [this, contentPanel, addressBar, navPanel, filterPanel, searchController](const QString& url, const QString& displayPath) {
        if (searchController && searchController->searchEdit()) {
            searchController->searchEdit()->blockSignals(true);
            searchController->searchEdit()->clear();
            searchController->searchEdit()->blockSignals(false);
        }
        ContentPanel* targetPanel = (m_activeContentPanel && m_activeContentPanel->isVisible()) ? m_activeContentPanel.data() : contentPanel;
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

        connect(favoritePanel, &FavoritePanel::requestLocateFile, this, [this, contentPanel](const QString& path) {
            QFileInfo fi(path);
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->setPendingSelectName(fi.fileName(), false);
            }
            NavigationService::instance().navigateTo(fi.absolutePath());
        });
    }

    if (contentPanel) {
        connect(contentPanel, &ContentPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(contentPanel, &ContentPanel::dualPanePathsChanged, this, [this, titleBar, contentPanel](const QString&, const QString&) {
            if (titleBar && titleBar->tabBar()) {
                ContentPanel* root = contentPanel ? contentPanel->rootPane() : nullptr;
                if (root && root->splitManager()) {
                    TabSplitState state = root->splitManager()->exportSplitState();
                    titleBar->tabBar()->updateSplitTabTitle(state);
                }
            }
        });

        if (contentPanel->columnView()) {
            connect(contentPanel->columnView(), &ColumnViewWidget::pathNavigated, this, [filterPanel](const QString& path) {
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                NavigationService::instance().navigateTo(path);
            });
        }

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

    // 2. 内容面板选中项改变 / 界面数据修改 -> 元数据面板 0 毫秒极速同步
    if (contentPanel && metaPanel) {
        auto updateMetaPanelFromPanel = [metaPanel](ContentPanel* panel) {
            if (!panel || !metaPanel) return;
            QStringList paths = panel->getSelectedPaths();
            metaPanel->setSelectedPaths(paths);

            if (paths.isEmpty()) {
                metaPanel->setImagePreview(QPixmap());
                metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
                metaPanel->setRating(0, false);
                metaPanel->setColor(QString(""), false);
                metaPanel->setTags(QStringList());
                metaPanel->setNote(QString(""));
                metaPanel->setURL(QString(""));
                metaPanel->setPalettes({});
            } else if (paths.size() == 1) {
                QModelIndexList selectedIndices = panel->getSelectedIndexes();
                QModelIndex idx = selectedIndices.isEmpty() ? QModelIndex() : selectedIndices.first();

                QString path = paths.first();
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
                    QStringList finalTagsList = !tags.isEmpty() ? tags : meta.tags;
                    QString finalNote = !note.isEmpty() ? note : QString::fromStdWString(meta.note);
                    QString finalUrl = !url.isEmpty() ? url : QString::fromStdWString(meta.url);

                    metaPanel->setRating(finalRating, false);
                    metaPanel->setColor(finalColor, false);
                    metaPanel->setTags(finalTagsList);
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
        };

        auto wireSelectionAndDataToMeta = [this, metaPanel, updateMetaPanelFromPanel](ContentPanel* panel) {
            if (!panel) return;

            // 1. 选中项改变时同步更新 MetaPanel
            connect(panel, &ContentPanel::selectionChanged, metaPanel, [this, panel, updateMetaPanelFromPanel](const QStringList&) {
                if (m_activeContentPanel == panel || (!m_activeContentPanel && panel == m_contentPanel.data())) {
                    updateMetaPanelFromPanel(panel);
                }
            });

            // 2. 补全主/副窗格数据变动同步：监听该面板 model 的 dataChanged，在就地修改（右键/快捷键等）时实时刷出至 MetaPanel！
            if (panel->model()) {
                connect(panel->model(), &QAbstractItemModel::dataChanged, metaPanel,
                        [this, panel, updateMetaPanelFromPanel](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>& roles) {
                    if (m_activeContentPanel != panel && (m_activeContentPanel || panel != m_contentPanel.data())) {
                        return;
                    }
                    if (!roles.isEmpty() && !roles.contains(RatingRole) && !roles.contains(ColorRole) && !roles.contains(TagsRole) && !roles.contains(NoteRole) && !roles.contains(UrlRole)) {
                        return;
                    }

                    QString changedPath = QDir::cleanPath(topLeft.data(PathRole).toString());
                    QStringList selPaths = panel->getSelectedPaths();
                    bool containsPath = false;
                    for (const QString& sp : selPaths) {
                        if (QString::compare(QDir::cleanPath(sp), changedPath, Qt::CaseInsensitive) == 0) {
                            containsPath = true;
                            break;
                        }
                    }

                    if (containsPath || selPaths.isEmpty()) {
                        updateMetaPanelFromPanel(panel);
                    }
                });
            }
        };

        m_activeContentPanel = contentPanel;

        auto bindPanelActivation = std::make_shared<std::function<void(ContentPanel*)>>();
        *bindPanelActivation = [this, addressBar, bindPanelActivation, wireSelectionAndDataToMeta](ContentPanel* panel) {
            if (!panel) return;
            connect(panel, &ContentPanel::panelActivated, this, [this, addressBar](ContentPanel* activePanel) {
                if (m_activeContentPanel != activePanel) {
                    m_activeContentPanel = activePanel;
                    emit activeContentPanelChanged(activePanel);
                }
                if (addressBar) {
                    addressBar->setPath(activePanel->currentPath());
                }
            });
            connect(panel, &ContentPanel::secondaryPaneCreated, this, [wireSelectionAndDataToMeta, bindPanelActivation](ContentPanel* pane) {
                wireSelectionAndDataToMeta(pane);
                if (*bindPanelActivation) {
                    (*bindPanelActivation)(pane);
                }
            });
        };

        (*bindPanelActivation)(contentPanel);
        wireSelectionAndDataToMeta(contentPanel);

        connect(this, &PanelMediator::activeContentPanelChanged, this, [contentPanel, updateMetaPanelFromPanel](ContentPanel* activePanel) {
            std::function<void(ContentPanel*)> updateActiveState = [&updateActiveState, activePanel](ContentPanel* node) {
                if (!node) return;
                node->setActivePane(node == activePanel);
                if (node->isSplitMode()) {
                    for (ContentPanel* pane : node->panes()) {
                        updateActiveState(pane);
                    }
                }
            };
            updateActiveState(contentPanel);
            updateMetaPanelFromPanel(activePanel);
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
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (!target) return;
        QString prev = target->getAdjacentFilePath(m_currentQuickLookPath, -1);
        if (!prev.isEmpty()) {
            m_currentQuickLookPath = prev;
            QuickLookWindow::instance().previewFile(prev);
            target->selectAndScrollToPath(prev);
        }
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::nextRequested, this, [this, contentPanel]() {
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (!target) return;
        QString next = target->getAdjacentFilePath(m_currentQuickLookPath, 1);
        if (!next.isEmpty()) {
            m_currentQuickLookPath = next;
            QuickLookWindow::instance().previewFile(next);
            target->selectAndScrollToPath(next);
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

        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (metaPanel) metaPanel->setRating(rating, false);
        if (target) target->updateItemMetadata(m_currentQuickLookPath);
    });

    // QuickLook 改颜色 -> 同步更新内容面板卡片
    connect(&QuickLookWindow::instance(), &QuickLookWindow::colorRequested, this, [this, metaPanel, contentPanel](const QString& color) {
        if (m_currentQuickLookPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths << m_currentQuickLookPath;
        cmd.params["color"] = color;
        CoreEngine::instance().executeCommand(cmd);

        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (metaPanel) metaPanel->setColor(color, false);
        if (target) target->updateItemMetadata(m_currentQuickLookPath);
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::deleteRequested, this, [this, contentPanel](const QString& path) {
        if (path.isEmpty()) return;
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;

        if (TrashService::instance().moveToTrash({path}, target)) {
            if (target) {
                QString next = target->getAdjacentFilePath(path, 1);
                if (!next.isEmpty()) {
                    m_currentQuickLookPath = next;
                    QuickLookWindow::instance().previewFile(next);
                } else {
                    QString prev = target->getAdjacentFilePath(path, -1);
                    if (!prev.isEmpty()) {
                        m_currentQuickLookPath = prev;
                        QuickLookWindow::instance().previewFile(prev);
                    } else {
                        QuickLookWindow::instance().closePreview();
                    }
                }
                target->refreshAll();
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

    // 4. 统计与过滤联动 (动态支持多分栏焦点切换)
    if (filterPanel) {
        // 用于管理当前绑定面板的连接断开，防止后台窗格发送统计冲刷侧边栏
        auto activeStatsConn = std::make_shared<QMetaObject::Connection>();

        auto bindFilterToActivePanel = [this, filterPanel, activeStatsConn](ContentPanel* activePanel) {
            if (!activePanel) return;

            // 1. 严格解绑上一个窗格的统计信号，杜绝后台非焦点窗格冲刷侧边栏
            if (*activeStatsConn) {
                QObject::disconnect(*activeStatsConn);
            }

            // 2. 绑定新焦点窗格的统计就绪信号
            *activeStatsConn = connect(activePanel, &ContentPanel::directoryStatsReady, filterPanel, [filterPanel](const ScanStats& stats) {
                filterPanel->populateStats(stats);
                AppEvent ev;
                ev.type = AppEventType::FilterStateChanged;
                CentralEventHub::instance().publishEvent(ev);
            });

            // 3. 切换焦点的瞬间：反向刷出新焦点窗格已有的 FilterState 与统计数据给 FilterPanel
            filterPanel->blockSignals(true);
            filterPanel->syncUIFromFilterState();
            filterPanel->blockSignals(false);

            // 触发当前焦点窗格重新计算并广播统计
            activePanel->recalculateAndEmitStats();
        };

        // 绑定初始主面板
        if (contentPanel) {
            bindFilterToActivePanel(contentPanel);
        }

        // 焦点分栏切换时动态绑定并同步刷出状态
        connect(this, &PanelMediator::activeContentPanelChanged, this, [this, filterPanel, bindFilterToActivePanel](ContentPanel* newActivePanel) {
            if (newActivePanel) {
                bindFilterToActivePanel(newActivePanel);
            }
        });

        // 筛选条件变动时精准应用至当前激活分栏
        connect(filterPanel, &FilterPanel::filterChanged, this, [this](const FilterState& state) {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : m_contentPanel.data();
            if (target) {
                target->applyFilters(state);
            }
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

    // 6. 响应元数据面板解耦信号 -> 驱动 CoreEngine 与当前激活 ContentPanel 同步
    if (metaPanel && contentPanel) {
        auto activeOrRootPanel = [this, contentPanel]() -> ContentPanel* {
            return m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        };

        connect(metaPanel, &MetaPanel::ratingChanged, this, [activeOrRootPanel](const QStringList& paths, int rating) {
            if (paths.isEmpty()) return;
            AppCommand cmd;
            cmd.type = AppCommandType::SetRating;
            cmd.targetPaths = paths;
            cmd.params["rating"] = rating;
            CoreEngine::instance().executeCommand(cmd);
            ContentPanel* target = activeOrRootPanel();
            if (target) {
                for (const QString& p : paths) {
                    target->updateItemMetadata(p);
                }
                target->recalculateAndEmitStats();
            }
        });

        connect(metaPanel, &MetaPanel::colorChanged, this, [activeOrRootPanel](const QStringList& paths, const QString& hexColor) {
            if (paths.isEmpty()) return;
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = paths;
            cmd.params["color"] = hexColor;
            CoreEngine::instance().executeCommand(cmd);
            ContentPanel* target = activeOrRootPanel();
            if (target) {
                for (const QString& p : paths) {
                    target->updateItemMetadata(p);
                }
                target->recalculateAndEmitStats();
            }
        });

        connect(metaPanel, &MetaPanel::primaryColorChanged, this, [activeOrRootPanel](const QString& path, const QColor& color) {
            if (path.isEmpty()) return;
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = {path};
            cmd.params["color"] = color.name(QColor::HexRgb);
            CoreEngine::instance().executeCommand(cmd);
            ContentPanel* target = activeOrRootPanel();
            if (target) {
                target->updateItemMetadata(path);
                target->recalculateAndEmitStats();
            }
        });

        connect(metaPanel, &MetaPanel::tagAddRequested, this, [activeOrRootPanel](const QStringList& paths, const QString& newTag) {
            if (!paths.isEmpty() && !newTag.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::AddTag;
                cmd.targetPaths = paths;
                cmd.params["tag"] = newTag;
                CoreEngine::instance().executeCommand(cmd);
                ContentPanel* target = activeOrRootPanel();
                if (target) {
                    for (const QString& p : paths) {
                        target->updateItemMetadata(p);
                    }
                    target->recalculateAndEmitStats();
                }
            }
        });

        connect(metaPanel, &MetaPanel::tagRemoveRequested, this, [activeOrRootPanel](const QStringList& paths, const QString& removeTag) {
            if (!paths.isEmpty() && !removeTag.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::RemoveTag;
                cmd.targetPaths = paths;
                cmd.params["tag"] = removeTag;
                CoreEngine::instance().executeCommand(cmd);
                ContentPanel* target = activeOrRootPanel();
                if (target) {
                    for (const QString& p : paths) {
                        target->updateItemMetadata(p);
                    }
                    target->recalculateAndEmitStats();
                }
            }
        });

        if (filterPanel) {
            connect(metaPanel, &MetaPanel::searchByColor, filterPanel, [filterPanel](const QColor& color) {
                filterPanel->selectColor(color);
            });
        }

        connect(metaPanel, &MetaPanel::renameRequested, this, [activeOrRootPanel](const QString& oldPath, const QString& newPath) {
            ContentPanel* target = activeOrRootPanel();
            if (ShellHelper::renameItem(oldPath, newPath)) {
                if (target) {
                    target->migrateModelCache(oldPath, newPath);
                    target->refreshAll();
                }
            } else {
                if (target) {
                    target->updateItemMetadata(oldPath);
                }
            }
        });

        connect(metaPanel, &MetaPanel::noteEdited, this, [activeOrRootPanel](const QStringList& paths, const QString& newNote) {
            if (!paths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetNote;
                cmd.targetPaths = paths;
                cmd.params["note"] = newNote;
                CoreEngine::instance().executeCommand(cmd);
                ContentPanel* target = activeOrRootPanel();
                if (target) {
                    for (const QString& p : paths) {
                        target->updateItemMetadata(p);
                    }
                }
            }
        });

        connect(metaPanel, &MetaPanel::linkEdited, this, [activeOrRootPanel](const QStringList& paths, const QString& newLink) {
            if (!paths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetURL;
                cmd.targetPaths = paths;
                cmd.params["url"] = newLink;
                CoreEngine::instance().executeCommand(cmd);
                ContentPanel* target = activeOrRootPanel();
                if (target) {
                    for (const QString& p : paths) {
                        target->updateItemMetadata(p);
                    }
                }
            }
        });
    }

    // 7. 全局事件总线 CentralEventHub 增量通知响应
    connect(&CentralEventHub::instance(), &CentralEventHub::eventOccurred, this, [this, contentPanel, metaPanel](const QuarkMeta::AppEvent& event) {
        ContentPanel* activeOrRoot = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (!activeOrRoot) return;

        if (event.type == QuarkMeta::AppEventType::MetadataUpdated) {
            if (!event.targetPath.isEmpty()) {
                activeOrRoot->updateItemMetadata(event.targetPath);
                if (contentPanel && contentPanel != activeOrRoot) {
                    contentPanel->updateItemMetadata(event.targetPath);
                }
                if (metaPanel) {
                    QString targetClean = QDir::cleanPath(event.targetPath);
                    for (const QString& p : activeOrRoot->getSelectedPaths()) {
                        if (QString::compare(QDir::cleanPath(p), targetClean, Qt::CaseInsensitive) == 0) {
                            if (event.payload.contains("field") && event.payload["field"].toString() == "color") {
                                QString newColor = event.payload.value("value").toString();
                                metaPanel->setColor(newColor, false);
                            } else if (event.payload.contains("field") && event.payload["field"].toString() == "rating") {
                                int newRating = event.payload.value("value").toInt();
                                metaPanel->setRating(newRating, false);
                            }
                            break;
                        }
                    }
                }
            } else if (!event.paths.isEmpty()) {
                for (const QString& p : event.paths) {
                    activeOrRoot->updateItemMetadata(p);
                    if (contentPanel && contentPanel != activeOrRoot) {
                        contentPanel->updateItemMetadata(p);
                    }
                }
            } else {
                activeOrRoot->refreshAll();
                if (contentPanel && contentPanel != activeOrRoot) {
                    contentPanel->refreshAll();
                }
            }
            activeOrRoot->recalculateAndEmitStats();
        } else if (event.type == QuarkMeta::AppEventType::ItemsDeleted ||
                   event.type == QuarkMeta::AppEventType::ItemsRenamed ||
                   event.type == QuarkMeta::AppEventType::UndoRedoPerformed) {
            activeOrRoot->refreshAll();
            if (contentPanel && contentPanel != activeOrRoot) {
                contentPanel->refreshAll();
            }
        }
    });
}

} // namespace QuarkMeta
