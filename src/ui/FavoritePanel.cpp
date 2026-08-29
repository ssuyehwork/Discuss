#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "../meta/FavoriteDao.h"
#include <QPainter>
#include "../core/AppConfig.h"
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QFileInfo>
#include <QDir>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

namespace QuarkMeta {

void FavoriteItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Draw selection/hover background and text
    painter->save();
    
    // Custom background fill if selected or hovered
    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor("#37373D"));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }

    // Calculate layout geometries for icon and text
    int leftMargin = 10;
    int iconSize = 18;
    int spacing = 6;

    QRect iconRect(opt.rect.left() + leftMargin, opt.rect.top() + (opt.rect.height() - iconSize) / 2, iconSize, iconSize);
    QRect textRect(iconRect.right() + spacing, opt.rect.top(), opt.rect.width() - leftMargin - iconSize - spacing, opt.rect.height());

    // Paint Icon in QIcon::Normal mode (prevents Qt selected state darkening mask)
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) {
        icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
    }

    // Paint Text
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
    painter->setFont(opt.font);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

    painter->restore();
}

FavoritePanel::FavoritePanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("ListContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
    setStyleSheet("FavoritePanel { background-color: #1E1E1E; color: #EEEEEE; }");

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
    // 固定顶栏 Header
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);
    header->setStyleSheet(
        "QWidget#ContainerHeader {"
        "  background-color: #252526;"
        "  border-bottom: 1px solid #333333;"
        "}"
    );
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(15, 0, 5, 0);
    headerLayout->setSpacing(5);

    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("star_filled", QColor("#FDB70A"), 18).pixmap(18, 18));
    headerLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("收藏夹", header);
    titleLabel->setStyleSheet("color: #FDB70A; font-size: 13px; font-weight: bold; background: transparent; border: none;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    // 收藏夹树视图
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

    // 树视图 QSS 样式
    QString treeStyle = QString(
        "QTreeView { background-color: transparent; border: none; font-size: 12px; outline: none; padding-left: 10px; }"
        "QTreeView::item { height: 28px; padding-left: 0px; color: #EEEEEE; }"
        "QTreeView::item:hover { background-color: #2A2D2E; }"
        "QTreeView::item:selected { background-color: #37373D; color: #FFFFFF; }"
    );
    m_favoriteView->setStyleSheet(treeStyle);

    m_mainLayout->addWidget(m_favoriteView, 1);

    // 信号绑定
    connect(m_favoriteView, &QTreeView::clicked, this, &FavoritePanel::onFavoriteClicked);
    connect(m_favoriteView, &QWidget::customContextMenuRequested, this, &FavoritePanel::onFavoriteContextMenu);
    connect(m_favoriteView, &DropTreeView::pathsDropped, this, &FavoritePanel::onPathsDroppedToFavorite);

    // 模型数据变动监听
    auto updateFavAndSave = [this](){ saveFavorites(); };
    connect(m_favoriteModel, &QStandardItemModel::rowsMoved, this, updateFavAndSave);
    connect(m_favoriteModel, &QStandardItemModel::rowsInserted, this, updateFavAndSave);
    connect(m_favoriteModel, &QStandardItemModel::rowsRemoved, this, updateFavAndSave);
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
    if (curIconKey.isEmpty()) curIconKey = "folder";
    if (curColorHex.isEmpty()) curColorHex = "#FDB70A";

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder", QColor("#EEEEEE")), "切换图标");
    static const QPair<QString, QString> iconOptions[] = {
        { "folder", "标准文件夹" },
        { "star", "星号" },
        { "heart", "红心" },
        { "bookmark", "书签" },
        { "tag", "标签" }
    };
    for (const auto& opt : iconOptions) {
        QAction* act = iconMenu->addAction(UiHelper::getIcon(opt.first, QColor(curColorHex)), opt.second);
        connect(act, &QAction::triggered, this, [this, path, opt, curColorHex, index]() {
            FavoriteDao::updateFavorite(path, opt.first, curColorHex);
            QIcon newIcon = UiHelper::getIcon(opt.first, QColor(curColorHex), 18);
            m_favoriteModel->itemFromIndex(index)->setIcon(newIcon);
            m_favoriteModel->itemFromIndex(index)->setData(opt.first, Qt::UserRole + 2);
        });
    }

    QMenu* colorMenu = menu.addMenu(UiHelper::getIcon("circle_filled", QColor(curColorHex)), "切换色标");
    static const QPair<QString, QString> colorOptions[] = {
        { "#FDB70A", "金色" },
        { "#E24B4A", "红色" },
        { "#EF9F27", "橙色" },
        { "#639922", "绿色" },
        { "#1D9E75", "青色" },
        { "#378ADD", "蓝色" },
        { "#7F77DD", "紫色" }
    };
    for (const auto& opt : colorOptions) {
        QAction* act = colorMenu->addAction(UiHelper::getIcon("circle_filled", QColor(opt.first)), opt.second);
        connect(act, &QAction::triggered, this, [this, path, curIconKey, opt, index]() {
            FavoriteDao::updateFavorite(path, curIconKey, opt.first);
            QIcon newIcon = UiHelper::getIcon(curIconKey, QColor(opt.first), 18);
            m_favoriteModel->itemFromIndex(index)->setIcon(newIcon);
            m_favoriteModel->itemFromIndex(index)->setData(opt.first, Qt::UserRole + 3);
        });
    }

    menu.addSeparator();

    QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "取消收藏");
    connect(removeAct, &QAction::triggered, this, [this, path, index]() {
        FavoriteDao::removeFavorite(path);
        m_favoriteModel->removeRow(index.row());
    });

    menu.exec(m_favoriteView->viewport()->mapToGlobal(pos));
}

