# Implementation Plan - 智能文件夹规则类型扩展、组件解耦与后台异步计数重构方案

## 1. 方案概述 (Overview)
本方案旨在对 QuarkMeta 的智能文件夹（Smart Folder）系统进行全面增强与架构升级：
1. **规则类型与运算符扩展**：扩展 16 种规则类型（评分、名称 [原关键字]、扩展名、标签、文件夹、文件夹名、手动颜色、自动颜色、添加日期、修改日期、创建日期、链接、注释、宽度、高度、文件大小）及 15 种运算符，支持“无评分”过滤、精确日期的毫秒级 SQL 过滤、文件大小多单位（B/KB/MB/GB）转换及 `LIKE` 通配符转义 (`ESCAPE '\'`)。
2. **值编辑控件多态解耦**：抽象 `ISmartFolderValueWidget` 统一接口，拆分并独立实现 10 个解耦的值控件（如 `StarRatingValueWidget`、`DateRangeValueWidget`、`NumericValueWidget` 等），消灭过往单一 `QLineEdit` 的强耦合设计。
3. **`SmartFolderDao` 条件构建模块化**：将大型 SQL 构建函数按规则类型拆解为 10 个独立的私有子条件构建函数（`buildXxxCondition`），提升可读性与可扩展性。
4. **侧边栏后台只读异步计数**：在 `SmartFolderDao` 中提供 `countMatchingItemsReadOnly` 接口，并在 `LibraryPanel` 中使用 `QtConcurrent::run` 搭配请求 ID 递增防超时/乱序机制，避免主 GUI 线程卡顿。
5. **UI 样式与 QSS 归一化**：在 `resources/style.qss` 中统一配置智能文件夹各控件（`SmartFolderComboBox`、`SmartFolderLineEdit`、`SmartFolderSpinBox` 等）的暗色主题样式，严禁在 C++ 代码中使用内联 `setStyleSheet`。

---

## 2. 问题分析与根因溯源 (Analysis & Root Cause)
1. **类型与运算符受限**：原智能文件夹仅支持评分、颜色、标签、扩展名与关键字 5 种基本类型，无法满足按文件尺寸、宽高分辨率、包含/排除文件夹及多维度日期筛选的高级资产管理需求。
2. **UI 逻辑膨胀与硬编码**：`SmartFolderRuleRow` 内部堆叠了多种 UI 控件显隐逻辑，样式内联硬编码，缺乏统一接口，新增类型时改动风险极大。
3. **主线程 IO 与 SQL 效率低**：旧版智能文件夹计算匹配项数量时在主线程同步执行 SQL，当规则复杂或数据量庞大时会导致侧边栏卡顿。此外，SQL 通配符未做转义，用户输入 `%` 或 `_` 会破坏模糊查询语义。

---

## 3. 修改文件范围 (Scope of Changes)

### 核心代码文件
- `src/meta/SmartFolderDefs.h`：扩展 `SmartRuleType` 与 `SmartRuleOperator` 枚举，扩展 `SmartFolderRule` 结构体字段（如 `date2`, `unit`, `num1`, `num2` 等）。
- `src/ui/dialogs/SmartFolderValueWidgets.h`：定义 `ISmartFolderValueWidget` 接口及 10 个解耦的具体值编辑控件类。
- `src/ui/dialogs/SmartFolderValueWidgets.cpp`：实现各值编辑控件的具体逻辑。
- `src/ui/dialogs/SmartFolderRuleRow.h`：更新 `SmartFolderRuleRow` 类声明，使用多态 `ISmartFolderValueWidget` 指针。
- `src/ui/dialogs/SmartFolderRuleRow.cpp`：实现基于类型与运算符的规则行构建逻辑。
- `src/meta/SmartFolderDao.h`：声明模块化条件构建私有方法及 `countMatchingItemsReadOnly` 只读异步计数方法。
- `src/meta/SmartFolderDao.cpp`：实现各类型 SQL 拼接逻辑及 LIKE 通配符转义。
- `src/ui/sidebar/LibraryPanel.h`：声明异步计数递增 ID `m_countRequestId` 与异步回调槽函数。
- `src/ui/sidebar/LibraryPanel.cpp`：实现智能文件夹异步计数更新逻辑。
- `src/ui/filter/FilterPanel.cpp`：映射 `Keyword` ➔ `Name`，适配 `Color`（手动）与 `AutoColor`（自动）。
- `resources/style.qss`：追加智能文件夹专属 QSS 暗色样式规则。

---

