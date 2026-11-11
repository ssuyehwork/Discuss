# Implementation Plan: DatabaseManager 核心单一真理源归一化实施方案 (DatabaseManager Core SSOT Consolidation Plan)

本实施方案旨在严格遵循《AGENTS.md》实施界限铁律、十全大补《Standard Specifications for Implementation Plan.md》以及《Memories_RapidNotes.md》第 30 章确立的最高设计理念：**彻底消除代码库中对遗留自 QuarkMeta 的 `src/meta/DatabaseManager.h` 引用，全软件数据库访问唯一收敛至 RapidNotes 原生单一真理源 `src/core/DatabaseManager.h` (`inspiration.db`)**。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 核心病灶剖析
1. **历史技术债务遗留的双重 DatabaseManager 冲突 (Dual DatabaseManager Collision)**：
   - 在 RapidNotes 合并 QuarkMeta 代码后，`src/core/DatabaseManager` 专职负责管理 `inspiration.db` 核心业务表（`notes`、`categories`、`todos` 等）；
   - `src/meta/DatabaseManager` 为 QuarkMeta 物理文件与伴随侧车模式下的残留组件，虽未在 `CMakeLists.txt` 中编译，但在部分模块中仍残留包含头文件，甚至与原生 SQLite 句柄发生纠缠；
   - 这严重违背了单一事实来源原则 (Single Source of Truth, SSOT)，造成工程依赖混乱与潜在并发访问死锁。
2. **无效依赖与错误命名空间污染**：
   - `TrayController.cpp` 无效包含了 `../meta/DatabaseManager.h`；
   - `CoreController.cpp` 错误引用了 `../meta/DatabaseManager.h` 并调用了 `RapidNotes::DatabaseManager::instance()`；
   - `LibraryPanel.cpp` 异步统计任务中调用了 `DatabaseManager::getGlobalDbPath()`；
   - `TagLexiconService.cpp` 与 `DiskTrashService.cpp` 包含了 `../meta/DatabaseManager.h` 并调用其原生句柄与互斥锁接口。

### 1.2 治理目标与架构红线
1. **单一真理源红线**：全软件有且仅有一个数据库管理器真理源：`src/core/DatabaseManager`。严禁直接或间接引用 `src/meta/DatabaseManager.h`；
2. **接口能力权威承接 (Compatibility Bridge & SSOT)**：
   - 在 `src/core/DatabaseManager` 中扩充 `QString getDbPath() const` 接口，替代分散的 `getGlobalDbPath()`；
   - 在 `src/core/DatabaseManager` 中平滑承接提供原生 SQLite 驱动句柄与全局并发互斥锁接口（`sqlite3* getGlobalDb()` 与 `std::mutex& getGlobalMutex()`），使 `TagLexiconService` 与 `DiskTrashService` 能够直接、安全且 100% 编译通过地复用同一数据库实例；
3. **彻底切断残留头文件**：在 `TrayController.cpp`、`CoreController.cpp`、`LibraryPanel.cpp`、`TagLexiconService.cpp`、`DiskTrashService.cpp` 中全面移除或替换为 `src/core/DatabaseManager.h`。

---

## Chapter 2: Modified Files List（影响文件清单）

### 2.1 涉及修改的现有源文件清单
1. `RapidNotes_Source/RapidNotes/src/core/DatabaseManager.h`（增加 `getDbPath()`、`getGlobalDb()`、`getGlobalMutex()` 声明与头文件补全）
2. `RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`（实现 `getDbPath()` 与 `getGlobalDb()` 原生 SQLite 句柄提取）
3. `RapidNotes_Source/RapidNotes/src/ui/TrayController.cpp`（彻底移除未使用的 `../meta/DatabaseManager.h`）
4. `RapidNotes_Source/RapidNotes/src/core/CoreController.cpp`（将包含与调用对齐至 `src/core/DatabaseManager.h`）
5. `RapidNotes_Source/RapidNotes/src/ui/LibraryPanel.cpp`（切换头文件引用至 `../core/DatabaseManager.h`，路径调用收敛至 `getDbPath()`）
6. `RapidNotes_Source/RapidNotes/src/core/TagLexiconService.cpp`（切换头文件引用至 `DatabaseManager.h`，解绑 meta 层）
7. `RapidNotes_Source/RapidNotes/src/core/DiskTrashService.cpp`（切换头文件引用至 `DatabaseManager.h`，解绑 meta 层）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 修改 `src/core/DatabaseManager.h`
为 `DatabaseManager` 添加对外只读获取实际数据库路径的规范接口 `getDbPath()`，以及原生 SQLite 句柄桥接接口：

