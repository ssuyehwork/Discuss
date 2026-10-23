# Implementation Plan - Fix PanelMediator Missing FavoriteService.h Include

## Overview
This plan resolves MSVC compiler errors C2653 (`'FavoriteService': is not a class or namespace name`) and C3861 (`'instance': identifier not found`) in `src/ui/PanelMediator.cpp`.
`PanelMediator.cpp` invokes `FavoriteService::instance()`, but was missing `#include "../meta/FavoriteService.h"`.
Adding `#include "../meta/FavoriteService.h"` fixes header completeness for `PanelMediator.cpp`.

---

## Modified Files List
- `src/ui/PanelMediator.cpp`

---

## Detailed Line-by-Line Changes

### 1. Update `src/ui/PanelMediator.cpp`

```diff
<<<<<<< SEARCH
#include "../util/ShellHelper.h"
#include "../meta/MetadataManager.h"
#include "UiHelper.h"
=======
#include "../util/ShellHelper.h"
#include "../meta/MetadataManager.h"
#include "../meta/FavoriteService.h"
#include "UiHelper.h"
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Recompile standard C++ build target.
2. Confirm MSVC C2653 and C3861 errors in `PanelMediator.cpp` are completely eliminated.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `FavoriteService::instance().isFavorite(...)` SSOT API.

---

## Header API Signature Verification
- `FavoriteService::instance()`: verified signature in `src/meta/FavoriteService.h`.

---

## Header Inclusion Chain & Type Completeness Check
- `src/ui/PanelMediator.cpp`: `#include "../meta/FavoriteService.h"` provides complete type definition for `FavoriteService`.
