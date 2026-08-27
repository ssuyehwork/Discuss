# 项目架构缺陷与全局事件过滤器审计报告 (Architectural-Issues-Audit.md)

## 1. 概述与背景
在 QuarkMeta 项目演进过程中，因部分模块未严谨遵循 Qt 事件分发机制与单一职责原则（SRP），出现了**局部逻辑全局化（Global Event Infiltration）**与**隐式耦合（Implicit Coupling）**等架构缺陷。
典型表现为：**原本服务于主窗口拉伸的 `ResizeEventFilter` 被全局安装至 `QCoreApplication`，导致系统托盘右键菜单（`QMenu`）及其他 `Qt::Popup` 控件在分发鼠标点击与释放事件时被强行拦截，造成弹出菜单选项点击无响应、瞬间消失或失去焦点**。

本报告对代码库中所有 `installEventFilter` 挂载点及跨模块事件交互进行了深度审计，列出架构缺陷风险清单并提供规范的修复建议。

---

## 2. 架构缺陷典型案例分析

### 案例 1：`ResizeEventFilter` 全局提权与侵入式事件拦截 (高危)
- **物理源码位置**：`src/ui/MainWindow.cpp:192`
  ```cpp
  QCoreApplication::instance()->installEventFilter(m_resizeFilter);
  ```
- **架构缺陷原因**：
  `ResizeEventFilter` 原本只需负责 `MainWindow` 无边框窗口四周边缘的鼠标悬停与拖拽拉伸。但开发过程中将其直接挂载到了 `QCoreApplication::instance()` 实例上。
  由于没有在 `eventFilter(QObject* watched, QEvent* event)` 入口对被监听对象 `watched` 是否为 `m_window`（主窗口自身）进行校验，导致系统中**任何顶级控件或弹窗（如 `QMenu`、`QDialog`、`ToolTipOverlay`）的每一个 `QMouseEvent` 都会被强行送入拉伸判断流程**。
- **故障因果链**：
  1. 用户右键点击托盘图标弹出 `QMenu`（`Qt::Popup` 属性）；
  2. 用户鼠标点击 `QMenu` 中的菜单项；
  3. 全局 `ResizeEventFilter` 捕获该 `MouseButtonPress` / `MouseButtonRelease` 事件，在映射坐标系时因 `m_window` 坐标冲突产生混乱；
  4. `QMenu` 判定自身“被外部点击”或丢失全局鼠标抓取（Mouse Grab），在触发 `QAction::triggered` 之前便立即退出了模态事件循环并关闭，导致菜单点击彻底失效。
- **标准规范修复方案**：
  严禁将窗口级过滤器提权为全局过滤器；若必须全局挂载，必须在入口加入严格的对象白名单断言：
  ```cpp
  bool ResizeEventFilter::eventFilter(QObject* watched, QEvent* event) {
      if (!m_window || watched != m_window || m_window->isMaximized()) {
          return QObject::eventFilter(watched, event);
      }
      // ... 仅处理主窗口自身的拖拽拉伸 ...
  }
  ```

---

### 案例 2：`TagSelectorOverlay` 全局监听与模态点击穿透 (中危)
- **物理源码位置**：`src/ui/TagSelectorOverlay.cpp:39`
  ```cpp
  qApp->installEventFilter(this);
  ```
- **架构缺陷原因**：
  `TagSelectorOverlay` 为自动吸附的标签选择浮层。为了实现在浮层外部点击时自动关闭功能，其将自身作为事件过滤器安装到了 `qApp` 上。
  但在 `eventFilter` 实现中：
  ```cpp
  if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent* me = static_cast<QMouseEvent*>(event);
      if (isVisible() && !geometry().contains(me->globalPosition().toPoint())) {
          closeOverlay();
          return false; // 不拦截，允许底层控件正常响应点击
      }
  }
  ```
  尽管返回了 `false`，但因为直接使用了 `globalPosition()` 判定，忽略了多屏 DPI 缩放以及弹出菜单/右键上下文菜单的父子关联，容易引发全局鼠标按键状态断层。
- **标准规范修复方案**：
  重构浮层失焦关闭逻辑，优先依赖 Qt 原生的 `Qt::Popup` 模式或 `QEvent::ActivationChange` 监听，减少对 `qApp` 事件流的直接干预。

---

### 案例 3：`TitleBarEventFilter` 跨控件坐标混淆 (低危)
- **物理源码位置**：`src/ui/TitleBarEventFilter.cpp`
- **架构缺陷原因**：
  标题栏事件过滤器同时安装在 `m_titleBarWidget`、`m_logoLabel` 及 `m_appNameLabel` 上。在计算拖拽与双击全屏时，依赖全局坐标与父窗口本地坐标的混算，若子控件样式或 Margin 发生变动，容易导致拖拽感应区域错位。
- **标准规范修复方案**：
  收拢事件监听接口，仅在顶层标题栏容器 widget 处理事件，避免将子控件逐个安装事件过滤器。

---

## 3. 事件过滤器开发红线规范

为了杜绝后续再次出现类似的架构缺陷与分析断层，项目组制定以下**三大事件过滤器开发红线**：

1. **原则 1：严禁无校验的全局挂载（Strict Object Guard）**
   除非是全局快捷键拦截器或日志审计，严禁将事件过滤器安装至 `qApp` / `QCoreApplication`。若确需安装，`eventFilter` 函数的第一行代码**必须**断言 `watched` 对象：
   ```cpp
   if (watched != m_targetObject) return QObject::eventFilter(watched, event);
   ```

2. **原则 2：不得破坏 Popup 控件的 WindowFlags**
   对 `QMenu` 或模态弹窗应用自定义样式（如 `applyMenuStyle`）时，**禁止直接使用 `setWindowFlag()` 覆写**，必须使用按位或运算符保持已有的 `Qt::Popup` 属性：
   ```cpp
   // 正确做法：
   menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
   ```

3. **原则 3：高内聚低耦合的事件通信**
   跨面板/跨模块通信优先使用 `QObject::connect` 信号槽或 `CentralEventHub` 中央事件总线，禁止通过在其他面板上安装事件过滤器的方式硬编码偷取事件。
