#include "ContextMenuFactory.h"
#include "../UiHelper.h"
#include "../ToolTipOverlay.h"
#include "../TabBarWidget.h"
#include "../../core/NavigationService.h"
#include "../StyleLibrary.h"
#include "../ColorPicker.h"
#include "../../core/CoreEngine.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QWidgetAction>
#include <QGridLayout>
#include <QPushButton>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>

namespace RapidNotes {

bool ContextMenuFactory::togglePinState(const QStringList& paths, bool pin) {
    if (paths.isEmpty()) return false;

    AppCommand cmd;
    cmd.type = AppCommandType::SetPinned;
    cmd.targetPaths = paths;
    cmd.params["pinned"] = pin;
    return CoreEngine::instance().executeCommand(cmd);
}

bool ContextMenuFactory::extractContentToClipboard(const QString& path) {
    if (path.isEmpty()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "提取失败：未选择有效条目", 1500, QColor("#e81123"));
        return false;
    }

    ToolTipOverlay::instance()->showText(QCursor::pos(), "笔记内容请在详情或卡片中查看", 1500, QColor("#3498db"));
    return true;
}

QAction* ContextMenuFactory::buildExtractContentAction(QMenu* menu, const QString& path, QObject* receiver) {
    if (!menu || path.isEmpty()) return nullptr;

    QAction* actExtract = menu->addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "提取标题至剪贴板");
    QObject::connect(actExtract, &QAction::triggered, receiver ? receiver : menu, [path]() {
        extractContentToClipboard(path);
    });
    return actExtract;
}

QAction* ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, bool isBlankSpace, QObject* receiver) {
    Q_UNUSED(isBlankSpace);
    if (!menu || path.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), kTextShowInExplorer);
    action->setEnabled(false); // 笔记领域无物理资源管理器定位
    return action;
}

