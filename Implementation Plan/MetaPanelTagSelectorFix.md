# Implementation Plan: MetaPanelTagSelectorFix.md

## 1. Overview（概述与解决的问题）
修复 `MetaPanel` 标签选择浮层（`TagSelectorOverlay`）在多选标签时从第二个勾选标签起无法成功绑定至目标项目的问题。

### 根因与解耦修复原理：
- **旧实现弊端**：在 `MetaPanel::openTagSelectorOverlay` 中，`selectionChanged` 的 Lambda 闭包直接引用了实时成员变量 `m_selectedPaths` 与 `m_currentTagsSet`。浮层存活期间，一旦内容面板的选区改变或重置，`m_selectedPaths` 变成空列表，导致二次及后续抛出的 `tagAddRequested(m_selectedPaths, t)` 包含空路径，被 `PanelMediator` 直接拦截舍弃。
- **全新修复方案**：
  1. **固定目标路径快照**：在浮层打开的瞬间捕获 `const QStringList targetPaths = m_selectedPaths;` 闭包变量。浮层存活期间所有 `tagAddRequested` / `tagRemoveRequested` 均严格绑定该固定快照；
  2. **局部已提交集合**：在 Lambda 中维持 `QSet<QString> appliedTagsSet = m_currentTagsSet;` 局部状态，差集比对完全基于该局部快照；
  3. **面板呈现防护**：仅在 `m_selectedPaths == targetPaths` 时才触发 `setTags(newSelectedTags)` 界面回显。

---

## 2. Modified Files List（影响文件清单）
1. `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（包含精准替换块）

### `src/ui/MetaPanel.cpp`

```
<<<<<<< SEARCH
    m_tagSelectorOverlay->show();

    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this](const QStringList& newSelectedTags) {
        QSet<QString> newSet(newSelectedTags.begin(), newSelectedTags.end());

        for (const QString& t : newSet) {
            if (!m_currentTagsSet.contains(t)) {
                emit tagAddRequested(m_selectedPaths, t);
            }
        }
        for (const QString& t : m_currentTagsSet) {
            if (!newSet.contains(t)) {
                emit tagRemoveRequested(m_selectedPaths, t);
            }
        }

        setTags(newSelectedTags);
    });
=======
    m_tagSelectorOverlay->show();

    // 1. 浮层打开瞬时，保存决议绑定的固定目标项目快照，不随 m_selectedPaths 动态漂移
    const QStringList targetPaths = m_selectedPaths;
    
    // 2. 浮层局部的已提交标签集合，打开时用 m_currentTagsSet 初始化
    auto appliedTagsSet = std::make_shared<QSet<QString>>(m_currentTagsSet);

    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this, targetPaths, appliedTagsSet](const QStringList& newSelectedTags) {
        if (targetPaths.isEmpty()) return;

        QSet<QString> newSet(newSelectedTags.begin(), newSelectedTags.end());

        // 新增的标签：与浮层局部的已提交集合比对
        for (const QString& t : newSet) {
            if (!appliedTagsSet->contains(t)) {
                emit tagAddRequested(targetPaths, t);
            }
        }
        // 移除的标签：与浮层局部的已提交集合比对
        for (const QString& t : *appliedTagsSet) {
            if (!newSet.contains(t)) {
                emit tagRemoveRequested(targetPaths, t);
            }
        }

        // 更新浮层局部的已提交状态
        *appliedTagsSet = newSet;

        // 仅在当前主选区依然与浮层固定目标一致时，才更新 MetaPanel 面板上的标签呈现
        if (m_selectedPaths == targetPaths) {
            setTags(newSelectedTags);
        }
    });
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
   - 运行程序，在内容面板中选中一个或多个项目；
   - 点击右侧 `MetaPanel` 中的“添加标签”唤起 `TagSelectorOverlay` 浮层；
   - 连续点击勾选 3~5 个标签；
   - 验证每一个勾选的标签都成功绑定至目标项目，数据库与属性面板无一遗漏。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- 100% 保持 `tagAddRequested(targetPaths, tag)` / `tagRemoveRequested(targetPaths, tag)` 信号的统一中介通道，零另起炉灶。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）
| 类名 | 调用的信号/函数物理签名 | 所在头文件 | 物理验证状态 |
| :--- | :--- | :--- | :--- |
| `MetaPanel` | `void tagAddRequested(const QStringList& paths, const QString& tag);` | `src/ui/MetaPanel.h` | 存在且物理一致 |
| `MetaPanel` | `void tagRemoveRequested(const QStringList& paths, const QString& tag);` | `src/ui/MetaPanel.h` | 存在且物理一致 |
| `MetaPanel` | `void setTags(const QStringList& tags);` | `src/ui/MetaPanel.h` | 存在且物理一致 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）
| 修改文件 | 包含的头文件 | 检查情况 |
| :--- | :--- | :--- |
| `src/ui/MetaPanel.cpp` | `#include <memory>` | 显式补充 `#include <memory>` 以支持 `std::make_shared` |
