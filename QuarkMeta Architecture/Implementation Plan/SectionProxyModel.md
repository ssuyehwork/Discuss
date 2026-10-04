# Implementation Plan - Step 1: SectionProxyModel Implementation

This implementation plan defines the addition of `SectionProxyModel`, a custom `QAbstractProxyModel` that wraps `FilterProxyModel` and injects section headers (Folder Header & File Header) into a unified single model stream.

## 1. Overview
The `SectionProxyModel` transforms a flat source model (`FilterProxyModel`) into a sectioned structure:
- `[Folder Header Row]` (present if folder count > 0, text: "文件夹 (N)")
- `[Folder Items...]`
- `[File Header Row]` (present if file count > 0 and folder count > 0, text: "文件 (M)")
- `[File Items...]`

Key rules:
- Header rows are non-selectable, non-editable, non-draggable, non-drop-target (`flags` excludes selection/edit/drag/drop).
- Header row `mapToSource` returns an invalid `QModelIndex()`.
- Added roles in `ModelContract.h`:
  - `SectionHeaderRole` (`Qt::UserRole + 212`): `true` for headers, `false` for normal items.
  - `SectionHeaderTextRole` (`Qt::UserRole + 213`): Header text string.
  - `SectionCollapsedRole` (`Qt::UserRole + 214`): Folder collapse state (`true`/`false`).
- Collapse state is stored internally (`setFolderCollapsed(bool)` / `isFolderCollapsed()`). When folder section is collapsed, folder rows are removed from index mapping while the folder header remains.
- Helper methods `folderCount()` and `fileCount()` return source counts (unaffected by collapse) for status bar statistics.
- Signal forwarding: `dataChanged` is mapped 1:1. Row insertions, removals, layout changes, and model resets trigger a mapping rebuild.

---

## 2. Modified Files List
1. `src/core/ModelContract.h` (Add section roles)
2. `src/ui/models/SectionProxyModel.h` (New header)
3. `src/ui/models/SectionProxyModel.cpp` (New source)
4. `CMakeLists.txt` (Register `SectionProxyModel.h` and `SectionProxyModel.cpp`)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/core/ModelContract.h`
<<<<<<< SEARCH
    // 列视图与拖放交互角色 (UserRole + 210..220)
    IsParentExpandedRole = Qt::UserRole + 210, // 列视图父目录展开高亮
    IsDropTargetRole     = Qt::UserRole + 211, // 拖拽目标悬停高亮
=======
    // 列视图与拖放交互角色 (UserRole + 210..220)
    IsParentExpandedRole = Qt::UserRole + 210, // 列视图父目录展开高亮
    IsDropTargetRole     = Qt::UserRole + 211, // 拖拽目标悬停高亮
    SectionHeaderRole    = Qt::UserRole + 212, // 是否为分区标头行
    SectionHeaderTextRole = Qt::UserRole + 213,// 分区标头显示文本
    SectionCollapsedRole = Qt::UserRole + 214, // 文件夹分区折叠状态
>>>>>>> REPLACE

---

### 3.2 `src/ui/models/SectionProxyModel.h`
```cpp
#pragma once

#include <QAbstractProxyModel>
#include <QVector>
#include <QString>

namespace QuarkMeta {

class SectionProxyModel : public QAbstractProxyModel {
    Q_OBJECT

public:
    enum class RowType {
        FolderHeader,
        FolderItem,
        FileHeader,
        FileItem
    };

    struct MappingEntry {
        RowType type;
        int sourceRow; // -1 for headers, >= 0 for items
    };

    explicit SectionProxyModel(QObject* parent = nullptr);
    ~SectionProxyModel() override = default;

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    // Folder collapse control
    void setFolderCollapsed(bool collapsed);
    bool isFolderCollapsed() const { return m_folderCollapsed; }

