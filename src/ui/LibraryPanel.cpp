#include "LibraryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "PresetTagsDialog.h"
#include "ColorPicker.h"
#include "ShellIconManager.h"
#include "../meta/LibraryDao.h"
#include "../meta/LibraryService.h"
#include "../core/CoreEngine.h"
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QWidgetAction>
#include <QGridLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <QDir>
#include <QCursor>
#include <QPainter>

namespace QuarkMeta {

QSize LibraryItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    sz.setHeight(28);
    return sz;
}

void LibraryItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor("#37373D"));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }

    int leftMargin = 10;
    int iconSize = 18;
    int spacing = 8;

    QRect iconRect(opt.rect.left() + leftMargin, opt.rect.top() + (opt.rect.height() - iconSize) / 2, iconSize, iconSize);
    QRect textRect(iconRect.right() + spacing, opt.rect.top(), opt.rect.width() - leftMargin - iconSize - spacing, opt.rect.height());

    QVariant decoData = index.data(Qt::DecorationRole);
    if (decoData.canConvert<QIcon>()) {
        QIcon icon = decoData.value<QIcon>();
        if (!icon.isNull()) {
            icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }
    }

    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
    painter->setFont(opt.font);

    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width() - 6);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    painter->restore();
}

LibraryPanel::LibraryPanel(QWidget* parent) : QFrame(parent) {
    setObjectName("LibraryContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    loadLibrary();
}

void LibraryPanel::initUi() {
    m_treeView = new DropTreeView(this);
    m_treeView->setObjectName("LibraryTreeView");
    m_treeView->setHeaderHidden(true);
    if (m_treeView->header()) {
        m_treeView->header()->setStretchLastSection(true);
        m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    m_treeView->setIndentation(15);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setDragEnabled(true);
    m_treeView->setAcceptDrops(true);
    m_treeView->setDropIndicatorShown(true);
    m_treeView->setDefaultDropAction(Qt::MoveAction);
    m_treeView->setDragDropMode(QAbstractItemView::DragDrop);
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_model = new QStandardItemModel(this);
    m_treeView->setModel(m_model);
    m_treeView->setItemDelegate(new LibraryItemDelegate(this));

    m_mainLayout->addWidget(m_treeView, 1);

    connect(m_treeView, &QTreeView::clicked, this, &LibraryPanel::onCategoryClicked);
    connect(m_treeView, &QWidget::customContextMenuRequested, this, &LibraryPanel::onCategoryContextMenu);
    connect(m_treeView, &DropTreeView::pathsDropped, this, &LibraryPanel::onPathsDroppedToCategory);

    connect(&LibraryService::instance(), &LibraryService::libraryChanged, this, [this]() {
        loadLibrary();
    });

    connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        bool isPathItem = item->data(Qt::UserRole + 4).toBool();
        if (!isPathItem) {
            int nodeId = item->data(Qt::UserRole + 1).toInt();
            if (nodeId > 0) {
                QString name = item->text();
                QString iconKey = item->data(Qt::UserRole + 2).toString();
                QString colorHex = item->data(Qt::UserRole + 3).toString();
                LibraryDao::updateCategoryNode(nodeId, name, iconKey, colorHex);
            }
        }
    });
}

void LibraryPanel::onCategoryClicked(const QModelIndex& index) {
    if (!index.isValid()) return;

    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId > 0) {
        QStringList paths = LibraryService::instance().getCategoryPaths(nodeId);
        emit categoryPathsSelected(paths);
    }
}

