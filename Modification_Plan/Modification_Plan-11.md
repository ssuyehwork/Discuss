# 移植 FERREX-META 三大视图模式与双尺寸调节机制方案 —— Modification_Plan-11.md

## 1. 任务背景
在目前的版本中，内容容器只有网格模式（QListView）和列表模式（QTreeView），排序和卡片尺寸缩放也比较零散（最小只能缩放到 96 像素）。
为了提供极致的文件和媒体展现能力，需要完美移植并引入 `FERREX-META` 版中的 **3 种高精度显示视图**（列表视图 `ListResultView`、等高合理排版视图 `JustifiedResultView`、网格卡片视图 `GridResultView`）以及 **2 种极速响应的卡片尺寸双重控制机制**（拖拽/点击滑动条、Ctrl+滚轮无极缩放）。

---

## 2. 问题定位与移植对账
在移植过程中，需对两套系统的控件、Delegate 及布局关系进行对账：
- **视图接口抽象**：
  在 `FERREX-META` 中，视图继承自 `IScanResultView`。我们需要将该抽象接口、`ListResultView`、`JustifiedResultView` 和 `GridResultView` 完全搬移至 `src/ui/` 目录下，并使其基准 Model 挂接至当前的 `FilterProxyModel`。
- **滑动条（Slider）交互点**：
  在主界面的工具栏（`MainWindow::initToolbar()`）或标题栏右侧加入 `m_sizeSlider`，初始值读取自 `AppConfig` 的缩放级设定。点击轨道触发通过 `QStyle::sliderValueFromPosition` 实现瞬时重定位。
- **滚轮缩放事件点**：
  必须在 `ContentPanel::eventFilter` 或者 `MainWindow::eventFilter` 中拦截 `isViewOrViewport` 对象的 `QEvent::Wheel` 事件。一旦判断 `modifiers() & Qt::ControlModifier` 激活，即直接更新 `m_sizeSlider` 滑块位置，由 QSlider 的 `valueChanged` 信号统一广播至当前激活的视图和 Delegates 实例，实现统一的单向数据流动态缩放。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 三种视图模式：列表、等高合理排版、网格卡片 | 将 `IScanResultView.h` 以及 3 种结果视图移植并注册加入 QStackedWidget 视图管理器，支持无缝一键切换 | ✅ 一致 |
| 2    | 滑动条直接拖拽 / 点击 | 工具栏加入 QSlider 控件，支持拖拽滑块与鼠标左键点击轨道瞬间精确定位数值 | ✅ 一致 |
| 3    | `Ctrl` + 鼠标滚轮异步缩放 | 拦截滚轮滚动，按住 `Ctrl` 键时，按 `+10` / `-10` 增减滑动条的值并实时回传各视图的缩放高度与卡片大小 | ✅ 一致 |

---

## 4. 详细解决方案

### 4.1 核心视图组件文件的迁移与类注册
将下列文件从 `FERREX-META/src/ui/` 拷贝、移植至当前工程的 `src/ui/` 目录：
- `IScanResultView.h`
- `ListResultView.h` 与 `ListResultView.cpp`
- `JustifiedResultView.h` 与 `JustifiedResultView.cpp`
- `GridResultView.h` 与 `GridResultView.cpp`
- `JustifiedView.h` 与 `JustifiedView.cpp`（如有涉及）

并在 `ContentPanel` 模块或主窗口的 `QStackedWidget` 视图管理器中，实例化这三大视图并挂载挂接。

---

### 4.2 尺寸滑动条（QSlider）的配置、注入与点击定位
在 `MainWindow` 导航栏或工具栏合适位置（如搜索框旁）中添加 QSlider：

```cpp
    m_sizeSlider = new QSlider(Qt::Horizontal, this);
    m_sizeSlider->setRange(32, 256); // 物理范围：32 像素至 256 像素
    m_sizeSlider->setValue(AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt());
    m_sizeSlider->setFixedSize(110, 20);
    m_sizeSlider->setCursor(Qt::PointingHandCursor);
    m_sizeSlider->installEventFilter(this); // 安装事件过滤器

    // 轨道点击精准定位：重写或在 eventFilter 中拦截 QEvent::MouseButtonPress
```

