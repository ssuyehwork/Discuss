# FileTypeGroup Refactoring Plan

## 1. Overview
把 `FilterPanel.cpp` 中 `rebuildGroups()` 里的“文件类型”分组，原样搬到独立的新类 `FileTypeGroup` 中。只做搬迁，不做重写、不做优化、不做重新设计。新类不持有 `FilterPanel` 指针，不包含 `FilterPanel.h`。

## 2. Modified Files List
- **新增**：`src/ui/FileTypeGroup.h`
- **新增**：`src/ui/FileTypeGroup.cpp`
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
    src/ui/FileTypeGroup.h
    src/ui/FileTypeGroup.cpp
>>>>>>> REPLACE
```

### `src/ui/FileTypeGroup.h` (New File)
```cpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QMap>
#include <QStringList>
#include <functional>
#include "FilterStateModel.h"

namespace QuarkMeta {

class FileTypeGroup {
public:
    using AddFilterRowFunc = std::function<QCheckBox*(QVBoxLayout* layout, const QString& label, int count)>;
    using SaveFilterHistoryFunc = std::function<void(const QString& key, const QString& text)>;

    static QLineEdit* populate(QObject* owner,
                               QWidget* groupWidget,
                               QVBoxLayout* contentLayout,
                               FilterStateModel* filterModel,
                               const FilterState& currentState,
                               int emptyFolderCount,
                               const QMap<QString, int>& typeCounts,
                               AddFilterRowFunc addFilterRow,
                               SaveFilterHistoryFunc saveHistory);
};

} // namespace QuarkMeta
```

### `src/ui/FileTypeGroup.cpp` (New File)
```cpp
#include "FileTypeGroup.h"
#include <QHBoxLayout>

