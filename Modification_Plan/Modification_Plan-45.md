# 调整卡片尺寸滑杆与排列视图按钮至顶部自定义标题栏移植 —— Modification_Plan-45.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在移除了原内容面板（ContentPanel）中臃肿混乱的滚轮自动多级切换后，用户希望重新设计一种直观、可控的 UI 交互机制。本方案旨在在主界面 `MainWindow` 顶部的自定义标题栏中优雅地嵌入并移植来自 `FERREX-META` 的“调整卡片尺寸滑杆（`m_sizeSlider`）”和“排列方式选择按钮（`m_viewBtn`）”（对应用户原话：“下一步单独来完成「滑杆+排列按钮移植」”）。

## 2. 问题定位
当前主界面顶部标题栏按钮组由 `MainWindow::setupCustomTitleBarButtons()` 方法负责创建，并设置给主窗口。目前只包含了侧边栏折叠按钮 `m_btnToggleDriveBar`、同步状态按钮 `m_btnSync`、布局管理按钮 `m_btnLayout`、新建按钮 `m_btnCreate` 等系统控制按钮。
为了移植新组件，我们需要：
1. **添加声明**：在 `src/ui/MainWindow.h` 中添加 `QSlider* m_sizeSlider` 与 `QPushButton* m_viewBtn` 成员变量声明。
2. **在初始化时构建**：在 `MainWindow::setupCustomTitleBarButtons()` 中实例化这两个控件，使用高画质图标对齐视觉标准。
3. **样式与布局对齐**：使用 Memories.md 规范和 FERREX-META 考古色值对滑杆及按钮样式进行美化，并将其整齐插入到标题栏的按钮水平布局中。
4. **绑定信号槽与控制反馈**：
   - 按钮 `m_viewBtn` 点击时弹出 QMenu，菜单项包括自适应、网格、列表。菜单动作关联至主内容面板 `m_contentPanel->setViewMode(ViewMode)`。
   - 滑杆 `m_sizeSlider` 的数值变化信号关联至 `m_contentPanel->setZoomLevel(int)`（需在 ContentPanel 中公开该接口或直接调用其成员），并触发 `updateGridSize()`。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 滑杆移植 | 在 `MainWindow` 自定义标题栏按钮组中新增水平滑杆 `m_sizeSlider`（范围 32~256，初始值 96，固定宽度 110px，手势为 PointingHandCursor），其数值改变时，将 `ContentPanel` 的缩放级更新并同步重绘视图。 | ✅ |
| 2    | 排列按钮移植 | 在自定义标题栏按钮组中新增 QPushButton 按钮 `m_viewBtn`，配置 `grid` 图标（悬停/按下高亮对齐标准），点击时 popup 一个精美定制样式的 QMenu，包含自适应、网格、列表菜单，并提供单选 checked 勾选。 | ✅ |

## 4. 详细解决方案

### 4.1 在 `MainWindow.h` 中引入声明
在 `MainWindow.h` 的 `private` 作用域内声明以下指针变量：
```cpp
    QSlider* m_sizeSlider = nullptr;
    QPushButton* m_viewBtn = nullptr;
```

### 4.2 在 `MainWindow.cpp` 中构建并进行 QSS 样式设计
在 `MainWindow::setupCustomTitleBarButtons()` 中，实例化滑杆与按钮，设置其属性。

1. **排列选择按钮 `m_viewBtn`**：
```cpp
    m_viewBtn = createTitleBtn("grid");
    m_viewBtn->setProperty("tooltipText", "排列方式");
    m_viewBtn->installEventFilter(m_hoverFilter);

    connect(m_viewBtn, &QPushButton::clicked, this, [this]() {
        QMenu* menu = new QMenu(this);
        UiHelper::applyMenuStyle(menu); // 维持现有的菜单高内聚样式规范

        QAction* actJustified = menu->addAction("自适应");
        actJustified->setCheckable(true);
        actJustified->setChecked(m_contentPanel->currentViewMode() == ContentPanel::JustifiedViewMode);

        QAction* actGrid = menu->addAction("网格");
        actGrid->setCheckable(true);
        actGrid->setChecked(m_contentPanel->currentViewMode() == ContentPanel::GridView);

        QAction* actList = menu->addAction("列表");
        actList->setCheckable(true);
        actList->setChecked(m_contentPanel->currentViewMode() == ContentPanel::ListView);

        QActionGroup* grp = new QActionGroup(menu);
        grp->addAction(actJustified);
        grp->addAction(actGrid);
        grp->addAction(actList);

        connect(actJustified, &QAction::triggered, this, [this]() {
            m_contentPanel->setViewMode(ContentPanel::JustifiedViewMode);
        });
        connect(actGrid, &QAction::triggered, this, [this]() {
            m_contentPanel->setViewMode(ContentPanel::GridView);
        });
        connect(actList, &QAction::triggered, this, [this]() {
            m_contentPanel->setViewMode(ContentPanel::ListView);
        });

        menu->exec(m_viewBtn->mapToGlobal(QPoint(0, m_viewBtn->height() + 2)));
    });
```

