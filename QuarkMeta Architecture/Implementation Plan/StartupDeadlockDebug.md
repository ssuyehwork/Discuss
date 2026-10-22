# StartupDeadlockDebug — 启动死锁追踪临时调试日志方案

> ⚠️ **纯临时调试用途**：定位卡死位置后必须还原所有改动。  
> 适用症状：双击后 QuarkMeta.exe 进程长驻任务管理器但无任何窗口出现。

---

## 第 1 关：起草前核查（Pre-Draft Evidence）

### 已读文件清单

| 文件 | 物理读取 |
|---|---|
| `src/main.cpp` | ✅ 已读（167 行） |
| `src/core/CoreController.cpp` | ✅ 已读（163 行） |
| `src/meta/DatabaseManager.cpp` | ✅ 已读（281 行） |
| `src/meta/DriveMetaDao.cpp` | ✅ 已读（117 行） |

### 签名原文摘录（方案中调用的每个外部函数）

本方案仅插入 C 标准库 `fopen/fputs/fflush/fclose` 与 `fprintf`，不调用任何 Qt 或工程内部成员函数的新增接口，无需额外签名核查。

### 既有 API 检索记录

调试日志不走 `Logger`（异步，死锁时不可靠），直接用 `fprintf(stderr, ...)` + 同步写文件，绕过所有 Qt/Logger 基础设施。不存在既有 SSOT API 的复用需求。

### Memories.md 相关条款

本方案为纯临时调试插桩，不涉及 UI 样式、控件、图标等 Memories.md 管辖范围。

---

## Overview（概述与解决的问题）

**问题**：双击主程序后，QuarkMeta.exe 进程存活于任务管理器但无任何窗口，日志文件未被写入。  
**根本原因（已排查确认）**：`DatabaseManager::getGlobalDb()` 持 `m_initMutex`（非递归 `std::mutex`）锁后调用 `DatabaseManager::init()`，`init()` 内再次对同一非递归锁加锁 → **同线程二次加锁 UB，触发无限死锁**。  
**本方案目标**：在死锁点前后的每个关键步骤插入**同步写文件的调试日志**，运行一次后通过日志文件精确定位程序停在哪一步。

---

## Modified Files List（影响文件清单）

| 文件 | 改动性质 |
|---|---|
| `src/main.cpp` | 添加启动路径打点 |
| `src/core/CoreController.cpp` | 在 `initializeCoreComponents()` 每个子步骤前后打点 |
| `src/meta/DatabaseManager.cpp` | 在 `init()` 和 `getGlobalDb()` 每个关键行前后打点 |
| `src/meta/DriveMetaDao.cpp` | 在 `initTable()` 入口打点 |

---

## Detailed Line-by-Line Changes（精准替换块）

> 日志写入辅助宏定义：所有文件共用同一个宏，宏仅在调试文件头部展开一次（`main.cpp`），其余文件直接 `#include` 同一个辅助头或复用同一个静态函数。  
> **简化方案**：在每个文件顶部用内联 lambda / 静态函数，避免引入新头文件，减少 CMake 改动。

---

### 修改点 1 — `src/main.cpp`

在 `main()` 函数最顶部（`CreateMutexA` 之前）注入日志辅助函数，并在启动链各步骤前后打点。

