# 开发需求台账 Development_Plan.md

## [2024-07-20] 扩大范围排查当前版本的整体逻辑架构

- 用户描述的现象/问题：当前版本的整体逻辑架构可能存在职责过载、运行流程上下文冲突、重复造轮子、性能问题（死循环、线程竞争、假死、卡顿）以及线程相互干扰。
- 用户期望的结果：扩大范围排查这些潜在架构及运行问题，并将排查到的结果详细记载到 `Inspect and Mark.md` 文档里。
- 本次任务边界：对当前版本的整体逻辑架构展开全面的代码审计与静态排查，针对指定5点列出详细排查结果，格式化输出到 `Inspect and Mark.md` 中。因为当前角色是分析师，仅对代码逻辑及架构进行审计分析，不修改任何功能性代码文件。
- 不在本次范围内的是：实际修改代码或引入任何功能性代码的改动。
- 对应方案文档：无（本次为分析委托，直接产出 `Inspect and Mark.md` 文档，不需要单独编写 `Modification_Plan-N.md` 并执行代码修改，因为没有代码修改需求）

## [2026-07-20] 缩略图“先显示后秒消失”故障分析与自适应缓存容量优化

- 用户描述的现象/问题：在内容面板中显示数据时（尤其是SVG或图像等图形文件），缩略图会先短暂显示，随后在1秒内瞬间消失并退化为纯灰色占位符。
- 用户期望的结果：分析该现象并给出彻底根治的逻辑架构方案，同时评估 QCache 的 LRU 淘汰机制是否为故障根因，重新设计一套能与虚拟滚动可见区域动态关联、避免在常见规模文件夹下产生缩略图淘汰的自适应动态缓存容量公式，并在超大/百万级文件夹下做合理的防御性妥协。
- 本次任务边界：对 `src/ui/ContentPanel.cpp` 中的 `FerrexVirtualDbModel` 缩略图加载与缓存机制进行根因定位。定位并重构后台提取线程中直接操纵 GUI 敏感 `QPixmap` 导致的硬件句柄失效 Bug。在 `setRecords`（初次加载）与 `loadThumbnailsForRows`（滚动视口刷新）双阶段中同步引入自适应 `maxCost` 计算逻辑，确保无损与防御性限幅。
- 不在本次范围内的是：修改物理 MFT 扫描、USN 监控底座以及其他非缩略图相关的逻辑。
- 对应方案文档：Modification_Plan-33.md

## [2026-07-20] 内容面板视图模式对齐重构与自定义调整滑杆及视图按钮移植

- 用户描述的现象/问题：当前版本现有的三个视图模式没有达到预期；且原有Ctrl+滚轮缩放兼切换多视图的逻辑未达预期；同时顶部缺少方便调节卡片尺寸 and 快速切换视图排列方式的控件。
- 用户期望的结果：在当前版本中移植并实现类似于 FERREX-META 版本的“调整卡片尺寸的滑杆（m_sizeSlider）”和“排列方式 of 视图按钮（viewBtn）”；完全放弃原有Ctrl+滚轮多级缩放切换视图的逻辑；且参照 FERREX-META 的实现彻底重构移植三个视图模式（列表、自适应、网格），确保职责单一模块化。
- 本次任务边界：重写并替换当前版本的 `ListResultView`、`GridResultView`、`JustifiedResultView` 等视图接口与实现，剔除不合预期的多级滚轮缩放切换逻辑；并在 `ContentPanel` 顶部标题栏区域中嵌入 `m_sizeSlider` 与 `viewBtn` 控件，实现卡片尺寸 32~256 像素无缝调节及列表/自适应/网格三种排版模式的一键式极速切换。
- 不在本次范围内的是：修改 MFT / USN 底层扫描或数据库检索流程，移植与视图显示无关 of 外部控制器组件。
- 对应方案文档：Modification_Plan-34.md

## [2026-07-20] 三种视图架构设计缺陷排查与极致重构规划

