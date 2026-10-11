# LibraryPanel 自定义分类分组与底栏搜索过滤实施方案

## 1. Overview（概述与解决的问题）

在 `LibraryPanel`（“库”标签页）中提升用户自定义分类的管理体验与检索效率：
1. **新增 `"文件夹(N)"` 动态折叠分组 Header**：将所有自定义分类收纳在 `"文件夹(N)"` 分组节点下，Header 显示自定义分类总数 `N`，左侧配备展开/折叠箭头（`"scroll-008"` / `"scroll-010"`），右侧配备 `+` 新建新建分类快捷按钮（图标 `"add"`）。折叠状态通过 `AppConfig::instance().getBool("Library/CustomSectionCollapsed", false)` 跨会话持久化存储。
2. **新增底栏 `QLineEdit#LibraryFilterEdit` 搜索框**：固定在 `LibraryPanel` 底部，配备 Leading `"seach-7"` 图标与 QSS 外联选择器，实时对“系统分类”与“自定义分类”进行不区分大小写字串过滤。
3. **数字计数右对齐与名称截断**：分类项数字计数右对齐以 `#999999` 加粗绘制在最右侧（距右边界 10px），分类名称左对齐并施加 `Qt::ElideRight` 截断，避免与数字计数重叠。
4. **过滤关键字匹配高亮**：在搜索模式下，匹配的搜索文本高亮以 `#3498db` 色彩渲染。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/LibraryPanel.h`
2. `src/ui/LibraryPanel.cpp`
3. `resources/style.qss`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/LibraryPanel.h`

<<<<<<< SEARCH
#pragma once

#include <QFrame>
#include <QTreeView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QStyledItemDelegate>
#include "DropTreeView.h"

namespace QuarkMeta {

class LibraryItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit LibraryItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

class LibraryPanel : public QFrame {
    Q_OBJECT

public:
    explicit LibraryPanel(QWidget* parent = nullptr);
    ~LibraryPanel() override = default;

    void loadLibrary();

signals:
    void categoryPathsSelected(const QStringList& paths);
    void requestLocateFile(const QString& path);

private slots:
    void onCategoryClicked(const QModelIndex& index);
    void onCategoryContextMenu(const QPoint& pos);
    void onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target);

private:
    void initUi();
    void createAndEditCategory(int parentId = 0);
    QStandardItem* findItemByNodeId(QStandardItem* parent, int nodeId);

    QVBoxLayout* m_mainLayout = nullptr;
    DropTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
    bool m_isLoading = false;
    int m_pendingEditNodeId = 0;
};

} // namespace QuarkMeta
=======
#pragma once

#include <QFrame>
#include <QTreeView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include "DropTreeView.h"

namespace QuarkMeta {

class LibraryItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit LibraryItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void setFilterKeyword(const QString& keyword) { m_filterKeyword = keyword; }

private:
    QString m_filterKeyword;
};

class LibraryPanel : public QFrame {
    Q_OBJECT

public:
    explicit LibraryPanel(QWidget* parent = nullptr);
    ~LibraryPanel() override = default;

    void loadLibrary();

signals:
    void categoryPathsSelected(const QStringList& paths);
    void requestLocateFile(const QString& path);

private slots:
    void onCategoryClicked(const QModelIndex& index);
    void onCategoryContextMenu(const QPoint& pos);
    void onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target);

private:
    void initUi();
    void createAndEditCategory(int parentId = 0);
    void filterTreeNodes(const QString& keyword);
    QStandardItem* findItemByNodeId(QStandardItem* parent, int nodeId);

    QVBoxLayout* m_mainLayout = nullptr;
    DropTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
    LibraryItemDelegate* m_delegate = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    bool m_isLoading = false;
    int m_pendingEditNodeId = 0;
};

} // namespace QuarkMeta
>>>>>>> REPLACE

---

### 3.2 `src/ui/LibraryPanel.cpp`

