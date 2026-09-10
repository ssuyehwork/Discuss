# Implementation Plan - FilterPanel Date Sorting Fix (FilterPanelDateSortingFix.md)

## 1. Overview
This implementation plan fixes the incorrect date sorting bug in `FilterPanel`'s "创建日期" (Creation Date) and "修改日期" (Modification Date) groups.

Previously, `rebuildDateCheckboxes` sorted date strings formatted as `dd-MM-yyyy` using plain string comparisons (`a > b`), causing day values (e.g. `22-06-2019`) to sort higher than later years with smaller day numbers (e.g. `09-09-2026`).

This plan updates the sorting comparator to parse date strings with `QDate::fromString(..., "dd-MM-yyyy")` so that date checkboxes are accurately ordered by true chronological date value.

---

## 2. Modified Files List
- `src/ui/FilterPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/FilterPanel.cpp`
Include `<QDate>` and update the comparator inside `rebuildDateCheckboxes` to compare `QDate` instances.

```
<<<<<<< SEARCH
#include "FilterPanel.h"
#include "../core/AppConfig.h"
#include <QSet>
=======
#include "FilterPanel.h"
#include "../core/AppConfig.h"
#include <QSet>
#include <QDate>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QStringList dates = counts.keys();
    std::sort(dates.begin(), dates.end(), [descending](const QString& a, const QString& b) {
        return descending ? (a > b) : (a < b);
    });
=======
    QStringList dates = counts.keys();
    std::sort(dates.begin(), dates.end(), [descending](const QString& a, const QString& b) {
        QDate dateA = QDate::fromString(a, "dd-MM-yyyy");
        QDate dateB = QDate::fromString(b, "dd-MM-yyyy");
        if (dateA.isValid() && dateB.isValid()) {
            return descending ? (dateA > dateB) : (dateA < dateB);
        }
        return descending ? (a > b) : (a < b);
    });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Functional Verification**:
   - Open a folder with files created or modified across different years/months (e.g., `2019` vs `2026`).
   - Inspect the "创建日期" and "修改日期" groups in `FilterPanel`.
   - Verify that in descending order, recent dates like `09-09-2026` appear **above** older dates like `22-06-2019`.
   - Click the sort toggle button on the date group header to verify ascending vs. descending toggle sorts dates chronologically.
