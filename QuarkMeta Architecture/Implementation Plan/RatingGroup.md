# RatingGroup Refactoring Plan

## 1. Overview
把 `FilterPanel.cpp` 中 `rebuildGroups()` 里的“评级”分组以及全局仅在此调用的 `static QString ratingDisplayName(int r)` 函数，原样搬到独立的新类 `RatingGroup` 中。只做搬迁，不做重写、不做优化、不做重新设计。新类不持有 `FilterPanel` 指针，不包含 `FilterPanel.h`。

## 2. Modified Files List
- **新增**：`src/ui/RatingGroup.h`
- **新增**：`src/ui/RatingGroup.cpp`
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
    src/ui/RatingGroup.h
    src/ui/RatingGroup.cpp
>>>>>>> REPLACE
```

### `src/ui/RatingGroup.h` (New File)
```cpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QMap>
#include <functional>
#include "FilterStateModel.h"

namespace QuarkMeta {

class RatingGroup {
public:
    using AddFilterRowFunc = std::function<QCheckBox*(QVBoxLayout* layout, const QString& label, int count)>;

    static void populate(QWidget* parentWidget,
                         QVBoxLayout* contentLayout,
                         FilterStateModel* filterModel,
                         const QMap<int, int>& ratingCounts,
                         const FilterState& currentState,
                         AddFilterRowFunc addFilterRow);
};

} // namespace QuarkMeta
```

### `src/ui/RatingGroup.cpp` (New File)
```cpp
#include "RatingGroup.h"
#include "components/ClickableRow.h"
#include "UiHelper.h"
#include <QLabel>
#include <QPainter>
#include <QPixmap>

namespace QuarkMeta {

static QString ratingDisplayName(int r) {
    return r == 0 ? "无评级" : QString("★").repeated(r);
}

void RatingGroup::populate(QWidget* parentWidget,
                           QVBoxLayout* contentLayout,
                           FilterStateModel* filterModel,
                           const QMap<int, int>& ratingCounts,
                           const FilterState& currentState,
                           AddFilterRowFunc addFilterRow) {
    if (!contentLayout || !filterModel || !addFilterRow) return;

    QWidget* contextObj = parentWidget ? parentWidget : contentLayout->parentWidget();

    for (int r : {0, 1, 2, 3, 4, 5}) {
        int cnt = ratingCounts.value(r, 0);
        bool isChecked = currentState.ratings.contains(r);
        if (cnt <= 0 && !isChecked) continue;

        QCheckBox* cb = addFilterRow(contentLayout, ratingDisplayName(r), cnt);
        cb->blockSignals(true);
        cb->setChecked(isChecked);
        cb->blockSignals(false);

        ClickableRow* row = qobject_cast<ClickableRow*>(cb->parentWidget());
        if (row) {
            row->setProperty("ratingValue", r);
            if (r > 0) {
                QLabel* lbl = row->findChild<QLabel*>("FilterItemLabel");
                if (lbl) {
                    int starSize = 12;
                    int spacing = 2;
                    int totalW = r * starSize + (r - 1) * spacing;
                    QPixmap pix(totalW, starSize);
                    pix.fill(Qt::transparent);
                    QPainter painter(&pix);
                    QPixmap starPix = UiHelper::getIcon("star_filled", QColor("#CCCCCC"), starSize).pixmap(starSize, starSize);
                    for (int i = 0; i < r; ++i) {
                        painter.drawPixmap(i * (starSize + spacing), 0, starPix);
                    }
                    painter.end();
                    lbl->setPixmap(pix);
                }
            }
        }

        QObject::connect(cb, &QCheckBox::toggled, contextObj, [filterModel, r](bool on) {
            FilterState st = filterModel->state();
            if (on) { if (!st.ratings.contains(r)) st.ratings.append(r); }
            else st.ratings.removeAll(r);
            filterModel->setState(st);
        });
    }
}

} // namespace QuarkMeta
```

### `src/ui/FilterPanel.cpp`
```diff
<<<<<<< SEARCH
static QString ratingDisplayName(int r) {
    return r == 0 ? "无评级" : QString("★").repeated(r);
}
=======
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
#include "FilterPanel.h"
=======
#include "FilterPanel.h"
#include "RatingGroup.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 2. 评级 ────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("评级", gl);
        for (int r : {0, 1, 2, 3, 4, 5}) {
            int cnt = m_ratingCounts.value(r, 0);
            bool isChecked = currentSt.ratings.contains(r);
            if (cnt <= 0 && !isChecked) continue;

            QCheckBox* cb = addFilterRow(gl, ratingDisplayName(r), cnt);
            cb->blockSignals(true);
            cb->setChecked(isChecked);
            cb->blockSignals(false);

            ClickableRow* row = qobject_cast<ClickableRow*>(cb->parentWidget());
            if (row) {
                row->setProperty("ratingValue", r);
                if (r > 0) {
                    QLabel* lbl = row->findChild<QLabel*>("FilterItemLabel");
                    if (lbl) {
                        int starSize = 12;
                        int spacing = 2;
                        int totalW = r * starSize + (r - 1) * spacing;
                        QPixmap pix(totalW, starSize);
                        pix.fill(Qt::transparent);
                        QPainter painter(&pix);
                        QPixmap starPix = UiHelper::getIcon("star_filled", QColor("#CCCCCC"), starSize).pixmap(starSize, starSize);
                        for (int i = 0; i < r; ++i) {
                            painter.drawPixmap(i * (starSize + spacing), 0, starPix);
                        }
                        painter.end();
                        lbl->setPixmap(pix);
                    }
                }
            }

            connect(cb, &QCheckBox::toggled, this, [this, r](bool on) {
                FilterState st = m_filterModel->state();
                if (on) { if (!st.ratings.contains(r)) st.ratings.append(r); }
                else st.ratings.removeAll(r);
                m_filterModel->setState(st);
            });
        }

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 2. 评级 ────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("评级", gl);

        RatingGroup::populate(g, gl, m_filterModel, m_ratingCounts, currentSt,
            [this](QVBoxLayout* layout, const QString& label, int count) {
                return addFilterRow(layout, label, count);
            });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 运行 CMake 配置并编译项目。
2. 验证“评级”分组下 0~5 星各选项的显示，核对 1~5 星绘制星星图与 0 星“无评级”文字。
3. 测试勾选/取消勾选、多选叠加及 `FilterPanel::syncUIFromFilterState()` 根据 `ratingValue` 识别评级行选中状态。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 恪守现有 SSOT 规范，状态修改统一经由 `FilterStateModel::setState()` 驱动；
- `addFilterRow` 闭包回调 `FilterPanel` 现有接口，且 `ratingDisplayName` 函数完整移至 `RatingGroup.cpp`。

## 6. Header API Signature Verification
- `FilterStateModel::state()`: `FilterState state() const`
- `FilterStateModel::setState(const FilterState&)`: `void setState(const FilterState& state)`
- `UiHelper::getIcon(...)`: `static QIcon getIcon(const QString& name, const QColor& color, int size)`

## 7. Header Inclusion Chain & Type Completeness Check
- `RatingGroup.h` 包含了 `<QWidget>`, `<QVBoxLayout>`, `<QCheckBox>`, `<QMap>`, `<functional>`, `"FilterStateModel.h"`，类型完整。
- `RatingGroup.cpp` 包含了 `"RatingGroup.h"`, `"components/ClickableRow.h"`, `"UiHelper.h"`, `<QLabel>`, `<QPainter>`, `<QPixmap>`，无隐式类型断裂。
