# Implementation Plan - Fix DiskItemModel Compilation & Pure Virtual Contract Alignment

## Overview
Fix compilation issues caused by removing `CoreEngine.h`, `setQuery`, and missing `<QPointer>` include in `DiskItemModel.h` and `ItemModelBase.h`.

## Modified Files List
- `src/ui/models/ItemModelBase.h`
- `src/ui/models/DiskItemModel.h`

## Detailed Line-by-Line Changes

### 1. `src/ui/models/ItemModelBase.h`

#### Remove dead pure virtual function `setQuery`
<<<<<<< SEARCH
    virtual void clear() = 0;
    virtual void setQuery(const QString& query) = 0;
    virtual void updateRecordMetadata(const QString& path) = 0;
=======
    virtual void clear() = 0;
    virtual void updateRecordMetadata(const QString& path) = 0;
>>>>>>> REPLACE

### 2. `src/ui/models/DiskItemModel.h`

#### Restore `CoreEngine.h` & add `<QPointer>`
<<<<<<< SEARCH
#include <unordered_map>
#include <QSet>
=======
#include <unordered_map>
#include <QSet>
#include <QPointer>
#include "../../core/CoreEngine.h"
>>>>>>> REPLACE

## Build & Verification Steps
1. Verify `ItemModelBase.h` and `DiskItemModel.h` code changes.
2. Build or test project.
