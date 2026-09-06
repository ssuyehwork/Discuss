#include "ContentContextMenu.h"
#include "../ContentPanel.h"
#include "ContentSortController.h"
#include "../UiHelper.h"
#include "../StyleLibrary.h"
#include "../ToolTipOverlay.h"
#include "../ColorPicker.h"
#include "../FavoritePanel.h"
#include "../BatchRenameDialog.h"
#include "../BatchCreateDialog.h"
#include "../FramelessDialog.h"
#include "../dialogs/FramelessInputDialog.h"
#include "../../util/ShellHelper.h"
#include "../../util/DeepThumbnailExtractor.h"
#include "../../core/TrashService.h"
#include "../../core/DiskTrashService.h"
#include "../../core/PermanentDeleteService.h"
#include "../../core/ClipboardService.h"
#include "../../core/NavigationHistoryService.h"
#include "../../core/OperationSnapshotEngine.h"
#include "../../util/DiskIoService.h"
#include "../FramelessFileDialog.h"
#include "../../core/CoreEngine.h"
#include "../../meta/MetadataManager.h"
#include "../../meta/FavoriteDao.h"
#include "../../crypto/EncryptionManager.h"
#include "../../core/LastOperationManager.h"

#include <QMenu>
#include <QWidgetAction>
#include <QActionGroup>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QClipboard>
#include <QThreadPool>
#include <QPointer>

