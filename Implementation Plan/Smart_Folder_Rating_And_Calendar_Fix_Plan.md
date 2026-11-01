# 智能文件夹“星级”纯矢量呈现与日历弹窗宽度修复方案

## Chapter 1: Overview（方案概述与背景剖析）

### 1.1 背景与用户核心痛点
1. **智能文件夹星级规则视觉与命名缺陷**：
   - 第一列规则类型原名称为“评分”，应统一规范更名为“**星级**”；
   - 原星级选择下拉框（`StarRatingValueWidget`）中包含了违背规范的“无 (0星)”以及带有文字拼凑的“%1 星”；
   - 原绘制方案将 5 颗星绘制在一张宽图上，但 `QComboBox` 默认 `iconSize` 仅为 16×16，导致整行星星被强制等比缩放为不可辨认的细小微点（如用户提供的图一）；
   - 用户明确提供的**图二**设计要求：
     - 类型名称展示为“**星级**”；
     - 移除“无评级”项，直接提供 1 星至 5 星的纯矢量星级选项；
     - 每一行仅绘制对应数量的高亮浅色矢量星（1颗星、2颗星、3颗星、4颗星、5颗星），**绝对不绘制未点亮的灰色背景星**；
     - 设置足够的 `iconSize`（如 100×18），确保星星饱满清晰、间距均匀舒展。
2. **日期选择器日历弹窗宽度不足**：
   - 规则行中的日期选择框（`SingleDateValueWidget` 与 `DateRangeValueWidget`）由于宽度较窄，其内部原生的 `QCalendarWidget` 弹窗宽度被强制压缩，导致顶部的星期表头（一至日）被截断为 `...`，破坏了整体软件的界面质感。

### 1.2 改造目标
1. 规范 `SmartFolderRuleRow` 中规则类型“评分”更名为“星级”；
2. 彻底重构 `StarRatingValueWidget`：
   - 显式设置 `m_combo->setIconSize(QSize(100, 18))`；
   - 移除“无”选项，仅提供 1 到 5 星纯矢量浅色星（颜色采用 `#C5D1DE` / `#EEEEEE`），不绘制任何灰星；
   - 选项文字清空为空字符串 `""`；
3. 为 `SingleDateValueWidget` 和 `DateRangeValueWidget` 的 `calendarWidget()` 设置最小宽度（`260px`），并在 `style.qss` 中补充暗黑主题日历样式。

---

## Chapter 2: Modified Files List（涉及修改文件清单）

1. `QuarkMeta_Source/src/ui/dialogs/SmartFolderRuleRow.cpp`
2. `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`
3. `QuarkMeta_Source/resources/style.qss`

---

## Chapter 3: Detailed Line-by-Line Changes（逐行代码修改方案）

### 3.1 `QuarkMeta_Source/src/ui/dialogs/SmartFolderRuleRow.cpp`
<<<<<<< SEARCH
    m_typeCombo = new QComboBox(this);
    m_typeCombo->setObjectName("SmartFolderComboBox");
    m_typeCombo->addItem("评分", static_cast<int>(SmartRuleType::Rating));
=======
    m_typeCombo = new QComboBox(this);
    m_typeCombo->setObjectName("SmartFolderComboBox");
    m_typeCombo->addItem("星级", static_cast<int>(SmartRuleType::Rating));
>>>>>>> REPLACE

---

### 3.2 `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`
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
    m_combo->setIconSize(QSize(100, 18));

    const int starSize = 14;
    const int spacing = 4;
    for (int stars = 1; stars <= 5; ++stars) {
        int maxW = stars * starSize + (stars - 1) * spacing;
        QPixmap pixmap(maxW, starSize);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        int x = 0;
        for (int i = 0; i < stars; ++i) {
            QIcon icon = UiHelper::getIcon("star_filled", QColor("#C5D1DE"));
            icon.paint(&painter, x, 0, starSize, starSize);
            x += starSize + spacing;
        }

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

### 3.3 `QuarkMeta_Source/resources/style.qss`
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
1. **星级下拉框图二效果验收**：
   - 打开“智能文件夹规则配置”对话框；
   - 第一列规则类型显示为“**星级**”；
   - 点击第三列的星级下拉列表框；
   - **预期现象**：
     - 下拉列表中无“无评级”项，直接显示 1 星、2 星、3 星、4 星、5 星；
     - 每一项仅显示相应数量的纯白/浅色矢量实心星（`#C5D1DE`），无未激活灰色背景星，且完全消除“1星”等阿拉伯数字文字；
     - 星星饱满清晰，绝无图一中被压缩为微小黑点的畸形现象，与图二效果 100% 物理对齐。
2. **日历弹出框宽度舒展验收**：
   - 添加或查看一条日期规则；
   - 点击日期输入框日历按钮，日历宽度维持 260px，星期表头完整展示，无 `...` 截断。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）
1. 矢量图标复用：复用 `UiHelper::getIcon("star_filled", ...)`。
2. 尺寸控制：直接复用 Qt 原生公有 API `QComboBox::setIconSize` 与 `QDateEdit::calendarWidget()->setMinimumWidth(260)`。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 / 控件 | 唯一 SSOT 数据源 | 派生副本 / 观察者 | 同步与刷新机制 |
| :--- | :--- | :--- | :--- |
| 星级规则值 | `SmartFolderRule::value` / `num1` (整型 1~5) | `StarRatingValueWidget::m_combo` | 选定后通过 `getRuleValues` 序列化保存 |
| 日期规则值 | `SmartFolderRule::date1` / `date2` (`yyyy-MM-dd`) | `SingleDateValueWidget` / `DateRangeValueWidget` | `QDateEdit::dateChanged` 触发规则同步 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）
本方案零修改头文件签名，所有公有 `.h` 接口保持冻结只读。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）
在 `SmartFolderValueWidgets.cpp` 显式包含 `<QCalendarWidget>`，依赖完整闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 状态 | 说明 |
| :---: | :--- | :---: | :--- |
| 1 | 方案是否物理隔离在 `Implementation Plan/` 目录下 | 满足 | 放置于指定目录 |
| 2 | 是否使用纯中文进行阐述与说明 | 满足 | 全文使用中文 |
| 3 | 是否包含全部 10 大必备章节规范 | 满足 | Chapter 1 至 10 完整包含 |
| 4 | SEARCH 块是否在目标文件中物理存在且唯一匹配 | 满足 | 经过自动化脚本物理校验通过 |
| 5 | 对外公开 `.h` 接口签名是否处于只读冻结状态 | 满足 | 保持完全只读 |
| 6 | 是否消除图一压缩与文字缺陷并达成图二效果 | 满足 | 增加 setIconSize 并精炼矢量星星 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）
| 既有功能 / 交互模式 | 方案处理方式 | 行为变化说明 |
| :--- | :--- | :--- |
| 星级规则过滤 | 原封不动保留 | 选中后 `getRuleValues` / `setRuleValues` 逻辑不变 |
| 日期选择与数据过滤 | 原封不动保留 | 机制完全不变 |
| 星级展示效果 | 按照图二精准升级 | 修复压缩问题，移除冗余背景星，与图二效果完全一致 |
