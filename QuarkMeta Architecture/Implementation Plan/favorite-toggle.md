# Implementation Plan - Dynamic Toggle for Favorite Actions in Context Menus

## 1. Overview
This implementation plan addresses the context menu flaw where items already bookmarked still display "添加至收藏夹" (Add to Favorites). It introduces a dynamic check against the `FavoritePanel` model state so that context menus (in both `ContentPanel` and `QuickLookWindow`) toggle dynamically between "添加至收藏夹" and "取消收藏" (Remove from Favorites).

## 2. Modified Files List
- `src/ui/FavoritePanel.h`
- `src/ui/FavoritePanel.cpp`
- `src/ui/ContentPanel.cpp`
- `src/ui/QuickLookWindow.cpp`

## 3. Detailed Line-by-Line Changes

```diff
<<<<<<< SEARCH
    /**
     * @brief 向收藏夹追加项目并防重
     */
    void addFavoriteItem(const QString& path);
=======
    /**
     * @brief 检查路径是否已在收藏夹中
     */
    bool isFavorite(const QString& path) const;

    /**
     * @brief 从收藏夹中移除指定路径项目
     */
    void removeFavoriteItem(const QString& path);

    /**
     * @brief 向收藏夹追加项目并防重
     */
    void addFavoriteItem(const QString& path);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void FavoritePanel::addFavoriteItem(const QString& path) {
=======
bool FavoritePanel::isFavorite(const QString& path) const {
    if (!m_favoriteModel || path.isEmpty()) return false;
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QString existingPath = QDir::toNativeSeparators(QDir::cleanPath(m_favoriteModel->item(i)->data(Qt::UserRole + 1).toString()));
        if (QString::compare(existingPath, cleanPath, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

void FavoritePanel::removeFavoriteItem(const QString& path) {
    if (!m_favoriteModel || path.isEmpty()) return;
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QString existingPath = QDir::toNativeSeparators(QDir::cleanPath(m_favoriteModel->item(i)->data(Qt::UserRole + 1).toString()));
        if (QString::compare(existingPath, cleanPath, Qt::CaseInsensitive) == 0) {
            m_favoriteModel->removeRow(i);
            saveFavorites();
            return;
        }
    }
}

void FavoritePanel::addFavoriteItem(const QString& path) {
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
            bool isPinned = currentIndex.data(IsLockedRole).toBool(); 
            menu.addAction(isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ActionUnpin : ActionPin); 
            menu.addAction("添加至收藏夹")->setData(ActionAddToFavorites); 
=======
            bool isPinned = currentIndex.data(IsLockedRole).toBool(); 
            menu.addAction(isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ActionUnpin : ActionPin); 
            bool fav = isItemFavorite(currentIndex.data(PathRole).toString());
            menu.addAction(fav ? "取消收藏" : "添加至收藏夹")->setData(ActionAddToFavorites); 
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    QAction* actFavorite = menu.addAction("添加至收藏夹");
=======
    bool fav = isCurrentFileFavorite();
    QAction* actFavorite = menu.addAction(fav ? "取消收藏" : "添加至收藏夹");
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild application using CMake & Qt build pipeline.
2. Right-click an unbookmarked file/folder in ContentPanel. Verify the menu displays "添加至收藏夹".
3. Add the item to favorites. Right-click the same file/folder again. Verify the menu dynamically displays "取消收藏".
4. Repeat verification in `QuickLookWindow` preview right-click context menu.
