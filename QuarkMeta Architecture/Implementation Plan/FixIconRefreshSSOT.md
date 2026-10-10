# Implementation Plan - FixIconRefreshSSOT.md

## Overview
Restore instant SSOT view refresh when changing icons/colors in `FavoritePanel` and `LibraryPanel` by removing `blockSignals(true/false)` while retaining `QPersistentModelIndex` safety.

## Root Cause Analysis
`blockSignals(true)` was previously added around `item->setIcon()` to suppress potential re-entrant model updates. However, blocking signals prevented `QStandardItemModel` from emitting `dataChanged` signals to `QTreeView`, causing the view to miss icon updates until an explicit reload occurred.

## Solution Strategy
1. Remove `blockSignals(true)` and `blockSignals(false)` from the icon and color selection callbacks in `FavoritePanel.cpp` and `LibraryPanel.cpp`.
2. Allow `QStandardItem::setIcon()` and `setData()` to emit standard `dataChanged` signals naturally.
3. Retain `QPersistentModelIndex` checks to prevent access violations if item indices become invalid during context menu display.

## Modified Files List
- `src/ui/FavoritePanel.cpp`
- `src/ui/LibraryPanel.cpp`

## Detailed Changes

### `src/ui/FavoritePanel.cpp`
```
<<<<<<< SEARCH
                QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
                if (m_favoriteModel) m_favoriteModel->blockSignals(true);
                item->setIcon(newIcon);
                item->setData(iconKey, Qt::UserRole + 2);
                if (m_favoriteModel) m_favoriteModel->blockSignals(false);
=======
                QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
                item->setIcon(newIcon);
                item->setData(iconKey, Qt::UserRole + 2);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
                if (m_favoriteModel) m_favoriteModel->blockSignals(true);
                item->setIcon(newIcon);
                item->setData(finalColor, Qt::UserRole + 3);
                if (m_favoriteModel) m_favoriteModel->blockSignals(false);
=======
                QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
                item->setIcon(newIcon);
                item->setData(finalColor, Qt::UserRole + 3);
>>>>>>> REPLACE
```

### `src/ui/LibraryPanel.cpp`
```
<<<<<<< SEARCH
            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
            if (m_model) m_model->blockSignals(true);
            item->setIcon(newIcon);
            item->setData(iconKey, Qt::UserRole + 2);
            if (m_model) m_model->blockSignals(false);
=======
            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
            item->setIcon(newIcon);
            item->setData(iconKey, Qt::UserRole + 2);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
            if (m_model) m_model->blockSignals(true);
            item->setIcon(newIcon);
            item->setData(finalColor, Qt::UserRole + 3);
            if (m_model) m_model->blockSignals(false);
=======
            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
            item->setIcon(newIcon);
            item->setData(finalColor, Qt::UserRole + 3);
>>>>>>> REPLACE
```

## Build & Verification
1. Open FavoritePanel or LibraryPanel.
2. Select a category/folder and change its icon or color via right-click menu.
3. Verify that the icon/color updates immediately in the tree view upon selection.
