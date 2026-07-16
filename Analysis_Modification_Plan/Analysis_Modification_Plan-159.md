# 目录导航去耦合与物理扫描回归 —— Analysis_Modification_Plan-159.md

## 1. 任务背景
在当前版本中，当用户在“目录导航（物理路径）”模式下进入 `ArcMeta.Library_[盘符]` 托管文件夹内部时，系统底层为了实现加速，会自动切换为“内存镜像加载模式”（即跳过真实的物理磁盘 I/O 读取，直接从 `MetadataManager` 缓存的内存树中查询并提取子项数据）。

为了消除这一自动切换行为给用户造成的困扰，我们期望彻底将物理路径导航与内存镜像加速逻辑进行解耦，让两种加载驱动模式回归最本质的职责：物理路径导航在任何情况下都始终执行原生的物理磁盘扫描；而内存镜像加载模式仅在专门的逻辑分类视图（侧边栏分类模式）中发挥高速读取作用。

## 2. 问题定位
* **自动分流机制**：在 `src/ui/ContentPanel.cpp` 中的 `ContentPanel::loadDirectory` 函数内，目前存在以下路径托管库检测及加速重定向逻辑：
  ```cpp
  // 镜像加载模式（加速）
  if (isInsideLibrary && !recursive) {
      (void)QtConcurrent::run([panelPtr, path, reqId]() {
          ...
          // 从 MetadataManager 内存镜像中过滤出该路径下的直接子项
          ...
      });
      return;
  }
  ```
  该逻辑会导致只要物理路径处于托管库内部且没有显式开启递归扫描，系统便会自动切换至内存模式加载数据，从而形成了自动重定向行为。
* **职责混淆**：目录导航应当在任何目录下均表现出一致的物理 I/O 扫描行为，不应由于路径处于托管库内外而自动变换加载引擎。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 从现在开始，采取不再自动切换为“内存镜像加载”（对应用户原话） | 彻底移除 `ContentPanel::loadDirectory` 中针对 `isInsideLibrary` 状态判断的分流加载代码，从而取消在该路径下自动切换至“内存镜像加载”的行为。 | ✅        |
| 2    | 各自执行各自的逻辑即可（对应用户原话） | 目录导航（物理路径导航）模式不再读取内存缓存，其在托管库内部导航时与托管库外部一样，完全一致、毫无例外地调用线程池执行纯粹的“物理扫描模式”；而内存模式（DB 驱动）继续专属用于“侧边栏分类模式”及逻辑视图加载。 | ✅        |

## 4. 详细解决方案

### 4.1 移除 `ContentPanel::loadDirectory` 内部自动分流代码
在 `src/ui/ContentPanel.cpp` 中定位 `ContentPanel::loadDirectory` 方法，定位并清理用于分流“镜像加载模式（加速）”的条件分支与对应的异步数据提取逻辑。

* **具体修改点（仅供逻辑设计参考，禁止直接输出/修改代码文件）**：
  移除以下代码块：
  ```cpp
  // 镜像加载模式（加速）
  if (isInsideLibrary && !recursive) {
      (void)QtConcurrent::run([panelPtr, path, reqId]() {
          if (!panelPtr) return;
          std::vector<ItemRecord> allItems;

          // 从 MetadataManager 内存镜像中过滤出该路径下的直接子项
          std::wstring normParent = MetadataManager::normalizePath(path.toStdWString());
          if (!normParent.empty() && normParent.back() != L'\\' && normParent.back() != L'/') normParent += L'\\';

          MetadataManager::instance().forEachCachedItem([&](const std::wstring& p, const RuntimeMeta& /*meta*/) {
              if (p.find(normParent) == 0) {
                  std::wstring sub = p.substr(normParent.length());
                  if (sub.find_first_of(L"\\/") == std::wstring::npos) {
                      allItems.push_back(ContentPanel::createItemRecord(QString::fromStdWString(p)));
                  }
              }
          });

          QMetaObject::invokeMethod(QCoreApplication::instance(), [panelPtr, allItems, reqId]() {
              if (panelPtr && panelPtr->m_loadRequestId == reqId) {
                  panelPtr->m_model->setRecords(allItems);
                  panelPtr->m_proxyModel->sort(0, Qt::AscendingOrder);
                  panelPtr->m_isLoading = false;
                  panelPtr->recalculateAndEmitStats();
                  panelPtr->applyFilters();
                  ArcMeta::Logger::log(QString("[Content] 托管库镜像加载完成 [%1]").arg(reqId));
              }
          });
      });
      return;
  }
  ```

