# Memories.md Multi-Tab & Split View Architecture Guidelines Implementation Plan

## 1. Overview
Add Section 12 to `Memories.md` documenting the top-level design principles, interaction rules, shortcut contracts, and architecture specifications for the Multi-Tab Bar (`TabBarWidget`) and Dual-Pane Split View (`ContentPanel`) system.

## 2. Modified Files List
- `Memories.md`

## 3. Detailed Line-by-Line Changes

### 3.1 `Memories.md`
Add Section 12 at the end of `Memories.md`.

```
<<<<<<< SEARCH
   - **智能扩展名保护与按键流转**：分栏视图触发行内重命名时，获取焦点的编辑器必须具备“文件只高亮选中主文件名/自动避开扩展名，文件夹全选”的智能选区逻辑，且必须完整配备统一的按键拦截处理（阻断上下方向键导致 View 焦点漂移，优化左右方向键定位至基名末端）；
   - **应用专属右键菜单与几何对齐**：行内编辑器必须严格遵守系统专属暗色右键菜单契约（带 100% 语义匹配单色矢量图标与 10px 间距），其渲染几何区域必须精确定位在左侧 32px 留白与右侧 22px 级联指示器箭头之间，确保全视图绝对一致的重命名体验与架构纯洁性。
3. **具备重命名能力的统一代理基类契约 (RenameCapableDelegate Base Contract)**：
   - 全软件所有需要行内重命名能力的视图渲染代理（包括树状/列表视图代理 `TreeItemDelegate`、网格卡片视图代理 `ThumbnailDelegate` 及分栏视图代理 `ColumnItemDelegate` 等），必须统一继承抽象基类 `RenameCapableDelegate`；
   - **编辑生命周期强制统一与编译器锁**：基类 `RenameCapableDelegate` 统一实现并用 `override final` 密封 `createEditor`、`setEditorData` 与 `setModelData` 虚函数。所有具体子类 Delegate 绝对禁止且无法重新覆盖这三个函数，确保全软件重命名编辑框的创建、数据填充与模型提交逻辑 100% 绝对一致；
   - **几何边界隔离**：编辑框的布局呈现与定位边界（`updateEditorGeometry`）保留为子类虚函数，由各视图代理根据各自的卡片/行数物理布局引擎进行针对性精准绘制，实现架构统一与布局灵活度的完美结合。
=======
   - **智能扩展名保护与按键流转**：分栏视图触发行内重命名时，获取焦点的编辑器必须具备“文件只高亮选中主文件名/自动避开扩展名，文件夹全选”的智能选区逻辑，且必须完整配备统一的按键拦截处理（阻断上下方向键导致 View 焦点漂移，优化左右方向键定位至基名末端）；
   - **应用专属右键菜单与几何对齐**：行内编辑器必须严格遵守系统专属暗色右键菜单契约（带 100% 语义匹配单色矢量图标与 10px 间距），其渲染几何区域必须精确定位在左侧 32px 留白与右侧 22px 级联指示器箭头之间，确保全视图绝对一致的重命名体验与架构纯洁性。
3. **具备重命名能力的统一代理基类契约 (RenameCapableDelegate Base Contract)**：
   - 全软件所有需要行内重命名能力的视图渲染代理（包括树状/列表视图代理 `TreeItemDelegate`、网格卡片视图代理 `ThumbnailDelegate` 及分栏视图代理 `ColumnItemDelegate` 等），必须统一继承抽象基类 `RenameCapableDelegate`；
   - **编辑生命周期强制统一与编译器锁**：基类 `RenameCapableDelegate` 统一实现并用 `override final` 密封 `createEditor`、`setEditorData` 与 `setModelData` 虚函数。所有具体子类 Delegate 绝对禁止且无法重新覆盖这三个函数，确保全软件重命名编辑框的创建、数据填充与模型提交逻辑 100% 绝对一致；
   - **几何边界隔离**：编辑框的布局呈现与定位边界（`updateEditorGeometry`）保留为子类虚函数，由各视图代理根据各自的卡片/行数物理布局引擎进行针对性精准绘制，实现架构统一与布局灵活度的完美结合。

---

## 12. 多标签页 (Multi-Tab Bar) 与 双窗格 (Dual-Pane Split View) 顶层交互与架构规范 (Multi-Tab & Split View Contract)

为确保 QuarkMeta 在多标签页浏览与双窗格分栏操作中具备现代浏览器级的极致体验与工业级高内聚架构，全软件必须遵循以下规范：

1. **标题栏内嵌多标签页组件 (`TabBarWidget`) 架构**：
   - **标题栏植入与软硬件边距**：多标签页控件 (`TabBarWidget`) 直接内嵌于标题栏 (`TitleBarWidget`) 中间区域，替换原静态应用标题，物理高度锁定为 28px，保持与操作系统控制按钮精致对齐；
   - **标签生命周期与持久化**：全面支持新建标签（`Ctrl+T`）、关闭当前/其他/左侧/右侧标签、复制标签、恢复刚关闭标签（`Ctrl+Shift+T`）及持久化存储（`AppConfig`）；支持全局快捷键切换（`Ctrl+1~9`、`Ctrl+Tab`、`Ctrl+Shift+Tab`）；
   - **交互形态定制**：支持每个标签页单独配置专属色彩标签与矢量图标（选色器/图标网格子菜单），物理存储与状态无缝联动。

2. **双窗格分栏与工具栏按钮契约 (`ContentHeaderWidget` Split Button Contract)**：
   - **分栏按钮位置**：在主/副内容面板顶部 `ContentHeaderWidget` 工具栏右侧嵌入 `columns` 图标按钮 (`m_btnSplitView`)；
   - **单/双窗格动态流转**：在单窗格模式下点击分栏按钮，自动触发 horizontal 双窗格分栏，并优先从 `NavigationHistoryService` 读取上一次打开的文件夹路径加载至副窗格；在双窗格模式下点击分栏按钮或副窗格分栏按钮，自动收起关闭副窗格复原单窗格视角；
   - **等权路由与面板激活**：主/副内容面板处于 50/50 平等权重，通过 `panelActivated` 信号动态绑定 `m_activeContentPanel`，确保地址栏导航、快捷键与菜单命令始终精准路由至当前获得焦点的活动面板。
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Verify `Memories.md` formatting and markdown validity.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Documented SSOT rules for `TabBarWidget`, `TitleBarWidget`, `ContentHeaderWidget`, and `ContentPanel`.

## 6. Header API Signature Verification
- N/A (Documentation file).