    // Statistics
    int folderCount() const { return m_folderCount; }
    int fileCount() const { return m_fileCount; }

private slots:
    void onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void onSourceRowsInserted(const QModelIndex& parent, int start, int end);
    void onSourceRowsRemoved(const QModelIndex& parent, int start, int end);
    void onSourceModelReset();
    void onSourceLayoutAboutToBeChanged();
    void onSourceLayoutChanged();

private:
    void rebuildMapping();
    void updateCounts();

    QVector<MappingEntry> m_mapping;
    bool m_folderCollapsed = false;
    int m_folderCount = 0;
    int m_fileCount = 0;
    int m_firstFileSourceRow = -1;
};

} // namespace QuarkMeta
```

---

### 3.3 `src/ui/models/SectionProxyModel.cpp`
```cpp
#include "SectionProxyModel.h"
#include "../../core/ModelContract.h"

namespace QuarkMeta {

SectionProxyModel::SectionProxyModel(QObject* parent)
    : QAbstractProxyModel(parent) {
}

void SectionProxyModel::setSourceModel(QAbstractItemModel* newSourceModel) {
    if (sourceModel() == newSourceModel) return;

    if (sourceModel()) {
        disconnect(sourceModel(), &QAbstractItemModel::dataChanged, this, &SectionProxyModel::onSourceDataChanged);
        disconnect(sourceModel(), &QAbstractItemModel::rowsInserted, this, &SectionProxyModel::onSourceRowsInserted);
        disconnect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, &SectionProxyModel::onSourceRowsRemoved);
        disconnect(sourceModel(), &QAbstractItemModel::modelReset, this, &SectionProxyModel::onSourceModelReset);
        disconnect(sourceModel(), &QAbstractItemModel::layoutAboutToBeChanged, this, &SectionProxyModel::onSourceLayoutAboutToBeChanged);
        disconnect(sourceModel(), &QAbstractItemModel::layoutChanged, this, &SectionProxyModel::onSourceLayoutChanged);
    }

    QAbstractProxyModel::setSourceModel(newSourceModel);

    if (sourceModel()) {
        connect(sourceModel(), &QAbstractItemModel::dataChanged, this, &SectionProxyModel::onSourceDataChanged);
        connect(sourceModel(), &QAbstractItemModel::rowsInserted, this, &SectionProxyModel::onSourceRowsInserted);
        connect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, &SectionProxyModel::onSourceRowsRemoved);
        connect(sourceModel(), &QAbstractItemModel::modelReset, this, &SectionProxyModel::onSourceModelReset);
        connect(sourceModel(), &QAbstractItemModel::layoutAboutToBeChanged, this, &SectionProxyModel::onSourceLayoutAboutToBeChanged);
        connect(sourceModel(), &QAbstractItemModel::layoutChanged, this, &SectionProxyModel::onSourceLayoutChanged);
    }

    rebuildMapping();
}

void SectionProxyModel::rebuildMapping() {
    beginResetModel();
    m_mapping.clear();
    updateCounts();

    if (!sourceModel() || (m_folderCount == 0 && m_fileCount == 0)) {
        endResetModel();
        return;
    }

    int srcRows = sourceModel()->rowCount();

    // 1. Folder Header
    if (m_folderCount > 0) {
        m_mapping.append({RowType::FolderHeader, -1});
        if (!m_folderCollapsed) {
            for (int r = 0; r < m_folderCount; ++r) {
                m_mapping.append({RowType::FolderItem, r});
            }
        }
    }

    // 2. File Header & File Items
    if (m_fileCount > 0) {
        if (m_folderCount > 0) {
            m_mapping.append({RowType::FileHeader, -1});
        }
        int startFileRow = (m_firstFileSourceRow != -1) ? m_firstFileSourceRow : m_folderCount;
        for (int r = startFileRow; r < srcRows; ++r) {
            m_mapping.append({RowType::FileItem, r});
        }
    }

    endResetModel();
}

