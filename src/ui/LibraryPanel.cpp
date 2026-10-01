#include "LibraryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "PresetTagsDialog.h"
#include "../meta/LibraryDao.h"
#include "../meta/LibraryService.h"
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QHeaderView>
#include <QCursor>

namespace QuarkMeta {

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

    m_mainLayout->addWidget(m_treeView, 1);

    connect(m_treeView, &QTreeView::clicked, this, &LibraryPanel::onCategoryClicked);
    connect(m_treeView, &QWidget::customContextMenuRequested, this, &LibraryPanel::onCategoryContextMenu);
    connect(m_treeView, &DropTreeView::pathsDropped, this, &LibraryPanel::onPathsDroppedToCategory);

    connect(&LibraryService::instance(), &LibraryService::libraryChanged, this, [this]() {
        loadLibrary();
    });

    connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        int nodeId = item->data(Qt::UserRole + 1).toInt();
        if (nodeId > 0) {
            QString name = item->text();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            QString colorHex = item->data(Qt::UserRole + 3).toString();
            LibraryDao::updateCategoryNode(nodeId, name, iconKey, colorHex);
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

    int parentId = 0;
    if (index.isValid()) {
        parentId = index.data(Qt::UserRole + 1).toInt();
    }

    QAction* newCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建库分类");
    connect(newCatAct, &QAction::triggered, this, [this, parentId]() {
        createAndEditCategory(parentId);
    });

    if (index.isValid()) {
        int nodeId = index.data(Qt::UserRole + 1).toInt();

        QAction* newSubCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建子分类");
        connect(newSubCatAct, &QAction::triggered, this, [this, nodeId]() {
            createAndEditCategory(nodeId);
        });

        QAction* presetTagAct = menu.addAction(UiHelper::getIcon("tag_filled", QColor("#9B59B6")), "设置预设标签");
        connect(presetTagAct, &QAction::triggered, this, [this, nodeId]() {
            PresetTagsDialog dlg(nodeId, this);
            dlg.exec();
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
    }

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void LibraryPanel::onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target) {
    if (!target.isValid()) return;
    int nodeId = target.data(Qt::UserRole + 1).toInt();
    if (nodeId > 0 && !paths.isEmpty()) {
        LibraryService::instance().addPathsToCategory(nodeId, paths);
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已关联 %1 个路径到当前分类").arg(paths.size()), 1500, QColor("#2ecc71"));
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
}

void LibraryPanel::createAndEditCategory(int parentId) {
    int newId = LibraryService::instance().createCategory("新建分类", parentId);
    if (newId > 0) {
        loadLibrary();
    }
}

} // namespace QuarkMeta
