#include "LibraryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "PresetTagsDialog.h"
#include "ColorPicker.h"
#include "ShellIconManager.h"
#include "ViewDragDropHelper.h"
#include "../meta/LibraryDao.h"
#include "../meta/LibraryService.h"
#include "../meta/QuarkMetaJsonStore.h"
#include "../core/CoreEngine.h"
#include "../core/ModelContract.h"
#include "controllers/ContextMenuFactory.h"
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

    bool isDropTarget = index.data(IsDropTargetRole).toBool() ||
                       ViewDragDropHelper::isDropTarget(qobject_cast<const QAbstractItemView*>(option.widget), index);

    if (isDropTarget) {
        QColor dropBg("#3498db");
        dropBg.setAlphaF(0.35f);
        painter->fillRect(opt.rect, dropBg);
    } else if (opt.state & QStyle::State_Selected) {
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
    int count = index.data(Qt::UserRole + 9).toInt();
    if (count > 0) {
        text += QString(" (%1)").arg(count);
    }

    painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
    painter->setFont(opt.font);

    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width() - 6);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    painter->restore();
}

void LibraryItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(index);
    if (!editor) return;

    QRect textRect = option.rect;
    textRect.setLeft(option.rect.left() + 33);
    textRect.setRight(option.rect.right() - 3);

    editor->setGeometry(textRect);
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
    m_treeView->setIndentation(0);
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
        QTimer::singleShot(0, this, [this]() {
            loadLibrary();
        });
    });

    connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        int nodeId = item->data(Qt::UserRole + 1).toInt();
        if (nodeId > 0) {
            QString name = item->text();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            QString colorHex = item->data(Qt::UserRole + 3).toString();
            LibraryDao::updateCategoryNode(nodeId, name, iconKey, colorHex);
            QTimer::singleShot(0, this, [this]() {
                loadLibrary();
            });
        }
    });
}

void LibraryPanel::onCategoryClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId != 0) {
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
    if (nodeId < 0) {
        // 系统分类禁止弹出修改菜单
        return;
    }

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

    ContextMenuFactory::buildIconPickerMenu(&menu, curIconKey, curColorHex,
        [this, index](const QString& iconKey) {
            QStandardItem* item = m_model->itemFromIndex(index);
            if (!item) return;

            QString colorHex = item->data(Qt::UserRole + 3).toString();
            if (colorHex.isEmpty()) colorHex = "#888888";

            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
            item->setIcon(newIcon);
            item->setData(iconKey, Qt::UserRole + 2);

            int nodeId = item->data(Qt::UserRole + 1).toInt();
            LibraryDao::updateCategoryNode(nodeId, item->text(), iconKey, colorHex);
        },
        [this, index](const QString& hexColor) {
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
        }
    );

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
    if (paths.isEmpty()) return;

    // 拖拽到空白处时默认为“未分类” (-2)，拖到具体分类上则为对应的分类 ID
    int nodeId = -2;
    if (target.isValid()) {
        nodeId = target.data(Qt::UserRole + 1).toInt();
    }

    if (nodeId > 0 || nodeId == -2) {
        LibraryService::instance().addPathsToCategory(nodeId, paths);

        // 🚀【索引建库】：无损读取每个项目的 .QuarkMeta.json 并写入中心 library_item_index
        for (const QString& path : paths) {
            QFileInfo fi(path);
            if (fi.isDir()) {
                QDir dir(path);
                QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
                for (const auto& entry : entries) {
                    ItemMeta meta;
                    std::wstring wPath = entry.absoluteFilePath().toStdWString();
                    if (QuarkMetaJsonStore::instance().readItemMeta(wPath, meta)) {
                        LibraryService::instance().indexItem(nodeId, entry.absoluteFilePath(), meta);
                    }
                }
            } else if (fi.isFile()) {
                ItemMeta meta;
                std::wstring wPath = path.toStdWString();
                if (QuarkMetaJsonStore::instance().readItemMeta(wPath, meta)) {
                    LibraryService::instance().indexItem(nodeId, path, meta);
                }
            }
        }

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

    // 1. 注入 3 个固定系统分类 (带动态计数)
    auto addSystemItem = [this](const QString& name, const QString& iconKey, const QString& colorHex, int sysId) {
        int count = LibraryDao::getCategoryPaths(sysId).size();
        QIcon icon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, name);
        item->setData(sysId, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(colorHex, Qt::UserRole + 3);
        item->setData(count, Qt::UserRole + 9);
        item->setEditable(false);
        m_model->appendRow(item);
    };

    addSystemItem("全部数据", "all_data", "#3498db", -1);
    addSystemItem("未分类", "uncategorized", "#95a5a6", -2);
    addSystemItem("未标签", "untagged", "#7f8c8d", -3);

    // 2. 加载用户自定义分类 (带动态计数)
    auto list = LibraryDao::getAllCategories();

    QMap<int, QStandardItem*> itemMap;
    for (const auto& rec : list) {
        int count = LibraryDao::getCategoryPaths(rec.id).size();
        QIcon icon = UiHelper::getIcon(rec.iconKey, QColor(rec.colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, rec.name);
        item->setData(rec.id, Qt::UserRole + 1);
        item->setData(rec.iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);
        item->setData(count, Qt::UserRole + 9);

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
