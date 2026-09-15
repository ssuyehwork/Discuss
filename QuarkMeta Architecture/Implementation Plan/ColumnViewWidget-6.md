# Implementation Plan - ColumnViewWidget-6.md

## 1. Overview
In QuarkMeta's Column View mode (`ColumnViewWidget`), changing sort criteria via the status bar sort button or context menu appears ineffective.

### Root Causes:
1. **Missing Mediator Link in ContentPanel**: `ContentSortController::sortCriteriaChanged` in `ContentPanel.cpp` only calls `m_sortController->applySortToModel(m_proxyModel)`, but does not pass the new sort criteria to `m_columnView->applySort(...)`.
2. **Missing Re-Sort After Async Load**: In `ColumnViewPane::loadDirectory()`, when async disk scanning completes and sets records into `m_model`, `FilterProxyModel` is reset without re-enforcing the current `m_sortType` and `m_sortOrder`.

This implementation plan fixes both gaps to ensure Column View columns correctly apply and maintain sorting when changed or loaded.

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`
- `src/ui/ColumnViewWidget.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ContentPanel.cpp`
In `ContentPanel::ContentPanel`: Ensure `m_sortController`'s `sortCriteriaChanged` signal also forwards sort criteria to `m_columnView`.

```
<<<<<<< SEARCH
    m_sortController = new ContentSortController(this);
    connect(m_sortController, &ContentSortController::sortCriteriaChanged, this, [this](SortType, Qt::SortOrder) {
        m_sortController->applySortToModel(m_proxyModel);
    });
    m_sortController->applySortToModel(m_proxyModel);
=======
    m_sortController = new ContentSortController(this);
    connect(m_sortController, &ContentSortController::sortCriteriaChanged, this, [this](SortType type, Qt::SortOrder order) {
        m_sortController->applySortToModel(m_proxyModel);
        if (m_columnView) {
            m_columnView->applySort(static_cast<int>(type), order);
        }
    });
    m_sortController->applySortToModel(m_proxyModel);
>>>>>>> REPLACE
```

### `src/ui/ColumnViewWidget.cpp`
In `ColumnViewPane::loadDirectory()`: Re-apply current sort settings to `m_proxyModel` after async directory records are set.

```
<<<<<<< SEARCH
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakSelf, items]() {
            if (weakSelf && weakSelf->m_model) {
                weakSelf->m_model->setRecords(items);
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
=======
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakSelf, items]() {
            if (weakSelf && weakSelf->m_model) {
                weakSelf->m_model->setRecords(items);
                if (weakSelf->m_contentPanel && weakSelf->m_proxyModel) {
                    weakSelf->m_proxyModel->setSortType(static_cast<int>(weakSelf->m_contentPanel->currentSortType()));
                    weakSelf->m_proxyModel->sort(0, weakSelf->m_contentPanel->currentSortOrder());
                }
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation Check**:
   Run `cmake --build build` to verify clean compilation.
2. **Behavioral Verification**:
   - Open QuarkMeta and switch to Column View mode.
   - Click the status bar sort direction toggle or change sort type via context menu (e.g. Sort by Rating or Sort by Modify Date).
   - Confirm that all columns in Column View update their row order accordingly.
   - Expand new subfolders and verify that the newly created sub-columns automatically load with the active sort order applied.
