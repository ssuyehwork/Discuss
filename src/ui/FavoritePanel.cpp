#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "../util/DiskMediaExtractor.h"
#include "ColorPicker.h"
#include "../meta/FavoriteDao.h"
#include "../meta/MetadataManager.h"
#include "../meta/DriveMetaDao.h"
#include <QPainter>
#include <QPainterPath>
#include "../core/AppConfig.h"
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QWidgetAction>
#include <QGridLayout>
#include <QFileInfo>
#include <QDir>
#include <QHeaderView>
#include <QCoreApplication>
#include <QtConcurrent>
#include <QPointer>

namespace QuarkMeta {

void FavoriteItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    
    // 背景高亮
    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor("#37373D"));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }

    // 几何布局
    int leftMargin = 10;
    int iconSize = 18;
    int spacing = 8;

    QRect iconRect(opt.rect.left() + leftMargin, opt.rect.top() + (opt.rect.height() - iconSize) / 2, iconSize, iconSize);
    QRect textRect(iconRect.right() + spacing, opt.rect.top(), opt.rect.width() - leftMargin - iconSize - spacing, opt.rect.height());

    // 微卡片圆角绘制
    QVariant decoData = index.data(Qt::DecorationRole);
    bool isFolder = index.data(Qt::UserRole + 4).toBool();
    bool hasCustomThumb = index.data(Qt::UserRole + 5).toBool();

    if (!isFolder && hasCustomThumb && decoData.canConvert<QIcon>()) {
        QIcon icon = decoData.value<QIcon>();
        QPixmap pix = icon.pixmap(QSize(64, 64));

        if (!pix.isNull()) {
            QPainterPath clipPath;
            clipPath.addRoundedRect(iconRect, 3, 3);
            painter->save();
            painter->setClipPath(clipPath);

            QPixmap scaled = pix.scaled(iconRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            int x = iconRect.center().x() - scaled.width() / 2;
            int y = iconRect.center().y() - scaled.height() / 2;
            painter->drawPixmap(x, y, scaled);
            painter->restore();
        } else {
            icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }
    } else {
        QIcon icon = decoData.value<QIcon>();
        if (!icon.isNull()) {
            icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }
    }

    // 绘制文本
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
    painter->setFont(opt.font);
    
    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width() - 6);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    painter->restore();
}

FavoritePanel::FavoritePanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("FavoriteContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    loadFavorites();
}

void FavoritePanel::setFocusHighlight(bool visible) {
    Q_UNUSED(visible);
}

void FavoritePanel::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);
// ContainerHeader in style.qss
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(15, 0, 5, 0);
    headerLayout->setSpacing(5);

    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("star_filled", QColor("#FDB70A"), 18).pixmap(18, 18));
    headerLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("收藏夹", header);
    titleLabel->setObjectName("FavoritePanelTitleLabel");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_favoriteView = new DropTreeView(this);
    m_favoriteView->setHeaderHidden(true);
    if (m_favoriteView->header()) {
        m_favoriteView->header()->setStretchLastSection(true);
        m_favoriteView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    m_favoriteView->setIndentation(0);
    m_favoriteView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_favoriteView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_favoriteView->setDragEnabled(true);
    m_favoriteView->setAcceptDrops(true);
    m_favoriteView->setDropIndicatorShown(true);
    m_favoriteView->setDefaultDropAction(Qt::MoveAction);
    m_favoriteView->setDragDropMode(QAbstractItemView::DragDrop);
    m_favoriteView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_favoriteView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_favoriteView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_favoriteModel = new QStandardItemModel(this);
    m_favoriteView->setModel(m_favoriteModel);
    m_favoriteView->setItemDelegate(new FavoriteItemDelegate(this));

    QString treeStyle = QString(
        "QTreeView { background-color: transparent; border: none; font-size: 12px; outline: none; padding-left: 10px; }"
        "QTreeView::item { height: 28px; padding-left: 0px; color: #EEEEEE; }"
        "QTreeView::item:hover { background-color: #2A2D2E; }"
        "QTreeView::item:selected { background-color: #37373D; color: #FFFFFF; }"
    );
    m_favoriteView->setStyleSheet(treeStyle);

    m_mainLayout->addWidget(m_favoriteView, 1);

    connect(m_favoriteView, &QTreeView::clicked, this, &FavoritePanel::onFavoriteClicked);
    connect(m_favoriteView, &QWidget::customContextMenuRequested, this, &FavoritePanel::onFavoriteContextMenu);
    connect(m_favoriteView, &DropTreeView::pathsDropped, this, &FavoritePanel::onPathsDroppedToFavorite);

    auto updateFavAndSave = [this](){
        if (!m_isLoading) saveFavorites();
    };
    connect(m_favoriteModel, &QStandardItemModel::rowsMoved, this, updateFavAndSave, Qt::QueuedConnection);
    connect(m_favoriteModel, &QStandardItemModel::rowsInserted, this, updateFavAndSave, Qt::QueuedConnection);
    connect(m_favoriteModel, &QStandardItemModel::rowsRemoved, this, updateFavAndSave, Qt::QueuedConnection);
}