<<<<<<< SEARCH
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
#include <QLineEdit>
#include <QTimer>

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
                       ViewDragDropHelper::isDropTarget(option.widget, index);

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
    if (count >= 0) {
        text += QString(" (%1)").arg(count);
    }

    painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
    painter->setFont(opt.font);

    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width() - 6);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    painter->restore();
}
=======
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
#include "../core/AppConfig.h"
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
#include <QLineEdit>
#include <QTimer>
#include <functional>

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

    int nodeId = index.data(Qt::UserRole + 1).toInt();

    bool isDropTarget = index.data(IsDropTargetRole).toBool() ||
                       ViewDragDropHelper::isDropTarget(option.widget, index);

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

    // 1. 渲染自定义分类折叠分组 Header (nodeId == 0)
    if (nodeId == 0) {
        const QTreeView* treeView = qobject_cast<const QTreeView*>(option.widget);
        bool isExpanded = treeView ? treeView->isExpanded(index) : true;
        int customCount = index.data(Qt::UserRole + 9).toInt();

        // 绘制折叠/展开箭头 (scroll-008: 展开向下, scroll-010: 折叠向右)
        QString arrowKey = isExpanded ? "scroll-008" : "scroll-010";
        QIcon arrowIcon = UiHelper::getIcon(arrowKey, QColor("#888888"), 14);
        QRect arrowRect(opt.rect.left() + 6, opt.rect.top() + (opt.rect.height() - 14) / 2, 14, 14);
        arrowIcon.paint(painter, arrowRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);

        // 绘制 Header 文字 "文件夹(N)"
        QString sectionText = QString("文件夹(%1)").arg(customCount);
        QRect textRect(arrowRect.right() + 6, opt.rect.top(), opt.rect.width() - 50, opt.rect.height());

        QFont hdrFont = opt.font;
        hdrFont.setPointSize(11);
        hdrFont.setBold(true);
        painter->setFont(hdrFont);
        painter->setPen((opt.state & QStyle::State_MouseOver) ? QColor("#FFFFFF") : QColor("#888888"));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, sectionText);

        // 右侧加号按钮 (+) 图标 ("add")
        QIcon addIcon = UiHelper::getIcon("add", (opt.state & QStyle::State_MouseOver) ? QColor("#FFFFFF") : QColor("#888888"), 16);
        QRect addRect(opt.rect.right() - 24, opt.rect.top() + (opt.rect.height() - 16) / 2, 16, 16);
        addIcon.paint(painter, addRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);

        painter->restore();
        return;
    }

    // 2. 渲染系统分类与普通自定义分类节点 (nodeId != 0)
    int leftMargin = 10;
    int iconSize = 18;
    int spacing = 8;
    int rightMargin = 10;

    int count = index.data(Qt::UserRole + 9).toInt();
    QString countStr = (count >= 0) ? QString::number(count) : "";
    int countWidth = countStr.isEmpty() ? 0 : (opt.fontMetrics.horizontalAdvance(countStr) + 12);

    QRect iconRect(opt.rect.left() + leftMargin, opt.rect.top() + (opt.rect.height() - iconSize) / 2, iconSize, iconSize);
    QRect textRect(iconRect.right() + spacing, opt.rect.top(), opt.rect.width() - leftMargin - iconSize - spacing - countWidth - rightMargin, opt.rect.height());

    QVariant decoData = index.data(Qt::DecorationRole);
    if (decoData.canConvert<QIcon>()) {
        QIcon icon = decoData.value<QIcon>();
        if (!icon.isNull()) {
            icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }
    }

    QString text = index.data(Qt::DisplayRole).toString();
    painter->setFont(opt.font);

    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width() - 4);

    if (!m_filterKeyword.isEmpty() && text.contains(m_filterKeyword, Qt::CaseInsensitive)) {
        painter->setPen(QColor("#3498db"));
    } else {
        painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
    }
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    // 绘制右对齐数字计数
    if (!countStr.isEmpty()) {
        QRect countRect(opt.rect.right() - countWidth - rightMargin, opt.rect.top(), countWidth, opt.rect.height());
        QFont countFont = opt.font;
        countFont.setBold(true);
        painter->setFont(countFont);
        painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#999999"));
        painter->drawText(countRect, Qt::AlignRight | Qt::AlignVCenter, countStr);
    }

    painter->restore();
}
>>>>>>> REPLACE

---

### 3.3 `src/ui/LibraryPanel.cpp` (initUi & Filter Wire-up)

