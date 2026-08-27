# Implementation Plan - Fix Missing Header & QSet Duplicate Paths Compatibility (`duplicate-2.md`)

## 1. Overview
This implementation plan fixes MSVC/Qt compilation errors:
1. **Missing `#include <unordered_set>` / `std::hash<QString>` Operator Ambiguity**: `std::unordered_set<QString>` in MSVC fails because `std::hash<QString>` is not defined by default. Refactored `duplicatePaths` to native `QSet<QString>`, which has native hash support for `QString`.
2. **Missing `#include "../meta/DuplicateDetectorService.h"`**: `ContentPanel.cpp` was missing the include for `DuplicateDetectorService.h`, resulting in `"DuplicateDetectorService": is not a class or namespace name`.

---

## 2. Modified Files List
- `src/ui/ScanStats.h`
- `src/meta/DuplicateDetectorService.h`
- `src/meta/DuplicateDetectorService.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ScanStats.h`
```
<<<<<<< SEARCH
#include <QMap>
#include <QString>

namespace QuarkMeta {
=======
#include <QMap>
#include <QString>
#include <QSet>

namespace QuarkMeta {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    std::unordered_set<QString> duplicatePaths;
=======
    QSet<QString> duplicatePaths;
>>>>>>> REPLACE
```

### 3.2 `src/meta/DuplicateDetectorService.h`
```
<<<<<<< SEARCH
#include <unordered_set>
#include "../core/ItemRecord.h"
=======
#include <QSet>
#include "../core/ItemRecord.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    static std::unordered_set<QString> findDuplicatePaths(const std::vector<ItemRecord>& records);
=======
    static QSet<QString> findDuplicatePaths(const std::vector<ItemRecord>& records);
>>>>>>> REPLACE
```

### 3.3 `src/ui/ContentPanel.h`
```
<<<<<<< SEARCH
    void setCachedDuplicatePaths(const std::unordered_set<QString>& paths);
=======
    void setCachedDuplicatePaths(const QSet<QString>& paths);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    std::unordered_set<QString> m_cachedDuplicatePaths; // 纯内存集合，主线程 0 磁盘 I/O
=======
    QSet<QString> m_cachedDuplicatePaths; // 纯内存集合，主线程 0 磁盘 I/O
>>>>>>> REPLACE
```

### 3.4 `src/ui/ContentPanel.cpp`
Add missing `#include "../meta/DuplicateDetectorService.h"`.

---

## 4. Build & Verification Steps
1. Build code with CMake / Ninja.
2. Confirm zero MSVC compiler warnings/errors regarding `unordered_set`, `DuplicateDetectorService`, or `operator=`.
