# Implementation Plan: FavoritePanel Tree View Scrollbar Range Bottom Margin Fix

## 1. Overview
The previous usage of `setViewportMargins(0, 0, 0, 84)` subtracted margin from outside the tree view's viewport, which failed to extend the scrollable content range and made the bottom area non-interactive for right-clicking.

This implementation plan overrides `updateGeometries()` in `DropTreeView` to extend `verticalScrollBar()->maximum()` by `m_bottomMargin` (84px, equivalent to 3 rows). This allows `DropTreeView` to scroll 84px past the last item, rendering true blank canvas space inside the active viewport where `indexAt(pos)` returns an invalid index, reliably triggering the blank space context menu.

---

## 2. Modified Files List
1. `src/ui/DropTreeView.h`
2. `src/ui/DropTreeView.cpp`
3. `src/ui/FavoritePanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/DropTreeView.h`

```
<<<<<<< SEARCH
    int rowHeight(const QModelIndex& index) const { return QTreeView::rowHeight(index); }
    void setEmptyHint(const QString& hint) { m_emptyHint = hint; }
    void setBottomMargin(int bottom) { setViewportMargins(0, 0, 0, bottom); }

    void applyColumnPolicies();
=======
    int rowHeight(const QModelIndex& index) const { return QTreeView::rowHeight(index); }
    void setEmptyHint(const QString& hint) { m_emptyHint = hint; }
    void setBottomMargin(int bottom) { m_bottomMargin = bottom; updateGeometries(); }

    void applyColumnPolicies();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
protected:
    void startDrag(Qt::DropActions supportedActions) override;

    void keyboardSearch(const QString& search) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QTimer* m_autoExpandTimer = nullptr;
    QModelIndex m_hoverIndex;
    QString m_emptyHint;
};
=======
protected:
    void startDrag(Qt::DropActions supportedActions) override;

    void keyboardSearch(const QString& search) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void updateGeometries() override;

private:
    QTimer* m_autoExpandTimer = nullptr;
    QModelIndex m_hoverIndex;
    QString m_emptyHint;
    int m_bottomMargin = 0;
};
>>>>>>> REPLACE
```

### 3.2 `src/ui/DropTreeView.cpp`

```
<<<<<<< SEARCH
void DropTreeView::resizeEvent(QResizeEvent* event) {
    QTreeView::resizeEvent(event);
    applyColumnPolicies();
}
=======
void DropTreeView::resizeEvent(QResizeEvent* event) {
    QTreeView::resizeEvent(event);
    applyColumnPolicies();
}

void DropTreeView::updateGeometries() {
    QTreeView::updateGeometries();
    if (m_bottomMargin > 0 && verticalScrollBar()) {
        QScrollBar* bar = verticalScrollBar();
        bar->setRange(bar->minimum(), bar->maximum() + m_bottomMargin);
    }
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
    m_favoriteView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_favoriteView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_favoriteView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_favoriteView->setBottomMargin(84);
=======
    m_favoriteView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_favoriteView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_favoriteView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_favoriteView->setBottomMargin(84);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Open the application with a filled FavoritePanel tree.
2. Scroll to the bottom of the tree view.
3. Observe that there is an extra 84px (3 rows height) of empty space past the last item.
4. Right-click inside this empty space area.
5. Verify that `onFavoriteContextMenu` receives an invalid `QModelIndex` and successfully displays the blank space menu ("新建文件夹", "排列").
