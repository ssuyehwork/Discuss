# MainWindow 初始化时序与标题栏按钮状态同步实施方案 (MainWindow-2.md)

## 1. Overview
修复在 `MainWindow` 初始化过程中，由于 `restoreGeometry` 早于 `m_titleBarWidget` 实例化执行，导致 `WindowStateChange` 事件触发时 `m_titleBarWidget` 尚为空指针、无法同步更新标题栏最大化/还原按钮图标的 Lifecycle Bug。

主要变更：
1. 调整 `MainWindow::initUi()` 中的初始化顺序：先实例化并拼装 `m_titleBarWidget`，再调用 `restoreGeometry()`，确保状态恢复时事件能准确送达 `TitleBarWidget`；
2. 在 `TitleBarWidget` 构造/初始化完成时添加对宿主窗口状态的显式主动拉取（Pull），防御后续任何潜在的时序遗漏。

---

## 2. Modified Files List
- `src/ui/MainWindow.cpp`
- `src/ui/TitleBarWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### `src/ui/MainWindow.cpp`

<<<<<<< SEARCH
void MainWindow::initUi() {
    QByteArray savedGeom = AppConfig::instance().getValue("MainWindow/Geometry").toByteArray();
    if (!savedGeom.isEmpty()) {
        restoreGeometry(savedGeom);
    } else {
        resize(1180, 800);
    }

    QWidget* centralC = new QWidget(this);
    centralC->setObjectName("CentralWidget");
    QVBoxLayout* mainL = new QVBoxLayout(centralC);
    mainL->setContentsMargins(0, 0, 0, 0);
    mainL->setSpacing(0);

    // 1. 顶层子组件实例化 (TitleBar / NavBar / DriveBar)
    m_titleBarWidget = new TitleBarWidget(centralC, m_hoverFilter);
=======
void MainWindow::initUi() {
    QWidget* centralC = new QWidget(this);
    centralC->setObjectName("CentralWidget");
    QVBoxLayout* mainL = new QVBoxLayout(centralC);
    mainL->setContentsMargins(0, 0, 0, 0);
    mainL->setSpacing(0);

    // 1. 顶层子组件实例化 (TitleBar / NavBar / DriveBar)
    m_titleBarWidget = new TitleBarWidget(centralC, m_hoverFilter);

    QByteArray savedGeom = AppConfig::instance().getValue("MainWindow/Geometry").toByteArray();
    if (!savedGeom.isEmpty()) {
        restoreGeometry(savedGeom);
    } else {
        resize(1180, 800);
    }
>>>>>>> REPLACE

---

### `src/ui/TitleBarWidget.cpp`

<<<<<<< SEARCH
    m_layout->addWidget(m_btnMin, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnMax, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnClose, 0, Qt::AlignVCenter);
}
=======
    m_layout->addWidget(m_btnMin, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnMax, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnClose, 0, Qt::AlignVCenter);

    // 主动同步宿主窗口当前的最大化状态，防止初始化时序漏更
    QTimer::singleShot(0, this, [this]() {
        if (window() && m_btnMax) {
            QString iconKey = window()->isMaximized() ? "restore_line" : "maximize";
            m_btnMax->setIcon(UiHelper::getIcon(iconKey, QColor("#EEEEEE")));
        }
    });
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. 使用 CMake 编译项目：
   ```bash
   cmake --build --preset x64-Debug
   ```
2. 运行 `QuarkMeta.exe`；
3. 点击标题栏最大化按钮将主窗口最大化；
4. 关闭主程序并重新启动；
5. 验证：启动后主窗口保持最大化状态，且标题栏上的按钮图标正确显示为【还原图标】(`restore_line`)，悬浮提示保持正常。
