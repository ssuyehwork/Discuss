# Implementation Plan: 从“库”中彻底移除项目 DAO 事务与元数据同步闭环实施方案

本实施方案旨在严格遵循工业级 C++/Qt 架构规范与 `Memories.md`（第 33 章），在 `QuarkMeta_Source` 桌面应用中修复并完善：
1. **彻底解决右键菜单“从“库”中移除”失败且无法在数据库中删除的缺陷**：在 `LibraryDao` 中实现专属的 `removePathsFromLibrary` 接口，废弃错误的 `addPathsToCategory(0, paths)` 调用；
2. **打通 SQLite `library_category_paths` 分类路径表与 `library_items` 元数据表的同步清理闭环**。

---

## 一、方案设计与根因分析 (Overview & Root Cause Analysis)

### 1.1 问题根因分析
在 `QuarkMeta_Source/src/meta/LibraryService.cpp` 中：
```cpp
bool LibraryService::removePathsFromLibrary(const QStringList& paths) {
    if (paths.isEmpty()) return false;
    bool ok = LibraryDao::addPathsToCategory(0, paths); // category_id = 0 清除归属与索引
    ...
```
其作者意图通过将 `category_id` 指定为 `0` 来清除路径。然而，在 `QuarkMeta_Source/src/meta/LibraryDao.cpp` 中：
```cpp
bool LibraryDao::addPathsToCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id == 0 || paths.isEmpty()) return false; // 👈 致命守卫拦截！
```
当 `id == 0` 时，函数在第 335 行直接返回 `false`，导致操作直接被拦截，界面始终弹出“移除失败”，数据库未被做任何变更。

### 1.2 架构设计与修复原则
1. 在 `LibraryDao.h` 和 `LibraryDao.cpp` 中新增专属接口：
   ```cpp
   static bool removePathsFromLibrary(const QStringList& paths);
   ```
   内部获取全局互斥锁并开启 `SqlTransaction`，执行：
   ```sql
   DELETE FROM library_category_paths WHERE path = ?;
   ```
   在事务提交后执行 Passive Checkpoint，确保数据物理落盘；
2. 在 `LibraryService::removePathsFromLibrary` 中调用 `LibraryDao::removePathsFromLibrary(paths)`；
3. 保留后续的 `LibrarySyncService::instance().removePaths(paths)` 与 `emit libraryChanged()`，确保 `library_items` 表和前端 UI 同步刷新。

---

## 二、影响文件清单 (Modified Files List)

1. `QuarkMeta_Source/src/meta/LibraryDao.h`
2. `QuarkMeta_Source/src/meta/LibraryDao.cpp`
3. `QuarkMeta_Source/src/meta/LibraryService.cpp`

---

## 三、精准替换块 (Detailed Line-by-Line Changes)

### 3.1 `QuarkMeta_Source/src/meta/LibraryDao.h`

声明 `removePathsFromLibrary` 接口：
<<<<<<< SEARCH
    static bool addPathsToCategory(int id, const QStringList& paths);
    static bool removePathsFromCategory(int id, const QStringList& paths);
=======
    static bool addPathsToCategory(int id, const QStringList& paths);
    static bool removePathsFromCategory(int id, const QStringList& paths);
    static bool removePathsFromLibrary(const QStringList& paths);
>>>>>>> REPLACE

---

### 3.2 `QuarkMeta_Source/src/meta/LibraryDao.cpp`

实现 `removePathsFromLibrary` 接口：
<<<<<<< SEARCH
bool LibraryDao::removePathsFromCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0 || paths.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "DELETE FROM library_category_paths WHERE category_id = ? AND path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    for (const QString& p : paths) {
        QString cleanP = QDir::toNativeSeparators(QDir::cleanPath(p));
        std::string pStd = cleanP.toStdString();
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, pStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return true;
}
=======
bool LibraryDao::removePathsFromCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0 || paths.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "DELETE FROM library_category_paths WHERE category_id = ? AND path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    for (const QString& p : paths) {
        QString cleanP = QDir::toNativeSeparators(QDir::cleanPath(p));
        std::string pStd = cleanP.toStdString();
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, pStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return true;
}

