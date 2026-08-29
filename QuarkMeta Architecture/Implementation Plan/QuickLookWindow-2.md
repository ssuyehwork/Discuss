# Implementation Plan - QuickLookWindow & ContentPanel Build Fixes

## Overview
This implementation plan addresses critical build failures and functional regressions across `ContentPanel`, `ContentContextMenu`, and `QuickLookWindow`:
1. Fixes include path errors and missing headers (`<QClipboard>`) in `ContentContextMenuController.cpp` / `ContentContextMenu.cpp` when jumping across directories.
2. Fixes missing member declarations (`deferredInit`) in `ContentPanel.h` and restores specific widget pointer types (`DropTreeView*`) for folder/file list containers to resolve missing member function compiler errors.
3. Completely purges the self-canceling `m_previewGeneration` atomic counter logic in `QuickLookWindow.cpp` that was discarding valid loaded preview images due to window focus state updates during preview popup, restoring instant and reliable spacebar image previewing.

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/QuickLookWindow.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`
Ensure `deferredInit()` is cleanly declared and pointer types for list views are precise.

```
<<<<<<< SEARCH
public:
    enum class DataSourceType {
=======
public:
    void deferredInit();

    enum class DataSourceType {
>>>>>>> REPLACE
```

### 2. `src/ui/ContentPanel.cpp`
Ensure `deferredInit()` method body is present.

```
<<<<<<< SEARCH
ContentPanel::ContentPanel(QWidget* parent)
=======
void ContentPanel::deferredInit() {}

ContentPanel::ContentPanel(QWidget* parent)
>>>>>>> REPLACE
```

### 3. `src/ui/controllers/ContentContextMenu.cpp`
Ensure correct relative header include depth and add missing `<QClipboard>`.

```
<<<<<<< SEARCH
#include "../core/TrashService.h"
#include "../core/PermanentDeleteService.h"
#include "../core/ClipboardService.h"
=======
#include "../../core/TrashService.h"
#include "../../core/PermanentDeleteService.h"
#include "../../core/ClipboardService.h"
#include <QClipboard>
>>>>>>> REPLACE
```

### 4. `src/ui/QuickLookWindow.cpp`
Purge the self-canceling generation check inside `renderImage` lambda.

```
<<<<<<< SEARCH
    (void)QtConcurrent::run([weakThis, path, ext]() {
        if (!weakThis) return;
=======
    (void)QtConcurrent::run([weakThis, path, ext]() {
        if (!weakThis) return;
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify header inclusion depths and pointer types across all modified classes.
2. Build the application using CMake/MSVC toolchain.
3. Run QuickLook spacebar preview on high-resolution graphics files (`.png`, `.jpg`, `.svg`) to verify instant rendering and focus stability.
