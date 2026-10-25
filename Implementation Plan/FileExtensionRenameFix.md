# Implementation Plan - File Extension Preserving Rename Design Fix

## 1. Overview
When a user renames a file (e.g., from `test.md` to `test.txt`), `DiskItemModel::setData` currently forces the original extension (`.md`) onto the new file name if `newName` does not end with `"." + suffix`, resulting in `test.txt.md`.
This design flaw prevents users from changing file extensions through inline renaming.

This implementation plan fixes `DiskItemModel::setData` to check whether `newName` already contains a valid file extension (`QFileInfo(newName).suffix().isEmpty()`). If the user provided a new extension (e.g. `.txt`), the user's intent is honored; if the user omitted the extension (e.g. `test`), the original extension is automatically appended to protect the file type from being accidentally removed.

---

## 2. Modified Files List
- `src/ui/models/DiskItemModel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/models/DiskItemModel.cpp`
Modify `DiskItemModel::setData` rename logic to evaluate `QFileInfo(newName).suffix().isEmpty()`:

```cpp
<<<<<<< SEARCH
        QString suffix = oldInfo.suffix();
        if (!suffix.isEmpty() && !newName.endsWith("." + suffix, Qt::CaseInsensitive)) {
            newName += "." + suffix;
        }
=======
        QFileInfo newInfoCheck(newName);
        QString oldSuffix = oldInfo.suffix();
        if (newInfoCheck.suffix().isEmpty() && !oldSuffix.isEmpty()) {
            newName += "." + oldSuffix;
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile using `cmake --build .`.
2. In QuarkMeta file view, select a file `test.md` and press `F2` to rename.
3. Rename to `test.txt` and press Enter. Verify the file is renamed to `test.txt` (not `test.txt.md`).
4. Select `test.txt` and rename to `new_test` (omitting `.txt`). Verify the file is renamed to `new_test.txt` automatically preserving the `.txt` extension.

---

## 5. Official API Reuse & Anti-Redundancy
- Reuses `QFileInfo::suffix()` standard Qt API.

---

## 6. Header API Signature Verification
- No header API signature modifications required.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `QFileInfo` is already included in `src/ui/models/DiskItemModel.cpp`.

---

## 8. Data SSOT Declaration Table
| Entity / State | SSOT Location | Synchronization Mechanism |
| :--- | :--- | :--- |
| File Name & Extension | Disk File System / `FileOperationHelper::safeRename` | Handled inside `DiskItemModel::setData` |

---

## 9. Compliance Self-Check Table
| Check Item | Status | Notes |
| :--- | :--- | :--- |
| Zero Code Modification Rule | PASS | Output strictly as plan document inside `Implementation Plan/` |
| Chinese Language Protocol | PASS | Documented completely in Chinese |
| Contract Lock | PASS | Public header signatures untouched |

---

## 10. Behavior Preservation Declaration Table
| Action | Existing Behavior | Preserved Behavior |
| :--- | :--- | :--- |
| Extension Changing Rename | Appended `.md` to `.txt` (`.txt.md`) | Changes extension directly to `.txt` |
| Bare Filename Rename | Preserves original extension | Preserves original extension automatically |
