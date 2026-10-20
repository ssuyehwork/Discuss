# Implementation Plan - Database Schema Refactoring Iteration 1 (MSVC C4505 Dead Function Cleanup)

## Pre-Draft Evidence (Gate 1)

### 1. Read Files List
- `src/meta/SmartFolderDao.h` (Read using `read_file`)
- `src/meta/SmartFolderDao.cpp` (Read using `run_in_bash_session`)
- `AGENTS.md` Section 3.1 & Section 3.7 (Read using `read_file`)

### 2. Header Method Signatures Excerpts
From `src/meta/SmartFolderDao.h`:
```cpp
class SmartFolderDao {
public:
    static bool initTable();
    static QList<SmartFolderRecord> getAllSmartFolders();
    static SmartFolderRecord getSmartFolderById(int id);
    static int addSmartFolder(const QString& name, const QString& matchMode, const QString& iconKey, const QString& colorHex, const QList<SmartFolderRule>& rules);
    static bool updateSmartFolder(int id, const QString& name, const QString& matchMode, const QString& iconKey, const QString& colorHex, const QList<SmartFolderRule>& rules);
    static bool removeSmartFolder(int id);
    static int countMatchingItems(const SmartFolderRecord& rec);
    static QList<QString> getMatchingItemPaths(const SmartFolderRecord& rec, int limit = 1000);
};
```

### 3. SSOT Search Log
- Searched `evaluateRuleForRuntimeMeta` and `evaluateRuleForItemMeta` references across `src/meta/`:
  - `SmartFolderDao.cpp`: Defined at lines 220 and 270 as `static bool` helper functions.
  - Callers: No internal or external callers remain after SQL-based filtering replaced in-memory evaluation.
- Result: Unreferenced static helper functions trigger MSVC warning C4505.

### 4. Memories.md Key Rules Alignment
- **AGENTS.md Section 3.1 & 3.7**: Submitted implementation plans are read-only. Iterative fixes for compilation errors/warnings must produce a new plan with an incremental suffix (`DatabaseSchemaRefactoring-1.md`).
- **AGENTS.md Section 2.4-3**: Obsolete/dead code must be physically purged when refactoring.

---

## 1. Overview
### 1.1 Problem Statement & Root Cause Attribution
During MSVC compilation of `SmartFolderDao.cpp`, MSVC emitted compiler warning C4505:
- `warning C4505: "QuarkMeta::evaluateRuleForRuntimeMeta": unreferenced local function has been removed`
- `warning C4505: "QuarkMeta::evaluateRuleForItemMeta": unreferenced local function has been removed`

**Root Cause Attribution**: Previously, Smart Folder rule matching was performed in-memory using `evaluateRuleForRuntimeMeta` and `evaluateRuleForItemMeta`. When `SmartFolderDao` was refactored to perform direct SQL queries (`countMatchingItems` / `getMatchingItemPaths`) over `library_items` and `file_tags`, these two static functions became unreferenced dead code.

### 1.2 Iteration Goal
1. Physically remove `evaluateRuleForRuntimeMeta` and `evaluateRuleForItemMeta` static helper functions in `SmartFolderDao.cpp`.
2. Fully eliminate MSVC C4505 compiler warnings and satisfy zero-dead-code requirements.

---

## 2. Modified Files List
- `src/meta/SmartFolderDao.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/meta/SmartFolderDao.cpp`

