# Development Plan —— 物理监控与侧边栏分类树自动同步开发计划

## 1. 核心需求
在注销/停用了 NTFS USN 日志监听（`MftReader` 处于不点火状态）、将文件系统变更监控全部收拢回 `NativeFolderWatcher` (IOCP) 之后，系统需要保证：
- 当用户在内容面板中执行“迁移”将文件夹移动至 `ArcMeta.Library_[盘符]`，或者以其他物理方式在托管库/自定义监控文件夹内创建、拷贝、移入新的子目录时，`NativeFolderWatcher` 能够实时、自动地检测到这一级联变动。
- 物理变化检测到后，必须无缝、实时地在 SQLite 数据库中构建对应的 1:1 分类树记录（即向 `categories` 表里创建分类记录），并立即触发侧边栏 `CategoryPanel` 的全量重建刷新。
- 绝不需重启主程序，新迁移或新增的物理文件夹能够立刻作为对应的分类条目展现在侧边栏树中。

## 2. 解决方案概述
1. **建立架构闭环**：
   在 `NativeFolderWatcher::handleNotification` (`src/core/NativeFolderWatcher.cpp`) 中，当检测到有效变动 Action（如 `FILE_ACTION_ADDED` 等）且对象为目录（`info.isDir()`）时：
   - 废除原来的仅仅只写元数据的 `MetadataManager::instance().markAsRegistered(fullPath)` 调用。
   - 改为通过 `QtConcurrent::run` 异步拉起级联自动入库与分类同步引擎：`AutoImportManager::instance().handleRecursiveIngestion(fullPath)`。
2. **实现效果**：
   - 该引擎会自动递归扫描该物理目录、为物理目录生成 1:1 的 `categories` 表分类节点并写入数据库、同步登记文件至 `metadata` 表、最后自动调用并派发 `notifyFullUIRebuild()` 重建信号。
   - `CategoryPanel` 实时捕获到该信号后，在 200ms 内对多重高频信号完成防抖合并，并在超时后全量刷新重载侧边栏树结构，实现无重启、无延迟、无感知的完美平滑同步刷新。
