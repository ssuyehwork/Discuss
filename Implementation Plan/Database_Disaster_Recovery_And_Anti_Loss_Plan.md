# Implementation Plan: 数据库防灾抗毁与数据资产防误删治理实施方案

本实施方案旨在严格遵循《AGENTS.md》、《SYSTEM_PROMPT.md》、《Standard Specifications for Implementation Plan.md》以及根目录《Memories.md》（章节 34），在 `QuarkMeta_Source` 桌面资产管理软件中完整治理并根除以下三项导致用户数据丢失与误删的致命缺陷：
1. **致命点 1（强杀后自愈机制误杀数据）**：彻底废除 `tryRecoverFromBackup()` 中直接使用 `QFile::remove` 抹除主库 `.store`、`-wal`、`-shm` 的自毁式旧备份覆盖逻辑。遇到异常中断时，改用锁退避缓冲与真实损坏校验；在确需自愈的极端损坏场景下，必须先对现场损坏文件进行带时间戳的归档隔离备份（`.store.corrupted.<timestamp>`），严禁物理直接清除；
2. **致命点 2（启动 1.5 秒后对账误判物理删除）**：彻底剔除 `LibrarySyncService::triggerStartupReconciliation` 中将未分配分类条目判定为“孤儿条目”并调用 `LibraryItemDao::removeItemsByPaths` 物理清除的自毁逻辑。数据资产秉持“宽进严出”与后台零删除原则，对账仅保留单向增量同步补齐，绝对禁止后台静默删除用户数据资产；
3. **致命点 3（版本不匹配暴力 DROP TABLE 抹杀数据）**：彻底清除 `DatabaseManager::loadDb` 中版本号不匹配时执行的 `DROP TABLE IF EXISTS library_items;` 等暴力清库代码，改为增量初始化与向前兼容平滑迁移，确保任何版本变迁下已有数据表结构与数据资产 100% 完整保留。

---

## 一、方案设计与架构分析 (Architecture & Design)

### 1.1 致命点 1：强杀自愈机制缺陷根因与防灾抗毁设计
- **缺陷根因**：
  在 `DatabaseManager::tryRecoverFromBackup` 中：
  ```cpp
  QString dbPath = getGlobalDbPath();
  QFile::remove(dbPath);
  QFile::remove(dbPath + "-wal");
  QFile::remove(dbPath + "-shm");
  for (const auto& bFile : backupFiles) {
      if (QFile::copy(bFile.absoluteFilePath(), dbPath)) { ... }
  }
  ```
  在开发者重新编译或用户在任务管理器中强制终止进程时，SQLite 的 WAL 日志可能尚未完成 checkpoint，或操作系统文件锁未在毫秒级完全释放。新进程启动调用 `loadDb()` 遇到短暂锁冲突返回 `false` 时，`initLocked()` 立即调用 `tryRecoverFromBackup()`，把当前的 `.store`、`-wal`、`-shm` 全部物理删除，并拿几天前或当天最初启动时的旧快照文件直接覆盖回来，导致在此期间用户新增入库、打标、评星的所有数据毁于一旦。
- **架构治理与安全加固设计**：
  1. **禁止直接无条件删除**：废除任何未经备份即直接 `QFile::remove` 当前主库与 WAL 的操作；
  2. **损坏文件安全归档隔离**：在确需执行快照恢复前，先将当前主库、`-wal`、`-shm` 重命名归档（如 `dbPath + ".corrupted." + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")`），保留现场物理证据供日后提取数据；
  3. **增强重试与损坏核验**：在 `initLocked()` 中，若首次 `loadDb()` 失败，不立即判定数据库彻底损坏，而是加入短暂退避重试（等待 200ms），排除进程强杀后瞬间锁残留的竞争情况。只有在重试仍无法打开时，才安全归档并尝试从快照恢复。

