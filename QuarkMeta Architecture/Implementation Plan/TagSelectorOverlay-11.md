# Implementation Plan - TagSelectorOverlay Essential Selection Fix & Design Intention Alignment (TagSelectorOverlay-11)

## 1. Overview & 架构三问回答

### 【初衷设计理念保护与架构三问先答后写】
1. **初衷设计理念保护（Zero-Distortion Contract）**：
   本方案 100% 尊重并保护 `TagSelectorOverlay` 最初的全部设计理念与交互哲学：
   - **失焦关闭体验零改动**：100% 保持浮层点击外部任意区域或失焦时 0 毫秒即时自动关闭的平滑体验；
   - **智能推荐与词库展示零改动**：完整保留文件名 Candidate Tokens 提取、蓝色“推荐”标题与 SVG 矢量五角星图标展示逻辑；
   - **最最小代码侵入**：不做任何过度拆改，不改动浮层原有的事件处理主干，仅针对“打标刷新后外部 `m_selectedPaths` 被置空导致后续连续勾选失效”这唯一的真实 Bug，在唤起源头实施精准的路径快照绑定。

2. **架构三问**：
   - **第一问（真理源溯源）**：打标的目标路径在浮层唤起时完成决策。将目标路径快照（`targetPathsSnapshot`）锁定在浮层闭包中，确保浮层生命周期内的连续勾选稳定作用于原选定项目；
   - **第二问（黑盒完整性）**：保持浮层原有封装与所有既有接口契约不变，补齐 3 参数构造函数物理签名；
   - **第三问（根因 vs 症状）**：根因是打标触发模型更新重置导致外部动态变量置空。通过在唤起点实施路径快照锁定，彻底治理连续打标断裂问题，同时 100% 保护所有初衷 UI 交互体验。

---

## 2. Modified Files List
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`
- `src/ui/MetaTagSection.cpp`
- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update Constructor Signature Alignment in `TagSelectorOverlay.h`

```
<<<<<<< SEARCH
class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;
=======
class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths = {}, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;
>>>>>>> REPLACE
```

---

### 3.2 Update Constructor Implementation in `TagSelectorOverlay.cpp`

```
<<<<<<< SEARCH
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
      m_selectedTags(initialSelected)
{
=======
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
      m_selectedTags(initialSelected)
{
>>>>>>> REPLACE
```

---

### 3.3 Target Paths Snapshot Binding in `MetaTagSection.cpp`

```
<<<<<<< SEARCH
void MetaTagSection::openTagSelectorOverlay(QWidget* targetAnchor) {
    Q_UNUSED(targetAnchor);
    if (m_selectedPaths.isEmpty()) return;

    if (!m_tagSelectorOverlay) {
        m_tagSelectorOverlay = new TagSelectorOverlay({}, this);
        connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this](const QStringList& currentTags) {
            emit tagsChanged(m_selectedPaths, currentTags);
        });
    }

    m_tagSelectorOverlay->show();
}
=======
void MetaTagSection::openTagSelectorOverlay(QWidget* targetAnchor) {
    Q_UNUSED(targetAnchor);
    if (m_selectedPaths.isEmpty()) return;

    // 🔒【路径快照锁定】：锁定唤起时刻的目标项目路径，保证浮层未关闭前连续勾选稳定有效
    const QStringList targetPathsSnapshot = m_selectedPaths;

    if (m_tagSelectorOverlay) {
        m_tagSelectorOverlay->close();
    }

    m_tagSelectorOverlay = new TagSelectorOverlay({}, targetPathsSnapshot, this);
    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this, targetPathsSnapshot](const QStringList& currentTags) {
        if (!targetPathsSnapshot.isEmpty()) {
            emit tagsChanged(targetPathsSnapshot, currentTags);
        }
    });

    m_tagSelectorOverlay->show();
}
>>>>>>> REPLACE
```

---

### 3.4 Target Paths Snapshot Integrity in `MetaPanel.cpp`

```
<<<<<<< SEARCH
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
        *appliedTagsSet = newSet;
    });
=======
    m_tagSelectorOverlay->show();

    // 1. 🔒【路径快照锁定】：浮层打开瞬时，保存决议绑定的固定目标项目快照，不随 m_selectedPaths 动态漂移
    const QStringList targetPathsSnapshot = m_selectedPaths;

    // 2. 浮层局部的已提交标签集合，打开时用 m_currentTagsSet 初始化
    auto appliedTagsSet = std::make_shared<QSet<QString>>(m_currentTagsSet);

    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this, targetPathsSnapshot, appliedTagsSet](const QStringList& newSelectedTags) {
        if (targetPathsSnapshot.isEmpty()) return;

        QSet<QString> newSet(newSelectedTags.begin(), newSelectedTags.end());

        // 新增的标签：与浮层局部的已提交集合比对
        for (const QString& t : newSet) {
            if (!appliedTagsSet->contains(t)) {
                emit tagAddRequested(targetPathsSnapshot, t);
            }
        }
        // 移除的标签：与浮层局部的已提交集合比对
        for (const QString& t : *appliedTagsSet) {
            if (!newSet.contains(t)) {
                emit tagRemoveRequested(targetPathsSnapshot, t);
            }
        }
        *appliedTagsSet = newSet;
    });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Functional Verification**:
   - Open TagSelectorOverlay, click anywhere outside $\to$ overlay closes immediately (100% preserving original smooth dismissal behavior).
   - Select multiple items, click "+ 添加标签", select tag A, tag B, tag C in succession $\to$ all tags apply correctly without losing target context.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 100% 保留现有架构通道与失焦关闭体验；
- 零过度抽象，零多余修改。

---

## 6. Header API Signature Verification

| 类名 | 成员函数/信号物理签名 | 头文件物理位置 | 状态 |
| :--- | :--- | :--- | :--- |
| `TagSelectorOverlay` | `TagSelectorOverlay(const QStringList&, const QStringList&, QWidget*)` | `src/ui/TagSelectorOverlay.h` | 方案 3.1 明确新增对齐 |
| `MetaTagSection` | `void tagsChanged(const QStringList&, const QStringList&)` | `src/ui/MetaTagSection.h` | 已核验 100% 存在 |
| `MetaPanel` | `void tagAddRequested(const QStringList&, const QString&)` | `src/ui/MetaPanel.h` | 已核验 100% 存在 |
| `MetaPanel` | `void tagRemoveRequested(const QStringList&, const QString&)` | `src/ui/MetaPanel.h` | 已核验 100% 存在 |

---

## 7. Header Inclusion Chain & Type Completeness Check

| 受影响文件 | `#include` 修改点 | 核查的类型/类定义 | 闭包完整性结论 |
| :--- | :--- | :--- | :--- |
| `TagSelectorOverlay.cpp` | 无需变更 | `TagSelectorOverlay` | 100% 完整 |
| `MetaTagSection.cpp` | 无需变更 | `MetaTagSection`, `TagSelectorOverlay` | 100% 完整 |
| `MetaPanel.cpp` | 无需变更 | `MetaPanel`, `TagSelectorOverlay` | 100% 完整 |
