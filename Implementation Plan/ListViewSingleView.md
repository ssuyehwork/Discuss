# Implementation Plan - Step 3: Single View Transformation for DropTreeView (List Mode)

This plan details transforming `ContentPanel`'s List mode to use a single `DropTreeView` attached to `SectionProxyModel`.

## 1. Overview
Currently, `ContentPanel` uses `SectionedScrollCanvas` (`m_listCanvas`) containing two `DropTreeView` instances (`m_folderTreeView` and `m_treeView`).
This refactoring removes `SectionedScrollCanvas` for list view, makes `m_treeView` (a `DropTreeView`) the sole tree/list view instance, and sets its model to `SectionProxyModel`.

---

## 2. Modified Files List
1. `src/ui/TreeItemDelegate.h` & `src/ui/TreeItemDelegate.cpp` (Draw section headers and section zebra-striping)
2. `src/ui/ContentPanel.h` (Remove `m_listCanvas`, `m_folderTreeView`, `m_folderProxyModel`; rename `m_fileProxyModel` -> `m_listProxyModel`)
3. `src/ui/ContentPanel.cpp` (Setup single tree view & proxy model, set first column spanned for header rows, handle clicks on headers)
4. `src/ui/controllers/ContentSortController.cpp` (Forward column header sorting directly to proxy model)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/TreeItemDelegate.cpp`
<<<<<<< SEARCH
    // TreeItemDelegate paint method
=======
void TreeItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (index.data(SectionHeaderRole).toBool()) {
        painter->save();
        QRect rect = option.rect;
        painter->fillRect(rect, QColor("#1E1E1E"));

        QFont font = painter->font();
        font.setBold(true);
        font.setPixelSize(12);
        painter->setFont(font);
        painter->setPen(QColor("#3498db"));

        QString text = index.data(SectionHeaderTextRole).toString();
        QRect textRect = rect.adjusted(10, 0, -30, 0);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

        if (text.startsWith("文件夹")) {
            bool collapsed = index.data(SectionCollapsedRole).toBool();
            QIcon arrowIcon = UiHelper::getIcon(collapsed ? "scroll-008" : "scroll-010", "#3498db");
            QRect iconRect(rect.right() - 25, rect.top() + (rect.height() - 12) / 2, 12, 12);
            arrowIcon.paint(painter, iconRect);
        }

        painter->restore();
        return;
    }

    // Normal item rendering with section zebra striping...
    QStyleOptionViewItem opt = option;
    // Calculate section row index (row minus headers before it) for alternating background
    int pRow = index.row();
    // ...
    QStyledItemDelegate::paint(painter, opt, index);
}

QSize TreeItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (index.data(SectionHeaderRole).toBool()) {
        return QSize(option.rect.width(), 28);
    }
    return QStyledItemDelegate::sizeHint(option, index);
}
>>>>>>> REPLACE

---

### 3.2 `src/ui/ContentPanel.h`
<<<<<<< SEARCH
    SectionedScrollCanvas* m_listCanvas = nullptr;
    DropTreeView* m_folderTreeView = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
=======
    DropTreeView* m_treeView = nullptr;
    FilterProxyModel* m_listProxyModel = nullptr;
    SectionProxyModel* m_listSectionProxyModel = nullptr;
>>>>>>> REPLACE

---

### 3.3 `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
    // Setup single m_treeView with m_listSectionProxyModel
=======
    m_listProxyModel = new FilterProxyModel(this);
    m_listSectionProxyModel = new SectionProxyModel(this);
    m_listSectionProxyModel->setSourceModel(m_listProxyModel);

    m_treeView = new DropTreeView(this);
    m_treeView->setModel(m_listSectionProxyModel);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Apply first column spanning for header rows on model reset or layout changes
    connect(m_listSectionProxyModel, &QAbstractItemModel::modelReset, this, [this]() {
        int total = m_listSectionProxyModel->rowCount();
        for (int r = 0; r < total; ++r) {
            QModelIndex idx = m_listSectionProxyModel->index(r, 0);
            if (idx.data(SectionHeaderRole).toBool()) {
                m_treeView->setFirstColumnSpanned(r, QModelIndex(), true);
            }
        }
    });
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Apply changes to `TreeItemDelegate`, `ContentPanel.h/.cpp`, `ContentSortController`.
2. Compile project using CMake.
3. Verify List view mode displays single `DropTreeView` with folder and file section headers spanning all columns.
4. Verify sorting on column headers correctly sorts items while preserving header rows at section tops.