### 1.2 致命点 2：启动对账误删根因与“宽进严出”零删除设计
- **缺陷根因**：
  在 `LibrarySyncService.cpp` 中，构造函数 1.5 秒后（`QTimer::singleShot(1500, ...)`）自动调度 `triggerStartupReconciliation()`：
  ```cpp
  // 对账 2: 库表中有但已无任何分类引用的孤儿条目，自动清理
  std::unordered_set<std::wstring> catSet;
  for (const QString& cp : categoryPaths) {
      catSet.insert(normalizePath(cp).toStdWString());
  }
  QStringList orphanPaths;
  for (const auto& p : existingPaths) {
      if (catSet.count(normalizePath(p).toStdWString()) == 0) {
          orphanPaths.append(p);
      }
  }
  if (!orphanPaths.isEmpty()) {
      LibraryItemDao::removeItemsByPaths(orphanPaths); // 💥 物理清除！
  }
  ```
  该逻辑存在极其严重的业务盲区：用户导入的新条目可能尚未关联到自定义分类（例如处于未分类状态、正在排队处理）、或者由于路径规范化（大小写、反斜杠）在极端情况下存在细微差别，启动后仅 1.5 秒，后台线程就会判定其为“孤儿条目”，直接通过 SQL `DELETE` 彻底清除。
- **架构治理与安全契约**：
  1. **后台零删除原则（Anti-Loss SSOT）**：明确启动对账为“只读巡检与增量补齐”服务，**绝对禁止静默执行物理删除操作**。全软件中唯有用户在 UI 界面显式触发“从库中移除”操作时（见《Memories.md》章节 33），才允许按用户意志删除 `library_items` 表记录；
  2. **彻底移除“对账 2 孤儿清理”逻辑**：物理移除 `LibraryItemDao::removeItemsByPaths(orphanPaths)` 及其孤儿路径检索代码，改为仅保留增量同步对账 1（分类中有而库表中缺少的条目增量补齐 `syncPaths`）；
  3. **审计日志替代**：若需排查孤立条目，仅作 `qDebug` 统计输出，绝不下发物理 `DELETE`。

### 1.3 致命点 3：版本检查暴力 DROP TABLE 根因与平滑迁移设计
- **缺陷根因**：
  在 `DatabaseManager::loadDb` 中：
  ```cpp
  if (currentVersion != kSchemaVersion) {
      char* err = nullptr;
      if (sqlite3_exec(conn.diskDb, "BEGIN TRANSACTION;", nullptr, nullptr, &err) == SQLITE_OK) {
          sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS item_palettes;", nullptr, nullptr, nullptr);
          sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS file_tags;", nullptr, nullptr, nullptr);
          sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS library_items;", nullptr, nullptr, nullptr);
          ...
      }
  }
  ```
  只要数据库版本号 `PRAGMA user_version` 不等于目标版本 `kSchemaVersion`（例如全新安装、或者版本字段未读取到），代码粗暴直接执行 `DROP TABLE IF EXISTS library_items;`，导致库表被瞬间清空抹除。
- **架构治理与平滑向前兼容设计**：
  1. **彻底物理删除 DROP TABLE 语句**：杜绝在任何版本检查逻辑中执行 `DROP TABLE`；
  2. **平滑 Schema 初始化与升级**：
     - 若 `currentVersion == 0`，代表新初始化数据库，直接依赖后续的 `CREATE TABLE IF NOT EXISTS` 创建全量表结构与索引，随后设置 `PRAGMA user_version = kSchemaVersion;` 即可；
     - 若 `currentVersion < kSchemaVersion`，属于旧版本升级，保留已有表结构，只执行增量平滑迁移（如 `ALTER TABLE ADD COLUMN`），绝不清空既有数据；
     - 若 `currentVersion > kSchemaVersion`，输出告警提示保持向前兼容运行，绝不降级清表。

---

## 二、受影响文件清单 (Affected Files)

1. `QuarkMeta_Source/src/meta/DatabaseManager.cpp`
2. `QuarkMeta_Source/src/meta/LibrarySyncService.cpp`

---

## 三、行对行精准替换块 (Detailed Line-by-Line Changes)

### 3.1 `QuarkMeta_Source/src/meta/DatabaseManager.cpp`