<<<<<<< SEARCH
void LibraryPanel::initUi() {
    m_treeView = new DropTreeView(this);
    m_treeView->setObjectName("LibraryTreeView");
    m_treeView->setHeaderHidden(true);
    if (m_treeView->header()) {
        m_treeView->header()->setStretchLastSection(true);
        m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    m_treeView->setIndentation(16);
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
=======
void LibraryPanel::initUi() {
    m_treeView = new DropTreeView(this);
    m_treeView->setObjectName("LibraryTreeView");
    m_treeView->setHeaderHidden(true);
    if (m_treeView->header()) {
        m_treeView->header()->setStretchLastSection(true);
        m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    m_treeView->setIndentation(16);
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
    m_delegate = new LibraryItemDelegate(this);
    m_treeView->setItemDelegate(m_delegate);

    m_mainLayout->addWidget(m_treeView, 1);

    // 底部搜索过滤框部件创建与配置
    QWidget* filterContainer = new QWidget(this);
    filterContainer->setObjectName("LibraryFilterContainer");
    QVBoxLayout* filterLayout = new QVBoxLayout(filterContainer);
    filterLayout->setContentsMargins(8, 8, 8, 8);
    filterLayout->setSpacing(0);

    m_filterEdit = new QLineEdit(filterContainer);
    m_filterEdit->setObjectName("LibraryFilterEdit");
    m_filterEdit->setPlaceholderText("搜索文件夹...");
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setFixedHeight(28);
    m_filterEdit->addAction(UiHelper::getIcon("seach-7", QColor("#888888"), 14), QLineEdit::LeadingPosition);
    filterLayout->addWidget(m_filterEdit);

    m_mainLayout->addWidget(filterContainer, 0);

    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        QString kw = text.trimmed();
        if (m_delegate) {
            m_delegate->setFilterKeyword(kw);
        }
        filterTreeNodes(kw);
        if (m_treeView && m_treeView->viewport()) {
            m_treeView->viewport()->update();
        }
    });

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
    if (nodeId == 0) {
        // "文件夹(N)" Section 节点点击处理
        QPoint clickPos = m_treeView->viewport()->mapFromGlobal(QCursor::pos());
        QRect visualRect = m_treeView->visualRect(index);
        QRect plusRect(visualRect.right() - 28, visualRect.top() + (visualRect.height() - 16) / 2, 20, 16);

        if (plusRect.contains(clickPos)) {
            // 点击右侧 '+' 图标，新建根级自定义分类
            createAndEditCategory(0);
            return;
        }

        // 点击 Header 其它区域，切换展开/折叠状态并持久化记录
        bool isExpanded = m_treeView->isExpanded(index);
        m_treeView->setExpanded(index, !isExpanded);
        AppConfig::instance().setBool("Library/CustomSectionCollapsed", isExpanded);
        return;
    }

    QStringList paths = LibraryService::instance().getCategoryPaths(nodeId);
    emit categoryPathsSelected(paths);
}

void LibraryPanel::filterTreeNodes(const QString& keyword) {
    if (!m_model || !m_treeView) return;

    std::function<bool(QStandardItem*)> filterRecursive = [&](QStandardItem* item) -> bool {
        if (!item) return false;
        int nodeId = item->data(Qt::UserRole + 1).toInt();

        bool matchesSelf = keyword.isEmpty() || item->text().contains(keyword, Qt::CaseInsensitive);
        bool anyChildMatches = false;

        for (int i = 0; i < item->rowCount(); ++i) {
            QStandardItem* child = item->child(i);
            if (filterRecursive(child)) {
                anyChildMatches = true;
                m_treeView->setRowHidden(i, item->index(), false);
            } else {
                m_treeView->setRowHidden(i, item->index(), true);
            }
        }

        if (nodeId == 0) {
            return anyChildMatches;
        }

        bool visible = matchesSelf || anyChildMatches;
        if (!keyword.isEmpty() && visible && item->hasChildren()) {
            m_treeView->expand(item->index());
        }
        return visible;
    };

    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem* topItem = m_model->item(i);
        if (!topItem) continue;
        int nodeId = topItem->data(Qt::UserRole + 1).toInt();
        if (nodeId < 0) {
            bool visible = keyword.isEmpty() || topItem->text().contains(keyword, Qt::CaseInsensitive);
            m_treeView->setRowHidden(i, QModelIndex(), !visible);
        } else if (nodeId == 0) {
            bool visible = filterRecursive(topItem);
            m_treeView->setRowHidden(i, QModelIndex(), !visible);
            if (!keyword.isEmpty() && visible) {
                m_treeView->expand(topItem->index());
            }
        }
    }
}
>>>>>>> REPLACE

