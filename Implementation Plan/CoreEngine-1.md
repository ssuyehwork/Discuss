# CoreEngine Implementation Plan

## Overview
This plan fixes the root cause of stale operations recorded in `LastOperationManager` during F4 (Repeat Last Operation) color/rating/tag operations. Previously, operations executed via `CoreEngine` (e.g. from `MetaPanel` or `PanelMediator`) updated metadata in `MetadataManager` but omitted recording the latest operation into `LastOperationManager`.

## Modified Files List
- `src/core/CoreEngine.cpp`

## Detailed Line-by-Line Changes

### `src/core/CoreEngine.cpp`

```diff
<<<<<<< SEARCH
#include "CoreEngine.h"
#include "../meta/MetadataManager.h"
=======
#include "CoreEngine.h"
#include "LastOperationManager.h"
#include "../meta/MetadataManager.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void CoreEngine::handleSetRating(const QStringList& paths, int rating) {
    for (const QString& path : paths) {
=======
void CoreEngine::handleSetRating(const QStringList& paths, int rating) {
    LastOperationManager::instance().recordSetRating(rating);
    for (const QString& path : paths) {
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void CoreEngine::handleSetColor(const QStringList& paths, const QString& color) {
    for (const QString& path : paths) {
=======
void CoreEngine::handleSetColor(const QStringList& paths, const QString& color) {
    LastOperationManager::instance().recordSetColor(color);
    for (const QString& path : paths) {
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void CoreEngine::handleSetTags(const QStringList& paths, const QStringList& tags) {
    // 🚨 铁律第一步：确保这一批标签全部已在 global.db 主词典中登记
=======
void CoreEngine::handleSetTags(const QStringList& paths, const QStringList& tags) {
    LastOperationManager::instance().recordPasteTags(tags);
    // 🚨 铁律第一步：确保这一批标签全部已在 global.db 主词典中登记
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Set an item's color to Green (e.g. `"#639922"`) via `MetaPanel` or `MetaRatingColorWidget`.
2. Select a different item in the list/grid view and press `F4`.
3. Verify that `LastOperationManager::instance().color()` returns the Green color (matching the last MetaPanel operation) rather than a stale previous color, correctly applying Green to the new selection.
