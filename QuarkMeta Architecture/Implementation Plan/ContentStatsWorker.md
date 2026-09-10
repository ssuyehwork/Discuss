# Implementation Plan - FilterPanel Stats Include Folders (FilterPanelStatsFolderFix.md)

## 1. Overview
This implementation plan fixes an omission in `ContentStatsWorker::calculateStats` where metadata attributes applicable to both files and folders (specifically links, notes, and tags) were only being counted for non-directory records.

By lifting the `hasLinkCount`, `noLinkCount`, `hasNoteCount`, `noNoteCount`, `hasTagCount`, and `noTagCount` statistics calculations outside the `if (!record.isDir)` branch, folders with or without notes, links, and tags will be accurately reflected in `FilterPanel`'s count badges.

---

## 2. Modified Files List
- `src/ui/workers/ContentStatsWorker.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/workers/ContentStatsWorker.cpp`
Move the link, note, and tag presence counters so they execute for both files and directories, keeping file-specific calculations (like file extensions, image ratios, and thumbnail status) inside the `!record.isDir` branch.

```
<<<<<<< SEARCH
        if (record.isDir) {
            stats.typeCounts["folder"]++;
            if (record.isEmpty) stats.emptyFolderCount++;
        } else {
            stats.typeCounts["file"]++;
            stats.typeCounts[record.suffix.toUpper()]++;
            if (!record.url.isEmpty()) stats.hasLinkCount++; else stats.noLinkCount++;
            if (!record.note.isEmpty()) stats.hasNoteCount++; else stats.noNoteCount++;
            if (!record.tags.isEmpty()) stats.hasTagCount++; else stats.noTagCount++;

            if (record.width > 0 && record.height > 0) {
=======
        if (!record.url.isEmpty()) stats.hasLinkCount++; else stats.noLinkCount++;
        if (!record.note.isEmpty()) stats.hasNoteCount++; else stats.noNoteCount++;
        if (!record.tags.isEmpty()) stats.hasTagCount++; else stats.noTagCount++;

        if (record.isDir) {
            stats.typeCounts["folder"]++;
            if (record.isEmpty) stats.emptyFolderCount++;
        } else {
            stats.typeCounts["file"]++;
            stats.typeCounts[record.suffix.toUpper()]++;

            if (record.width > 0 && record.height > 0) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Functional Verification**:
   - Open a folder containing both directories and files.
   - Inspect the right-hand `FilterPanel` ("筛选" 面板).
   - Verify that under the "备注" (Note), "标签" (Tag), and "链接" (Link) groups, the sum of `有备注` + `无备注`, `已标签` + `未标签`, `有链接` + `无链接` equals the **total number of items** (including folders and files), rather than excluding folders.
   - Add a note or tag to a folder and verify that the corresponding `有备注` / `已标签` count dynamically increments by 1.
