# Favorite System Unification & Anti-Duplication Implementation Plan
(QuarkMeta Architecture/Implementation Plan/favorite_system_unification.md)

---

## 1. Overview

Currently, the application has 4 separate entry points for "Add to Favorites":
1. AddressBar Breadcrumb context menu
2. ContentPanel / ContentContextMenu
3. FavoritePanel Drag & Drop
4. QuickLookWindow context menu

While all of them eventually touch `FavoriteDao`, they bypass the `FavoritePanel` model in some places (e.g. `AddressBar.cpp` calling `FavoriteDao::addFavorite` directly), causing `FavoritePanel` UI to fail to update in real time without a app restart. Furthermore, `ContentContextMenu.cpp` uses `findChild<FavoritePanel*>()` hacks instead of the standard mediator.

### Refactoring Objectives
1. **Unify Entry Points via Mediator / `FavoritePanel`**: All favorite additions/removals will be routed through `FavoritePanel::addFavoriteItem(path)` and `FavoritePanel::removeFavoriteItem(path)`, or through `PanelMediator` signals (`requestToggleFavorite`).
2. **Eliminate Direct `FavoriteDao` Calls in UI Classes**: Remove direct `FavoriteDao::addFavorite` / `FavoriteDao::removeFavorite` calls from `AddressBar.cpp` and `ContentContextMenu.cpp`.
3. **Global Signal Broadcast (`favoriteStateChanged`)**: When `FavoritePanel` updates an item, it emits a signal so `AddressBar`, `ContentPanel`, and `QuickLook` immediately reflect the updated star state.

---

## 2. Modified Files List

1. `src/ui/FavoritePanel.h` & `src/ui/FavoritePanel.cpp`
2. `src/ui/AddressBar.cpp`
3. `src/ui/controllers/ContentContextMenu.cpp`
4. `src/ui/PanelMediator.h` & `src/ui/PanelMediator.cpp`
5. `src/ui/QuickLookWindow.cpp`

---

## 3. Detailed Changes Strategy

### A. `FavoritePanel` (Domain & View Owner)
- Ensure `addFavoriteItem(path)` and `removeFavoriteItem(path)` emit a public signal `favoriteStateChanged(const QString& path, bool isFavorite)`.
- Model automatically updates UI and saves to `FavoriteDao`.

### B. `AddressBar.cpp`
- Replace direct calls to `FavoriteDao::addFavorite` and `FavoriteDao::removeFavorite` with Mediator signal or `FavoritePanel` slot via `PanelMediator`.
- Listen to `favoriteStateChanged` to refresh UI if needed.

### C. `ContentContextMenu.cpp`
- Replace `findChild<FavoritePanel*>()` with Mediator standard delegation (`emit m_panel->requestToggleFavorite(paths)`).

### D. `QuickLookWindow.cpp`
- Route favorite toggle through `PanelMediator` / `requestAddFavorite`.

---

## 4. Build & Verification Steps

1. Compile the project.
2. Add a folder to favorites from the AddressBar right-click menu. Verify that the folder immediately appears in the left `FavoritePanel` tree without restarting.
3. Remove a folder from favorites via `ContentContextMenu`. Verify that it immediately disappears from `FavoritePanel`.
4. Drag a folder to `FavoritePanel`. Verify that right-click menus on that folder now correctly display "从收藏夹移除" (Remove from Favorites).
