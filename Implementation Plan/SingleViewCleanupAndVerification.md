# Implementation Plan - Step 5: Legacy Code Cleanup & Full Regression Verification

This plan covers removing obsolete dual-view containers (`SectionedScrollCanvas`, `DualSectionPanel`, `FolderSectionWidget`) and conducting comprehensive regression verification across all view modes.

## 1. Overview
With Steps 1 through 4 converting Grid/Justified, List, and Column views to single-view architectures backed by `SectionProxyModel`, the dual-view infrastructure is obsolete.
This step physically removes obsolete header and source files, cleans up `CMakeLists.txt`, removes dead members from `ContentPanel` and `ContentViewCoordinator`, and performs regression testing.

---

## 2. Modified Files List
1. Files to Delete:
   - `src/ui/SectionedScrollCanvas.h`
   - `src/ui/SectionedScrollCanvas.cpp`
   - `src/ui/DualSectionPanel.h`
   - `src/ui/DualSectionPanel.cpp`
   - `src/ui/FolderSectionWidget.h`
   - `src/ui/FolderSectionWidget.cpp`
2. `CMakeLists.txt` (Remove deleted files from `SOURCES`)
3. `src/ui/ContentPanel.h` & `src/ui/ContentPanel.cpp` (Remove `gridCanvas()`, `listCanvas()`, `folderView()` methods/members)
4. `src/ui/controllers/ContentViewCoordinator.h` & `src/ui/controllers/ContentViewCoordinator.cpp` (Remove `updateListSectionCounts` and `updateGridSectionCounts`)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
<<<<<<< SEARCH
    src/ui/DualSectionPanel.h
    src/ui/DualSectionPanel.cpp
    src/ui/SectionedScrollCanvas.h
    src/ui/SectionedScrollCanvas.cpp
    src/ui/FolderSectionWidget.cpp
    src/ui/FolderSectionWidget.h
=======
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### 4.1 Compilation
Run CMake build to verify 0 compiler errors or unresolved symbols across all targets.

### 4.2 Comprehensive Regression Testing Matrix
1. **Multi-Selection (Ctrl+A / Shift+Click / Box Selection)**:
   - Perform Ctrl+A in Grid, List, and Column views. Confirm both folders and files are selected simultaneously while section headers remain unselected.
   - Right-click after Ctrl+A: Confirm selection remains intact and context menu actions apply to all items.
2. **Section Header Interactions**:
   - Confirm section headers cannot be selected or dragged.
   - Click folder section header: Confirm folder section collapses/expands smoothly.
   - Click file section header: Confirm no action/collapse occurs.
3. **In-Place Editing & Navigation**:
   - F2 / Edit trigger on items: Confirm in-place `FileNameLineEdit` opens without popup.
   - Double-click item: Open file / navigate into folder.
   - Drag & Drop: Drag items onto folder rows / external targets.
4. **Thumbnail Scanning & Scrolling**:
   - Scroll through large directories: Verify smooth thumbnail scanning and section headers filtering in `rowsInRange`.
5. **Directory Switching & Persistence**:
   - Change directories and switch view modes: Confirm folder collapse state persists.

---

## 5. Anti-Redundancy & Architecture Self-Check
- Confirms zero dead code or orphaned dual-view files remain in `src/ui/`.
- Confirms all view modes use single-view controls (`DropJustifiedView`, `DropTreeView`, `DropListView`) wrapped by `SectionProxyModel`.
