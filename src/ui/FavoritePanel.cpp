#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "../util/DiskMediaExtractor.h"
#include "ColorPicker.h"
#include "ToolTipOverlay.h"
#include "PresetTagsDialog.h"
#include "dialogs/FramelessConfirmDialog.h"
#include "../meta/FavoriteDao.h"
#include "../meta/FavoriteService.h"
#include "../meta/MetadataManager.h"
#include "../meta/DriveMetaDao.h"
#include "controllers/ContextMenuFactory.h"
#include "ThumbnailPipelineService.h"
#include <QThreadPool>
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

QSize FavoriteItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    sz.setHeight(28);
    return sz;
}

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

    // 🚀【行内编辑即时落库】：用户重命名编辑完成后，即时更新 SQLite
    connect(m_favoriteModel, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        int nodeId = item->data(Qt::UserRole + 6).toInt();
        if (nodeId > 0) {
            QString name = item->text();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            QString colorHex = item->data(Qt::UserRole + 3).toString();
            FavoriteDao::updateFavoriteNode(nodeId, name, iconKey, colorHex);
        }
    });

    connect(&FavoriteService::instance(), &FavoriteService::favoriteChanged, this, [this](const QString& path, bool isFav) {
        Q_UNUSED(path);
        Q_UNUSED(isFav);
        loadFavorites();
    });
    connect(&FavoriteService::instance(), &FavoriteService::favoritesReloaded, this, [this]() {
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
    if (!fi.exists()) {
        int recId = index.data(Qt::UserRole + 6).toInt();
        QString msg = QString("无法找到路径：%1\n该文件或文件夹可能已被移动、重命名或删除。\n\n是否将其从收藏夹中移除？").arg(path);
        FramelessConfirmDialog dlg("提示", msg, FramelessConfirmDialog::OkCancel, "alert_warning", QColor("#e74c3c"), this);
        if (dlg.exec() == QDialog::Accepted) {
            if (recId > 0) {
                FavoriteService::instance().removeFavoriteById(recId);
            } else {
                FavoriteService::instance().removeFavorite(path);
            }
            loadFavorites();
        }
        return;
    }

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
        auto* sortMenu = menu.addMenu(UiHelper::getIcon("list_ul", QColor("#AAAAAA")), "排列");
        UiHelper::applyMenuStyle(sortMenu);
        QAction* sortAsc = sortMenu->addAction("按名称 (A→Z)");
        connect(sortAsc, &QAction::triggered, this, [this]() { sortItemsByName(true); });
        QAction* sortDesc = sortMenu->addAction("按名称 (Z→A)");
        connect(sortDesc, &QAction::triggered, this, [this]() { sortItemsByName(false); });

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

    // 缓存图标按钮指针，以便在换色时动态刷新子菜单图标色彩
    QList<QPair<QPushButton*, QString>> iconButtons;

    if (isFolder) {
        ContextMenuFactory::buildIconPickerMenu(&menu, curIconKey, curColorHex,
            [this, index](const QString& iconKey) {
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
            },
            [this, index](const QString& hexColor) {
                QStandardItem* item = m_favoriteModel->itemFromIndex(index);
                if (!item) return;

                QString finalColor = hexColor.isEmpty() ? "#888888" : hexColor.toUpper();
                QString iconKey = item->data(Qt::UserRole + 2).toString();
                if (iconKey.isEmpty()) iconKey = "folder_filled";
                QString targetPath = item->data(Qt::UserRole + 1).toString();

                QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
                item->setIcon(newIcon);
                item->setData(finalColor, Qt::UserRole + 3);

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
            }
        );
    }

    // 3. 重命名 (直接唤起行内编辑框)
    QAction* renameAct = menu.addAction(UiHelper::getIcon("edit", QColor("#EEEEEE")), "重命名");
    connect(renameAct, &QAction::triggered, this, [this, index]() {
        if (m_favoriteView && index.isValid()) {
            m_favoriteView->edit(index);
        }
    });

    // 4. 删除 / 取消收藏
    if (isVirtual) {
        QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "删除");
        connect(removeAct, &QAction::triggered, this, [this, nodeId, &isItemRemoved]() {
            isItemRemoved = true;
            FavoriteService::instance().removeFavoriteById(nodeId);
        });
    } else {
        QAction* favAct = FavoriteService::instance().buildFavoriteAction(&menu, path, this);
        if (favAct) {
            connect(favAct, &QAction::triggered, this, [&isItemRemoved]() {
                isItemRemoved = true;
            });
        }
    }

    menu.addSeparator();

    // 5. 排列子菜单
    auto* sortMenu = menu.addMenu(UiHelper::getIcon("list_ul", QColor("#AAAAAA")), "排列");
    UiHelper::applyMenuStyle(sortMenu);
    QAction* sortAsc = sortMenu->addAction("按名称 (A→Z)");
    connect(sortAsc, &QAction::triggered, this, [this]() { sortItemsByName(true); });
    QAction* sortDesc = sortMenu->addAction("按名称 (Z→A)");
    connect(sortDesc, &QAction::triggered, this, [this]() { sortItemsByName(false); });

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

    int addedCount = 0;
    int duplicateCount = 0;
    for (const QString& path : paths) {
        if (FavoriteService::instance().isFavorite(path)) {
            duplicateCount++;
        } else {
            addFavoriteItem(path, parentId);
            addedCount++;
        }
    }

    if (addedCount > 0) {
        saveFavorites();
        if (duplicateCount == 0) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, QColor("#2ecc71"));
        } else {
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已添加 %1 个项目 (其余 %2 个已在收藏夹中)").arg(addedCount).arg(duplicateCount), 2000, QColor("#3498db"));
        }
    } else if (duplicateCount > 0) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "该文件夹已在收藏夹中，请勿重复添加", 2000, QColor("#e81123"));
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

    // 🚀 【时序对齐】：新建节点完成后，找到对应节点并唤起行内重命名编辑框
    if (m_pendingEditNodeId > 0 && m_favoriteView) {
        int targetNodeId = m_pendingEditNodeId;
        m_pendingEditNodeId = 0;

        std::function<QModelIndex(QStandardItem*)> findIndexByNodeId = [&](QStandardItem* parentItem) -> QModelIndex {
            int rowCount = parentItem ? parentItem->rowCount() : m_favoriteModel->rowCount();
            for (int i = 0; i < rowCount; ++i) {
                QStandardItem* item = parentItem ? parentItem->child(i) : m_favoriteModel->item(i);
                if (!item) continue;
                if (item->data(Qt::UserRole + 6).toInt() == targetNodeId) {
                    return item->index();
                }
                QModelIndex childIdx = findIndexByNodeId(item);
                if (childIdx.isValid()) return childIdx;
            }
            return QModelIndex();
        };

        QModelIndex targetIndex = findIndexByNodeId(nullptr);
        if (targetIndex.isValid()) {
            m_favoriteView->setCurrentIndex(targetIndex);
            m_favoriteView->edit(targetIndex);
        }
    }

    if (!pathsToExtract.isEmpty()) {
        QPointer<FavoritePanel> weakThis(this);
        for (const QString& path : pathsToExtract) {
            (void)QtConcurrent::run(ThumbnailPipelineService::instance().decodePool(), [weakThis, path]() {
                if (!weakThis) return;
                // 磁盘缓存只有 230 一档；收藏面板要 128，从 230 缩小，不单独写盘
                QImage img = DiskMediaExtractor::getCapsuleThumbnail(path, DiskMediaExtractor::kThumbSize);
                if (!img.isNull()) {
                    if (img.width() > 128 || img.height() > 128) {
                        img = img.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    }
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

void FavoritePanel::createAndEditCategory(int parentId) {
    int newId = FavoriteService::instance().addVirtualCategory("新建文件夹", parentId);
    if (newId > 0) {
        m_pendingEditNodeId = newId;
    }
}

void FavoritePanel::sortItemsByName(bool ascending) {
    if (!m_favoriteModel) return;

    std::function<void(QStandardItem*)> sortChildren = [&](QStandardItem* parentItem) {
        int rowCount = parentItem ? parentItem->rowCount() : m_favoriteModel->rowCount();
        if (rowCount <= 1) return;

        QList<QStandardItem*> childItems;
        for (int i = rowCount - 1; i >= 0; --i) {
            QStandardItem* item = parentItem ? parentItem->takeRow(i).first() : m_favoriteModel->takeRow(i).first();
            if (item) childItems.prepend(item);
        }

        std::sort(childItems.begin(), childItems.end(), [ascending](QStandardItem* a, QStandardItem* b) {
            return ascending ? (a->text().localeAwareCompare(b->text()) < 0)
                              : (a->text().localeAwareCompare(b->text()) > 0);
        });

        for (QStandardItem* item : childItems) {
            if (parentItem) {
                parentItem->appendRow(item);
            } else {
                m_favoriteModel->appendRow(item);
            }
            sortChildren(item);
        }
    };

    sortChildren(nullptr);
    saveFavorites();
}

} // namespace QuarkMeta