* **逻辑演进效果**：
  在删除上述加速分流段后，`ContentPanel::loadDirectory` 函数里的全部物理导航请求（包括进入托管库路径和外部常规磁盘路径）在判定完“computer://”等虚拟节点后，将无条件顺延执行底层的 **“物理扫描模式（原逻辑）”** 分支，利用全局线程池通过 `dir.entryInfoList` 执行真实的物理磁盘 I/O 目录树读取。

### 4.2 清除无用的本地辅助局部变量
在 `ContentPanel::loadDirectory` 的头部，计算 `isInsideLibrary` 状态的一段辅助代码（通过 `AppConfig` 读取卷配置和比对路径）如果后续没有其他地方使用，应在未来修改中一并移除（或仅保留相关的未定义用途），使 `loadDirectory` 保持轻量、高效。

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/ContentPanel.cpp` 中的 `ContentPanel::loadDirectory` 成员函数加载分流逻辑。

**明确禁止越界修改的范围：**
- [ ] 严禁修改任何 `MetadataManager` 底层的元数据维护或倒排树索引。
- [ ] 严禁修改 `CategoryModel`、`CategoryPanel` 或 `CategoryRepo` 的数据库驱动分类加载逻辑。
- [ ] 严禁由于此次去耦合而触碰或阻断 `NativeFolderWatcher` 触发的底层实时变动监控与元数据流式登记闭环。

## 6. 实现准则与预警【核心】

1. **依赖头文件与声明预警**：
   * 本次修改仅涉及移去分流控制段，不涉及任何新依赖或库函数的引入，因此不会产生头文件缺失导致的编译中断（仍完全依赖 `src/ui/ContentPanel.cpp` 已有头文件）。
2. **多线程并发安全预警**：
   * 原本的加速模式（`isInsideLibrary` 分流段）采用了 `QtConcurrent::run` 在临时线程中轮询内存字典；而正常的“物理扫描模式”则是通过投递任务到 `QThreadPool::globalInstance()` 进行递归 I/O。
   * 去除加速重定向后，所有的物理导航都通过线程池任务执行，必须严格注意多线程中的指针与数据生命周期。原有的 `panelPtr`（基于 `QPointer` 的弱引用保护）在异步回调中通过 `QMetaObject::invokeMethod` 跨线程安全切回 UI 线程的逻辑必须完好保留，绝对禁止在回调中使用裸指针，防范潜在的异步闪退问题。
3. **开箱即用与上下文一致性**：
   * 在删除加速模式分支后，应当顺延保留其下方的“物理扫描模式（原逻辑）”主体部分，确保 `loadDirectory` 函数体中对 `m_loadRequestId` 的竞态拦截、`m_isLoading` 加载阻断状态恢复等上下文完全一致，保持原汁原味的磁盘多线程递归与单层扫描能力。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 1:1 绝对镜像 | 在 `ArcMeta.Library_` 下，分类树与物理目录保持 1:1 复刻，物理决定逻辑（对应 Memories 1.2） | ✅ 符合。去耦合后，物理目录导航完全读取物理磁盘本身，百分之百对齐物理决定逻辑。 |
| UI 异步加载与防闪烁 | 异步数据扫描前，禁止调用 `m_model->clear()`，避免抖动并使用 `m_loadRequestId` 守护竞态保护（对应 Memories 8） | ✅ 符合。本方案只对 `loadDirectory` 中的加载源进行去耦合，后续的多线程物理扫描及弱引用保护机制完整保留，防闪烁规范完全不打折扣。 |
| 清除按钮规范 | 每一个输入框必须配置 Qt 原生的 `setClearButtonEnabled(true)` | ✅ 本次分析不涉及输入框或 UI 新增，故不存在违规风险。 |

## 8. 待确认事项（可选）
* **近期访问历史（AutoImportManager::recordRecentVisitedFolder）**：
  即使不切换为内存模式，在物理导航进入托管库时，我们依然建议保留对该物理路径的“最近访问文件夹历史记录（开门即记录）”操作，从而保证侧边栏近期访问计数准确无误。此操作若无特别异议，将在实际编码执行中予以保留。
