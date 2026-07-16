# 分离磁盘路径模式与侧边栏分类模式加载驱动 —— Modification_Plan-3.md

## 1. 任务背景
用户期望彻底取消“自动切换为内存镜像加载”（对应用户原话：“彻底取消“自动切换为内存镜像加载”的设计”）的混杂机制，并让物理磁盘路径模式与侧边栏分类模式两种驱动模式各司其职、互不干扰（对应用户原话：“让两种驱动模式各司其职、互不干扰：”）。为此，我们需要解耦现有的自动分流加速逻辑，确保磁盘路径模式始终只采用实时的物理 I/O 检索读取，而不受任何内存或 DB 镜像缓存的影响。

## 2. 问题定位
* **关键函数**：`src/ui/ContentPanel.cpp` 中的 `ContentPanel::loadDirectory(const QString& path, bool recursive)`。
* **分流逻辑位置**：该函数内部第 2470 行至 2490 行之间，存在以下逻辑：
  ```cpp
  // 2026-07-xx 按照 Plan-116：检测是否导航进入托管库内部
  std::wstring wp = path.toStdWString();
  ...
  // 镜像加载模式（加速）
  if (isInsideLibrary && !recursive) {
      (void)QtConcurrent::run([panelPtr, path, reqId]() {
          ...
      });
      return;
  }
  ```
  该分支正是造成“物理导航时自动切换到内存镜像加载（零物理 I/O 检索）”的根本原因。需要将此分流逻辑段完全废除。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 磁盘路径模式：在任何情况下均只执行纯粹的实时物理 I/O 驱动扫描，不再走任何基于内存/数据库的镜像提取加速逻辑 | 彻底删除 `ContentPanel::loadDirectory` 内部导航进入托管目录时切换为内存镜像加载的 `if (isInsideLibrary && !recursive)` 代码块，使所有磁盘导航请求均统一进入物理扫描流程。 | ✅ |
| 2    | 侧边栏分类模式：继续保持其原有的 DB 数据库驱动 | 侧边栏分类模式对应的加载、过滤和分类更新逻辑保持完全不动，继续依靠已成熟的 SQLite 内存数据库和 Mft 内存缓存体系。 | ✅ |

## 4. 详细解决方案

### 4.1. 彻底移除内存镜像自动切换代码
1. 在 `src/ui/ContentPanel.cpp` 中的 `ContentPanel::loadDirectory` 函数体中，找到检测和分流进入托管库加速分支的实现。
2. 彻底移除（对应用户原话：“彻底取消“自动切换为内存镜像加载”的设计”）下述检测和分支阻断结构：
   ```cpp
   // 彻底移除此段检测和分流加速代码：
   /*
   std::wstring wp = path.toStdWString();
   std::wstring volSerial = MetadataManager::getVolumeSerialNumber(wp);
   QString key = QString("ManagedFolder/Volume_%1").arg(QString::fromStdWString(volSerial));
   QString relPath = AppConfig::instance().getValue(key, "").toString();
   bool isInsideLibrary = false;
   if (!relPath.isEmpty()) {
       QString drive = path.left(3);
       QString managedAbs = QDir::toNativeSeparators(drive + relPath).toLower();
       if (path.toLower().startsWith(managedAbs)) isInsideLibrary = true;
   }

   QPointer<ContentPanel> panelPtr(this); 
   
   if (isInsideLibrary && !recursive) {
       (void)QtConcurrent::run([panelPtr, path, reqId]() {
           ...
       });
       return;
   }
   */
   ```

### 4.2. 验证落入纯物理 I/O 扫描分支
1. 移除上述加速逻辑后，任何进入 `loadDirectory(path, recursive)` 的调用流程将直接向下平铺，最终统一进入原有的 **“物理扫描模式（原逻辑）”**。
2. 该纯物理扫描模式使用 `QDir::entryInfoList` 在磁盘上进行最真实物理检索读取（对应用户原话：“纯粹的 实时物理 I/O 驱动扫描（通过 QDir 等在磁盘上进行实际目录检索与读取）”），确保无论在库外、库内还是库内的深度嵌套子树，均呈现一致的物理扫描特性。

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [x] 模块/文件：`src/ui/ContentPanel.cpp`（完全解除导航加速分流）

**明确禁止越界修改的范围：**
- [ ] 模块/文件：侧边栏分类模式所驱动的系统，包括 `CategoryModel`、`CategoryPanel` 绝不允许做出影响 DB 查询驱动的任何逻辑微调。

## 6. 实现准则与预警【核心】
1. **防抖与异步交互**：磁盘扫描（尤其是递归模式下）属于高延迟、高消耗的物理操作，必须确保依然利用 `QThreadPool::globalInstance()->start()` 后台线程池进行异步加载，杜绝在前台 UI 线程发生死锁、假死、卡顿或响应延迟。
2. **防重入与请求幂等**：任何时候连续点击或多次双击进入新目录时，必须依托 `m_loadRequestId` 计数器自增实现新旧请求的异步拦截与抛弃逻辑，防止异步多线程读取冲突导致 UI 渲染乱序。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式| 严禁修改代码、创建代码、执行构建或测试 | ✅ (本案仅以高标准提供修改计划文档，不执行源码编辑) |

## 8. 待确认事项（可选）
* 无。
