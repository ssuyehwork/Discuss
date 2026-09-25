#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "../util/DiskMediaExtractor.h"
#include "ColorPicker.h"
#include "../meta/FavoriteDao.h"
#include "../meta/FavoriteService.h"
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
    iconLabel->setPixmap(UiHelper::getIcon("star_filled", QColor("#888888"), 18).pixmap(18, 18));
    headerLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("收藏夹", header);
    titleLabel->setObjectName("FavoritePanelTitleLabel");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_favoriteView = new DropTreeView(this);
    m_favoriteView->setObjectName("FavoriteTreeView");
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

    connect(&FavoriteService::instance(), &FavoriteService::favoriteChanged, this, [this](const QString& path, bool isFav) {
        Q_UNUSED(path);
        Q_UNUSED(isFav);
        loadFavorites();
    });
}

void FavoritePanel::onFavoriteClicked(const QModelIndex& index) {
    bool isVirtual = index.data(Qt::UserRole + 7).toBool();
    if (isVirtual) {
        if (m_favoriteView) {
            if (m_favoriteView->isExpanded(index)) {
                m_favoriteView->collapse(index);
            } else {
                m_favoriteView->expand(index);
            }
        }
        return;
    }

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

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    if (!index.isValid()) {
        // 空白处右键：新建顶级虚拟分类
        QAction* newCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建虚拟分类");
        connect(newCatAct, &QAction::triggered, this, [this]() {
            addVirtualCategory("新建分类", 0);
        });
        menu.exec(m_favoriteView->viewport()->mapToGlobal(pos));
        return;
    }

    QString path = index.data(Qt::UserRole + 1).toString();
    QString curIconKey = index.data(Qt::UserRole + 2).toString();
    QString curColorHex = index.data(Qt::UserRole + 3).toString();
    int nodeId = index.data(Qt::UserRole + 6).toInt();
    bool isVirtual = index.data(Qt::UserRole + 7).toBool();

    if (curIconKey.isEmpty()) curIconKey = "folder_filled";
    if (curColorHex.isEmpty()) curColorHex = "#888888";

    QFileInfo fi(path);
    bool isFolder = isVirtual ? true : fi.isDir();
    bool isItemRemoved = false;

    if (isVirtual) {
        QAction* newSubCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建子分类");
        connect(newSubCatAct, &QAction::triggered, this, [this, nodeId]() {
            addVirtualCategory("新建子分类", nodeId);
        });
        menu.addSeparator();
    }

    // 缓存图标按钮指针，以便在换色时动态刷新子菜单图标色彩
    QList<QPair<QPushButton*, QString>> iconButtons;

    if (isFolder) {
        // 1. 颜色条组件
        QWidgetAction* colorPickerAction = new QWidgetAction(&menu);
        ColorStripPicker* colorPickerWidget = new ColorStripPicker(curColorHex, &menu);
        colorPickerAction->setDefaultWidget(colorPickerWidget);
        menu.addAction(colorPickerAction);

        // 2. 图标九宫格子菜单
        QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "切换图标");
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
                if (colorHex.isEmpty()) colorHex = "#888888";

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

            QString finalColor = hexColor.isEmpty() ? "#888888" : hexColor.toUpper();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            if (iconKey.isEmpty()) iconKey = "folder_filled";
            QString targetPath = item->data(Qt::UserRole + 1).toString();

            // 1. 实时就地刷新左侧收藏项
            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
            item->setIcon(newIcon);
            item->setData(finalColor, Qt::UserRole + 3);

            // 2. 联动刷新子菜单自身的头部图标与内部 50 个小图标颜色
            iconMenu->setIcon(UiHelper::getIcon("folder_filled", QColor(finalColor)));
            for (const auto& btnPair : iconButtons) {
                btnPair.first->setIcon(UiHelper::getIcon(btnPair.second, QColor(finalColor), 18));
            }

            // 3. 全局 Command 发起设色，触发 MetadataManager 与 CentralEventHub
            if (!targetPath.isEmpty() && !targetPath.startsWith("virtual_cat_")) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetColor;
                cmd.targetPaths = {targetPath};
                cmd.params["color"] = finalColor;
                CoreEngine::instance().executeCommand(cmd);
            }

            if (m_favoriteView && m_favoriteView->viewport()) {
                m_favoriteView->viewport()->update();
            }
        });

        menu.addSeparator();
    }

    QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), isVirtual ? "删除分类" : "取消收藏");
    connect(removeAct, &QAction::triggered, this, [this, path, nodeId, isVirtual, &isItemRemoved]() {
        isItemRemoved = true;
        if (isVirtual) {
            FavoriteService::instance().removeFavoriteById(nodeId);
        } else {
            removeFavoriteItem(path);
        }
    });

    // 阻塞展示菜单
    menu.exec(m_favoriteView->viewport()->mapToGlobal(pos));

    // 🚀【失焦退出机制】：菜单自然关闭后物理数据库持久化！
    if (isFolder && !isItemRemoved && index.isValid()) {
        QStandardItem* item = m_favoriteModel->itemFromIndex(index);
        if (item) {
            QString finalPath = item->data(Qt::UserRole + 1).toString();
            QString finalIconKey = item->data(Qt::UserRole + 2).toString();
            QString finalColorHex = item->data(Qt::UserRole + 3).toString();
            QString finalName = item->text();
            FavoriteDao::updateFavoriteNode(nodeId, finalName, finalIconKey, finalColorHex);
        }
    }
}

