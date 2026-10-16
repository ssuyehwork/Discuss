# Implementation Plan - TagSelectorOverlay Dual Constructor Overload C2665 Fix (TagSelectorOverlay-14)

## 1. Overview & 架构三问回答

### 【架构三问先答后写】
1. **第一问（真理源溯源）**：
   - 本修改解决 `PresetTagsDialog.cpp:153` 调用 `new TagSelectorOverlay(m_presetTags, window())` 时由于 C++ 默认参数匹配规则导致形参 2 (`const QStringList&`) 试图匹配实参 2 (`QWidget*`) 引起的 MSVC C2665 重载匹配错误；
   - 通过在物理头文件中显式拆分出 2 参数与 3 参数两个明确的构造函数重载入口，彻底治理类型匹配歧义。
2. **第二问（黑盒完整性）**：
   - 保持黑盒封装，2 参数构造函数内部平滑委托（Delegating Constructor）给 3 参数构造函数，完全向下兼容 `PresetTagsDialog` 及现有所有调用方。
3. **第三问（根因 vs 症状）**：
   - 根因：C++ 默认参数不允许跳过中间形参将实参 2（`QWidget*`）绑定至形参 3（`QWidget*`）。通过显式提供双重重载从根源消除编译期推导二义性。

---

## 2. Modified Files List
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Provide Explicit Dual Constructor Overloads in `TagSelectorOverlay.h`

```
<<<<<<< SEARCH
class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths = {}, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;
=======
class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent = nullptr);
    TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;
>>>>>>> REPLACE
```

---

### 3.2 Implement Delegating Constructor in `TagSelectorOverlay.cpp`

```
<<<<<<< SEARCH
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
      m_selectedTags(initialSelected)
{
=======
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent)
    : TagSelectorOverlay(initialSelected, QStringList{}, parent)
{
}

TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
      m_selectedTags(initialSelected)
{
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   - Verify MSVC compilation succeeds for `PresetTagsDialog.cpp:153` (`new TagSelectorOverlay(m_presetTags, window())`) without C2665 errors.
   - Verify MSVC compilation succeeds for `MetaPanel.cpp` (`new TagSelectorOverlay(m_currentTagsSet.values(), targetPathsSnapshot, topWidget)`).

2. **Functional Verification**:
   - Open TagSelectorOverlay from PresetTagsDialog $\to$ opens smoothly.
   - Open TagSelectorOverlay from MetaPanel $\to$ opens with auto-focus, collapsible sections ('推荐' and '最近使用'), and snapshot lock.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 使用 C++11 标准构造函数委托 (Delegating Constructor)，彻底消除代码冗余；
- 对外完全保持物理兼容契约。

---

## 6. Header API Signature Verification

| 类名 | 成员函数/信号物理签名 | 头文件物理位置 | 状态 |
| :--- | :--- | :--- | :--- |
| `TagSelectorOverlay` | `TagSelectorOverlay(const QStringList&, QWidget*)` | `src/ui/TagSelectorOverlay.h` | 方案 3.1 显式对齐重载 1 |
| `TagSelectorOverlay` | `TagSelectorOverlay(const QStringList&, const QStringList&, QWidget*)` | `src/ui/TagSelectorOverlay.h` | 方案 3.1 显式对齐重载 2 |

---

## 7. Header Inclusion Chain & Type Completeness Check

| 受影响文件 | `#include` 修改点 | 核查的类型/类定义 | 闭包完整性结论 |
| :--- | :--- | :--- | :--- |
| `TagSelectorOverlay.h` | 无需变更 | `TagSelectorOverlay` | 100% 完整 |
| `TagSelectorOverlay.cpp` | 无需变更 | `TagSelectorOverlay` | 100% 完整 |
