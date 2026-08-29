# UI 视觉与度量设计规范 (UI_DESIGN_SPEC.md)

---

## 1. 颜色与主题规范 (Color System)

| 色彩语义 | 十六进制色值 | 使用场景 / 控件约束 |
| :--- | :--- | :--- |
| **品牌主色 (Brand Orange)** | `#FF551C` | Logo 高亮、选中标注、顶级激活状态 |
| **数据库/主主题色 (Primary Blue)**| `#4A90E2` | 标题栏焦点、活动选择框、按钮聚焦边框 |
| **深色背景 (Background Deep)** | `#1E1E1E` | 中央主工作区、属性面板背景 |
| **深色集区背景 (Background Dark)** | `#252526` | 侧边栏（NavPanel / FavoritePanel / FilterPanel）背景 |
| **卡片/浮层背景 (Layer 3)** | `#2D2D2D` | 托盘菜单、弹出下拉框、ContextMenu 菜单项背景 |
| **边框颜色 (Border Color)** | `#333333` / `#444444` | Splitter 分隔线、编辑框边框 |
| **主文字颜色 (Text Main)** | `#EEEEEE` | 核心标题、文件名文本 |
| **暗色/次要文本 (Text Muted/Dim)** | `#888888` / `#AAAAAA` | 面包屑箭头、文件属性辅助文字 |
| **错误/成功颜色 (Error / Success)** | `#E81123` / `#1ABC9C` | 物理删除操作、快捷收藏成功反馈 |
| **PSD 标签配色 (PSD Badge)** | 背景色 `#001D26` / 字体色 `#02B1DD` | Photoshop 文件 Badge 专属配色 |
| **EPS 标签配色 (EPS Badge)** | 背景色 `#35483D` / 字体色 `#F88025` | EPS 矢量文件 Badge 专属配色 |
| **AI 标签配色 (AI Badge)** | 背景色 `#F88025` / 字体色 `#35483D` | Illustrator 文件 Badge 专属配色 |
| **其他扩展名 (Other Badges)** | `global.db` 持久化表 / 动态生成刷盘 | 非硬编码扩展名持久化固化于 `global.db` `extension_colors` 表 |
| **收藏夹默认主题色 (Favorite Color)** | `#FDB70A` | 收藏夹矢量图标与标题默认金色 |

---

## 2. 布局尺寸与度量规范 (Layout Metrics)

| 布局属性 / 控件名称 | 度量数值 | 约束说明 |
| :--- | :--- | :--- |
| **五栏面板默认/最小宽度** | `230 px` | NavPanel / FavoritePanel / MetaPanel / FilterPanel 的最小分配宽度 |
| **边缘拉伸感应热区 Margin** | `6 px` (按 DPI 自动放缩) | `ResizeEventFilter` 无边框拉伸触发热区宽度 |
| **标题栏物理高度 (TitleBar)** | `34 px` | 顶部无边框标题栏固定高度 |
| **驱动/导航栏物理高度** | `42 px` | DriveBar / AddressBar 固定高度 |
| **底栏/状态栏物理高度** | `28 px` | StatusBar 底部固定高度 |
| **预览缩略图标准尺寸** | `240 x 160 px` | MetaPanel 顶部文件缩略图保持长宽比缩放尺寸 |
| **面包屑矢量箭头尺寸** | `12 x 12 px` | `chevron_right` 矢量 SVG 箭头像素尺寸 |
| **评级/清除按钮标准尺寸** | `22 x 22 px` | 星级评级按钮与 `no_color` 清除按钮控件尺寸 |

---

## 3. 交互与反馈停留时间规范 (Interaction Timings)

| 交互类型 / 控件 | 停留/延迟时长 | 行为规范说明 |
| :--- | :--- | :--- |
| **撤销/反馈 Snackbar (UndoToastOverlay)** | **7000 ms (7 秒)** | 统一固定停留时长，确保用户具备充足的操作与撤销时间窗口 |
| **轻量级 Hover 提示 (ToolTipOverlay)** | `1500 ms` | 快捷操作反馈提示自动消失时长 |
| **搜索框防抖延迟 (Search Debounce)** | `300 ms` | 键盘输入停止后触发模糊过滤搜索的防抖时间窗口 |
| **选区变更防抖延迟 (Selection Debounce)** | `50 ms` | 避免多选/框选时连续触发 UI 重计算的防抖窗口 |
