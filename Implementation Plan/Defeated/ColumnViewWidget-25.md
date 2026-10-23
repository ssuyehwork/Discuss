# Implementation Plan - ColumnViewWidget-25.md

## Overview
This implementation plan fixes the root cause where directory metadata (ratings, tags, colors) in the rightmost column was not reflected in the Filter Panel immediately upon loading, but only appeared after an item was clicked/selected.

## Root Cause Analysis
In `ColumnViewPane::loadDirectory()`, after `DiskScanService::scanDirectory` returns raw `items` (which only contain basic disk properties and `rating = 0`), `weakSelf->m_model->setRecords(items)` is called. `DiskItemModel::setRecords` decorates `m_allRecords` inside `m_model` with `MetaCacheDecorator::decorate` (loading ratings, tags, colors, notes).

However, `ColumnViewPane::loadDirectory()` previously emitted `recordsLoaded(items)` passing the raw, undecorated `items` vector. As a result, `activeColumnRecordsChanged` received records with zero ratings/tags until `selectionChanged` was later fired by a user click, which used `m_model->allRecords()`.

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`

## Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.cpp`
In `ColumnViewPane::loadDirectory()`, change `emit weakSelf->recordsLoaded(items);` to `emit weakSelf->recordsLoaded(weakSelf->m_model->allRecords());`.

```git
<<<<<<< SEARCH
                emit weakSelf->recordsLoaded(items);
=======
                emit weakSelf->recordsLoaded(weakSelf->m_model->allRecords());
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify git diff for `src/ui/ColumnViewWidget.cpp`.
2. Confirm `recordsLoaded` emits decorated `m_model->allRecords()`.
