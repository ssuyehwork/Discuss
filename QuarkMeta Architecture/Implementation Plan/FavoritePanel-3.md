# Implementation Plan: FavoritePanel Bottom Margin Padding Fix

## 1. Overview
This implementation plan adds a forced bottom viewport margin of at least 3 row heights (84px, assuming row height is ~28px) to `m_favoriteView` in `FavoritePanel`. This ensures that even when the favorite tree contains many items or expanded folders, there is always empty whitespace at the bottom of the panel for user interaction (such as left-clicking or right-clicking to open the blank space context menu with "新建文件夹" and "排列").

---

## 2. Modified Files List
1. `src/ui/FavoritePanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
    m_favoriteView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_favoriteView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_favoriteView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_favoriteModel = new QStandardItemModel(this);
=======
    m_favoriteView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_favoriteView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_favoriteView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_favoriteView->setViewportMargins(0, 0, 0, 84);

    m_favoriteModel = new QStandardItemModel(this);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Add enough items to `FavoritePanel` to fill the entire visible height.
2. Scroll to the very bottom.
3. Observe that there is at least 84px (~3 rows) of empty space below the last item in `m_favoriteView`.
4. Right-click on the empty space area.
5. Verify that the context menu pops up showing blank space actions ("新建文件夹", "排列").

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Entry**: Utilizes standard Qt Viewport Margins (`QAbstractScrollArea::setViewportMargins`) without altering or hacking window properties or private delegates.