bool LibraryDao::removePathsFromLibrary(const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || paths.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    SqlTransaction trans(db);

    const char* sql = "DELETE FROM library_category_paths WHERE path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    for (const QString& p : paths) {
        std::string pStd = QDir::toNativeSeparators(QDir::cleanPath(p)).toStdString();
        sqlite3_bind_text(stmt, 1, pStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    bool ok = trans.commit();
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return ok;
}
>>>>>>> REPLACE

---

### 3.3 `QuarkMeta_Source/src/meta/LibraryService.cpp`

调用新 DAO 接口：
<<<<<<< SEARCH
bool LibraryService::removePathsFromLibrary(const QStringList& paths) {
    if (paths.isEmpty()) return false;
    bool ok = LibraryDao::addPathsToCategory(0, paths); // category_id = 0 清除归属与索引
    if (ok) {
        LibrarySyncService::instance().removePaths(paths);
        emit libraryChanged();
    }
    return ok;
}
=======
bool LibraryService::removePathsFromLibrary(const QStringList& paths) {
    if (paths.isEmpty()) return false;
    bool ok = LibraryDao::removePathsFromLibrary(paths);
    if (ok) {
        LibrarySyncService::instance().removePaths(paths);
        emit libraryChanged();
    }
    return ok;
}
>>>>>>> REPLACE

---

## 四、构建与运行时验收标准 (Build & Verification Steps)

### 4.1 编译指令
```bash
cmake --build ArcMeta_Source/ArcMeta_Build/x64-release --config Release
```

### 4.2 运行时验收用例
1. **从库中移除测试**：
   - 选中一个已经在“库”中的文件；
   - 鼠标右键弹出上下文菜单，点击“从“库”中移除”；
   - 界面弹出绿色 Toast 提示：“已将 1 个项目从库中移除”；
   - 查询 SQLite `library_category_paths` 表与 `library_items` 表，该文件的记录已被彻底物理删除；
   - 左侧“库”面板中的全部数据与未分类计数即时减 1，右侧内容面板列表同步消失。

---

## 五、官方 API 复用与防另起炉灶 (Official API Reuse)
1. **事务管理**：严格复用 `SqlTransaction`。
2. **全局数据库连接**：严格复用 `DatabaseManager::instance().getGlobalDb()` 与 `getGlobalMutex()`。
3. **元数据清理**：严格复用 `LibrarySyncService::instance().removePaths(paths)`。

---

## 六、数据单一事实来源声明表 (Data SSOT Declaration Table)

| 数据项 | 唯一真理源 (SSOT) | 变更通知通道 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 分类路径关联 | SQLite `library_category_paths` 表 | `LibraryService::libraryChanged` | DB -> LibraryPanel |
| 库元数据索引 | SQLite `library_items` 表 | `LibrarySyncService::libraryItemsSynced` | DB -> SmartFolder/ContentPanel |

---

## 七、接口签名与访问级别一致性核查 (Interface Consistency Check)

1. `LibraryDao::removePathsFromLibrary(const QStringList& paths)`：`public` 静态成员函数；
2. `LibraryService::removePathsFromLibrary(const QStringList& paths)`：`public` 成员函数，签名一致。

---

## 八、头文件包含链与类型完整性检查 (Header Chain Check)

1. `QuarkMeta_Source/src/meta/LibraryDao.h`：包含 `<QStringList>`，类型完整；
2. `QuarkMeta_Source/src/meta/LibraryDao.cpp`：已包含 `DatabaseManager.h`、`SqlTransaction.h`、`<QDir>`，类型闭合；
3. `QuarkMeta_Source/src/meta/LibraryService.cpp`：已包含 `LibraryDao.h`，类型闭合。

---

## 九、合规与质量自检核对表 (Compliance Self-Check Table)

| 检查项 | 状态 | 规范与位置说明 |
| :--- | :--- | :--- |
| 1. 单一事实来源 (SSOT) 遵守 | 满足 | 库数据统一归口 LibraryDao 与 LibraryService |
| 2. 避免另起炉灶 (Search First) | 满足 | 复用既有 SqlTransaction 与 DatabaseManager |
| 3. 语言规范（全中文） | 满足 | 实施方案全文除标识符外全部采用规范中文 |
| 4. 实施方案物理隔离 | 满足 | 存放于 `Implementation Plan/` 目录下 |
| 5. 命名与映射规范 | 满足 | 命名清晰描述业务功能与主导类 |
| 6. 代码替换块 Git Merge Diff 规范 | 满足 | 采用标准 SEARCH / REPLACE 块，原代码唯一匹配 |
| 7. 接口签名冻结与只读契约 | 满足 | 既有 public 接口不破坏，新增静态方法对称完整 |
| 8. 访问级别一致性检查 | 满足 | 所有调用成员均为 public 级别 |
| 9. 内存与指针生命周期安全 | 满足 | 无悬空指针，RAII 事务与语句终结完整 |
| 10. 交互设计零篡改 | 满足 | 保持既有右键菜单形态与 Toast 反馈 |

---

## 十、行为零篡改与数值零篡改声明表 (Behavior Preservation Table)

| 既有行为/参数 | 原始设计 | 本次重构设计 | 结论 |
| :--- | :--- | :--- | :--- |
| 移除成功提示 | 绿色 Toast 提示 | 保持绿色 Toast 提示 | 100% 行为与样式一致 |
| 移除失败提示 | 红色 Toast 提示 | 保持红色 Toast 提示 | 100% 行为与样式一致 |
| 数据库清除效果 | 因入参拦截导致无法清除 | 修复为彻底清除分类与元数据表 | 修复根因缺陷 |
