# Implementation Plan: Memory Mode & Managed Library Residual Code Purge (memory-2.md)

## Overview
This plan provides the line-by-line implementation details for purging remaining memory mode and managed library legacy code remnants in `src/ui/MainWindow.cpp`, `src/core/DiskScanService.h`, and `src/meta/MetadataManager.cpp`.

Specifically:
1. Purge unused `targetLib` variable in `MainWindow::onVolumeUnplugged` (`src/ui/MainWindow.cpp`).
2. Update obsolete comment in `src/core/DiskScanService.h`.
3. Update obsolete comment in `src/meta/MetadataManager.cpp`.

---

## Modified Files List
1. `src/ui/MainWindow.cpp`
2. `src/core/DiskScanService.h`
3. `src/meta/MetadataManager.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/MainWindow.cpp`
Remove unused legacy `targetLib` memory mode variable in `onVolumeUnplugged`.

<<<<<<< SEARCH
void MainWindow::onVolumeUnplugged(const QString& driveLetter) {
    QString targetLib = "QuarkMeta.library_" + driveLetter.toLower();

    bool isCurrentOnUnpluggedDrive = false;
=======
void MainWindow::onVolumeUnplugged(const QString& driveLetter) {
    bool isCurrentOnUnpluggedDrive = false;
>>>>>>> REPLACE

---

### 2. `src/core/DiskScanService.h`
Clean up obsolete memory mode comment in header.

<<<<<<< SEARCH
 * 绝不对 .arc 容器做任何解包翻译——这是磁盘模式与内存数据库模式 100% 隔离的物理保证。
=======
 * 绝不对 .arc 容器做任何解包翻译——确保磁盘直连浏览体验。
>>>>>>> REPLACE

---

### 3. `src/meta/MetadataManager.cpp`
Clean up obsolete memory mode comment.

<<<<<<< SEARCH
    // 1. 内存库操作 (Memory Commit)
=======
    // 1. 数据库记录操作
>>>>>>> REPLACE

---

## Build & Verification Steps
1. Ensure `memory-2.md` is present in `QuarkMeta Architecture/Implementation Plan/`.
2. Verify all modified files have valid syntax and zero compilation errors.
3. Build the project:
   `cmake -B build && cmake --build build`
