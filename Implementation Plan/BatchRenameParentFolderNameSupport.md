# Implementation Plan - BatchRenameParentFolderNameSupport

Base Commit Hash: `943037f85524b65875859191f2d3c1dba8aae1c1`
Prerequisite Plans: `Implementation Plan/BatchRenameLayoutAndPresetRefactor.md`

---

### 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/core/BatchRenameService.h`
   - `src/core/BatchRenameService.cpp`
   - `src/ui/RuleRow.h`
   - `src/ui/RuleRow.cpp`
2. **头文件原样签名**：
   - `src/core/BatchRenameService.h:13`: `enum class RenameComponentType`
   - `src/ui/RuleRow.h:13`: `class RuleRow : public QWidget`
3. **既有 API 检索日志**：
   - `grep -rn "RenameComponentType" src/`

---

## Chapter 1: Overview
在批量重命名规则构造组件（`RuleRow`）中，当前下拉框选项仅包含“文本”、“序列数字”、“原文件名”、“日期”四项。为了满足用户按照文件所属上层目录自动包含文件夹名称的重命名需求，本实施方案补充了 **“文件夹名称”**（`ParentFolderName`）规则选项支持：

1. 在 `RenameComponentType` 枚举中新增 `ParentFolderName`；
2. 在 `BatchRenameService::processOne` 计算预览与执行重命名时，解析文件的上层目录名 `QFileInfo(path).dir().dirName()`；
3. 在 `RuleRow` 下拉框中增加 `"文件夹名称"` 选项，选中时参数堆叠区展示空占位面板（与“原文件名”一致无需额外参数输入）。

---

