# 纯磁盘目录模式·内存模式与托管库僵尸代码根除无脑实施方案 (Pure Disk Mode Memory & Managed Code Purge Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案严格遵守 `AGENTS.md` 规范第 5 节硬性标准，旨在彻底、无死角地清除双模式时期遗留在系统中的所有内存托管库、.arc 胶囊容器、Base36 算法、多分盘 `QuarkMeta_*.db`、`categories` / `category_items` 关系表及相关废弃逻辑，确保项目 **100% 编译通过且无任何“未声明标识符”或“找不到函数成员”错误**：
1. **数据库引擎降维（`DatabaseManager`）**：彻底剔除 `getDbForPath`、`getActiveMemoryDbs`、`getDriveDb` 等多库分盘路由，将全工程所有数据库访问点无缝重构收敛直连至唯一全局库句柄 `getGlobalDb()`。
2. **托管库 API 全量解耦（`MetadataManager`）**：剔除 `getManagedLibraryPath`、`isInsideManagedLibrary`、`setManaged` 等托管库 API，同步修正 `CoreController`、`SystemBootstrapper`、`BatchCreateDialog` 中的外部调用点。
3. **回收站仓库与统计服务清理（`DiskTrashService` / `DiskTrashRepo` / `StatisticsService` / `TrashRepository`）**：将全量废弃的 `getActiveMemoryDbs()` 遍历与 `getDbForPath()` 检索重构为直接查询 `getGlobalDb()`。
4. **内容面板与数据模型归一化（`ContentPanel` / `DiskItemModel`）**：清理 `isMirrorSource()` / `isManagedContext()` 分流逻辑，移除 `ManagedRole` 相关菜单项与渲染阻断。

---

## 2. Modified Files List（影响文件清单）

1. `src/meta/DatabaseManager.h`
2. `src/meta/DatabaseManager.cpp`
3. `src/meta/MetadataManager.h`
4. `src/meta/MetadataManager.cpp`
5. `src/core/DiskTrashService.cpp`
6. `src/meta/DiskTrashRepo.cpp`
7. `src/meta/StatisticsService.cpp`
8. `src/meta/TrashRepository.cpp`
9. `src/core/CoreController.cpp`
10. `src/core/SystemBootstrapper.cpp`
11. `src/ui/BatchCreateDialog.cpp`
12. `src/ui/ContentPanel.cpp`
13. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/meta/DatabaseManager.h`
从 `DatabaseManager.h` 中彻底移除 `getDbForPath` 和 `getActiveMemoryDbs` 废弃函数声明。

```
<<<<<<< SEARCH
    sqlite3* getDbForPath(const std::wstring& path); 
    std::vector<sqlite3*> getActiveMemoryDbs();
