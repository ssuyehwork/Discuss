# CreateDateGroup Refactoring Plan

## 1. Overview
把 `FilterPanel.cpp` 中 `rebuildGroups()` 里的“创建日期”分组，原样搬到独立的新类 `CreateDateGroup` 中。只做搬迁，不做重写、不做优化、不做重新设计。新类不持有 `FilterPanel` 指针，不包含 `FilterPanel.h`。

## 2. Modified Files List
- **新增**：`src/ui/CreateDateGroup.h`
- **新增**：`src/ui/CreateDateGroup.cpp`
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
    src/ui/CreateDateGroup.h
    src/ui/CreateDateGroup.cpp
>>>>>>> REPLACE
```

### `src/ui/CreateDateGroup.h` (New File)
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

class CreateDateGroup {
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

### `src/ui/CreateDateGroup.cpp` (New File)
```cpp
#include "CreateDateGroup.h"
#include "UiHelper.h"

namespace QuarkMeta {

QLineEdit* CreateDateGroup::populate(QObject* owner,
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
        if (rebuildCheckboxes) rebuildCheckboxes(true, descending);
    });

    QWidget* wCreateDate = new QWidget(groupWidget);
    QHBoxLayout* lCreateDate = new QHBoxLayout(wCreateDate);
    lCreateDate->setContentsMargins(5, 6, 5, 4);
    lCreateDate->setSpacing(0);

    QLineEdit* editCreateDate = new QLineEdit(wCreateDate);
    editCreateDate->setClearButtonEnabled(true);
    editCreateDate->setPlaceholderText("例： 2025 / 03-2025...");
    editCreateDate->setText(currentState.createDateFilterText);
    editCreateDate->setObjectName("FilterSearchEdit");
    editCreateDate->setFixedHeight(22);
    if (owner) editCreateDate->installEventFilter(owner);

    QObject::connect(editCreateDate, &QLineEdit::returnPressed, owner, [filterModel, editCreateDate, saveHistory]() {
        FilterState st = filterModel->state();
        st.createDateFilterText = editCreateDate->text();
        if (saveHistory) saveHistory("CreateDate", st.createDateFilterText);
        filterModel->setState(st);
    });

    QObject::connect(editCreateDate, &QLineEdit::textChanged, owner, [filterModel](const QString& text) {
        FilterState st = filterModel->state();
        if (text.isEmpty() && !st.createDateFilterText.isEmpty()) {
            st.createDateFilterText = "";
            filterModel->setState(st);
        }
    });

    lCreateDate->addWidget(editCreateDate);
    contentLayout->addWidget(wCreateDate);

    if (rebuildCheckboxes) rebuildCheckboxes(true, descending);

    return editCreateDate;
}

} // namespace QuarkMeta
```

### `src/ui/FilterPanel.cpp`
```diff
<<<<<<< SEARCH
#include "FilterPanel.h"
=======
#include "FilterPanel.h"
#include "CreateDateGroup.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 5. 创建日期 ──────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("创建日期", gl, &hdrLayout);
        m_createDateLayout = gl;

        QPushButton* btnSort = new QPushButton(g);
        btnSort->setFixedSize(16, 16);
        btnSort->setIconSize(QSize(12, 12));
        btnSort->setIcon(UiHelper::getIcon(m_createDateDesc ? "scroll-010.svg" : "scroll-007.svg", QColor("#B0B0B0")));
        btnSort->setFlat(true);
        btnSort->setCursor(Qt::PointingHandCursor);
        btnSort->setObjectName("FilterBtnSort");
        hdrLayout->addWidget(btnSort);
        connect(btnSort, &QPushButton::clicked, this, [this, btnSort]() {
            m_createDateDesc = !m_createDateDesc;
            btnSort->setIcon(UiHelper::getIcon(m_createDateDesc ? "scroll-010.svg" : "scroll-007.svg", QColor("#B0B0B0")));
            rebuildDateCheckboxes(true, m_createDateDesc);
        });

        QWidget* wCreateDate = new QWidget(g);
        QHBoxLayout* lCreateDate = new QHBoxLayout(wCreateDate);
        lCreateDate->setContentsMargins(5, 6, 5, 4);
        lCreateDate->setSpacing(0);

        m_editCreateDate = new QLineEdit(wCreateDate);
        m_editCreateDate->setClearButtonEnabled(true);
        m_editCreateDate->setPlaceholderText("例： 2025 / 03-2025...");
        m_editCreateDate->setText(currentSt.createDateFilterText);
        m_editCreateDate->setObjectName("FilterSearchEdit");
        m_editCreateDate->setFixedHeight(22);
        m_editCreateDate->installEventFilter(this);
        connect(m_editCreateDate, &QLineEdit::returnPressed, this, [this]() {
            FilterState st = m_filterModel->state();
            st.createDateFilterText = m_editCreateDate->text();
            saveFilterHistory("CreateDate", st.createDateFilterText);
            m_filterModel->setState(st);
        });
        connect(m_editCreateDate, &QLineEdit::textChanged, this, [this](const QString& text) {
            FilterState st = m_filterModel->state();
            if (text.isEmpty() && !st.createDateFilterText.isEmpty()) {
                st.createDateFilterText = "";
                m_filterModel->setState(st);
            }
        });
        lCreateDate->addWidget(m_editCreateDate);
        gl->addWidget(wCreateDate);

        rebuildDateCheckboxes(true, m_createDateDesc);
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 5. 创建日期 ──────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("创建日期", gl, &hdrLayout);
        m_createDateLayout = gl;

        m_editCreateDate = CreateDateGroup::populate(this, g, hdrLayout, gl, m_filterModel, currentSt,
            m_createDateDesc,
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
2. 验证“创建日期”分组下排序按钮与日期筛选搜索输入框的渲染。
3. 测试点击排序按钮翻转 `m_createDateDesc` 与图标、输入框回车提交/历史记录触发/清空逻辑，核对 `m_createDateLayout` 在 `rebuildDateCheckboxes` 前已完成赋值。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 恪守现有 SSOT 规范，状态修改统一经由 `FilterStateModel::setState()` 驱动；
- `m_createDateLayout` 保留在 `FilterPanel.cpp` 主流程赋值，保证 `rebuildDateCheckboxes` 的 SSOT 操作无盲区。

## 6. Header API Signature Verification
- `FilterStateModel::state()`: `FilterState state() const`
- `FilterStateModel::setState(const FilterState&)`: `void setState(const FilterState& state)`
- `FilterPanel::rebuildDateCheckboxes(...)`: `void rebuildDateCheckboxes(bool isCreateDate, bool descending)`
- `FilterPanel::saveFilterHistory(...)`: `void saveFilterHistory(const QString& key, const QString& text)`

## 7. Header Inclusion Chain & Type Completeness Check
- `CreateDateGroup.h` 包含了 `<QWidget>`, `<QVBoxLayout>`, `<QHBoxLayout>`, `<QLineEdit>`, `<QPushButton>`, `<functional>`, `"FilterStateModel.h"`，类型完整。
- `CreateDateGroup.cpp` 包含了 `"CreateDateGroup.h"`, `"UiHelper.h"`，保证 `UiHelper::getIcon` 定义可用。