void FavoritePanel::onFavoriteClicked(const QModelIndex& index) {
    QString path = index.data(Qt::UserRole + 1).toString();
    if (path.isEmpty()) return;

    QFileInfo fi(path);
    if (fi.isDir()) {
        emit directorySelected(path);
    } else {
        emit requestLocateFile(path);
    }
}

void FavoritePanel::onFavoriteContextMenu(const QPoint& pos) {
    QModelIndex index = m_favoriteView->indexAt(pos);
    if (!index.isValid()) return;

    QString path = index.data(Qt::UserRole + 1).toString();
    QString curIconKey = index.data(Qt::UserRole + 2).toString();
    QString curColorHex = index.data(Qt::UserRole + 3).toString();
    if (curIconKey.isEmpty()) curIconKey = "folder_filled";
    if (curColorHex.isEmpty()) curColorHex = "#FDB70A";

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    QFileInfo fi(path);
    bool isFolder = fi.isDir();
    bool isItemRemoved = false;

    // 缓存图标按钮指针，以便在换色时动态刷新子菜单图标色彩
    QList<QPair<QPushButton*, QString>> iconButtons;

    if (isFolder) {
        // 1. 颜色条组件
        QWidgetAction* colorPickerAction = new QWidgetAction(&menu);
        ColorStripPicker* colorPickerWidget = new ColorStripPicker(curColorHex, &menu);
        colorPickerAction->setDefaultWidget(colorPickerWidget);
        menu.addAction(colorPickerAction);

        // 2. 图标九宫格子菜单
        QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder_filled", QColor(curColorHex)), "切换图标");
        UiHelper::applyMenuStyle(iconMenu);

        QWidgetAction* pickerAction = new QWidgetAction(iconMenu);
        QWidget* pickerWidget = new QWidget(iconMenu);
        QGridLayout* pickerLayout = new QGridLayout(pickerWidget);
        pickerLayout->setContentsMargins(6, 6, 6, 6);
        pickerLayout->setSpacing(6);

        static const QList<QPair<QString, QString>> builtInIcons = {
            {"默认文件夹", "folder_filled"}, {"照片媒体", "image_filled"}, {"相册图片", "image_picture"},
            {"时钟历史", "clock_filled"}, {"星标收藏", "star_filled"}, {"实心星标", "star_001"},
            {"空心星标", "star_002"}, {"爱心常用", "heart_filled"}, {"加密安全", "lock_filled"},
            {"图书文档", "book"}, {"附加文档", "document_attach"}, {"配置管理", "settings_filled"},
            {"网络球体", "globe_filled"}, {"主页主路径", "home_filled"}, {"标签标记", "tag_filled"},
            {"书签指示", "bookmark_filled"}, {"音频音乐", "music_filled"}, {"视频影视", "video_filled"},
            {"摄影相机", "camera_filled"}, {"盾牌防护", "shield_filled"}, {"物理硬盘", "hard_drive"},
            {"云端同步", "cloud_filled"}, {"闪电极速", "zap_filled"}, {"魔法火花", "sparkles_filled"},
            {"旗帜标记", "flag_filled"}, {"旗帜标示", "flag"}, {"礼物珍藏", "gift_filled"},
            {"奖星勋章", "award_filled"}, {"回收废弃", "trash_filled"}, {"邮件通信", "mail_filled"},
            {"消息通知", "message_filled"}, {"电话联系", "phone_filled"}, {"地理定位", "map_pin_filled"},
            {"日光白天", "sun_filled"}, {"夜间月亮", "moon_filled"}, {"日历日程", "calendar_filled"},
            {"今日任务", "today_filled"}, {"九宫网格", "grid_filled"}, {"布局排版", "layout_filled"},
            {"数据表格", "table_filled"}, {"磁盘保存", "save_filled"}, {"魔棒工具", "wand_filled"},
            {"附件剪辑", "paperclip"}, {"归档文件", "archive"},
            {"OneNote笔记", "onenote"}, {"下载中心", "download"}
        };

        QColor catColor = QColor(curColorHex);
        int row = 0, col = 0;
        for (const auto& pair : builtInIcons) {
            QString iconKey = pair.second;
            QPushButton* btn = new QPushButton(pickerWidget);
            btn->setFixedSize(28, 28);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setObjectName("FavPickerIconBtn");
            btn->setIcon(UiHelper::getIcon(iconKey, catColor, 18));
            btn->setIconSize(QSize(18, 18));
            pickerLayout->addWidget(btn, row, col);

            iconButtons.append({btn, iconKey});

            // 🚀【持续点击 0ms 就地预览，绝对不调用 close()】
            connect(btn, &QPushButton::clicked, this, [this, index, iconKey]() {
                QStandardItem* item = m_favoriteModel->itemFromIndex(index);
                if (!item) return;

                QString colorHex = item->data(Qt::UserRole + 3).toString();
                if (colorHex.isEmpty()) colorHex = "#FDB70A";

                QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
                item->setIcon(newIcon);
                item->setData(iconKey, Qt::UserRole + 2);

                if (m_favoriteView && m_favoriteView->viewport()) {
                    m_favoriteView->viewport()->update();
                }
            });

            col++;
            if (col >= 5) { col = 0; row++; }
        }

        pickerWidget->setLayout(pickerLayout);
        pickerAction->setDefaultWidget(pickerWidget);
        iconMenu->addAction(pickerAction);

        // 🚀【持续改色 0ms 就地预览，绝对不调用 close()】
        connect(colorPickerWidget, &ColorStripPicker::colorSelected, this, [this, index, iconMenu, iconButtons](const QString& hexColor) {
            QStandardItem* item = m_favoriteModel->itemFromIndex(index);
            if (!item) return;

            QString finalColor = hexColor.isEmpty() ? "#FDB70A" : hexColor.toUpper();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            if (iconKey.isEmpty()) iconKey = "folder_filled";

            // 1. 实时就地刷新左侧收藏项
            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
            item->setIcon(newIcon);
            item->setData(finalColor, Qt::UserRole + 3);

            // 2. 联动刷新子菜单自身的头部图标与内部 50 个小图标颜色
            iconMenu->setIcon(UiHelper::getIcon("folder_filled", QColor(finalColor)));
            for (const auto& btnPair : iconButtons) {
                btnPair.first->setIcon(UiHelper::getIcon(btnPair.second, QColor(finalColor), 18));
            }

            if (m_favoriteView && m_favoriteView->viewport()) {
                m_favoriteView->viewport()->update();
            }
        });

        menu.addSeparator();
    }

    QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "取消收藏");
    connect(removeAct, &QAction::triggered, this, [this, path, index, &isItemRemoved]() {
        isItemRemoved = true;
        FavoriteDao::removeFavorite(path);
        m_favoriteModel->removeRow(index.row());
    });

    // 阻塞展示菜单（期间用户可随意连点试选 100 次）
    menu.exec(m_favoriteView->viewport()->mapToGlobal(pos));

    // 🚀【失焦退出机制】：菜单自然关闭后，仅在此处执行【唯一 1 次】物理数据库持久化！
    if (isFolder && !isItemRemoved && index.isValid()) {
        QStandardItem* item = m_favoriteModel->itemFromIndex(index);
        if (item) {
            QString finalPath = item->data(Qt::UserRole + 1).toString();
            QString finalIconKey = item->data(Qt::UserRole + 2).toString();
            QString finalColorHex = item->data(Qt::UserRole + 3).toString();
            FavoriteDao::updateFavorite(finalPath, finalIconKey, finalColorHex);
        }
    }
}