```cpp
<<<<<<< SEARCH
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

static bool evaluateRuleForRuntimeMeta(const SmartFolderRule& rule,
                                       const QString& filePath,
                                       const QString& fileName,
                                       const QString& suffix,
                                       const RuntimeMeta& meta) {
    if (rule.type == SmartRuleType::Rating) {
        int rVal = rule.value.toInt();
        if (rule.op == SmartRuleOperator::GreaterOrEqual) return (meta.rating >= rVal);
        if (rule.op == SmartRuleOperator::Equals) return (meta.rating == rVal);
        if (rule.op == SmartRuleOperator::LessOrEqual) return (meta.rating <= rVal);
        if (rule.op == SmartRuleOperator::Contains) return (meta.rating == rVal);
        if (rule.op == SmartRuleOperator::NotContains) return (meta.rating != rVal);
    } else if (rule.type == SmartRuleType::Extension) {
        QString targetExt = rule.value.trimmed();
        if (targetExt.startsWith('.')) targetExt.remove(0, 1);
        bool equals = (suffix.compare(targetExt, Qt::CaseInsensitive) == 0) ||
                      (fileName.endsWith("." + targetExt, Qt::CaseInsensitive));
        if (rule.op == SmartRuleOperator::NotContains) return !equals;
        return equals;
    } else if (rule.type == SmartRuleType::Color) {
        QString itemColor = QString::fromStdWString(meta.manualColor);
        if (itemColor.isEmpty()) itemColor = QString::fromStdWString(meta.autoColor);
        bool matchColor = (itemColor.compare(rule.value.trimmed(), Qt::CaseInsensitive) == 0);
        if (rule.op == SmartRuleOperator::NotContains) return !matchColor;
        return matchColor;
    } else if (rule.type == SmartRuleType::Tag) {
        bool hasTag = false;
        QString valTrimmed = rule.value.trimmed();
        for (const QString& t : meta.tags) {
            if (t.compare(valTrimmed, Qt::CaseInsensitive) == 0) {
                hasTag = true;
                break;
            }
        }
        if (rule.op == SmartRuleOperator::NotContains) return !hasTag;
        return hasTag;
    } else if (rule.type == SmartRuleType::Keyword) {
        QString note = QString::fromStdWString(meta.note);
        QString url = QString::fromStdWString(meta.url);
        QString kw = rule.value.trimmed();
        bool matchKw = fileName.contains(kw, Qt::CaseInsensitive) ||
                       filePath.contains(kw, Qt::CaseInsensitive) ||
                       note.contains(kw, Qt::CaseInsensitive) ||
                       url.contains(kw, Qt::CaseInsensitive);
        if (rule.op == SmartRuleOperator::NotContains) return !matchKw;
        return matchKw;
    }
    return false;
}

static bool evaluateRuleForItemMeta(const SmartFolderRule& rule,
                                    const QString& filePath,
                                    const QString& fileName,
                                    const QString& suffix,
                                    const ItemMeta& meta) {
    if (rule.type == SmartRuleType::Rating) {
        int rVal = rule.value.toInt();
        if (rule.op == SmartRuleOperator::GreaterOrEqual) return (meta.rating >= rVal);
        if (rule.op == SmartRuleOperator::Equals) return (meta.rating == rVal);
        if (rule.op == SmartRuleOperator::LessOrEqual) return (meta.rating <= rVal);
        if (rule.op == SmartRuleOperator::Contains) return (meta.rating == rVal);
        if (rule.op == SmartRuleOperator::NotContains) return (meta.rating != rVal);
    } else if (rule.type == SmartRuleType::Extension) {
        QString targetExt = rule.value.trimmed();
        if (targetExt.startsWith('.')) targetExt.remove(0, 1);
        bool equals = (suffix.compare(targetExt, Qt::CaseInsensitive) == 0) ||
                      (fileName.endsWith("." + targetExt, Qt::CaseInsensitive));
        if (rule.op == SmartRuleOperator::NotContains) return !equals;
        return equals;
    } else if (rule.type == SmartRuleType::Color) {
        QString itemColor = QString::fromStdWString(meta.color);
        if (itemColor.isEmpty()) itemColor = QString::fromStdWString(meta.autoColor);
        bool matchColor = (itemColor.compare(rule.value.trimmed(), Qt::CaseInsensitive) == 0);
        if (rule.op == SmartRuleOperator::NotContains) return !matchColor;
        return matchColor;
    } else if (rule.type == SmartRuleType::Tag) {
        bool hasTag = false;
        QString valTrimmed = rule.value.trimmed();
        for (const auto& t : meta.tags) {
            if (QString::fromStdWString(t).compare(valTrimmed, Qt::CaseInsensitive) == 0) {
                hasTag = true;
                break;
            }
        }
        if (rule.op == SmartRuleOperator::NotContains) return !hasTag;
        return hasTag;
    } else if (rule.type == SmartRuleType::Keyword) {
        QString note = QString::fromStdWString(meta.note);
        QString url = QString::fromStdWString(meta.url);
        QString kw = rule.value.trimmed();
        bool matchKw = fileName.contains(kw, Qt::CaseInsensitive) ||
                       filePath.contains(kw, Qt::CaseInsensitive) ||
                       note.contains(kw, Qt::CaseInsensitive) ||
                       url.contains(kw, Qt::CaseInsensitive);
        if (rule.op == SmartRuleOperator::NotContains) return !matchKw;
        return matchKw;
    }
    return false;
}

int SmartFolderDao::countMatchingItems(const SmartFolderRecord& rec) {
=======
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

int SmartFolderDao::countMatchingItems(const SmartFolderRecord& rec) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile `SmartFolderDao.cpp` under MSVC.
2. Confirm that MSVC warning C4505 for `evaluateRuleForRuntimeMeta` and `evaluateRuleForItemMeta` is completely eliminated.
3. Verify that `SmartFolderDao::countMatchingItems` and `getMatchingItemPaths` function without regression.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Dead Code Cleanup**: Purging unreferenced static functions directly enforces single SSOT query handling via SQL statements.

---

## 6. Header API Signature Verification

| Calling Class | Method Called | Header Declaration Location | Verified Signature |
|---|---|---|---|
| External Caller | `SmartFolderDao::countMatchingItems` | `src/meta/SmartFolderDao.h:32` | `static int countMatchingItems(const SmartFolderRecord& rec);` |
| External Caller | `SmartFolderDao::getMatchingItemPaths` | `src/meta/SmartFolderDao.h:33` | `static QList<QString> getMatchingItemPaths(const SmartFolderRecord& rec, int limit = 1000);` |

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/meta/SmartFolderDao.cpp` includes `"SmartFolderDao.h"`, `"DatabaseManager.h"`, and `<sqlite3.h>`.
- Removing unreferenced static helpers does not alter header inclusion or type dependencies.

