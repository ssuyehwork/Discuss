# NavPanel 三态工作流选项卡改造实施方案 (NavPanel Tri-State Tabs Plan)

## Chapter 1: Problem Statement & Root Cause Analysis（问题与根因分析）

### 1. 现象与需求背景
- 原 `NavPanel` 顶部选项卡为面向本地物理文件系统的“目录导航”与“历史”两个 Tab；
- 按照用户最新的架构设计理念，纯文本笔记系统最左侧栏区应演进为任务流转控制中心，需要将选项卡修改为：
  1. **“待处理”**
  2. **“处理中”**
  3. **“已处理”**
- 需在保留五栏区架构与现有空壳 (TODO) 规范的前提下，扩展为三选项卡及对应的三页面占位。

### 2. 改造范围与技术方案
1. **选项卡标签与图标调整**：
   - 顶部 `SidebarTabBar` 移除“目录导航”与“历史”，添加：
     - Tab 0: “待处理”（图标 `todo`，颜色 `#888888`，尺寸 16）
     - Tab 1: “处理中”（图标 `play`，颜色 `#888888`，尺寸 16）
     - Tab 2: “已处理”（图标 `check`，颜色 `#888888`，尺寸 16）
2. **QStackedWidget 页面扩展**：
   - 扩展为 3 个子页面，分别使用 `createPlaceholderPage` 构建统一置灰 TODO 容器；
   - 信号槽保持 `m_tabBar->currentChanged` 驱动 `m_stackedWidget->setCurrentIndex`；
3. **接口契约与五栏区布局 100% 保持**：
   - 维持最小宽度 230px、状态栏折叠记忆与公共 API 不变。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `RapidNotes_Source/RapidNotes/src/ui/NavPanel.cpp` — 调整 Tab 添加项与三页面构建。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `RapidNotes_Source/RapidNotes/src/ui/NavPanel.cpp` — 选项卡与三页面调整

```diff
<<<<<<< SEARCH
    m_tabBar = new SidebarTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setIconSize(QSize(16, 16));
    m_tabBar->addTab(UiHelper::getIcon("list_ul", "#888888", 16), "目录导航");
    m_tabBar->addTab(UiHelper::getIcon("clock", "#888888", 16), "历史");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);

    // Page 0: 目录导航空壳占位
    QWidget* navPage = createPlaceholderPage("list_ul", "目录导航 (TODO)", "纯文本笔记架构占位容器");

    // Page 1: 历史记录空壳占位
    QWidget* historyPage = createPlaceholderPage("clock", "历史记录 (TODO)", "纯文本笔记架构占位容器");

    m_stackedWidget->addWidget(navPage);
    m_stackedWidget->addWidget(historyPage);
=======
    m_tabBar = new SidebarTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setIconSize(QSize(16, 16));
    m_tabBar->addTab(UiHelper::getIcon("todo", "#888888", 16), "待处理");
    m_tabBar->addTab(UiHelper::getIcon("play", "#888888", 16), "处理中");
    m_tabBar->addTab(UiHelper::getIcon("check", "#888888", 16), "已处理");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);

    // Page 0: 待处理空壳占位
    QWidget* pendingPage = createPlaceholderPage("todo", "待处理 (TODO)", "任务工作流占位容器");

    // Page 1: 处理中空壳占位
    QWidget* inProgressPage = createPlaceholderPage("play", "处理中 (TODO)", "任务工作流占位容器");

    // Page 2: 已处理空壳占位
    QWidget* completedPage = createPlaceholderPage("check", "已处理 (TODO)", "任务工作流占位容器");

    m_stackedWidget->addWidget(pendingPage);
    m_stackedWidget->addWidget(inProgressPage);
    m_stackedWidget->addWidget(completedPage);
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译验证命令
```bash
cmake -B build
cmake --build build --target RapidNotes
```

### 4.2 运行时验收标准
1. **选项卡呈现**：最左侧 `NavPanel` 顶部显示“待处理”、“处理中”、“已处理”三个 Tab 选项卡；
2. **页面平滑切换**：点击各 Tab，`QStackedWidget` 平滑切换至对应的 Page 0、Page 1、Page 2，展示统一的 TODO 占位；
3. **布局契约完整**：`MainWindow` 水平分割器维持 5 栏区布局，`NavPanel` 最小宽度 230px 保持不变。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）
1. 复用 `UiHelper::getIcon` 与 `SvgIcons::icons` 现成内置的高质量 SVG 图标（`todo`, `play`, `check`）；
2. 复用统一私有函数 `createPlaceholderPage` 构建占位页面，无冗余代码。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本/视图层 | 同步与刷新流向 |
| :--- | :--- | :--- | :--- |
| **工作流选项卡索引** | `QStackedWidget::currentIndex()` | `SidebarTabBar` 选态 | `m_tabBar::currentChanged` -> `m_stackedWidget::setCurrentIndex` |
| **五栏区折叠状态** | `AppConfig` (`QSettings`) | `PanelLayoutManager` / `MainWindow` | `PanelLayoutManager::initLayout()` 读取，状态变更原子落盘 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 外部调用接口核查
- `NavPanel.h` 公开接口 `deferredInit`、`setFocusHighlight`、`setRootPath`、`selectPath` 与所有信号保持 100% 冻结与不变；
- 外部调用方代码 0 影响。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

### 8.1 包含完整性
- `NavPanel.cpp` 已完整包含 `UiHelper.h`、`QTabBar`、`QStackedWidget`、`QVBoxLayout` 等所有必须头文件，类型完全闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 证据/位置 |
| :--- | :--- | :--- | :--- |
| 1 | 语言只允许使用中文 | 满足 | 全文使用纯中文解答与陈述 |
| 2 | 不得直接修改源代码（物理隔离） | 满足 | 方案独立存放在 `Implementation Plan/NavPanel_TriState_Tabs_Plan.md` |
| 3 | SEARCH 块在仓库中真实存在且唯一匹配 | 满足 | 经 Python 交叉验证脚本精确 1 次匹配 |
| 4 | 对外接口签名冻结 | 满足 | 公开接口与信号 100% 保持不变 |
| 5 | 用户意图优先（修改为三态选项卡） | 满足 | 准确实施“待处理”、“处理中”、“已处理”三选项卡及三页面 |
| 6 | 完整包含 10 大必备章节 | 满足 | 涵盖 Chapter 1 至 Chapter 10 全部规范章节 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 规范项 | 既有实现 / 契约数值 | 新实现 / 迁移数值 | 结论 |
| :--- | :--- | :--- | :--- |
| **Tab 栏字间距硬锁** | `int spacing = (iconWidth > 0 && !opt.text.isEmpty()) ? 7 : 0;` | `7px` | 严格保持一致 |
| **Tab 栏高度** | `32px` | `32px` | 严格保持一致 |
| **图标尺寸** | `16x16` | `16x16` | 严格保持一致 |