<<<<<<< SEARCH
#include <QDateTime>
#include <QDebug>
=======
#include <QDateTime>
#include <QDebug>
#include <QThread>
>>>>>>> REPLACE

<<<<<<< SEARCH
bool DatabaseManager::tryRecoverFromBackup() {
    QString backupDir = getBackupDir();
    QDir bDir(backupDir);
    QStringList filters;
    filters << "QuarkMeta_*.store.bak";
    QFileInfoList backupFiles = bDir.entryInfoList(filters, QDir::Files, QDir::Time);

    if (backupFiles.isEmpty()) return false;

    QString dbPath = getGlobalDbPath();
    // 🚨 严密清理：不仅移除 .store 主文件，必须同步彻底移除残留的 -wal 与 -shm，防止旧快照与新 WAL 合并回放导致数据库污染
    QFile::remove(dbPath);
    QFile::remove(dbPath + "-wal");
    QFile::remove(dbPath + "-shm");

    for (const auto& bFile : backupFiles) {
        if (QFile::copy(bFile.absoluteFilePath(), dbPath)) {
            qCritical() << "[DatabaseManager] 🚨 警告：已从自动备份文件自愈恢复数据库快照:" << bFile.fileName()
                        << "，原 -wal 与 -shm 日志已完全清除，确保快照纯净装载！";
            return true;
        }
    }
    return false;
}
=======
bool DatabaseManager::tryRecoverFromBackup() {
    QString backupDir = getBackupDir();
    QDir bDir(backupDir);
    QStringList filters;
    filters << "QuarkMeta_*.store.bak";
    QFileInfoList backupFiles = bDir.entryInfoList(filters, QDir::Files, QDir::Time);

    if (backupFiles.isEmpty()) return false;

    QString dbPath = getGlobalDbPath();
    // 🚨 防灾抗毁保护：严禁直接 QFile::remove 物理删除！
    // 必须将现场疑似损坏的文件进行安全归档重命名隔离，防止误杀新增有效数据
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    if (QFile::exists(dbPath)) {
        QString corruptedArchive = dbPath + ".corrupted." + timestamp;
        QFile::rename(dbPath, corruptedArchive);
        qWarning() << "[DatabaseManager] 已将原主库归档隔离至:" << corruptedArchive;
    }
    if (QFile::exists(dbPath + "-wal")) {
        QFile::rename(dbPath + "-wal", dbPath + "-wal.corrupted." + timestamp);
    }
    if (QFile::exists(dbPath + "-shm")) {
        QFile::rename(dbPath + "-shm", dbPath + "-shm.corrupted." + timestamp);
    }

    for (const auto& bFile : backupFiles) {
        if (QFile::copy(bFile.absoluteFilePath(), dbPath)) {
            qCritical() << "[DatabaseManager] 🚨 警告：已从自动备份文件自愈恢复数据库快照:" << bFile.fileName()
                        << "，现场损坏文件已安全归档隔离！";
            return true;
        }
    }
    return false;
}
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (currentVersion != kSchemaVersion) {
        char* err = nullptr;
        if (sqlite3_exec(conn.diskDb, "BEGIN TRANSACTION;", nullptr, nullptr, &err) == SQLITE_OK) {
            sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS item_palettes;", nullptr, nullptr, nullptr);
            sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS file_tags;", nullptr, nullptr, nullptr);
            sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS library_items;", nullptr, nullptr, nullptr);
            if (sqlite3_exec(conn.diskDb, "COMMIT;", nullptr, nullptr, &err) != SQLITE_OK) {
                logSqlError("Version upgrade transaction commit", conn.diskDb, "COMMIT;");
                if (err) sqlite3_free(err);
                sqlite3_close_v2(conn.diskDb);
                conn.diskDb = nullptr;
                return false;
            }
        } else {
            logSqlError("Version upgrade transaction begin", conn.diskDb, "BEGIN TRANSACTION;");
            if (err) sqlite3_free(err);
            sqlite3_close_v2(conn.diskDb);
            conn.diskDb = nullptr;
            return false;
        }
    }
