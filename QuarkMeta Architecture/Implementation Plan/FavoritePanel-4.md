# Implementation Plan: Fix C2248 Protected Viewport Margins Access in DropTreeView

## 1. Overview
This implementation plan resolves MSVC error `C2248: 'QAbstractScrollArea::setViewportMargins': cannot access protected member` by exposing a public method `setBottomMargin(int bottom)` in `DropTreeView`.

---

## 2. Modified Files List
1. `src/ui/DropTreeView.h`
2. `src/ui/FavoritePanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/DropTreeView.h`

```
<<<<<<< SEARCH
    int rowHeight(const QModelIndex& index) const { return QTreeView::rowHeight(index); }
    void setEmptyHint(const QString& hint) { m_emptyHint = hint; }

    void applyColumnPolicies();
=======
    int rowHeight(const QModelIndex& index) const { return QTreeView::rowHeight(index); }
    void setEmptyHint(const QString& hint) { m_emptyHint = hint; }
    void setBottomMargin(int bottom) { setViewportMargins(0, 0, 0, bottom); }

    void applyColumnPolicies();
>>>>>>> REPLACE
```

### 3.2 `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
    m_favoriteView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_favoriteView->setViewportMargins(0, 0, 0, 84);

    m_favoriteModel = new QStandardItemModel(this);
=======
    m_favoriteView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_favoriteView->setBottomMargin(84);

    m_favoriteModel = new QStandardItemModel(this);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile the solution.
2. Confirm MSVC C2248 error is eliminated.
3. Verify that `FavoritePanel` preserves an 84px bottom margin for blank space interactions.