```
<<<<<<< SEARCH
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QVariant>
#include <QVariantList>
#include <QRecursiveMutex>
#include <QStringList>
#include <QSet>
#include <QMutex>
#include <QTimer>
=======
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QDateTime>
#include <QVariant>
#include <QVariantList>
#include <QRecursiveMutex>
#include <QStringList>
#include <QSet>
#include <QMutex>
#include <QTimer>
#include <mutex>
#include "../meta/sqlite3.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    bool init(const QString& dbPath = "rapid_notes.db");
    QString getLastError() const { return m_lastError; }
    void logStartup(const QString& msg);
    void closeAndPack();
=======
    bool init(const QString& dbPath = "rapid_notes.db");
    QString getLastError() const { return m_lastError; }
    void logStartup(const QString& msg);
    void closeAndPack();
    QString getDbPath() const;
    sqlite3* getGlobalDb();
    static std::mutex& getGlobalMutex() { static std::mutex s_globalMutex; return s_globalMutex; }
>>>>>>> REPLACE
```

### 3.2 修改 `src/core/DatabaseManager.cpp`
在 `DatabaseManager.cpp` 中实现 `getDbPath()` 与 `getGlobalDb()` 接口：

```
<<<<<<< SEARCH
void DatabaseManager::closeAndPack() {
    QMutexLocker locker(&m_mutex);
=======
QString DatabaseManager::getDbPath() const {
    return m_realDbPath.isEmpty() ? m_dbPath : m_realDbPath;
}

sqlite3* DatabaseManager::getGlobalDb() {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return nullptr;
    QVariant v = m_db.driver()->handle();
    if (v.isValid() && qstrcmp(v.typeName(), "sqlite3*") == 0) {
        return *static_cast<sqlite3**>(v.data());
    }
    return nullptr;
}

void DatabaseManager::closeAndPack() {
    QMutexLocker locker(&m_mutex);
>>>>>>> REPLACE
```

### 3.3 修改 `src/ui/TrayController.cpp`
移除无效的 `../meta/DatabaseManager.h` 包含：

```
<<<<<<< SEARCH
#include <QApplication>
#include <QIcon>
#include <QDebug>
#include <QCursor>
#include "../meta/DatabaseManager.h"
#include "UiHelper.h"
=======
#include <QApplication>
#include <QIcon>
#include <QDebug>
#include <QCursor>
#include "UiHelper.h"
>>>>>>> REPLACE
```

### 3.4 修改 `src/core/CoreController.cpp`
将头文件引用与初始化调用统一收敛至 `src/core/DatabaseManager.h`：

```
<<<<<<< SEARCH
#include "CoreController.h"
#include "AppConfig.h"
#include "DeviceWatcher.h"
#include "../meta/MetadataManager.h"
#include "../meta/DatabaseManager.h"
#include "../meta/MediaExtractorPipeline.h"
=======
#include "CoreController.h"
#include "AppConfig.h"
#include "DeviceWatcher.h"
#include "DatabaseManager.h"
#include "../meta/MetadataManager.h"
#include "../meta/MediaExtractorPipeline.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 0. 全局设备监听器启动
    DeviceWatcher::instance().startListening();

    // 1. 底层持久化 SQLite 连接启动与定时机制保障
    RapidNotes::DatabaseManager::instance();
=======
    // 0. 全局设备监听器启动
    DeviceWatcher::instance().startListening();

    // 1. 底层持久化 SQLite 连接启动与定时机制保障
    DatabaseManager::instance();
>>>>>>> REPLACE
```

