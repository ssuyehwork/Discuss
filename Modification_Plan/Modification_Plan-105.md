# 容器与状态栏 5px 隙缝悬浮进度条与状态栏计时联动落地实现 —— Modification_Plan-105.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
主界面五个容器与底部状态栏之间存在 5 像素的物理间距（由 `m_bodyLayout` 的底部内边距 `kEdgeMargin` 决定）。为了在该间距内无缝且低开销地展示元数据后台同步进度与实时耗时，本方案在不调整任何既有布局边距的情况下，引入一个高度为 5 像素的悬浮覆盖进度条，并与后台 `SyncStatusService` 信号及状态栏常态文本进行完美地联动显隐与耗时统计。

## 2. 问题定位
- 现有布局结构：中央大容器 `centralC` 包含了一个 `QVBoxLayout`（`mainL`），依次排布 `m_titleBarWidget`、`m_driveBarWidget`、`m_navBarWidget`、`bodyWrapper`、以及底部状态栏 `statusBar`。
- `bodyWrapper` 的布局 `m_bodyLayout` 的 Margins 设置为了 `kEdgeMargin, 0, kEdgeMargin, kEdgeMargin`（其中 `kEdgeMargin = 5`），这使得 `bodyWrapper` 内的五个容器底部与 `statusBar` 之间留出了 5 像素的物理留白隙缝。
- 若直接通过 `mainL->insertWidget` 添加普通进度条，则会在显示/隐藏进度条时强行触发整个主窗口的重绘与重新布局，导致界面分栏产生闪烁和抖动。
- 根治思路：采用 **“无布局悬浮覆盖层（Overlay Widget）+ 绝对定位（Absolute Positioning）”** 的设计。进度条不加入任何 Layout，作为 `centralC` 的非布局子控件，在 `resizeEvent` 中计算主体物理边界，将其绝对定位放置在 5 像素的隙缝之上并提升图层。同时通过耗时定时器控制状态栏百分比和耗时的联动显示。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 五个容器与状态栏之间是不是有着5像素的间距？（用户原话） | 确认了 `m_bodyLayout` 底部边距导致的 5 像素留白物理间距。 | ✅ |
| 2    | 这个5像素的间距是不是由Margin决定的呢？（用户原话） | 确认了该间距由 `m_bodyLayout->setContentsMargins` 的底部边距确定。 | ✅ |
| 3    | 这5像素的间距不做任何调整（不为0）情况下，如果我要将一个进度条高度为5像素覆盖在其上方显示，是否可行？（用户原话） | 采用绝对定位不加布局的形式，在不调整 5 像素 Margin 的前提下，使 5px 高度进度条覆盖其上显示。 | ✅ |
| 4    | 后台开始扫描或对账，5px的隙缝处瞬间被一条5px高的蓝色极光光条完全填满覆盖（用户原话） | 进度条在检测到同步开始时自动 `show()`，并设置对应的高度、样式与位置。 | ✅ |
| 5    | 并在 `resizeEvent` 中通过 `statusBar->geometry().top() - 5` 实时计算 Y 轴坐标（用户原话） | 在 `resizeEvent` 中调用 `updateProgressBarGeometry`，使用该公式实现绝对吸附。 | ✅ |
| 6    | 定时器超时联动 `m_statusLeft` 文本，并监听 `SyncStatusService` 信号（用户原话） | 添加 `QTimer` 定时器刷新逻辑，完美监听 `SyncStatusService` 的进度信号并完成联动。 | ✅ |

## 4. 详细解决方案

### 4.1 在头文件定义状态与成员变量
在 `src/ui/MainWindow.h` 中：
1. 重写 `resizeEvent` 保护方法，以便在主窗体拉伸、最大化和还原时实时动态重算进度条坐标。
2. 声明相关槽函数 `updateProgressBarGeometry()` 以及控制变量（进度条 `m_topProgressBar`、刷新定时器 `m_elapsedTimer` 以及记录同步开始时刻的毫秒时间戳 `m_syncStartTime`）。

### 4.2 初始化悬浮进度条（不进 Layout）
在 `src/ui/MainWindow.cpp` 的 `setupSplitters()` 函数末尾：
1. 创建 `QProgressBar` 并将其父级设为中央容器 `centralC`。
2. 强制其固定高度为 5 像素，并去除文字，使其作为一个极其纯净的水平极光条。
3. 应用扁平化的 CSS 样式（背景完全透明，高亮部分采用系统标准的 PrimaryBlue 颜色）。
4. 默认调用 `hide()` 使其初始状态静默隐藏。

### 3.3 实现窗口缩放与绝对定位计算
在 `src/ui/MainWindow.cpp` 中实现：
1. `resizeEvent(QResizeEvent* event)`：
   - 先调用基类的 `QMainWindow::resizeEvent(event)`。
   - 随即调用 `updateProgressBarGeometry()` 刷新进度条。