namespace QuarkMeta {

QLineEdit* FileTypeGroup::populate(QObject* owner,
                                   QWidget* groupWidget,
                                   QVBoxLayout* contentLayout,
                                   FilterStateModel* filterModel,
                                   const FilterState& currentState,
                                   int emptyFolderCount,
                                   const QMap<QString, int>& typeCounts,
                                   AddFilterRowFunc addFilterRow,
                                   SaveFilterHistoryFunc saveHistory) {
    if (!contentLayout || !filterModel || !addFilterRow) return nullptr;

    QWidget* wType = new QWidget(groupWidget);
    QHBoxLayout* lType = new QHBoxLayout(wType);
    lType->setContentsMargins(5, 6, 5, 4);
    lType->setSpacing(0);

    QLineEdit* editType = new QLineEdit(wType);
    editType->setClearButtonEnabled(true);
    editType->setPlaceholderText("例： png / 文件夹...");
    editType->setText(currentState.typeFilterText);
    editType->setObjectName("FilterSearchEdit");
    editType->setFixedHeight(22);
    if (owner) editType->installEventFilter(owner);

    QObject::connect(editType, &QLineEdit::returnPressed, owner, [filterModel, editType, saveHistory]() {
        FilterState st = filterModel->state();
        st.typeFilterText = editType->text();
        if (saveHistory) saveHistory("Type", st.typeFilterText);
        filterModel->setState(st);
    });

    QObject::connect(editType, &QLineEdit::textChanged, owner, [filterModel](const QString& text) {
        FilterState st = filterModel->state();
        if (text.isEmpty() && !st.typeFilterText.isEmpty()) {
            st.typeFilterText = "";
            filterModel->setState(st);
        }
    });

    lType->addWidget(editType);
    contentLayout->addWidget(wType);

    if (emptyFolderCount > 0) {
        QCheckBox* cb = addFilterRow(contentLayout, "空文件夹", emptyFolderCount);
        cb->blockSignals(true);
        cb->setChecked(currentState.types.contains("空文件夹"));
        cb->blockSignals(false);
        QObject::connect(cb, &QCheckBox::toggled, owner, [filterModel](bool on) {
            FilterState st = filterModel->state();
            if (on) { if (!st.types.contains("空文件夹")) st.types.append("空文件夹"); }
            else    st.types.removeAll("空文件夹");
            filterModel->setState(st);
        });
    }

    if (typeCounts.contains("folder") && typeCounts["folder"] > 0) {
        QCheckBox* cb = addFilterRow(contentLayout, "文件夹", typeCounts["folder"]);
        cb->blockSignals(true);
        cb->setChecked(currentState.types.contains("folder"));
        cb->blockSignals(false);
        QObject::connect(cb, &QCheckBox::toggled, owner, [filterModel](bool on) {
            FilterState st = filterModel->state();
            if (on) { if (!st.types.contains("folder")) st.types.append("folder"); }
            else    st.types.removeAll("folder");
            filterModel->setState(st);
        });
    }

    if (typeCounts.contains("file") && typeCounts["file"] > 0) {
        QCheckBox* cb = addFilterRow(contentLayout, "文件", typeCounts["file"]);
        cb->blockSignals(true);
        cb->setChecked(currentState.types.contains("file"));
        cb->blockSignals(false);
        QObject::connect(cb, &QCheckBox::toggled, owner, [filterModel](bool on) {
            FilterState st = filterModel->state();
            if (on) { if (!st.types.contains("file")) st.types.append("file"); }
            else    st.types.removeAll("file");
            filterModel->setState(st);
        });
    }

    QStringList exts = typeCounts.keys(); exts.sort();
    for (const QString& ext : exts) {
        if (ext == "folder" || ext == "file" || ext == "空文件夹" || typeCounts[ext] <= 0) continue;
        QString label = ext.isEmpty() ? "无扩展名" : ext;
        QCheckBox* cb = addFilterRow(contentLayout, label, typeCounts[ext]);
        cb->blockSignals(true);
        cb->setChecked(currentState.types.contains(ext));
        cb->blockSignals(false);
        QObject::connect(cb, &QCheckBox::toggled, owner, [filterModel, ext](bool on) {
            FilterState st = filterModel->state();
            if (on) { if (!st.types.contains(ext)) st.types.append(ext); }
            else st.types.removeAll(ext);
            filterModel->setState(st);
        });
    }

    return editType;
}

} // namespace QuarkMeta
```

### `src/ui/FilterPanel.cpp`
```diff
<<<<<<< SEARCH
#include "FilterPanel.h"
=======
#include "FilterPanel.h"
#include "FileTypeGroup.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 4. 文件类型 ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("文件类型", gl);

        QWidget* wType = new QWidget(g);
        QHBoxLayout* lType = new QHBoxLayout(wType);
        lType->setContentsMargins(5, 6, 5, 4);
        lType->setSpacing(0);

        m_editType = new QLineEdit(wType);
        m_editType->setClearButtonEnabled(true);
        m_editType->setPlaceholderText("例： png / 文件夹...");
        m_editType->setText(currentSt.typeFilterText);
        m_editType->setObjectName("FilterSearchEdit");
        m_editType->setFixedHeight(22);
        m_editType->installEventFilter(this);
        connect(m_editType, &QLineEdit::returnPressed, this, [this]() {
            FilterState st = m_filterModel->state();
            st.typeFilterText = m_editType->text();
            saveFilterHistory("Type", st.typeFilterText);
            m_filterModel->setState(st);
        });
        connect(m_editType, &QLineEdit::textChanged, this, [this](const QString& text) {
            FilterState st = m_filterModel->state();
            if (text.isEmpty() && !st.typeFilterText.isEmpty()) {
                st.typeFilterText = "";
                m_filterModel->setState(st);
            }
        });
        lType->addWidget(m_editType);
        gl->addWidget(wType);

        if (m_emptyFolderCount > 0) {
            QCheckBox* cb = addFilterRow(gl, "空文件夹", m_emptyFolderCount);
            cb->blockSignals(true);
            cb->setChecked(currentSt.types.contains("空文件夹"));
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this](bool on) {
                FilterState st = m_filterModel->state();
                if (on) { if (!st.types.contains("空文件夹")) st.types.append("空文件夹"); }
                else    st.types.removeAll("空文件夹");
                m_filterModel->setState(st);
            });
        }

        if (m_typeCounts.contains("folder") && m_typeCounts["folder"] > 0) {
            QCheckBox* cb = addFilterRow(gl, "文件夹", m_typeCounts["folder"]);
            cb->blockSignals(true);
            cb->setChecked(currentSt.types.contains("folder"));
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this](bool on) {
                FilterState st = m_filterModel->state();
                if (on) { if (!st.types.contains("folder")) st.types.append("folder"); }
                else    st.types.removeAll("folder");
                m_filterModel->setState(st);
            });
        }
        if (m_typeCounts.contains("file") && m_typeCounts["file"] > 0) {
            QCheckBox* cb = addFilterRow(gl, "文件", m_typeCounts["file"]);
            cb->blockSignals(true);
            cb->setChecked(currentSt.types.contains("file"));
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this](bool on) {
                FilterState st = m_filterModel->state();
                if (on) { if (!st.types.contains("file")) st.types.append("file"); }
                else    st.types.removeAll("file");
                m_filterModel->setState(st);
            });
        }
        QStringList exts = m_typeCounts.keys(); exts.sort();
        for (const QString& ext : exts) {
            if (ext == "folder" || ext == "file" || ext == "空文件夹" || m_typeCounts[ext] <= 0) continue;
            QString label = ext.isEmpty() ? "无扩展名" : ext;
            QCheckBox* cb = addFilterRow(gl, label, m_typeCounts[ext]);
            cb->blockSignals(true);
            cb->setChecked(currentSt.types.contains(ext));
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this, ext](bool on) {
                FilterState st = m_filterModel->state();
                if (on) { if (!st.types.contains(ext)) st.types.append(ext); }
                else st.types.removeAll(ext);
                m_filterModel->setState(st);
            });
        }
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 4. 文件类型 ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("文件类型", gl);

        m_editType = FileTypeGroup::populate(this, g, gl, m_filterModel, currentSt,
            m_emptyFolderCount, m_typeCounts,
            [this](QVBoxLayout* layout, const QString& label, int count) {
                return addFilterRow(layout, label, count);
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
2. 验证“文件类型”分组下搜索输入框与“空文件夹”、“文件夹”、“文件”及特定扩展名列表的渲染。
3. 测试输入框回车提交/历史记录触发/清空逻辑以及复选框切换时状态读写逻辑。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 恪守现有 SSOT 规范，状态修改统一经由 `FilterStateModel::setState()` 驱动；
- 输入框返回指针直接赋给 `m_editType`，保持 `FilterPanel::eventFilter()` 对双击输入框历史记录绑定的识别不变。

## 6. Header API Signature Verification
- `FilterStateModel::state()`: `FilterState state() const`
- `FilterStateModel::setState(const FilterState&)`: `void setState(const FilterState& state)`
- `FilterPanel::saveFilterHistory(...)`: `void saveFilterHistory(const QString& key, const QString& text)`

## 7. Header Inclusion Chain & Type Completeness Check
- `FileTypeGroup.h` 包含了 `<QWidget>`, `<QVBoxLayout>`, `<QLineEdit>`, `<QCheckBox>`, `<QMap>`, `<QStringList>`, `<functional>`, `"FilterStateModel.h"`，类型完整。
- `FileTypeGroup.cpp` 包含了 `"FileTypeGroup.h"`, `<QHBoxLayout>`，无隐式类型断裂。
