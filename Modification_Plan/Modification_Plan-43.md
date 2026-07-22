# 卡片尺寸滑杆与排列视图按钮功能移植及Ctrl+滚轮线性缩放重构 —— Modification_Plan-43.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
当前版本的内容面板（`ContentPanel`）顶部缺少可以直接对文件卡片大小进行连续、直观调节的滑杆控件，也缺少能一键弹出菜单快速切换不同排版模式的排列选择按钮（对应用户原话：“移植到当前版本里两个“调整卡片尺寸”和“排列”方式的功能”）。此外，当前版本的 `Ctrl+滚轮` 缩放逻辑极其不符合期望（对应用户原话：“替代当前版本的Ctrl+滚轮键的运行逻辑”），其代码在稍微滚动鼠标时就会自动在列表视图与卡片网格视图之间强行越界切换，使用体验极其不连贯。用户期望移植 “FERREX-META” 版本的控件与无级缩放设计，完全革新当前版本的整体交互。

## 2. 问题定位
* **控件缺失**：当前面板顶部的容器尚未声明并配置 `m_sizeSlider` 与 `m_viewBtn` 按钮（对应用户原话：“这两个“调整卡片尺寸”和“排列”方式的功能”）。
* **滚轮事件逻辑过载且臃肿**：当前版本 `src/ui/ContentPanel.cpp` 中的 `wheelEvent` 绑定了硬编码的视图阈值切换代码。一旦放大高度超过 `96px`，或者缩小高度低于 `96px`，就会强行修改 `setViewMode()`，属于交互逻辑职责过载。
* **物理拦截缺失**：当焦点处于列表或网格视图内部时，默认的鼠标滚轮会被其底层的 `viewport()` 优先吞掉，导致用户即使按住 `Ctrl` 在视图中滚动也无法无冲突地调节大卡片尺寸。
* **解决思路**：
  1. 在 `ContentPanel` 工具栏布局的开关按钮最左侧（对应用户原话：“功能移植到当前版本里”），声明并排版添加 `m_sizeSlider` 和 `viewBtn`。
  2. 废除原有的阈值切换视图滚轮逻辑。重写 `ContentPanel::wheelEvent`，当按下 `Ctrl` 时，根据滚轮方向线性、无级地调节 `m_sizeSlider` 步进值，每次加或减 `10` 像素（对应用户原话：“替代当前版本的Ctrl+滚轮键的运行逻辑”）。
  3. 在 `ContentPanel::eventFilter` 中，对所有子视图的 `viewport()` 的 `QEvent::Wheel` 进行物理拦截：如果按下了 `Ctrl`（`Ctrl+滚轮`），同样截获并让其转换为控制 `m_sizeSlider` 步进 `10` 像素（对应用户原话：“替代当前版本的Ctrl+滚轮键的运行逻辑”），最后返回 `true` 吃掉事件以防止视图产生默认纵向滚动。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 我期望将“FERREX-META”版本里这两个“调整卡片尺寸”和“排列”方式的功能移植到当前版本里 | 在当前版本 `ContentPanel` 顶部声明并初始化 `m_sizeSlider` 与 `viewBtn` 按钮，绑定对应的槽连接，完美配置 QSS 样式并加入布局。 | ✅ 一致 |
| 2    | 将“FERREX-META”版本里的Ctrl+滚轮键运行逻辑替代当前版本的Ctrl+滚轮键的运行逻辑 | 完全废除现有的滚轮阈值切换视图机制，改为在 `wheelEvent` 和 `eventFilter` 拦截 `Ctrl+滚轮` 事件，直接调用并增减滑杆 `10` 像素的尺寸，实现无缝无级缩放。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 控件成员变量定义
在 `src/ui/ContentPanel.h` 中追加定义（对应用户原话：“移植到当前版本里两个“调整卡片尺寸”和“排列”方式的功能”）：
```cpp
    QPushButton* m_viewBtn = nullptr;  // 排列方式按钮
    QSlider* m_sizeSlider = nullptr;   // 调整卡片尺寸滑杆
```

### 4.2 按钮及卡片尺寸滑杆初始化与槽绑定（完全对齐 Memories.md 与考古视觉标准）
在 `src/ui/ContentPanel.cpp` 中的 `ContentPanel::initUi()` 工具栏布局处插入以下物理构建逻辑：