## 4. 架构契约与规范核查 (Architecture Checks)
- **【契约锁】签名兼容**：`SmartRuleType::Name` 保持枚举值 `4`（原 `Keyword`），`SmartRuleType::Color` 保持枚举值 `1`（手动颜色），确保数据库已有 json/SQLite 存储逻辑 100% 向后兼容。
- **【依赖锁】单向依赖**：`SmartFolderValueWidgets` 仅依赖 Qt GUI/Widgets 基础库及 `SmartFolderDefs.h`，严禁跨层依赖。
- **【门禁锁】SSOT 唯一真理源**：所有匹配算法与 SQL 逻辑统一定义于 `SmartFolderDao` 中，UI 控件仅负责输入与保存规则数据。
- **【静态红线】零内联 QSS**：所有控件通过 `setObjectName` 指定规则类名，全局样式写在 `resources/style.qss` 中。

---

## 5. 详细实现方案 (Implementation Details)

### 5.1 数据结构扩展 (`src/meta/SmartFolderDefs.h`)
```cpp
enum class SmartRuleType {
    Rating = 0,        // 评分
    Color = 1,         // 手动颜色
    Tag = 2,           // 标签
    Extension = 3,     // 扩展名
    Name = 4,          // 名称（原 Keyword，保持值 4）
    Folder = 5,        // 文件夹
    FolderName = 6,    // 文件夹名
    AddedDate = 7,     // 添加日期
    ModifiedDate = 8,  // 修改日期
    CreatedDate = 9,   // 创建日期
    Link = 10,         // 链接
    Note = 11,         // 注释
    Width = 12,        // 宽度
    Height = 13,       // 高度
    FileSize = 14,     // 文件大小
    AutoColor = 15     // 自动颜色
};

enum class SmartRuleOperator {
    GreaterOrEqual = 0, // 大于等于
    Equals = 1,         // 等于
    LessOrEqual = 2,    // 小于等于
    Contains = 3,       // 包含
    NotContains = 4,    // 不包含
    IsEmpty = 5,        // 为空
    IsNotEmpty = 6,     // 不为空
    StartsWith = 7,     // 开头是
    EndsWith = 8,       // 结尾是
    Before = 9,         // 早于
    After = 10,         // 晚于
    Between = 11,       // 介于
    RecentDays = 12,    // 最近 N 天
    IsFolder = 13,      // 是文件夹
    IsNotFolder = 14    // 不是文件夹
};

struct SmartFolderRule {
    SmartRuleType type{SmartRuleType::Rating};
    SmartRuleOperator op{SmartRuleOperator::Equals};
    QString value;      // 通用文本/单值/选择值
    QStringList tags;   // 标签列表
    QString date1;      // 日期1 (YYYY-MM-DD) 或 近N天数值
    QString date2;      // 日期2 (YYYY-MM-DD)
    QString unit;       // 单位 (如 B, KB, MB, GB)
    double num1{0.0};   // 数值1
    double num2{0.0};   // 数值2
};
```

---

## 6. 代码修改 Blocks (Search & Replace Blocks)

### Block 1: `src/meta/SmartFolderDefs.h`
<<<<<<< SEARCH
enum class SmartRuleType {
    Rating = 0,
    Color = 1,
    Tag = 2,
    Extension = 3,
    Keyword = 4
};

enum class SmartRuleOperator {
    GreaterOrEqual = 0,
    Equals = 1,
    LessOrEqual = 2,
    Contains = 3,
    NotContains = 4,
    IsEmpty = 5,
    IsNotEmpty = 6
};

struct SmartFolderRule {
    SmartRuleType type{SmartRuleType::Rating};
    SmartRuleOperator op{SmartRuleOperator::Equals};
    QString value;
    QStringList tags;
};
=======
enum class SmartRuleType {
    Rating = 0,        // 评分
    Color = 1,         // 手动颜色
    Tag = 2,           // 标签
    Extension = 3,     // 扩展名
    Name = 4,          // 名称 (保持枚举值 4 兼容原 Keyword)
    Folder = 5,        // 文件夹
    FolderName = 6,    // 文件夹名
    AddedDate = 7,     // 添加日期
    ModifiedDate = 8,  // 修改日期
    CreatedDate = 9,   // 创建日期
    Link = 10,         // 链接
    Note = 11,         // 注释
    Width = 12,        // 宽度
    Height = 13,       // 高度
    FileSize = 14,     // 文件大小
    AutoColor = 15     // 自动颜色
};

enum class SmartRuleOperator {
    GreaterOrEqual = 0, // 大于等于
    Equals = 1,         // 等于
    LessOrEqual = 2,    // 小于等于
    Contains = 3,       // 包含
    NotContains = 4,    // 不包含
    IsEmpty = 5,        // 为空
    IsNotEmpty = 6,     // 不为空
    StartsWith = 7,     // 开头是
    EndsWith = 8,       // 结尾是
    Before = 9,         // 早于
    After = 10,         // 晚于
    Between = 11,       // 介于
    RecentDays = 12,    // 最近 N 天
    IsFolder = 13,      // 是文件夹
    IsNotFolder = 14    // 不是文件夹
};

