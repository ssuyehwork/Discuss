# 主程序启动崩溃排查报告
> 排查时间：2026-10-22  |  仅排查、仅汇报，无修改

---

## 1. 最近改动的完整 diff —— 改动文件清单

本次未提交的改动共 **9 个文件**：

| 文件 | 改动性质 |
|---|---|
| `src/util/ShellHelper.h` | 新增 `AssocAppInfo` 结构体、`getAssociatedApps()`、`showOpenWithDialog()` 声明；新增 `QIcon`/`QWidget` 头文件 |
| `src/util/ShellHelper.cpp` | 新增 `shlobj.h`/`shobjidl.h` 引入；实现 `getAssociatedApps()`（含 `SHAssocEnumHandlers` + `ExtractIconW` 逐条提图）；实现 `showOpenWithDialog()` |
| `src/ui/controllers/ContextMenuFactory.h` | 新增 `buildOpenWithMenu()` 静态方法声明 |
| `src/ui/controllers/ContextMenuFactory.cpp` | 新增 `QProcess`/`QDesktopServices` 引入；实现 `buildOpenWithMenu()`（调用 `ShellHelper::getAssociatedApps`） |
| `src/ui/controllers/ContentContextMenu.cpp` | 在单文件右键菜单中增加 `ContextMenuFactory::buildOpenWithMenu(...)` 调用 |
| `src/ui/controllers/ContentKeyHandler.cpp` | `PasteTags` 逻辑从直接 `model->setData` 改为走 `CoreEngine::executeCommand` |
| `src/core/ClipboardService.cpp` | `executePasteTags` 逻辑同上，改走 `CoreEngine::executeCommand`；新增 `CoreEngine.h` 引入 |
| `src/meta/DatabaseManager.cpp` | **重大重构**：删除 `initLocked()` 实现，将其逻辑合并进 `init()`；重写 `getGlobalDb()` 使其在持 `m_initMutex` 锁的情况下直接调用 `init()` |
| `src/ui/QuickLookWindow.cpp` | 菜单文字改动（纯文字）|

> [!NOTE] `Memories.md` 和 `SSOT.md` 也有改动，但均为文档类，与启动无关。

---

## 2. 启动路径日志分析 —— 崩溃停在哪一步

### 结论：**崩溃发生在 Logger 初始化之前，日志文件完全未被写入**

**证据：**
- `quarkmeta_debug.log` 最后修改时间为 `2026-10-21 23:13:05`，最后一条记录也是此时间。
- 今天（22日）`9:41` 编译出新 exe 后，启动 → 立即退出，日志文件时间戳**没有任何变化**。
- 用 PowerShell `Start-Process ... -Wait` 实测，进程退出代码为：

  ```
  ExitCode = -1073740791  (即 0xC0000409 = STATUS_STACK_BUFFER_OVERRUN)
  ```

**STATUS_STACK_BUFFER_OVERRUN** 是 Windows 安全机制（`__fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE)` 或 `/GS` 栈 Cookie 校验失败）触发的崩溃，常见于：
- `std::mutex::lock()` 在已持锁的同一线程上再次被加锁（非递归互斥量 UB/死锁触发 CRT abort）；
- 栈溢出；
- 未定义行为导致栈破坏。

**日志启动路径**（`main.cpp` 分析）：  
`CreateMutexA` → `QApplication` → `CoInitializeEx` → `CoreController::initializeCoreComponents()` → **此处崩溃**，Logger 从未被安装（Logger 在 QApplication 构造后安装，崩溃点在 `initializeCoreComponents` 内）。

> [!IMPORTANT] 崩溃点：在 `CoreController::initializeCoreComponents()` 内，`DatabaseManager::instance()` 被触发，触发懒加载 `getGlobalDb()`，`getGlobalDb()` 持 `m_initMutex` 后调用 `init()`，`init()` 尝试再次加同一非递归互斥量 → **UB，MSVC CRT 检测后调用 `__fastfail` → 0xC0000409**。

---

## 3. ShellIconManager / ExtractIconW / SHGetFileInfo 调用点

### 结论：**`ExtractIconW` 仅在右键菜单运行时调用，不在启动期执行；无两处重复提取同一图标**

| 调用点 | 位置 | 是否在启动期 |
|---|---|---|
| `ExtractIconW` | `src/util/ShellHelper.cpp` → `getAssociatedApps()` | **否**，仅在右键菜单"打开方式"弹出时才触发 |
| `SHAssocEnumHandlers` | 同上 | **否** |
| `SHGetFileInfo` | 当前主工程 `src/` 下**未找到** | — |
| `ShellIconManager::getFileIcon/getFileIconFast` | `DiskItemModel.cpp`、`LibraryAssetModel.cpp`、`ContentPanel.cpp`、`NavPanel.cpp`、`QuickLookWindow.cpp` | **否**，均在文件列表渲染时按需调用 |

`getAssociatedApps()` 与 `ShellIconManager` 功能域完全不同（前者提取关联应用图标，后者提取文件自身图标），**不存在同一图标两处各自提取的重复问题**。

---

## 4. `getGlobalDb()` / `init()` / `initTable()` —— 启动期死锁路径