void LibraryPanel::onCategoryContextMenu(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    if (!index.isValid()) {
        QAction* newCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建库分类");
        connect(newCatAct, &QAction::triggered, this, [this]() {
            createAndEditCategory(0);
        });
        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
        return;
    }


    int nodeId = index.data(Qt::UserRole + 1).toInt();
    QString curIconKey = index.data(Qt::UserRole + 2).toString();
    QString curColorHex = index.data(Qt::UserRole + 3).toString();

    if (curIconKey.isEmpty()) curIconKey = "folder_filled";
    if (curColorHex.isEmpty()) curColorHex = "#888888";

    QAction* newSubCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建子分类");
    connect(newSubCatAct, &QAction::triggered, this, [this, nodeId]() {
        createAndEditCategory(nodeId);
    });

    QAction* presetTagAct = menu.addAction(UiHelper::getIcon("tag_filled", QColor("#9B59B6")), "设置预设标签");
    connect(presetTagAct, &QAction::triggered, this, [this, nodeId]() {
        PresetTagsDialog dlg(nodeId, this, true);
        dlg.exec();
    });

    menu.addSeparator();

    // 1. 颜色条组件
    QWidgetAction* colorPickerAction = new QWidgetAction(&menu);
    ColorStripPicker* colorPickerWidget = new ColorStripPicker(curColorHex, &menu);
    colorPickerAction->setDefaultWidget(colorPickerWidget);
    menu.addAction(colorPickerAction);

    // 2. 切换图标
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
        {"图书文档", "book"}, {"配置管理", "settings_filled"}, {"网络球体", "globe_filled"},
        {"主页主路径", "home_filled"}, {"标签标记", "tag_filled"}, {"书签指示", "bookmark_filled"},
        {"音频音乐", "music_filled"}, {"视频影视", "video_filled"}, {"摄影相机", "camera_filled"},
        {"盾牌防护", "shield_filled"}, {"物理硬盘", "hard_drive"}, {"云端同步", "cloud_filled"},
        {"闪电极速", "zap_filled"}, {"魔法火花", "sparkles_filled"}, {"旗帜标记", "flag_filled"}
    };

    QColor catColor = QColor(curColorHex);
    int row = 0, col = 0;
    QList<QPair<QPushButton*, QString>> iconButtons;
    for (const auto& pair : builtInIcons) {
        QString iconKey = pair.second;
        QPushButton* btn = new QPushButton(pickerWidget);
        btn->setFixedSize(28, 28);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setIcon(UiHelper::getIcon(iconKey, catColor, 18));
        btn->setIconSize(QSize(18, 18));
        pickerLayout->addWidget(btn, row, col);

        iconButtons.append({btn, iconKey});

        connect(btn, &QPushButton::clicked, this, [this, index, iconKey]() {
            QStandardItem* item = m_model->itemFromIndex(index);
            if (!item) return;

            QString colorHex = item->data(Qt::UserRole + 3).toString();
            if (colorHex.isEmpty()) colorHex = "#888888";

            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
            item->setIcon(newIcon);
            item->setData(iconKey, Qt::UserRole + 2);

            int nodeId = item->data(Qt::UserRole + 1).toInt();
            LibraryDao::updateCategoryNode(nodeId, item->text(), iconKey, colorHex);
        });

        col++;
        if (col >= 4) { col = 0; row++; }
    }

    pickerWidget->setLayout(pickerLayout);
    pickerAction->setDefaultWidget(pickerWidget);
    iconMenu->addAction(pickerAction);

    connect(colorPickerWidget, &ColorStripPicker::colorSelected, this, [this, index, iconMenu, iconButtons](const QString& hexColor) {
        QStandardItem* item = m_model->itemFromIndex(index);
        if (!item) return;

        QString finalColor = hexColor.isEmpty() ? "#888888" : hexColor.toUpper();
        QString iconKey = item->data(Qt::UserRole + 2).toString();
        if (iconKey.isEmpty()) iconKey = "folder_filled";

        QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
        item->setIcon(newIcon);
        item->setData(finalColor, Qt::UserRole + 3);

        int nodeId = item->data(Qt::UserRole + 1).toInt();
        LibraryDao::updateCategoryNode(nodeId, item->text(), iconKey, finalColor);
    });

    menu.addSeparator();

    QAction* renameAct = menu.addAction(UiHelper::getIcon("edit", QColor("#EEEEEE")), "重命名");
    connect(renameAct, &QAction::triggered, this, [this, index]() {
        if (m_treeView) m_treeView->edit(index);
    });

    QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "删除分类");
    connect(removeAct, &QAction::triggered, this, [this, nodeId]() {
        LibraryService::instance().removeCategory(nodeId);
    });

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void LibraryPanel::onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target) {
    if (!target.isValid() || paths.isEmpty()) return;
    int nodeId = target.data(Qt::UserRole + 1).toInt();
    bool isPathItem = target.data(Qt::UserRole + 4).toBool();
    if (isPathItem) {
        nodeId = target.data(Qt::UserRole + 6).toInt();
    }

    if (nodeId > 0) {
        LibraryService::instance().addPathsToCategory(nodeId, paths);

        // 自动将预设标签批量加至入库文件
        auto categories = LibraryDao::getAllCategories();
        QStringList presetTags;
        for (const auto& cat : categories) {
            if (cat.id == nodeId) {
                presetTags = cat.presetTags;
                break;
            }
        }

        if (!presetTags.isEmpty()) {
            for (const QString& tag : presetTags) {
                AppCommand cmd;
                cmd.type = AppCommandType::AddTag;
                cmd.targetPaths = paths;
                cmd.params["tag"] = tag;
                CoreEngine::instance().executeCommand(cmd);
            }
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已将 %1 个项目关联至当前分类，并自动应用了 %2 个预设标签").arg(paths.size()).arg(presetTags.size()), 2000, QColor("#2ecc71"));
        } else {
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已关联 %1 个路径到当前分类").arg(paths.size()), 1500, QColor("#2ecc71"));
        }
    }
}

void LibraryPanel::loadLibrary() {
    if (!m_model) return;
    m_isLoading = true;
    m_model->clear();

    LibraryDao::initTable();
    auto list = LibraryDao::getAllCategories();

    QMap<int, QStandardItem*> itemMap;
    for (const auto& rec : list) {
        QIcon icon = UiHelper::getIcon(rec.iconKey, QColor(rec.colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, rec.name);
        item->setData(rec.id, Qt::UserRole + 1);
        item->setData(rec.iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);
        item->setData(false, Qt::UserRole + 4); // false = Category node


        itemMap.insert(rec.id, item);
    }

    for (const auto& rec : list) {
        if (!itemMap.contains(rec.id)) continue;
        QStandardItem* item = itemMap.value(rec.id);

        if (rec.parentId > 0 && itemMap.contains(rec.parentId)) {
            itemMap.value(rec.parentId)->appendRow(item);
        } else {
            m_model->appendRow(item);
        }
    }

    if (m_treeView) m_treeView->expandAll();
    m_isLoading = false;

    if (m_pendingEditNodeId > 0 && m_treeView) {
        int targetNodeId = m_pendingEditNodeId;
        m_pendingEditNodeId = 0;

        for (int i = 0; i < m_model->rowCount(); ++i) {
            QStandardItem* item = m_model->item(i);
            if (item && item->data(Qt::UserRole + 1).toInt() == targetNodeId) {
                m_treeView->setCurrentIndex(item->index());
                m_treeView->edit(item->index());
                break;
            }
        }
    }
}

void LibraryPanel::createAndEditCategory(int parentId) {
    int newId = LibraryService::instance().createCategory("新建分类", parentId);
    if (newId > 0) {
        m_pendingEditNodeId = newId;
        loadLibrary();
    }
}

} // namespace QuarkMeta
