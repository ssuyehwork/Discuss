# CoreEngine Architecture Convergence Implementation Plan

## 1. Overview
This implementation plan addresses the critical architectural issue where business operations bypass the central brain (`CoreEngine`) and event bus (`CentralEventHub`), directly accessing DAO/Service classes in UI controllers and views. 

By expanding `AppCommandType` and `AppEventType`, routing all favorite actions, trash operations, and file metadata modifications through `CoreEngine`, and publishing granular `AppEvent` notifications, the application eliminates full-page scans (`refreshAll()`) in favor of millisecond-level incremental updates.

## 2. Modified Files List
- `src/core/CoreEngine.h`
- `src/core/CoreEngine.cpp`
- `src/core/CentralEventHub.h`
- `src/ui/controllers/ContentContextMenu.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/core/CoreEngine.h`
Add missing command types for favorites and trash management, and add private handler helper declarations.

```
<<<<<<< SEARCH
enum class AppCommandType {
    SetRating,         // 设置星级
    SetColor,          // 设置颜色标记
    SetTags,           // 设置/添加/移除标签
    AddTag,            // 批量添加标签
    RemoveTag,         // 批量移除标签
    RenameTag,         // 重命名全局标签
    RemoveGlobalTag,   // 擦除全局标签
    SetNote,           // 设置备注
    SetURL,            // 设置链接
    SetPinned,         // 置顶/取消置顶
    RenameItems,       // 重命名文件/文件夹
    DeletePermanently, // 物理删除文件
    RecordAccess       // 记录访问历史
};
=======
enum class AppCommandType {
    SetRating,         // 设置星级
    SetColor,          // 设置颜色标记
    SetTags,           // 设置/添加/移除标签
    AddTag,            // 批量添加标签
    RemoveTag,         // 批量移除标签
    RenameTag,         // 重命名全局标签
    RemoveGlobalTag,   // 擦除全局标签
    SetNote,           // 设置备注
    SetURL,            // 设置链接
    SetPinned,         // 置顶/取消置顶
    ToggleFavorite,    // 添加/取消收藏
    MoveToTrash,       // 移入回收站
    RestoreFromTrash,  // 从回收站还原
    RenameItems,       // 重命名文件/文件夹
    DeletePermanently, // 物理删除文件
    RecordAccess       // 记录访问历史
};
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void handleSetNote(const QStringList& paths, const QString& note);
    void handleSetURL(const QStringList& paths, const QString& url);
    void handleRecordAccess(const QStringList& paths);
};
=======
    void handleSetNote(const QStringList& paths, const QString& note);
    void handleSetURL(const QStringList& paths, const QString& url);
    void handleRecordAccess(const QStringList& paths);
    void handleToggleFavorite(const QStringList& paths);
};
>>>>>>> REPLACE
```

### 3.2 Update `src/core/CoreEngine.cpp`
Implement handlers for `ToggleFavorite`, dispatching database mutations via `FavoriteDao` and notifying `CentralEventHub`.

```
<<<<<<< SEARCH
#include "CoreEngine.h"
#include "../meta/MetadataManager.h"
#include "TagLexiconService.h"
=======
#include "CoreEngine.h"
#include "../meta/MetadataManager.h"
#include "../meta/FavoriteDao.h"
#include "TagLexiconService.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    case AppCommandType::RecordAccess: {
        handleRecordAccess(cmd.targetPaths);
        break;
    }
    default:
        return false;
    }
=======
    case AppCommandType::RecordAccess: {
        handleRecordAccess(cmd.targetPaths);
        break;
    }
    case AppCommandType::ToggleFavorite: {
        handleToggleFavorite(cmd.targetPaths);
        break;
    }
    default:
        return false;
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void CoreEngine::handleRecordAccess(const QStringList& paths) {
    for (const QString& path : paths) {
        MetadataManager::instance().recordAccess(path.toStdWString());
    }
}
=======
void CoreEngine::handleRecordAccess(const QStringList& paths) {
    for (const QString& path : paths) {
        MetadataManager::instance().recordAccess(path.toStdWString());
    }
}

void CoreEngine::handleToggleFavorite(const QStringList& paths) {
    if (paths.isEmpty()) return;

    bool allFav = true;
    for (const QString& p : paths) {
        if (!FavoriteDao::containsPath(p)) {
            allFav = false;
            break;
        }
    }

    for (const QString& p : paths) {
        if (allFav) {
            FavoriteDao::removeFavoritePath(p);
        } else {
            FavoriteDao::addFavoritePath(p);
        }
    }

    AppEvent ev;
    ev.type = AppEventType::FavoritesUpdated;
    ev.paths = paths;
    ev.payload["isFavorite"] = !allFav;
    CentralEventHub::instance().publishEvent(ev);
}
>>>>>>> REPLACE
```