void FavoritePanel::onPathsDroppedToFavorite(const QStringList& paths, const QModelIndex& target) {
    Q_UNUSED(target);
    for (const QString& path : paths) {
        addFavoriteItem(path);
    }
}

void FavoritePanel::updateItemThumbnail(const QString& path, const QPixmap& pix) {
    if (!m_favoriteModel || pix.isNull()) return;

    QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(path));
    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QStandardItem* item = m_favoriteModel->item(i);
        if (!item) continue;
        QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(item->data(Qt::UserRole + 1).toString()));

        if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0) {
            item->setIcon(QIcon(pix));
            item->setData(true, Qt::UserRole + 5);
            if (m_favoriteView && m_favoriteView->viewport()) {
                m_favoriteView->viewport()->update();
            }
            break;
        }
    }
}

void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_isLoading = true;
    m_favoriteModel->clear();

    FavoriteDao::initTable();
    auto list = FavoriteDao::getAllFavorites();

    QStringList pathsToExtract;

    for (const auto& rec : list) {
        QString nativePath = QDir::toNativeSeparators(QDir::cleanPath(rec.path));
        QFileInfo fi(nativePath);
        if (!fi.exists()) continue;

        QColor itemColor = QColor(rec.colorHex);
        if (!itemColor.isValid()) itemColor = QColor("#FDB70A");

        QString iconKey = rec.iconKey.isEmpty() ? "folder_filled" : rec.iconKey;
        if (iconKey == "folder") iconKey = "folder_filled";

        bool isDir = fi.isDir();
        QIcon icon;

        if (isDir) {
            icon = UiHelper::getIcon(iconKey, itemColor, 18);
        } else {
            icon = ShellIconManager::getFileIcon(nativePath);
            QString ext = fi.suffix().toLower();
            if (UiHelper::isGraphicsFile(ext) || ext == "psd" || ext == "ai" || ext == "eps" || ext == "pdf" || ext == "svg") {
                pathsToExtract << nativePath;
            }
        }

        QStandardItem* item = new QStandardItem(icon, rec.name.isEmpty() ? fi.fileName() : rec.name);
        item->setData(nativePath, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);
        item->setData(isDir, Qt::UserRole + 4);
        item->setData(false, Qt::UserRole + 5);

        m_favoriteModel->appendRow(item);
    }

    m_isLoading = false;

    if (!pathsToExtract.isEmpty()) {
        QPointer<FavoritePanel> weakThis(this);
        for (const QString& path : pathsToExtract) {
            (void)QtConcurrent::run([weakThis, path]() {
                if (!weakThis) return;
                QImage img = DiskMediaExtractor::getCapsuleThumbnail(path, 128);
                if (!img.isNull()) {
                    QPixmap pix = QPixmap::fromImage(img);
                    QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, path, pix]() {
                        if (weakThis) {
                            weakThis->updateItemThumbnail(path, pix);
                        }
                    }, Qt::QueuedConnection);
                }
            });
        }
    }
}

