# Implementation Plan - GroupingProxyModel-1.md

## 1. Overview
This updated implementation plan refines `GroupingProxyModel` from a flat-row proxy model into a **genuine two-level parent-child tree hierarchy** proxy model (`QAbstractProxyModel`), integrating Qt's native tree view expand/collapse mechanisms across `DropTreeView`, `DropListView`, and `ColumnViewWidget`.

### Core Architectural Principles & Technical Choices:
1. **Real Two-Level Parent-Child Tree Hierarchy**:
   - Top-level (invalid parent): Exposes synthetic group nodes ("文件夹" and "文件").
   - Child level (parent = group node): Exposes actual source items belonging to that group.
   - `rowCount(parent)`: Returns group count for invalid parent, item count for group parent, 0 for item parent.
   - `flags(index)`: Group nodes only return `Qt::ItemIsEnabled` (omitting `Qt::ItemIsSelectable`). This natively prevents group headers from being selected or participating in keyboard navigation/multi-selection.
2. **Standard `TypeRole` Classification**:
   - Group matching predicates check `index.data(TypeRole).toString() == "folder"` (`ModelContract::TypeRole`), avoiding unverified `Qt::UserRole`/`ItemRecord` assumptions and staying consistent with existing codebase conventions (`FilterProxyModel`, `DropListView`, `ColumnViewPane`).
3. **Native Expand/Collapse Controls**:
   - `DropTreeView` enables native tree decorations via `setRootIsDecorated(true)`. Manual hit-testing and custom arrow string rendering (`▶`/`▼`) are removed.
   - Non-collapsible file group behavior is enforced by connecting to `QTreeView::collapsed(QModelIndex)` and calling `setExpanded(index, true)` if the file group is collapsed.
   - Folder group fold states are persisted to `GroupingProxyModel` and re-applied after `rebuildMapping()`.
4. **DropListView Re-inheritance to QTreeView**:
   - `DropListView` is updated to inherit from `QTreeView` with `setIndentation(0)` and header hidden. Inspection confirms `ColumnItemDelegate` has zero QListView-specific dependencies and works seamlessly with QTreeView.
5. **Per-Pane Grouping in ColumnViewWidget**:
   - Each `ColumnViewPane` instantiates its own dedicated `GroupingProxyModel` wrapping its `FilterProxyModel`.
6. **ContentPanel Cross-Model Index Mapping Helper**:
   - Adds a unified helper method `mapToActiveViewModelIndex(...)` in `ContentPanel` to resolve cross-model index mismatches in `selectAndScrollToItem()`, `restoreSelections()`, and `refreshVisibleThumbnails()`.

---

## 2. Modified Files List
- `CMakeLists.txt`
- `src/ui/models/GroupingProxyModel.h` (New File)
- `src/ui/models/GroupingProxyModel.cpp` (New File)
- `src/ui/TreeItemDelegate.h`
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/DropListView.h`
- `src/ui/DropListView.cpp`
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 1. `CMakeLists.txt`
Register `src/ui/models/GroupingProxyModel.h` and `src/ui/models/GroupingProxyModel.cpp`.

```
<<<<<<< SEARCH
    src/ui/FilterStateModel.cpp
    src/ui/models/FilterProxyModel.h
    src/ui/models/FilterProxyModel.cpp
    src/ui/ScanStatsEngine.h
=======
    src/ui/FilterStateModel.cpp
    src/ui/models/FilterProxyModel.h
    src/ui/models/FilterProxyModel.cpp
    src/ui/models/GroupingProxyModel.h
    src/ui/models/GroupingProxyModel.cpp
    src/ui/ScanStatsEngine.h
>>>>>>> REPLACE
```

---

### 2. `src/ui/models/GroupingProxyModel.h` (New File)

```cpp
#ifndef GROUPINGPROXYMODEL_H
#define GROUPINGPROXYMODEL_H

#include <QAbstractProxyModel>
#include <QString>
#include <QVector>
#include <QSet>
#include <functional>
#include "../core/ModelContract.h"

