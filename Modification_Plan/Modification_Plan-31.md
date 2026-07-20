# CategoryPanel 拖拽搬运决策移出与纯哑侧边栏构建 —— Modification_Plan-31.md

## 1. 任务背景
在《ArchitectureComplianceAudit.md》全量架构合规性审查审计报告中，`CategoryPanel`（判定为 **FAIL** 的第 15 项）作为侧边栏分类树核心面板，本应是 100% 专注于分类视图的展开状态持久化、树形布局展示和选中过滤信号抛出的纯表现层 View 容器。然而，由于历史开发的不断堆叠，它插手了过多复杂的物理磁盘底层搬运策略：在处理拖拽入库（`pathsDropped` 信号回调的 Lambda 段）时，它深度硬编码并混合了物理盘符前缀裁剪、匹配卷序列号以读取 `AppConfig` 的库配置、物理导入冲突过滤判定、乃至直接侵入发令调用底层的物理文件搬运引擎 `ImportHelper::importPaths`。这种强穿透越权行为，破坏了 MVC 经典分层的内聚纯度。为了彻底重构这一典型越权设计，必须将具体的拖拽物理判定和物理执行逻辑全量移出、下沉并剥离。

## 2. 问题定位
- **定位模块 1（拖拽物理规则硬编码越权）**：
  在 `src/ui/CategoryPanel.cpp` 连接 `pathsDropped` 的信号回调（第 1066 行至第 1120 行）内部：
  ```cpp
  connect(m_categoryTree, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& proxyIndex) {
  ```
  该匿名函数直接展开了大量的物理盘符截取 `firstPath.left(3)`、读取 `ManagedFolder/Volume_` 路径信息、对入库冲突进行 `isInsideManagedLibrary` 与 `ingestionStatus == 1` 物理标志拦截、并直接发令拉起：
  ```cpp
  ImportHelper::importPaths(finalPaths, managedRoot, this);
  ```
  这直接在 UI 视图内部深度实现了具体的底层物理决策与调用。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 视图职责绝对纯净化 | 将 `CategoryPanel` 彻底哑化，移除一切物理导入逻辑与盘符匹配，仅保留纯哑的树形侧边栏 UI。 | ✅ 一致 |
| 2    | 物理搬运动作与 MVC 解耦 | 移除硬编码在 View 面板内的导入规则拦截和 ImportHelper 物理搬运调用。 | ✅ 一致 |
| 3    | 异步 Action-Delegate 委托分流 | 拖入事件发生时，UI 仅负责发出事件通知，具体物理校验和搬移执行由外部 Controller 进行中转处理。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 实现事件与执行完全分离（Action-Delegate）
1. **取消物理判定，保持 UI “哑状态”**：
   在 `src/ui/CategoryPanel.cpp` 中，**彻底删除**在连接 `pathsDropped` 时大段关于 `ImportHelper::importPaths` 的物理调用、托管库前缀解析和冲突检测 Lambda。
2. **新增纯哑信号抛出**：
   在 `CategoryPanel.h` 中，新增专门的事件抛出信号：
   ```cpp
   signals:
       /**
        * @brief 当外部文件拖入侧边栏分类树时，视图仅抛出通知，不执行任何物理操作
        * @param paths 拖入的绝对路径列表
        * @param targetCatId 目标分类 ID (空白或顶级为 0)
        */
       void pathsDroppedToCategory(const QStringList& paths, int targetCatId);
   ```
   当 `pathsDropped` 触发时，`CategoryPanel` 仅负责对选中行 target 翻译为 ID，并原地 emit 抛出 `pathsDroppedToCategory(finalPaths, targetCatId)`。

### 4.2 控制层（Controller）代理物理迁移执行
1. **控制器拦截与物理执行**：
   将移出出的物理检查（卷序列号、托管根目录解析）和入库冲突拦截（`meta.ingestionStatus == 1` 过滤），下移或委托上移至 Controller 层（如主控制器或 `DragDropImportHandler` / `MainWindow` 的槽函数）。
2. **连接分流槽逻辑**：
   控制器监听 `pathsDroppedToCategory` 信号并执行内聚的物理动作：
   ```cpp
   // 控制器在连接时：
   // 1. 进行 Ingestion 冲突校验与 tooltip 弹出
   // 2. 解析 Volume Serial 与 ManagedFolder 路径
   // 3. 最终发令调用 ImportHelper::importPaths
   ```
   如此一来，复杂的物理搬运机制和 NTFS 文件决策退化为典型的 Controller 逻辑，表现层不参演任何磁盘层面的搬运行为。

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/file：
  - `src/ui/CategoryPanel.h` / `src/ui/CategoryPanel.cpp` （新增 `pathsDroppedToCategory` 哑分发通道；彻底移除 pathsDropped 内部直接拉起 ImportHelper 及物理盘符检索的越权 Lambda，改由 emitter 一句抛出）

**明确禁止越界修改的范围：**
- [ ] 明确禁止改动 `CategoryPanel` 现有的展开/折叠状态持久化记忆读写（`loadExpandedStateFromSettings`——不修改）。
- [ ] 明确禁止对 `m_categoryTree` 树节点展开逻辑及 `CategoryFilterProxyModel` 本地过滤的改动。

## 6. 实现准则与预警【核心】
1. **防止槽连接失效（Connection Health）**：重构后必须保证 MainWindow 或控制器在实例化 `CategoryPanel` 后，在 `deferredInit` 周期内正确连接 `pathsDroppedToCategory` 信号，确保物理入库动作的开箱即用。
2. **避免重复入库提示闪烁**：搬运判定在 Controller 层面执行时，ToolTip 提示框的坐标显示应适配当前鼠标悬停区域（`QCursor::pos()`），确保极佳的表现层连贯性。
3. **消除头文件粘滞**：将 `#include "ImportHelper.h"` 从 `CategoryPanel.cpp` 彻底摘除，隔离 UI 面板对 Utility 磁盘迁移层的非必要编译强耦合。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 `Modification_Plan-31.md` |
| 考古原则 | 重构代码必须基于现有实现保持高度的代码整齐度与风格一致性 | ✅ 符合，新定义的信号和回调分发风格完全尊重现有代码习惯 |
| MVC 分离 | 表示层 UI Panels 绝对禁止直接插手或硬编码具体的底层 NTFS 磁盘操作 | ✅ 符合，将物理迁移归拢彻底交由控制器和 DataService 中继执行 |

## 8. 待确认事项（可选）
- **拖入空白处分流默认规则**：按照目前设计，若拖至侧边栏空白处（`targetCatId = 0`），其归入“我的分类”顶级目录。重构后该逻辑将由控制器统一对齐维持不变。
