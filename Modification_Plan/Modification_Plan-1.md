# 恢复 NativeFolderWatcher (IOCP) 监控机制 —— Modification_Plan-1.md

## 1. 任务背景
用户期望在当前版本中不再采用 USN Journal（USN 日志）方式来监控 “ArcMeta.Library_[盘符]” 文件夹的变动，而是恢复使用原先具有高性能优势的 `NativeFolderWatcher` (IOCP) 监控机制。因此，我们需要设计一套完整的重构计划，将 `NativeFolderWatcher` (IOCP) 重新集成到当前版本中，并将 USN Journal 相关的监听与处理彻底移除。

## 2. 问题定位
* **变动来源移除**：当前版本的 `AutoImportManager::startListening()` 及 `stopListening()` 均与 `MftReader` 的 USN 更新信号（`entryAdded` / `entryUpdated` / `entryRemoved`）连接，这正是 USN 监控的核心驱动力。
* **文件缺失**：当前版本的 `CMakeLists.txt` 中已彻底去除了 `src/core/NativeFolderWatcher.cpp` 和 `src/core/NativeFolderWatcher.h` 文件，导致编译体系中完全没有该组件。
* **启动逻辑**：在 `src/core/CoreController.cpp` 中系统初始化时，调用的是 `AutoImportManager::instance().startListening()` (开启 USN 主轨) 及 `MftReader::instance().loadFromCache()` (加载 Mft 缓存)。
* **对账逻辑**：原 `NativeFolderWatcher` 会在 IOCP 线程中捕获到添加/修改信号并调用 `MetadataManager::instance()` 的注册逻辑。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 从现在开始不再采用 USN Journal（USN 日志）方式监控 “ArcMeta.Library_[盘符]” 文件夹的变动 | 在 `AutoImportManager` 中完全废除并断开 `MftReader` 的信号连接，并在 `CoreController` 中停止激活 `MftReader` 缓存及 USN 线程。 | ✅ |
| 2    | 采用NativeFolderWatcher (IOCP) 机制的方式，将NativeFolderWatcher (IOCP) 机制进行恢复 | 完整恢复 `NativeFolderWatcher.h` & `NativeFolderWatcher.cpp` 两个源文件，并在 `CMakeLists.txt` 中进行登记。在 `CoreController` 系统初始化中对其进行初始化与注册。 | ✅ |

## 4. 详细解决方案

### 4.1. 恢复 NativeFolderWatcher 代码文件
1. 重新在 `src/core/` 目录下创建 `NativeFolderWatcher.h` 与 `NativeFolderWatcher.cpp`，代码逻辑从旧版本中完美继承。
2. 确保 `NativeFolderWatcher` 的文件过滤和信号分发与当前版本的 `MetadataManager` 接口匹配：
   - 目录：调用 `MetadataManager::instance().markAsRegistered(fullPath)` 触发级联登记。
   - 文件：调用 `MetadataManager::instance().registerItemsAsync({QString::fromStdWString(fullPath)}, true)` 触发异步批量注册。

### 4.2. 在 CMakeLists.txt 中注册 NativeFolderWatcher
在 `CMakeLists.txt` 的 `set(SOURCES ...)` 列表中加入 `src/core/NativeFolderWatcher.cpp` 和 `src/core/NativeFolderWatcher.h`，确保它们能够被正确编译和链接。

### 4.3. 修改 CoreController 恢复 NativeFolderWatcher 启动机制
在 `src/core/CoreController.cpp` 的 `startSystem()` 中进行如下调整：
1. 注销原 USN 监控监听：
   ```cpp
   // AutoImportManager::instance().startListening(); // 注销 USN 日志监听
   // MftReader::instance().loadFromCache();          // 注销 Mft MftReader 缓存加载
   ```
2. 启动原生 IOCP 监控服务：
   ```cpp
   // 启动原生监控服务 (对应用户原话："采用NativeFolderWatcher (IOCP) 机制的方式")
   const auto drives = QDir::drives();
   for (const QFileInfo& d : drives) {
       std::wstring wPath = d.absolutePath().toStdWString();
       std::wstring volSerial = MetadataManager::getVolumeSerialNumber(wPath);
       QString letter = d.absolutePath().left(1).toUpper();

       if (volSerial != L"UNKNOWN") {
           std::wstring managedAbsW = MetadataManager::getManagedLibraryPath(volSerial, letter);
           if (!managedAbsW.empty()) {
               qDebug() << "[Core] 识别到托管库，开启 IOCP 监控:" << QString::fromStdWString(managedAbsW);
               NativeFolderWatcher::instance().addWatch(managedAbsW);
           }
       }
   }
   ```

### 4.4. 停止并废除 AutoImportManager 的 USN 监听
1. 修改 `src/core/AutoImportManager.cpp`，将 `startListening()` 和 `stopListening()` 函数中的信号连接逻辑完全注销或清空。
2. 移除 `isUnderManagedLibrary`、`onEntryAdded`、`onEntryUpdated`、`onEntryRemoved` 中涉及 USN 和 MftReader 数据结构的过载依赖。

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [x] 模块/文件：`CMakeLists.txt`（注册 `NativeFolderWatcher`）
- [x] 模块/文件：`src/core/NativeFolderWatcher.h` & `src/core/NativeFolderWatcher.cpp`（完全恢复）
- [x] 模块/文件：`src/core/CoreController.cpp`（初始化 IOCP 监控、注销 USN 启动）
- [x] 模块/文件：`src/core/AutoImportManager.cpp` & `src/core/AutoImportManager.h`（移除 USN 信号连接）

**明确禁止越界修改的范围：**
- [ ] 模块/文件：`src/meta/sqlite3.c` & `src/meta/sqlite3.h` （底层数据库核心，禁止做任何不相干调整）
- [ ] 模块/文件：`src/ui/` 下所有非必要 UI 页面文件及布局配置。

## 6. 实现准则与预警【核心】
1. **Windows 句柄管理**：`NativeFolderWatcher` 底层基于 Win32 API 句柄（IOCP 和 `ReadDirectoryChangesW`）。在 `shutdown()` 时必须妥善关闭句柄，防止资源泄露导致系统级崩溃。
2. **多线程并发安全**：IOCP 的工作线程是在多线程环境下回调 `MetadataManager` 进行入库注册，须注意底层数据库和文件锁的并发访问冲突，配合原先存在的 `s_dbAccessMutex` 等进行同步。
3. **不要移除不必要的依赖项**：在修改 `CMakeLists.txt` 时，原先链接的 `ntdll`、`ole32`、`bcrypt`、`psapi` 等库千万不能丢，否则会引发符号未定义或编译链接失败。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| UI考古      | 必须先在现有代码中搜索同类已实现的案例 | ✅ (已考古 NativeFolderWatcher 并以此作为恢复模板) |
| 纯分析师模式| 严禁修改代码、创建代码、执行构建或测试 | ✅ (本案不修改或创建任何代码源文件，仅作为文档输出) |

## 8. 待确认事项（可选）
* 无。
