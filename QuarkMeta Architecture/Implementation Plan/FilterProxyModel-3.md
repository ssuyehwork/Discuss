# Implementation Plan: Multi-Item Search Filter Enhancement (FilterProxyModel-3.md)

## 1. Overview
本实施方案旨在增强系统搜索框的匹配筛选能力。当用户在顶部搜索框输入或粘贴包含分行 (`\r\n`)、顿号 (`、`) 或逗号 (`,`, `，`) 的文本时，系统自动将检索字符串解构为多个独立的子关键词，并在筛选引擎 `FilterProxyModel` 中执行多项目并行匹配（逻辑 OR 匹配）。

只要文件项目的名称、关联标签或备注包含任意一个子关键词，即判定匹配成功。该改动与右键菜单“复制名称（分行、顿号、逗号）”无缝闭环，大幅提升多项目批量检索效率与容错率。

---

## 2. Modified Files List
* `src/ui/models/FilterProxyModel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File: `src/ui/models/FilterProxyModel.cpp`

<<<<<<< SEARCH
#include "FilterProxyModel.h"
#include "ItemModelBase.h"
#include "../UiHelper.h"
#include <QDateTime>
#include <cmath>
=======
#include "FilterProxyModel.h"
#include "ItemModelBase.h"
#include "../UiHelper.h"
#include <QDateTime>
#include <QRegularExpression>
#include <cmath>
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 7. 搜索关键词匹配
    if (!currentFilter.keyword.isEmpty()) {
        const QString& kw = currentFilter.keyword;
        bool match = record.filename.contains(kw, Qt::CaseInsensitive);

        if (!match) {
            for (const QString& tag : record.tags) {
                if (tag.contains(kw, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
        }

        if (!match && !record.note.isEmpty()) {
            if (record.note.contains(kw, Qt::CaseInsensitive)) {
                match = true;
            }
        }

        if (!match) return false;
    }
=======
    // 7. 搜索关键词匹配 (支持分行、顿号、逗号切割的多项目 OR 匹配)
    if (!currentFilter.keyword.isEmpty()) {
        static const QRegularExpression sepRegex("[\r\n、,，]+");
        QStringList keywords = currentFilter.keyword.split(sepRegex, Qt::SkipEmptyParts);

        if (!keywords.isEmpty()) {
            bool anyMatch = false;
            for (const QString& rawKw : keywords) {
                QString kw = rawKw.trimmed();
                if (kw.isEmpty()) continue;

                bool match = record.filename.contains(kw, Qt::CaseInsensitive);

                if (!match) {
                    for (const QString& tag : record.tags) {
                        if (tag.contains(kw, Qt::CaseInsensitive)) {
                            match = true;
                            break;
                        }
                    }
                }

                if (!match && !record.note.isEmpty()) {
                    if (record.note.contains(kw, Qt::CaseInsensitive)) {
                        match = true;
                    }
                }

                if (match) {
                    anyMatch = true;
                    break;
                }
            }

            if (!anyMatch) return false;
        }
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### 构建步骤
使用 CMake 增量编译项目：
```bash
cmake --build build --config Debug --target QuarkMeta
```

### 验证步骤
1. 启动 QuarkMeta 应用程序；
2. 在顶部搜索框中分别测试输入/粘贴以下三种格式的字符串：
   - **分行格式**：`文件A.txt\n文件B.psd`
   - **顿号格式**：`文件A.txt、文件B.psd`
   - **逗号格式**：`文件A.txt, 文件B.psd` 或 `文件A.txt，文件B.psd`
3. 验证结果列表同时精确定位展示出 `文件A.txt` 与 `文件B.psd` 两个文件；
4. 复制多个项目名称，直接快捷键 `Ctrl+V` 粘贴至顶部搜索框，验证能够一键搜出刚才复制的所有项目；
5. 清空搜索框，验证视图平滑复原全量展示。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

| 自查项 | 结论 | 说明 |
| :--- | :--- | :--- |
| **是否复用既有 SSOT 通道？** | **是** | 严格集中于 `FilterProxyModel::filterAcceptsRow` 过滤引擎 |
| **是否存在另起炉灶行为？** | **否** | 无需新增代理类，完全在既有模式匹配链条中增强 |
| **匹配效率与性能** | **高效** | 采用静态 `QRegularExpression` 预编译正则，循环中匹配成功即 `break` 提前退出 |

---

## 6. Header API Signature Verification

| 被调用的类/函数 | `.h` 文件真实物理签名 | 核查状态 |
| :--- | :--- | :--- |
| `QString::split` | `QStringList split(const QRegularExpression& re, QString::SplitBehavior behavior = QString::KeepEmptyParts) const;` | Qt 标准接口，匹配 |
| `QRegularExpression` | `explicit QRegularExpression(const QString& pattern, PatternOptions options = NoPatternOption);` | Qt 标准类，匹配 |

---

## 7. Header Inclusion Chain & Type Completeness Check

| 修改文件 | 新增/引用的类与枚举 | 头文件包含状态 |
| :--- | :--- | :--- |
| `FilterProxyModel.cpp` | `QRegularExpression` | 显式新增 `#include <QRegularExpression>`，包含链闭合完整 |