QMenu* ContextMenuFactory::buildIconPickerMenu(QMenu* parentMenu,
                                               const QString& curIconKey,
                                               const QString& curColorHex,
                                               std::function<void(const QString& iconKey)> onIconSelected,
                                               std::function<void(const QString& colorHex)> onColorSelected) {
    if (!parentMenu) return nullptr;

    QString iconKey = curIconKey.isEmpty() ? "folder_filled" : curIconKey;
    QString colorHex = curColorHex.isEmpty() ? "#888888" : curColorHex;

    // 1. 颜色条组件
    QWidgetAction* colorPickerAction = new QWidgetAction(parentMenu);
    ColorStripPicker* colorPickerWidget = new ColorStripPicker(colorHex, parentMenu);
    colorPickerAction->setDefaultWidget(colorPickerWidget);
    parentMenu->addAction(colorPickerAction);

    // 2. 切换图标二级菜单
    QMenu* iconMenu = parentMenu->addMenu(UiHelper::getIcon(iconKey, QColor(colorHex)), "切换图标");
    UiHelper::applyMenuStyle(iconMenu);

    QWidgetAction* pickerAction = new QWidgetAction(iconMenu);
    QWidget* pickerWidget = new QWidget(iconMenu);
    QGridLayout* pickerLayout = new QGridLayout(pickerWidget);
    pickerLayout->setContentsMargins(6, 6, 6, 6);
    pickerLayout->setSpacing(6);

    static const QList<QPair<QString, QString>> builtInIcons = {
        {"默认分类", "folder_filled"}, {"照片媒体", "image_filled"}, {"相册图片", "image_picture"},
        {"时钟历史", "clock_filled"}, {"星标收藏", "star_filled"}, {"实心星标", "star_001"},
        {"空心星标", "star_002"}, {"爱心常用", "heart_filled"}, {"附加文档", "document_attach"},
        {"网络球体", "globe_filled"}, {"主页主路径", "home_filled"}, {"标签标记", "tag_filled"},
        {"书签指示", "bookmark_filled"}, {"视频影视", "video_filled"}, {"摄影相机", "camera_filled"},
        {"盾牌防护", "shield_filled"}, {"物理硬盘", "hard_drive"}, {"云端同步", "cloud_filled"},
        {"闪电极速", "zap_filled"}, {"魔法火花", "sparkles_filled"}, {"旗帜标记", "flag_filled"},
        {"旗帜标示", "flag"}, {"礼物珍藏", "gift_filled"}, {"奖星勋章", "award_filled"},
        {"回收废弃", "trash_filled"}, {"邮件通信", "mail_filled"}, {"电话联系", "phone_filled"},
        {"日历日程", "calendar_filled"}, {"今日任务", "today_filled"}, {"数据表格", "table_filled"},
        {"磁盘保存", "save_filled"}, {"附件剪辑", "paperclip"}, {"归档文件", "archive"},
        {"OneNote笔记", "onenote"}, {"下载中心", "download"}, {"全部数据", "all_data"},
        {"分类管理", "Data-Classification.svg"}
    };

    QColor catColor = QColor(colorHex);
    int row = 0, col = 0;
    QList<QPair<QPushButton*, QString>> iconButtons;

    for (const auto& pair : builtInIcons) {
        QString key = pair.second;
        QPushButton* btn = new QPushButton(pickerWidget);
        btn->setFixedSize(28, 28);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setObjectName("FavPickerIconBtn");
        btn->setIcon(UiHelper::getIcon(key, catColor, 18));
        btn->setIconSize(QSize(18, 18));
        pickerLayout->addWidget(btn, row, col);

        iconButtons.append({btn, key});

        QObject::connect(btn, &QPushButton::clicked, parentMenu, [key, onIconSelected]() {
            if (onIconSelected) onIconSelected(key);
        });

        col++;
        if (col >= 5) { col = 0; row++; }
    }

    pickerWidget->setLayout(pickerLayout);
    pickerAction->setDefaultWidget(pickerWidget);
    iconMenu->addAction(pickerAction);

    QObject::connect(colorPickerWidget, &ColorStripPicker::colorSelected, parentMenu, [iconMenu, iconButtons, onColorSelected](const QString& selectedHex) {
        QString finalColor = selectedHex.isEmpty() ? "#888888" : selectedHex.toUpper();
        iconMenu->setIcon(UiHelper::getIcon("folder_filled", QColor(finalColor)));
        for (const auto& btnPair : iconButtons) {
            btnPair.first->setIcon(UiHelper::getIcon(btnPair.second, QColor(finalColor), 18));
        }
        if (onColorSelected) onColorSelected(finalColor);
    });

    parentMenu->addSeparator();
    return iconMenu;
}

bool ContextMenuFactory::copyPathsToClipboard(const QStringList& paths, bool showOverlay) {
    if (paths.isEmpty()) return false;

    QApplication::clipboard()->setText(paths.join("\r\n"));
    if (showOverlay) {
        ToolTipOverlay::instance()->showToast("已复制标识至剪贴板", 1500, Style::SuccessGreen);
    }
    return true;
}

bool ContextMenuFactory::copyNamesToClipboard(const QStringList& paths, NameDelimiter delimiter, bool showOverlay) {
    if (paths.isEmpty()) return false;

    QStringList names;
    for (const auto& p : paths) {
        if (!p.isEmpty()) {
            QString name = p;
            if (name.startsWith("note://")) {
                name = name.mid(7);
            }
            if (!name.isEmpty() && !names.contains(name)) {
                names << name;
            }
        }
    }
    if (names.isEmpty()) return false;

    QString sep;
    switch (delimiter) {
        case NameDelimiter::NewLine:
            sep = "\r\n";
            break;
        case NameDelimiter::Enumeration:
            sep = "、";
            break;
        case NameDelimiter::Comma:
            sep = ",";
            break;
    }

    QApplication::clipboard()->setText(names.join(sep));
    if (showOverlay) {
        ToolTipOverlay::instance()->showToast("已复制名称至剪贴板", 1500, Style::SuccessGreen);
    }
    return true;
}

bool ContextMenuFactory::copyNamesToClipboard(const QStringList& paths, bool showOverlay) {
    return copyNamesToClipboard(paths, NameDelimiter::NewLine, showOverlay);
}