2. `updateProgressBarGeometry()`：
   - 提取主体包裹控件 `bodyWrapper`（即 `m_mainSplitter` 的父对象）以及状态栏控件 `statusBar`（即 `m_statusLeft` 的父对象）。
   - 计算得到 Y 轴绝对吸附坐标：`int y = statusBar->geometry().top() - 5;`（对应用户原话："在 resizeEvent 中通过 statusBar->geometry().top() - 5 实时计算 Y 轴坐标"）。
   - 绝对计算 X 坐标 `bodyWrapper->geometry().left()` 与宽度 `bodyWrapper->geometry().width()`。
   - 调用 `m_topProgressBar->setGeometry(x, y, width, 5)` 将其精准放置于 5 像素间隙中。
   - 调用 `m_topProgressBar->raise()` 提升组件的绘制次序，防止被窗口的普通背景覆盖。

### 3.4 状态栏耗时与进度服务联动
在 `src/ui/MainWindow.cpp` 的 `initUi()` 尾部：
1. 创建定时刷新器 `m_elapsedTimer`，定时间隔设为 `100ms`。
2. 绑定 `m_elapsedTimer` 的 `timeout` 信号：
   - 若同步已经开始（`m_syncStartTime > 0`），计算累计经过的秒数 `elapsedSec`。
   - 实时拼接格式文本（例如：“正在同步元数据... X%  |  耗时: Ys”），并更新给状态栏标签 `m_statusLeft`。
3. 连接全局同步状态信号 `SyncStatusService::instance()` 的 `statusUpdated` 槽函数：
   - **当 `syncing` 为 `true` 且待处理项 `count > 0` 时**：
     - 若为首次监听到同步，记录当前系统毫秒时间戳 `m_syncStartTime`，启动 `m_elapsedTimer`，调用 `updateProgressBarGeometry()` 重新刷正进度条坐标，并将其 `show()` 展现出来。
     - 进度值根据待落盘数量安全映射，限制范围在 `10%` 到 `95%` 之间，防止未写完就提前拉满。
   - **当同步完成（`syncing` 为 `false` 且 `count <= 0`）时**：
     - 如果检测到正在进行中状态，将进度条瞬间设为 `100%`，停止耗时定时器。
     - 状态栏显示“元数据处理完成  |  总耗时: Zs”。
     - 开启一个 `singleShot` 延迟 400ms，延时过后 `hide()` 隐藏进度条，并在 3 秒后自动调用系统现有的 `updateStatusBar()` 恢复到常态状态栏文本。

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] `src/ui/MainWindow.h`：重写 `resizeEvent`，添加变量声明与进度条计算函数。
- [ ] `src/ui/MainWindow.cpp`：实现 `setupSplitters` 处的进度条初始化、`resizeEvent`、`updateProgressBarGeometry` 以及在 `initUi` 中增加耗时与信号联动。

**明确禁止越界修改的范围：**
- [ ] `SyncStatusService` 底层状态机实现——不修改
- [ ] `updateStatusBar` 的常态项目统计逻辑——不修改

## 6. 实现准则与预警【核心】

1. **头文件依赖预防**：
   在 `MainWindow.h` 中必须前置声明 `#include <QProgressBar>`、`#include <QTimer>`、`#include <QDateTime>`，或者在 `MainWindow.cpp` 中精准导入。
2. **零布局冲突**：
   千万不要在中央大容器的 `mainL` 布局中执行 `addWidget(m_topProgressBar)`。它必须作为一个单纯的子控件（直接 `new QProgressBar(centralC)`），避免引起界面其他区域重绘时产生的位移。
3. **图层遮挡防御**：
   定位计算完成后，必须调用一次 `m_topProgressBar->raise()` 确保进度条在 `centralC` 各种动态子面板的最上层，不被其他控件遮盖。
4. **多线程安全提示**：
   `SyncStatusService` 的信号可能由非 UI 线程异步发射，但在连接时 Qt 默认会自动转换为排队连接（QueuedConnection）安全地将槽执行在主 UI 线程，本方案安全无虞。
5. **还原重置**：
   进度条隐藏后，利用 `QTimer::singleShot` 延迟 3 秒调用 `updateStatusBar()` 自动复位，保证常态信息不丢失。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 样式规范 | 背景完全透明不遮挡主体，亮蓝色前景无缝配合暗黑主题 | ✅ 符合 |
| 内存安全 | `m_topProgressBar` 指定了 `centralC` 作为父对象，内存由 Qt 对象树自动管辖释放，定时器绑定 `this` 避免泄露 | ✅ 符合 |

## 8. 待确认事项（可选）
- 暂无待确认事项。
