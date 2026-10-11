#include "ContextMenuFactory.h"
#include "../UiHelper.h"
#include "../../util/ShellHelper.h"
#include "../ToolTipOverlay.h"
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

namespace QuarkMeta {

bool ContextMenuFactory::togglePinState(const QStringList& paths, bool pin) {
    if (paths.isEmpty()) return false;

    AppCommand cmd;
    cmd.type = AppCommandType::SetPinned;
    cmd.targetPaths = paths;
    cmd.params["pinned"] = pin;
    return CoreEngine::instance().executeCommand(cmd);
}

bool ContextMenuFactory::extractContentToClipboard(const QString& path) {
    if (path.isEmpty() || QFileInfo(path).isDir()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "提取失败：只能提取文件内容", 1500, QColor("#e81123"));
        return false;
    }

    QString ext = QFileInfo(path).suffix().toLower();
    if (!UiHelper::isTextFile(ext)) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "不支持提取内容：文件格式超出纯文本范围", 1500, QColor("#e81123"));
        return false;
    }

    QString content;
    if (UiHelper::extractTextContent(path, content)) {
        QApplication::clipboard()->setText(content);
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已成功提取内容并存入剪贴板 (共 %1 字符)").arg(content.length()), 1500, QColor("#2ecc71"));
        return true;
    } else {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "提取失败：文件超过限制或无法作为纯文本解析", 1500, QColor("#e81123"));
        return false;
    }
}

QAction* ContextMenuFactory::buildExtractContentAction(QMenu* menu, const QString& path, QObject* receiver) {
    if (!menu || path.isEmpty()) return nullptr;

    bool isFolder = QFileInfo(path).isDir();
    QString fileExt = QFileInfo(path).suffix().toLower();
    bool canExtract = !isFolder && UiHelper::isTextFile(fileExt);

    if (canExtract) {
        QAction* actExtract = menu->addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "支持提取内容");
        QObject::connect(actExtract, &QAction::triggered, receiver ? receiver : menu, [path]() {
            extractContentToClipboard(path);
        });
        return actExtract;
    } else {
        QAction* actDisabled = menu->addAction(UiHelper::getIcon("prohibit", QColor("#888888"), 18), "不支持提取内容");
        actDisabled->setEnabled(false);
        return actDisabled;
    }
}

QAction* ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver) {
    if (!menu || path.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [path]() {
        ShellHelper::openInExplorer(path);
    });
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
        {"默认文件夹", "folder_filled"}, {"照片媒体", "image_filled"}, {"相册图片", "image_picture"},
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
        {"托管文件夹", "folder_managed"}, {"托管文件", "file_managed"}, {"ZIP压缩包", "zip"},
        {"下载图标", "download-svgrepo-com"}, {"云端下载", "cloud_download"}, {"Word文档", "file_word"},
        {"棕榈树", "palm_tree"}, {"代码文件夹", "folder-code-svgrepo-com"}, {"下载盒", "archive-svgrepo-com"}
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

    QStringList cleanPaths;
    for (const auto& p : paths) {
        if (!p.isEmpty()) {
            cleanPaths << QDir::toNativeSeparators(p);
        }
    }
    if (cleanPaths.isEmpty()) return false;

    QApplication::clipboard()->setText(cleanPaths.join("\r\n"));
    if (showOverlay) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制路径至剪贴板", 1500, Style::SuccessGreen);
    }
    return true;
}

bool ContextMenuFactory::copyNamesToClipboard(const QStringList& paths, bool showOverlay) {
    if (paths.isEmpty()) return false;

    QStringList names;
    for (const auto& p : paths) {
        if (!p.isEmpty()) {
            names << QFileInfo(p).fileName();
        }
    }
    if (names.isEmpty()) return false;

    QApplication::clipboard()->setText(names.join("\r\n"));
    if (showOverlay) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制名称至剪贴板", 1500, Style::SuccessGreen);
    }
    return true;
}

QAction* ContextMenuFactory::buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制完整路径");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        copyPathsToClipboard(paths, true);
    });
    return action;
}

QAction* ContextMenuFactory::buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        copyNamesToClipboard(paths, true);
    });
    return action;
}

QAction* ContextMenuFactory::buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver) {
    if (!menu) return nullptr;

    QIcon icon = UiHelper::getIcon(isPinned ? "pin_tilted" : "pin_vertical", QColor("#EEEEEE"), 18);
    QString text = isPinned ? "取消置顶" : "置顶";

    QAction* action = menu->addAction(icon, text);
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [isPinned, onToggle]() {
        if (onToggle) onToggle(!isPinned);
    });
    return action;
}

} // namespace QuarkMeta
