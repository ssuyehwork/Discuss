# DuplicateConflictDialog Dead Code Removal Plan

## Overview
This plan removes the deprecated and unused `DuplicateConflictDialog` (`src/ui/DuplicateConflictDialog.h` and `src/ui/DuplicateConflictDialog.cpp`), which has been completely superseded by `FileCollisionDialog` across all file operation contexts (`ClipboardService`, `ContentKeyHandler`, `ContentFileOpsHandler`). Removing this legacy dialog eliminates code duplication and prevents maintenance confusion.

## Modified Files List
- `CMakeLists.txt` (Removed build targets for `DuplicateConflictDialog.h` and `DuplicateConflictDialog.cpp`)
- `src/ui/DuplicateConflictDialog.h` (Deleted)
- `src/ui/DuplicateConflictDialog.cpp` (Deleted)

## Detailed Line-by-Line Changes

### `CMakeLists.txt`
```diff
<<<<<<< SEARCH
    src/ui/DuplicateConflictDialog.h
    src/ui/DuplicateConflictDialog.cpp
=======
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Delete physical source files:
   - `src/ui/DuplicateConflictDialog.h`
   - `src/ui/DuplicateConflictDialog.cpp`
2. Run CMake configure and build:
   ```bash
   cmake -B build
   cmake --build build
   ```
3. Verify that no reference to `DuplicateConflictDialog` exists in the codebase and the build succeeds without warning or error.