QAction* ContextMenuFactory::buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制条目标识");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        copyPathsToClipboard(paths, true);
    });
    return action;
}

QMenu* ContextMenuFactory::buildTabCascadeMenu(QMenu* parentMenu, const QString& path, QWidget* contextWidget) {
    if (!parentMenu || path.isEmpty()) return nullptr;

    QMenu* tabCascadeMenu = parentMenu->addMenu(UiHelper::getIcon("add", QColor("#EEEEEE"), 18), "标签页");
    UiHelper::applyMenuStyle(tabCascadeMenu);

    QAction* actOpenNewTab = tabCascadeMenu->addAction(UiHelper::getIcon("add", QColor("#EEEEEE"), 16), "在新标签页中打开");
    QObject::connect(actOpenNewTab, &QAction::triggered, contextWidget ? contextWidget : parentMenu, [path]() {
        NavigationService::instance().openInNewTab(path);
    });

    QMenu* openInExistingTabMenu = tabCascadeMenu->addMenu(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 16), "在标签页中打开");
    UiHelper::applyMenuStyle(openInExistingTabMenu);

    QWidget* windowWidget = contextWidget ? contextWidget->window() : nullptr;
    TabBarWidget* tabBar = windowWidget ? windowWidget->findChild<TabBarWidget*>() : nullptr;
    if (tabBar) {
        int count = tabBar->tabCount();
        for (int i = 0; i < count; ++i) {
            QString tabTitle = tabBar->tabTitle(i);
            QAction* actMergeTab = openInExistingTabMenu->addAction(UiHelper::getIcon("tab", QColor("#EEEEEE"), 16), tabTitle);
            QObject::connect(actMergeTab, &QAction::triggered, contextWidget ? contextWidget : parentMenu, [tabBar, i, path]() {
                tabBar->openInExistingTab(i, path);
            });
        }
    }

    return tabCascadeMenu;
}

QMenu* ContextMenuFactory::buildOpenWithMenu(QMenu* parentMenu, const QString& filePath, QWidget* parentWidget) {
    if (filePath.isEmpty()) return nullptr;
    return buildOpenWithMenu(parentMenu, QStringList() << filePath, parentWidget);
}

QMenu* ContextMenuFactory::buildOpenWithMenu(QMenu* parentMenu, const QStringList& filePaths, QWidget* parentWidget) {
    Q_UNUSED(filePaths);
    Q_UNUSED(parentWidget);
    if (!parentMenu) return nullptr;
    return nullptr;
}

QAction* ContextMenuFactory::buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QMenu* subMenu = menu->addMenu(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称");
    UiHelper::applyMenuStyle(subMenu);

    QObject* target = receiver ? receiver : menu;

    QAction* actNewLine = subMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "分行名称");
    QObject::connect(actNewLine, &QAction::triggered, target, [paths]() {
        copyNamesToClipboard(paths, NameDelimiter::NewLine, true);
    });

    QAction* actEnum = subMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "顿号分割名称");
    QObject::connect(actEnum, &QAction::triggered, target, [paths]() {
        copyNamesToClipboard(paths, NameDelimiter::Enumeration, true);
    });

    QAction* actComma = subMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "逗号分割名称");
    QObject::connect(actComma, &QAction::triggered, target, [paths]() {
        copyNamesToClipboard(paths, NameDelimiter::Comma, true);
    });

    return subMenu->menuAction();
}

QAction* ContextMenuFactory::buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver) {
    if (!menu) return nullptr;

    QIcon icon = UiHelper::getIcon(isPinned ? "pin_tilted" : "pin_vertical", QColor("#EEEEEE"), 18);
    QString text = isPinned ? "取消置顶（Alt+D）" : "置顶（Alt+D）";

    QAction* action = menu->addAction(icon, text);
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [isPinned, onToggle]() {
        if (onToggle) onToggle(!isPinned);
    });
    return action;
}

} // namespace RapidNotes
