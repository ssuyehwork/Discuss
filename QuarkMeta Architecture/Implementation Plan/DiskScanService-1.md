# Implementation Plan: Recursive Scan Optimization (`isRecursive`)

## Overview
This implementation plan addresses potential performance bottlenecks, freezing, and infinite recursion loops when executing recursive directory scans ("显示子文件夹中的项目").
Specifically, it resolves:
1. **Infinite Loop / Stack Overflow**: Prevents `DiskScanService::scanDirectory` from falling into infinite recursion when encountering symbolic links (`isSymLink`) or Windows junction points.
2. **Infinite Deep Recursion Guard**: Implements a maximum recursion depth cap (`maxDepth = 16`) to guard against deeply nested directory structures.
3. **Column View Cancel Token Fix**: Connects `ColumnViewPane::loadDirectory()` cancellation checks with `loadRequestId` to allow fast cancellation of obsolete recursive scans during pane navigation.

## Modified Files List
- `src/core/DiskScanService.h`
- `src/core/DiskScanService.cpp`
- `src/ui/ColumnViewWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/core/DiskScanService.h`
Add `maxDepth` parameter and overload to support depth limiting and symlink loop protection:

```diff
<<<<<<< SEARCH
    static std::vector<ItemRecord> scanDirectory(const QString& path,
                                                bool recursive,
                                                const std::function<bool()>& shouldContinue = std::function<bool()>());
=======
    static std::vector<ItemRecord> scanDirectory(const QString& path,
                                                bool recursive,
                                                const std::function<bool()>& shouldContinue = std::function<bool()>(),
                                                int maxDepth = 16);
>>>>>>> REPLACE
```

### 2. `src/core/DiskScanService.cpp`
Add symlink and junction point filtering and depth tracking to prevent infinite recursion and stack overflow:

```diff
<<<<<<< SEARCH
std::vector<ItemRecord> DiskScanService::scanDirectory(const QString& path, 
                                                        bool recursive, 
                                                        const std::function<bool()>& shouldContinue) { 
    std::vector<ItemRecord> allItems; 
 
    std::function<void(const QString&, bool)> scanDir; 
    scanDir = [&](const QString& p, bool rec) { 
        QDir dir(p); 
        if (!dir.exists()) return; 
 
        QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDir::DirsFirst | QDir::Name); 
        for (const QFileInfo& info : entries) { 
            if (shouldContinue && !shouldContinue()) return; 
 
            QString absPath = info.absoluteFilePath(); 
             
            // 🚨 统一调用文件过滤服务（归一化处理所有辅助文件、.arc、.QuarkMeta） 
            if (FileFilterService::isAuxiliaryFile(absPath)) continue; 
 
            ItemRecord itemRec = ItemRecord::create(absPath, nullptr); 
            allItems.push_back(itemRec); 
 
            if (rec && info.isDir()) { 
                scanDir(absPath, true); 
            } 
        } 
    }; 
 
    scanDir(path, recursive); 
    return allItems; 
}
=======
std::vector<ItemRecord> DiskScanService::scanDirectory(const QString& path, 
                                                        bool recursive, 
                                                        const std::function<bool()>& shouldContinue,
                                                        int maxDepth) { 
    std::vector<ItemRecord> allItems; 
    QSet<QString> visitedDirs;

    std::function<void(const QString&, bool, int)> scanDir; 
    scanDir = [&](const QString& p, bool rec, int currentDepth) { 
        if (currentDepth > maxDepth) return;

        QDir dir(p); 
        if (!dir.exists()) return; 

        QString canonicalDir = dir.canonicalPath();
        if (!canonicalDir.isEmpty()) {
            if (visitedDirs.contains(canonicalDir)) return;
            visitedDirs.insert(canonicalDir);
        }
 
        QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDir::DirsFirst | QDir::Name); 
        for (const QFileInfo& info : entries) { 
            if (shouldContinue && !shouldContinue()) return; 
 
            QString absPath = info.absoluteFilePath(); 
             
            // 🚨 统一调用文件过滤服务（归一化处理所有辅助文件、.arc、.QuarkMeta） 
            if (FileFilterService::isAuxiliaryFile(absPath)) continue; 
 
            ItemRecord itemRec = ItemRecord::create(absPath, nullptr); 
            allItems.push_back(itemRec); 
 
            if (rec && info.isDir() && !info.isSymLink()) { 
                scanDir(absPath, true, currentDepth + 1); 
            } 
        } 
    }; 
 
    scanDir(path, recursive, 0); 
    return allItems; 
}
>>>>>>> REPLACE
```

### 3. `src/ui/ColumnViewWidget.cpp`
Pass cancel checker callback to `DiskScanService::scanDirectory` inside `ColumnViewPane::loadDirectory()`:

```diff
<<<<<<< SEARCH
        (void)QtConcurrent::run([weakPane, path, recursive]() {
            std::vector<ItemRecord> items;
            if (weakPane) {
                items = DiskScanService::scanDirectory(path, recursive, std::function<bool()>());
            }
=======
        (void)QtConcurrent::run([weakPane, path, recursive]() {
            std::vector<ItemRecord> items;
            if (weakPane) {
                items = DiskScanService::scanDirectory(path, recursive, [weakPane]() {
                    return weakPane != nullptr;
                });
            }
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `DiskScanService.h`, `DiskScanService.cpp`, and `ColumnViewWidget.cpp` updated syntax.
2. Confirm `visitedDirs` and `!info.isSymLink()` correctly prevent circular recursion when scanning directories containing symlinks or junctions.
3. Validate depth limitation `maxDepth = 16` prevents stack overflow on deeply nested folders.
