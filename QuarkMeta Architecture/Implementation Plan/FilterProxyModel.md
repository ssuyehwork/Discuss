# Implementation Plan - FilterProxyModel Date Sorting Normalization (FilterProxyModel.md)

## 1. Overview
This implementation plan normalizes the date sorting behavior in `FilterProxyModel` (used by main item views when sorting via right-click context menu) to align with `FilterPanelDateSortingFix.md`.

Currently, `FilterProxyModel::lessThan` compares raw `ctime` (creation time) and `mtime` (modification time) timestamps directly at the millisecond level. However, when filtering by creation/modification dates, `FilterProxyModel` and `FilterPanel` format records into `dd-MM-yyyy` calendar dates (`QDateTime::fromMSecsSinceEpoch(ms).date()`).

To align with `FilterPanelDateSortingFix.md` and eliminate sorting discrepancies between the main list view and the `FilterPanel` count badges:
- Primary Comparison: Compare the `QDate` calendar date (`QDateTime::fromMSecsSinceEpoch(ms).date()`).
- Secondary Tie-Breaker: When calendar dates are identical, fall back to millisecond timestamp (`ctime`/`mtime`), followed by `localeAwareCompare` filename comparison.

## 2. Modified Files List
- `src/ui/models/FilterProxyModel.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/models/FilterProxyModel.cpp`

```
<<<<<<< SEARCH
        case 1:
            if (leftRec.ctime != rightRec.ctime) return leftRec.ctime < rightRec.ctime;
            return compareNames(leftRec, rightRec);
        case 2:
            if (leftRec.mtime != rightRec.mtime) return leftRec.mtime < rightRec.mtime;
            return compareNames(leftRec, rightRec);
=======
        case 1: {
            QDate dateL = QDateTime::fromMSecsSinceEpoch(leftRec.ctime).date();
            QDate dateR = QDateTime::fromMSecsSinceEpoch(rightRec.ctime).date();
            if (dateL != dateR) return dateL < dateR;
            if (leftRec.ctime != rightRec.ctime) return leftRec.ctime < rightRec.ctime;
            return compareNames(leftRec, rightRec);
        }
        case 2: {
            QDate dateL = QDateTime::fromMSecsSinceEpoch(leftRec.mtime).date();
            QDate dateR = QDateTime::fromMSecsSinceEpoch(rightRec.mtime).date();
            if (dateL != dateR) return dateL < dateR;
            if (leftRec.mtime != rightRec.mtime) return leftRec.mtime < rightRec.mtime;
            return compareNames(leftRec, rightRec);
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps

1. **Build Verification**:
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```

2. **Functional Verification**:
   - Right-click in the main content panel, select "排序方式" -> "创建日期" or "修改日期".
   - Verify that items are sorted chronologically by calendar date (`QDate`), aligned with the `dd-MM-yyyy` date grouping in `FilterPanel`.
   - Verify that items created/modified on the exact same day are deterministically ordered by millisecond timestamp and filename.
