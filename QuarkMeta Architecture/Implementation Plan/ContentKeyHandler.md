# ContentKeyHandler Implementation Plan

## Overview
This plan fixes the F4 (Repeat Last Operation) color tagging feature in `src/ui/controllers/ContentKeyHandler.cpp` and `src/ui/controllers/ContentContextMenu.cpp`. Previously, F4 set `ColorRole` on target items but omitted updating `Qt::DecorationRole` via `ShellIconManager::getFileIcon(path, 128)`, causing the view item thumbnails/icons not to repaint immediately with the newly applied color badge.

## Modified Files List
- `src/ui/controllers/ContentContextMenu.cpp`
- `src/ui/controllers/ContentKeyHandler.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/controllers/ContentContextMenu.cpp`

```diff
<<<<<<< SEARCH
#include "../../meta/FavoriteDao.h"
#include "../../crypto/EncryptionManager.h"
#include "../../core/LastOperationManager.h"
=======
#include "../../meta/FavoriteDao.h"
#include "../../crypto/EncryptionManager.h"
#include "../../core/LastOperationManager.h"
#include "../ShellIconManager.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
                    } else if (type == LastOperationType::SetColor) {
                        m_panel->getProxyModel()->setData(idx, LastOperationManager::instance().color(), ColorRole);
                    } else if (type == LastOperationType::PasteTags) {
=======
                    } else if (type == LastOperationType::SetColor) {
                        QString colorVal = LastOperationManager::instance().color();
                        m_panel->getProxyModel()->setData(idx, colorVal, ColorRole);
                        QString path = idx.data(PathRole).toString();
                        QIcon coloredIcon = ShellIconManager::getFileIcon(path, 128);
                        m_panel->getProxyModel()->setData(idx, coloredIcon, Qt::DecorationRole);
                    } else if (type == LastOperationType::PasteTags) {
>>>>>>> REPLACE
```

### 2. `src/ui/controllers/ContentKeyHandler.cpp`

```diff
<<<<<<< SEARCH
                } else if (type == LastOperationType::SetColor) {
                    m_panel->getProxyModel()->setData(targetIdx, LastOperationManager::instance().color(), ColorRole);
                } else if (type == LastOperationType::PasteTags) {
=======
                } else if (type == LastOperationType::SetColor) {
                    QString colorVal = LastOperationManager::instance().color();
                    m_panel->getProxyModel()->setData(targetIdx, colorVal, ColorRole);
                    QString path = targetIdx.data(PathRole).toString();
                    QIcon coloredIcon = ShellIconManager::getFileIcon(path, 128);
                    m_panel->getProxyModel()->setData(targetIdx, coloredIcon, Qt::DecorationRole);
                } else if (type == LastOperationType::PasteTags) {
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Perform a color tagging operation (e.g. via `Alt+1` or Color Picker).
2. Select a different file or folder item in ContentPanel.
3. Press `F4` (or trigger "Repeat Last Operation" from context menu).
4. Verify that `ColorRole` and `Qt::DecorationRole` update synchronously, immediately repainting the color badge icon on the item card/row.
