# Implementation Plan: 智能文件夹评分纯矢量渲染与日历弹出框尺寸修复实施方案

Base Commit Hash: `2ed3357542a13cdfa996de90da4b0a0a52f91c3b`  
Target Files:
- `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`
- `QuarkMeta_Source/resources/style.qss`

本实施方案旨在严格遵循顶层架构规范与《Memories.md》（章节 0、章节 9 及章节 27），在 `QuarkMeta_Source` 桌面应用中彻底解决以下两个核心问题：
1. **智能文件夹评分下拉列表框违规文字清除**：彻底消除“1 星”、“2 星”等阿拉伯数字与文字组合，清除“无 (0星)”中冗余括号说明，纯净规范为“无评级”，1~5 星仅使用 SVG 矢量图标（`star_filled`）进行纯图形渲染，文本置空，实现星级表达 SSOT 契约 100% 物理合规；
2. **日历弹出框宽度不足导致星期表头被挤截断成省略号（`...`）缺陷修复**：为 `QDateEdit` 弹出之内部 `QCalendarWidget` 设置充足的最小宽度（`setMinimumWidth(260)`），并在全局 QSS 中补齐清晰舒展的暗色日历表头与单元格样式，彻底消除省略号截断。

---

## 前置：已读物理证据清单 (Pre-Draft Physical Evidence)

### 1. 物理读取文件列表与相对路径
- `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.h` (已确认无接口破坏)
- `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp` (核心逻辑修改)
- `QuarkMeta_Source/resources/style.qss` (日历弹窗样式补充)
- `Memories.md` (章节 0、章节 9、章节 27 星级表达与日历规范 SSOT)

### 2. 头文件原样声明签名与接口
- `QDateEdit::calendarWidget`:
  ```cpp
  QCalendarWidget* calendarWidget() const;
  ```
- `QWidget::setMinimumWidth`:
  ```cpp
  void setMinimumWidth(int minw);
  ```
- `QComboBox::addItem`:
  ```cpp
  void addItem(const QIcon& icon, const QString& text, const QVariant& userData = QVariant());
  ```
- `UiHelper::getIcon`:
  ```cpp
  static QIcon getIcon(const QString& name, const QColor& color = QColor(), int size = 16);
  ```

### 3. 既有违规代码检索证据 (Search-First)
- `SmartFolderValueWidgets.cpp:39`:
  ```cpp
  m_combo->addItem("无 (0星)", 0);
  ...
  m_combo->addItem(QIcon(pixmap), QString("%1 星").arg(stars), stars);
  ```
  直接违背了 `Memories.md` 章节 9“绝对禁止使用‘1星’、‘2星’等阿拉伯数字与文字组合”。
- `SmartFolderValueWidgets.cpp:85, 112, 117`:
  ```cpp
  m_dateEdit->setCalendarPopup(true);
  ```
  未对 `calendarWidget()` 赋予最小宽度，导致弹出日历被紧凑父布局宽度严重挤压，星期表头被截断为 `...`。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题根因与背景
1. **评分表达违背 SSOT 铁律**：
   在 `Memories.md` 第 0 章与第 9 章中明确指出：“全软件所有涉及星级评级的 UI 展示、下拉选择框、筛选菜单与规则编辑器，绝对禁止使用‘1星’、‘2星’等阿拉伯数字与文字组合，同时也严格遵守‘严禁使用 Unicode 符号替代图标’的红线！必须统一使用 SVG 矢量图标（`star_filled`）动态绘制 1~5 颗星的 Pixmap/QIcon 进行渲染展示”。而原 `SmartFolderValueWidgets.cpp` 中存在硬编码字符串 `QString("%1 星").arg(stars)`，导致下拉框出现“1 星”、“2 星”等违规文字。
2. **日历弹出框宽度不足表头挤压截断**：
   在 `SmartFolderValueWidgets.cpp` 的 `SingleDateValueWidget` 与 `DateRangeValueWidget` 中，`QDateEdit` 开启了 `setCalendarPopup(true)`。当用户点击下拉日历时，Qt 默认弹出的 `QCalendarWidget` 宽度受限，其水平表头（HeaderView）每一列单元格宽度无法容纳中文字符“一”、“二”至“日”，直接触发了 Qt ItemViews 的 `Elide` 策略，被全部截断成了 `...`。

### 1.2 重构设计目标
1. **评分下拉框纯矢量净化**：
   - 0 星选项文本对齐为“**无评级**”；
   - 1~5 星选项文本**完全置为空字符串 `""`**，只展示动态绘制的 SVG 矢量星星图标；
