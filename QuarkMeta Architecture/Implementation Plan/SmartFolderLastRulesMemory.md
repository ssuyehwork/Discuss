# SmartFolderLastRulesMemory.md - 智能文件夹新建规则模板记忆持久化实施方案

---

## 起草前核查（Pre-Draft Evidence）

### 1. 已读文件清单
- `src/ui/dialogs/SmartFolderDialog.h` / `.cpp`（已用 `read_file` 物理读取）
- `src/meta/SmartFolderDao.h` / `.cpp`（已用 `read_file` 物理读取）
- `src/core/AppConfig.h`（已用 `read_file` 物理读取）

### 2. 签名原文摘录
- `SmartFolderRule::toJson`:
  `QJsonObject toJson() const;`
- `SmartFolderRule::fromJson`:
  `static SmartFolderRule fromJson(const QJsonObject& obj);`
- `AppConfig::setValue`:
  `void setValue(const QString& key, const QVariant& value);`
- `AppConfig::getValue`:
  `QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;`

### 3. 既有 API 检索记录
- 配置持久化 SSOT：`AppConfig::instance().setValue(...)` / `getValue(...)` 检索存在。
- 规则 JSON 转换：`SmartFolderRule::toJson` / `fromJson` 检索存在。

### 4. Memories.md 相关条款
- 图标与 UI 规范：保留现有 `SmartFolderDialog` 样式与控件尺寸，零数值篡改。
- 数据与配置 SSOT：配置持久化必须走 `AppConfig::instance()` 入口。

---

## 1. Overview（概述与解决的问题）

### 1.1 核心动机
当前每次打开“智能文件夹规则配置”新建智能文件夹时，界面固定只弹出一条默认的“星级”规则。用户期望能够**记忆上一次保存的整套规则模板**，以便在下一次新建智能文件夹时自动加载呈现上一次配置好的整套规则与匹配模式（AND/OR）。

### 1.2 解决实施方案
1. **规则模板保存**：
   - 在 `SmartFolderDialog::onSaveAndAccept()` 中，当用户点击“保存”时，将当前界面配置的匹配模式（`m_record.matchMode`）以及全部规则列表（`m_record.rules` 转换为 `QJsonArray` 字符串）保存至 `AppConfig::instance()` 的全应用键名 `"SmartFolder/LastMatchMode"` 和 `"SmartFolder/LastTemplateRules"` 中。
2. **新建智能文件夹时的模板加载与恢复**：
   - 在 `SmartFolderDialog::setupUi()` 中，判断是否为新建智能文件夹（即 `m_record.rules.isEmpty()`）。
   - 若为新建且 `AppConfig` 中存在上一次保存的规则模板：
     - 读取并恢复 `matchMode`；
     - 反序列化 `LastTemplateRules` 对应的 `SmartFolderRule` 列表并自动填入界面 `m_ruleRows`；
   - 若不存在历史保存模板，则维持原有的默认添加一条“星级”规则作为保底逻辑。
