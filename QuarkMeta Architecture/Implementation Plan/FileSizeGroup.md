# FileSizeGroup Refactoring Plan

## 1. Overview
把 `FilterPanel.cpp` 中 `rebuildGroups()` 里的“文件大小”分组，原样搬到独立的新类 `FileSizeGroup` 中。只做搬迁，不做重写、不做优化、不做重新设计。新类不持有 `FilterPanel` 指针，不包含 `FilterPanel.h`。

## 2. Modified Files List
- **新增**：`src/ui/FileSizeGroup.h`
- **新增**：`src/ui/FileSizeGroup.cpp`
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
    src/ui/FileSizeGroup.h
    src/ui/FileSizeGroup.cpp
>>>>>>> REPLACE
```

### `src/ui/FileSizeGroup.h` (New File)
```cpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include "FilterStateModel.h"

namespace QuarkMeta {

class FileSizeGroup {
public:
    static void populate(QWidget* parentWidget,
                         QVBoxLayout* contentLayout,
                         FilterStateModel* filterModel);
};

} // namespace QuarkMeta
```

### `src/ui/FileSizeGroup.cpp` (New File)
```cpp
#include "FileSizeGroup.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>

namespace QuarkMeta {

void FileSizeGroup::populate(QWidget* parentWidget,
                             QVBoxLayout* contentLayout,
                             FilterStateModel* filterModel) {
    if (!contentLayout || !filterModel) return;

    QWidget* contextObj = parentWidget ? parentWidget : contentLayout->parentWidget();

    QHBoxLayout* hs = new QHBoxLayout();
    hs->setContentsMargins(5, 4, 5, 8);
    hs->setSpacing(8);

    QLineEdit* minEdit = new QLineEdit(parentWidget);
    minEdit->setClearButtonEnabled(true);
    QLineEdit* maxEdit = new QLineEdit(parentWidget);
    maxEdit->setClearButtonEnabled(true);
    QComboBox* unitCombo = new QComboBox(parentWidget);
    unitCombo->addItems({"KB", "MB", "GB"});
    unitCombo->setCurrentIndex(1);

    minEdit->setObjectName("FilterSizeEdit");
    maxEdit->setObjectName("FilterSizeEdit");
    unitCombo->setObjectName("FilterUnitCombo");
    minEdit->setPlaceholderText("最小");
    maxEdit->setPlaceholderText("最大");
    minEdit->setFixedHeight(24);
    maxEdit->setFixedHeight(24);

    unitCombo->setFixedHeight(24);
    unitCombo->setFixedWidth(52);

    hs->addWidget(minEdit);
    QLabel* sep = new QLabel("-", parentWidget); sep->setObjectName("FilterSepLabel"); hs->addWidget(sep);
    hs->addWidget(maxEdit);
    hs->addWidget(unitCombo);
    contentLayout->addLayout(hs);

    auto updateSizeFilter = [filterModel, minEdit, maxEdit, unitCombo]() {
        auto toBytes = [](const QString& txt, const QString& unit) -> long long {
            if (txt.isEmpty()) return -1;
            bool ok;
            double val = txt.toDouble(&ok);
            if (!ok) return -1;
            long long factor = 1024;
            if (unit == "MB") factor = 1024 * 1024;
            else if (unit == "GB") factor = 1024 * 1024 * 1024;
            return (long long)(val * factor);
        };
        FilterState st = filterModel->state();
        st.minSize = toBytes(minEdit->text(), unitCombo->currentText());
        st.maxSize = toBytes(maxEdit->text(), unitCombo->currentText());
        filterModel->setState(st);
    };

    QObject::connect(minEdit, &QLineEdit::editingFinished, contextObj, updateSizeFilter);
    QObject::connect(minEdit, &QLineEdit::textChanged, contextObj, [updateSizeFilter](const QString& text) {
        if (text.isEmpty()) updateSizeFilter();
    });
    QObject::connect(maxEdit, &QLineEdit::editingFinished, contextObj, updateSizeFilter);
    QObject::connect(maxEdit, &QLineEdit::textChanged, contextObj, [updateSizeFilter](const QString& text) {
        if (text.isEmpty()) updateSizeFilter();
    });
    QObject::connect(unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), contextObj, [updateSizeFilter](int){ updateSizeFilter(); });
}

} // namespace QuarkMeta
```

### `src/ui/FilterPanel.cpp`
```diff
<<<<<<< SEARCH
#include "FilterPanel.h"
=======
#include "FilterPanel.h"
#include "FileSizeGroup.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 9. 文件大小 ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("文件大小", gl);

        QHBoxLayout* hs = new QHBoxLayout();
        hs->setContentsMargins(5, 4, 5, 8);
        hs->setSpacing(8);

        QLineEdit* minEdit = new QLineEdit(g);
        minEdit->setClearButtonEnabled(true);
        QLineEdit* maxEdit = new QLineEdit(g);
        maxEdit->setClearButtonEnabled(true);
        QComboBox* unitCombo = new QComboBox(g);
        unitCombo->addItems({"KB", "MB", "GB"});
        unitCombo->setCurrentIndex(1);

        minEdit->setObjectName("FilterSizeEdit");
        maxEdit->setObjectName("FilterSizeEdit");
        unitCombo->setObjectName("FilterUnitCombo");
        minEdit->setPlaceholderText("最小");
        maxEdit->setPlaceholderText("最大");
        minEdit->setFixedHeight(24);
        maxEdit->setFixedHeight(24);

        unitCombo->setFixedHeight(24);
        unitCombo->setFixedWidth(52);

        hs->addWidget(minEdit);
        QLabel* sep = new QLabel("-", g); sep->setObjectName("FilterSepLabel"); hs->addWidget(sep);
        hs->addWidget(maxEdit);
        hs->addWidget(unitCombo);
        gl->addLayout(hs);

        auto updateSizeFilter = [this, minEdit, maxEdit, unitCombo]() {
            auto toBytes = [](const QString& txt, const QString& unit) -> long long {
                if (txt.isEmpty()) return -1;
                bool ok;
                double val = txt.toDouble(&ok);
                if (!ok) return -1;
                long long factor = 1024;
                if (unit == "MB") factor = 1024 * 1024;
                else if (unit == "GB") factor = 1024 * 1024 * 1024;
                return (long long)(val * factor);
            };
            FilterState st = m_filterModel->state();
            st.minSize = toBytes(minEdit->text(), unitCombo->currentText());
            st.maxSize = toBytes(maxEdit->text(), unitCombo->currentText());
            m_filterModel->setState(st);
        };

        connect(minEdit, &QLineEdit::editingFinished, this, updateSizeFilter);
        connect(minEdit, &QLineEdit::textChanged, this, [updateSizeFilter](const QString& text) {
            if (text.isEmpty()) updateSizeFilter();
        });
        connect(maxEdit, &QLineEdit::editingFinished, this, updateSizeFilter);
        connect(maxEdit, &QLineEdit::textChanged, this, [updateSizeFilter](const QString& text) {
            if (text.isEmpty()) updateSizeFilter();
        });
        connect(unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [updateSizeFilter](int){ updateSizeFilter(); });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 9. 文件大小 ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("文件大小", gl);

        FileSizeGroup::populate(g, gl, m_filterModel);

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 运行 CMake 配置并编译项目。
2. 验证“文件大小”分组下的输入框（“最小”、“最大”）、分隔标签与单位下拉框（KB/MB/GB，默认 MB）外观与尺寸属性。
3. 测试数值输入与单位切换时的 5 处触发刷新逻辑。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 恪守现有 SSOT 规范，状态修改统一经由 `FilterStateModel::setState()` 驱动；
- 参数精简传递，仅传入 `g`, `gl`, `m_filterModel`，不冗余传递 `ScanStats` 或 `FilterState`。

## 6. Header API Signature Verification
- `FilterStateModel::state()`: `FilterState state() const`
- `FilterStateModel::setState(const FilterState&)`: `void setState(const FilterState& state)`

## 7. Header Inclusion Chain & Type Completeness Check
- `FileSizeGroup.h` 包含了 `<QWidget>`, `<QVBoxLayout>`, `"FilterStateModel.h"`，类型完整。
- `FileSizeGroup.cpp` 包含了 `"FileSizeGroup.h"`, `<QHBoxLayout>`, `<QLineEdit>`, `<QComboBox>`, `<QLabel>`，无隐式类型断裂。
