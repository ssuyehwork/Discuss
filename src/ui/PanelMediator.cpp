#include "PanelMediator.h"
#include "MainWindow.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "AddressBar.h"
#include "QuickLookWindow.h"
#include "ToolTipOverlay.h"
#include "../core/CoreEngine.h"
#include "../core/CentralEventHub.h"
#include "../core/VolumeOnlineManager.h"
#include "../core/ModelContract.h"
#include "../util/ShellHelper.h"
#include "models/DiskItemModel.h"
#include "StyleLibrary.h"

#include <QFileInfo>
#include <QDateTime>
#include <QTextDocument>
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>

namespace QuarkMeta {

PanelMediator::PanelMediator(MainWindow* mainWindow, QObject* parent)
    : QObject(parent), m_mainWindow(mainWindow) {
}

void PanelMediator::setupConnections() {
    if (!m_mainWindow) return;

    NavPanel* navPanel = m_mainWindow->m_navPanel;
    FavoritePanel* favoritePanel = m_mainWindow->m_favoritePanel;
    ContentPanel* contentPanel = m_mainWindow->m_contentPanel;
    MetaPanel* metaPanel = m_mainWindow->m_metaPanel;
    FilterPanel* filterPanel = m_mainWindow->m_filterPanel;
    AddressBar* addressBar = m_mainWindow->m_addressBar;

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

    connect(&VolumeOnlineManager::instance(), &VolumeOnlineManager::volumeStateChanged,
            m_mainWindow, [this](const QString& driveLetter, bool isOnline) {
        if (!isOnline) {
            m_mainWindow->onVolumeUnplugged(driveLetter);
        }
    });

    // 2. 内容面板选中项改变 -> 元数据面板 0 毫秒极速同步
    if (contentPanel && metaPanel) {
        connect(contentPanel, &ContentPanel::selectionChanged, m_mainWindow, [this, contentPanel, metaPanel](const QStringList& paths) {
            metaPanel->setSelectedPaths(paths);
            if (paths.isEmpty()) {
                metaPanel->setImagePreview(QPixmap());
                metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
                metaPanel->setRating(0);
                metaPanel->setColor(L"");
                metaPanel->setTags(QStringList());
                metaPanel->setNote(L"");
                metaPanel->setURL(L"");
                metaPanel->setPalettes({});
            } else {
                auto indexes = contentPanel->getSelectedIndexes();
                QModelIndex idx;
                if (!indexes.isEmpty()) {
                    idx = indexes.first();
                }
                QString path = paths.first();
                QFileInfo fi(path);

                QString name;
                QString type;
                QString sizeStr;
                QString mtimeStr;

                if (idx.isValid()) {
                    name = idx.sibling(idx.row(), 0).data(Qt::DisplayRole).toString();
                    type = (idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), 4).data(Qt::DisplayRole).toString() + " 文件";
                    sizeStr = idx.sibling(idx.row(), 5).data(Qt::DisplayRole).toString();
                    mtimeStr = idx.sibling(idx.row(), 6).data(Qt::DisplayRole).toString();
                }

                if (name.isEmpty()) name = fi.fileName();
                if (type.isEmpty()) type = fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件";

                int width = 0;
                int height = 0;
                QString ctimeStr = "-";
                QString atimeStr = "-";
                QString noteStr;
                QString urlStr;
                QStringList cleanTags;
                QVector<QPair<QColor, float>> palettes;

                if (contentPanel->model()) {
                    const auto* diskModel = qobject_cast<const DiskItemModel*>(contentPanel->model());
                    if (diskModel) {
                        const auto& allRecs = diskModel->allRecords();
                        int srcRow = contentPanel->getProxyModel()->mapToSource(idx).row();
                        if (srcRow >= 0 && srcRow < static_cast<int>(allRecs.size())) {
                            const auto& rec = allRecs[srcRow];
                            width = rec.width;
                            height = rec.height;
                            if (rec.ctime > 0) ctimeStr = QDateTime::fromMSecsSinceEpoch(rec.ctime).toString("dd-MM-yyyy HH:mm");
                            if (rec.atime > 0) atimeStr = QDateTime::fromMSecsSinceEpoch(rec.atime).toString("dd-MM-yyyy HH:mm");
                            noteStr = rec.note;
                            urlStr = rec.url;
                            for (const QString& t : rec.tags) {
                                QString cleanT = t.trimmed();
                                if (!cleanT.isEmpty() && !cleanT.contains(":\\") && !cleanT.contains(":/") && cleanT != path) {
                                    cleanTags.append(cleanT);
                                }
                            }
                            for (const auto& p : rec.palettes) {
                                palettes.append({p.first, p.second});
                            }
                        }
                    }
                }

                metaPanel->updateInfo(
                    name, type, sizeStr, ctimeStr, mtimeStr, atimeStr,
                    path, idx.data(EncryptedRole).toBool(), width, height
                );
                metaPanel->setRating(idx.data(RatingRole).toInt());
                metaPanel->setColor(idx.data(ColorRole).toString().toStdWString());
                metaPanel->setTags(cleanTags); 
                metaPanel->setNote(noteStr);
                metaPanel->setURL(urlStr);
                metaPanel->setPalettes(palettes);

                QPixmap previewPixmap;
                QVariant decData = idx.data(Qt::DecorationRole);
                if (decData.canConvert<QIcon>()) {
                    previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                } else if (decData.canConvert<QPixmap>()) {
                    previewPixmap = decData.value<QPixmap>();
                }
                metaPanel->setImagePreview(previewPixmap);
            }

            m_mainWindow->onStatusBarStatsUpdated(0, 0, 0);
        });
    }

