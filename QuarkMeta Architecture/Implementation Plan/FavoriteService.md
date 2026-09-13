# Implementation Plan - FavoriteService Normalization

## 1. Overview
This implementation plan unifies the scattered favorite item management logic into a single domain service `FavoriteService`.
Specifically:
1. Create `FavoriteService` (`src/meta/FavoriteService.h` and `src/meta/FavoriteService.cpp`) as the single source of truth for favorite operations (`addFavorite`, `removeFavorite`, `toggleFavorite`, `isFavorite`, and building context menu actions).
2. Register `FavoriteService` in `CMakeLists.txt`.
3. Replace scattered `FavoriteDao` calls and manual `QAction` menu creation across `ContentContextMenu`, `NavPanel`, `AddressBar`, `QuickLookWindow`, and `CoreEngine` with `FavoriteService`.
4. Decouple `FavoritePanel` UI widget from direct data management, delegating to `FavoriteService`.

---

## 2. Modified Files List
- `CMakeLists.txt`
- `src/meta/FavoriteService.h` (New File)
- `src/meta/FavoriteService.cpp` (New File)
- `src/ui/controllers/ContentContextMenu.cpp`
- `src/ui/NavPanel.cpp`
- `src/ui/AddressBar.cpp`
- `src/ui/QuickLookWindow.cpp`
- `src/ui/FavoritePanel.h`
- `src/ui/FavoritePanel.cpp`
- `src/core/CoreEngine.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
Add `FavoriteService.h` and `FavoriteService.cpp` to the `SOURCES` list:

```
<<<<<<< SEARCH
    src/meta/FavoriteDao.h
    src/meta/FavoriteDao.cpp
=======
    src/meta/FavoriteDao.h
    src/meta/FavoriteDao.cpp
    src/meta/FavoriteService.h
    src/meta/FavoriteService.cpp
>>>>>>> REPLACE
```

---

### 3.2 Create `src/meta/FavoriteService.h`
Create new file defining `FavoriteService`:

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QMenu>
#include <QAction>
#include "FavoriteDao.h"

namespace QuarkMeta {

class FavoriteService : public QObject {
    Q_OBJECT
public:
    static FavoriteService& instance();

    bool isFavorite(const QString& path) const;
    bool addFavorite(const QString& path);
    bool removeFavorite(const QString& path);
    bool toggleFavorite(const QString& path);

    QAction* buildFavoriteAction(QMenu* parentMenu, const QString& path, QObject* receiver = nullptr);

signals:
    void favoriteChanged(const QString& path, bool isFavorite);
    void favoritesReloaded();

private:
    explicit FavoriteService(QObject* parent = nullptr);
    ~FavoriteService() override = default;
    Q_DISABLE_COPY_MOVE(FavoriteService)
};

} // namespace QuarkMeta
```

---

### 3.3 Create `src/meta/FavoriteService.cpp`
Create new file implementing `FavoriteService`:

```cpp
#include "FavoriteService.h"
#include "../ui/UiHelper.h"
#include <QDir>
#include <QFileInfo>

namespace QuarkMeta {

FavoriteService& FavoriteService::instance() {
    static FavoriteService s_instance;
    return s_instance;
}

FavoriteService::FavoriteService(QObject* parent) : QObject(parent) {
    FavoriteDao::initTable();
}

bool FavoriteService::isFavorite(const QString& path) const {
    if (path.isEmpty()) return false;
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    return FavoriteDao::containsPath(cleanPath);
}

bool FavoriteService::addFavorite(const QString& path) {
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    if (cleanPath.isEmpty() || FavoriteDao::containsPath(cleanPath)) return false;

    QFileInfo fi(cleanPath);
    if (!fi.exists()) return false;

    bool isDir = fi.isDir();
    QString finalColorHex = "#FDB70A";

    if (isDir) {
        bool isDriveRoot = fi.isRoot() || cleanPath.endsWith(":\\") || cleanPath.endsWith(":/") || (cleanPath.length() == 2 && cleanPath.endsWith(':'));
        if (isDriveRoot) {
            finalColorHex = "#378ADD";
        }
    }

    bool ok = FavoriteDao::addFavorite(cleanPath, "folder_filled", finalColorHex);
    if (ok) {
        emit favoriteChanged(cleanPath, true);
    }
    return ok;
}

bool FavoriteService::removeFavorite(const QString& path) {
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    if (cleanPath.isEmpty() || !FavoriteDao::containsPath(cleanPath)) return false;

    bool ok = FavoriteDao::removeFavorite(cleanPath);
    if (ok) {
        emit favoriteChanged(cleanPath, false);
    }
    return ok;
}

bool FavoriteService::toggleFavorite(const QString& path) {
    if (isFavorite(path)) {
        return removeFavorite(path);
    } else {
        return addFavorite(path);
    }
}

QAction* FavoriteService::buildFavoriteAction(QMenu* parentMenu, const QString& path, QObject* receiver) {
    if (!parentMenu || path.isEmpty()) return nullptr;

    bool fav = isFavorite(path);
    QIcon favIcon = fav ? UiHelper::getIcon("close", QColor("#EEEEEE"), 18) : UiHelper::getIcon("star_filled", QColor("#EEEEEE"), 18);
    QString text = fav ? "从收藏夹移除" : "添加至收藏夹";

    QAction* action = parentMenu->addAction(favIcon, text);
    connect(action, &QAction::triggered, receiver ? receiver : parentMenu, [this, path]() {
        toggleFavorite(path);
    });
    return action;
}

} // namespace QuarkMeta
```