### 结论：**确认：存在同线程非递归互斥量三层嵌套加锁路径，是本次崩溃的直接根本原因**

**锁类型**：`m_initMutex` 为 `std::mutex`（**非递归**），`DatabaseManager.h` 第 77 行确认。

**死锁调用链（单线程，主线程）**：

```
CoreController::initializeCoreComponents()       [CoreController.cpp L33]
  └─ DatabaseManager::instance()
  └─ MetadataManager::instance()
       └─ loadFromDb(DatabaseManager::instance().getGlobalDb())  [or equivalent]
            └─ getGlobalDb()                     [DatabaseManager.cpp L273]
                 └─ lock(m_initMutex)            ← 第 1 次加锁 ✅
                 └─ !m_isInitialized → init()    [L276]
                      └─ lock(m_initMutex)       ← 第 2 次加锁 ❌ UB
                      └─ DriveMetaDao::initTable()  [L261]
                           └─ getGlobalDb()      [DriveMetaDao.cpp L10]
                                └─ lock(m_initMutex)  ← 第 3 次加锁 ❌❌
```

**具体代码（`src/meta/DatabaseManager.cpp` 当前工作区）**：

```cpp
// L273-278：getGlobalDb 持锁后调用 init
sqlite3* DatabaseManager::getGlobalDb() {
    std::lock_guard<std::mutex> lock(m_initMutex);   // ← 第1次加锁
    if (!m_isInitialized || !m_globalDb.diskDb) {
        init();   // ← init 内还会再加同一把锁
    }
    return m_globalDb.diskDb;
}

// L248-263：init 本身也加同一把锁
bool DatabaseManager::init() {
    std::lock_guard<std::mutex> lock(m_initMutex);   // ← 第2次加锁 → UB
    ...
    DriveMetaDao::initTable();   // ← 内部还调 getGlobalDb → 第3次加锁
    ...
}
```

**`DriveMetaDao.cpp L9-10`**：
```cpp
bool DriveMetaDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();  // ← 第3次加锁
```

**旧版本（git HEAD 基准）的正确实现**：`getGlobalDb()` 持锁时只调用私有的 `initLocked()`（无锁版本），`DriveMetaDao::initTable()` 在 `init()` 释放锁之后才调用。新版本错误地将所有加锁逻辑平铺合并，制造了不可避免的同线程递归锁。

**附加异常**：`DatabaseManager.h` 第 74 行仍保留 `bool initLocked();` 声明，但 `DatabaseManager.cpp` 中**已无该实现**（diff 中被删除）。这会在任何调用方引用它时产生链接错误，且属于悬空声明垃圾。

---

## 5. 全新空 `.QuarkMeta` 目录 vs 保留旧库

### 结论：**两种情况结果相同，均在数据库文件打开之前就已崩溃**

死锁触发点在 `init()` 内的 `std::lock_guard` 构造时（第2次加锁），此时程序还未走到 `loadDb()`（`sqlite3_open16`）那一步。因此：

- `.QuarkMeta` 目录是否存在、是否有旧库文件 —— 对本次崩溃**无任何影响**。
- **无需测试**，逻辑上可确认为相同结果。

---

## 6. 编译状态与构建输出

### 结论：**exe 已于今天 9:41 重新编译成功；编译过程中存在潜在警告但未能捕获完整输出**

- 可执行文件 `QuarkMeta\QuarkMeta.exe` 最后修改时间 `2026/10/22 09:41:18`，确认今天已完成重新编译。
- 构建日志未找到（`out/build/x64-Debug/` 内无日志文件）。
- **已知潜在编译问题**：
  1. `DatabaseManager.h:74` 保留 `bool initLocked();` 声明，`.cpp` 无实现 → 若任何翻译单元调用将产生链接错误，目前无调用点，但属垃圾声明。
  2. `ShellHelper.h` 新增 `#include <QWidget>` 在工具类头文件中，会被所有包含 ShellHelper.h 的 cpp 间接引入 QWidget 完整定义，扩大编译依赖范围。

---

## 汇总结论（逐项）

| 问题项 | 结论 |
|---|---|
| **1. 完整 diff** | 已列出 9 个改动文件；最危险改动：`DatabaseManager.cpp` 锁结构重构 |
| **2. 启动日志/崩溃位置** | 日志完全未更新；退出码 `0xC0000409 (STATUS_STACK_BUFFER_OVERRUN)`；崩溃在 `CoreController::initializeCoreComponents()` 内，Logger 从未安装 |
| **3. ExtractIconW/ShellIconManager** | 均不在启动期执行；无重复提图问题 |
| **4. getGlobalDb/init/initTable 死锁** | ✅ **确认**：`getGlobalDb()` 持 `m_initMutex` → 调 `init()` → 再次加同一非递归锁 → UB → 崩溃；`DriveMetaDao::initTable()` 内形成第三层嵌套 |
| **5. 空目录 vs 旧库** | 无差别，崩溃在 SQLite 文件打开之前 |
| **6. 编译输出** | 编译成功生成 exe；未捕获完整构建输出；`initLocked()` 悬空声明已确认 |