void FavoritePanel::onPathsDroppedToFavorite(const QStringList& paths, const QModelIndex& target) {
    Q_UNUSED(target);
    for (const QString& path : paths) {
        addFavoriteItem(path);
    }
    saveFavorites();
}

void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_favoriteModel->clear();

    FavoriteDao::initTable();
    auto list = FavoriteDao::getAllFavorites();

    for (const auto& rec : list) {
        QFileInfo fi(rec.path);
        if (!fi.exists()) continue;

        QColor itemColor = QColor(rec.colorHex);
        if (!itemColor.isValid()) itemColor = QColor("#FDB70A");

        QIcon icon = UiHelper::getIcon(rec.iconKey.isEmpty() ? "folder" : rec.iconKey, itemColor, 18);
        QStandardItem* item = new QStandardItem(icon, rec.name.isEmpty() ? fi.fileName() : rec.name);
        item->setData(rec.path, Qt::UserRole + 1);
        item->setData(rec.iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);

        m_favoriteModel->appendRow(item);
    }
}

void FavoritePanel::saveFavorites() {
    if (!m_favoriteModel) return;

    QList<QPair<QString, int>> orders;
    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QStandardItem* item = m_favoriteModel->item(i);
        QString path = item->data(Qt::UserRole + 1).toString();
        orders.append({ path, i + 1 });
    }
    FavoriteDao::updateSortOrders(orders);
}

bool FavoritePanel::containsPath(const QString& path) const {
    if (path.isEmpty()) return false;
    return FavoriteDao::containsPath(path);
}

void FavoritePanel::removeFavoriteItem(const QString& path) {
    if (path.isEmpty()) return;
    FavoriteDao::removeFavorite(path);

    if (!m_favoriteModel) return;
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
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

    FavoriteDao::addFavorite(cleanPath, "folder", "#FDB70A");

    QIcon icon = UiHelper::getIcon("folder", QColor("#FDB70A"), 18);
    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? cleanPath : fi.fileName());
    item->setData(cleanPath, Qt::UserRole + 1);
    item->setData("folder", Qt::UserRole + 2);
    item->setData("#FDB70A", Qt::UserRole + 3);

    m_favoriteModel->appendRow(item);
}

} // namespace QuarkMeta