void SectionProxyModel::updateCounts() {
    m_folderCount = 0;
    m_fileCount = 0;
    m_firstFileSourceRow = -1;

    if (!sourceModel()) return;

    int total = sourceModel()->rowCount();
    for (int i = 0; i < total; ++i) {
        QModelIndex srcIdx = sourceModel()->index(i, 0);
        QString typeStr = srcIdx.data(TypeRole).toString();
        if (typeStr == "folder") {
            m_folderCount++;
        } else {
            if (m_firstFileSourceRow == -1) {
                m_firstFileSourceRow = i;
            }
            m_fileCount++;
        }
    }
}

QModelIndex SectionProxyModel::mapToSource(const QModelIndex& proxyIndex) const {
    if (!proxyIndex.isValid() || proxyIndex.row() < 0 || proxyIndex.row() >= m_mapping.size()) {
        return QModelIndex();
    }

    const auto& entry = m_mapping.at(proxyIndex.row());
    if (entry.sourceRow < 0 || !sourceModel()) {
        return QModelIndex();
    }

    return sourceModel()->index(entry.sourceRow, proxyIndex.column());
}

QModelIndex SectionProxyModel::mapFromSource(const QModelIndex& sourceIndex) const {
    if (!sourceIndex.isValid() || !sourceModel()) {
        return QModelIndex();
    }

    int srcRow = sourceIndex.row();
    for (int pRow = 0; pRow < m_mapping.size(); ++pRow) {
        if (m_mapping.at(pRow).sourceRow == srcRow) {
            return createIndex(pRow, sourceIndex.column());
        }
    }

    return QModelIndex();
}

QModelIndex SectionProxyModel::index(int row, int column, const QModelIndex& parent) const {
    if (parent.isValid() || row < 0 || row >= m_mapping.size() || column < 0 || column >= columnCount()) {
        return QModelIndex();
    }
    return createIndex(row, column);
}

QModelIndex SectionProxyModel::parent(const QModelIndex& child) const {
    Q_UNUSED(child);
    return QModelIndex();
}

int SectionProxyModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_mapping.size();
}

int SectionProxyModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return sourceModel() ? sourceModel()->columnCount() : 1;
}

QVariant SectionProxyModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_mapping.size()) {
        return QVariant();
    }

    const auto& entry = m_mapping.at(index.row());

    if (role == SectionHeaderRole) {
        return (entry.type == RowType::FolderHeader || entry.type == RowType::FileHeader);
    }

    if (role == SectionHeaderTextRole) {
        if (entry.type == RowType::FolderHeader) {
            return QString("文件夹 (%1)").arg(m_folderCount);
        }
        if (entry.type == RowType::FileHeader) {
            return QString("文件 (%1)").arg(m_fileCount);
        }
        return QVariant();
    }

    if (role == SectionCollapsedRole) {
        if (entry.type == RowType::FolderHeader) {
            return m_folderCollapsed;
        }
        return false;
    }

    if (entry.sourceRow < 0) {
        return QVariant();
    }

    QModelIndex srcIdx = mapToSource(index);
    return srcIdx.isValid() ? srcIdx.data(role) : QVariant();
}

bool SectionProxyModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_mapping.size()) {
        return false;
    }

    const auto& entry = m_mapping.at(index.row());
    if (entry.sourceRow < 0) {
        return false; // Headers cannot accept setData
    }

    QModelIndex srcIdx = mapToSource(index);
    return srcIdx.isValid() ? sourceModel()->setData(srcIdx, value, role) : false;
}

Qt::ItemFlags SectionProxyModel::flags(const QModelIndex& index) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_mapping.size()) {
        return Qt::NoItemFlags;
    }

    const auto& entry = m_mapping.at(index.row());
    if (entry.type == RowType::FolderHeader || entry.type == RowType::FileHeader) {
        return Qt::ItemIsEnabled; // Not selectable, editable, draggable, or drop target
    }

    QModelIndex srcIdx = mapToSource(index);
    return srcIdx.isValid() ? sourceModel()->flags(srcIdx) : Qt::NoItemFlags;
}