### 3.3 Update `src/core/CentralEventHub.h`
Add `FavoritesUpdated` to `AppEventType` enum.

```
<<<<<<< SEARCH
enum class AppEventType {
    VolumeStateChanged,     // 驱动器挂载/卸载
    PathNavigated,          // 目录导航变更
    SelectionChanged,       // UI选中项变更
    MetadataUpdated,        // 元数据变动(星级/颜色/标签/备注/置顶等)
    ItemsDeleted,           // 文件物理擦除/删除
    ItemsRenamed,           // 文件批量或单项重命名
    FilterStateChanged      // 条件筛选状态变更
};
=======
enum class AppEventType {
    VolumeStateChanged,     // 驱动器挂载/卸载
    PathNavigated,          // 目录导航变更
    SelectionChanged,       // UI选中项变更
    MetadataUpdated,        // 元数据变动(星级/颜色/标签/备注/置顶等)
    FavoritesUpdated,       // 收藏夹状态变更
    ItemsDeleted,           // 文件物理擦除/删除
    ItemsRenamed,           // 文件批量或单项重命名
    FilterStateChanged      // 条件筛选状态变更
};
>>>>>>> REPLACE
```

### 3.4 Update `src/ui/controllers/ContentContextMenu.cpp`
Refactor `ActionAddToFavorites` to route through `CoreEngine::executeCommand`.

```
<<<<<<< SEARCH
        case ContentPanel::ActionAddToFavorites: {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (selectedPaths.isEmpty() && !path.isEmpty()) {
                selectedPaths << path;
            }

            if (!selectedPaths.isEmpty()) {
                bool allFav = true;
                for (const QString& p : selectedPaths) {
                    if (!FavoriteDao::containsPath(p)) {
                        allFav = false;
                        break;
                    }
                }

                if (allFav) {
                    emit m_panel->requestRemoveFavorite(selectedPaths);
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "已从收藏夹移除", 1500, QColor("#e81123"));
                } else {
                    OperationSnapshotEngine::instance().executeWithSnapshot(
                        m_panel,
                        SnapshotOperationType::ToggleFavorite,
                        selectedPaths,
                        "已成功添加至收藏夹",
                        [this, selectedPaths]() {
                            emit m_panel->requestAddFavorite(selectedPaths);
                            return true;
                        },
                        [](const QVector<AssetItemSnapshot>& beforeState) {
                            for (const auto& snap : beforeState) {
                                AppCommand cmd;
                                cmd.type = AppCommandType::SetPinned;
                                cmd.targetPaths << snap.path;
                                cmd.params["pinned"] = snap.isPinned;
                                CoreEngine::instance().executeCommand(cmd);
                            }
                            return true;
                        }
                    );
                }
            }
            break;
        }
=======
        case ContentPanel::ActionAddToFavorites: {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (selectedPaths.isEmpty() && !path.isEmpty()) {
                selectedPaths << path;
            }

            if (!selectedPaths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::ToggleFavorite;
                cmd.targetPaths = selectedPaths;
                CoreEngine::instance().executeCommand(cmd);
            }
            break;
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation**: Run CMake configure & build via Visual Studio CMake / MSVC.
2. **Behavioral Testing**: Right-click on any file item in the view and select "添加至收藏夹" / "取消收藏".
3. **Event Verification**: Confirm that `CentralEventHub` emits `AppEventType::FavoritesUpdated`, triggering incremental UI updating without calling `refreshAll()`.
