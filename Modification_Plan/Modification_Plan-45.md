# 视图按钮及缩放滑杆功能移植重构(顺序调整版) —— Modification_Plan-45.md

> 状态：已批准，执行中

## 1. 任务背景
本方案承接自 Modification_Plan-44.md，在 44 号方案基础上根据用户最新的要求微调了自定义标题栏按钮的物理摆放顺序。
本方案将彻底迁移并融合 `FERREX-META` 版本中的缩放滑杆（`m_sizeSlider`）和视图模式排列按钮（`viewBtn`），并将缩放逻辑与内容视图的 `wheelEvent` 进行同频联动和持久化。

## 2. 问题定位
- **定位模块 1（MainWindow）**：
  `src/ui/MainWindow.h` 与 `src/ui/MainWindow.cpp` 包含自定义标题栏的构建逻辑。在 `setupCustomTitleBarButtons()` 中需要新增“视图按钮”及“缩放滑杆”控件，并将其加入到标题栏布局中，调整物理排序。
- **定位模块 2（ContentPanel）**：
  `src/ui/ContentPanel.h` 与 `src/ui/ContentPanel.cpp` 包含主内容展示区。需要通过信号槽暴露 `zoomLevel` 的控制函数，并重构 `wheelEvent` 捕获 Ctrl + 滚轮动作，控制 `m_zoomLevel` 在 [96, 128] 像素安全范围内调整。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 将“FERREX-META”版本里的滑杆和视图按钮功能迁移到当前版本里使用 | 移植滑杆与视图切换按钮至自定义标题栏。 | ✅ 一致 |
| 2    | 排列方式与滑杆顺序微调 | 按照用户修改的顺序依次添加：滑杆、视图、折叠盘符按钮。 | ✅ 一致 |
| 3    | 滚轮缩放联动 | 捕获内容面板 Ctrl+滚轮事件，同频更新 `m_zoomLevel` 与标题栏滑杆的值。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 引入视图与缩放联动接口
在 `src/ui/ContentPanel.h` 中，提供修改缩放级别及切换视图模式的槽函数和信号：
- 新增信号：
  ```cpp
  void zoomLevelChanged(int level);
  void viewModeChanged(ViewMode mode);
  ```
- 新增公共槽函数：
  ```cpp
  void setZoomLevel(int level);
  ```

在 `src/ui/ContentPanel.cpp` 中：
- 实现 `setZoomLevel(int level)`：
  ```cpp
  void ContentPanel::setZoomLevel(int level) {
      int boundedLevel = qBound(96, level, 128);
      if (m_zoomLevel == boundedLevel) return;
      m_zoomLevel = boundedLevel;
      updateGridSize();
      emit zoomLevelChanged(m_zoomLevel);
  }
  ```
- 改造 `setViewMode(ViewMode mode)`，使其在切换视图模式后，不仅调用 `updateGridSize()`，更触发 `viewModeChanged(mode)` 信号：
  ```cpp
  void ContentPanel::setViewMode(ViewMode mode) {
      m_currentViewMode = mode;
      // ... 原有 stack 切换逻辑
      updateGridSize();
      emit viewModeChanged(mode);
      m_visibleTimer->start();
  }
  ```
- 重构 `wheelEvent(QWheelEvent* event)` 以捕获 Ctrl + 滚轮：
  ```cpp
  void ContentPanel::wheelEvent(QWheelEvent* event) {
      if (event->modifiers() & Qt::ControlModifier) {
          int deltaY = event->angleDelta().y();
          int newZoom = m_zoomLevel + (deltaY > 0 ? 8 : -8);
          setZoomLevel(newZoom);
          event->accept();
          return;
      }
      QWidget::wheelEvent(event);
  }
  ```

### 4.2 移植滑杆（m_sizeSlider）与视图排列按钮（viewBtn）
在 `src/ui/MainWindow.h` 中，新增私有成员及事件过滤器声明：
```cpp
    QPushButton* m_btnViewMenu = nullptr;
    QSlider* m_sizeSlider = nullptr;
```

在 `src/ui/MainWindow.cpp` 中，重构 `setupCustomTitleBarButtons()` 逻辑：
1. **构建“排列方式”视图按钮**：
   ```cpp
   m_btnViewMenu = createTitleBtn("grid"); // 使用默认网格图标
   m_btnViewMenu->setProperty("tooltipText", "排列方式");
   m_btnViewMenu->installEventFilter(m_hoverFilter);

   connect(m_btnViewMenu, &QPushButton::clicked, this, [this]() {
       QMenu menu(this);
       UiHelper::applyMenuStyle(&menu);

       QAction* actAdaptive = menu.addAction("自适应");
       QAction* actGrid = menu.addAction("网格");
       QAction* actList = menu.addAction("列表");

       actAdaptive->setCheckable(true);
       actGrid->setCheckable(true);
       actList->setCheckable(true);

       // 状态打勾对齐
       auto currentMode = m_contentPanel->property("currentViewMode").toInt();
       ContentPanel::ViewMode mode = static_cast<ContentPanel::ViewMode>(currentMode);
       actAdaptive->setChecked(mode == ContentPanel::JustifiedViewMode);
       actGrid->setChecked(mode == ContentPanel::GridView);
       actList->setChecked(mode == ContentPanel::ListView);

       connect(actAdaptive, &QAction::triggered, this, [this]() {
           m_contentPanel->setViewMode(ContentPanel::JustifiedViewMode);
       });
       connect(actGrid, &QAction::triggered, this, [this]() {
           m_contentPanel->setViewMode(ContentPanel::GridView);
       });
       connect(actList, &QAction::triggered, this, [this]() {
           m_contentPanel->setViewMode(ContentPanel::ListView);
       });

       menu.exec(m_btnViewMenu->mapToGlobal(QPoint(0, m_btnViewMenu->height())));
   });
   ```

