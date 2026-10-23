# Implementation Plan - LibraryPanel-2.md

## 1. Overview
This implementation plan fixes the visual indentation mismatch between the "收藏夹" (FavoritePanel) and "库" (LibraryPanel) tabs in the left sidebar.

As shown in user screenshots:
- In "收藏夹", items are aligned closely to the left edge (`setIndentation(0)`).
- In "库", top-level category nodes have a 15px left indentation padding gap because `LibraryPanel.cpp` calls `m_treeView->setIndentation(15);`.

By setting `m_treeView->setIndentation(0);` in `LibraryPanel::initUi()`, category tree nodes in "库" will align perfectly to the left edge, achieving 100% visual consistency with "收藏夹".

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/LibraryPanel-2.md`.

## 2. Modified Files List
- `src/ui/LibraryPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/LibraryPanel.cpp`

```diff
<<<<<<< SEARCH
    m_treeView->setIndentation(15);
=======
    m_treeView->setIndentation(0);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`, open the left sidebar:
   - Switch between "收藏夹" and "库" tabs.
   - Verify that category nodes in "库" align closely to the left edge, matching the left padding and indentation of "收藏夹" perfectly.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Standard `QTreeView::setIndentation(0)` API used in alignment with `FavoritePanel.cpp`.
- **Zero Redundancy**: Modifies a single property setting for perfect UI uniformity.

## 6. Header API Signature Verification
- `LibraryPanel` class signature in `src/ui/LibraryPanel.h` remains 100% unchanged.

## 7. Header Inclusion Chain & Type Completeness Check
- No headers modified or removed.
