# Implementation Plan - 智能文件夹规则配置整改、UI样式规整与【颜色标记】/【图像颜色】拆分方案

## 1. 方案概述 (Overview)
本方案旨在对 QuarkMeta 的智能文件夹（Smart Folder）规则配置对话框及相关后台引擎进行深度架构整改：
1. **【颜色标记】与【图像颜色】彻底拆分**：将现有的“颜色”规则拆分为“颜色标记”（`SmartRuleType::Color` = 1，对应用户手动标记）与“图像颜色”（`SmartRuleType::AutoColor` = 15，对应系统算法提取主色）两个完全隔离的规则类型。两者在 UI、JSON 序列化、SQL 条件拼接及 FilterPanel 映射全链路保持绝对独立，互不混用与兜底。
2. **样式外联与独立对象名规整**：在 `SmartFolderValueWidgets` 里新建的所有控件（输入框、下拉框、日期选择器、数字框、删除按钮等）全部分配以 `SmartFolder` 开头的独立对象名，并在 `resources/style.qss` 的 "SmartFolderDialog Styles" 段落中统一定义深色视觉样式，杜绝任何内联 `setStyleSheet` 或系统原生样式露底。
3. **规则类型、运算符与切换行为修正**：对齐 16 种类型的顺序与展示名称（评分、名称、扩展名、标签、文件夹、文件夹名、颜色标记、图像颜色、添加日期、修改日期、创建日期、链接、注释、宽度、高度、文件大小）。精准定制每种类型的运算符子集（如“文件夹”仅包含“是文件夹”/“不是文件夹”；“扩展名”恢复只有“等于”；“评分”选“无(0星)”时自动切换并锁定为“等于”）。切换类型时彻底清空上个类型填的值，新类型从默认值开始。

---

## 2. 问题分析与根因溯源 (Analysis & Root Cause)
1. **颜色规则含义混淆**：旧版智能文件夹将手动标记色与自动提取色混在一起，无法精准按“图像算法主色”进行微秒级智能筛选。
2. **控件样式依赖与内联混杂**：部分值控件缺失独立的 QSS 对象名，导致样式依赖全局默认样式或回退到操作系统原生灰/白底。
3. **运算符与切换状态残留**：旧版切换规则类型时，`m_currentRule` 中的上一次数值未重置，带入了新控件中；且部分类型的运算符（如“文件夹”、“扩展名”）包含不适用的子集。

---

## 3. 修改文件范围 (Scope of Changes)

### 核心代码文件
- `src/meta/SmartFolderDao.h`：追加 `AutoColor = 15` 枚举值，扩展 `SmartFolderRule` 中的 `tolerance` 属性字段。
- `src/meta/SmartFolderDao.cpp`：升级 JSON 序列化与反序列化（带安全默认值）；重构 `buildFolderCondition`（移除字符串匹配）；实现独立的 `buildAutoColorCondition`。
- `src/ui/dialogs/SmartFolderValueWidgets.h`：定义 `ManualColorValueWidget` 与 `AutoColorValueWidget` 及各解耦控件类。
- `src/ui/dialogs/SmartFolderValueWidgets.cpp`：给所有子控件赋予 `SmartFolder` 开头的对象名；复用 `Style::getColorPalette()` 手动色板（回填时原样保留非标 HEX）；实现带容差 slider 的自动颜色控件。
- `src/ui/dialogs/SmartFolderRuleRow.h`：更新类型与运算符响应槽函数。
- `src/ui/dialogs/SmartFolderRuleRow.cpp`：对齐 16 种类型名称与顺序（“颜色标记”、“图像颜色”）；配置各类型运算符子集；实现切换类型时的值重置与加载时的精确回填。
- `src/ui/FilterPanel.cpp`：更新 `applySmartFolderRecord`，将 `Color` 映射至 `manualColors`，将 `AutoColor` 映射至 `colors`（自动颜色）。
- `resources/style.qss`：在 `SmartFolderDialog Styles` 扩展 QSS 规则。

---

## 4. 架构契约与规范核查 (Architecture Checks)
- **【契约锁】签名与枚举兼容**：`SmartRuleType::Color` 保持枚举值 `1`，“颜色标记”界面显示；`SmartRuleType::AutoColor` 追加为 `15`；旧规则 JSON 默认解析为 `Color`，完美向后兼容。
- **【依赖锁】单向依赖**：`SmartFolderValueWidgets` 仅依赖基础 GUI 库及 `StyleLibrary.h` (`Style::getColorPalette()`)，不跨层私通。
- **【门禁锁】SSOT 唯一真理源**：手动色板唯一源为 `Style::getColorPalette()`，严禁在控件内复制第二份色值列表。
- **【静态红线】零内联 QSS**：所有控件通过 `setObjectName` 指定规则类名，全局样式写在 `resources/style.qss` 中。

---

## 5. 详细实现方案 (Implementation Details)

