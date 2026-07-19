# ContentPanel 越权访问剥离与纯 UI 表示层解耦 —— Modification_Plan-28.md

## 1. 任务背景
在《ArchitectureComplianceAudit.md》架构合规性审计报告中，`ContentPanel`（判定为 **FAIL** 的第 5 项，且命中条件 1 “职责不单一”与条件 4 “越权越界访问细节”）作为核心表现层 UI Panel，除了承载标准视图代理与展示工作外，还深度混杂了多项中后台核心业务细节。它直接在右键菜单中调用 `MetadataManager::isInsideManagedLibrary` 判断当前路径是否处于托管库内部；在目录加载（`loadDirectory`）过程中擅自越级调用 `AutoImportManager::recordRecentVisitedFolder` 将导航记录持久化落盘；甚至在 Model 提取数据和元数据更新时直接获取底层 `RuntimeMeta` 内存镜像并强耦合拆解其私有属性。这种“越权越界访问”使 View 层（表现层）与 Model/Cache 层（数据层）深度咬死。一旦底层 `RuntimeMeta` 的成员字段增减或同步模型变动，`ContentPanel` 就会发生大范围编译断裂，严重违背了 MVC 经典分层依赖。为此，必须对该越权穿透进行优雅剥离。

## 2. 问题定位
- **定位模块 1（导航历史落盘强耦合）**：
  在 `src/ui/ContentPanel.cpp` 的 `ContentPanel::loadDirectory` 函数（第 2906 行）中，在进入新物理路径时，直接调用了 `AutoImportManager::recordRecentVisitedFolder(...)`，让 UI 组件直接发令去写数据库/配置历史，破坏了分层纯净。
- **定位模块 2（右键业务策略硬编码穿透）**：
  在 `ContentPanel::onCustomContextMenuRequested` 函数（第 2042 行与第 2200 行）中，为了获取当前目录是否属于“镜像源”或“托管库”，直接穿透调用了 `MetadataManager::isInsideManagedLibrary`，使得 View 直接感知底层的库范围管理细节。
- **定位模块 3（内存镜像字段拆解越权）**：
  在 `FerrexVirtualDbModel::updateRecordMetadata`（第 434 行）与 `ContentPanel::createItemRecord`（第 980 行）中，大量调用 `MetadataManager::instance().getMeta(path)` 并在 UI/Model 逻辑中拆解其 rating, color, tags, fileId128, pinned, encrypted 等状态，形成了大面积编译脆弱点。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 消灭越权直接访问，实现 Model/Delegate 彻底代理化 | 彻底废除 `ContentPanel` 内部直接获取 `RuntimeMeta` 内存镜像并自行拆解其内部私有成员的做法。设计高内聚属性代理方法，将元数据属性表现交由标准的 Model `data()` 及代理进行中转获取，UI 仅做几何选中与选中行分发。 | ✅ 一致 |
| 2    | 物理导航历史与业务剥离（MVC 分层隔离） | 将导航目录历史落盘（`recordRecentVisitedFolder`）移出 `ContentPanel`，在 UI 载入时通过已有的信号链（如 `directorySelected(path)`）由主控制器或 `CoreController` 进行拦截和状态落盘。 | ✅ 一致 |
| 3    | 右键业务策略分流 | 废除右键菜单中硬编码的底层托管库判断。将是否在库内、是否支持“同步/重新扫描”动作的处理，封装为基于当前 Model 行数据的解耦映射，解开对底层 `MetadataManager` 物理镜像的直接穿透。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 物理导航历史与业务剥离
1. **取消 View 直写历史逻辑**：
   在 `ContentPanel::loadDirectory` 函数内部，彻底删除以下硬编码物理落盘命令：
   ```cpp
   // 废除：AutoImportManager::recordRecentVisitedFolder(path.toStdWString());
   ```
2. **控制器统一收拢落盘**：
   `ContentPanel` 在加载目录后已触发信号抛出。我们应该在主导航路由或控制器中（如 `CoreController` 或 `MainWindow` 在连接 `directorySelected` / `directoryLoaded` 信号时），统一执行落盘操作，确保表现层处于 100% 的“哑状态”。

