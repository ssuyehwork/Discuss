# AspectRatioGroup Refactoring Plan

## 1. Overview
把 `FilterPanel.cpp` 中 `rebuildGroups()` 里的“图像比例”分组，原样搬到独立的新类 `AspectRatioGroup` 中。只做搬迁，不做重写、不做优化、不做重新设计。新类不持有 `FilterPanel` 指针，不包含 `FilterPanel.h`。

## 2. Modified Files List
- **新增**：`src/ui/AspectRatioGroup.h`
- **新增**：`src/ui/AspectRatioGroup.cpp`
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
    src/ui/AspectRatioGroup.h
    src/ui/AspectRatioGroup.cpp
>>>>>>> REPLACE
```

### `src/ui/AspectRatioGroup.h` (New File)
```cpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <functional>
#include "FilterStateModel.h"
#include "ScanStatsEngine.h"

namespace QuarkMeta {

class AspectRatioGroup {
public:
    using AddFilterRowFunc = std::function<QCheckBox*(QVBoxLayout* layout, const QString& label, int count)>;

    static void populate(QWidget* parentWidget,
                         QVBoxLayout* contentLayout,
                         FilterStateModel* filterModel,
                         const QuarkMeta::ScanStats& currentStats,
                         const FilterState& currentState,
                         AddFilterRowFunc addFilterRow);
};

} // namespace QuarkMeta
```

### `src/ui/AspectRatioGroup.cpp` (New File)
```cpp
#include "AspectRatioGroup.h"
#include <QButtonGroup>
#include <QList>
#include <tuple>

namespace QuarkMeta {

void AspectRatioGroup::populate(QWidget* parentWidget,
                                QVBoxLayout* contentLayout,
                                FilterStateModel* filterModel,
                                const QuarkMeta::ScanStats& currentStats,
                                const FilterState& currentState,
                                AddFilterRowFunc addFilterRow) {
    if (!contentLayout || !filterModel || !addFilterRow) return;

    QButtonGroup* ratioGroup = new QButtonGroup(parentWidget ? parentWidget : contentLayout->parentWidget());
    ratioGroup->setExclusive(false);

    const QList<std::tuple<FilterState::AspectRatio, QString, int>> ratioItems = {
        {FilterState::Horizontal, "横图", currentStats.ratioHorizontalCount},
        {FilterState::Vertical, "竖图", currentStats.ratioVerticalCount},
        {FilterState::Square, "方形", currentStats.ratioSquareCount},
        {FilterState::Ratio169, "16:9", currentStats.ratio169Count}
    };
    for (const auto& [ratio, label, count] : ratioItems) {
        QCheckBox* cb = addFilterRow(contentLayout, label, count);
        if (currentState.ratio == ratio) cb->setChecked(true);
        QObject::connect(cb, &QCheckBox::toggled, cb, [filterModel, ratio, ratioGroup, cb](bool on) {
            FilterState st = filterModel->state();
            if (on) {
                for (QAbstractButton* b : ratioGroup->buttons()) if (b != cb && b->isChecked()) b->setChecked(false);
                st.ratio = ratio;
            } else st.ratio = FilterState::AspectAny;
            filterModel->setState(st);
        });
        ratioGroup->addButton(cb);
    }
}

} // namespace QuarkMeta
```

### `src/ui/FilterPanel.cpp`
```diff
<<<<<<< SEARCH
#include "FilterPanel.h"
=======
#include "FilterPanel.h"
#include "AspectRatioGroup.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 11. 图像比例 ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("图像比例", gl);

        QButtonGroup* ratioGroup = new QButtonGroup(g);
        ratioGroup->setExclusive(false);

        const QList<std::tuple<FilterState::AspectRatio, QString, int>> ratioItems = {
            {FilterState::Horizontal, "横图", m_currentStats.ratioHorizontalCount},
            {FilterState::Vertical, "竖图", m_currentStats.ratioVerticalCount},
            {FilterState::Square, "方形", m_currentStats.ratioSquareCount},
            {FilterState::Ratio169, "16:9", m_currentStats.ratio169Count}
        };
        for (const auto& [ratio, label, count] : ratioItems) {
            QCheckBox* cb = addFilterRow(gl, label, count);
            if (currentSt.ratio == ratio) cb->setChecked(true);
            connect(cb, &QCheckBox::toggled, this, [this, ratio, ratioGroup, cb](bool on) {
                FilterState st = m_filterModel->state();
                if (on) {
                    for (QAbstractButton* b : ratioGroup->buttons()) if (b != cb && b->isChecked()) b->setChecked(false);
                    st.ratio = ratio;
                } else st.ratio = FilterState::AspectAny;
                m_filterModel->setState(st);
            });
            ratioGroup->addButton(cb);
        }
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 11. 图像比例 ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("图像比例", gl);

        AspectRatioGroup::populate(g, gl, m_filterModel, m_currentStats, currentSt,
            [this](QVBoxLayout* layout, const QString& label, int count) {
                return addFilterRow(layout, label, count);
            });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 运行 CMake 配置并编译项目。
2. 验证“图像比例”分组下的四个选项（“横图”、“竖图”、“方形”、“16:9”）及其数量显示。
3. 测试选项点击、多选互斥取消、取消勾选恢复 `AspectAny` 及重置筛选等功能。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 恪守现有 SSOT 规范，状态修改统一经由 `FilterStateModel::setState()` 驱动；
- `addFilterRow` 统一回调 `FilterPanel` 现有接口，无私自另起炉灶。

## 6. Header API Signature Verification
- `FilterStateModel::state()`: `FilterState state() const`
- `FilterStateModel::setState(const FilterState&)`: `void setState(const FilterState& state)`
- `FilterPanel::addFilterRow(QVBoxLayout*, const QString&, int)`: `QCheckBox* addFilterRow(...)`

## 7. Header Inclusion Chain & Type Completeness Check
- `AspectRatioGroup.h` 包含了 `<QWidget>`, `<QVBoxLayout>`, `<QCheckBox>`, `<functional>`, `"FilterStateModel.h"`, `"ScanStatsEngine.h"`，类型完整。
- `AspectRatioGroup.cpp` 包含了 `"AspectRatioGroup.h"`, `<QButtonGroup>`, `<QList>`, `<tuple>`，无隐式类型断裂。