```
<<<<<<< SEARCH
int main(int argc, char *argv[]) {
    // -------------------------------------------------------------
    // 重构 2：启动安全。最顶端优先执行单实例互斥量哨兵检测
    // -------------------------------------------------------------
    HANDLE hMutex = nullptr;
=======
// ======= [DEBUG] 临时启动调试辅助 — 定位后删除 =======
static FILE* s_dbgFile = nullptr;
static void dbgLog(const char* msg) {
    if (!s_dbgFile) {
        s_dbgFile = fopen("G:/C++/QuarkMeta/QuarkMeta/QuarkMeta/startup_debug.log", "a");
    }
    if (s_dbgFile) {
        fputs(msg, s_dbgFile);
        fputs("\n", s_dbgFile);
        fflush(s_dbgFile);
    }
}
// ======= [DEBUG END] =======

int main(int argc, char *argv[]) {
    dbgLog("[main] ===== 启动开始 =====");
    // -------------------------------------------------------------
    // 重构 2：启动安全。最顶端优先执行单实例互斥量哨兵检测
    // -------------------------------------------------------------
    HANDLE hMutex = nullptr;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    hMutex = CreateMutexA(NULL, TRUE, "QuarkMeta_SingleInstance_Mutex");
    if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        // 单实例检测失败，由于尚未影响任何运行状态及日志，直接优雅退出
        return 0;
    }
=======
    hMutex = CreateMutexA(NULL, TRUE, "QuarkMeta_SingleInstance_Mutex");
    if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        dbgLog("[main] 单实例检测失败 → 静默退出");
        if (hMutex) CloseHandle(hMutex);
        // 单实例检测失败，由于尚未影响任何运行状态及日志，直接优雅退出
        return 0;
    }
    dbgLog("[main] 单实例检测通过");
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 设置高 DPI 支持：Qt 6 默认行为，此处显式设置 PassThrough 以防旧设备缩放模糊
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QuarkMeta::QuarkApplication a(argc, argv);
=======
    dbgLog("[main] 即将构造 QApplication");
    // 设置高 DPI 支持：Qt 6 默认行为，此处显式设置 PassThrough 以防旧设备缩放模糊
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QuarkMeta::QuarkApplication a(argc, argv);
    dbgLog("[main] QApplication 构造完成");
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
#ifdef Q_OS_WIN
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif

    // -------------------------------------------------------------
    // 重构 3：生命期。引入 AppCoreController 统一拓扑预热
    // -------------------------------------------------------------
    QuarkMeta::CoreController::initializeCoreComponents();

    QuarkMeta::ThemeManager::instance().initialize(&a);

    QuarkMeta::MainWindow w;
=======
#ifdef Q_OS_WIN
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif
    dbgLog("[main] CoInitializeEx 完成");

    // -------------------------------------------------------------
    // 重构 3：生命期。引入 AppCoreController 统一拓扑预热
    // -------------------------------------------------------------
    dbgLog("[main] 即将调用 CoreController::initializeCoreComponents()");
    QuarkMeta::CoreController::initializeCoreComponents();
    dbgLog("[main] CoreController::initializeCoreComponents() 返回");

    dbgLog("[main] 即将初始化 ThemeManager");
    QuarkMeta::ThemeManager::instance().initialize(&a);
    dbgLog("[main] ThemeManager 初始化完成");

    dbgLog("[main] 即将构造 MainWindow");
    QuarkMeta::MainWindow w;
    dbgLog("[main] MainWindow 构造完成");
>>>>>>> REPLACE
```

---

### 修改点 2 — `src/core/CoreController.cpp`

在 `initializeCoreComponents()` 内每个子步骤前后打点。