    // 3. 内容面板请求预览 -> QuickLook
    if (contentPanel) {
        connect(contentPanel, &ContentPanel::requestQuickLook, m_mainWindow, [this](const QString& path) {
            m_mainWindow->m_currentQuickLookPath = path;
            QuickLookWindow::instance().previewFile(path);
        });

        // 4. 内容面板统计信息更新 -> 状态栏
        connect(contentPanel, &ContentPanel::statusBarStatsUpdated, m_mainWindow, &MainWindow::onStatusBarStatsUpdated);
    }

    connect(&QuickLookWindow::instance(), &QuickLookWindow::prevRequested, m_mainWindow, [this, contentPanel]() {
        if (!contentPanel) return;
        QString prev = contentPanel->getAdjacentFilePath(m_mainWindow->m_currentQuickLookPath, -1);
        if (!prev.isEmpty()) {
            m_mainWindow->m_currentQuickLookPath = prev;
            QuickLookWindow::instance().previewFile(prev);
            contentPanel->selectAndScrollToPath(prev);
        }
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::nextRequested, m_mainWindow, [this, contentPanel]() {
        if (!contentPanel) return;
        QString next = contentPanel->getAdjacentFilePath(m_mainWindow->m_currentQuickLookPath, 1);
        if (!next.isEmpty()) {
            m_mainWindow->m_currentQuickLookPath = next;
            QuickLookWindow::instance().previewFile(next);
            contentPanel->selectAndScrollToPath(next);
        }
    });

    // 4. 元数据变化 -> 通过 CoreEngine 指令中心提交持久化
    connect(&QuickLookWindow::instance(), &QuickLookWindow::ratingRequested, m_mainWindow, [this, metaPanel](int rating) {
        if (m_mainWindow->m_currentQuickLookPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetRating;
        cmd.targetPaths << m_mainWindow->m_currentQuickLookPath;
        cmd.params["rating"] = rating;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setRating(rating);
        
        QString starsStr;
        int activeStars = qBound(0, rating, 5);
        for (int i = 1; i <= 5; ++i) {
            if (i <= activeStars) {
                starsStr += "<span style='color: #FF551C; font-size: 14pt; margin-right: 2px;'>★</span>";
            } else {
                starsStr += "<span style='color: #444444; font-size: 14pt; margin-right: 2px;'>★</span>";
            }
        }
        QString msg = QString("<div style='text-align: center; padding: 4px 10px;'>%1</div>").arg(starsStr);
        
        QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
        if (!screen) screen = QGuiApplication::primaryScreen();
        QRect screenGeom = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);

        QTextDocument doc;
        doc.setHtml(msg);
        doc.setDefaultStyleSheet("body, div, p, span, b, i { color: #EEEEEE !important; font-family: 'Microsoft YaHei', 'Segoe UI'; font-size: 9pt; }");
        doc.setDocumentMargin(0);
        qreal idealW = doc.idealWidth();
        if (idealW > 450) idealW = 450;
        int w = static_cast<int>(idealW) + 24;
        
        int centerX = screenGeom.x() + screenGeom.width() / 2;
        int targetX = centerX - w / 2;
        int targetY = screenGeom.y() + 50;

        ToolTipOverlay::instance()->showText(QPoint(targetX, targetY), msg, 1500, QColor("#FF551C"), true);
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::colorRequested, m_mainWindow, [this, metaPanel](const QString& color) {
        if (m_mainWindow->m_currentQuickLookPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths << m_mainWindow->m_currentQuickLookPath;
        cmd.params["color"] = color;
        CoreEngine::instance().executeCommand(cmd);

        if (metaPanel) metaPanel->setColor(color.toStdWString());
        
        QColor colorHex = QColor("#2B2B2B");
        QColor borderCol = QColor("#888888");
        
        if (color == "red") { colorHex = QColor("#E81123"); borderCol = QColor("#FF6B6B"); }
        else if (color == "orange") { colorHex = QColor("#FF551C"); borderCol = QColor("#FF8C00"); }
        else if (color == "yellow") { colorHex = QColor("#FECF0E"); borderCol = QColor("#FFF200"); }
        else if (color == "green") { colorHex = QColor("#2ECC71"); borderCol = QColor("#2ECC71"); }
        else if (color == "cyan") { colorHex = QColor("#41F2F2"); borderCol = QColor("#E0FFFF"); }
        else if (color == "blue") { colorHex = QColor("#3498DB"); borderCol = QColor("#00BFFF"); }
        else if (color == "purple") { colorHex = QColor("#9B59B6"); borderCol = QColor("#EE82EE"); }
        else if (color == "gray") { colorHex = QColor("#95A5A6"); borderCol = QColor("#BDC3C7"); }

        QString msg = "";
        
        QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
        if (!screen) screen = QGuiApplication::primaryScreen();
        QRect screenGeom = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);

        int w = 60;
        int centerX = screenGeom.x() + screenGeom.width() / 2;
        int targetX = centerX - w / 2;
        int targetY = screenGeom.y() + 50;

        ToolTipOverlay::instance()->showText(QPoint(targetX, targetY), msg, 1500, borderCol, true, colorHex);
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::deleteRequested, m_mainWindow, [this, contentPanel](const QString& path) {
        if (path.isEmpty()) return;
        if (ShellHelper::moveToTrash({path})) {
            if (contentPanel) {
                QString next = contentPanel->getAdjacentFilePath(path, 1);
                if (!next.isEmpty()) {
                    m_mainWindow->m_currentQuickLookPath = next;
                    QuickLookWindow::instance().previewFile(next);
                } else {
                    QString prev = contentPanel->getAdjacentFilePath(path, -1);
                    if (!prev.isEmpty()) {
                        m_mainWindow->m_currentQuickLookPath = prev;
                        QuickLookWindow::instance().previewFile(prev);
                    } else {
                        QuickLookWindow::instance().closePreview();
                    }
                }
                contentPanel->refreshAll();
            }
        }
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::favoriteRequested, m_mainWindow, [favoritePanel](const QString& path) {
        if (!path.isEmpty() && favoritePanel) {
            favoritePanel->addFavoriteItem(path);
            favoritePanel->saveFavorites();
            ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, Style::SuccessGreen);
        }
    });

    // 5a. 目录装载完成 -> 通过事件中枢触发 FilterPanel 动态填充
    if (contentPanel) {
        connect(contentPanel, &ContentPanel::directoryStatsReady, m_mainWindow, [filterPanel](const ScanStats& stats) {
            if (filterPanel) {
                filterPanel->populateStats(stats);
            }
            AppEvent ev;
            ev.type = AppEventType::FilterStateChanged;
            CentralEventHub::instance().publishEvent(ev);
        });
    }

    // 5b. FilterPanel 状态变化 -> 内容面板过滤
    if (filterPanel && contentPanel) {
        connect(filterPanel, &FilterPanel::filterChanged, m_mainWindow, [this, contentPanel](const FilterState& state) {
            FilterState mergedState = state;
            if (m_mainWindow->m_searchEdit) {
                mergedState.keyword = m_mainWindow->m_searchEdit->text().trimmed();
            }
            contentPanel->applyFilters(mergedState);
            m_mainWindow->updateStatusBar();
        });
    }

    // 6. 地址栏路径跳转与刷新
    if (addressBar) {
        connect(addressBar, &AddressBar::pathChanged, m_mainWindow, [this](const QString& path) {
            m_mainWindow->unifiedNavigateTo(path);
        });

        connect(addressBar, &AddressBar::refreshRequested, m_mainWindow, [contentPanel]() {
            if (contentPanel) contentPanel->refreshAll();
        });
    }

    // 8. 响应元数据面板自己的星级/颜色变更
    if (metaPanel && contentPanel) {
        connect(metaPanel, &MetaPanel::metadataChanged, m_mainWindow, [contentPanel](int rating, const std::wstring& color) {
            auto indexes = contentPanel->getSelectedIndexes();
            QStringList paths;
            for (const auto& idx : indexes) {
                QString path = idx.data(PathRole).toString(); 
                if(!path.isEmpty()) paths << path;
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

        // 添加标签管网 
        connect(metaPanel, &MetaPanel::tagAddRequested, m_mainWindow, [contentPanel](const QStringList& paths, const QString& newTag) { 
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
 
        // 删除标签管网 
        connect(metaPanel, &MetaPanel::tagRemoveRequested, m_mainWindow, [contentPanel](const QStringList& paths, const QString& removeTag) { 
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

        // 标签批量更新
        connect(metaPanel, &MetaPanel::tagsChanged, m_mainWindow, [contentPanel](const QStringList& paths, const QStringList&) {
            for (const QString& p : paths) {
                contentPanel->updateItemMetadata(p);
            }
        });

        // 调色盘搜索联动
        connect(metaPanel, &MetaPanel::searchByColor, m_mainWindow, [filterPanel](const QColor& color) {
            if (filterPanel) {
                filterPanel->selectColor(color);
            }
        });

        // 重命名信号
        connect(metaPanel, &MetaPanel::renameRequested, m_mainWindow, [contentPanel](const QString& oldPath, const QString& newPath) {
            if (ShellHelper::renameItem(oldPath, newPath)) {
                contentPanel->migrateModelCache(oldPath, newPath);
                contentPanel->refreshAll();
            } else {
                contentPanel->updateItemMetadata(oldPath);
            }
        });

        connect(metaPanel, &MetaPanel::noteEdited, m_mainWindow, [](const QStringList& paths, const QString& newNote) {
            if (!paths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetNote;
                cmd.targetPaths = paths;
                cmd.params["note"] = newNote;
                CoreEngine::instance().executeCommand(cmd);
            }
        });

        connect(metaPanel, &MetaPanel::linkEdited, m_mainWindow, [](const QStringList& paths, const QString& newLink) {
            if (!paths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetURL;
                cmd.targetPaths = paths;
                cmd.params["url"] = newLink;
                CoreEngine::instance().executeCommand(cmd);
            }
        });
    }

    // 9. 响应中央中枢事件
    connect(&CentralEventHub::instance(), &CentralEventHub::eventOccurred, m_mainWindow, [contentPanel](const QuarkMeta::AppEvent& event) {
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
        } else if (event.type == QuarkMeta::AppEventType::ItemsDeleted || event.type == QuarkMeta::AppEventType::ItemsRenamed) {
            contentPanel->refreshAll();
        }
    });
}

} // namespace QuarkMeta
