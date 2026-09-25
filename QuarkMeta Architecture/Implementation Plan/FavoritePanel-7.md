# Implementation Plan: Remove Bottom Margin Extra Padding Logic from DropTreeView & FavoritePanel

## 1. Overview
This implementation plan completely removes all custom bottom margin padding logic (`m_bottomMargin`, `setBottomMargin`, and scrollbar range extensions in `updateGeometries`) from `DropTreeView` and `FavoritePanel`, restoring default native `QTreeView` scroll behavior.

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
    void setBottomMargin(int bottom) { m_bottomMargin = bottom; updateGeometries(); }

    void applyColumnPolicies();
=======
    int rowHeight(const QModelIndex& index) const { return QTreeView::rowHeight(index); }
    void setEmptyHint(const QString& hint) { m_emptyHint = hint; }

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
    void updateGeometries() override;

private:
    QTimer* m_autoExpandTimer = nullptr;
    QModelIndex m_hoverIndex;
    QString m_emptyHint;
    int m_bottomMargin = 0;
};
=======
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
>>>>>>> REPLACE
```

### 3.2 `src/ui/DropTreeView.cpp`

```
<<<<<<< SEARCH
void DropTreeView::updateGeometries() {
    QTreeView::updateGeometries();
    if (m_bottomMargin > 0 && verticalScrollBar()) {
        QScrollBar* bar = verticalScrollBar();
        if (bar->maximum() > 0) {
            int extra = (verticalScrollMode() == QAbstractItemView::ScrollPerPixel)
                        ? m_bottomMargin
                        : qMax(1, m_bottomMargin / 28);
            bar->setRange(bar->minimum(), bar->maximum() + extra);
        }
    }
}
=======
>>>>>>> REPLACE
```

### 3.3 `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
    m_favoriteView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_favoriteView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_favoriteView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_favoriteView->setBottomMargin(84);

    m_favoriteModel = new QStandardItemModel(this);
=======
    m_favoriteView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_favoriteView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_favoriteView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_favoriteModel = new QStandardItemModel(this);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile the project.
2. Verify that `DropTreeView` builds cleanly without any `m_bottomMargin` references.
3. Observe `FavoritePanel` and confirm that native scroll geometry is restored with zero custom padding appended to the bottom.