1. **排列按钮 `m_viewBtn` 构建（对应用户原话：““排列”方式的功能移植到当前版本里”）**：
   ```cpp
   m_viewBtn = new QPushButton(titleBar);
   m_viewBtn->setFixedSize(24, 24); // 严格满足外框 24x24px 考古规范
   m_viewBtn->setIcon(UiHelper::getIcon("grid", QColor("#CCCCCC"), 18)); // 严格满足图标 18x18px
   m_viewBtn->setIconSize(QSize(18, 18));
   m_viewBtn->setCursor(Qt::PointingHandCursor);
   m_viewBtn->setProperty("tooltipText", "排列方式");
   m_viewBtn->installEventFilter(this);
   m_viewBtn->setStyleSheet(
       "QPushButton { background: transparent; border: none; border-radius: 4px; padding: 0; }"
       "QPushButton:hover { background: #3E3E42; }"    // 悬停色 #3E3E42 考古标准
       "QPushButton:pressed { background: #4E4E52; }"  // 按下色 #4E4E52 考古标准
   );

   connect(m_viewBtn, &QPushButton::clicked, this, [this]() {
       QMenu* menu = new QMenu(this);
       menu->setStyleSheet(
           "QMenu { background: #1A1A1A; color: #CCC; border: 1px solid #333; border-radius: 6px; }"
           "QMenu::item { padding: 6px 24px; }"
           "QMenu::item:selected { background: #2A2A2A; color: #FFF; }"
           "QMenu::item:checked { color: #FF551C; }" // 选中激活色对齐置顶激活橙 #ff551c
       );

       QAction* jModeAct = menu->addAction("自适应(A)");
       jModeAct->setCheckable(true);
       jModeAct->setChecked(m_currentViewMode == JustifiedViewMode);

       QAction* gModeAct = menu->addAction("网格(G)");
       gModeAct->setCheckable(true);
       gModeAct->setChecked(m_currentViewMode == GridView);

       QAction* listModeAct = menu->addAction("列表(L)");
       listModeAct->setCheckable(true);
       listModeAct->setChecked(m_currentViewMode == ListView);

       QActionGroup* modeGrp = new QActionGroup(menu);
       modeGrp->addAction(jModeAct);
       modeGrp->addAction(gModeAct);
       modeGrp->addAction(listModeAct);

       connect(jModeAct, &QAction::triggered, this, [this]() { setViewMode(JustifiedViewMode); });
       connect(gModeAct, &QAction::triggered, this, [this]() { setViewMode(GridView); });
       connect(listModeAct, &QAction::triggered, this, [this]() { setViewMode(ListView); });

       menu->exec(m_viewBtn->mapToGlobal(QPoint(0, m_viewBtn->height() + 2)));
   });
   ```

2. **尺寸滑杆 `m_sizeSlider` 构建（对应用户原话：““调整卡片尺寸”的功能移植到当前版本里”）**：
   ```cpp
   m_sizeSlider = new QSlider(Qt::Horizontal, titleBar);
   m_sizeSlider->setRange(32, 256); // 严格调节范围限制 32 至 256
   m_sizeSlider->setValue(m_zoomLevel);
   m_sizeSlider->setFixedSize(110, 20); // 严格满足滑杆 110px 宽度
   m_sizeSlider->setCursor(Qt::PointingHandCursor);
   m_sizeSlider->installEventFilter(this);
   m_sizeSlider->setStyleSheet(
       "QSlider { background: transparent; margin-right: 1px; }"
       "QSlider::groove:horizontal { height: 3px; background: #3F3F3F; border-radius: 2px; }"
       "QSlider::sub-page:horizontal { background: #FF551C; border-radius: 2px; }" // 填充对齐激活色 #ff551c
       "QSlider::handle:horizontal { width: 12px; height: 12px; margin: -5px 0; "
       "  background: #FF551C; border-radius: 6px; }" // 按钮对齐激活色 #ff551c
   );

   connect(m_sizeSlider, &QSlider::valueChanged, this, [this](int v) {
       m_zoomLevel = v;
       // 更新当前正在使用的活动视图中的卡片/图标渲染高度
       updateGridSize(); // 调和并同步当前视图中的网格高度，消除闪烁
       AppConfig::instance().setValue("UI/GridZoomLevel", m_zoomLevel);
   });
   ```

3. **布局插入位置**：
   在原有标题栏水平布局 `titleL` 中，将新增加的两个核心控件塞入到原有开关按钮的最左侧（对应用户原话：“功能移植到当前版本里”）：
   ```cpp
   titleL->addWidget(titleLabel);
   titleL->addStretch();
   titleL->addWidget(m_viewBtn);     // 排列方式选择按钮
   titleL->addWidget(m_sizeSlider);  // 尺寸调整滑动杆
   titleL->addWidget(m_btnToggleFolders, 0, Qt::AlignVCenter);
   ```