## Chapter 2: Modified Files List
- `src/core/BatchRenameService.h`
- `src/core/BatchRenameService.cpp`
- `src/ui/RuleRow.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes

### 1. `src/core/BatchRenameService.h`
<<<<<<< SEARCH
enum class RenameComponentType {
    Text,           // 固定文本
    Sequence,       // 序列数字
    Date,           // 日期
    OriginalName,   // 原始文件名
    Metadata        // 元数据标记
};
=======
enum class RenameComponentType {
    Text,           // 固定文本
    Sequence,       // 序列数字
    Date,           // 日期
    OriginalName,   // 原始文件名
    Metadata,       // 元数据标记
    ParentFolderName // 父文件夹名称
};
>>>>>>> REPLACE

---

### 2. `src/core/BatchRenameService.cpp`
<<<<<<< SEARCH
            case RenameComponentType::OriginalName:
                newName += info.baseName();
                break;
            case RenameComponentType::Metadata:
                newName += "[QuarkMeta]";
                break;
        }
=======
            case RenameComponentType::OriginalName:
                newName += info.baseName();
                break;
            case RenameComponentType::Metadata:
                newName += "[QuarkMeta]";
                break;
            case RenameComponentType::ParentFolderName:
                newName += info.dir().dirName();
                break;
        }
>>>>>>> REPLACE

---

### 3. `src/ui/RuleRow.cpp`
<<<<<<< SEARCH
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("文本", static_cast<int>(RenameComponentType::Text));
    m_typeCombo->addItem("序列数字", static_cast<int>(RenameComponentType::Sequence));
    m_typeCombo->addItem("原文件名", static_cast<int>(RenameComponentType::OriginalName));
    m_typeCombo->addItem("日期", static_cast<int>(RenameComponentType::Date));
    m_typeCombo->setFixedWidth(100);
=======
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("文本", static_cast<int>(RenameComponentType::Text));
    m_typeCombo->addItem("序列数字", static_cast<int>(RenameComponentType::Sequence));
    m_typeCombo->addItem("原文件名", static_cast<int>(RenameComponentType::OriginalName));
    m_typeCombo->addItem("日期", static_cast<int>(RenameComponentType::Date));
    m_typeCombo->addItem("文件夹名称", static_cast<int>(RenameComponentType::ParentFolderName));
    m_typeCombo->setFixedWidth(100);
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 3. OriginalName Param (Empty)
    QWidget* emptyW = new QWidget(this);
    m_paramStack->addWidget(emptyW);

    // 4. Date Param
    m_dateFormatCombo = new QComboBox(this);
    m_dateFormatCombo->addItems({"yyyyMMdd", "yyyy-MM-dd", "yyyy_MM_dd", "yyyy", "MM", "dd"});
    m_dateFormatCombo->setEditable(true);
    m_dateFormatCombo->setFixedHeight(25);
    m_dateFormatCombo->setObjectName("RuleCombo");
    m_paramStack->addWidget(m_dateFormatCombo);
=======
    // 3. OriginalName Param (Empty)
    QWidget* emptyW = new QWidget(this);
    m_paramStack->addWidget(emptyW);

    // 4. Date Param
    m_dateFormatCombo = new QComboBox(this);
    m_dateFormatCombo->addItems({"yyyyMMdd", "yyyy-MM-dd", "yyyy_MM_dd", "yyyy", "MM", "dd"});
    m_dateFormatCombo->setEditable(true);
    m_dateFormatCombo->setFixedHeight(25);
    m_dateFormatCombo->setObjectName("RuleCombo");
    m_paramStack->addWidget(m_dateFormatCombo);

    // 5. ParentFolderName Param (Empty)
    QWidget* emptyW2 = new QWidget(this);
    m_paramStack->addWidget(emptyW2);
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps
1. **构建步骤**：
   在 sandbox 根目录运行 `cmake --build build` 编译项目。
2. **运行时与 UI 效果核验**：
   - 打开“批量重命名”窗口，在规则构造行的组件类型下拉框中展开，确认出现 **“文件夹名称”** 选项；
   - 选择“文件夹名称”，右侧预览表格实时更新展示对应文件所属父目录名称；
   - 组合测试：“文本”+“文件夹名称”+“序列数字”，确认计算 preview 与实际重命名命名正确。

---

## Chapter 5: Official API Reuse & Anti-Redundancy
- **QFileInfo::dir()**：直接使用 Qt 原生 API `QFileInfo(path).dir().dirName()` 获取上层文件夹名称，无冗余路径计算代码；
- **RuleRow 堆叠映射机制**：复用 `m_typeCombo->itemData(i)` 动态绑定 `RenameComponentType` 枚举与 `m_paramStack` 索引，实现低耦合扩展。

---

## Chapter 6: Header API Signature Verification
- `src/core/BatchRenameService.h` 的 `RenameComponentType` 枚举尾部追加 `ParentFolderName`；
- 所有 `BatchRenameService` 与 `RuleRow` 公开 API 保持 100% 相同。

---

## Chapter 7: Header Inclusion Chain & Type Completeness Check
- `src/core/BatchRenameService.cpp` 中包含 `<QFileInfo>` 和 `<QDir>`，支持 `dir().dirName()` 成员函数的无缝调用。

---

## Chapter 8: Data SSOT Declaration Table
| 数据项 | 唯一 SSOT 源 | 派生/缓存副本 | 同步方向 |
| :--- | :--- | :--- | :--- |
| 重命名规则组件类型 | `RenameComponentType` 枚举 | `RuleRow` 下拉框项与 JSON 预设 | `RenameComponentType` <-> `RuleRow` <-> `BatchRenameService` |

---

## Chapter 9: Compliance & Quality Self-Check Table

| 序号 | 检查项目 | 结果 | 位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否只修改实施方案 | 满足 | 仅创建 `Implementation Plan/BatchRenameParentFolderNameSupport.md` |
| 2 | SEARCH 块是否物理匹配且唯一 | 满足 | 精确匹配对应的 `.h` 与 `.cpp` |
| 3 | 是否破坏冻结的 .h API | 满足 | 枚举尾部追加，零破坏 |
| 4 | 是否包含 CMake 文件注册变更 | 不涉及 | 未新建 C++ 文件 |
| 5 | 是否遵循单向依赖流 | 满足 | UI -> DomainService |
| 6 | 是否包含完整 10 大章节 | 满足 | 包含 Chapter 1 至 Chapter 10 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table
- **现有选项计算**：原“文本”、“序列数字”、“原文件名”、“日期”计算规则保持 100% 不变。
