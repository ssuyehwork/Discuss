#include "QuickContextMenuBuilder.h"
#include "../QuickWindow.h"
#include "QuickMediator.h"
#include "../DropTreeView.h"
#include "../IconHelper.h"
#include "../models/NoteModel.h"
#include "../models/CategoryModel.h"
#include "../../core/DatabaseManager.h"
#include "../../core/TagCodec.h"
#include "../../core/FileStorageHelper.h"
#include "../StringUtils.h"
#include "../ToolTipOverlay.h"
#include "../FramelessDialog.h"
#include "../CategoryPasswordDialog.h"
#include "../NoteEditWindow.h"
#include "../../core/ShortcutManager.h"
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QActionGroup>
#include <QColorDialog>
#include <QRandomGenerator>
#include <QFileDialog>
#include <QTimer>
#include <QCursor>

namespace RapidNotes {

static QString getShortcutHint(const QString& id) {
    QKeySequence seq = ShortcutManager::instance().getShortcut(id);
    return seq.isEmpty() ? "" : " (" + seq.toString(QKeySequence::NativeText).replace("+", " + ") + ")";
}

QMenu* QuickContextMenuBuilder::buildListContextMenu(QuickWindow* window, const QModelIndexList& selected, QWidget* parent) {
    if (!window) return nullptr;
    auto* menu = new QMenu(parent);
    IconHelper::setupMenu(menu);

    int selCount = selected.size();
    auto* mediator = window->m_mediator;

    // 空白处右键
    if (selCount == 0) {
        menu->addAction(IconHelper::getIcon("add", "#3498db", 18), "新建数据" + getShortcutHint("qw_new_idea"), window, &QuickWindow::doNewIdea);
        return menu;
    }

    // 单项选中专属项
    if (selCount == 1) {
        menu->addAction(IconHelper::getIcon("eye", "#41F2F2", 18), "预览" + getShortcutHint("qw_preview"), window, &QuickWindow::doPreview);

        QString content = selected.first().data(NoteModel::ContentRole).toString();
        QString type = selected.first().data(NoteModel::TypeRole).toString();

        QString firstUrl = StringUtils::extractFirstUrl(content);
        if (!firstUrl.isEmpty()) {
            menu->addAction(IconHelper::getIcon("link", "#17B345", 18), "打开链接", [firstUrl]() {
                QDesktopServices::openUrl(QUrl(firstUrl));
            });
        }

        bool isPath = (type == "file" || type == "local_file" || type == "local_folder" || type == "local_batch");
        QString plainContent = StringUtils::htmlToPlainText(content).trimmed();
        QString path = content;
        if (!isPath && QFileInfo(plainContent).exists() && QFileInfo(plainContent).isAbsolute()) {
            isPath = true;
            path = plainContent;
        }

        if (isPath) {
            if (path.startsWith("attachments/")) {
                path = QCoreApplication::applicationDirPath() + "/" + path;
            }
            if (QFileInfo::exists(path)) {
                menu->addAction(IconHelper::getIcon("folder", "#3A90FF", 18), "在资源管理器中显示", [path]() {
                    StringUtils::locateInExplorer(path, true);
                });
            } else {
                QAction* invalidAction = menu->addAction(IconHelper::getIcon("folder", "#555555", 18), "无效项目 (文件已丢失)");
                invalidAction->setEnabled(false);
            }
        }
    }

    // 复制 / 提取
    menu->addAction(IconHelper::getIcon("copy", "#1abc9c", 18), QString("复制 (%1)").arg(selCount), window, &QuickWindow::doExtractContent);

    // 多选合并
    if (selCount >= 2) {
        menu->addAction(IconHelper::getIcon("merge", "#3498db", 18), QString("合并数据 (%1)").arg(selCount), window, &QuickWindow::doMergeSelected);
    }
    menu->addSeparator();

    // 单项编辑与标签复制/粘贴
    if (selCount == 1) {
        menu->addAction(IconHelper::getIcon("edit", "#4a90e2", 18), "编辑" + getShortcutHint("qw_edit"), window, &QuickWindow::doEditSelected);

        QString tags = selected.first().data(NoteModel::TagsRole).toString();
        if (!tags.trimmed().isEmpty()) {
            menu->addAction(IconHelper::getIcon("copy_tags", "#9b59b6", 18), "复制标签" + getShortcutHint("qw_copy_tags"), window, &QuickWindow::doCopyTags);
        }
        if (!DatabaseManager::getTagClipboard().isEmpty()) {
            menu->addAction(IconHelper::getIcon("paste_tags", "#e67e22", 18), "粘贴标签" + getShortcutHint("qw_paste_tags"), window, &QuickWindow::doPasteTags);
        }
        menu->addSeparator();
    }

    // 提取选中的 Note ID 列表
    QList<int> ids;
    ids.reserve(selCount);
    for (const auto& idx : selected) {
        ids.append(idx.data(NoteModel::IdRole).toInt());
    }

    // 评级菜单
    auto* ratingMenu = menu->addMenu(IconHelper::getIcon("star_filled", "#f39c12", 18), QString("标记星级 (%1)").arg(selCount));
    auto* starGroup = new QActionGroup(menu);
    int currentRating = (selCount == 1) ? selected.first().data(NoteModel::RatingRole).toInt() : -1;
    for (int i = 1; i <= 5; ++i) {
        QString stars = QString("★").repeated(i);
        QAction* act = ratingMenu->addAction(stars, [mediator, ids, i]() {
            if (mediator) mediator->requestSetRating(ids, i);
        });
        act->setCheckable(true);
        if (i == currentRating) act->setChecked(true);
        starGroup->addAction(act);
    }
    ratingMenu->addSeparator();
    ratingMenu->addAction("清除评级", [mediator, ids]() {
        if (mediator) mediator->requestSetRating(ids, 0);
    });

    // 收藏切换
    bool isFavorite = selected.first().data(NoteModel::FavoriteRole).toBool();
    menu->addAction(IconHelper::getIcon(isFavorite ? "bookmark_filled" : "bookmark", isFavorite ? "#2ECC71" : "#aaaaaa", 18),
                    isFavorite ? "取消收藏" : "添加收藏" + getShortcutHint("qw_favorite"), [mediator, ids]() {
        if (mediator) mediator->requestToggleFavorite(ids);
    });

    // 置顶切换
    bool isPinned = selected.first().data(NoteModel::PinnedRole).toBool();
    menu->addAction(IconHelper::getIcon(isPinned ? "pin_vertical" : "pin_tilted", isPinned ? "#FF551C" : "#aaaaaa", 18),
                    isPinned ? "取消置顶" : "置顶选中项" + getShortcutHint("qw_pin"), [mediator, ids, isPinned]() {
        if (mediator) mediator->requestTogglePinned(ids, !isPinned);
    });

    menu->addSeparator();

    // 移动到分类子菜单
    auto* catMenu = menu->addMenu(IconHelper::getIcon("branch", "#cccccc", 18), QString("移动选中项到分类 (%1)").arg(selCount));
    catMenu->addAction(IconHelper::getIcon("uncategorized", "#e67e22", 18), "未分类", [mediator, ids]() {
        if (mediator) mediator->requestMoveToCategory(ids, -1);
    });

    QVariantList recentCats = StringUtils::getRecentCategories();
    auto allCategories = DatabaseManager::instance().getAllCategories();
    QMap<int, QVariantMap> catMap;
    for (const auto& cat : std::as_const(allCategories)) {
        catMap[cat.value("id").toInt()] = cat;
    }
    int count = 0;
    for (const auto& v : std::as_const(recentCats)) {
        if (count >= 10) break;
        int cid = v.toInt();
        if (catMap.contains(cid)) {
            const auto& cat = catMap.value(cid);
            catMenu->addAction(IconHelper::getIcon("branch", cat.value("color").toString(), 18), cat.value("name").toString(), [mediator, ids, cid]() {
                if (mediator) mediator->requestMoveToCategory(ids, cid);
            });
            count++;
        }
    }

    menu->addSeparator();

    // 回收站模式 vs 常规删除
    bool inTrash = (window->m_currentFilterType == "trash");
    if (inTrash) {
        QString restoreText = (selCount > 1) ? QString("恢复选中项 (%1)").arg(selCount) : "恢复";
        menu->addAction(IconHelper::getIcon("refresh", "#2ecc71", 18), restoreText, [window, mediator, selected]() {
            QList<int> noteIds;
            QList<int> catIds;
            for (const auto& idx : selected) {
                QString type = idx.data(NoteModel::TypeRole).toString();
                int id = idx.data(NoteModel::IdRole).toInt();
                if (type == "deleted_category") catIds << id;
                else noteIds << id;
            }
            if (mediator) mediator->requestRestoreFromTrash(noteIds, catIds);
            QString successMsg = (selected.size() > 1) ? QString("[OK] 已恢复选中的 %1 个项目").arg(selected.size()) : "[OK] 已恢复 1 个项目";
            ToolTipOverlay::instance()->showText(QCursor::pos(), "<b style='color: #2ecc71;'>" + successMsg + "</b>");
        });

        menu->addAction(IconHelper::getIcon("refresh", "#3498db", 18), "全部恢复 (还原所有)", [mediator]() {
            if (mediator) {
                mediator->requestRestoreAllTrash();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "<b style='color: #2ecc71;'>[OK] 已将回收站内容全量还原</b>");
            }
        });

        menu->addAction(IconHelper::getIcon("trash", "#e74c3c", 18), "彻底删除 (不可逆)", window, [window]() {
            window->doDeleteSelected(true);
        });
    } else {
        menu->addAction(IconHelper::getIcon("trash", "#e74c3c", 18), "删除" + getShortcutHint("qw_delete_soft"), window, [window]() {
            window->doDeleteSelected(false);
        });
    }