```
<<<<<<< SEARCH
void CoreController::initializeCoreComponents() {
    // 0. 全局设备监听器启动
    DeviceWatcher::instance().startListening();

    // 1. 底层持久化 SQLite 连接启动与定时机制保障
    QuarkMeta::DatabaseManager::instance();
    
    // 2. 元数据内存索引结构预热
    QuarkMeta::MetadataManager::instance();
    
    // 3. 后台提取特征管道、定时器及事件队列预热
    QuarkMeta::MediaExtractorPipeline::instance();
    
    // 4. 定时合并刷新缩略图提取失败标记落盘
    QTimer* failureFlushTimer = new QTimer(QCoreApplication::instance());
    failureFlushTimer->setInterval(1000);
    QObject::connect(failureFlushTimer, &QTimer::timeout, []() {
        (void)QtConcurrent::run(DiskMediaExtractor::flushPendingUpdates);
    });
    failureFlushTimer->start();
}
=======
// ======= [DEBUG] 临时调试辅助声明 =======
static FILE* s_dbgFileCc = nullptr;
static void dbgLogCc(const char* msg) {
    if (!s_dbgFileCc) {
        s_dbgFileCc = fopen("G:/C++/QuarkMeta/QuarkMeta/QuarkMeta/startup_debug.log", "a");
    }
    if (s_dbgFileCc) {
        fputs(msg, s_dbgFileCc);
        fputs("\n", s_dbgFileCc);
        fflush(s_dbgFileCc);
    }
}
// ======= [DEBUG END] =======

void CoreController::initializeCoreComponents() {
    dbgLogCc("[CoreController] initializeCoreComponents 进入");

    // 0. 全局设备监听器启动
    dbgLogCc("[CoreController] Step0: DeviceWatcher::startListening() 前");
    DeviceWatcher::instance().startListening();
    dbgLogCc("[CoreController] Step0: DeviceWatcher::startListening() 后");

    // 1. 底层持久化 SQLite 连接启动与定时机制保障
    dbgLogCc("[CoreController] Step1: DatabaseManager::instance() 前");
    QuarkMeta::DatabaseManager::instance();
    dbgLogCc("[CoreController] Step1: DatabaseManager::instance() 后");
    
    // 2. 元数据内存索引结构预热
    dbgLogCc("[CoreController] Step2: MetadataManager::instance() 前");
    QuarkMeta::MetadataManager::instance();
    dbgLogCc("[CoreController] Step2: MetadataManager::instance() 后");
    
    // 3. 后台提取特征管道、定时器及事件队列预热
    dbgLogCc("[CoreController] Step3: MediaExtractorPipeline::instance() 前");
    QuarkMeta::MediaExtractorPipeline::instance();
    dbgLogCc("[CoreController] Step3: MediaExtractorPipeline::instance() 后");
    
    // 4. 定时合并刷新缩略图提取失败标记落盘
    dbgLogCc("[CoreController] Step4: failureFlushTimer 启动前");
    QTimer* failureFlushTimer = new QTimer(QCoreApplication::instance());
    failureFlushTimer->setInterval(1000);
    QObject::connect(failureFlushTimer, &QTimer::timeout, []() {
        (void)QtConcurrent::run(DiskMediaExtractor::flushPendingUpdates);
    });
    failureFlushTimer->start();
    dbgLogCc("[CoreController] initializeCoreComponents 完成");
}
>>>>>>> REPLACE
```

---

### 修改点 3 — `src/meta/DatabaseManager.cpp`

在 `init()` 与 `getGlobalDb()` 内每一个关键执行步骤（包括锁的获取点）前后打点。

> ⚠️ 由于死锁就发生在 `lock(m_initMutex)` 处，打点必须在 `lock_guard` 构造**之前**就写入日志，才能区分是"进入函数但卡在锁上"还是"根本没进函数"。