2. **构建“缩放滑杆”**：
   ```cpp
   m_sizeSlider = new QSlider(Qt::Horizontal, this);
   m_sizeSlider->setRange(96, 128); // 卡片缩放红线
   m_sizeSlider->setFixedSize(100, 20);
   m_sizeSlider->setCursor(Qt::PointingHandCursor);
   m_sizeSlider->setStyleSheet(
       "QSlider { background: transparent; }"
       "QSlider::groove:horizontal { height: 3px; background: #3F3F3F; border-radius: 2px; }"
       "QSlider::sub-page:horizontal { background: #FF8C00; border-radius: 2px; }"
       "QSlider::handle:horizontal { width: 10px; height: 10px; background: #FF8C00; border-radius: 5px; margin: -4px 0; }"
   );

   // 绑定滑杆滑动到内容面板
   connect(m_sizeSlider, &QSlider::valueChanged, this, [this](int value) {
       m_contentPanel->setZoomLevel(value);
   });
   ```

3. **双向数据流联动与初始化**：
   ```cpp
   // 从内容面板向滑杆单向传递（例如 Ctrl+滚轮 变化时）
   connect(m_contentPanel, &ContentPanel::zoomLevelChanged, this, [this](int level) {
       QSignalBlocker blocker(m_sizeSlider);
       m_sizeSlider->setValue(level);
   });
   // 视图变化时联动更新视图按钮图标
   connect(m_contentPanel, &ContentPanel::viewModeChanged, this, [this](ContentPanel::ViewMode mode) {
       QString iconKey = "grid";
       if (mode == ContentPanel::ListView) iconKey = "list";
       else if (mode == ContentPanel::JustifiedViewMode) iconKey = "columns";
       m_btnViewMenu->setIcon(UiHelper::getIcon(iconKey, QColor("#EEEEEE")));
   });

   // 初始化滑杆位置
   int initZoom = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt();
   m_sizeSlider->setValue(qBound(96, initZoom, 128));
   ```

4. **精确对齐装配到标题栏布局**（对应用户原话）：
   ```cpp
   layout->addWidget(m_sizeSlider, 0, Qt::AlignVCenter);   // 调节滑杆
   layout->addWidget(m_btnViewMenu, 0, Qt::AlignVCenter); // 排列方式按钮
   layout->addWidget(m_btnToggleDriveBar, 0, Qt::AlignVCenter);
   layout->addWidget(m_btnSync, 0, Qt::AlignVCenter);
   layout->addWidget(m_btnLayout, 0, Qt::AlignVCenter);
   layout->addWidget(m_btnCreate, 0, Qt::AlignVCenter);
   layout->addWidget(m_btnPinTop, 0, Qt::AlignVCenter);
   layout->addWidget(m_btnMin, 0, Qt::AlignVCenter);
   layout->addWidget(m_btnMax, 0, Qt::AlignVCenter);
   layout->addWidget(m_btnClose, 0, Qt::AlignVCenter);
   ```

## 5. 修改边界声明【范围】
- **本次方案涉及范围**：
  - `src/ui/ContentPanel.h` / `src/ui/ContentPanel.cpp`
  - `src/ui/MainWindow.h` / `src/ui/MainWindow.cpp`
- **明确禁止越界修改的范围**：
  - 严禁修改物理底层 `DbManager` 或者是 `MetadataManager` 逻辑。
  - 严禁在其他地方改动 `m_zoomLevel` 的持久化命名。

## 6. 实现准则与预警【核心】
1. **防止无限循环触发**：
   滑杆的 `valueChanged` 会调用 `setZoomLevel`，而 `setZoomLevel` 触发的 `zoomLevelChanged` 信号又会改变滑杆的值。为杜绝可能引起的死循环，当信号反馈至 `MainWindow` 调整滑杆时，必须使用 `QSignalBlocker blocker(m_sizeSlider);` 暂时屏蔽滑杆的信号。
2. **完美对准 96 像素硬红线**：
   根据 UI 规范和考古守则，网格卡片的物理强制缩放底限为 96 像素。滑杆的范围设为 `[96, 128]` 完美符合物理硬红线。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 输入框清除功能 | 一律使用 Qt 原生 `setClearButtonEnabled(true)`。 | ✅ 符合（本方案未添加新输入框，不涉及） |
| 窗口置顶 | 一律使用 Win32 原生 `SetWindowPos`（`HWND_TOPMOST`/`HWND_NOTOPMOST`），必须配合 `SWP_NOSENDCHANGING` | ✅ 符合（本方案未重构置顶逻辑，不涉及） |
| 标题栏按钮样式 | 悬停：`#3E3E42`；按下：`#4E4E52`。禁用 rgba 蒙版。 | ✅ 符合（移植的视图按钮完全复用 `createTitleBtn` 函数以套用标准标题栏悬停样式，符合全局规范要求） |

## 8. 待确认事项
目前设计方案自洽。
