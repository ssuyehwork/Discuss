# DatabaseManager 与 MetadataManager 废弃 API 外部调用残留清除专项无脑实施方案 (Purge Deprecated API Legacy Calls Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案专门解决因底层废弃 API（`DatabaseManager::getDbForPath`、`DatabaseManager::getActiveMemoryDbs` 与 `MetadataManager::getManagedLibraryPath`）已从头文件中物理拔除，但外部源代码中依然残留旧调用而导致的 **13 条编译错误**：
1. **清除 `getDbForPath` 废弃调用残留（6 处）**：将 `DiskTrashService.cpp`（4 处）、`TrashRepository.cpp`（1 处）、`MetadataManager.cpp`（1 处）中残留的 `getDbForPath(...)` 精准重构替换为直连唯一全局数据库句柄 `getGlobalDb()`。
2. **清除 `getActiveMemoryDbs` 废弃调用残留（5 处）**：将 `DiskTrashService.cpp`（2 处）、`DiskTrashRepo.cpp`（1 处）、`StatisticsService.cpp`（1 处）、`TrashRepository.cpp`（1 处）中残留的 `getActiveMemoryDbs()` 遍历重构替换为查询全局库 `getGlobalDb()`。
3. **清除 `getManagedLibraryPath` 废弃调用残留（2 处）**：擦除 `CoreController.cpp`（1 处）、`SystemBootstrapper.cpp`（1 处）与 `BatchCreateDialog.cpp`（1 处）对 `getManagedLibraryPath` 的残留调用。

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
12. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/meta/DatabaseManager.h`
确保从 `DatabaseManager.h` 中清除废弃函数声明。

```
<<<<<<< SEARCH
    sqlite3* getDbForPath(const std::wstring& path);
    std::vector<sqlite3*> getActiveMemoryDbs();
=======
>>>>>>> REPLACE
```

---

### 3.2 `src/meta/DatabaseManager.cpp`
确保从 `DatabaseManager.cpp` 中删除废弃函数实现。

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
彻底清除 `DiskTrashService.cpp` 中的 4 处 `getDbForPath` 和 2 处 `getActiveMemoryDbs` 废弃调用残留。

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
彻底清除 `DiskTrashRepo.cpp` 中的 `getActiveMemoryDbs` 废弃调用残留。

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
彻底清除 `StatisticsService.cpp` 中的 `getActiveMemoryDbs` 废弃调用残留。

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
彻底清除 `TrashRepository.cpp` 中的 `getActiveMemoryDbs` 和 `getDbForPath` 废弃调用残留。

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
彻底清除 `MetadataManager` 中的 `getManagedLibraryPath` 声明与实现，并将内部 `getDbForPath` 废弃调用点替换为 `getGlobalDb()`。

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
彻底清除 `CoreController.cpp` 中的 `getManagedLibraryPath` 废弃调用残留。

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
彻底清除 `SystemBootstrapper.cpp` 中的 `getManagedLibraryPath` 废弃调用残留。

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
彻底清除 `BatchCreateDialog.cpp` 中的 `getManagedLibraryPath` 废弃调用残留。

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

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在命令行运行 CMake 编译，验证清除上述所有 13 处废弃调用残留后，全工程 **100% 一次性编译通过**，零 C2039 / C3861 错误：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **校验 Checkpoint**：
   全局搜索 `getDbForPath` / `getActiveMemoryDbs` / `getManagedLibraryPath`，确认全工程所有文件的匹配计数彻底归零（0 匹配）。