---

## 8. Compliance Self-Check Table

| Item | Source Clause | Status | Location in Plan |
|---|---|---|---|
| Filename mapping 1:1 with core class/module | 3.1-2 | Compliant | `DatabaseSchemaRefactoring-1.md` |
| Isolated in physical plan directory | 3.1-1 | Compliant | `QuarkMeta Architecture/Implementation Plan/` |
| All SEARCH blocks exist in codebase | 3.2 | Compliant | Section 3 |
| CMake alignment for Q_OBJECT classes | 3.3 | Not Applicable | No new classes added |
| Chapters 1~9 complete | 3.4 | Compliant | Sections 1~9 |
| External member calls matched with headers | 3.5-1/2 | Compliant | Section 6 |
| Header inclusion chain verified | 3.5-3 | Compliant | Section 7 |
| Reused SSOT entry points | 2.4 | Compliant | Section 5 |
| No inline setStyleSheet | 4.1 | Compliant | Section 1 |
| No platform hacks | 4.2 | Compliant | Section 1 |
| Aggregated broadcast for batch operations | 4.3 | Not Applicable | Dead code purge |
| Zero value alteration | 4.4 | Compliant | Section 9 |
| Preserved user interaction design | 1.2 / Memories.md | Compliant | Section 1 |
| No files modified outside scope | 5.3 | Compliant | Section 2 |

---

## 9. Zero-Value-Alteration Check
- **Not Applicable**: This iteration purges dead static helper functions only. No UI numerical parameters are changed.