- 用户描述的现象/问题：当前版本中的三种视图（列表、网格、合理自适应对齐模式）存在逻辑冗余、职责过载以及交互/渲染卡顿等傻逼逻辑架构问题。
- 用户期望的结果：对三种视图底层控制链展开深度排查，找出不合理的架构缺陷并形成极致解耦与性能提升的优化方案，并产出详细设计。
- 本次任务边界：审计并理清 `JustifiedView`、`ListResultView`、`GridResultView`、`JustifiedResultView` 以及 `ContentPanel` 之间在视图控制、数据映射 and 布局渲染上的依赖关系，设计职责更清晰、耦合度更低的模块化结构。
- 不在本次范围内的是：实际修改或编译代码，移植非视图类的其他外部功能。
- 对应方案文档：Modification_Plan-35.md

## [2026-07-20] 彻底放弃当前 QuickLookWindow 运行逻辑并使用 FERREX-META 逻辑替代重构

- 用户描述的现象/问题：当前版本的 `QuickLookWindow` 预览运行逻辑不符合预期，其处理逻辑和界面交互（如图像缩放、音视频媒体处理、文本二进制检测及编码识别等）功能不够完备。
- 用户期望的结果：彻底放弃当前版本的 `QuickLookWindow` 运行逻辑，将其替换为 `FERREX-META` 版本中的 `QuickLookWindow` 运行逻辑。这包括采用专门设计的 `QuickLookGraphicsView`（支持滚轮缩放、双击恢复/适配大小、鼠标拖拽移动及光标跟随变动）、引入更完整的多媒体及文本二进制与编码自动检测识别渲染，实现更好的大文件和特殊文件类型的预览。
- 本次任务边界：彻底重构 `src/ui/QuickLookWindow.h` and `src/ui/QuickLookWindow.cpp`，将 `FERREX-META` 版本的 `QuickLookWindow` 和 `QuickLookGraphicsView` 控制与交互机制移植并合并至其中，适配 `ArcMeta` 命名空间及符合本项目标准的样式设计。同时确保当前项目正在运行的核心快捷键/信号链路继续工作（评分打标 `ratingRequested`、颜色标签打标 `colorRequested` 以及切图 `prevRequested`/`nextRequested` 信号），并对音视频进行优雅的占位降级兼容。
- 不在本次范围内的是：修改 `MainWindow` 的主导航调度机制 or 任何非预览关联的面板组件。
- 对应方案文档：Modification_Plan-36.md

## [2026-07-21] 预览界面滚动条样式对齐考古规范优化

- 用户描述的现象/问题：按下空格键打开预览界面后的滚动条样式，违背了考古，由上一代 AI 导致。
- 用户期望的结果：将预览界面（QuickLookWindow）内的滚动条样式彻底重构，使其严格遵循全局规范：宽度 10px、圆角 3px、背景透明、Handle 颜色对齐 `BorderColor` #333333。
- 本次任务边界：修改 `src/ui/QuickLookWindow.cpp` 中 `QuickLookGraphicsView` 和 `QPlainTextEdit` 控件的水平及垂直滚动条的 QSS 样式。
- 不在本次范围内的是：修改预览界面其他的图片/文本渲染、缩放或事件分流逻辑。
- 对应方案文档：Modification_Plan-37.md

## [2026-07-21] 预览界面多媒体残留代码彻底物理根除与净化

- 用户描述的现象/问题：上一代 AI 在收到彻底根除多媒体播放功能的指令后，依然在 `QuickLookWindow` 中保留了多媒体播放及占位相关冗余代码。
- 用户期望的结果：彻底删除并根除 `QuickLookWindow` 中的所有多媒体组件（如播放、暂停、进度条、多媒体相关容器控件和占位框等）和无用快捷键与接口，音视频转走标准系统大图标降级展示。
- 本次任务边界：彻底删除 `src/ui/QuickLookWindow.h` 与 `src/ui/QuickLookWindow.cpp` 中的所有多媒体成员变量、宏定义、多媒体容器构建、以及 `renderMedia`、`resetMedia`、键盘 `P` 按键处理逻辑等。
- 不在本次范围内的是：修改预览界面其他的图片/文本渲染、缩放或事件分流逻辑。
- 对应方案文档：Modification_Plan-38.md

## [2026-07-21] 预览窗快捷键 ToolTip 屏幕上方居中淡进淡出显示、切图/导航加固及 ContentPanel 选中联动

