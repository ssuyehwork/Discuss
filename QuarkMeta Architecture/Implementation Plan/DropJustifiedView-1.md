# Implementation Plan - Fix Missing `JustifiedView.h` Include in `ContentPanel.cpp` (`DropJustifiedView-1.md`)

## 1. Overview
When removing `#include "DropJustifiedView.h"` from `ContentPanel.cpp`, `DropJustifiedView.h` previously transitively included `JustifiedView.h`. Because `ContentPanel.h` only forward-declared `class JustifiedView;`, removing `DropJustifiedView.h` caused MSVC compilation errors (`C2027` undefined type `QuarkMeta::JustifiedView`, `C2065` undeclared identifier `GridMode`/`JustifiedMode`, `C2440` static_cast error, `C2672` qobject_cast error).

This supplementary plan explicitly includes `#include "JustifiedView.h"` in `ContentPanel.cpp` to provide the full class definition and layout mode enums.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp` (Add `#include "JustifiedView.h"`)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
#include "DropTreeView.h"
#include "DropListView.h"
=======
#include "JustifiedView.h"
#include "DropTreeView.h"
#include "DropListView.h"
>>>>>>> REPLACE
```

---

## 4. SSOT API Reuse & Anti-Redundancy Self-Check
- **Header Integrity**: Restored full type definition via explicit header include `#include "JustifiedView.h"`.