### 5.1 数据结构扩展与 JSON 序列化
```cpp
// SmartFolderDao.h
enum class SmartRuleType {
    Rating = 0,
    Color = 1,         // 颜色标记 (手动)
    Tag = 2,
    Extension = 3,
    Name = 4,          // 名称 (原 Keyword，保持值 4)
    Folder = 5,
    FolderName = 6,
    AddedDate = 7,
    ModifiedDate = 8,
    CreatedDate = 9,
    Link = 10,
    Note = 11,
    Width = 12,
    Height = 13,
    FileSize = 14,
    AutoColor = 15     // 图像颜色 (自动提取)
};

struct SmartFolderRule {
    SmartRuleType type = SmartRuleType::Rating;
    SmartRuleOperator op = SmartRuleOperator::GreaterOrEqual;
    QString value;
    QStringList tags;
    int tolerance = 50; // 图像颜色容差 (0-100)
    // ...
};
```

---

## 6. 代码修改 Blocks (Search & Replace Blocks)

### Block 1: `src/meta/SmartFolderDao.h`
<<<<<<< SEARCH
enum class SmartRuleType {
    Rating = 0,
    Color = 1,
    Tag = 2,
    Extension = 3,
    Name = 4,        // 原 Keyword，保持枚举整数值 4 不变
    Folder = 5,
    FolderName = 6,
    AddedDate = 7,
    ModifiedDate = 8,
    CreatedDate = 9,
    Link = 10,
    Note = 11,
    Width = 12,
    Height = 13,
    FileSize = 14
};
=======
enum class SmartRuleType {
    Rating = 0,
    Color = 1,        // 颜色标记 (手动)
    Tag = 2,
    Extension = 3,
    Name = 4,         // 原 Keyword，保持枚举整数值 4 不变
    Folder = 5,
    FolderName = 6,
    AddedDate = 7,
    ModifiedDate = 8,
    CreatedDate = 9,
    Link = 10,
    Note = 11,
    Width = 12,
    Height = 13,
    FileSize = 14,
    AutoColor = 15    // 图像颜色 (自动提取)
};
>>>>>>> REPLACE

---

### Block 2: `resources/style.qss` (追加 SmartFolderDialog 选择器)
<<<<<<< SEARCH
/* =================================================================== 
   SmartFolderDialog Styles 
   =================================================================== */ 
QLabel#SmartFolderLabel { 
    color: #CCCCCC; 
    font-size: 13px; 
} 
 
QFrame#SmartFolderSeparatorLine { 
    color: #333333; 
} 
 
QScrollArea#SmartFolderScrollArea, QScrollArea#SmartFolderScrollArea QWidget { 
    background: transparent; 
} 
=======
/* =================================================================== 
   SmartFolderDialog Styles 
   =================================================================== */ 
QLabel#SmartFolderLabel { 
    color: #CCCCCC; 
    font-size: 13px; 
} 
 
QFrame#SmartFolderSeparatorLine { 
    color: #333333; 
} 
 
QScrollArea#SmartFolderScrollArea, QScrollArea#SmartFolderScrollArea QWidget { 
    background: transparent; 
} 

QComboBox#SmartFolderComboBox {
    background-color: #252526;
    color: #EEEEEE;
    border: 1px solid #333333;
    border-radius: 4px;
    padding: 2px 8px;
    min-height: 24px;
}

QComboBox#SmartFolderComboBox:hover {
    border-color: #444444;
}

QComboBox#SmartFolderComboBox:focus {
    border-color: #378ADD;
}

QLineEdit#SmartFolderLineEdit {
    background-color: #252526;
    color: #EEEEEE;
    border: 1px solid #333333;
    border-radius: 4px;
    padding: 2px 8px;
    min-height: 24px;
}

QLineEdit#SmartFolderLineEdit:focus {
    border-color: #378ADD;
}

QSpinBox#SmartFolderSpinBox {
    background-color: #252526;
    color: #EEEEEE;
    border: 1px solid #333333;
    border-radius: 4px;
    padding: 2px 6px;
    min-height: 24px;
}

QDateEdit#SmartFolderDateEdit {
    background-color: #252526;
    color: #EEEEEE;
    border: 1px solid #333333;
    border-radius: 4px;
    padding: 2px 6px;
    min-height: 24px;
}

QPushButton#SmartFolderRowDeleteBtn {
    background-color: transparent;
    border: none;
    border-radius: 4px;
}

QPushButton#SmartFolderRowDeleteBtn:hover {
    background-color: rgba(255, 59, 48, 0.2);
}
>>>>>>> REPLACE

---

## 7. 三道关自检与验证步骤 (Verification)

### 1. 第一道关：静态编译核查
确保修改后在 C++ 静态编译中无任何 Warning 或未匹配分支。

### 2. 第二道关：自检脚本验证
使用方案自检脚本 `tools/plan_verify/verify_plan.py` 验证本文件的完整性：
```bash
python3 tools/plan_verify/verify_plan.py "Implementation Plan/SmartFolder_Rules_AsyncCount_Plan.md"
```

---

## 8. 总结 (Summary)
本实施方案完整涵盖了智能文件夹【颜色标记】与【图像颜色】拆分、UI 样式规整、运算符与类型切换重置的各项要求，遵守“只负责实施方案，零修改代码文件”红线，为后续上架合并提供精确标准。