- 用户描述的现象/问题：
  1. 在执行“QuickLookWindow”里的评分（1-5）和颜色标记（Alt+1-9）快捷键时，其 ToolTip 提示显示在偏角（QPoint(50,50)），而不是靠齐屏幕上方居中，且不具备优雅的淡进淡出（Fade In/Out）效果。
  2. 切图/导航功能（Up/Left 切上一张，Down/Right 切下一张）失效，按下按键无响应。
- 用户期望的结果：
  1. 支持将 ToolTip 提示在屏幕上方居中且自带淡进淡出的视觉效果展现出来。
  2. 修复切图/导航快捷键响应。按下 Arrow keys 时能精准跳转上一个或下一个文件，并让主界面的 ContentPanel 对应的高亮选中项也同步更新滚动。
- 本次任务边界：
  1. 在 `ToolTipOverlay` 中引入 QPropertyAnimation，实现 `windowOpacity` 属性动画（淡进淡出 150ms 优雅呈现），并新增 `exactPosition` 精确定位参数。
  2. 在 `QuickLookWindow::eventFilter` 中加装事件过滤器物理拦截 `m_graphicsView` 和 `m_textEdit` 的 `KeyPress` 动作，拦截 arrow keys、数字、Alt 等核心快捷键并转发分发给 `keyPressEvent`。
  3. 在 `ContentPanel` 中新增 `selectAndScrollToPath(path)` 方法，保持主视图选中态随切图进行双向联动。
- 不在本次范围内的是：重构预览界面或主界面的非快捷键/切图逻辑。
- 对应方案文档：Modification_Plan-39.md

## [2026-07-21] ThumbnailDelegate 职责过载审计与模块化拆分规划

- 用户描述的现象/问题：单元格展示委托 `ThumbnailDelegate` 存在多维重绘、生命期、排版和提示的职责过载，难以维护及复用。
- 用户期望的结果：对其进行代码职责过载分析并输出模块化的单一职责拆解和落地规划。
- 本次任务边界：对 `src/ui/ThumbnailDelegate.h` 与 `src/ui/ThumbnailDelegate.cpp` 展开整体架构梳理，定位其过载现状，设计面向高内聚、零开销、低耦合的模块化（视觉物理渲染、文本排版、控制器生命期分流）拆分图纸。由于本 Turn 为只读分析师模式，不修改任何项目代码文件。
- 不在本次范围内的是：修改逻辑代码、在外部执行构建验证。
- 对应方案文档：Modification_Plan-40.md

## [2026-07-22] 列表视图最左侧微型缩略图卡片自适应显示与大图渲染优化

- 用户描述的现象/问题：当前版本列表视图中最左侧缩略图卡片里的显示效果不符合预期。图片的缩略图（如psd、jpg等）在卡片里是以常规小图标的形式呈现，导致卡片中留存大量灰色背景，且整体缩略图被缩得极小，无法凸显大卡片圆角预览的视觉效果。
- 用户期望的结果：对比“FERREX-META”版本的卡片渲染样式，让图片的缩略图在列表微型卡片（`m_drawMiniCards` 名称列）中完全自适应居中或填满绘制（Cover/Contain平滑拉伸），不呈现缩小后的默认小图标样式，确保高画质的大图预览体验。
- 本次任务边界：修改 `src/ui/TreeItemDelegate.h` 中 Column 0 处于 `m_drawMiniCards` 状态时的图片/图标自绘逻辑。打通 `QIcon` 缓存 of QPixmap 提取与平滑缩放绘制链路，保证即使模型返回 QIcon 类型的缓存，也能够自动、高品质地缩放以填满 4px 圆角微型卡片容器。
- 不在本次范围内的是：修改 Grid/Justified 视图的自绘流程，或者修改模型层 `FerrexVirtualDbModel` 的缓存和线程载入逻辑。
- 对应方案文档：Modification_Plan-42.md

## [2026-07-22] 调整卡片尺寸滑杆与排列视图按钮功能移植及Ctrl+滚轮无级缩放重构

