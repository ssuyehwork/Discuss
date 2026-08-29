# Implementation Plan - FavoritePanel-11

This implementation plan resolves a crash during internal drag-and-drop reordering in `FavoritePanel`.

## 1. Overview
- **Root Cause Analysis**: During internal drag-and-drop item moves in `QStandardItemModel`, Qt fires `rowsInserted` / `rowsMoved` signals before population of newly created item rows is finished. Synchronous execution of `saveFavorites()` accessed `m_favoriteModel->item(i)` when it was still `nullptr`, leading to immediate null pointer dereferences and crashes.
- **Null Pointer Safeguard**: Added `if (!item) continue;` and `if (!path.isEmpty())` guards inside `FavoritePanel::saveFavorites()`.
- **Queued Signal Dispatch**: Switched `rowsMoved`, `rowsInserted`, and `rowsRemoved` signal connections in `FavoritePanel.cpp` to `Qt::QueuedConnection` to ensure `saveFavorites()` runs safely after `QStandardItemModel` completes full internal row migration and cleanup.

## 2. Modified Files List
- `src/ui/FavoritePanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`
```diff
<<<<<<< SEARCH
    // 模型数据变动监听
    auto updateFavAndSave = [this](){ saveFavorites(); };
    connect(m_favoriteModel, &QStandardItemModel::rowsMoved, this, updateFavAndSave);
    connect(m_favoriteModel, &QStandardItemModel::rowsInserted, this, updateFavAndSave);
    connect(m_favoriteModel, &QStandardItemModel::rowsRemoved, this, updateFavAndSave);
=======
    // 模型数据变动监听 (使用 QueuedConnection 避开 model 拖拽中间状态)
    auto updateFavAndSave = [this](){ saveFavorites(); };
    connect(m_favoriteModel, &QStandardItemModel::rowsMoved, this, updateFavAndSave, Qt::QueuedConnection);
    connect(m_favoriteModel, &QStandardItemModel::rowsInserted, this, updateFavAndSave, Qt::QueuedConnection);
    connect(m_favoriteModel, &QStandardItemModel::rowsRemoved, this, updateFavAndSave, Qt::QueuedConnection);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void FavoritePanel::saveFavorites() {
    if (!m_favoriteModel) return;

    QList<QPair<QString, int>> orders;
    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QStandardItem* item = m_favoriteModel->item(i);
        QString path = item->data(Qt::UserRole + 1).toString();
        orders.append({ path, i + 1 });
    }
    FavoriteDao::updateSortOrders(orders);
}
=======
void FavoritePanel::saveFavorites() {
    if (!m_favoriteModel) return;

    QList<QPair<QString, int>> orders;
    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QStandardItem* item = m_favoriteModel->item(i);
        if (!item) continue;
        QString path = item->data(Qt::UserRole + 1).toString();
        if (!path.isEmpty()) {
            orders.append({ path, i + 1 });
        }
    }
    FavoriteDao::updateSortOrders(orders);
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild the project using CMake:
   ```bash
   cmake -B build
   cmake --build build
   ```
2. Test internal reordering in `FavoritePanel`:
   - Drag items up and down repeatedly: Verify item position updates smoothly without crash and updated sort order persists to SQLite `global.db`.