    // 排序菜单
    if (window->m_currentFilterType != "recently_visited") {
        menu->addSeparator();
        auto* sortMenu = menu->addMenu(IconHelper::getIcon("list_ol", "#aaaaaa", 18), "排列");
        sortMenu->addAction("上移" + getShortcutHint("qw_move_up"), [window]() { window->doMoveNote(DatabaseManager::Up); });
        sortMenu->addAction("下移" + getShortcutHint("qw_move_down"), [window]() { window->doMoveNote(DatabaseManager::Down); });
        sortMenu->addAction("移至顶部", [window]() { window->doMoveNote(DatabaseManager::Top); });
        sortMenu->addAction("移至底部", [window]() { window->doMoveNote(DatabaseManager::Bottom); });
        sortMenu->addSeparator();
        sortMenu->addAction("按标题 A→Z 排列", [mediator, window]() {
            if (mediator) mediator->requestReorderNotes(window->m_currentFilterType, window->m_currentFilterValue, true);
        });
        sortMenu->addAction("按标题 Z→A 排列", [mediator, window]() {
            if (mediator) mediator->requestReorderNotes(window->m_currentFilterType, window->m_currentFilterValue, false);
        });

        if (window->m_currentFilterType == "category" && window->m_currentFilterValue.toInt() > 0) {
            sortMenu->addSeparator();
            int catId = window->m_currentFilterValue.toInt();
            QString currentMode = DatabaseManager::instance().getCategorySortMode(catId);

            auto* manualAction = sortMenu->addAction("手动排序模式", [mediator, catId]() {
                if (mediator) mediator->requestSetCategorySortMode(catId, "manual");
            });
            manualAction->setCheckable(true);
            manualAction->setChecked(currentMode != "created_at_desc");

            auto* createdAction = sortMenu->addAction("按创建日期排列", [mediator, catId]() {
                if (mediator) mediator->requestSetCategorySortMode(catId, "created_at_desc");
            });
            createdAction->setCheckable(true);
            createdAction->setChecked(currentMode == "created_at_desc");
        }
    }