void SectionProxyModel::sort(int column, Qt::SortOrder order) {
    if (sourceModel()) {
        sourceModel()->sort(column, order);
    }
}

void SectionProxyModel::setFolderCollapsed(bool collapsed) {
    if (m_folderCollapsed == collapsed) return;
    m_folderCollapsed = collapsed;
    rebuildMapping();
}

void SectionProxyModel::onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
    if (!topLeft.isValid() || !bottomRight.isValid()) return;

    for (int r = topLeft.row(); r <= bottomRight.row(); ++r) {
        for (int c = topLeft.column(); c <= bottomRight.column(); ++c) {
            QModelIndex srcIdx = sourceModel()->index(r, c);
            QModelIndex proxyIdx = mapFromSource(srcIdx);
            if (proxyIdx.isValid()) {
                emit dataChanged(proxyIdx, proxyIdx, roles);
            }
        }
    }
}

void SectionProxyModel::onSourceRowsInserted(const QModelIndex& parent, int start, int end) {
    Q_UNUSED(parent); Q_UNUSED(start); Q_UNUSED(end);
    rebuildMapping();
}

void SectionProxyModel::onSourceRowsRemoved(const QModelIndex& parent, int start, int end) {
    Q_UNUSED(parent); Q_UNUSED(start); Q_UNUSED(end);
    rebuildMapping();
}

void SectionProxyModel::onSourceModelReset() {
    rebuildMapping();
}

void SectionProxyModel::onSourceLayoutAboutToBeChanged() {
    emit layoutAboutToBeChanged();
}

void SectionProxyModel::onSourceLayoutChanged() {
    rebuildMapping();
    emit layoutChanged();
}

} // namespace QuarkMeta
```

---

### 3.4 `CMakeLists.txt`
<<<<<<< SEARCH
    src/ui/models/FilterProxyModel.h
    src/ui/models/FilterProxyModel.cpp
=======
    src/ui/models/FilterProxyModel.h
    src/ui/models/FilterProxyModel.cpp
    src/ui/models/SectionProxyModel.h
    src/ui/models/SectionProxyModel.cpp
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Create `SectionProxyModel.h` and `SectionProxyModel.cpp` in `src/ui/models/`.
2. Update `ModelContract.h` and `CMakeLists.txt`.
3. Compile using CMake / MSVC.
4. Verify no compilation errors or missing symbols.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `ModelContract.h` for role definition (`SectionHeaderRole`, `SectionHeaderTextRole`, `SectionCollapsedRole`).
- Reuses standard Qt proxy model mechanisms without creating duplicate state stores.

---

## 6. Header API Signature Verification
- `QAbstractProxyModel::setSourceModel(QAbstractItemModel*)`
- `QAbstractProxyModel::mapToSource(const QModelIndex&) const`
- `QAbstractProxyModel::mapFromSource(const QModelIndex&) const`
- `QAbstractProxyModel::index(int, int, const QModelIndex&) const`
- `QAbstractProxyModel::parent(const QModelIndex&) const`
- `QAbstractProxyModel::rowCount(const QModelIndex&) const`
- `QAbstractProxyModel::columnCount(const QModelIndex&) const`
- `QAbstractProxyModel::data(const QModelIndex&, int) const`
- `QAbstractProxyModel::setData(const QModelIndex&, const QVariant&, int)`
- `QAbstractProxyModel::flags(const QModelIndex&) const`
- `QAbstractProxyModel::sort(int, Qt::SortOrder)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `SectionProxyModel.h` includes `<QAbstractProxyModel>`, `<QVector>`, `<QString>`.
- `SectionProxyModel.cpp` includes `"SectionProxyModel.h"` and `"../../core/ModelContract.h"`.