2. **尺寸滑杆 `m_sizeSlider`**：
```cpp
    m_sizeSlider = new QSlider(Qt::Horizontal, this);
    m_sizeSlider->setRange(32, 256);
    m_sizeSlider->setValue(m_contentPanel->zoomLevel()); // 默认与 ContentPanel 的初始 zoomLevel 保持一致
    m_sizeSlider->setFixedSize(110, 20);
    m_sizeSlider->setCursor(Qt::PointingHandCursor);
    m_sizeSlider->setStyleSheet(
        "QSlider { background: transparent; margin-right: 5px; }"
        "QSlider::groove:horizontal { height: 3px; background: #333333; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #FF8C00; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 12px; height: 12px; margin: -5px 0; "
        "  background: #FF8C00; border-radius: 6px; }"
    );

    connect(m_sizeSlider, &QSlider::valueChanged, this, [this](int val) {
        m_contentPanel->setZoomLevel(val);
    });
```

### 4.3 在布局容器中插入组件
在 `MainWindow::setupCustomTitleBarButtons()` 布局排版部分，将滑杆与按钮放置在折叠盘符按钮和同步状态按钮的左边，或新建按钮的右侧。例如紧贴在 `m_btnCreate` 新建按钮右侧，或者根据视觉平衡插入：
```cpp
    layout->addWidget(m_btnToggleDriveBar, 0, Qt::AlignVCenter);
    layout->addWidget(m_btnSync, 0, Qt::AlignVCenter);
    layout->addWidget(m_btnLayout, 0, Qt::AlignVCenter);
    layout->addWidget(m_btnCreate, 0, Qt::AlignVCenter);
    layout->addWidget(m_viewBtn, 0, Qt::AlignVCenter); // 插入排列方式按钮
    layout->addWidget(m_sizeSlider, 0, Qt::AlignVCenter); // 插入滑杆
    layout->addWidget(m_btnPinTop, 0, Qt::AlignVCenter);
    // ...
```

### 4.4 为 `ContentPanel` 补充关联辅助方法
为了让滑杆能实时同步，我们需要在 `ContentPanel.h` 中公开公开获取和修改 `zoomLevel` 的接口：
```cpp
// src/ui/ContentPanel.h
public:
    int zoomLevel() const { return m_zoomLevel; }
    void setZoomLevel(int level) {
        m_zoomLevel = level;
        updateGridSize();
    }
    ContentPanel::ViewMode currentViewMode() const { return m_currentViewMode; }
```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/MainWindow.h` 增加成员指针声明。
- [ ] 模块/文件：`src/ui/MainWindow.cpp` 中的 `setupCustomTitleBarButtons` 函数，完成控件初始化、样式及信号槽绑定。
- [ ] 模块/文件：`src/ui/ContentPanel.h` 增加 `zoomLevel()`, `setZoomLevel()`, `currentViewMode()` 的公共访问函数。

**明确禁止越界修改的范围：**
- [ ] 系统扫描底层、数据库及模型数据绑定 —— 严禁触碰
- [ ] 视图本身的绘图 Delegate 逻辑 —— 严禁触碰

## 6. 实现准则与预警【核心】
1. **防爆/找不到标识符**：必须核对在 `MainWindow.cpp` 引入 `<QSlider>` 与 `<QActionGroup>` 头文件，防范未声明错误。
2. **开箱即用样式**：滑杆的 Handle、Track、以及 sub-page 均严格对齐考古版本的色彩，滑槽粗细、高度等完全一致。
3. **自适应处理**：切换不同视图时（如列表与网格），由于模型底层可能对大图和小行高有自动换挡的处理，在调用 `updateGridSize` 时会自适应调整，不会产生排版交叠。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 标题栏按钮样式 | 悬停：#3E3E42（Style::HoverBackground），按下：#4E4E52（Style::PressedBackground），严禁使用 rgba 蒙版 | ✅（移植的排列按钮与 MainWindow 内置按钮一致，使用 `createTitleBtn` 函数生成，无色差） |
| 输入框清除功能 | 一律使用 Qt 原生 setClearButtonEnabled(true)，严禁通过 addAction、手动绘图或自定义按钮模拟清除逻辑 | ✅（本方案不增加或变更任何输入框） |

## 8. 待确认事项（可选）
（无）