=======
>>>>>>> REPLACE
```

---

### 3.2 `src/meta/DatabaseManager.cpp`
从 `DatabaseManager.cpp` 中物理删除 `getActiveMemoryDbs` 和 `getDbForPath` 的实现体。

```
<<<<<<< SEARCH
std::vector<sqlite3*> DatabaseManager::getActiveMemoryDbs() {
    std::lock_guard<std::mutex> lock(m_driveDbMutex);
    std::vector<sqlite3*> dbs;
    for (auto& pair : m_driveDbs) {
        if (pair.second) dbs.push_back(pair.second);
    }
    return dbs;
}
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
sqlite3* DatabaseManager::getDbForPath(const std::wstring& path) { 
    std::wstring root = getDriveRoot(path); 
    if (root.empty()) return getGlobalDb(); 
    return getDriveDb(root); 
}
=======
>>>>>>> REPLACE
```

---

### 3.3 `src/core/DiskTrashService.cpp`
将 `DiskTrashService.cpp` 中所有 4 处 `getDbForPath` 和 2 处 `getActiveMemoryDbs` 调用精确替换为直连 `getGlobalDb()`。

```
<<<<<<< SEARCH
        if (QFile::rename(p, dest)) {
            sqlite3* db = DatabaseManager::instance().getDbForPath(p.toStdWString());
            if (!db) {
=======
        if (QFile::rename(p, dest)) {
            sqlite3* db = DatabaseManager::instance().getGlobalDb();
            if (!db) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            QString trashPath = DiskTrashRepo::getTrashPathForFileId(fileId);
            if (!trashPath.isEmpty()) {
                sqlite3* db = DatabaseManager::instance().getDbForPath(trashPath.toStdWString());
=======
            QString trashPath = DiskTrashRepo::getTrashPathForFileId(fileId);
            if (!trashPath.isEmpty()) {
                sqlite3* db = DatabaseManager::instance().getGlobalDb();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QString trashPath = DiskTrashRepo::getTrashPathForFileId(fileId);
    if (trashPath.isEmpty()) return false;

    sqlite3* db = DatabaseManager::instance().getDbForPath(trashPath.toStdWString());
=======
    QString trashPath = DiskTrashRepo::getTrashPathForFileId(fileId);
    if (trashPath.isEmpty()) return false;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QString trashPath = DiskTrashRepo::getTrashPathForFileId(fileId);
    if (trashPath.isEmpty()) return false;

    sqlite3* db = DatabaseManager::instance().getDbForPath(trashPath.toStdWString());
=======
    QString trashPath = DiskTrashRepo::getTrashPathForFileId(fileId);
    if (trashPath.isEmpty()) return false;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    std::vector<sqlite3*> dbs = DatabaseManager::instance().getActiveMemoryDbs();
    for (sqlite3* db : dbs) {
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (db) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    std::vector<sqlite3*> dbs = DatabaseManager::instance().getActiveMemoryDbs();
    for (sqlite3* db : dbs) {
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (db) {
>>>>>>> REPLACE
```

---

### 3.4 `src/meta/DiskTrashRepo.cpp`
将 `DiskTrashRepo.cpp` 中 `getActiveMemoryDbs()` 的调用替换为查询全局唯一 `getGlobalDb()`。

```
<<<<<<< SEARCH
    std::vector<sqlite3*> dbs = DatabaseManager::instance().getActiveMemoryDbs(); 
    for (sqlite3* db : dbs) {
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb(); 
    if (db) {
>>>>>>> REPLACE
```

---

### 3.5 `src/meta/StatisticsService.cpp`
将 `StatisticsService.cpp` 中 `getActiveMemoryDbs()` 替换为查询唯一 `getGlobalDb()`。

```
<<<<<<< SEARCH
    auto dbs = DatabaseManager::instance().getActiveMemoryDbs();
    for (sqlite3* db : dbs) {
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (db) {
>>>>>>> REPLACE
```

---

### 3.6 `src/meta/TrashRepository.cpp`
将 `TrashRepository.cpp` 中 `getActiveMemoryDbs()` 与 `getDbForPath()` 替换为直连 `getGlobalDb()`。

```
<<<<<<< SEARCH
    auto dbs = DatabaseManager::instance().getActiveMemoryDbs();
    for (sqlite3* db : dbs) {
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (db) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    sqlite3* db = DatabaseManager::instance().getDbForPath(originalPath);
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
>>>>>>> REPLACE
```

---

### 3.7 `src/meta/MetadataManager.h` & `src/meta/MetadataManager.cpp`
从 `MetadataManager.h` 和 `MetadataManager.cpp` 中清理 `getManagedLibraryPath` 的声明与实现，并将内部 `getDbForPath` 调用替换为 `getGlobalDb()`。

```
<<<<<<< SEARCH
    static std::wstring getManagedLibraryPath(const std::wstring& volSerial, const QString& driveLetter);
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    sqlite3* db = DatabaseManager::instance().getDbForPath(nPath); 
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb(); 
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
std::wstring MetadataManager::getManagedLibraryPath(const std::wstring& volSerial, const QString& driveLetter) {
    if (volSerial.empty()) return L"";
    
    // 从 QSettings / AppConfig 中读取保存的绝对路径映射 (键名如: ManagedFolder/Volume_12345678)
    QString key = QString("ManagedFolder/Volume_%1").arg(QString::fromStdWString(volSerial));
    QString savedPath = AppConfig::instance().getValue(key, "").toString();
    
    if (!savedPath.isEmpty() && QDir(savedPath).exists()) {
        return QDir::toNativeSeparators(savedPath).toStdWString();
    }
    
    // 若配置未命中但盘符非空，退避至旧版盘符根目录
    if (!driveLetter.isEmpty()) {
        QString fallback = driveLetter;
        if (!fallback.endsWith("/") && !fallback.endsWith("\\")) fallback += "/";
        return QDir::toNativeSeparators(fallback).toStdWString();
    }
    
    return L"";
}
=======
>>>>>>> REPLACE
```

---

### 3.8 `src/core/CoreController.cpp`
移除 `CoreController.cpp` 中对 `MetadataManager::getManagedLibraryPath` 的废弃调用。

```
<<<<<<< SEARCH
                if (volSerial != L"UNKNOWN") {
                    std::wstring managedAbsW = MetadataManager::getManagedLibraryPath(volSerial, letter);
                    if (!managedAbsW.empty()) {
                        // NativeFolderWatcher::instance().addWatch(managedAbsW);
                    }
                }
=======
>>>>>>> REPLACE
```

---

### 3.9 `src/core/SystemBootstrapper.cpp`
移除 `SystemBootstrapper.cpp` 中对 `MetadataManager::getManagedLibraryPath` 的废弃调用。

```
<<<<<<< SEARCH
            if (volSerial != L"UNKNOWN") {
                std::wstring managedAbsW = MetadataManager::getManagedLibraryPath(volSerial, letter);
                if (!managedAbsW.empty()) {
                    // NativeFolderWatcher::instance().addWatch(managedAbsW);
                }
            }
=======
>>>>>>> REPLACE
```

---

### 3.10 `src/ui/BatchCreateDialog.cpp`
移除 `BatchCreateDialog.cpp` 中对 `MetadataManager::getManagedLibraryPath` 的废弃调用。

```
<<<<<<< SEARCH
            std::wstring managedW = MetadataManager::getManagedLibraryPath(volSerial, letter);
            if (!managedW.empty()) {
                m_targetDir = QString::fromStdWString(managedW);
            }
=======
>>>>>>> REPLACE
```

---

### 3.11 `src/ui/ContentPanel.cpp`
移除右键菜单中对 `ManagedRole` 相关菜单项（如“重新扫描”、“取消导入并清除数据”）的渲染与控制。

```
<<<<<<< SEARCH
        // 2026-07-xx 按照 Development_Plan 2.1：始终显示“重新扫描”选项 (仅限资源库内项目)
        if (currentIndex.data(ManagedRole).toBool()) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#378ADD"), 18), "重新扫描")->setData(ActionRescan);
        }

        // 2026-07-27 按照 Plan-107：仅对已在资源库中登记的文件夹，增加“取消导入并清除数据”菜单项
        if (currentIndex.data(TypeRole).toString() == "folder" && currentIndex.data(ManagedRole).toBool()) {
            menu.addAction(UiHelper::getIcon("close", QColor("#e81123"), 18), "取消导入并清除数据")->setData(ActionCancelImport);
        }
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（分阶段自检与编译验证）

1. **编译确认**：
   在命令行运行 CMake 编译，验证全量调用点清理后**100% 一次性编译通过**，零 C2039 / C3861 / C2660 符号缺失与参数不匹配错误：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **全局自检断言 Checkpoints**：
   - 全局搜索 `getDbForPath` / `getActiveMemoryDbs` / `getManagedLibraryPath`，确认全工程匹配计数均为 0。
   - 启动程序，验证回收站清空/还原、文件物理操作与右键菜单展示正常无卡顿。
