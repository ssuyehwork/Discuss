# QuarkMeta 视图层“重复造轮子”与架构归一化全面排查报告

**排查日期**：2026-09-20
**架构师/审查员**：Jules
**排查范围**：`src/ui/` 目录下全部视图控件（View）、渲染代理（Delegate）、布局容器（Container）及控制协调层（Controller/Mediator）

---

## 一、 排查背景与核心判定依据

为彻底消除打补丁、另起炉灶与分散实现的隐患，根据 **`AGENTS.md` 逻辑归一化重构规范（Section 5）**、**`SYSTEM_PROMPT.md`【五道工程硬锁】** 及 **`Memories.md`** 中的各模块契约，对全项目所有视图控件进行了全盘普查。

**归一化/重复造轮子判定三大标准**：
1. **概念同一性**：多处代码是否在做同一件概念上的事，仅细节参数或调用位置不同？
2. **潜在一致性 Bug**：改动一处，另一处未同步修改，是否会导致交互表现脱节或行为不一致？
3. **扩展冗余性**：新增同类场景时，是否需要再次照抄手写一份现有实现？

---

## 二、 六大核心横切通用行为排查对照表

| 横切行为/功能模块 | 标准 SSOT 通道/规范 | 统一接入良好模块 | 存在分散/重复造轮子隐患模块 | 违规细节与风险分析 |
| :--- | :--- | :--- | :--- | :--- |
| **1. 右键上下文菜单** | **`ContextMenuFactory` + `ContentContextMenu`** | `ContentPanel` (网格/列表/瀑布流/分栏), `NavPanel` | `FavoritePanel` (`onFavoriteContextMenu`), `TagManagerDialog` | `FavoritePanel` 内部手写 `QMenu` 并逐个添加 Action，未接入 `ContextMenuFactory` 通用 Action 工厂；存在图标样式与 10px 间距未统一风险。 |
| **2. 跨视图拖拽与悬停高亮** | **`ViewDragDropHelper` + `Drop*View`** | `DropJustifiedView`, `DropTreeView`, `DropListView` | `ColumnViewWidget` (`ColumnBlankCanvasWidget`) | `ColumnBlankCanvasWidget` 内部手写了 `dragEnterEvent` 与 `dropEvent` 拖拽接受逻辑，未完全封装至 `ViewDragDropHelper`。 |
| **3. 视图数据变动/文件操作刷新** | **`ContentPanel::refreshAll()`** | `ContentFileOpsHandler`, `ContentKeyHandler`, `PanelMediator` | 局部 Controller 残留 `loadDirectory()` 强制重置 | `ContentContextMenu` 中在某些操作完成后误调用了 `loadDirectory(curDir)` 替代 `refreshAll()`，会导致当前视图视角/展开列被强制重置弹回。 |
| **4. 行内重命名与编辑器** | **`RenameCapableDelegate` + `FileNameLineEdit`** | `ThumbnailDelegate`, `TreeItemDelegate`, `ColumnItemDelegate` | `FavoriteItemDelegate` (无编辑能力) | 核心三主视图代理已 100% 归一化继承 `RenameCapableDelegate`，`FavoritePanel` 使用独立的 `QStyledItemDelegate` 但不需重命名，符合架构分工。 |
| **5. 选择集同步与属性面板路由** | **`PanelMediator` (中介防抖路由)** | `GridView`, `ListView`, `JustifiedView`, `ColumnView` | 早期残存视图直接连信号 | 部分叶子控件存在直接 `selectionChanged` 监听，但已大部分被 `PanelMediator` 统一代理与防抖缓冲。 |
| **6. 文本框专属暗色右键菜单** | **`UiHelper::applyMenuStyle` + `QLineEdit` 右键拦截** | `TagSelectorOverlay`, `FilterPanel` 输入框 | `AddressBar` (`QLineEdit`) 部分场景 | `UiHelper::setupLineEditContextMenu` 已提供统一入口，少数自定义对话框输入框未显式安装。 |

---

## 三、 普查发现的 4 处重点归一化重构点与消除方案

### 1. 右键菜单构建工厂归一化（`ContextMenuFactory` 扩充）
- **现象描述**：`FavoritePanel`（收藏夹）中的右键菜单在 `FavoritePanel::onFavoriteContextMenu` 内部通过原生 `new QMenu` 手写 Action 构建（“移除收藏”、“重命名”、“在资源管理器中打开”）。
- **隐患分析**：违规绕过了 `ContextMenuFactory`，当后续调整菜单视觉样式、100% 语义矢量图标或 10px 图文间距时，`FavoritePanel` 会成为漏网之鱼。
- **重构建议**：将收藏项的右键菜单 Action 构建抽离并补充至 `ContextMenuFactory::buildFavoriteContextMenu(...)`，实现右键菜单 100% 统一管理。

### 2. `ContentContextMenu` 刷新通道归一化（`refreshAll()` 彻底替代 `loadDirectory()`）
- **现象描述**：在 `ContentContextMenu.cpp` 的部分回调（如文件重命名、属性修改后）中，仍有 3 处使用 `loadDirectory(curDir)`。
- **隐患分析**：违反 `Memories.md` 2.4 节【核心通用行为 SSOT 入口字典】铁律。使用 `loadDirectory()` 会导致分栏视图（Miller Columns）已展开的子列被销毁重置，损害用户体验。
- **重构建议**：将这 3 处回调中的 `loadDirectory(curDir)` 统一替换为 `refreshAll()` 原位刷出变动。

### 3. 分栏视图空白留白区拖拽与右键路由收敛
- **现象描述**：`ColumnViewWidget.cpp` 中的 `ColumnBlankCanvasWidget` 拥有自己的 `dragEnterEvent` / `dropEvent` 及右键菜单槽函数。
- **隐患分析**：属于“为了适应留白区而手写的一套平行事件处理”。
- **重构建议**：将留白区的事件响应统一路由回 `ContentPanel::onPathsDropped` 与 `ContentPanel::onCustomContextMenuRequested`，剥离留白区自身的业务判断逻辑。

### 4. 文本输入框上下文菜单（App-Exclusive LineEdit Context Menu）100% 覆盖自查
- **现象描述**：全软件输入框绝大部分已接入 `UiHelper::setupLineEditContextMenu`，但少数对话框（如 `FramelessInputDialog`）在新建时未自动应用该拦截。
- **隐患分析**：在特定弹窗输入框中右键仍可能弹出 Win32 原生英文菜单，破坏“100% QuarkMeta 统一暗色视觉体验”。
- **重构建议**：在 `UiHelper` 或 `FileNameLineEdit` 中提供构造自动挂载，或在输入框基类中统一拦截。

---

## 四、 归一化总结与架构健康度评估

目前 QuarkMeta 视图层整体架构健康度良好：
1. **行内重命名框架**已通过 `RenameCapableDelegate` + `FileNameLineEdit` 实现了 100% 的统一密封；
2. **全局事件过滤**已收敛至 `QuarkApplication`；
3. **分栏视图与主视图**已通过 `PanelMediator` 实现中介解耦。

上文列出的 4 处微小离散点均属于历史演进过程中残留的孤立代码，后续只需在对应的物理隔离实施方案中按“五步归一化规范”逐步清理即可。