3. **编辑已有智能文件夹的兼容**：
   - 编辑已有智能文件夹时，优先使用该记录自身的 `record.rules` 和 `record.matchMode`，完全不受影响。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/dialogs/SmartFolderDialog.cpp` (添加 `AppConfig` 头文件引用与模板记忆存取逻辑)

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/dialogs/SmartFolderDialog.cpp`
```cpp
<<<<<<< SEARCH
#include "SmartFolderDialog.h"
#include "../UiHelper.h"
#include <QScrollArea>
#include <QPainter>
#include <QPixmap>
=======
#include "SmartFolderDialog.h"
#include "../UiHelper.h"
#include "../../core/AppConfig.h"
#include <QScrollArea>
#include <QPainter>
#include <QPixmap>
#include <QJsonDocument>
#include <QJsonArray>
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    // 加载已有规则，若无规则则默认添加一条
    if (!m_record.rules.isEmpty()) {
        for (const auto& rule : m_record.rules) {
            SmartFolderRuleRow* row = new SmartFolderRuleRow(rule, scrollContent);
            connect(row, &SmartFolderRuleRow::removeRequested, this, &SmartFolderDialog::onRemoveRule);
            m_rulesLayout->insertWidget(m_rulesLayout->count() - 1, row);
            m_ruleRows.append(row);
        }
    } else {
        onAddRule();
    }
=======
    // 加载已有规则；若为新建且无指定规则，尝试从 AppConfig 恢复上一次保存的整套模板规则
    if (m_record.rules.isEmpty()) {
        QString lastMatchMode = AppConfig::instance().getValue("SmartFolder/LastMatchMode", "").toString();
        if (!lastMatchMode.isEmpty()) {
            m_record.matchMode = lastMatchMode;
            if (lastMatchMode == "ANY") {
                m_matchModeCombo->setCurrentIndex(1);
            } else {
                m_matchModeCombo->setCurrentIndex(0);
            }
        }

        QString lastRulesJsonStr = AppConfig::instance().getValue("SmartFolder/LastTemplateRules", "").toString();
        if (!lastRulesJsonStr.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(lastRulesJsonStr.toUtf8());
            if (doc.isArray()) {
                QJsonArray arr = doc.array();
                for (const auto& val : arr) {
                    if (val.isObject()) {
                        m_record.rules.append(SmartFolderRule::fromJson(val.toObject()));
                    }
                }
            }
        }
    }

    if (!m_record.rules.isEmpty()) {
        for (const auto& rule : m_record.rules) {
            SmartFolderRuleRow* row = new SmartFolderRuleRow(rule, scrollContent);
            connect(row, &SmartFolderRuleRow::removeRequested, this, &SmartFolderDialog::onRemoveRule);
            m_rulesLayout->insertWidget(m_rulesLayout->count() - 1, row);
            m_ruleRows.append(row);
        }
    } else {
        onAddRule();
    }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    for (SmartFolderRuleRow* row : m_ruleRows) {
        if (row) {
            m_record.rules.append(row->getRule());
        }
    }

    accept();
=======
    for (SmartFolderRuleRow* row : m_ruleRows) {
        if (row) {
            m_record.rules.append(row->getRule());
        }
    }

    // 将整套规则模板序列化持久化至 AppConfig
    AppConfig::instance().setValue("SmartFolder/LastMatchMode", m_record.matchMode);
    QJsonArray rulesArr;
    for (const auto& rule : m_record.rules) {
        rulesArr.append(rule.toJson());
    }
    QJsonDocument doc(rulesArr);
    AppConfig::instance().setValue("SmartFolder/LastTemplateRules", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    accept();
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译与验证步骤）

### 4.1 编译验证
通过 CMake 增量编译项目，确保无 C2039/C2065 或语法错误：
```bash
cmake --build build --config Release
```

### 4.2 验收测试序列
- **测试 1（添加多条规则并保存）**：打开“智能文件夹规则配置”，设置匹配模式为“ANY”，添加两条规则（例如：“扩展名等于 png”、“颜色等于 #FF0000”），保存。
- **测试 2（新建智能文件夹恢复模板）**：再次点击“+ 新建智能文件夹”，观察打开的界面：匹配模式应为“ANY”，且规则列表自动呈现上一次保存的“扩展名”和“颜色”两条规则。
- **测试 3（编辑已有智能文件夹）**：右键已有特定规则的智能文件夹选择“编辑规则”，检查显示的规则是否为该文件夹自身的规则，不受影响。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用自查）

- **配置持久化**：统一复用 `AppConfig::instance().setValue(...)` 与 `getValue(...)` 入口，无私设本地 QSettings 文件。
- **规则 JSON 转换**：统一复用 `SmartFolderRule::toJson()` 与 `SmartFolderRule::fromJson()`，无另起炉灶。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用类/函数 | 声明头文件 | 物理签名原文 |
|---|---|---|
| `AppConfig::setValue` | `AppConfig.h` | `void setValue(const QString& key, const QVariant& value);` |
| `AppConfig::getValue` | `AppConfig.h` | `QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;` |
| `SmartFolderRule::toJson` | `SmartFolderDao.h` | `QJsonObject toJson() const;` |
| `SmartFolderRule::fromJson` | `SmartFolderDao.h` | `static SmartFolderRule fromJson(const QJsonObject& obj);` |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `SmartFolderDialog.cpp`: 补充 `#include "../../core/AppConfig.h"`, `<QJsonDocument>`, `<QJsonArray>`，类型完整性闭合。

---

## 8. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 核对结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 方案文件名 `SmartFolderLastRulesMemory.md` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 第 3 章替换块 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 满足 | 未新增 Q_OBJECT 类，不涉及 |
| 第 1~9 章齐全 | 3.4 | 满足 | 全章节 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 第 6 章核查表 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 第 7 章核查表 |
| 已复用既有 SSOT 入口，无另起炉灶 | 2.4 | 满足 | 第 5 章 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 满足 | 不涉及样式改动 |
| 无平台级 Hack | 4.2 | 满足 | 无 Hack |
| 迁移的所有数值原样照抄 | 4.4 | 满足 | 第 9 章 |
| 未改动用户既有设计理念与交互方式 | 1.2 / Memories.md | 满足 | 准确满足用户明确要求 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 第 2 章文件清单 |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

- **界面样式与几何尺寸**：`SmartFolderDialog` 的 `resize(520, 420)`、`setContentsMargins(20, 15, 20, 20)`、`setSpacing(12)`、`setFixedSize(80, 32)` 等数值完全原封不动保留。