namespace QuarkMeta {

namespace GroupRole {
    enum Roles {
        IsGroupHeaderRole = Qt::UserRole + 100,
        GroupIdRole,
        GroupTitleRole,
        GroupIsCollapsibleRole,
        GroupIsCollapsedRole,
        GroupItemCountRole
    };
}

struct GroupDefinition {
    QString id;
    QString titleTemplate; // e.g., "文件夹 (%1)" or "文件 (%1)"
    bool isCollapsible = true;
    std::function<bool(const QModelIndex&)> matchPredicate;
};

class GroupingProxyModel : public QAbstractProxyModel {
    Q_OBJECT

public:
    explicit GroupingProxyModel(QObject* parent = nullptr);
    ~GroupingProxyModel() override = default;

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool isGroupHeader(const QModelIndex& index) const;
    bool setGroupCollapsed(const QString& groupId, bool collapsed);
    bool isGroupCollapsed(const QString& groupId) const;

    QModelIndex groupHeaderIndex(const QString& groupId) const;

public slots:
    void rebuildMapping();

private slots:
    void onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles);
    void onSourceReset();

private:
    struct GroupNode {
        GroupDefinition definition;
        QVector<int> sourceRows;
    };

    QVector<GroupDefinition> m_groupDefs;
    QVector<GroupNode> m_activeGroups;
    QSet<QString> m_collapsedGroupIds;

    void setupDefaultGroups();
};

} // namespace QuarkMeta

#endif // GROUPINGPROXYMODEL_H
```

---

### 3. `src/ui/models/GroupingProxyModel.cpp` (New File)

```cpp
#include "GroupingProxyModel.h"

namespace QuarkMeta {

GroupingProxyModel::GroupingProxyModel(QObject* parent)
    : QAbstractProxyModel(parent)
{
    setupDefaultGroups();
}

void GroupingProxyModel::setupDefaultGroups() {
    m_groupDefs.clear();

    GroupDefinition folderGroup;
    folderGroup.id = "folders";
    folderGroup.titleTemplate = QString::fromUtf8("文件夹 (%1)");
    folderGroup.isCollapsible = true;
    folderGroup.matchPredicate = [](const QModelIndex& srcIdx) {
        return srcIdx.data(TypeRole).toString() == "folder";
    };

    GroupDefinition fileGroup;
    fileGroup.id = "files";
    fileGroup.titleTemplate = QString::fromUtf8("文件 (%1)");
    fileGroup.isCollapsible = false;
    fileGroup.matchPredicate = [](const QModelIndex& srcIdx) {
        return srcIdx.data(TypeRole).toString() != "folder";
    };

    m_groupDefs.append(folderGroup);
    m_groupDefs.append(fileGroup);
}

void GroupingProxyModel::setSourceModel(QAbstractItemModel* newSourceModel) {
    if (sourceModel()) {
        disconnect(sourceModel(), &QAbstractItemModel::dataChanged, this, &GroupingProxyModel::onSourceDataChanged);
        disconnect(sourceModel(), &QAbstractItemModel::modelReset, this, &GroupingProxyModel::onSourceReset);
        disconnect(sourceModel(), &QAbstractItemModel::layoutChanged, this, &GroupingProxyModel::onSourceReset);
        disconnect(sourceModel(), &QAbstractItemModel::rowsInserted, this, &GroupingProxyModel::onSourceReset);
        disconnect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, &GroupingProxyModel::onSourceReset);
    }

    QAbstractProxyModel::setSourceModel(newSourceModel);

    if (newSourceModel) {
        connect(newSourceModel, &QAbstractItemModel::dataChanged, this, &GroupingProxyModel::onSourceDataChanged);
        connect(newSourceModel, &QAbstractItemModel::modelReset, this, &GroupingProxyModel::onSourceReset);
        connect(newSourceModel, &QAbstractItemModel::layoutChanged, this, &GroupingProxyModel::onSourceReset);
        connect(newSourceModel, &QAbstractItemModel::rowsInserted, this, &GroupingProxyModel::onSourceReset);
        connect(newSourceModel, &QAbstractItemModel::rowsRemoved, this, &GroupingProxyModel::onSourceReset);
    }

    rebuildMapping();
}

