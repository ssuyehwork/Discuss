# Implementation Plan - FavoritePanel-17

## Overview
Fix MSVC compilation error `C2660` ("`loadBatchAsync`: function does not take 2 arguments") in `FavoritePanel.cpp`. The signature of `ThumbnailPipelineService::loadBatchAsync` requires 3 arguments: `(const QStringList& filePaths, int targetSize, std::function<void(const QString& path, const QPixmap& pixmap)> onSingleLoaded)`. This change supplies the 3rd `onSingleLoaded` callback lambda to update the `QStandardItem` icon in `m_favoriteModel` once the micro-thumbnail is generated.

## Modified Files List
- `src/ui/FavoritePanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`
```cpp
<<<<<<< SEARCH
                    icon = ShellIconManager::getFileIcon(rec.path);
                    ThumbnailPipelineService::instance().loadBatchAsync({rec.path}, 64);
=======
                    icon = ShellIconManager::getFileIcon(rec.path);
                    ThumbnailPipelineService::instance().loadBatchAsync({rec.path}, 64, [this](const QString& path, const QPixmap& pix) {
                        if (!m_favoriteModel || pix.isNull()) return;
                        for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
                            QStandardItem* item = m_favoriteModel->item(i);
                            if (item && item->data(Qt::UserRole + 1).toString() == path) {
                                item->setIcon(QIcon(pix));
                                break;
                            }
                        }
                    });
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
                icon = ShellIconManager::getFileIcon(cleanPath);
                ThumbnailPipelineService::instance().loadBatchAsync({cleanPath}, 64);
=======
                icon = ShellIconManager::getFileIcon(cleanPath);
                ThumbnailPipelineService::instance().loadBatchAsync({cleanPath}, 64, [this](const QString& path, const QPixmap& pix) {
                    if (!m_favoriteModel || pix.isNull()) return;
                    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
                        QStandardItem* item = m_favoriteModel->item(i);
                        if (item && item->data(Qt::UserRole + 1).toString() == path) {
                            item->setIcon(QIcon(pix));
                            break;
                        }
                    }
                });
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `FavoritePanel-17.md` exists and contains Git Merge Diff blocks.
2. Verify `FavoritePanel.cpp` compiles cleanly with zero parameter count mismatch errors on `ThumbnailPipelineService::loadBatchAsync`.