void FavoritePanel::saveFavorites() {
    if (!m_favoriteModel || m_isLoading) return;

    QList<QPair<QString, int>> orders;
    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QStandardItem* item = m_favoriteModel->item(i);
        if (!item) continue;
        QString path = item->data(Qt::UserRole + 1).toString();
        if (!path.isEmpty()) {
            orders.append({ path, i + 1 });
        }
    }
    FavoriteDao::updateSortOrders(orders);
}

bool FavoritePanel::containsPath(const QString& path) const {
    if (path.isEmpty()) return false;
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    return FavoriteDao::containsPath(cleanPath);
}

void FavoritePanel::removeFavoriteItem(const QString& path) {
    if (path.isEmpty()) return;
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    FavoriteDao::removeFavorite(cleanPath);
    
    if (!m_favoriteModel) return;
    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QString existingPath = QDir::toNativeSeparators(QDir::cleanPath(m_favoriteModel->item(i)->data(Qt::UserRole + 1).toString()));
        if (QString::compare(existingPath, cleanPath, Qt::CaseInsensitive) == 0) {
            m_favoriteModel->removeRow(i);
            return;
        }
    }
}

void FavoritePanel::addFavoriteItem(const QString& path) {
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    if (cleanPath.isEmpty()) return;

    if (FavoriteDao::containsPath(cleanPath)) return;

    QFileInfo fi(cleanPath);
    if (!fi.exists()) return;

    bool isDir = fi.isDir();
    QString finalColorHex = "#FDB70A";

    if (isDir) {
        bool isDriveRoot = fi.isRoot() || cleanPath.endsWith(":\\") || cleanPath.endsWith(":/") || (cleanPath.length() == 2 && cleanPath.endsWith(':'));
        if (isDriveRoot) {
            std::wstring normWPath = MetadataManager::normalizePath(cleanPath.toStdWString());
            auto driveRec = DriveMetaDao::getDriveMeta(normWPath);
            QString driveColor = QString::fromStdWString(driveRec.color);
            if (!driveColor.isEmpty()) {
                finalColorHex = UiHelper::normalizeColorHex(driveColor);
            }
        } else {
            RuntimeMeta meta = MetadataManager::instance().getMeta(cleanPath.toStdWString());
            QString folderColor = QString::fromStdWString(meta.manualColor);
            if (!folderColor.isEmpty()) {
                finalColorHex = UiHelper::normalizeColorHex(folderColor);
            }
        }
    }

    FavoriteDao::addFavorite(cleanPath, "folder_filled", finalColorHex);

    QIcon icon;
    if (isDir) {
        icon = UiHelper::getIcon("folder_filled", QColor(finalColorHex), 18);
    } else {
        icon = ShellIconManager::getFileIcon(cleanPath);
    }

    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? cleanPath : fi.fileName());
    item->setData(cleanPath, Qt::UserRole + 1);
    item->setData("folder_filled", Qt::UserRole + 2);
    item->setData(finalColorHex, Qt::UserRole + 3);
    item->setData(isDir, Qt::UserRole + 4);
    item->setData(false, Qt::UserRole + 5);

    m_favoriteModel->appendRow(item);

    if (!isDir) {
        QString ext = fi.suffix().toLower();
        if (UiHelper::isGraphicsFile(ext) || ext == "psd" || ext == "ai" || ext == "eps" || ext == "pdf" || ext == "svg") {
            QPointer<FavoritePanel> weakThis(this);
            (void)QtConcurrent::run([weakThis, cleanPath]() {
                if (!weakThis) return;
                QImage img = DiskMediaExtractor::getCapsuleThumbnail(cleanPath, 128);
                if (!img.isNull()) {
                    QPixmap pix = QPixmap::fromImage(img);
                    QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, cleanPath, pix]() {
                        if (weakThis) weakThis->updateItemThumbnail(cleanPath, pix);
                    }, Qt::QueuedConnection);
                }
            });
        }
    }
}

} // namespace QuarkMeta