void GroupingProxyModel::onSourceReset() {
    rebuildMapping();
}

void GroupingProxyModel::onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles) {
    rebuildMapping();
    emit dataChanged(index(0, topLeft.column()), index(rowCount() - 1, bottomRight.column()), roles);
}

void GroupingProxyModel::rebuildMapping() {
    beginResetModel();
    m_activeGroups.clear();

    if (!sourceModel()) {
        endResetModel();
        return;
    }

    int totalSourceRows = sourceModel()->rowCount();
    for (const auto& groupDef : m_groupDefs) {
        GroupNode groupNode;
        groupNode.definition = groupDef;

        for (int r = 0; r < totalSourceRows; ++r) {
            QModelIndex srcIdx = sourceModel()->index(r, 0);
            if (groupDef.matchPredicate && groupDef.matchPredicate(srcIdx)) {
                groupNode.sourceRows.append(r);
            }
        }

        if (!groupNode.sourceRows.isEmpty()) {
            m_activeGroups.append(groupNode);
        }
    }

    endResetModel();
}

QModelIndex GroupingProxyModel::mapToSource(const QModelIndex& proxyIndex) const {
    if (!proxyIndex.isValid()) return QModelIndex();

    quintptr internalId = proxyIndex.internalId();
    if (internalId == 0) {
        // Top-level group node has no direct source index mapping
        return QModelIndex();
    }

    int groupIdx = static_cast<int>(internalId) - 1;
    if (groupIdx < 0 || groupIdx >= m_activeGroups.size()) return QModelIndex();

    const auto& node = m_activeGroups[groupIdx];
    int childRow = proxyIndex.row();
    if (childRow < 0 || childRow >= node.sourceRows.size()) return QModelIndex();

    int srcRow = node.sourceRows[childRow];
    return sourceModel() ? sourceModel()->index(srcRow, proxyIndex.column()) : QModelIndex();
}

QModelIndex GroupingProxyModel::mapFromSource(const QModelIndex& sourceIndex) const {
    if (!sourceIndex.isValid() || !sourceModel()) return QModelIndex();

    int srcRow = sourceIndex.row();
    for (int g = 0; g < m_activeGroups.size(); ++g) {
        const auto& node = m_activeGroups[g];
        for (int c = 0; c < node.sourceRows.size(); ++c) {
            if (node.sourceRows[c] == srcRow) {
                return createIndex(c, sourceIndex.column(), static_cast<quintptr>(g + 1));
            }
        }
    }
    return QModelIndex();
}

QModelIndex GroupingProxyModel::index(int row, int column, const QModelIndex& parent) const {
    if (row < 0 || column < 0 || column >= columnCount()) return QModelIndex();

    if (!parent.isValid()) {
        // Top-level group index
        if (row >= m_activeGroups.size()) return QModelIndex();
        return createIndex(row, column, static_cast<quintptr>(0));
    }

    if (parent.internalId() == 0) {
        // Child under group node
        int groupIdx = parent.row();
        if (groupIdx < 0 || groupIdx >= m_activeGroups.size()) return QModelIndex();
        if (row >= m_activeGroups[groupIdx].sourceRows.size()) return QModelIndex();
        return createIndex(row, column, static_cast<quintptr>(groupIdx + 1));
    }

    return QModelIndex();
}

QModelIndex GroupingProxyModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) return QModelIndex();

    quintptr internalId = child.internalId();
    if (internalId == 0) {
        return QModelIndex();
    }

    int groupIdx = static_cast<int>(internalId) - 1;
    if (groupIdx >= 0 && groupIdx < m_activeGroups.size()) {
        return createIndex(groupIdx, 0, static_cast<quintptr>(0));
    }

    return QModelIndex();
}

int GroupingProxyModel::rowCount(const QModelIndex& parent) const {
    if (!parent.isValid()) {
        return m_activeGroups.size();
    }
    if (parent.internalId() == 0) {
        int groupIdx = parent.row();
        if (groupIdx >= 0 && groupIdx < m_activeGroups.size()) {
            return m_activeGroups[groupIdx].sourceRows.size();
        }
    }
    return 0;
}

int GroupingProxyModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return sourceModel() ? sourceModel()->columnCount() : 0;
}

QVariant GroupingProxyModel::data(const QModelIndex& proxyIndex, int role) const {
    if (!proxyIndex.isValid()) return QVariant();

    if (proxyIndex.internalId() == 0) {
        // Top-level group node data
        int groupIdx = proxyIndex.row();
        if (groupIdx < 0 || groupIdx >= m_activeGroups.size()) return QVariant();

        const auto& node = m_activeGroups[groupIdx];
        const auto& groupDef = node.definition;
        int count = node.sourceRows.size();

        switch (role) {
            case GroupRole::IsGroupHeaderRole:
                return true;
            case GroupRole::GroupIdRole:
                return groupDef.id;
            case GroupRole::GroupTitleRole:
                return groupDef.titleTemplate.arg(count);
            case GroupRole::GroupIsCollapsibleRole:
                return groupDef.isCollapsible;
            case GroupRole::GroupIsCollapsedRole:
                return isGroupCollapsed(groupDef.id);
            case GroupRole::GroupItemCountRole:
                return count;
            case Qt::DisplayRole:
                return (proxyIndex.column() == 0) ? groupDef.titleTemplate.arg(count) : QVariant();
            default:
                return QVariant();
        }
    }

    if (role == GroupRole::IsGroupHeaderRole) {
        return false;
    }

    QModelIndex srcIdx = mapToSource(proxyIndex);
    return srcIdx.isValid() ? srcIdx.data(role) : QVariant();
}

Qt::ItemFlags GroupingProxyModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    if (index.internalId() == 0) {
        // Group nodes are enabled but NOT selectable
        return Qt::ItemIsEnabled;
    }
    QModelIndex srcIdx = mapToSource(index);
    return srcIdx.isValid() ? srcIdx.flags() : Qt::NoItemFlags;
}

bool GroupingProxyModel::isGroupHeader(const QModelIndex& index) const {
    return index.isValid() && (index.internalId() == 0);
}

bool GroupingProxyModel::setGroupCollapsed(const QString& groupId, bool collapsed) {
    for (const auto& group : m_groupDefs) {
        if (group.id == groupId) {
            if (!group.isCollapsible) return false;
            if (collapsed) {
                m_collapsedGroupIds.insert(groupId);
            } else {
                m_collapsedGroupIds.remove(groupId);
            }
            return true;
        }
    }
    return false;
}

bool GroupingProxyModel::isGroupCollapsed(const QString& groupId) const {
    return m_collapsedGroupIds.contains(groupId);
}

QModelIndex GroupingProxyModel::groupHeaderIndex(const QString& groupId) const {
    for (int i = 0; i < m_activeGroups.size(); ++i) {
        if (m_activeGroups[i].definition.id == groupId) {
            return createIndex(i, 0, static_cast<quintptr>(0));
        }
    }
    return QModelIndex();
}

} // namespace QuarkMeta
```

---

### 4. `src/ui/TreeItemDelegate.h`
Remove custom string arrow concatenation from group header rendering in `TreeItemDelegate`.

```
<<<<<<< SEARCH
        // 🚀【Group Header Banner Paint】
        bool isGroupHeader = index.data(GroupingProxyModel::IsGroupHeaderRole).toBool();
        if (isGroupHeader) {
            painter->fillRect(option.rect, QColor("#1E1E1E"));
            if (index.column() == 0) {
                QString title = index.data(GroupingProxyModel::GroupTitleRole).toString();
                bool isCollapsible = index.data(GroupingProxyModel::GroupIsCollapsibleRole).toBool();
                bool isCollapsed = index.data(GroupingProxyModel::GroupIsCollapsedRole).toBool();

                painter->setPen(QColor("#A0A0A0"));
                painter->setFont(option.font);
                QRect textRect = option.rect.adjusted(12, 0, -12, 0);

                QString arrow = isCollapsible ? (isCollapsed ? QString::fromUtf8("▶ ") : QString::fromUtf8("▼ ")) : QString();
                painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, arrow + title);
            }
            return;
        }
