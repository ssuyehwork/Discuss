# Implementation Plan - ColumnViewWidget-5.md

## 1. Overview
When navigating subfolders in Column View mode (`ColumnViewWidget`), clicking a folder expands a new rightmost column (`rightmostPane`). Because parent columns only filter hidden items (`parentFilter.showHidden`), subfolders remain visible for navigation. However, the new rightmost column receives the full global filter (`m_currentFilter`). If active filters (e.g., file type, rating, or unchecked "folders" toggle) reject all items in the subfolder, the rightmost column appears empty, causing confusion even though items exist (`fullCount > 0 && visibleCount == 0`).

This plan introduces an empty-state filter banner/overlay in `ColumnViewPane` that displays an informative message when items exist in the directory but are completely filtered out by active filter criteria.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.h`
Add an empty overlay label pointer in `ColumnViewPane`:

```
<<<<<<< SEARCH
    QListView* m_listView = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
=======
    QListView* m_listView = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    QLabel* m_emptyFilterHintLabel = nullptr;
>>>>>>> REPLACE
```

### `src/ui/ColumnViewWidget.cpp`
1. Initialize `m_emptyFilterHintLabel` in `ColumnViewPane::ColumnViewPane` and connect `modelReset`/`layoutChanged` to update the hint:

```
<<<<<<< SEARCH
    auto* delegate = new ColumnItemDelegate(this);
    m_listView->setItemDelegate(delegate);
    layout->addWidget(m_listView);
=======
    auto* delegate = new ColumnItemDelegate(this);
    m_listView->setItemDelegate(delegate);
    layout->addWidget(m_listView);

    m_emptyFilterHintLabel = new QLabel(this);
    m_emptyFilterHintLabel->setAlignment(Qt::AlignCenter);
    m_emptyFilterHintLabel->setWordWrap(true);
    m_emptyFilterHintLabel->setStyleSheet("color: #888888; font-size: 12px; padding: 16px;");
    m_emptyFilterHintLabel->hide();
    layout->addWidget(m_emptyFilterHintLabel);
>>>>>>> REPLACE
```

2. Implement a helper method `updateEmptyFilterHint()` and connect it to model layout/reset changes in `ColumnViewPane`:

```
<<<<<<< SEARCH
    connect(m_proxyModel, &QAbstractItemModel::modelReset, this, &ColumnViewPane::tryPendingSelection);
    connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, &ColumnViewPane::tryPendingSelection);
=======
    auto checkEmptyHint = [this]() {
        tryPendingSelection();
        if (!m_model || !m_proxyModel || !m_emptyFilterHintLabel) return;
        int fullCount = m_model->rowCount();
        int visibleCount = m_proxyModel->rowCount();
        int hiddenCount = fullCount - visibleCount;

        if (fullCount > 0 && visibleCount == 0) {
            m_emptyFilterHintLabel->setText(QString("所有内容已被筛选隐藏 (%1 个项目)").arg(hiddenCount));
            m_emptyFilterHintLabel->show();
            if (m_listView) m_listView->hide();
        } else {
            m_emptyFilterHintLabel->hide();
            if (m_listView) m_listView->show();
        }
    };

    connect(m_proxyModel, &QAbstractItemModel::modelReset, this, checkEmptyHint);
    connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, checkEmptyHint);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation Verification**:
   Run `cmake --build build` to verify clean compilation with Qt MOC updates.
2. **Behavioral Verification**:
   - Set a restrictive filter (e.g. uncheck "Folders" or filter by a specific extension in the right Filter Panel).
   - In Column View mode, click a subfolder whose items are all hidden by the filter.
   - Verify that the new column displays the empty filter hint ("所有内容已被筛选隐藏 (7 个项目)") instead of appearing mysteriously blank.
