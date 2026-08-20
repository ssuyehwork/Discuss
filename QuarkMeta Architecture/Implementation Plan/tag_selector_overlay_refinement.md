# TagSelectorOverlay 界面精细化改造无脑实施方案 (TagSelectorOverlay Refinement Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在对 **`TagSelectorOverlay`（标签选择悬浮界面）** 进行精细化 UI 与交互改造：
1. **新增侧边栏折叠按钮（`m_btnToggleSidebar`）**：在顶部搜索框右侧加入固定 26x26 的 SVG 矢量图标按钮，平滑控制左侧分类群组列表（`m_groupList`）的折叠与显示。
2. **彻底移除底部快捷键提示栏**：物理删除 `initUi()` 中原本包含 Tab/方向键/Enter/ESC 帮助提示文字的 `bottomBar` 控件，精简界面高度。
3. **实现窗口尺寸（Size）持久化**：重写 `resizeEvent` 与 `mouseReleaseEvent`，在拖拽改变窗口大小后将尺寸写入 `AppConfig` 的 `TagSelectorOverlay/Size` 节点，再次唤起时完美还原上次尺寸。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/TagSelectorOverlay.h`
2. `src/ui/TagSelectorOverlay.cpp`
3. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/TagSelectorOverlay.h`
在 `TagSelectorOverlay.h` 中新增 `resizeEvent` 声明与 `m_btnToggleSidebar` 按钮成员变量。

```
<<<<<<< SEARCH
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
=======
protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QMap<QString, QPushButton*> m_tagButtons;
=======
    QMap<QString, QPushButton*> m_tagButtons;
    QPushButton* m_btnToggleSidebar = nullptr; // 搜索框右侧的侧边栏折叠按钮
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/TagSelectorOverlay.cpp`
在 `initUi()` 中引入顶部 `topSearchLayout`（搜索框 + 折叠按钮），移除底部的 `bottomBar`，并在文件末尾新增 `resizeEvent` 与在 `mouseReleaseEvent` 中持久化尺寸。

```
<<<<<<< SEARCH
    // 搜索与创建栏
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("搜索或新建标签...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedHeight(26);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background: #151515; border: 1px solid #333; border-radius: 4px; padding: 0 8px; color: #EEE; font-size: 11px; }"
        "QLineEdit:focus { border-color: #1C97EA; }"
    );
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() {
        filterTags();
    });
    mainL->addWidget(m_searchEdit);

    // 中部双视口（左侧群组、右侧标签）
    QHBoxLayout* bodyL = new QHBoxLayout();
=======
    // 1. 顶部操作栏（搜索框 + 右侧侧边栏折叠按钮）
    QHBoxLayout* topSearchLayout = new QHBoxLayout();
    topSearchLayout->setContentsMargins(0, 0, 0, 0);
    topSearchLayout->setSpacing(6);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("搜索或新建标签...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedHeight(26);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background: #151515; border: 1px solid #333; border-radius: 4px; padding: 0 8px; color: #EEE; font-size: 11px; }"
        "QLineEdit:focus { border-color: #1C97EA; }"
    );
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() {
        filterTags();
    });
    topSearchLayout->addWidget(m_searchEdit, 1);

    m_btnToggleSidebar = new QPushButton(this);
    m_btnToggleSidebar->setFixedSize(26, 26);
    m_btnToggleSidebar->setCheckable(true);
    m_btnToggleSidebar->setChecked(true);
    m_btnToggleSidebar->setIcon(UiHelper::getIcon("sidebar", QColor("#AAAAAA"), 16));
    m_btnToggleSidebar->setIconSize(QSize(16, 16));
    m_btnToggleSidebar->setCursor(Qt::PointingHandCursor);
    m_btnToggleSidebar->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-radius: 4px; padding: 0; }"
        "QPushButton:hover { background-color: #3E3E42; }"
        "QPushButton:pressed { background-color: #4E4E52; }"
    );
    connect(m_btnToggleSidebar, &QPushButton::toggled, this, [this](bool checked) {
        if (m_groupList) m_groupList->setVisible(checked);
    });
    topSearchLayout->addWidget(m_btnToggleSidebar);

    mainL->addLayout(topSearchLayout);

    // 中部双视口（左侧群组、右侧标签）
    QHBoxLayout* bodyL = new QHBoxLayout();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 底部快捷键提示栏
    QWidget* bottomBar = new QWidget(this);
    bottomBar->setFixedHeight(22);
    bottomBar->setStyleSheet("background-color: #151515; border-radius: 3px;");
    QHBoxLayout* bottomL = new QHBoxLayout(bottomBar);
    bottomL->setContentsMargins(8, 0, 8, 0);

    QLabel* helpTips = new QLabel(bottomBar);
    helpTips->setText("切换 <font color='#1C97EA'><b>Tab</b></font>    移动 <font color='#1C97EA'><b>↑↓←→</b></font>    选中/新建 <font color='#1C97EA'><b>⏎</b></font>");
    helpTips->setStyleSheet("color: #888; font-size: 10px;");
    bottomL->addWidget(helpTips);

    bottomL->addStretch();

    QLabel* closeTips = new QLabel("关闭 ESC", bottomBar);
    closeTips->setStyleSheet("color: #888; font-size: 10px;");
    bottomL->addWidget(closeTips);

    mainL->addWidget(bottomBar);

    resize(400, 240); // 初始大小
=======
    // 从配置中恢复持久化的窗口尺寸
    QSize savedSize = AppConfig::instance().getValue("TagSelectorOverlay/Size", QSize(400, 240)).toSize();
    resize(savedSize.expandedTo(QSize(250, 150)));
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void TagSelectorOverlay::mouseReleaseEvent(QMouseEvent* event) {
    m_isDragging = false;
    m_resizeDir = 0;
    setCursor(Qt::ArrowCursor);
    QFrame::mouseReleaseEvent(event);
}
=======
void TagSelectorOverlay::mouseReleaseEvent(QMouseEvent* event) {
    if (m_resizeDir != 0) {
        AppConfig::instance().setValue("TagSelectorOverlay/Size", size());
    }
    m_isDragging = false;
    m_resizeDir = 0;
    setCursor(Qt::ArrowCursor);
    QFrame::mouseReleaseEvent(event);
}

void TagSelectorOverlay::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    if (isVisible()) {
        AppConfig::instance().setValue("TagSelectorOverlay/Size", size());
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在命令行运行 CMake 编译，验证全工程无符号缺失报错：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **功能验证**：
   - **折叠按钮验证**：唤起 `TagSelectorOverlay`，点击搜索框右侧折叠按钮，确认左侧分类列表能够平滑收起/展开。
   - **底栏移除验证**：确认界面底部没有多余的“快捷键提示栏”，视觉精简。
   - **尺寸持久化验证**：用鼠标拖拽拉伸选择器窗口大小后关闭，再次打开确认尺寸精确还原为上一次调整后的状态。