    return menu;
}

QMenu* QuickContextMenuBuilder::buildSidebarContextMenu(QuickWindow* window, DropTreeView* tree, const QModelIndex& index, const QModelIndexList& selected, QWidget* parent) {
    if (!window || !tree) return nullptr;
    auto* menu = new QMenu(parent);
    IconHelper::setupMenu(menu);

    auto* mediator = window->m_mediator;
    QString type = index.data(CategoryModel::TypeRole).toString();
    QString idxName = index.data(CategoryModel::NameRole).toString();

    static const QStringList silentTypes = { "recently_visited", "untagged" };
    if (silentTypes.contains(type)) {
        delete menu;
        return nullptr;
    }

    // 镜像组快速访问移除
    QModelIndex parentIdx = index.parent();
    if (parentIdx.isValid() && parentIdx.data(CategoryModel::NameRole).toString() == "快速访问") {
        int catId = index.data(CategoryModel::IdRole).toInt();
        menu->addAction(IconHelper::getIcon("pin_vertical", "#f1c40f", 18), "从“快速访问”移除", [mediator, catId]() {
            if (mediator) mediator->requestToggleCategoryPinned(catId);
        });
        return menu;
    }

    if (type == "all") {
        menu->addAction(IconHelper::getIcon("file_export", "#3498db", 18), "导出完整结构数据", [window]() {
            if (!window->verifyExportPermission()) return;
            FileStorageHelper::exportFullStructure(window);
        });
        return menu;
    }

    if (type == "today" || type == "yesterday" || type == "bookmark") {
        menu->addAction(IconHelper::getIcon("file_export", "#3498db", 18), QString("导出 [%1]").arg(idxName), [window, type, idxName]() {
            if (!window->verifyExportPermission()) return;
            FileStorageHelper::exportByFilter(type, QVariant(), idxName, window);
        });
        return menu;
    }

    // 空白处或“我的分类”根节点
    if (!index.isValid() || idxName == "我的分类") {
        menu->addAction(IconHelper::getIcon("add", "#3498db", 18), "新建分类", [window, mediator]() {
            FramelessInputDialog dlg("新建分类", "组名称:", "", window);
            if (dlg.exec() == QDialog::Accepted && !dlg.text().trimmed().isEmpty()) {
                if (mediator) mediator->requestAddCategory(dlg.text().trimmed());
            }
        });
        auto* importMenu = menu->addMenu(IconHelper::getIcon("file_import", "#1abc9c", 18), "导入数据");
        importMenu->addAction(IconHelper::getIcon("file", "#1abc9c", 18), "导入文件(s)...", [window]() {
            window->doImportCategory(-1);
        });
        importMenu->addAction(IconHelper::getIcon("folder", "#1abc9c", 18), "导入文件夹...", [window]() {
            window->doImportFolder(-1);
        });
        return menu;
    }

    if (type == "category") {
        int catId = index.data(CategoryModel::IdRole).toInt();
        QString currentName = index.data(CategoryModel::NameRole).toString();

        menu->addAction(IconHelper::getIcon("add", "#3498db", 18), "新建数据", [window, catId]() {
            auto* win = new NoteEditWindow();
            win->setDefaultCategory(catId);
            QObject::connect(win, &NoteEditWindow::noteSaved, window, &QuickWindow::refreshData);
            win->show();
        });
        menu->addSeparator();

        auto* importMenu = menu->addMenu(IconHelper::getIcon("file_import", "#1abc9c", 18), "导入数据");
        importMenu->addAction(IconHelper::getIcon("file", "#1abc9c", 18), "导入文件(s)...", [window, catId]() {
            window->doImportCategory(catId);
        });
        importMenu->addAction(IconHelper::getIcon("folder", "#1abc9c", 18), "导入文件夹...", [window, catId]() {
            window->doImportFolder(catId);
        });
        importMenu->addAction(IconHelper::getIcon("package_rnp", "#9b59b6", 18), "导入专属安装包 (.rnp)", [window]() {
            FileStorageHelper::importFromPackage(window);
            window->scheduleRefresh();
        });

        auto* exportMenu = menu->addMenu(IconHelper::getIcon("file_export", "#3498db", 18), "导出");
        QVariantMap rootCat = DatabaseManager::instance().getRootCategory(catId);
        QString rootName = rootCat.value("name").toString();
        int rootId = rootCat.value("id").toInt();

        exportMenu->addAction(IconHelper::getIcon("folder", "#3498db", 18), "整分类", [window, rootId, rootName]() {
            if (!window->verifyExportPermission()) return;
            FileStorageHelper::exportCategoryRecursive(rootId, rootName, window);
        });
        exportMenu->addAction(IconHelper::getIcon("package_rnp", "#9b59b6", 18), "导出为数据包 (.rnp)", [window, catId, currentName]() {
            if (!window->verifyExportPermission()) return;
            FileStorageHelper::exportToPackage(catId, currentName, window);
        });

        menu->addSeparator();
        menu->addAction(IconHelper::getIcon("palette", "#e67e22", 18), "设置颜色", [window, mediator, catId]() {
            auto* dlg = new QColorDialog(Qt::gray, window);
            dlg->setWindowTitle("选择分类颜色");
            dlg->setWindowFlags(dlg->windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            QObject::connect(dlg, &QColorDialog::colorSelected, [mediator, catId](const QColor& color) {
                if (color.isValid() && mediator) mediator->requestSetCategoryColor(catId, color.name());
            });
            QObject::connect(dlg, &QColorDialog::finished, dlg, &QObject::deleteLater);
            dlg->show();
        });

        menu->addAction(IconHelper::getIcon("random_color", "#FF6B9D", 18), "随机颜色", [mediator, catId]() {
            static const QStringList palette = {
                "#FF6B6B", "#4ECDC4", "#45B7D1", "#96CEB4", "#FFEEAD",
                "#D4A5A5", "#9B59B6", "#3498DB", "#E67E22", "#2ECC71",
                "#E74C3C", "#F1C40F", "#1ABC9C", "#34495E", "#95A5A6"
            };
            QString chosenColor = palette.at(QRandomGenerator::global()->bounded(palette.size()));
            if (mediator) mediator->requestSetCategoryColor(catId, chosenColor);
        });

        menu->addAction(IconHelper::getIcon("tag", "#FFAB91", 18), "设置预设标签", [window, mediator, catId]() {
            QString currentTags = DatabaseManager::instance().getCategoryPresetTags(catId);
            FramelessInputDialog dlg("设置预设标签", "标签 (逗号分隔):", currentTags, window);
            if (dlg.exec() == QDialog::Accepted && mediator) {
                mediator->requestSetCategoryPresetTags(catId, dlg.text().trimmed());
            }
        });

        menu->addAction(IconHelper::getIcon("switch_on", "#00A650", 18), "归类到此分类", [catId, currentName]() {
            DatabaseManager::instance().setAutoCategorizeEnabled(true);
            DatabaseManager::instance().setActiveCategoryId(catId);
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("<b style='color: #00A650;'>[OK] 已将“%1”设为自动归档目标并开启开关</b>").arg(currentName));
        });

        menu->addSeparator();

        menu->addAction(IconHelper::getIcon("add", "#3498db", 18), "新建分类", [window, mediator]() {
            FramelessInputDialog dlg("新建分类", "组名称:", "", window);
            if (dlg.exec() == QDialog::Accepted && !dlg.text().trimmed().isEmpty() && mediator) {
                mediator->requestAddCategory(dlg.text().trimmed());
            }
        });

        menu->addAction(IconHelper::getIcon("add", "#3498db", 18), "新建子分类", [window, mediator, catId]() {
            FramelessInputDialog dlg("新建子分类", "区名称:", "", window);
            if (dlg.exec() == QDialog::Accepted && !dlg.text().trimmed().isEmpty() && mediator) {
                mediator->requestAddCategory(dlg.text().trimmed(), catId);
            }
        });

        menu->addSeparator();

        if (selected.size() == 1) {
            bool isPinned = index.data(CategoryModel::PinnedRole).toBool();
            menu->addAction(IconHelper::getIcon(isPinned ? "pin_vertical" : "pin_tilted", isPinned ? "#FF551C" : "#aaaaaa", 18),
                            isPinned ? "从“快速访问”移除" : "添加至“快速访问”", [mediator, catId]() {
                if (mediator) mediator->requestToggleCategoryPinned(catId);
            });

            menu->addAction(IconHelper::getIcon("edit", "#aaaaaa", 18), "重命名", [tree, index]() {
                tree->edit(index);
            });
        }

        QString delText = (selected.size() > 1) ? QString("删除选中的 %1 个分类").arg(selected.size()) : "删除";
        menu->addAction(IconHelper::getIcon("trash", "#e74c3c", 18), delText, [window, mediator, selected]() {
            QString msg = (selected.size() > 1) ? "确定要删除选中的分类吗？\n(分类将永久消失，内容将删除)" : "确定要删除此分类吗？\n(分类将永久消失，内容将删除)";
            FramelessMessageBox dlg("确认删除", msg, window);
            if (dlg.exec() == QDialog::Accepted && mediator) {
                QList<int> ids;
                for (const auto& idx : selected) {
                    if (idx.data(CategoryModel::TypeRole).toString() == "category") {
                        ids << idx.data(CategoryModel::IdRole).toInt();
                    }
                }
                mediator->requestDeleteCategories(ids, true);
            }
        });

        menu->addSeparator();

        auto* sortMenu = menu->addMenu(IconHelper::getIcon("list_ol", "#aaaaaa", 18), "排列");
        int parentId = -1;
        QModelIndex pIdx = index.parent();
        if (pIdx.isValid() && pIdx.data(CategoryModel::TypeRole).toString() == "category") {
            parentId = pIdx.data(CategoryModel::IdRole).toInt();
        }

        sortMenu->addAction("标题(当前层级) (A→Z)", [mediator, parentId]() {
            if (mediator) mediator->requestReorderCategories(parentId, true);
        });
        sortMenu->addAction("标题(当前层级) (Z→A)", [mediator, parentId]() {
            if (mediator) mediator->requestReorderCategories(parentId, false);
        });
        sortMenu->addAction("标题(全部) (A→Z)", [mediator]() {
            if (mediator) mediator->requestReorderAllCategories(true);
        });
        sortMenu->addAction("标题(全部) (Z→A)", [mediator]() {
            if (mediator) mediator->requestReorderAllCategories(false);
        });

        menu->addSeparator();

        // 密码保护
        auto* pwdMenu = menu->addMenu(IconHelper::getIcon("lock", "#aaaaaa", 18), "密码保护");
        pwdMenu->addAction("设置", [window, catId]() {
            QTimer::singleShot(0, [window, catId]() {
                CategoryPasswordDialog dlg("设置密码", window);
                if (dlg.exec() == QDialog::Accepted) {
                    DatabaseManager::instance().setCategoryPassword(catId, dlg.password(), dlg.passwordHint());
                    window->scheduleRefresh();
                }
            });
        });
        pwdMenu->addAction("修改", [window, catId]() {
            QTimer::singleShot(0, [window, catId]() {
                FramelessInputDialog verifyDlg("验证旧密码", "请输入当前密码:", "", window);
                verifyDlg.setEchoMode(QLineEdit::Password);
                if (verifyDlg.exec() == QDialog::Accepted) {
                    if (DatabaseManager::instance().verifyCategoryPassword(catId, verifyDlg.text())) {
                        CategoryPasswordDialog dlg("修改密码", window);
                        QString currentHint;
                        auto cats = DatabaseManager::instance().getAllCategories();
                        for (const auto& c : std::as_const(cats)) {
                            if (c.value("id").toInt() == catId) currentHint = c.value("password_hint").toString();
                        }
                        dlg.setInitialData(currentHint);
                        if (dlg.exec() == QDialog::Accepted) {
                            DatabaseManager::instance().setCategoryPassword(catId, dlg.password(), dlg.passwordHint());
                            window->scheduleRefresh();
                        }
                    } else {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "<b style='color: #e74c3c;'>[ERR] 旧密码验证失败</b>");
                    }
                }
            });
        });
        pwdMenu->addAction("移除", [window, catId]() {
            QTimer::singleShot(0, [window, catId]() {
                FramelessInputDialog dlg("验证密码", "请输入当前密码以移除保护:", "", window);
                dlg.setEchoMode(QLineEdit::Password);
                if (dlg.exec() == QDialog::Accepted) {
                    if (DatabaseManager::instance().verifyCategoryPassword(catId, dlg.text())) {
                        DatabaseManager::instance().removeCategoryPassword(catId);
                        window->scheduleRefresh();
                    } else {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "<b style='color: #e74c3c;'>[ERR] 密码错误</b>");
                    }
                }
            });
        });
        pwdMenu->addAction("立即锁定", [window, catId]() {
            DatabaseManager::instance().lockCategory(catId);
            window->scheduleRefresh();
        })->setShortcut(QKeySequence("Ctrl+S"));

        return menu;
    }

    if (idxName == "未分类" || type == "uncategorized") {
        menu->addAction(IconHelper::getIcon("add", "#3498db", 18), "新建数据", [window]() {
            auto* win = new NoteEditWindow();
            QObject::connect(win, &NoteEditWindow::noteSaved, window, &QuickWindow::refreshData);
            win->show();
        });
        menu->addSeparator();
        menu->addAction(IconHelper::getIcon("file_export", "#3498db", 18), "导出 [未分类]", [window]() {
            if (!window->verifyExportPermission()) return;
            FileStorageHelper::exportByFilter("uncategorized", -1, "未分类灵感", window);
        });
        return menu;
    }

    if (idxName == "回收站" || type == "trash") {
        menu->addAction(IconHelper::getIcon("refresh", "#2ecc71", 18), "全部恢复（还原所有）", [mediator]() {
            if (mediator) mediator->requestRestoreAllTrash();
        });
        menu->addSeparator();
        menu->addAction(IconHelper::getIcon("trash", "#e74c3c", 18), "清空回收站", [window, mediator]() {
            FramelessMessageBox dlg("确认清空", "确定要永久删除回收站中的所有内容吗？\n(此操作不可逆)", window);
            if (dlg.exec() == QDialog::Accepted && mediator) {
                mediator->requestEmptyTrash();
            }
        });
        return menu;
    }

    return menu;
}

} // namespace RapidNotes