2. **日历弹出框宽度赋能与表头舒展**：
   - 在构造 `QDateEdit` 开启 `setCalendarPopup(true)` 后，显式获取其 `calendarWidget()` 并调用 `setMinimumWidth(260)`；
   - 在 `resources/style.qss` 中配置 `QCalendarWidget` 暗色风格与单元格充足留白，彻底解决截断为 `...` 的视觉缺陷。

---

## Chapter 2: Modified Files List（影响文件清单）

本次方案仅影响 2 个源文件，无需新建或删除任何文件：
1. `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp` (修改)
2. `QuarkMeta_Source/resources/style.qss` (修改)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`

<<<<<<< SEARCH
#include <QPainter>
#include <QPixmap>
=======
#include <QPainter>
#include <QPixmap>
#include <QCalendarWidget>
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 0 Stars / Unrated
    m_combo->addItem("无 (0星)", 0);

    const int starSize = 14;
    const int spacing = 2;
    for (int stars = 1; stars <= 5; ++stars) {
        int maxW = 5 * starSize + 4 * spacing;
        QPixmap pixmap(maxW, starSize);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        int x = 0;
        for (int i = 0; i < stars; ++i) {
            QIcon icon = UiHelper::getIcon("star_filled", QColor("#FFB800"));
            icon.paint(&painter, x, 0, starSize, starSize);
            x += starSize + spacing;
        }
        for (int i = stars; i < 5; ++i) {
            QIcon icon = UiHelper::getIcon("star_filled", QColor("#444444"));
            icon.paint(&painter, x, 0, starSize, starSize);
            x += starSize + spacing;
        }

        m_combo->addItem(QIcon(pixmap), QString("%1 星").arg(stars), stars);
    }
=======
    // 0 Stars / Unrated
    m_combo->addItem("无评级", 0);

    const int starSize = 14;
    const int spacing = 2;
    for (int stars = 1; stars <= 5; ++stars) {
        int maxW = 5 * starSize + 4 * spacing;
        QPixmap pixmap(maxW, starSize);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        int x = 0;
        for (int i = 0; i < stars; ++i) {
            QIcon icon = UiHelper::getIcon("star_filled", QColor("#FFB800"));
            icon.paint(&painter, x, 0, starSize, starSize);
            x += starSize + spacing;
        }
        for (int i = stars; i < 5; ++i) {
            QIcon icon = UiHelper::getIcon("star_filled", QColor("#444444"));
            icon.paint(&painter, x, 0, starSize, starSize);
            x += starSize + spacing;
        }

        // 严格遵循 Memories.md 第 9 章：禁止使用“1星”等文字组合，仅显示纯矢量星级图标
        m_combo->addItem(QIcon(pixmap), "", stars);
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
SingleDateValueWidget::SingleDateValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setObjectName("SmartFolderDateEdit");
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    layout->addWidget(m_dateEdit);
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &ISmartFolderValueWidget::valueChanged);
}
=======
SingleDateValueWidget::SingleDateValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setObjectName("SmartFolderDateEdit");
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    if (m_dateEdit->calendarWidget()) {
        m_dateEdit->calendarWidget()->setMinimumWidth(260);
    }
    layout->addWidget(m_dateEdit);
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &ISmartFolderValueWidget::valueChanged);
}
>>>>>>> REPLACE

<<<<<<< SEARCH
DateRangeValueWidget::DateRangeValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_date1Edit = new QDateEdit(QDate::currentDate(), this);
    m_date1Edit->setObjectName("SmartFolderDateEdit");
    m_date1Edit->setCalendarPopup(true);
    m_date1Edit->setDisplayFormat("yyyy-MM-dd");

    m_date2Edit = new QDateEdit(QDate::currentDate(), this);
    m_date2Edit->setObjectName("SmartFolderDateEdit");
    m_date2Edit->setCalendarPopup(true);
    m_date2Edit->setDisplayFormat("yyyy-MM-dd");
=======
DateRangeValueWidget::DateRangeValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_date1Edit = new QDateEdit(QDate::currentDate(), this);
    m_date1Edit->setObjectName("SmartFolderDateEdit");
    m_date1Edit->setCalendarPopup(true);
    m_date1Edit->setDisplayFormat("yyyy-MM-dd");
    if (m_date1Edit->calendarWidget()) {
        m_date1Edit->calendarWidget()->setMinimumWidth(260);
    }

    m_date2Edit = new QDateEdit(QDate::currentDate(), this);
    m_date2Edit->setObjectName("SmartFolderDateEdit");
    m_date2Edit->setCalendarPopup(true);
    m_date2Edit->setDisplayFormat("yyyy-MM-dd");
    if (m_date2Edit->calendarWidget()) {
        m_date2Edit->calendarWidget()->setMinimumWidth(260);
    }
>>>>>>> REPLACE

---

### 3.2 `QuarkMeta_Source/resources/style.qss`

<<<<<<< SEARCH
QDateEdit#SmartFolderDateEdit {
    background-color: #252526;
    color: #EEEEEE;
    border: 1px solid #333333;
    border-radius: 4px;
    padding: 2px 6px;
    min-height: 24px;
}
=======
QDateEdit#SmartFolderDateEdit {
    background-color: #252526;
    color: #EEEEEE;
    border: 1px solid #333333;
    border-radius: 4px;
    padding: 2px 6px;
    min-height: 24px;
}

QCalendarWidget {
    background-color: #1E1E1E;
    color: #EEEEEE;
}

QCalendarWidget QWidget#qt_calendar_navigationbar {
    background-color: #252526;
    min-height: 28px;
}

QCalendarWidget QAbstractItemView {
    background-color: #1E1E1E;
    color: #EEEEEE;
    selection-background-color: rgba(55, 138, 221, 0.35);
    selection-color: #FFFFFF;
    outline: none;
}
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译验证步骤
```bash
cmake --build build --target QuarkMeta
```

### 4.2 运行时验收标准
1. **评分下拉框纯矢量验收**：
   - 打开“智能文件夹规则配置”对话框；
   - 添加或查看一条“评分”规则；
   - 点击第三列的评分下拉列表框；
   - **预期现象**：
     - 首项为“无评级”，无多余 `(0星)` 文本；
     - 1 到 5 星选项均只有纯净饱满的矢量星星图标（未激活为深灰底色，激活为亮黄图标），**彻底消除“1 星”、“2 星”等违规文字拼凑**。
2. **日历弹出框宽度舒展验收**：
   - 添加或查看一条“添加日期”、“修改日期”或“创建日期”规则；
   - 点击日期输入框右侧的日历下拉按钮；
   - **预期现象**：
     - 弹出之日历控件宽度保持在 260px 以上；
     - 顶部的星期表头完整清晰显示“一”、“二”、“三”、“四”、“五”、“六”、“日”（或系统语言完整显示），**绝对不再出现省略号 `...` 截断**；
     - 日期数字网格舒展对齐，选中高亮保持 QuarkMeta 主题蓝规范。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **矢量图标绘制 SSOT**：
   复用 `UiHelper::getIcon("star_filled", ...)`，严格保持与 `FilterPanel`、`MetaPanel`、`QuickLookWindow` 等全软件所有星级图标的来源 100% 一致。
2. **日历控件原生 API 复用**：
   通过 `QDateEdit::calendarWidget()->setMinimumWidth(260)` 赋能标准日历尺寸，杜绝重新造一个独立的日历组件，零多余技术负债。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 / 控件 | 唯一 SSOT 数据源 | 派生副本 / 观察者 | 同步与刷新机制 |
| :--- | :--- | :--- | :--- |
| 评分规则值 | `SmartFolderRule::value` / `num1` (整型 0~5) | `StarRatingValueWidget::m_combo` | 选定后通过 `getRuleValues` 序列化为字符串与整型保存 |
| 日期规则值 | `SmartFolderRule::date1` / `date2` (`yyyy-MM-dd`) | `SingleDateValueWidget` / `DateRangeValueWidget` | `QDateEdit::dateChanged` 触发规则同步与高度自适应 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 外部调用接口签名物理核查
- `QDateEdit::calendarWidget() const`:
  - Qt 原生公有成员函数，返回底层 `QCalendarWidget*` 指针；
- `QWidget::setMinimumWidth(int minw)`:
  - Qt 原生公有成员函数；
- `UiHelper::getIcon(const QString& name, const QColor& color, int size)`:
  - 声明文件：`QuarkMeta_Source/src/ui/UiHelper.h:35`
  - 访问级别：`public static`，完全一致。

### 7.2 新增与修改接口全链核查
本方案未新增或修改任何头文件类签名，所有头文件保持冻结只读。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

### 8.1 显式 `#include` 闭合性分析
在 `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp` 头部显式补充了：
- `#include <QCalendarWidget>`：确保在操作 `m_dateEdit->calendarWidget()` 及其指针方法 `setMinimumWidth` 时获得完整的类型定义，杜绝前置声明不完整类型引起的编译失败。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 状态 | 实施方案具体位置 / 说明 |
| :---: | :--- | :---: | :--- |
| 1 | 方案是否物理隔离在 `Implementation Plan/` 目录下 | 满足 | `Implementation Plan/Smart_Folder_Rating_And_Calendar_Fix_Plan.md` |
| 2 | 是否使用纯中文进行阐述与注释说明 | 满足 | 全文使用中文进行技术阐述 |
| 3 | 是否包含全部 10 大必备章节规范 | 满足 | 完整包含 Chapter 1 至 Chapter 10 |
| 4 | 前置是否包含“已读物理证据清单” | 满足 | 前置完整列出读取文件与违规事实代码证据 |
| 5 | SEARCH 块是否在目标文件中物理存在且唯一匹配 | 满足 | 经过自动化脚本物理全词校验，100% 唯一匹配 |
| 6 | 对外公开 `.h` 接口签名是否处于只读冻结状态 | 满足 | 零修改头文件签名 |
| 7 | 是否杜绝 Win32 强夺焦点等平台级 Hack | 满足 | 零平台 Hack，基于标准 Qt 原生尺寸与样式 |
| 8 | 是否杜绝在 C++ 代码中内联 `setStyleSheet` | 满足 | 样式全部收敛到 `resources/style.qss` |
| 9 | 星级是否彻底消除数字拼凑与 Unicode 符号 | 满足 | 彻底删除 `"%1 星"`，仅保留矢量星星 Pixmap/Icon |
| 10 | 日历是否设定足够宽度消除省略号截断 | 满足 | 显式设置 `setMinimumWidth(260)` 彻底消除省略号 |
| 11 | 是否避免在异步循环内重复实例化原子变量 | 不涉及 | 本方案未涉及并发循环 |
| 12 | 异步 Lambda 是否避免按引用捕获局部栈变量 | 不涉及 | 无异步 Lambda |
| 13 | 递归 Lambda 是否避免捕获自身 shared_ptr | 不涉及 | 无递归结构 |
| 14 | 是否避免在 for 循环中并发 dispatch 顺序操作 | 不涉及 | 无文件写操作 |
| 15 | 新引入的类型是否包含显式 `#include` 头文件 | 满足 | 显式包含了 `<QCalendarWidget>` |
| 16 | CMakeLists.txt 是否完整同步注册与清理 | 满足 | 仅修改现有文件，无需调整 CMake |
| 17 | 是否完成用户设计理念向 `Memories.md` 的归档 | 满足 | 已归档至 `Memories.md` 第 27 章 |
| 18 | 是否运行自动化验证工具并附带全绿日志 | 满足 | 见下文自动化脚本运行输出日志 |