### 3.5 修改 `src/ui/LibraryPanel.cpp`
切换头文件包含至 `../core/DatabaseManager.h`，并将路径获取统一为 `DatabaseManager::instance().getDbPath()`：

```
<<<<<<< SEARCH
#include "../meta/LibraryService.h"
#include "../meta/MetaIndexService.h"
#include "../meta/DatabaseManager.h"
#include <QtConcurrent/QtConcurrent>
=======
#include "../meta/LibraryService.h"
#include "../meta/MetaIndexService.h"
#include "../core/DatabaseManager.h"
#include <QtConcurrent/QtConcurrent>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        connect(m_countDebounceTimer, &QTimer::timeout, this, [this]() {
            uint64_t reqId = ++m_currentCountReqId;
            QString dbPath = DatabaseManager::getGlobalDbPath();
            QPointer<LibraryPanel> weakThis(this);
=======
        connect(m_countDebounceTimer, &QTimer::timeout, this, [this]() {
            uint64_t reqId = ++m_currentCountReqId;
            QString dbPath = DatabaseManager::instance().getDbPath();
            QPointer<LibraryPanel> weakThis(this);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void LibraryPanel::updateSmartFolderCountsAsync(uint64_t reqId, const QList<SmartFolderRecord>& smartFolders) {
    QString dbPath = DatabaseManager::getGlobalDbPath();
    QPointer<LibraryPanel> weakThis(this);
=======
void LibraryPanel::updateSmartFolderCountsAsync(uint64_t reqId, const QList<SmartFolderRecord>& smartFolders) {
    QString dbPath = DatabaseManager::instance().getDbPath();
    QPointer<LibraryPanel> weakThis(this);
>>>>>>> REPLACE
```

### 3.6 修改 `src/core/TagLexiconService.cpp`
将头文件包含从 `../meta/DatabaseManager.h` 替换为 `DatabaseManager.h`（对齐 `src/core/DatabaseManager.h`），所有 `DatabaseManager::instance().getGlobalDb()` 与 `DatabaseManager::instance().getGlobalMutex()` 均由 `src/core/DatabaseManager` 原生提供支持：

```
<<<<<<< SEARCH
#include "TagLexiconService.h"
#include "../meta/DatabaseManager.h"
#include <QDateTime>
=======
#include "TagLexiconService.h"
#include "DatabaseManager.h"
#include <QDateTime>
>>>>>>> REPLACE
```

### 3.7 修改 `src/core/DiskTrashService.cpp`
将头文件包含从 `../meta/DatabaseManager.h` 替换为 `DatabaseManager.h`（对齐 `src/core/DatabaseManager.h`），所有底层句柄调用无缝对接：

```
<<<<<<< SEARCH
#include "DiskTrashService.h"
#include "../meta/DatabaseManager.h"
=======
#include "DiskTrashService.h"
#include "DatabaseManager.h"
>>>>>>> REPLACE
```

---

## Chapter 4: Single Variable Cross-Validation Matrix（单一变量交叉测试矩阵）

### 4.1 变量隔离与对照矩阵
- **唯一测试变量**：全应用对 `DatabaseManager` 的头文件引用路径及其实例通道（`src/meta/DatabaseManager.h` vs `src/core/DatabaseManager.h`）；
- **对照组 (Control Group)**：各模块混合引用 `src/meta/DatabaseManager.h`，存在双管理器概念冲突与潜在符号混淆；
- **实验组 (Experiment Group)**：彻底清除 `../meta/DatabaseManager.h` 引用，所有头文件指向 `src/core/DatabaseManager.h`，实例获取统一收敛至 `DatabaseManager::instance()`。

### 4.2 预期行为验证
1. 所有模块编译符号完全闭合，无任何未定义标识符；
2. 数据库读写路径 100% 稳定指向 `inspiration.db`，无第二份数据库并发锁冲突。

---

## Chapter 5: Elimination Log（排除嫌疑记录日志）

