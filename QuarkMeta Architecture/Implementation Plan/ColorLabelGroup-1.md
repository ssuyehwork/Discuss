# ColorLabelGroup Refactoring Plan

## 1. Overview
把 `FilterPanel.cpp` 中 `rebuildGroups()` 里的“颜色标记”分组，原样搬到独立的新类 `ColorLabelGroup` 中。只做搬迁，不做重写、不做优化、不做重新设计。新类不持有 `FilterPanel` 指针，不包含 `FilterPanel.h`。

## 2. Modified Files List
- **新增**：`src/ui/ColorLabelGroup.h`
- **新增**：`src/ui/ColorLabelGroup.cpp`
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
    src/ui/ColorLabelGroup.h
    src/ui/ColorLabelGroup.cpp
>>>>>>> REPLACE
```

### `src/ui/ColorLabelGroup.h` (New File)
```cpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QMap>
#include <QString>
#include <QColor>
#include <functional>
#include "FilterStateModel.h"

namespace QuarkMeta {

class ColorLabelGroup {
public:
    using AddFilterRowFunc = std::function<QCheckBox*(QVBoxLayout* layout, const QString& label, int count, const QColor& color)>;

    static void populate(QWidget* parentWidget,
                         QVBoxLayout* contentLayout,
                         FilterStateModel* filterModel,
                         const QMap<QString, int>& colorCounts,
                         const FilterState& currentState,
                         AddFilterRowFunc addFilterRow);
};

} // namespace QuarkMeta
```

### `src/ui/ColorLabelGroup.cpp` (New File)
```cpp
#include "ColorLabelGroup.h"
#include "StyleLibrary.h"

namespace QuarkMeta {

void ColorLabelGroup::populate(QWidget* parentWidget,
                               QVBoxLayout* contentLayout,
                               FilterStateModel* filterModel,
                               const QMap<QString, int>& colorCounts,
                               const FilterState& currentState,
                               AddFilterRowFunc addFilterRow) {
    if (!contentLayout || !filterModel || !addFilterRow) return;

    QWidget* contextObj = parentWidget ? parentWidget : contentLayout->parentWidget();
    const auto& colorsList = Style::getColorPalette();

    for (const auto& item : colorsList) {
        int cnt = colorCounts.value(item.hex, colorCounts.value(item.name, 0));
        bool isChecked = (currentState.colors.contains(item.name) || currentState.colors.contains(item.hex));

        if (cnt == 0 && !isChecked) {
            continue;
        }

        QCheckBox* cb = addFilterRow(contentLayout, item.name, cnt, item.color);
        cb->setChecked(isChecked);
        QObject::connect(cb, &QCheckBox::checkStateChanged, contextObj, [filterModel, name = item.name, hex = item.hex](Qt::CheckState state) {
            FilterState st = filterModel->state();
            if (state == Qt::Checked) {
                if (!st.colors.contains(name)) st.colors.append(name);
            } else {
                st.colors.removeAll(name);
                st.colors.removeAll(hex);
            }
            filterModel->setState(st);
        });
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
#include "ColorLabelGroup.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 3. 颜色标记 ────────────
    {
        const auto& colorsList = Style::getColorPalette();

        QVBoxLayout* gl = nullptr;
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("颜色标记", gl, &hdrLayout);

        for (const auto& item : colorsList) {
            int cnt = m_colorCounts.value(item.hex, m_colorCounts.value(item.name, 0));
            bool isChecked = (currentSt.colors.contains(item.name) || currentSt.colors.contains(item.hex));

            if (cnt == 0 && !isChecked) {
                continue;
            }

            QCheckBox* cb = addFilterRow(gl, item.name, cnt, item.color);
            cb->setChecked(isChecked);
            connect(cb, &QCheckBox::checkStateChanged, this, [this, name = item.name, hex = item.hex](Qt::CheckState state) {
                FilterState st = m_filterModel->state();
                if (state == Qt::Checked) {
                    if (!st.colors.contains(name)) st.colors.append(name);
                } else {
                    st.colors.removeAll(name);
                    st.colors.removeAll(hex);
                }
                m_filterModel->setState(st);
            });
        }

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 3. 颜色标记 ────────────
    {
        QVBoxLayout* gl = nullptr;
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("颜色标记", gl, &hdrLayout);

        ColorLabelGroup::populate(g, gl, m_filterModel, m_colorCounts, currentSt,
            [this](QVBoxLayout* layout, const QString& label, int count, const QColor& color) {
                return addFilterRow(layout, label, count, color);
            });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 运行 CMake 配置并编译项目。
2. 验证“颜色标记”分组下各个颜色块选项的渲染与数量计算。
3. 测试多选勾选追加 `name`，取消勾选同时移除 `name` 和 `hex`，及 `FilterPanel::selectColor()` 的UI选择状态同步。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 恪守现有 SSOT 规范，状态修改统一经由 `FilterStateModel::setState()` 驱动；
- `addFilterRow` 闭包显式包含四参数 `(layout, label, count, color)` 签名传递给 `FilterPanel`。

## 6. Header API Signature Verification
- `Style::getColorPalette()`: `static const QList<ColorItem>& getColorPalette()`
- `FilterStateModel::state()`: `FilterState state() const`
- `FilterStateModel::setState(const FilterState&)`: `void setState(const FilterState& state)`
- `FilterPanel::addFilterRow(...)`: `QCheckBox* addFilterRow(QVBoxLayout* layout, const QString& label, int count, const QColor& color = QColor())`

## 7. Header Inclusion Chain & Type Completeness Check
- `ColorLabelGroup.h` 包含了 `<QWidget>`, `<QVBoxLayout>`, `<QCheckBox>`, `<QMap>`, `<QString>`, `<QColor>`, `<functional>`, `"FilterStateModel.h"`，类型完整。
- `ColorLabelGroup.cpp` 包含了 `"ColorLabelGroup.h"`, `"StyleLibrary.h"`，保证 `Style::getColorPalette()` 完整定义可直接访问。