### 4.3 重构 Ctrl+滚轮 线性、无级缩放运行逻辑（替代旧臃肿逻辑）
1. **替换 `ContentPanel::wheelEvent` 实现（对应用户原话：“Ctrl+滚轮键的运行逻辑替代当前版本”）**：
   ```cpp
   void ContentPanel::wheelEvent(QWheelEvent* event) {
       if (event->modifiers() & Qt::ControlModifier) {
           int delta = event->angleDelta().y();
           if (m_sizeSlider) {
               // 每次滚轮滚动，平滑无级加/减 10 像素大小
               if (delta > 0) {
                   m_sizeSlider->setValue(m_sizeSlider->value() + 10);
               } else if (delta < 0) {
                   m_sizeSlider->setValue(m_sizeSlider->value() - 10);
               }
           }
           event->accept();
       } else {
           QWidget::wheelEvent(event);
       }
   }
   ```

2. **在 `ContentPanel::eventFilter` 中对子视图的 Viewport 进行滚轮拦截物理重写（对应用户原话：“替代当前版本的Ctrl+滚轮键的运行逻辑”）**：
   ```cpp
   if (event->type() == QEvent::Wheel) {
       // 检测是否是在我们注册过的子视图（或其 viewport）内部发生的滚轮动作
       if (obj == m_treeView || obj == m_treeView->viewport() || obj == m_gridView || obj == m_gridView->viewport()) {
           QWheelEvent* wEvent = static_cast<QWheelEvent*>(event);
           if (wEvent->modifiers() & Qt::ControlModifier) {
               int deltaY = wEvent->angleDelta().y();
               if (m_sizeSlider) {
                   // 拦截底层事件，直接修改卡片滑杆的数值以更新卡片尺寸，每次步进 10 像素
                   if (deltaY > 0) {
                       m_sizeSlider->setValue(m_sizeSlider->value() + 10);
                   } else if (deltaY < 0) {
                       m_sizeSlider->setValue(m_sizeSlider->value() - 10);
                   }
               }
               return true; // 拦截并消费此滚轮事件，掐断底层纵向滚动动作
           }
       }
   }
   ```

3. **对滑杆 `m_sizeSlider` 自身的点击跳转逻辑拦截物理配置**：
   ```cpp
   if (watched == m_sizeSlider && event->type() == QEvent::MouseButtonPress) {
       QMouseEvent* me = static_cast<QMouseEvent*>(event);
       if (me->button() == Qt::LeftButton) {
           int val = QStyle::sliderValueFromPosition(m_sizeSlider->minimum(), m_sizeSlider->maximum(), me->pos().x(), m_sizeSlider->width());
           m_sizeSlider->setValue(val);
           return true; // 消费此点击事件以实现精准直接跳转
       }
   }
   ```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/ContentPanel.h` — 声明 `m_sizeSlider`、`m_viewBtn` 等控件指针。
- [ ] 模块/文件：`src/ui/ContentPanel.cpp` — 负责控件实例化、QSS 样式渲染、槽逻辑绑定、以及 `wheelEvent` 和 `eventFilter` 两个核心位置物理拦截 `Ctrl+滚轮` 事件并驱动滑杆数值变化。

**明确禁止越界修改的范围：**
- [ ] 模块/文件：`src/ui/TreeItemDelegate.h` —— 不修改其绘制逻辑。
- [ ] 模块/文件：`src/ui/JustifiedView.cpp` —— 不修改其底层项大小重绘逻辑。

## 6. 实现准则与预警【核心】
1. **依赖的头文件**：必须保证 `src/ui/ContentPanel.cpp` 中包含了 `<QSlider>`、`<QPushButton>`、`<QMenu>`、`<QActionGroup>`、`<QWheelEvent>` 等 Qt 内置交互组件库文件，消除编译时“未定义类型”的风险。
2. **防崩溃空保护**：在 `wheelEvent` 和 `eventFilter` 中对 `m_sizeSlider` 进行操作前，必须进行 `if (m_sizeSlider)` 非空判定。
3. **消除越权干扰**：滑杆和滚轮在任何情况下，不能强行对 `setViewMode` 展开间接控制。`Ctrl+滚轮` 的作用域仅限制在滑杆数值的加减上。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 品牌橙色 / 置顶激活色 | 品牌色与置顶激活色独立（杜绝任何形式的色值脑补），置顶激活色为唯一合法色值 `#ff551c`。滑杆激活状态色值需对准 `#ff551c`。 | ✅ 符合，`m_sizeSlider` 的 `sub-page` 与 `handle` 样式完全配置对齐 `#ff551c`。 |
| 按钮物理参数 | 外框物理尺寸固定为 `24x24px`，图标尺寸为 `18x18px`，圆角为 `4px`，默认背景透明，悬停状态为 `#3E3E42`，按下状态为 `#4E4E52`。 | ✅ 符合，`m_viewBtn` 按钮的外框、图标尺寸、圆角以及悬停/按下背景色与规范完全一致。 |

## 8. 待确认事项
无。