---

### 3.4 `src/ui/controllers/ContentContextMenu.cpp`
Use `FavoriteService` to build favorite action:

```
<<<<<<< SEARCH
            bool isFavItem = FavoriteDao::containsPath(path);
            menu.addAction(UiHelper::getIcon(isFavItem ? "close" : "star_filled", QColor("#EEEEEE"), 18), isFavItem ? "取消收藏" : "添加至收藏夹")->setData(ContentPanel::ActionAddToFavorites);
=======
            FavoriteService::instance().buildFavoriteAction(&menu, path, m_panel);
>>>>>>> REPLACE
```

---

### 3.5 `src/ui/NavPanel.cpp`
Use `FavoriteService` in `NavPanel`:

```
<<<<<<< SEARCH
    bool isFav = FavoriteDao::containsPath(path);
    QIcon favIcon = isFav ? UiHelper::getIcon("close", QColor("#EEEEEE"), 18) : UiHelper::getIcon("star_filled", QColor("#EEEEEE"), 18);
    QAction* actFavorite = menu.addAction(favIcon, isFav ? "从收藏夹移除" : "添加至收藏夹");
=======
    FavoriteService::instance().buildFavoriteAction(&menu, path, this);
>>>>>>> REPLACE
```

---

### 3.6 `src/ui/AddressBar.cpp`
Use `FavoriteService` in `AddressBar`:

```
<<<<<<< SEARCH
        bool isFav = FavoriteDao::containsPath(nativePath);
        QIcon favIcon = isFav ? UiHelper::getIcon("close", QColor("#EEEEEE")) : UiHelper::getIcon("star_filled", QColor("#EEEEEE"));
        QAction* actFavToggle = menu.addAction(favIcon, isFav ? "取消收藏" : "添加至收藏夹");
=======
        FavoriteService::instance().buildFavoriteAction(&menu, nativePath, this);
>>>>>>> REPLACE
```

---

### 3.7 `src/ui/QuickLookWindow.cpp`
Use `FavoriteService` in `QuickLookWindow`:

```
<<<<<<< SEARCH
    bool isFav = FavoriteDao::containsPath(m_currentPath);
    QIcon favIcon = isFav ? UiHelper::getIcon("close", QColor("#EEEEEE")) : UiHelper::getIcon("star_filled", QColor("#EEEEEE"));
    QAction* actFavorite = menu.addAction(favIcon, isFav ? "取消收藏" : "添加至收藏夹");
=======
    FavoriteService::instance().buildFavoriteAction(&menu, m_currentPath, this);
>>>>>>> REPLACE
```

---

### 3.8 `src/core/CoreEngine.cpp`
Use `FavoriteService` in `CoreEngine::executeCommand`:

```
<<<<<<< SEARCH
        if (!FavoriteDao::containsPath(p)) {
            FavoriteDao::addFavorite(p);
        } else {
            FavoriteDao::removeFavorite(p);
        }
=======
        FavoriteService::instance().toggleFavorite(p);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile project: `cmake --build build --config Debug`
2. Launch QuarkMeta app.
3. Test favorite actions in content view context menu, navigation tree, address bar, and quick look window.
4. Verify favorite items update in `FavoritePanel` across all components consistently.