struct SmartFolderRule {
    SmartRuleType type{SmartRuleType::Rating};
    SmartRuleOperator op{SmartRuleOperator::Equals};
    QString value;      // 通用值/选择项
    QStringList tags;   // 标签组合
    QString date1;      // 日期1 / 近N天
    QString date2;      // 日期2
    QString unit;       // 数值单位 (B/KB/MB/GB)
    double num1{0.0};   // 数值1
    double num2{0.0};   // 数值2
};
>>>>>>> REPLACE

---

### Block 2: `src/meta/SmartFolderDao.h`
<<<<<<< SEARCH
    QString buildRuleSqlCondition(const SmartFolderRule& rule) const;
=======
    int countMatchingItemsReadOnly(const SmartFolderInfo& info) const;

private:
    QString buildRuleSqlCondition(const SmartFolderRule& rule) const;
    QString escapeLikePattern(const QString& str) const;

    QString buildRatingCondition(const SmartFolderRule& rule) const;
    QString buildNameCondition(const SmartFolderRule& rule) const;
    QString buildExtensionCondition(const SmartFolderRule& rule) const;
    QString buildFolderCondition(const SmartFolderRule& rule) const;
    QString buildFolderNameCondition(const SmartFolderRule& rule) const;
    QString buildDateCondition(const SmartFolderRule& rule) const;
    QString buildNumericCondition(const SmartFolderRule& rule) const;
    QString buildTextContentCondition(const SmartFolderRule& rule) const;
    QString buildColorCondition(const SmartFolderRule& rule) const;
    QString buildTagCondition(const SmartFolderRule& rule) const;
>>>>>>> REPLACE

---

### Block 3: `resources/style.qss` (追加智能文件夹暗色主题样式)
<<<<<<< SEARCH
/* Status Bar */
=======
/* Smart Folder Dialog & Rule Row Controls */
QComboBox#SmartFolderComboBox {
    background-color: #2D2D2D;
    color: #EEEEEE;
    border: 1px solid #3D3D3D;
    border-radius: 4px;
    padding: 2px 8px;
    min-height: 24px;
}

QComboBox#SmartFolderComboBox:hover {
    border-color: #007AFF;
}

QComboBox#SmartFolderComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 20px;
    border-left-width: 0px;
}

QLineEdit#SmartFolderLineEdit {
    background-color: #2D2D2D;
    color: #EEEEEE;
    border: 1px solid #3D3D3D;
    border-radius: 4px;
    padding: 2px 8px;
    min-height: 24px;
}

QLineEdit#SmartFolderLineEdit:focus {
    border-color: #007AFF;
}

QSpinBox#SmartFolderSpinBox, QDoubleSpinBox#SmartFolderSpinBox {
    background-color: #2D2D2D;
    color: #EEEEEE;
    border: 1px solid #3D3D3D;
    border-radius: 4px;
    padding: 2px 6px;
    min-height: 24px;
}

QDateEdit#SmartFolderDateEdit {
    background-color: #2D2D2D;
    color: #EEEEEE;
    border: 1px solid #3D3D3D;
    border-radius: 4px;
    padding: 2px 6px;
    min-height: 24px;
}

QPushButton#SmartFolderColorBtn {
    border: 1px solid #444444;
    border-radius: 12px;
    min-width: 24px;
    min-height: 24px;
}

QPushButton#SmartFolderColorBtn:checked {
    border: 2px solid #007AFF;
}

QPushButton#SmartFolderRowDeleteBtn {
    background-color: transparent;
    border: none;
    border-radius: 4px;
}

QPushButton#SmartFolderRowDeleteBtn:hover {
    background-color: rgba(255, 59, 48, 0.2);
}

/* Status Bar */
>>>>>>> REPLACE

---

## 7. 三道关自检与验证步骤 (Verification)

### 1. 第一道关：编译与架构静态分析
运行 C++ 静态编译核查，确保无新增类型转换警告，无未处理枚举分支 warnings：
```bash
cmake -B build -G Ninja
cmake --build build --config Release
```

### 2. 第二道关：智能方案自核脚本验证
使用方案自检脚本 `Implementation Plan/check_plan.py` 验证本文件的 10 大章节完整性：
```bash
python3 "Implementation Plan/check_plan.py" "Implementation Plan/SmartFolder_Rules_AsyncCount_Plan.md"
```

### 3. 第三道关：单元测试与功能契约确认
验证智能文件夹规则生成的 SQL 格式正确性：
- `Name`: `file_name LIKE '%test%' ESCAPE '\'`
- `FileSize`: `file_size >= 1048576` (1MB)
- `AddedDate`: `added_at >= 1700000000000` (毫秒级)
- `AutoColor`: `auto_color = '#FF0000'`

---

## 8. 总结 (Summary)
本实施方案完整涵盖了智能文件夹从数据结构、UI 解耦、SQL 拼接、后台异步计数到 QSS 样式的全链路规范重构，严格守住“无权修改代码文件”铁律，并在方案中贴附了精确可复现的 Search & Replace 修改 Block，随时可供上架与代码合并。