### 自动化验证工具运行输出日志
```
PASS: All 10 required chapters present.
PASS: Block 1 in QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp uniquely matches.
PASS: Block 2 in QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp uniquely matches.
PASS: Block 3 in QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp uniquely matches.
PASS: Block 4 in QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp uniquely matches.
PASS: Block 5 in QuarkMeta_Source/resources/style.qss uniquely matches.
```

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 行为与交互零篡改声明
| 既有功能 / 交互模式 | 方案处理方式 | 行为变化说明 |
| :--- | :--- | :--- |
| 评分选择与数据过滤 | 原封不动保留 | 选中后 `getRuleValues` / `setRuleValues` 逻辑完全不变 |
| 日期选择与数据过滤 | 原封不动保留 | `dateChanged` 触发规则同步机制完全不变 |
| 评分展示形式 | 视觉合规整改 | 去除违规文字，恢复纯矢量图标呈现 |
| 日历弹窗展示形式 | 尺寸优化整改 | 消除宽度不足导致的省略号，完整呈现星期表头 |

### 10.2 样式数值零篡改声明
| 属性项 | 既有规范数值 | 方案使用数值 | 偏差 |
| :--- | :--- | :--- | :---: |
| 星星图标尺寸 | `14px` | `14` | 0 |
| 星星间隙 | `2px` | `2` | 0 |
| 日历弹窗最小宽度 | 无（原默认导致截断） | `260px`（舒展显示阈值） | 优化新增 |
| 日历导航栏高度 | `28px` | `28px` | 0 |
| 日历选中底色 | `rgba(55, 138, 221, 0.35)` | `rgba(55, 138, 221, 0.35)` | 0 |
