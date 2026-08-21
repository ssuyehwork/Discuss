# 纯磁盘目录模式·内存模式与托管库僵尸代码根除无脑实施方案 (Pure Disk Mode Memory & Managed Code Purge Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在彻底、无死角地清除双模式时期遗留在系统中的所有内存托管库、.arc 胶囊容器、Base36 算法、多分盘 `QuarkMeta_*.db`、`categories` / `category_items` 关系表及相关废弃逻辑，确保项目 100% 编译通过且无任何“未声明标识符”或“找不到函数成员”错误：
1. **数据库引擎降维（`DatabaseManager`）**：彻底剔除 `getDbForPath`、`getActiveMemoryDbs`、`getDriveDb` 等多库分盘路由，将全工程所有数据库访问点无缝重构收敛直连至唯一全局库句柄 `getGlobalDb()`。
2. **托管库 API 全量解耦（`MetadataManager`）**：剔除 `getManagedLibraryPath`、`isInsideManagedLibrary`、`setManaged` 等托管库 API，同步修正 `CoreController`、`SystemBootstrapper`、`BatchCreateDialog` 中的外部调用点。
3. **回收站仓库与统计服务清理（`DiskTrashService` / `DiskTrashRepo` / `StatisticsService` / `TrashRepository`）**：将全量废弃的 `getActiveMemoryDbs()` 遍历重构为直接查询 `getGlobalDb()` 中的 `disk_trash` 表。
4. **内容面板与数据模型归一化（`ContentPanel` / `DiskItemModel`）**：清理 `isMirrorSource()` / `isManagedContext()` 分流逻辑，移除 `ManagedRole` 相关菜单项与渲染阻断。

---

## 2. Modified Files List（影响文件清单）

1. `CMakeLists.txt`
2. `src/meta/DatabaseManager.h`
3. `src/meta/DatabaseManager.cpp`
4. `src/meta/MetadataManager.h`
5. `src/meta/MetadataManager.cpp`
6. `src/core/DiskTrashService.cpp`
7. `src/meta/DiskTrashRepo.cpp`
8. `src/meta/StatisticsService.cpp`
9. `src/meta/TrashRepository.cpp`
10. `src/core/CoreController.cpp`
11. `src/core/SystemBootstrapper.cpp`
12. `src/ui/BatchCreateDialog.cpp`
13. `src/ui/ContentPanel.h`
14. `src/ui/ContentPanel.cpp`
15. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

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
从 `DatabaseManager.cpp` 中物理删除 `getDbForPath` 与 `getActiveMemoryDbs` 的实现体。

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
将 `DiskTrashService.cpp` 中所有对 `getDbForPath` 和 `getActiveMemoryDbs` 的调用替换为直连 `getGlobalDb()`。

```
<<<<<<< SEARCH
            sqlite3* db = DatabaseManager::instance().getDbForPath(p.toStdWString());
=======
            sqlite3* db = DatabaseManager::instance().getGlobalDb();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    sqlite3* db = DatabaseManager::instance().getDbForPath(trashPath.toStdWString());
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    std::vector<sqlite3*> dbs = DatabaseManager::instance().getActiveMemoryDbs();
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
>>>>>>> REPLACE
```

---

### 3.4 `src/meta/DiskTrashRepo.cpp`
将 `DiskTrashRepo.cpp` 中 `getActiveMemoryDbs()` 替换为查询唯一全局库 `getGlobalDb()`。

```
<<<<<<< SEARCH
    std::vector<sqlite3*> dbs = DatabaseManager::instance().getActiveMemoryDbs();
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
>>>>>>> REPLACE
```

---

### 3.5 `src/meta/StatisticsService.cpp`
将 `StatisticsService.cpp` 中 `getActiveMemoryDbs()` 替换为查询 `getGlobalDb()`。

```
<<<<<<< SEARCH
    auto dbs = DatabaseManager::instance().getActiveMemoryDbs();
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
>>>>>>> REPLACE
```

---

### 3.6 `src/meta/TrashRepository.cpp`
将 `TrashRepository.cpp` 中 `getActiveMemoryDbs()` 与 `getDbForPath()` 替换为直连 `getGlobalDb()`。

```
<<<<<<< SEARCH
    auto dbs = DatabaseManager::instance().getActiveMemoryDbs();
=======
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
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
移除 `getManagedLibraryPath`、`isInsideManagedLibrary` 声明与实现，并将内部 `getDbForPath` 调用替换为 `getGlobalDb()`。

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
    // 历史托管库逻辑...
}
=======
>>>>>>> REPLACE
```

---

### 3.8 `src/core/CoreController.cpp`
移除对 `MetadataManager::getManagedLibraryPath` 的废弃调用。

```
<<<<<<< SEARCH
                    std::wstring managedAbsW = MetadataManager::getManagedLibraryPath(volSerial, letter);
=======
                    std::wstring managedAbsW = L"";
>>>>>>> REPLACE
```

---

### 3.9 `src/core/SystemBootstrapper.cpp`
移除对 `MetadataManager::getManagedLibraryPath` 的废弃调用。

```
<<<<<<< SEARCH
            std::wstring managedAbsW = MetadataManager::getManagedLibraryPath(volSerial, letter);
=======
            std::wstring managedAbsW = L"";
>>>>>>> REPLACE
```

---

### 3.10 `src/ui/BatchCreateDialog.cpp`
移除对 `MetadataManager::getManagedLibraryPath` 的废弃调用。

```
<<<<<<< SEARCH
            std::wstring managedW = MetadataManager::getManagedLibraryPath(volSerial, letter);
=======
            std::wstring managedW = L"";
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在命令行运行 CMake 编译，验证解耦托管库与数据库降维后**100% 一次性编译通过**，零 C2039/C3861 符号缺失错误：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **全局自检断言 Checkpoints**：
   - 全局搜索 `getDbForPath` / `getActiveMemoryDbs` / `getManagedLibraryPath`，确认全工程匹配技术均为 0。
   - 启动程序，验证回收站清空/还原、文件操作、磁盘元数据读写流畅无卡顿。