```
<<<<<<< SEARCH
bool DatabaseManager::init() {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (m_isInitialized && m_globalDb.diskDb) {
        return true;
    }

    AppDirectoryInitializer::initializeStoragePath(getAppDir());

    std::wstring globalPath = getGlobalDbPath().toStdWString();
    if (!loadDb(globalPath, m_globalDb)) {
        return false;
    }

    DriveMetaDao::initTable();
    m_isInitialized = true;
    return true;
}
=======
// ======= [DEBUG] 临时调试辅助 =======
static FILE* s_dbgFileDm = nullptr;
static void dbgLogDm(const char* msg) {
    if (!s_dbgFileDm) {
        s_dbgFileDm = fopen("G:/C++/QuarkMeta/QuarkMeta/QuarkMeta/startup_debug.log", "a");
    }
    if (s_dbgFileDm) {
        fputs(msg, s_dbgFileDm);
        fputs("\n", s_dbgFileDm);
        fflush(s_dbgFileDm);
    }
}
// ======= [DEBUG END] =======

bool DatabaseManager::init() {
    dbgLogDm("[DatabaseManager::init] 进入函数");
    dbgLogDm("[DatabaseManager::init] 即将 lock(m_initMutex) ← 若卡在这里说明死锁在此");
    std::lock_guard<std::mutex> lock(m_initMutex);
    dbgLogDm("[DatabaseManager::init] lock(m_initMutex) 成功");
    if (m_isInitialized && m_globalDb.diskDb) {
        dbgLogDm("[DatabaseManager::init] 已初始化，提前返回 true");
        return true;
    }

    dbgLogDm("[DatabaseManager::init] 即将 initializeStoragePath");
    AppDirectoryInitializer::initializeStoragePath(getAppDir());
    dbgLogDm("[DatabaseManager::init] initializeStoragePath 完成");

    std::wstring globalPath = getGlobalDbPath().toStdWString();
    dbgLogDm("[DatabaseManager::init] 即将 loadDb");
    if (!loadDb(globalPath, m_globalDb)) {
        dbgLogDm("[DatabaseManager::init] loadDb 失败，返回 false");
        return false;
    }
    dbgLogDm("[DatabaseManager::init] loadDb 成功");

    dbgLogDm("[DatabaseManager::init] 即将调用 DriveMetaDao::initTable()");
    DriveMetaDao::initTable();
    dbgLogDm("[DatabaseManager::init] DriveMetaDao::initTable() 返回");
    m_isInitialized = true;
    dbgLogDm("[DatabaseManager::init] 完成，返回 true");
    return true;
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
sqlite3* DatabaseManager::getGlobalDb() {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (!m_isInitialized || !m_globalDb.diskDb) {
        init();
    }
    return m_globalDb.diskDb;
}
=======
sqlite3* DatabaseManager::getGlobalDb() {
    dbgLogDm("[DatabaseManager::getGlobalDb] 进入函数");
    dbgLogDm("[DatabaseManager::getGlobalDb] 即将 lock(m_initMutex) ← 若卡在这里说明死锁在此");
    std::lock_guard<std::mutex> lock(m_initMutex);
    dbgLogDm("[DatabaseManager::getGlobalDb] lock(m_initMutex) 成功");
    if (!m_isInitialized || !m_globalDb.diskDb) {
        dbgLogDm("[DatabaseManager::getGlobalDb] 未初始化，即将调用 init() ← 若下一条不出现则死锁在 init()");
        init();
        dbgLogDm("[DatabaseManager::getGlobalDb] init() 返回");
    }
    dbgLogDm("[DatabaseManager::getGlobalDb] 返回 db 指针");
    return m_globalDb.diskDb;
}
>>>>>>> REPLACE
```

---

### 修改点 4 — `src/meta/DriveMetaDao.cpp`

在 `initTable()` 入口打点，确认是否有机会进入此函数。

```
<<<<<<< SEARCH
bool DriveMetaDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;
=======
// ======= [DEBUG] 临时调试辅助 =======
static FILE* s_dbgFileDd = nullptr;
static void dbgLogDd(const char* msg) {
    if (!s_dbgFileDd) {
        s_dbgFileDd = fopen("G:/C++/QuarkMeta/QuarkMeta/QuarkMeta/startup_debug.log", "a");
    }
    if (s_dbgFileDd) {
        fputs(msg, s_dbgFileDd);
        fputs("\n", s_dbgFileDd);
        fflush(s_dbgFileDd);
    }
}
// ======= [DEBUG END] =======

bool DriveMetaDao::initTable() {
    dbgLogDd("[DriveMetaDao::initTable] 进入函数");
    dbgLogDd("[DriveMetaDao::initTable] 即将调用 getGlobalDb() ← 若卡在这里是第3层嵌套锁");
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    dbgLogDd("[DriveMetaDao::initTable] getGlobalDb() 返回");
    if (!db) return false;
>>>>>>> REPLACE
```

---

## Build & Verification Steps（编译与运行验证）