void FavoritePanel::onPathsDroppedToFavorite(const QStringList& paths, const QModelIndex& target) {
    int parentId = 0;
    if (target.isValid()) {
        bool isVirtualTarget = target.data(Qt::UserRole + 7).toBool();
        if (isVirtualTarget) {
            parentId = target.data(Qt::UserRole + 6).toInt();
        } else {
            parentId = target.data(Qt::UserRole + 8).toInt();
        }
    }
    for (const QString& path : paths) {
        addFavoriteItem(path, parentId);
    }
}

void FavoritePanel::updateItemThumbnail(const QString& path, const QPixmap& pix) {
    if (!m_favoriteModel || pix.isNull()) return;

    QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(path));

    std::function<bool(QStandardItem*)> findAndUpdate = [&](QStandardItem* parentItem) -> bool {
        int rowCount = parentItem ? parentItem->rowCount() : m_favoriteModel->rowCount();
        for (int i = 0; i < rowCount; ++i) {
            QStandardItem* item = parentItem ? parentItem->child(i) : m_favoriteModel->item(i);
            if (!item) continue;

            QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(item->data(Qt::UserRole + 1).toString()));
            if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0) {
                item->setIcon(QIcon(pix));
                item->setData(true, Qt::UserRole + 5);
                if (m_favoriteView && m_favoriteView->viewport()) {
                    m_favoriteView->viewport()->update();
                }
                return true;
            }

            if (findAndUpdate(item)) return true;
        }
        return false;
    };

    findAndUpdate(nullptr);
}

void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_isLoading = true;
    m_favoriteModel->clear();

    FavoriteDao::initTable();
    auto list = FavoriteDao::getAllFavorites();

    QStringList pathsToExtract;
    QMap<int, QStandardItem*> itemMap;

    // First Pass: Create QStandardItems
    for (const auto& rec : list) {
        bool isVirtual = (rec.nodeType == FavoriteNodeType::VirtualCategory);
        QString nativePath = isVirtual ? rec.path : QDir::toNativeSeparators(QDir::cleanPath(rec.path));

        if (!isVirtual) {
            QFileInfo fi(nativePath);
            if (!fi.exists()) continue;
        }

        QColor itemColor = QColor(rec.colorHex);
        if (!itemColor.isValid()) itemColor = QColor("#888888");

        QString iconKey = rec.iconKey.isEmpty() ? "folder_filled" : rec.iconKey;
        if (iconKey == "folder") iconKey = "folder_filled";

        bool isDir = isVirtual ? true : QFileInfo(nativePath).isDir();
        QIcon icon;

        if (isDir) {
            icon = UiHelper::getIcon(iconKey, itemColor, 18);
        } else {
            icon = ShellIconManager::getFileIcon(nativePath);
            QString ext = QFileInfo(nativePath).suffix().toLower();
            if (UiHelper::isGraphicsFile(ext) || ext == "psd" || ext == "ai" || ext == "eps" || ext == "pdf" || ext == "svg") {
                pathsToExtract << nativePath;
            }
        }

        QString displayName = rec.name;
        if (displayName.isEmpty() && !isVirtual) {
            displayName = QFileInfo(nativePath).fileName();
        }

        QStandardItem* item = new QStandardItem(icon, displayName);
        item->setData(nativePath, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);
        item->setData(isDir, Qt::UserRole + 4);
        item->setData(false, Qt::UserRole + 5);
        item->setData(rec.id, Qt::UserRole + 6);
        item->setData(isVirtual, Qt::UserRole + 7);
        item->setData(rec.parentId, Qt::UserRole + 8);

        itemMap.insert(rec.id, item);
    }

    // Second Pass: Build Tree Hierarchy
    for (const auto& rec : list) {
        if (!itemMap.contains(rec.id)) continue;
        QStandardItem* item = itemMap.value(rec.id);

        if (rec.parentId > 0 && itemMap.contains(rec.parentId)) {
            itemMap.value(rec.parentId)->appendRow(item);
        } else {
            m_favoriteModel->appendRow(item);
        }
    }

    if (m_favoriteView) {
        m_favoriteView->expandAll();
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

    QList<QPair<int, int>> orders;
    int orderCounter = 1;

    std::function<void(QStandardItem*, int)> traverseAndSave = [&](QStandardItem* parentItem, int parentId) {
        int rowCount = parentItem ? parentItem->rowCount() : m_favoriteModel->rowCount();
        for (int i = 0; i < rowCount; ++i) {
            QStandardItem* item = parentItem ? parentItem->child(i) : m_favoriteModel->item(i);
            if (!item) continue;

            int nodeId = item->data(Qt::UserRole + 6).toInt();
            if (nodeId > 0) {
                FavoriteDao::updateNodeParentAndOrder(nodeId, parentId, orderCounter++);
                traverseAndSave(item, nodeId);
            }
        }
    };

    traverseAndSave(nullptr, 0);
}

bool FavoritePanel::containsPath(const QString& path) const {
    return FavoriteService::instance().isFavorite(path);
}

void FavoritePanel::removeFavoriteItem(const QString& path) {
    FavoriteService::instance().removeFavorite(path);
}

void FavoritePanel::addFavoriteItem(const QString& path, int parentId) {
    FavoriteService::instance().addFavorite(path, parentId);
}

void FavoritePanel::addVirtualCategory(const QString& name, int parentId) {
    FavoriteService::instance().addVirtualCategory(name, parentId);
}

} // namespace QuarkMeta