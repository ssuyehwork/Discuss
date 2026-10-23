# ModifyDateGroup Refactoring Plan

## 1. Overview
把 `FilterPanel.cpp` 中 `rebuildGroups()` 里的“修改日期”分组，原样搬到独立的新类 `ModifyDateGroup` 中。只做搬迁，不做重写、不做优化、不做重新设计。新类不持有 `FilterPanel` 指针，不包含 `FilterPanel.h`。

## 2. Modified Files List
- **新增**：`src/ui/ModifyDateGroup.h`
- **新增**：`src/ui/ModifyDateGroup.cpp`
- **修改**：`src/ui/FilterPanel.h`（仅添加前置声明或必要包含）
- **修改**：`src/ui/FilterPanel.cpp`
- **修改**：`CMakeLists.txt`（添加新文件到 `SOURCES` 列表）

## 3. Detailed Line-by-Line Changes

### `CMakeLists.txt`
```diff
<<<<<<< SEARCH
    src/ui/FilterPanel.cpp
    src/ui/FilterPanel.h
=======
    src/ui/FilterPanel.cpp
    src/ui/FilterPanel.h
    src/ui/ModifyDateGroup.h
    src/ui/ModifyDateGroup.cpp
>>>>>>> REPLACE
```

### `src/ui/ModifyDateGroup.h` (New File)
```cpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <functional>
#include "FilterStateModel.h"

namespace QuarkMeta {

class ModifyDateGroup {
public:
    using RebuildDateCheckboxesFunc = std::function<void(bool isCreateDate, bool descending)>;
    using SaveFilterHistoryFunc = std::function<void(const QString& key, const QString& text)>;

    static QLineEdit* populate(QObject* owner,
                               QWidget* groupWidget,
                               QHBoxLayout* headerLayout,
                               QVBoxLayout* contentLayout,
                               FilterStateModel* filterModel,
                               const FilterState& currentState,
                               bool& descending,
                               RebuildDateCheckboxesFunc rebuildCheckboxes,
                               SaveFilterHistoryFunc saveHistory);
};

} // namespace QuarkMeta
```

### `src/ui/ModifyDateGroup.cpp` (New File)
```cpp
#include "ModifyDateGroup.h"
#include "UiHelper.h"

namespace QuarkMeta {

QLineEdit* ModifyDateGroup::populate(QObject* owner,
                                     QWidget* groupWidget,
                                     QHBoxLayout* headerLayout,
                                     QVBoxLayout* contentLayout,
                                     FilterStateModel* filterModel,
                                     const FilterState& currentState,
                                     bool& descending,
                                     RebuildDateCheckboxesFunc rebuildCheckboxes,
                                     SaveFilterHistoryFunc saveHistory) {
    if (!contentLayout || !filterModel || !headerLayout) return nullptr;

    QPushButton* btnSort = new QPushButton(groupWidget);
    btnSort->setFixedSize(16, 16);
    btnSort->setIconSize(QSize(12, 12));
    btnSort->setIcon(UiHelper::getIcon(descending ? "scroll-010.svg" : "scroll-007.svg", QColor("#B0B0B0")));
    btnSort->setFlat(true);
    btnSort->setCursor(Qt::PointingHandCursor);
    btnSort->setObjectName("FilterBtnSort");
    headerLayout->addWidget(btnSort);

    QObject::connect(btnSort, &QPushButton::clicked, owner, [owner, btnSort, &descending, rebuildCheckboxes]() {
        descending = !descending;
        btnSort->setIcon(UiHelper::getIcon(descending ? "scroll-010.svg" : "scroll-007.svg", QColor("#B0B0B0")));
        if (rebuildCheckboxes) rebuildCheckboxes(false, descending);
    });

    QWidget* wModifyDate = new QWidget(groupWidget);
    QHBoxLayout* lModifyDate = new QHBoxLayout(wModifyDate);
    lModifyDate->setContentsMargins(5, 6, 5, 4);
    lModifyDate->setSpacing(0);

    QLineEdit* editModifyDate = new QLineEdit(wModifyDate);
    editModifyDate->setClearButtonEnabled(true);
    editModifyDate->setPlaceholderText("例： 2025 / 03-2025...");
    editModifyDate->setText(currentState.modifyDateFilterText);
    editModifyDate->setObjectName("FilterSearchEdit");
    editModifyDate->setFixedHeight(22);
    if (owner) editModifyDate->installEventFilter(owner);

    QObject::connect(editModifyDate, &QLineEdit::returnPressed, owner, [filterModel, editModifyDate, saveHistory]() {
        FilterState st = filterModel->state();
        st.modifyDateFilterText = editModifyDate->text();
        if (saveHistory) saveHistory("ModifyDate", st.modifyDateFilterText);
        filterModel->setState(st);
    });

    QObject::connect(editModifyDate, &QLineEdit::textChanged, owner, [filterModel](const QString& text) {
        FilterState st = filterModel->state();
        if (text.isEmpty() && !st.modifyDateFilterText.isEmpty()) {
            st.modifyDateFilterText = "";
            filterModel->setState(st);
        }
    });

    lModifyDate->addWidget(editModifyDate);
    contentLayout->addWidget(wModifyDate);

    if (rebuildCheckboxes) rebuildCheckboxes(false, descending);

    return editModifyDate;
}

} // namespace QuarkMeta
```