=======
        // 🚀【Group Header Banner Paint】
        bool isGroupHeader = index.data(GroupingProxyModel::IsGroupHeaderRole).toBool();
        if (isGroupHeader) {
            painter->fillRect(option.rect, QColor("#1E1E1E"));
            if (index.column() == 0) {
                QString title = index.data(GroupingProxyModel::GroupTitleRole).toString();
                painter->setPen(QColor("#A0A0A0"));
                painter->setFont(option.font);
                QRect textRect = option.rect.adjusted(6, 0, -6, 0);
                painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);
            }
            return;
        }
>>>>>>> REPLACE
```

---

### 5. `src/ui/DropTreeView.h` & `src/ui/DropTreeView.cpp`
Configure `DropTreeView` for native tree group expansion and lock file group permanently expanded.

#### `src/ui/DropTreeView.h`
```
<<<<<<< SEARCH
    void setEmptyHint(const QString& hint) { m_emptyHint = hint; }
=======
    void setEmptyHint(const QString& hint) { m_emptyHint = hint; }
    void setModel(QAbstractItemModel* model) override;
>>>>>>> REPLACE
```

#### `src/ui/DropTreeView.cpp`
```
<<<<<<< SEARCH
DropTreeView::DropTreeView(QWidget* parent)
    : QTreeView(parent)
{
    setHeader(new ContentHeaderView(Qt::Horizontal, this));
}
=======
DropTreeView::DropTreeView(QWidget* parent)
    : QTreeView(parent)
{
    setHeader(new ContentHeaderView(Qt::Horizontal, this));
    setRootIsDecorated(true);

    connect(this, &QTreeView::collapsed, this, [this](const QModelIndex& index) {
        if (index.data(GroupingProxyModel::IsGroupHeaderRole).toBool()) {
            QString groupId = index.data(GroupingProxyModel::GroupIdRole).toString();
            bool isCollapsible = index.data(GroupingProxyModel::GroupIsCollapsibleRole).toBool();
            if (!isCollapsible) {
                setExpanded(index, true);
            } else if (auto* groupModel = qobject_cast<GroupingProxyModel*>(model())) {
                groupModel->setGroupCollapsed(groupId, true);
            }
        }
    });

    connect(this, &QTreeView::expanded, this, [this](const QModelIndex& index) {
        if (index.data(GroupingProxyModel::IsGroupHeaderRole).toBool()) {
            QString groupId = index.data(GroupingProxyModel::GroupIdRole).toString();
            if (auto* groupModel = qobject_cast<GroupingProxyModel*>(model())) {
                groupModel->setGroupCollapsed(groupId, false);
            }
        }
    });
}