---

### 3.4 `src/ui/LibraryPanel.cpp` (loadLibrary)

<<<<<<< SEARCH
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
        item->setData("system", TypeRole);
        item->setData(sysId, IdRole);
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

    // 2. 加载用户自定义分类 (带动态计数与完整树构建)
    auto list = LibraryDao::getAllCategories();

    QMap<int, QStandardItem*> itemMap;
    for (const auto& rec : list) {
        int count = LibraryDao::getCategoryPaths(rec.id).size();
        QIcon icon = UiHelper::getIcon(rec.iconKey, QColor(rec.colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, rec.name);
        item->setData("category", TypeRole);
        item->setData(rec.id, IdRole);
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
        QTimer::singleShot(0, this, [this, targetNodeId]() {
            if (m_pendingEditNodeId != targetNodeId) return;
            m_pendingEditNodeId = 0;

            QStandardItem* targetItem = findItemByNodeId(m_model->invisibleRootItem(), targetNodeId);
            if (targetItem && m_treeView) {
                QModelIndex parentIdx = targetItem->parent() ? targetItem->parent()->index() : QModelIndex();
                if (parentIdx.isValid()) {
                    m_treeView->expand(parentIdx);
                }
                m_treeView->setCurrentIndex(targetItem->index());
                m_treeView->edit(targetItem->index());
            }
        });
    }
}
=======
void LibraryPanel::loadLibrary() {
    if (!m_model) return;
    m_isLoading = true;
    m_model->clear();

    LibraryDao::initTable();

    // 1. 注入 2 个固定系统分类 (全部数据, 未分类 - 未标签已物理移除)
    auto addSystemItem = [this](const QString& name, const QString& iconKey, const QString& colorHex, int sysId) {
        int count = LibraryDao::getCategoryPaths(sysId).size();
        QIcon icon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, name);
        item->setData("system", TypeRole);
        item->setData(sysId, IdRole);
        item->setData(sysId, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(colorHex, Qt::UserRole + 3);
        item->setData(count, Qt::UserRole + 9);
        item->setEditable(false);
        m_model->appendRow(item);
    };

    addSystemItem("全部数据", "all_data", "#3498db", -1);
    addSystemItem("未分类", "uncategorized", "#95a5a6", -2);

    // 2. 构建 "文件夹(N)" 折叠分组 Header 节点
    auto list = LibraryDao::getAllCategories();
    int customCount = list.size();

    QStandardItem* sectionItem = new QStandardItem();
    sectionItem->setData("section", TypeRole);
    sectionItem->setData(0, IdRole);
    sectionItem->setData(0, Qt::UserRole + 1);
    sectionItem->setData(customCount, Qt::UserRole + 9);
    sectionItem->setEditable(false);
    m_model->appendRow(sectionItem);

    // 3. 加载用户自定义分类并挂载在 sectionItem 及其子节点下
    QMap<int, QStandardItem*> itemMap;
    for (const auto& rec : list) {
        int count = LibraryDao::getCategoryPaths(rec.id).size();
        QIcon icon = UiHelper::getIcon(rec.iconKey, QColor(rec.colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, rec.name);
        item->setData("category", TypeRole);
        item->setData(rec.id, IdRole);
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
            sectionItem->appendRow(item);
        }
    }

    // 恢复 "文件夹(N)" 节点折叠/展开持久化状态
    bool sectionCollapsed = AppConfig::instance().getBool("Library/CustomSectionCollapsed", false);
    if (m_treeView) {
        m_treeView->setExpanded(sectionItem->index(), !sectionCollapsed);
    }

    m_isLoading = false;

    // 若底栏存在搜索关键字，重新应用过滤
    if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
        filterTreeNodes(m_filterEdit->text().trimmed());
    }

    if (m_pendingEditNodeId > 0 && m_treeView) {
        int targetNodeId = m_pendingEditNodeId;
        QTimer::singleShot(0, this, [this, targetNodeId]() {
            if (m_pendingEditNodeId != targetNodeId) return;
            m_pendingEditNodeId = 0;

            QStandardItem* targetItem = findItemByNodeId(m_model->invisibleRootItem(), targetNodeId);
            if (targetItem && m_treeView) {
                QModelIndex parentIdx = targetItem->parent() ? targetItem->parent()->index() : QModelIndex();
                if (parentIdx.isValid()) {
                    m_treeView->expand(parentIdx);
                }
                m_treeView->setCurrentIndex(targetItem->index());
                m_treeView->edit(targetItem->index());
            }
        });
    }
}
>>>>>>> REPLACE

