# Implementation Plan: SmartFolderRatingStyleFix.md

## 1. Overview（概述与解决的问题）
本方案旨在更正 `SmartFolderDialog` 智能文件夹规则配置窗口中星级下拉框的表达形式。
根据 `Memories.md` 第 0 节最高规范【星级表达与真理源契约 (Rating Display SSOT Contract)】：
> 全软件所有涉及星级评级的 UI 展示、下拉选择框、筛选菜单与规则编辑器，**绝对禁止使用“1星”、“2星”、“3星”等阿拉伯数字与文字组合的硬编码文本**！必须统一使用实心五角星符号 `★` 的重复形式（`★`、`★★`、`★★★`、`★★★★`、`★★★★★`），与全软件星级展示 SSOT 通道保持 100% 绝对物理一致。

---

## 2. Modified Files List（影响文件清单）
1. `src/ui/dialogs/SmartFolderDialog.cpp`

---

## 3. Detailed Line-by-Line Changes（包含精准替换块）

### `src/ui/dialogs/SmartFolderDialog.cpp`

```
<<<<<<< SEARCH
        m_valueEdit->hide();
        m_valueCombo->clear();
        m_valueCombo->addItem("1 星", "1");
        m_valueCombo->addItem("2 星", "2");
        m_valueCombo->addItem("3 星", "3");
        m_valueCombo->addItem("4 星", "4");
        m_valueCombo->addItem("5 星", "5");
        m_valueCombo->show();
=======
        m_valueEdit->hide();
        m_valueCombo->clear();
        m_valueCombo->addItem("★", "1");
        m_valueCombo->addItem("★★", "2");
        m_valueCombo->addItem("★★★", "3");
        m_valueCombo->addItem("★★★★", "4");
        m_valueCombo->addItem("★★★★★", "5");
        m_valueCombo->show();
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. **编译命令**：
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --parallel
   ```
2. **验证步骤**：
   - 运行程序，在“智能文件夹”标头右侧点击 `+` 按钮弹出“智能文件夹规则配置”对话框；
   - 规则类型选择“星级”；
   - 展开最右侧的值下拉选择框，验证下拉选项已由 `"1 星"~"5 星"` 成功更正为 `★`、`★★`、`★★★`、`★★★★`、`★★★★★` 五角星展现，与 `FilterPanel` 等全局星级呈现风格 100% 物理一致。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- 物理复用 `★` 字符与既有数值 `1`~`5` 的数据映射逻辑，与全局星级表达契约 100% 对齐。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）
| 类名 | 调用的 API | 头文件 | 验证状态 |
| :--- | :--- | :--- | :--- |
| `QComboBox` | `addItem(const QString& text, const QVariant& userData)` | `<QComboBox>` | 官方 API 精准存在 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）
| 修改文件 | 头文件 | 检查情况 |
| :--- | :--- | :--- |
| `src/ui/dialogs/SmartFolderDialog.cpp` | `#include "SmartFolderDialog.h"` | 包含完备 |