namespace QuarkMeta {

ContentContextMenu::ContentContextMenu(ContentPanel* parentPanel)
    : QObject(parentPanel), m_panel(parentPanel) {
}

void ContentContextMenu::showMenu(QAbstractItemView* view, const QPoint& pos) {
    if (!m_panel || !view) return;

    QModelIndex currentIndex = view->indexAt(pos);
    bool onItem = currentIndex.isValid();

    if (onItem && view->selectionModel()) {
        if (!view->selectionModel()->isSelected(currentIndex)) {
            view->selectionModel()->select(currentIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            view->setCurrentIndex(currentIndex);
        }
    }

    QModelIndex col0Index = onItem ? currentIndex.sibling(currentIndex.row(), 0) : QModelIndex();
    QString path = onItem ? col0Index.data(PathRole).toString() : "";
    QFileInfo itemInfo(path);

    QString currentPath = m_panel->currentPath();
    QString currentCategoryType = m_panel->getCurrentCategoryType();

    bool isComputerRoot = (currentPath.isEmpty() || currentPath == "computer://");
    bool isTrashView = (currentCategoryType == "trash" || currentPath == "trash://");

    bool isDriveRoot = onItem && (itemInfo.isRoot() || path.endsWith(":\\") || path.endsWith(":/") || (path.length() == 2 && path.endsWith(':')));
    bool isFolder = onItem && (isDriveRoot || currentIndex.data(TypeRole).toString() == "folder");

    QMenu menu(m_panel);
    UiHelper::applyMenuStyle(&menu);

    // =========================================================================
    // 场景 1：回收站视图
    // =========================================================================
    if (isTrashView) {
        if (onItem) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "还原")->setData(ContentPanel::ActionRestore);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切")->setData(ContentPanel::ActionCut);
            menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "永久删除")->setData(ContentPanel::ActionSecureDelete);
            menu.addSeparator();
        }
        menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "还原全部")->setData(ContentPanel::ActionRestoreAll);
        menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "清空回收站")->setData(ContentPanel::ActionEmptyTrash);

        m_panel->setContextMenuActive(true);
        QAction* selectedAction = menu.exec(view->viewport()->mapToGlobal(pos));
        m_panel->setContextMenuActive(false);

        if (!selectedAction || !selectedAction->data().isValid()) return;

        ContentPanel::ContextAction action = static_cast<ContentPanel::ContextAction>(selectedAction->data().toInt());
        switch (action) {
            case ContentPanel::ActionRestore:
                TrashService::instance().restoreItems(m_panel->getSelectedTrashIds(), m_panel);
                break;
            case ContentPanel::ActionCut:
                ClipboardService::instance().cutItems(m_panel->getSelectedPaths());
                break;
            case ContentPanel::ActionSecureDelete:
                PermanentDeleteService::instance().execute(m_panel->getSelectedPaths(), m_panel);
                break;
            case ContentPanel::ActionRestoreAll:
                TrashService::instance().restoreAll(m_panel);
                break;
            case ContentPanel::ActionEmptyTrash:
                // 🚀【统一安全管线】：直接调用具有弹窗确认、进度条和扇区覆写的全量清空管线
                PermanentDeleteService::instance().executeEmptyTrash(m_panel);
                break;
            default:
                break;
        }
        return;
    }

    // =========================================================================
    // 场景 2：选中具体项目
    // =========================================================================
    if (onItem) {
        if (isDriveRoot) {
            menu.addAction(UiHelper::getIcon("open", QColor("#EEEEEE"), 18), "打开")->setData(ContentPanel::ActionOpen);
            menu.addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) m_panel->getProxyModel()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(IsLockedRole).toBool();
            menu.addAction(UiHelper::getIcon(isPinned ? "pin_tilted" : "pin_vertical", QColor("#EEEEEE"), 18), isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);

            bool isFavDrive = FavoriteDao::containsPath(path);
            menu.addAction(UiHelper::getIcon(isFavDrive ? "close" : "star_filled", QColor("#EEEEEE"), 18), isFavDrive ? "取消收藏" : "添加至收藏夹")->setData(ContentPanel::ActionAddToFavorites);

            menu.addSeparator();

            QAction* actItemPaste = menu.addAction(UiHelper::getIcon("paste", QColor("#EEEEEE"), 18), "粘贴");
            actItemPaste->setData(ContentPanel::ActionPaste);
            actItemPaste->setEnabled(m_panel->canPaste(path));

            menu.addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制路径")->setData(ContentPanel::ActionCopyPath);

            QMenu* moreMenuDrive = menu.addMenu(UiHelper::getIcon("more_horizontal", QColor("#EEEEEE"), 18), "更多");
            UiHelper::applyMenuStyle(moreMenuDrive);

            QString driveExt = QFileInfo(path).suffix().toLower();
            bool canExtractDrive = UiHelper::isTextFile(driveExt);
            if (canExtractDrive) {
                QAction* actExtract = moreMenuDrive->addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "支持提取内容");
                connect(actExtract, &QAction::triggered, this, [path]() {
                    QString content;
                    if (UiHelper::extractTextContent(path, content)) {
                        QApplication::clipboard()->setText(content);
                        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已成功提取内容并存入剪贴板 (共 %1 字符)").arg(content.length()), 1500, QColor("#2ecc71"));
                    } else {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "提取失败：文件超过限制或无法作为纯文本解析", 1500, QColor("#e81123"));
                    }
                });
            } else {
                QAction* actDisabled = moreMenuDrive->addAction(UiHelper::getIcon("prohibit", QColor("#888888"), 18), "不支持提取内容");
                actDisabled->setEnabled(false);
            }

            QString nativePath = QDir::toNativeSeparators(path);
            QStringList itemTags;
            if (!nativePath.isEmpty()) {
                itemTags = MetadataManager::instance().getMeta(nativePath.toStdWString()).tags;
            }
            if (itemTags.isEmpty() && col0Index.isValid()) {
                itemTags = col0Index.data(TagsRole).toStringList();
            }
            QStringList cleanTags;
            for (const QString& t : itemTags) {
                QString trimmed = t.trimmed();
                if (!trimmed.isEmpty()) cleanTags << trimmed;
            }
            QAction* actCopyTags = menu.addAction(UiHelper::getIcon("tag", QColor("#EEEEEE"), 18), "复制标签");
            actCopyTags->setData(ContentPanel::ActionCopyTags);
            actCopyTags->setEnabled(!cleanTags.isEmpty());

            QAction* actPasteTags = menu.addAction(UiHelper::getIcon("paste_tag", QColor("#EEEEEE"), 18), "粘贴标签");
            actPasteTags->setData(ContentPanel::ActionPasteTags);
            actPasteTags->setEnabled(ClipboardService::instance().hasCopiedTags());

            QAction* actRepeat = menu.addAction(UiHelper::getIcon("repeat", QColor("#EEEEEE"), 18), LastOperationManager::instance().displayText());
            actRepeat->setData(ContentPanel::ActionRepeatLastOp);
            actRepeat->setEnabled(LastOperationManager::instance().hasOperation());

            menu.addSeparator();
            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "刷新")->setData(ContentPanel::ActionRefresh);
        } else {
            menu.addAction(UiHelper::getIcon(isFolder ? "folder" : "open", QColor("#EEEEEE"), 18), isFolder ? "打开文件夹" : "打开")->setData(ContentPanel::ActionOpen);
            if (!isFolder) {
                menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
            }
            menu.addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) m_panel->getProxyModel()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(IsLockedRole).toBool();
            menu.addAction(UiHelper::getIcon(isPinned ? "pin_tilted" : "pin_vertical", QColor("#EEEEEE"), 18), isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);

            bool isFavItem = FavoriteDao::containsPath(path);
            menu.addAction(UiHelper::getIcon(isFavItem ? "close" : "star_filled", QColor("#EEEEEE"), 18), isFavItem ? "取消收藏" : "添加至收藏夹")->setData(ContentPanel::ActionAddToFavorites);

            menu.addSeparator();

            menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "复制")->setData(ContentPanel::ActionCopy);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切")->setData(ContentPanel::ActionCut);

            if (!isComputerRoot && !currentPath.isEmpty()) {
                std::wstring volSerial = MetadataManager::getVolumeSerialNumber(path.toStdWString());
                QStringList recentFolders = NavigationHistoryService::getRecentVisitedFolders(volSerial);
                recentFolders.removeAll(currentPath);

                QMenu* moveMenu = menu.addMenu(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 18), "移动到");
                UiHelper::applyMenuStyle(moveMenu);

                auto performMoveTo = [this](const QString& targetDir) {
                    QStringList selectedPaths = m_panel->getSelectedPaths();
                    if (selectedPaths.isEmpty()) return;

                    DiskIoContext ioCtx;
                    ioCtx.sources = selectedPaths;
                    ioCtx.destination = targetDir;
                    ioCtx.isMove = true;

                    QPointer<ContentPanel> weakPanel(m_panel);
                    DiskIoService::instance().executeAsync(ioCtx, [weakPanel](bool success) {
                        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, success]() {
                            if (weakPanel) {
                                if (success) {
                                    weakPanel->refreshAll();
                                    ToolTipOverlay::instance()->showText(QCursor::pos(), "文件移动成功", 1500, QColor("#2ecc71"));
                                } else {
                                    ToolTipOverlay::instance()->showText(QCursor::pos(), "移动失败：物理写入未能完成", 2000, QColor("#e81123"));
                                }
                            }
                        });
                    });
                };

                for (const QString& recentDir : recentFolders) {
                    QAction* actMove = moveMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 16), recentDir);
                    connect(actMove, &QAction::triggered, this, [performMoveTo, recentDir]() {
                        performMoveTo(recentDir);
                    });
                }

                if (!recentFolders.isEmpty()) {
                    moveMenu->addSeparator();
                }

                QAction* actBrowseMove = moveMenu->addAction(UiHelper::getIcon("folder", QColor("#EEEEEE"), 16), "浏览选择文件夹...");
                connect(actBrowseMove, &QAction::triggered, this, [this, performMoveTo]() {
                    QString selectedDir = FramelessFileDialog::getExistingDirectory(m_panel, "选择移动的目标文件夹", m_panel->currentPath());
                    if (!selectedDir.isEmpty()) {
                        performMoveTo(selectedDir);
                    }
                });
            }

            QAction* actItemPaste = menu.addAction(UiHelper::getIcon("paste", QColor("#EEEEEE"), 18), "粘贴");
            actItemPaste->setData(ContentPanel::ActionPaste);
            actItemPaste->setEnabled(m_panel->canPaste(isFolder ? path : currentPath));

            menu.addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制路径")->setData(ContentPanel::ActionCopyPath);

            QMenu* moreMenu = menu.addMenu(UiHelper::getIcon("more_horizontal", QColor("#EEEEEE"), 18), "更多");
            UiHelper::applyMenuStyle(moreMenu);

            QString fileExt = QFileInfo(path).suffix().toLower();
            bool canExtract = !isFolder && UiHelper::isTextFile(fileExt);
            if (canExtract) {
                QAction* actExtract = moreMenu->addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "支持提取内容");
                connect(actExtract, &QAction::triggered, this, [path]() {
                    QString content;
                    if (UiHelper::extractTextContent(path, content)) {
                        QApplication::clipboard()->setText(content);
                        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已成功提取内容并存入剪贴板 (共 %1 字符)").arg(content.length()), 1500, QColor("#2ecc71"));
                    } else {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "提取失败：文件超过限制或无法作为纯文本解析", 1500, QColor("#e81123"));
                    }
                });
            } else {
                QAction* actDisabled = moreMenu->addAction(UiHelper::getIcon("prohibit", QColor("#888888"), 18), "不支持提取内容");
                actDisabled->setEnabled(false);
            }

            QString nativePath = QDir::toNativeSeparators(path);
            QStringList itemTags;
            if (!nativePath.isEmpty()) {
                itemTags = MetadataManager::instance().getMeta(nativePath.toStdWString()).tags;
            }
            if (itemTags.isEmpty() && col0Index.isValid()) {
                itemTags = col0Index.data(TagsRole).toStringList();
            }
            QStringList cleanTags;
            for (const QString& t : itemTags) {
                QString trimmed = t.trimmed();
                if (!trimmed.isEmpty()) cleanTags << trimmed;
            }
            QAction* actCopyTags = menu.addAction(UiHelper::getIcon("tag", QColor("#EEEEEE"), 18), "复制标签");
            actCopyTags->setData(ContentPanel::ActionCopyTags);
            actCopyTags->setEnabled(!cleanTags.isEmpty());

            QAction* actPasteTags = menu.addAction(UiHelper::getIcon("paste_tag", QColor("#EEEEEE"), 18), "粘贴标签");
            actPasteTags->setData(ContentPanel::ActionPasteTags);
            actPasteTags->setEnabled(ClipboardService::instance().hasCopiedTags());

            QAction* actRepeat = menu.addAction(UiHelper::getIcon("repeat", QColor("#EEEEEE"), 18), LastOperationManager::instance().displayText());
            actRepeat->setData(ContentPanel::ActionRepeatLastOp);
            actRepeat->setEnabled(LastOperationManager::instance().hasOperation());

            menu.addAction(UiHelper::getIcon("edit", QColor("#EEEEEE"), 18), "重命名")->setData(ContentPanel::ActionRename);

            menu.addSeparator();
            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "刷新")->setData(ContentPanel::ActionRefresh);

            if (!isFolder) {
                menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "重新提取缩略图")->setData(ContentPanel::ActionReextractThumbnail);

                QMenu* cryptoMenu = menu.addMenu(UiHelper::getIcon("shield", QColor("#EEEEEE"), 18), "外壳保护");
                UiHelper::applyMenuStyle(cryptoMenu);
                cryptoMenu->addAction(UiHelper::getIcon("lock", QColor("#EEEEEE"), 18), "执行外壳保护")->setData(ContentPanel::ActionEncrypt);
                cryptoMenu->addAction(UiHelper::getIcon("unlock", QColor("#EEEEEE"), 18), "解除保护")->setData(ContentPanel::ActionDecrypt);
                cryptoMenu->addAction(UiHelper::getIcon("key", QColor("#EEEEEE"), 18), "修改保护密码")->setData(ContentPanel::ActionChangePwd);
            }
        }
    }
    // =========================================================================
    // 场景 3：点击空白处
    // =========================================================================
    else {
        if (isComputerRoot) {
            menu.addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);
            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "刷新")->setData(ContentPanel::ActionRefresh);
        } else {
            QMenu* newMenu = menu.addMenu(UiHelper::getIcon("add", QColor("#EEEEEE"), 18), "新建...");
            UiHelper::applyMenuStyle(newMenu);
            newMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "创建文件夹")->setData(ContentPanel::ActionNewFolder);
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建 Markdown")->setData(ContentPanel::ActionNewMd);
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建纯文本文件 (txt)")->setData(ContentPanel::ActionNewTxt);

            menu.addSeparator();
            menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE")), "批量创建项目...")->setData(ContentPanel::ActionBatchCreate);

            menu.addSeparator();
            QAction* actPaste = menu.addAction(UiHelper::getIcon("paste", QColor("#EEEEEE"), 18), "粘贴");
            actPaste->setData(ContentPanel::ActionPaste);
            actPaste->setEnabled(m_panel->canPaste(currentPath));

            menu.addSeparator();
            bool isPhysicalPath = !currentPath.isEmpty() && !currentPath.contains("://") && QDir(currentPath).exists();
            QAction* actShowInExp = menu.addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示");
            actShowInExp->setData(ContentPanel::ActionShowInExplorer);
            actShowInExp->setEnabled(isPhysicalPath);

            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "刷新")->setData(ContentPanel::ActionRefresh);
        }
    }

    menu.addSeparator();

    // 排序二级子菜单
    QMenu* sortMenu = menu.addMenu(UiHelper::getIcon("sort", QColor("#EEEEEE"), 18), "排序");
    UiHelper::applyMenuStyle(sortMenu);

    QActionGroup* typeGroup = new QActionGroup(this);
    auto addTypeAct = [&](const QString& label, ContentPanel::SortType type) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_panel->currentSortType() == type);
        typeGroup->addAction(act);
        connect(act, &QAction::triggered, [this, type]() {
            m_panel->setSortType(type);
        });
    };

    addTypeAct("名称", ContentPanel::SortByName);
    addTypeAct("创建日期", ContentPanel::SortByCreateDate);
    addTypeAct("修改日期", ContentPanel::SortByModifyDate);
    addTypeAct("扩展名", ContentPanel::SortByExtension);
    addTypeAct("大小", ContentPanel::SortBySize);
    addTypeAct("尺寸", ContentPanel::SortByDimension);
    addTypeAct("评分", ContentPanel::SortByRating);

    sortMenu->addSeparator();

    QActionGroup* orderGroup = new QActionGroup(this);
    auto addOrderAct = [&](const QString& label, Qt::SortOrder order) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_panel->currentSortOrder() == order);
        orderGroup->addAction(act);
        connect(act, &QAction::triggered, [this, order]() {
            m_panel->setSortOrder(order);
        });
    };

    addOrderAct("升序", Qt::AscendingOrder);
    addOrderAct("降序", Qt::DescendingOrder);

    // 删除子菜单
    if (onItem && !isDriveRoot) {
        menu.addSeparator();
        QMenu* delMenu = menu.addMenu(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "删除");
        UiHelper::applyMenuStyle(delMenu);
        delMenu->addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "移入回收站")->setData(ContentPanel::ActionDelete);
        delMenu->addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "永久删除")->setData(ContentPanel::ActionSecureDelete);
    }

    m_panel->setContextMenuActive(true);
    QAction* selectedAction = menu.exec(view->viewport()->mapToGlobal(pos));
    m_panel->setContextMenuActive(false);

    if (!selectedAction || !selectedAction->data().isValid()) return;

    ContentPanel::ContextAction action = static_cast<ContentPanel::ContextAction>(selectedAction->data().toInt());

    switch (action) {
        case ContentPanel::ActionOpen:
            m_panel->onDoubleClicked(currentIndex);
            break;
        case ContentPanel::ActionOpenDefault: {
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString filePath = idx.data(PathRole).toString();
                    if (!filePath.isEmpty() && QFileInfo::exists(filePath)) {
                        MetadataManager::instance().recordAccess(filePath.toStdWString());
                        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
                    }
                }
            }
            break;
        }
        case ContentPanel::ActionRepeatLastOp: {
            if (!LastOperationManager::instance().hasOperation()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "尚未记录任何可重复的操作", 1500, QColor("#e81123"));
                break;
            }
            auto indexes = view->selectionModel()->selectedIndexes();
            int count = 0;
            LastOperationType type = LastOperationManager::instance().type();
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    if (type == LastOperationType::SetRating) {
                        m_panel->getProxyModel()->setData(idx, LastOperationManager::instance().rating(), RatingRole);
                    } else if (type == LastOperationType::SetColor) {
                        m_panel->getProxyModel()->setData(idx, LastOperationManager::instance().color(), ColorRole);
                    } else if (type == LastOperationType::PasteTags) {
                        m_panel->getProxyModel()->setData(idx, LastOperationManager::instance().tags(), TagsRole);
                    }
                    count++;
                }
            }
            if (count > 0) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已对 %1 个项目重复执行上一次操作").arg(count), 1500, QColor("#2ecc71"));
            }
            break;
        }
        case ContentPanel::ActionShowInExplorer: {
            QString targetPath = onItem ? path : currentPath;
            if (!targetPath.isEmpty() && !targetPath.contains("://")) {
                ShellHelper::openInExplorer(targetPath);
            }
            break;
        }
        case ContentPanel::ActionNewFolder:
            m_panel->createNewItem("folder");
            break;
        case ContentPanel::ActionNewMd:
            m_panel->createNewItem("md");
            break;
        case ContentPanel::ActionNewTxt:
            m_panel->createNewItem("txt");
            break;
        case ContentPanel::ActionPin:
        case ContentPanel::ActionUnpin: {
            auto indexes = view->selectionModel()->selectedIndexes();
            bool pin = (action == ContentPanel::ActionPin);
            for (const QModelIndex& idx : indexes) {
                if (idx.column() == 0) {
                    m_panel->getProxyModel()->setData(idx, pin, IsLockedRole);
                }
            }
            m_panel->getProxyModel()->invalidate();
            m_panel->getProxyModel()->sort(0, m_panel->getProxyModel()->sortOrder());
            break;
        }
        case ContentPanel::ActionEncrypt: {
            FramelessInputDialog dlg("加密保护", "设置加密密码:", "", m_panel);
            dlg.setEchoMode(QLineEdit::Password);
            if (dlg.exec() == QDialog::Accepted) {
                QString pwd = dlg.text();
                if (pwd.isEmpty()) break;
                auto indexes = view->selectionModel()->selectedIndexes();
                QStringList targets;
                for (const auto& idx : indexes) if (idx.column() == 0) targets << idx.data(PathRole).toString();

                ToolTipOverlay::instance()->showText(QCursor::pos(), "加密任务已在后台启动...", 2000);

                std::string stdPwd = pwd.toStdString();
                QPointer<ContentPanel> self(m_panel);
                QString curDir = currentPath;

                (void)QThreadPool::globalInstance()->start([self, targets, stdPwd, curDir]() {
                    for (const QString& src : targets) {
                        QString dest = src + ".amenc";
                        if (EncryptionManager::instance().encryptFile(src.toStdWString(), dest.toStdWString(), stdPwd)) {
                            QFile::remove(src);
                            MetadataManager::instance().setEncrypted(dest.toStdWString(), true);
                        }
                    }
                    QMetaObject::invokeMethod(QCoreApplication::instance(), [self, curDir]() {
                        if (self && self->currentPath() == curDir) self->loadDirectory(curDir, self->isRecursive());
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "加密任务处理完成", 1500, QColor("#2ecc71"));
                    });
                });
            }
            break;
        }
        case ContentPanel::ActionDecrypt: {
            FramelessInputDialog dlg("解除加密", "输入加密密码:", "", m_panel);
            dlg.setEchoMode(QLineEdit::Password);
            if (dlg.exec() == QDialog::Accepted) {
                QString pwd = dlg.text();
                if (!pwd.isEmpty()) {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "解除加密逻辑已触发", 1500);
                }
            }
            break;
        }
        case ContentPanel::ActionBatchRename:
        case ContentPanel::ActionRename: {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (selectedPaths.size() > 1) {
                m_panel->performBatchRename();
            } else {
                view->edit(currentIndex);
            }
            break;
        }
        case ContentPanel::ActionCopy:
            ClipboardService::instance().copyItems(m_panel->getSelectedPaths());
            break;
        case ContentPanel::ActionCut:
            ClipboardService::instance().cutItems(m_panel->getSelectedPaths());
            break;
        case ContentPanel::ActionPaste:
            ClipboardService::instance().executePaste(isFolder ? path : currentPath, m_panel);
            break;
        case ContentPanel::ActionCopyTags: {
            QString nativePath = QDir::toNativeSeparators(path);
            QStringList tags;
            if (!nativePath.isEmpty()) {
                tags = MetadataManager::instance().getMeta(nativePath.toStdWString()).tags;
            }
            if (tags.isEmpty() && col0Index.isValid()) {
                tags = col0Index.data(TagsRole).toStringList();
            }
            QStringList cleanTags;
            for (const QString& t : tags) {
                QString trimmed = t.trimmed();
                if (!trimmed.isEmpty()) cleanTags << trimmed;
            }
            if (!cleanTags.isEmpty()) {
                ClipboardService::instance().setCopiedTags(cleanTags);
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已复制 %1 个标签").arg(cleanTags.size()), 1500, QColor("#2ecc71"));
            } else {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "当前项目未绑定任何标签", 1500, QColor("#e81123"));
            }
            break;
        }
        case ContentPanel::ActionPasteTags: {
            QStringList copiedTags = ClipboardService::instance().copiedTags();
            if (copiedTags.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "剪贴板无有效标签", 1500, QColor("#e81123"));
                break;
            }
            auto indexes = view->selectionModel()->selectedIndexes();
            int count = 0;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    m_panel->getProxyModel()->setData(idx, copiedTags, TagsRole);
                    count++;
                }
            }
            if (count > 0) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已将标签粘贴至 %1 个项目").arg(count), 1500, QColor("#2ecc71"));
            }
            break;
        }
        case ContentPanel::ActionBatchCreate: {
            BatchCreateDialog dlg(currentPath, m_panel);
            if (dlg.exec() == QDialog::Accepted) {
                m_panel->refreshAll();
            }
            break;
        }
        case ContentPanel::ActionRestore: {
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    if (idx.data(IsDiskTrashRole).toBool()) {
                        int id = idx.data(DiskTrashIdRole).toInt();
                        QString trashPath = idx.data(PathRole).toString();
                        DiskTrashService::restoreFromDiskTrash(id, trashPath);
                    }
                }
            }
            m_panel->refreshAll();
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
            break;
        }
        case ContentPanel::ActionDelete:
            TrashService::instance().moveToTrash(m_panel->getSelectedPaths(), m_panel);
            break;
        case ContentPanel::ActionSecureDelete:
            PermanentDeleteService::instance().execute(m_panel->getSelectedPaths(), m_panel);
            break;
        case ContentPanel::ActionAddToFavorites: {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (selectedPaths.isEmpty() && !path.isEmpty()) {
                selectedPaths << path;
            }

            if (!selectedPaths.isEmpty()) {
                bool allFav = true;
                for (const QString& p : selectedPaths) {
                    if (!FavoriteDao::containsPath(p)) {
                        allFav = false;
                        break;
                    }
                }

                if (allFav) {
                    emit m_panel->requestRemoveFavorite(selectedPaths);
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "已从收藏夹移除", 1500, QColor("#e81123"));
                } else {
                    OperationSnapshotEngine::instance().executeWithSnapshot(
                        m_panel,
                        SnapshotOperationType::ToggleFavorite,
                        selectedPaths,
                        "已成功添加至收藏夹",
                        [this, selectedPaths]() {
                            emit m_panel->requestAddFavorite(selectedPaths);
                            return true;
                        },
                        [](const QVector<AssetItemSnapshot>& beforeState) {
                            for (const auto& snap : beforeState) {
                                AppCommand cmd;
                                cmd.type = AppCommandType::SetPinned;
                                cmd.targetPaths << snap.path;
                                cmd.params["pinned"] = snap.isPinned;
                                CoreEngine::instance().executeCommand(cmd);
                            }
                            return true;
                        }
                    );
                }
            }
            break;
        }
        case ContentPanel::ActionCopyName: {
            QModelIndexList indexes = m_panel->getSelectedIndexes();
            QStringList targetNames;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetNames << QFileInfo(p).fileName();
                }
            }
            if (targetNames.isEmpty() && !path.isEmpty()) {
                targetNames << QFileInfo(path).fileName();
            }
            if (!targetNames.isEmpty()) {
                QApplication::clipboard()->setText(targetNames.join("\r\n"));
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制文件名到剪贴板", 1200, QColor("#2ecc71"));
            }
            break;
        }
        case ContentPanel::ActionCopyPath: {
            QModelIndexList indexes = m_panel->getSelectedIndexes();
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetPaths << QDir::toNativeSeparators(p);
                }
            }
            if (targetPaths.isEmpty() && !path.isEmpty()) {
                targetPaths << QDir::toNativeSeparators(path);
            }
            if (!targetPaths.isEmpty()) {
                QApplication::clipboard()->setText(targetPaths.join("\n"));
            }
            break;
        }
        case ContentPanel::ActionRefresh:
            m_panel->refreshAll();
            break;
        case ContentPanel::ActionReextractThumbnail: {
            auto indexes = view->selectionModel()->selectedIndexes();
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0 && !idx.data(PathRole).toString().isEmpty() && idx.data(TypeRole).toString() != "folder") {
                    targetPaths << idx.data(PathRole).toString();
                }
            }
            if (targetPaths.isEmpty() && !path.isEmpty() && !isFolder) {
                targetPaths << path;
            }

            if (!targetPaths.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("正在深度重新提取 %1 个项目的缩略图...").arg(targetPaths.size()), 2000, QColor("#3498db"));

                QPointer<ContentPanel> weakThis(m_panel);
                DeepThumbnailExtractor::instance().extractBatchAsync(
                    targetPaths,
                    [weakThis](const QString& itemPath, bool success) {
                        if (weakThis && success) {
                            weakThis->reloadThumbnailForPath(itemPath);
                        }
                    },
                    [weakThis](int successCount, int totalCount) {
                        if (weakThis) {
                            ToolTipOverlay::instance()->showText(
                                QCursor::pos(),
                                QString("缩略图提取完成：成功 %1 / 总计 %2").arg(successCount).arg(totalCount),
                                2000,
                                successCount > 0 ? QColor("#2ecc71") : QColor("#e81123")
                            );
                        }
                    }
                );
            }
            break;
        }
        default:
            break;
    }
}

} // namespace QuarkMeta