### `src/ui/FilterPanel.cpp`
```diff
<<<<<<< SEARCH
#include "FilterPanel.h"
=======
#include "FilterPanel.h"
#include "ModifyDateGroup.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 6. 修改日期 ──────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("修改日期", gl, &hdrLayout);
        m_modifyDateLayout = gl;

        QPushButton* btnSort = new QPushButton(g);
        btnSort->setFixedSize(16, 16);
        btnSort->setIconSize(QSize(12, 12));
        btnSort->setIcon(UiHelper::getIcon(m_modifyDateDesc ? "scroll-010.svg" : "scroll-007.svg", QColor("#B0B0B0")));
        btnSort->setFlat(true);
        btnSort->setCursor(Qt::PointingHandCursor);
        btnSort->setObjectName("FilterBtnSort");
        hdrLayout->addWidget(btnSort);
        connect(btnSort, &QPushButton::clicked, this, [this, btnSort]() {
            m_modifyDateDesc = !m_modifyDateDesc;
            btnSort->setIcon(UiHelper::getIcon(m_modifyDateDesc ? "scroll-010.svg" : "scroll-007.svg", QColor("#B0B0B0")));
            rebuildDateCheckboxes(false, m_modifyDateDesc);
        });

        QWidget* wModifyDate = new QWidget(g);
        QHBoxLayout* lModifyDate = new QHBoxLayout(wModifyDate);
        lModifyDate->setContentsMargins(5, 6, 5, 4);
        lModifyDate->setSpacing(0);

        m_editModifyDate = new QLineEdit(wModifyDate);
        m_editModifyDate->setClearButtonEnabled(true);
        m_editModifyDate->setPlaceholderText("例： 2025 / 03-2025...");
        m_editModifyDate->setText(currentSt.modifyDateFilterText);
        m_editModifyDate->setObjectName("FilterSearchEdit");
        m_editModifyDate->setFixedHeight(22);
        m_editModifyDate->installEventFilter(this);
        connect(m_editModifyDate, &QLineEdit::returnPressed, this, [this]() {
            FilterState st = m_filterModel->state();
            st.modifyDateFilterText = m_editModifyDate->text();
            saveFilterHistory("ModifyDate", st.modifyDateFilterText);
            m_filterModel->setState(st);
        });
        connect(m_editModifyDate, &QLineEdit::textChanged, this, [this](const QString& text) {
            FilterState st = m_filterModel->state();
            if (text.isEmpty() && !st.modifyDateFilterText.isEmpty()) {
                st.modifyDateFilterText = "";
                m_filterModel->setState(st);
            }
        });
        lModifyDate->addWidget(m_editModifyDate);
        gl->addWidget(wModifyDate);

        rebuildDateCheckboxes(false, m_modifyDateDesc);
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 6. 修改日期 ──────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("修改日期", gl, &hdrLayout);
        m_modifyDateLayout = gl;

        m_editModifyDate = ModifyDateGroup::populate(this, g, hdrLayout, gl, m_filterModel, currentSt,
            m_modifyDateDesc,
            [this](bool isCreateDate, bool descending) {
                rebuildDateCheckboxes(isCreateDate, descending);
            },
            [this](const QString& key, const QString& text) {
                saveFilterHistory(key, text);
            });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 运行 CMake 配置并编译项目。
2. 验证“修改日期”分组下排序按钮与日期筛选搜索输入框的渲染。
3. 测试点击排序按钮翻转 `m_modifyDateDesc` 与图标、输入框回车提交/历史记录触发/清空逻辑，核对 `m_modifyDateLayout` 在 `rebuildDateCheckboxes(false, descending)` 前已完成赋值且第一个参数为 `false`。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 恪守现有 SSOT 规范，状态修改统一经由 `FilterStateModel::setState()` 驱动；
- `m_modifyDateLayout` 保留在 `FilterPanel.cpp` 主流程赋值，保证 `rebuildDateCheckboxes` 的 SSOT 操作无盲区。

## 6. Header API Signature Verification
- `FilterStateModel::state()`: `FilterState state() const`
- `FilterStateModel::setState(const FilterState&)`: `void setState(const FilterState& state)`
- `FilterPanel::rebuildDateCheckboxes(...)`: `void rebuildDateCheckboxes(bool isCreateDate, bool descending)`
- `FilterPanel::saveFilterHistory(...)`: `void saveFilterHistory(const QString& key, const QString& text)`

## 7. Header Inclusion Chain & Type Completeness Check
- `ModifyDateGroup.h` 包含了 `<QWidget>`, `<QVBoxLayout>`, `<QHBoxLayout>`, `<QLineEdit>`, `<QPushButton>`, `<functional>`, `"FilterStateModel.h"`，类型完整。
- `ModifyDateGroup.cpp` 包含了 `"ModifyDateGroup.h"`, `"UiHelper.h"`，保证 `UiHelper::getIcon` 定义可用。
