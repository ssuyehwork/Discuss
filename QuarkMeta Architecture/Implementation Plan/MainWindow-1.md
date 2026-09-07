# MainWindow 职责归位重构实施方案

> 目标：把不该属于 MainWindow 的逻辑，逐一拆到该拆去的地方，让 MainWindow 回归"壳"的定位。
> 本方案只列拆分方向和落地位置，不涉及具体代码实现。

---

## 一、当前问题清单与拆分去向

### 1. 无边框窗口拖拽/缩放/最大化——整合至 `FramelessWindowHelper`
- 把 `WM_NCCALCSIZE`、`WM_GETMINMAXINFO` 以及 `WM_NCHITTEST` 原生消息拦截全部归集进 `FramelessWindowHelper` 类里。
- 在 `FramelessWindowHelper` 中通过 `QAbstractNativeEventFilter`（或与窗口事件结合）统一接管原生消息层与 Qt 事件层。
- 移除 `MainWindow` 内部对原生 Win32 消息和 `nativeEvent()` 的覆写。

### 2. 窗口几何状态保存与恢复——独立为 `WindowStateManager`
- 新建 `WindowStateManager` 状态持久化管理类，仅暴露 `saveState(QWidget* window)` 和 `restoreState(QWidget* window)` 两个纯净接口。
- `MainWindow::closeEvent()` 与 `initUi()` 中调用 `WindowStateManager::instance().saveState(this)` / `restoreState(this)` 托管原生 `saveGeometry()` 和 `restoreGeometry()`。

### 3. 导航栏响应式布局——封装至 `NavBar` 组件
- 将导航栏（后退/前进/上级按钮、地址栏、搜索框）从 `MainWindow` 中抽出，封装为独立的 `NavBar : public QWidget` 类。
- 由 `NavBar` 自行覆写 `resizeEvent()`，完成单行/双行响应式布局（`updateNavBarResponsiveLayout`）与布局切换判定。
- 从 `MainWindow` 中彻底移除像素级响应式布局计算。

### 4. 主窗口定位（保持现状）
- 参照 `PanelLayoutManager` 的拆分先例，`MainWindow` 仅作为外壳组装容器（Shell Container），只持有子面板/管理器引用并做信号槽连接。

---

## 二、MainWindow 拆分后保留的职责

- 持有各子面板（`NavPanel`、`FavoritePanel`、`ContentPanel`、`MetaPanel`、`FilterPanel`）与管理器的指针。
- 构造函数：组装各组件，建立信号槽关联。
- `closeEvent()`：转调 `WindowStateManager` 存盘并执行必要的退出协调。

---

## 三、后续开发红线

1. 禁止在 `MainWindow.cpp` 中直接写 `#ifdef Q_OS_WIN` 包裹的 Windows API 调用。
2. 禁止在 `MainWindow.cpp` 中硬编码裸调 `AppConfig` 进行批量几何状态存取。
3. 禁止 `MainWindow` 内部的 Qt 事件处理包含超过 5 行的具体业务/布局计算代码。
4. 新面板或子系统引入时必须参照 `PanelLayoutManager` 建立独立的管理类或组件类。