### 4.2 越权属性直接拆解剥离，实现 Model 代理化
1. **重构 `updateRecordMetadata`**：
   在 `FerrexVirtualDbModel::updateRecordMetadata` 内部，避免由 Model 直接遍历拆解 `RuntimeMeta` 的成员：
   ```cpp
   // 重构前：
   // auto meta = MetadataManager::instance().getMeta(nPath.toStdWString());
   // m_allRecords[i].rating = meta.rating;
   // m_allRecords[i].color = ...
   ```
   引入统一的 `ModelContract` 桥接或将属性复制行为直接封装进 `ItemRecord` 的静态方法（如 `ItemRecord::fromMetadata`）。
2. **规范 `createItemRecord` 元数据流向**：
   在 `ContentPanel::createItemRecord`（第 980 行）中，对于没有提供 `providedMeta` 时的底层穿透，将加载逻辑下沉，不在 UI 类内部执行带有磁盘 I/O 的 Windows API 的属性采样：
   ```cpp
   // 拆分与下沉逻辑，确保 ContentPanel 仅作为展现行的物理容器
   ```

### 4.3 右键菜单托管判断解耦（右键业务策略分流）
1. **使用 Model 数据源隔离**：
   在 `onCustomContextMenuRequested` 中判断是否在库内：
   ```cpp
   // 废除直接穿透：
   // isMirrorSource = MetadataManager::isInsideManagedLibrary(m_currentPath.toStdWString());
   ```
   统一通过当前 `m_model`（或通过 `FilterProxyModel` 获取源索引）的数据源角色（如专有的 `LibraryStatusRole` 或 `IsManagedRole`）来返回此状态。Model 本身持有这些信息，View 层直接向 Model 提问即可。

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/file：
  - `src/ui/ContentPanel.cpp` （移出 `AutoImportManager::recordRecentVisitedFolder` 调用；废除右键菜单中直接穿透的 `MetadataManager::isInsideManagedLibrary`，改为向 Model 接口提问；重置 `createItemRecord` 中的属性注入）
  - `src/ui/ContentPanel.h` （解耦头文件中对 `MetadataManager.h` 物理引用的依赖）

**明确禁止越界修改的范围：**
- [ ] 明确禁止改动内容面板的双向多列过滤及列表/网格的三大视图切换排版（JustifiedResultView 拼图——不修改）。
- [ ] 明确禁止破坏 `appendPaths` 中异步搜索流式返回时的并发安全机制。

## 6. 实现准则与预警【核心】
1. **消除编译粘滞**：在 `ContentPanel.h` 中，解除 `#include "../meta/MetadataManager.h"` 的紧密物理依赖，使用 `struct RuntimeMeta;` 进行前向声明，防止对 `ContentPanel.h` 的 include 导致编译传播，缩短构建耗时。
2. **MVC 分层闭环**：在移除落盘后，必须同步核对 `MainWindow` 的槽函数，确保导航落盘由 Controller 统一接管，做到开箱即用。
3. **防止越权自检**：重构完成后，应自检一遍 `ContentPanel` 是否仍保留任何对 `MetadataManager` 私有成员、物理 I/O 修改的直接调用，彻底防止发生脑补越权。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 `Modification_Plan-28.md` |
| 考古原则 | 重构代码必须基于现有实现保持高度的代码整齐度与风格一致性 | ✅ 符合，采用标准 Qt 信号槽和 Model-View Role 状态返回进行重写 |
| 越权判定 | 彻底根绝 View 层对物理数据库的直接 API 调用与大锁强耦合 | ✅ 符合，将物理落盘及托管库检测彻底代理化 |

## 8. 待确认事项（可选）
- **落盘逻辑上移接管位置**：对于 `AutoImportManager::recordRecentVisitedFolder` 的调用，目前建议在 `MainWindow` 或 `CoreController` 中拦截 `directorySelected` 信号后统一处理，已在方案 4.1 中规划。