在 `eventFilter` 捕获到滑动条的鼠标左键按下时，瞬间定位对应的值并强制更新：
```cpp
    if (watched == m_sizeSlider && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            int val = QStyle::sliderValueFromPosition(m_sizeSlider->minimum(), m_sizeSlider->maximum(), me->pos().x(), m_sizeSlider->width());
            m_sizeSlider->setValue(val);
            return true;
        }
    }
```

绑定变动信号，在值变更时通过 `setIconSize(v)` 瞬间动态重构当前活动视图的显示尺寸，并调用 `refreshLayout` 进行重新布局（同时利用防抖定时器触发缩略图缓存的批量重载）：
```cpp
    connect(m_sizeSlider, &QSlider::valueChanged, this, [this](int v) {
        AppConfig::instance().setValue("UI/GridZoomLevel", v);
        if (m_currentActiveView) {
            m_currentActiveView->setIconSize(v);
            m_currentActiveView->refreshLayout();
        }
        m_zoomDebounceTimer->start(); // 200ms 防抖，触发底层缩略图缓存更新
    });
```

---

### 4.3 鼠标滚轮缩放机制 (Ctrl + Mouse Wheel Scroll)
在系统事件过滤器（例如 `MainWindow::eventFilter` 拦截活动视图及其 Viewport 的滚轮事件）：

```cpp
    if (isViewOrViewport && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            int deltaY = wheelEvent->angleDelta().y();
            if (deltaY > 0) {
                // 滚轮向上，以 10px 为单位平滑放大
                m_sizeSlider->setValue(m_sizeSlider->value() + 10);
            } else if (deltaY < 0) {
                // 滚轮向下，以 10px 为单位平滑缩小
                m_sizeSlider->setValue(m_sizeSlider->value() - 10);
            }
            return true; // 拦截事件，防止触发常规滚动
        }
    }
```

由于 `m_sizeSlider->setValue` 会触发刚才连接的 `valueChanged` 信号，从而自动广播到 `m_currentActiveView->setIconSize()` 并完成更新，这一设计形成了一个极佳的单向数据流闭环，确保界面状态的 100% 同步和零抖动。

---

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/IScanResultView.h`、`src/ui/ListResultView.cpp/h`、`src/ui/JustifiedResultView.cpp/h`、`src/ui/GridResultView.cpp/h`、`src/ui/MainWindow.cpp/h`、`src/ui/ContentPanel.cpp/h`
  - 拷入视图移植相关源文件。
  - 注册 QSlider 尺寸调节条及 `valueChanged` 槽函数。
  - 在 `eventFilter` 中补充轨道定位检测与 `Ctrl` + 滚轮对位放大的捕获逻辑。

**明确禁止越界修改的范围：**
- [ ] 严禁改变底层的 MFT / IOCP 物理文件监听。
- [ ] 严禁修改其他与此功能不相关的 UI 组件或全局数据库字段结构。

---

## 6. 实现准则与预警【核心】
1. **防止界面重叠与尺寸剧烈抖动**：
   - 使用 200ms 的防抖定时器（`m_zoomDebounceTimer`），在用户使用滚轮或拖拽 QSlider 进行缩放时，合并高频的缓存重加载信号。仅在用户停止调节 200ms 后才触发批量生成和缓存，保证大批量卡片展示时的绝对平滑和零卡顿。
2. **复用 `IScanResultView` 对位渲染**：
   - 必须保持 3 种视图在同一数据模型（`FilterProxyModel`）的驱动下，由统一的 `setIconSize` 控制。这避免了因不同视图使用各自独立的尺寸属性导致多视图切换时的状态混乱和不一致。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 输入框与控件样式 | 唯一标准：一律使用原生，禁止使用 rgba 蒙版，悬停色 `#3E3E42`，按下色 `#4E4E52` | ✅ 符合。QSlider 和按钮样式严格对齐该色值设计。 |
| 双轨机制 | 排序与筛选执行范围与顶部高亮对位。 | ✅ 符合。本方案视图渲染完全在 `ContentPanel` 内容范围层内，完美融入双轨机制。 |