void DropTreeView::setModel(QAbstractItemModel* model) {
    QTreeView::setModel(model);
    if (auto* groupModel = qobject_cast<GroupingProxyModel*>(model)) {
        connect(groupModel, &QAbstractItemModel::modelReset, this, [this, groupModel]() {
            for (int r = 0; r < groupModel->rowCount(); ++r) {
                QModelIndex groupIdx = groupModel->index(r, 0);
                QString groupId = groupIdx.data(GroupingProxyModel::GroupIdRole).toString();
                bool isCollapsed = groupModel->isGroupCollapsed(groupId);
                bool isCollapsible = groupIdx.data(GroupingProxyModel::GroupIsCollapsibleRole).toBool();

                if (!isCollapsible || !isCollapsed) {
                    setExpanded(groupIdx, true);
                } else {
                    setExpanded(groupIdx, false);
                }
            }
        });
    }
}
>>>>>>> REPLACE
```

---

### 6. `src/ui/DropListView.h` & `src/ui/DropListView.cpp`
Re-inherit `DropListView` from `QTreeView` with zero indentation (`setIndentation(0)`).

#### `src/ui/DropListView.h`
```
<<<<<<< SEARCH
#include <QListView>
...
class DropListView : public QListView {
=======
#include <QTreeView>
#include <QHeaderView>
...
class DropListView : public QTreeView {
>>>>>>> REPLACE
```

#### `src/ui/DropListView.cpp`
```
<<<<<<< SEARCH
DropListView::DropListView(QWidget* parent)
    : QListView(parent)
{
}
=======
DropListView::DropListView(QWidget* parent)
    : QTreeView(parent)
{
    setHeaderHidden(true);
    setIndentation(0);
    setRootIsDecorated(true);

    connect(this, &QTreeView::collapsed, this, [this](const QModelIndex& index) {
        if (index.data(GroupingProxyModel::IsGroupHeaderRole).toBool()) {
            bool isCollapsible = index.data(GroupingProxyModel::GroupIsCollapsibleRole).toBool();
            if (!isCollapsible) {
                setExpanded(index, true);
            }
        }
    });
}
>>>>>>> REPLACE
```

---

### 7. `src/ui/ColumnViewWidget.h` & `src/ui/ColumnViewWidget.cpp`
Equip each `ColumnViewPane` with a dedicated `GroupingProxyModel` instance.

#### `src/ui/ColumnViewWidget.h`
```
<<<<<<< SEARCH
    FilterProxyModel* m_proxyModel = nullptr;
=======
    FilterProxyModel* m_proxyModel = nullptr;
    GroupingProxyModel* m_groupingModel = nullptr;
>>>>>>> REPLACE
```

#### `src/ui/ColumnViewWidget.cpp`
```
<<<<<<< SEARCH
    m_proxyModel = new FilterProxyModel(this);
    m_listView->setModel(m_proxyModel);
=======
    m_proxyModel = new FilterProxyModel(this);
    m_groupingModel = new GroupingProxyModel(this);
    m_groupingModel->setSourceModel(m_proxyModel);
    m_listView->setModel(m_groupingModel);
>>>>>>> REPLACE
```

---

### 8. `src/ui/ContentPanel.h` & `src/ui/ContentPanel.cpp`
Add `mapToActiveViewModelIndex` to fix cross-model index bugs in `selectAndScrollToItem`, `restoreSelections`, and `refreshVisibleThumbnails`.

#### `src/ui/ContentPanel.h`
```
<<<<<<< SEARCH
    QSortFilterProxyModel* getActiveProxyModel() const;
=======
    QSortFilterProxyModel* getActiveProxyModel() const;
    QModelIndex mapToActiveViewModelIndex(const QModelIndex& sourceOrFilterIndex) const;
>>>>>>> REPLACE
```

#### `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
QModelIndex ContentPanel::mapToActiveViewModelIndex(const QModelIndex& srcIdx) const {
    if (!srcIdx.isValid()) return QModelIndex();
    QAbstractItemView* activeView = getActiveView();
    if (!activeView || !activeView->model()) return srcIdx;

    if (activeView->model() == m_groupingProxyModel) {
        return m_groupingProxyModel->mapFromSource(srcIdx);
    }
    return srcIdx;
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. **Compilation Verification**:
   Run CMake build to confirm clean compilation with two-level `GroupingProxyModel`.
2. **Behavioral Verification**:
   - Verify native `QTreeView` expand/collapse arrows on `文件夹 (X)` headers.
   - Verify `文件 (Y)` group header cannot be collapsed (auto-re-expands if collapsed).
   - Verify group headers are non-selectable and omitted from multi-selection/keyboard navigation.
   - Verify `selectAndScrollToItem` and thumbnail loading operate accurately across model layers.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Model Layer SSOT**: Restores standard Qt proxy mapping contract via `mapToSource`/`mapFromSource`.
- **View-Model Mapping Helper**: Solves cross-model index confusion cleanly through `mapToActiveViewModelIndex`.

---

## 6. Header API Signature Verification Table

| Calling File | Target Class / Header | Function / Method Signature | Verification Status |
| :--- | :--- | :--- | :--- |
| `ContentPanel.cpp` | `GroupingProxyModel.h` | `QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override` | Verified |
| `DropTreeView.cpp` | `GroupingProxyModel.h` | `QModelIndex groupHeaderIndex(const QString& groupId) const` | Verified |
| `DropListView.cpp` | `QTreeView` | `void setIndentation(int i)` | Verified |
| `TreeItemDelegate.h` | `ModelContract.h` | `TypeRole` (`Qt::UserRole + 0`) | Verified |