---

### 3.5 `resources/style.qss`

<<<<<<< SEARCH
#FavoriteContainer, #LibraryContainer {
    background-color: #1E1E1E;
    border: none;
    margin: 0px;
    padding: 0px;
}
=======
#FavoriteContainer, #LibraryContainer {
    background-color: #1E1E1E;
    border: none;
    margin: 0px;
    padding: 0px;
}

/* LibraryPanel 搜索过滤框与容器外联 QSS 选择器 */
QWidget#LibraryFilterContainer {
    background-color: transparent;
    border-top: 1px solid #2D2D2D;
}
QLineEdit#LibraryFilterEdit {
    background: #2D2D2D;
    color: #CCCCCC;
    border: 1px solid #444444;
    border-radius: 4px;
    padding: 0px 6px;
    font-size: 11px;
}
QLineEdit#LibraryFilterEdit:focus {
    border-color: #378ADD;
    color: #FFFFFF;
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **执行编译验证**：
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```
2. **运行界面验证**：
   - 检查 `LibraryPanel` 中系统分类（全部数据、未分类）下方是否新增 `"文件夹(N)"` 分组 Header。
   - 检查 Header 左侧展开/折叠箭头与右侧 `+` 新建分类按钮。
   - 点击 `+` 按钮验证是否正确触发新建分类并处于编辑状态。
   - 点击 Header 任意其它位置，验证是否能展开/折叠自定义分类树，重启应用后验证折叠状态是否持久化。
   - 检查底部 `QLineEdit#LibraryFilterEdit` 搜索框，输入分类名称进行筛选，验证匹配节点高亮以及非匹配节点的连同隐藏。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **官方 Icon SSOT**：复用 `UiHelper::getIcon("seach-7")`、`UiHelper::getIcon("scroll-008")`、`UiHelper::getIcon("scroll-010")` 与 `UiHelper::getIcon("add")`，严格遵循 Icon 图标通道。
- **配置持久化 SSOT**：复用 `AppConfig::instance().getBool/setBool` 通道进行折叠状态持久化。
- **不另起炉灶自查**：通过 `m_treeView->setRowHidden` 原生机制实施节点筛选，零 Model 另起炉灶与零数据库访问损耗。

---

## 6. Header API Signature Verification

| 被调用接口 / 方法 | 所在头文件 | 物理精确签名 |
| :--- | :--- | :--- |
| `AppConfig::instance()` | `src/core/AppConfig.h` | `static AppConfig& instance()` |
| `AppConfig::getBool` | `src/core/AppConfig.h` | `bool getBool(const QString& key, bool defaultValue = false) const` |
| `AppConfig::setBool` | `src/core/AppConfig.h` | `void setBool(const QString& key, bool value)` |
| `UiHelper::getIcon` | `src/ui/UiHelper.h` | `static QIcon getIcon(const QString& name, const QColor& color = QColor(), int size = 16)` |
| `LibraryDao::getAllCategories` | `src/meta/LibraryDao.h` | `static QList<LibraryCategoryRecord> getAllCategories()` |

---

## 7. Header Inclusion Chain & Type Completeness Check

- `src/ui/LibraryPanel.h`: 显式补齐 `#include <QLineEdit>`。
- `src/ui/LibraryPanel.cpp`: 显式补齐 `#include "../core/AppConfig.h"` 以及 `<functional>`。
- 受影响 `.cpp` 中涉及的所有类型 (`QLineEdit`, `AppConfig`, `QStandardItem`, `DropTreeView`) 均具有完整的物理头文件定义，无任何间接包含断裂或 C2027/C2039 隐患。