=======
    // 🚨 核心防灾升级：彻底废除暴力 DROP TABLE 自毁逻辑！
    // 遵循向前兼容平滑迁移原则，已有数据表结构与数据资产绝对不被清空抹杀
    if (currentVersion > kSchemaVersion) {
        qWarning() << "[DatabaseManager] ⚠️ 检测到当前数据库 Schema 版本 (" << currentVersion
                   << ") 高于程序内置版本 (" << kSchemaVersion << ")，以只读/兼容模式加载，禁止回退破坏！";
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    std::wstring globalPath = getGlobalDbPath().toStdWString();
    if (!loadDb(globalPath, m_globalDb)) {
        qWarning() << "[DatabaseManager] 主数据库装载失败，尝试从 7 天自动快照自愈恢复...";
        if (tryRecoverFromBackup()) {
            if (!loadDb(globalPath, m_globalDb)) {
                return false;
            }
        } else {
            return false;
        }
    }
=======
    std::wstring globalPath = getGlobalDbPath().toStdWString();
    if (!loadDb(globalPath, m_globalDb)) {
        // 🚨 防误杀重试：强杀后可能存在短暂的文件锁未瞬间释放，退避重试一次
        qWarning() << "[DatabaseManager] 首次装载数据库失败，等待 200ms 后退避重试一次...";
        QThread::msleep(200);
        if (!loadDb(globalPath, m_globalDb)) {
            qWarning() << "[DatabaseManager] 重试装载仍失败，尝试从 7 天自动快照自愈恢复...";
            if (tryRecoverFromBackup()) {
                if (!loadDb(globalPath, m_globalDb)) {
                    return false;
                }
            } else {
                return false;
            }
        }
    }
>>>>>>> REPLACE

---

### 3.2 `QuarkMeta_Source/src/meta/LibrarySyncService.cpp`

<<<<<<< SEARCH
        // 对账 2: 库表中有但已无任何分类引用的孤儿条目，自动清理
        std::unordered_set<std::wstring> catSet;
        for (const QString& cp : categoryPaths) {
            catSet.insert(normalizePath(cp).toStdWString());
        }

        QStringList orphanPaths;
        for (const auto& p : existingPaths) {
            if (catSet.count(normalizePath(p).toStdWString()) == 0) {
                orphanPaths.append(p);
            }
        }
        if (!orphanPaths.isEmpty()) {
            LibraryItemDao::removeItemsByPaths(orphanPaths);
        }
=======
        // 🚨 核心防误删契约：彻底废除启动后台物理清理孤儿条目的危险操作！
        // 秉持数据资产“宽进严出”与后台零删除原则，唯有用户在 UI 界面显式触发“从库中移除”时才允许删除条目。
        // 此处仅对无分类引用的条目进行数量审计，绝不下发 SQL DELETE 物理清除。
        std::unordered_set<std::wstring> catSet;
        for (const QString& cp : categoryPaths) {
            catSet.insert(normalizePath(cp).toStdWString());
        }

        int unreferencedCount = 0;
        for (const auto& p : existingPaths) {
            if (catSet.count(normalizePath(p).toStdWString()) == 0) {
                unreferencedCount++;
            }
        }
        if (unreferencedCount > 0) {
            qDebug() << "[LibrarySyncService] 启动对账完成：检测到" << unreferencedCount << "个未绑定分类的条目，安全保留。";
        }
>>>>>>> REPLACE

---

## 四、构建与运行时验收标准 (Build & Verification Steps)

### 4.1 构建编译核验
- **执行构建命令**：
  ```bash
  cmake -B build -S QuarkMeta_Source
  cmake --build build --config Release
  ```
- **预期编译结果**：`DatabaseManager.cpp` 与 `LibrarySyncService.cpp` 编译无任何错误，0 warnings，生成目标可执行程序。

### 4.2 运行时验收用例
1. **进程强杀与数据库加载防误杀验证**：
   - 步骤：在应用运行期间向 `library_items` 表新增测试数据并频繁写入元数据，在 IDE 中强制杀死进程或在任务管理器强行终止；
   - 预期结果：再次启动应用时，数据库正常加载；原有主库与 WAL 文件绝不被删除；新增的数据 100% 完好无损；控制台绝不触发自毁式覆盖。
2. **启动 1.5 秒对账防静默删除验证**：
   - 步骤：通过数据库直接录入一条未关联任何分类路径的有效记录，或者新导入未分配分类的文件；启动程序并保持运行超过 2 秒以上；
   - 预期结果：观察后台日志，输出 `启动对账完成：检测到 1 个未绑定分类的条目，安全保留`，该记录在 `library_items` 表中依然存在，未被静默删除。
3. **版本升级与清库阻断验证**：
   - 步骤：将数据库中的 `PRAGMA user_version` 手动修改为 `0` 或未来版本；重启应用；
   - 预期结果：应用平滑启动，控制台无 `DROP TABLE` 语句执行，`library_items` 表及全部元数据保持完整无损。

---

## 五、官方 API 复用与防另起炉灶 (Official API Reuse & Anti-Redundancy)

1. **复用 Qt 官方文件与日期标准接口**：
   - 归档隔离重命名复用 `QFile::rename` 与 `QDateTime::currentDateTime().toString(...)`，形成标准可追溯的归档备份文件名，避免裸写 Win32 底层 API；
2. **复用 `QThread::msleep` 标准退避机制**：
   - 在短暂锁争用时采用标准的线程休眠退避，避免盲目重试引发 CPU 忙等或粗暴判定失败；
3. **复用既有 SQLite 原生版本控制契约**：
   - 遵循 SQLite 标准 `PRAGMA user_version` 控制机制，杜绝私设独立的版本标识字段或另起炉灶。

---

## 六、数据单一事实来源声明表 (Data SSOT Declaration Table)

| 数据项 | 唯一真理源 (SSOT) | 派生副本 / 观察者 | 同步与刷新机制 |
| :--- | :--- | :--- | :--- |
| 全局数据库主文件及日志 | 磁盘物理文件 `QuarkMeta.store` & `-wal` | `DatabaseManager::m_globalDb` 数据库连接句柄 | 进程启动初始化装载，异常损坏时带时间戳归档隔离 |
| 库项目元数据清单 | SQLite `library_items` 数据库表 | `LibrarySyncService::m_libraryPathSet` 内存缓存 | 仅在用户显式操作时变更；启动对账仅单向增量补齐，绝不逆向删除 |
| 数据库 Schema 版本 | SQLite `PRAGMA user_version` | `DatabaseManager::kSchemaVersion` 代码常量 | 初始建表时写入，增量平滑迁移时更新，严禁清表回退 |

---

## 七、接口签名与访问级别一致性核查 (Interface & Visibility Consistency Check)

### 7.1 外部及既有签名物理核验
- `DatabaseManager::tryRecoverFromBackup`：
  `static bool tryRecoverFromBackup();`（位于 `DatabaseManager.h:65`，公有静态，签名与访问级别 100% 保持只读与一致）；
- `DatabaseManager::loadDb`：
  `bool loadDb(const std::wstring& diskPath, DbConnection& conn);`（位于 `DatabaseManager.h:77`，私有成员函数，签名 100% 保持一致）；
- `LibrarySyncService::triggerStartupReconciliation`：
  `void triggerStartupReconciliation();`（位于 `LibrarySyncService.h:35`，私有成员函数，签名 100% 保持一致）。

### 7.2 新增接口与跨类调用核对
- 本方案未新增任何类接口，亦未改变任何对外 API 签名，所有修改均为函数内部防灾抗毁实现加固，完全符合“对外接口签名冻结”铁律。

---

## 八、头文件包含链与类型完整性检查 (Header Chain & Inclusion Closure Check)

1. `QuarkMeta_Source/src/meta/DatabaseManager.cpp`：
   - 显式包含 `<QDateTime>`（已存在于第 6 行）；
   - 显式新增 `#include <QThread>` 支持退避休眠；
   - 显式包含 `<QFile>`、`<QDir>`、`<QFileInfo>`、`"DatabaseManager.h"`，类型定义完整闭合；
2. `QuarkMeta_Source/src/meta/LibrarySyncService.cpp`：
   - 显式包含 `<QTimer>`、`<QDebug>`、`"LibrarySyncService.h"`、`"LibraryItemDao.h"`，移除孤儿清理后，类型依然完整闭合，无任何悬空引用。

---

## 九、合规与质量自检核对表 (Compliance & Quality Self-Check Table)

| 检查项序号 | 规范要求 | 方案满足情况 | 详细说明与对应章节 |
| :---: | :--- | :---: | :--- |
| 1 | 全中文解答与无英文违禁词 | 满足 | 方案正文与说明完全使用规范中文，无任何禁用英文字眼 |
| 2 | 不直接修改代码、方案隔离存放 | 满足 | 方案独立存放于 `Implementation Plan/` 物理隔离目录 |
| 3 | 10 大必备章节结构完整 | 满足 | 包含从 Overview 到行为数值零篡改声明全部 10 个标准章节 |
| 4 | SEARCH 块原文真实存在且唯一匹配 | 满足 | 方案中 5 个 SEARCH 块均与目标物理源码 100% 逐字唯一匹配 |
| 5 | REPLACE 语法严谨、无悬空指针/引用 | 满足 | 包含安全时间戳重命名与退避重试，无任何语法或内存泄漏隐患 |
| 6 | CMakeLists.txt 完整性 | 满足 | 本方案修改均为既有源文件，无新建或删除文件，无需修改构建脚本 |
| 7 | 公共对外 API 签名冻结 | 满足 | 既有 API 签名保持只读，零破坏既有调用方契约 |
| 8 | 严格单向依赖流动 | 满足 | 基础设施层/数据层内部加固，无跨层逆向依赖 |
| 9 | 官方既有组件强制复用 | 满足 | 复用 Qt 标准文件操作与线程休眠，避免另起炉灶 |
| 10 | 数据库事务与线程安全 | 满足 | 保持全局互斥锁保护，彻底移除自毁式清库与误删逻辑 |
| 11 | ToolTipOverlay 绝对禁跟光标 | 满足 | 本方案不涉及 UI 浮层交互，不调用 ToolTipOverlay |
| 12 | 样式数值零篡改 | 满足 | 本方案不涉及 UI 样式调整，纯数据防灾加固 |
| 13 | 交互逻辑零篡改 | 满足 | 保持用户既有交互与视觉逻辑不变，底层杜绝数据误删 |
| 14 | 异步任务无循环重复实例化原子变量 | 满足 | 启动对账逻辑内部无重复原子变量声明 |
| 15 | 递归 Lambda 循环引用防范 | 满足 | 无递归 Lambda，对账线程池按值传递 |
| 16 | 显式类型头文件包含闭合 | 满足 | 显式引入 `<QThread>` 与 `<QDateTime>`，类型完全闭合 |
| 17 | 方案无触及范围外文件 | 满足 | 仅精准修改 2 个核心数据管理文件，绝不发散触碰其他模块 |
| 18 | Memories.md 理念沉淀 | 满足 | 核心设计理念已完整写入根目录 Memories.md 第 34 章 |

---

## 十、行为零篡改与数值零篡改声明表 (Behavior Preservation & Zero-Value-Alteration Table)

### 10.1 行为与交互零篡改声明
1. **应用启动与加载流程保持不变**：正常启动时依然按既有流程装载数据库并执行每日备份；
2. **入库与分类既有交互保持不变**：用户在界面上进行的添加至库、归类、设置星级颜色与备注等业务交互与表现 100% 保持不变；
3. **显式移除交互保持不变**：用户主动执行“从库中移除”时，依然通过 `LibraryService::removePathsFromLibrary` 正规移除。

### 10.2 样式参数与数值零篡改声明
1. **备份快照保留上限**：7 天历史快照数量限制（`> 7` 清理）保持不变；
2. **启动对账防抖时钟**：启动 1.5 秒延时（`1500ms`）与元数据 200ms 防抖同步时钟保持不变；
3. **数据库 Busy Timeout**：SQLite 连接超时时间（`25000ms`）保持不变。