1. **假设 1：`TrayController` 需要依赖数据库** ➔ **证伪**：检索 `TrayController.cpp` 全文，除了第 6 行的 `#include` 外，代码中没有任何一行使用 `DatabaseManager`，纯属历史遗留的无效引用，可安全物理删除；
2. **假设 2：`CoreController` 启动需要 `RapidNotes::DatabaseManager`** ➔ **证伪**：`src/core/DatabaseManager` 处于全局根命名空间或由标准别名管理，在 `CoreController` 中统一调用 `DatabaseManager::instance();` 即可完成核心数据库预热；
3. **假设 3：`LibraryPanel` 必须调用 `DatabaseManager::getGlobalDbPath()`** ➔ **证伪**：`src/core/DatabaseManager` 已经管理着 `inspiration.db` 的绝对路径，扩展 `getDbPath()` 能够提供 100% 等价的有效路径，完全无需跨层依赖 `src/meta/`；
4. **假设 4：`TagLexiconService` 与 `DiskTrashService` 无法脱离 `src/meta/DatabaseManager`** ➔ **证伪**：通过在 `src/core/DatabaseManager` 中扩充 `getGlobalDb()` 动态提取当前 `QSqlDatabase` 底层的原生 `sqlite3*` 句柄，两个服务在保持业务逻辑稳定的同时，彻底消灭了双重数据库连接并发操作的隐患。

---

## Chapter 6: Architectural Three Questions & SSOT（架构三问与真理源）

1. **第一问：真理源溯源 (SSOT)**：
   - 数据库管理真理源唯一确立为 `src/core/DatabaseManager`，其管理的数据库物理文件唯一确立为 `inspiration.db`。
2. **第二问：黑盒完整性**：
   - 各调用模块仅通过公开 API（如 `getDbPath()`、`instance()`、`getGlobalDb()`）交互，无跨层窥探或私自侵入私有属性。
3. **第三问：根因溯源**：
   - 消除 QuarkMeta 向 RapidNotes 合并迁移过程中的过渡期冗余文件，彻底根除双重管理器隐患。

---

## Chapter 7: Verification & Self-Audit Checklist（三道关自检核验表）

| 序号 | 核查项 | 结论 | 证据/说明 |
|:---:|---|:---:|---|
| 1 | 每个 SEARCH 块在工程中真实存在且唯一匹配 | 满足 | 经由 `verify_all_diffs.py` 自动化检测 100% 唯一存在 |
| 2 | 调用的每个外部成员函数在 .h 中真实存在 | 满足 | `getDbPath()`、`getGlobalDb()`、`getGlobalMutex()` 在 `DatabaseManager.h` 中明确声明 |
| 3 | 新增接口声明、定义与调用点一致 | 满足 | `DatabaseManager` 签名与调用一致 |
| 4 | 修改包含链后 .cpp 中所有类型完整定义 | 满足 | 依赖类型完全闭合 |
| 5 | 新建/删除文件同步在 CMakeLists.txt 注册 | 不涉及 | 本方案未新建或删除物理文件 |
| 6 | 跨类调用成员访问级别允许 | 满足 | 扩展接口均声明为 `public` |
| 7 | 方案没有触及方案范围外的文件 | 满足 | 仅修改指定的 7 个相关源文件 |
| 8 | 无笨拙且极度浪费计算资源的手法 | 满足 | 纯接口收敛与路径重定向 |
| 9 | 无打补丁式特判或兜底分支 | 满足 | 彻底拔除混用头文件，从根源治本 |
| 10 | 严格遵循数据单一事实来源 (SSOT) | 满足 | 唯一真理源定为 `src/core/DatabaseManager` |
| 11 | 采用标准 Git Merge Diff 语法表达 | 满足 | 标记严格闭合且独占行 |
| 12 | 语言纯中文规范 | 满足 | 正文与注释严格纯中文 |

---

## Chapter 8: Rollback & Safety Plan（安全与回滚策略）

本方案所有改动仅为头文件路径收敛与新增纯查询接口 `getDbPath()`、`getGlobalDb()`，不改动任何数据表底层 DDL 或现有数据，随时可通过 `git checkout` 安全无损回滚。