- 用户描述的现象/问题：当前版本的内容面板（ContentPanel）顶部缺少直观调节卡片尺寸的滑杆与排列方式选择按钮；且现有的 Ctrl+滚轮 交互极其混乱臃肿，在稍微滑动鼠标时就会自动在列表和卡片视图间强行变动，使用体验不佳。
- 用户期望的结果：
  1. 移植 “FERREX-META” 版本中优雅的“卡片尺寸滑杆（`m_sizeSlider`）”和“排列方式选择按钮（`viewBtn`）”功能至当前版本的标题栏，满足圆角、交互交互反馈和色值对齐。
  2. 废除原有多级阈值自动切换视图模式的滚轮事件，完全替换为 “FERREX-META” 的顺滑滚轮交互：Ctrl+滚轮仅用于线性调节尺寸滑杆的值（每次加/减 10px），提供一致且稳定的无级缩放卡片/图标体验。
- 本次任务边界：
  1. 在 `src/ui/ContentPanel.h` 与 `src/ui/ContentPanel.cpp` 中定义并加入滑杆 `m_sizeSlider` 和按钮 `m_viewBtn`，配置对齐 `Memories.md` 的 QSS 交互样式与槽逻辑，并在 `initUi` 中排版。
  2. 彻底重写 `ContentPanel::wheelEvent`，去除阈值切换视图逻辑，改为当 `ControlModifier` 激活时每次加/减 10px 调节滑杆数值。
  3. 在 `ContentPanel::eventFilter` 中对注册在视图 Viewport 上的 `QEvent::Wheel` 事件进行物理拦截，当用户按住 Ctrl 滚动时同样触发滑杆步进。
- 不在本次范围内的是：重构底层的数据库和文件加载扫描机制。
- 对应方案文档：Modification_Plan-43.md

## [2026-07-22] 物理根除内容面板 Ctrl+滚轮 自动切换视图及缩放的交互逻辑

- 用户描述的现象/问题：先前版本或复杂尝试可能对应用程序在运行/启动时引入不稳定性。用户指示首先彻底根除内容面板中 Ctrl+滚轮 事件触发卡片尺寸自动变动和自动升降级/跳转切换视图模式的旧有混乱交互代码，以保证启动稳定性与逻辑净化。
- 用户期望的结果：在不改变其他功能前提下，彻底在内容面板（ContentPanel）中剔除 Ctrl+滚轮 导致的缩放/改变 `m_zoomLevel` 或是自动调用 `setViewMode` 的交互流程，使得 Ctrl+滚轮 退化为默认的安全滚轮滚动行为。
- 本次任务边界：修改 `src/ui/ContentPanel.cpp` 中的 `ContentPanel::wheelEvent` 与 `ContentPanel::eventFilter`。物理擦除所有拦截 `Qt::ControlModifier` 并且进行缩放/切换视图的处理逻辑，还原为标准的 Qt 默认事件分发。
- 不在本次范围内的是：添加卡片尺寸滑杆、添加排列方式选择按钮，或是修改其余视图或模型底层逻辑。
- 对应方案文档：Modification_Plan-44.md

## [2026-07-22] 调整卡片尺寸滑杆与排列视图按钮至顶部自定义标题栏移植

- 用户描述的现象/问题：当前主界面顶部标题栏缺少方便调节卡片/缩略图尺寸的滑杆以及快速切换视图排列方式的控件，而原本的滚轮快捷变动已被彻底根除。
- 用户期望的结果：在顶部自定义标题栏中优雅地嵌入移植自 FERREX-META 版本的“调整卡片尺寸滑杆（m_sizeSlider）”和“排列方式选择按钮（viewBtn）”，实现卡片尺寸在 32~256 像素间的无缝无级调节，以及一键调出菜单快速切换自适应/网格/列表三种视图排版模式。
- 本次任务边界：
  1. 在 `MainWindow::setupCustomTitleBarButtons` 中，构建水平滑杆 `m_sizeSlider` 与切换按钮 `m_viewBtn`，配置对齐 `Memories.md` 的 QSS 交互样式。
  2. 建立交互链路：当滑杆滑动时，将数值传递并调用 `ContentPanel` 的 `m_zoomLevel` 并刷新视图行高/格尺寸；当点击排列按钮弹出菜单并选中对应选项时，调用 `ContentPanel::setViewMode` 切换视图排版模式。
- 不在本次范围内的是：修改物理 MFT 扫描、USN 监控底座以及其他非滑杆/排列按钮移植的相关逻辑。
- 对应方案文档：Modification_Plan-45.md