1. **编译**：在 Qt Creator 或 CMake 构建链执行正常 Debug 构建。
2. **清除旧日志**（可选）：删除或清空 `QuarkMeta/startup_debug.log`，确保读取的是本次启动的输出。
3. **双击运行**：正常双击 `QuarkMeta.exe`，等待 5 秒（或观察任务管理器进程是否卡住）。
4. **读取日志**：打开 `G:/C++/QuarkMeta/QuarkMeta/QuarkMeta/startup_debug.log`，找到**最后一条打印的日志**，该行之后的下一个预期日志行即为死锁点。

### 预期结论对照表

| 最后出现的日志行 | 死锁位置 |
|---|---|
| `[main] 单实例检测失败 → 静默退出` | 单实例互斥量冲突（旧进程残留），并非死锁 |
| `[CoreController] Step1: DatabaseManager::instance() 前` | 单例构造期间卡死 |
| `[DatabaseManager::getGlobalDb] 即将 lock(m_initMutex)` | 第1次加锁处卡死（另一线程持锁） |
| `[DatabaseManager::getGlobalDb] lock 成功` 后停在 `[DatabaseManager::init] 即将 lock` | **确认死锁**：`getGlobalDb` 持锁后，`init()` 尝试同一非递归锁二次加锁 |
| `[DatabaseManager::init] 即将调用 DriveMetaDao::initTable()` | `init()` 内的逻辑到此，`DriveMetaDao::initTable()` 调 `getGlobalDb` → 第3层锁 |
| `[main] MainWindow 构造完成` 后无窗口 | 死锁在 MainWindow 构造链内，需进一步细化插桩 |

---

## SSOT API Reuse & Anti-Redundancy Self-Check

本方案为纯调试插桩，不涉及业务逻辑变更，不调用任何 SSOT 入口（`refreshAll`、`loadDirectory`、`AppConfig`、`CentralEventHub`），不存在另起炉灶行为。

---

## Header API Signature Verification

本方案仅使用 C 标准库函数 `fopen`/`fputs`/`fflush`，均来自 `<cstdio>`，无需额外头文件（各 `.cpp` 已隐式或显式包含标准库）。无外部成员函数调用，无签名核查风险。

---

## Header Inclusion Chain & Type Completeness Check

本方案不新增或删除任何 `#include` 行，不引入新类型，无包含链断裂风险。

---

## Zero-Value-Alteration Check

不涉及样式迁移或数值参数，写"不涉及"。

---

## Compliance Self-Check Table（合规核对表）

| 检查项 | 结果 | 对应位置 |
|---|---|---|
| 文件名与核心类名 1:1 映射 | 满足 | 文件名 `StartupDeadlockDebug.md`，对应调试目标 |
| 方案位于物理隔离目录 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 满足 | 已物理读取全部 4 个文件并核对 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 不涉及 | 未新增类，仅在既有 .cpp 中插入静态辅助函数 |
| 第 1~9 章齐全 | 满足 | 见上方各章节（9. Zero-Value-Alteration Check 已写） |
| 每个外部成员调用均有头文件原文签名对应 | 不涉及 | 仅用 C 标准库，无工程外部成员调用 |
| 每个被改/删的 `#include` 已核查间接包含链 | 不涉及 | 未改动任何 `#include` |
| 已复用既有 SSOT 入口，无另起炉灶 | 不涉及 | 纯调试插桩 |
| 无内联 `setStyleSheet`，样式走 style.qss | 不涉及 | 无 UI 改动 |
| 无平台级 Hack、无无范围全局事件过滤器 | 满足 | 仅 fopen/fputs |
| 批量操作为一次聚合广播 | 不涉及 | 无业务逻辑 |
| 迁移的所有数值原样照抄 | 不涉及 | 无数值迁移 |
| 未改动用户既有设计理念与交互方式 | 满足 | 纯插桩，零行为改动 |
| 未触及方案范围外的文件 | 满足 | 仅改动上述 4 个文件 |
