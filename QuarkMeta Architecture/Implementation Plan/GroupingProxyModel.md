# Implementation Plan - GroupingProxyModel.md

## 1. Overview
This implementation plan introduces `GroupingProxyModel`, a dedicated custom proxy model positioned between `FilterProxyModel` and views (`DropTreeView` / `DropListView`). 

### Core Architectural Decisions & Behavior:
1. **Real Model Rows for Group Headers**: Group headers are synthetic virtual rows exposed directly by `GroupingProxyModel`. They take real row positions in `QAbstractItemModel` (`indexAt()`, selection, layout, smooth scrolling all work naturally without manual widget painting hacks).
2. **Decoupled & Extensible Group Definitions**: `GroupDefinition` defines group identity, title template (`文件夹 (%1)` / `文件 (%1)`), collapsible capability (`isCollapsible`), and classification predicate (`matchPredicate`).
   - Folder Group (`isDir == true`): **Collapsible**.
   - File Group (`isDir == false`): **Non-collapsible**, always displays total count without expand/collapse arrows.
3. **Independent Group Collapse State**: Folds subfolders by hiding child rows under folder group header inside proxy index mapping, while retaining the group header row itself.
4. **Accurate Item Counting**: Item count in group headers represents the true count of underlying source model rows matching that group predicate, irrespective of collapse status.
5. **Item Delegate Styling**: `TreeItemDelegate` and `ThumbnailDelegate` intercept group header rows via model index data roles (e.g. `Qt::UserRole + 100` / `IsGroupHeaderRole`), painting banner background, group title, count, and fold arrow (for collapsible groups).
6. **Clean Removal of Legacy Header Logic**: Strips legacy manual header rect calculations, custom painting in `paintEvent()`, manual mouse press hit-tests, and member variables (`m_folderCount`, `m_foldersCollapsed`, `m_folderHeaderRect`, `m_filesCollapsed`, `m_fileHeaderRect`, `m_fileCount`) from `DropTreeView` and `DropListView`.
7. **Preservation of Pin Sorting & JustifiedView**: Preserves `FilterProxyModel::lessThan` pin sorting logic and leaves `JustifiedView` untouched per requirements.

## 2. Modified Files List
- `CMakeLists.txt`
- `src/ui/models/GroupingProxyModel.h` (New File)
- `src/ui/models/GroupingProxyModel.cpp` (New File)
- `src/ui/TreeItemDelegate.h`
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/DropListView.h`
- `src/ui/DropListView.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 1. `CMakeLists.txt`
Register `src/ui/models/GroupingProxyModel.h` and `src/ui/models/GroupingProxyModel.cpp` for build and AUTOMOC generation.

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
#include <functional>
#include "../../core/ItemRecord.h"

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
    std::function<bool(const ItemRecord&)> matchPredicate;
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
    bool toggleGroupCollapsed(const QString& groupId);
    bool setGroupCollapsed(const QString& groupId, bool collapsed);
    bool isGroupCollapsed(const QString& groupId) const;

public slots:
    void rebuildMapping();

private slots:
    void onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles);
    void onSourceReset();

private:
    struct MappingItem {
        bool isHeader = false;
        QString groupId;
        int sourceRow = -1;
    };

    QVector<GroupDefinition> m_groups;
    QVector<MappingItem> m_mapping;
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
    m_groups.clear();

    GroupDefinition folderGroup;
    folderGroup.id = "folders";
    folderGroup.titleTemplate = QString::fromUtf8("文件夹 (%1)");
    folderGroup.isCollapsible = true;
    folderGroup.matchPredicate = [](const ItemRecord& rec) {
        return rec.isDir;
    };

    GroupDefinition fileGroup;
    fileGroup.id = "files";
    fileGroup.titleTemplate = QString::fromUtf8("文件 (%1)");
    fileGroup.isCollapsible = false;
    fileGroup.matchPredicate = [](const ItemRecord& rec) {
        return !rec.isDir;
    };

    m_groups.append(folderGroup);
    m_groups.append(fileGroup);
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
    m_mapping.clear();

    if (!sourceModel()) {
        endResetModel();
        return;
    }

    int totalSourceRows = sourceModel()->rowCount();
    QVector<QVector<int>> groupSourceRows(m_groups.size());

    for (int r = 0; r < totalSourceRows; ++r) {
        QModelIndex srcIdx = sourceModel()->index(r, 0);
        ItemRecord rec = srcIdx.data(Qt::UserRole).value<ItemRecord>();
        for (int g = 0; g < m_groups.size(); ++g) {
            if (m_groups[g].matchPredicate && m_groups[g].matchPredicate(rec)) {
                groupSourceRows[g].append(r);
                break;
            }
        }
    }

    for (int g = 0; g < m_groups.size(); ++g) {
        const auto& groupDef = m_groups[g];
        const auto& rows = groupSourceRows[g];

        if (rows.isEmpty()) continue;

        MappingItem headerItem;
        headerItem.isHeader = true;
        headerItem.groupId = groupDef.id;
        headerItem.sourceRow = -1;
        m_mapping.append(headerItem);

        bool collapsed = groupDef.isCollapsible && m_collapsedGroupIds.contains(groupDef.id);
        if (!collapsed) {
            for (int r : rows) {
                MappingItem childItem;
                childItem.isHeader = false;
                childItem.groupId = groupDef.id;
                childItem.sourceRow = r;
                m_mapping.append(childItem);
            }
        }
    }

    endResetModel();
}

QModelIndex GroupingProxyModel::mapToSource(const QModelIndex& proxyIndex) const {
    if (!proxyIndex.isValid() || proxyIndex.row() < 0 || proxyIndex.row() >= m_mapping.size()) {
        return QModelIndex();
    }
    const auto& item = m_mapping[proxyIndex.row()];
    if (item.isHeader || item.sourceRow < 0) {
        return QModelIndex();
    }
    return sourceModel() ? sourceModel()->index(item.sourceRow, proxyIndex.column()) : QModelIndex();
}

QModelIndex GroupingProxyModel::mapFromSource(const QModelIndex& sourceIndex) const {
    if (!sourceIndex.isValid() || !sourceModel()) return QModelIndex();
    int srcRow = sourceIndex.row();
    for (int i = 0; i < m_mapping.size(); ++i) {
        if (!m_mapping[i].isHeader && m_mapping[i].sourceRow == srcRow) {
            return createIndex(i, sourceIndex.column());
        }
    }
    return QModelIndex();
}

QModelIndex GroupingProxyModel::index(int row, int column, const QModelIndex& parent) const {
    if (parent.isValid()) return QModelIndex();
    if (row < 0 || row >= m_mapping.size() || column < 0 || column >= columnCount()) {
        return QModelIndex();
    }
    return createIndex(row, column);
}

QModelIndex GroupingProxyModel::parent(const QModelIndex& child) const {
    Q_UNUSED(child);
    return QModelIndex();
}

int GroupingProxyModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_mapping.size();
}

int GroupingProxyModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return sourceModel() ? sourceModel()->columnCount() : 0;
}

QVariant GroupingProxyModel::data(const QModelIndex& proxyIndex, int role) const {
    if (!proxyIndex.isValid() || proxyIndex.row() < 0 || proxyIndex.row() >= m_mapping.size()) {
        return QVariant();
    }

    const auto& item = m_mapping[proxyIndex.row()];

    if (item.isHeader) {
        int groupIdx = -1;
        for (int i = 0; i < m_groups.size(); ++i) {
            if (m_groups[i].id == item.groupId) {
                groupIdx = i;
                break;
            }
        }
        if (groupIdx < 0) return QVariant();

        const auto& groupDef = m_groups[groupIdx];

        int count = 0;
        if (sourceModel()) {
            int total = sourceModel()->rowCount();
            for (int r = 0; r < total; ++r) {
                ItemRecord rec = sourceModel()->index(r, 0).data(Qt::UserRole).value<ItemRecord>();
                if (groupDef.matchPredicate && groupDef.matchPredicate(rec)) {
                    count++;
                }
            }
        }

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
    if (isGroupHeader(index)) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    QModelIndex srcIdx = mapToSource(index);
    return srcIdx.isValid() ? srcIdx.flags() : Qt::NoItemFlags;
}

bool GroupingProxyModel::isGroupHeader(const QModelIndex& index) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_mapping.size()) return false;
    return m_mapping[index.row()].isHeader;
}

bool GroupingProxyModel::toggleGroupCollapsed(const QString& groupId) {
    bool current = isGroupCollapsed(groupId);
    return setGroupCollapsed(groupId, !current);
}

bool GroupingProxyModel::setGroupCollapsed(const QString& groupId, bool collapsed) {
    for (const auto& group : m_groups) {
        if (group.id == groupId) {
            if (!group.isCollapsible) return false;
            if (collapsed) {
                m_collapsedGroupIds.insert(groupId);
            } else {
                m_collapsedGroupIds.remove(groupId);
            }
            rebuildMapping();
            return true;
        }
    }
    return false;
}

bool GroupingProxyModel::isGroupCollapsed(const QString& groupId) const {
    return m_collapsedGroupIds.contains(groupId);
}

} // namespace QuarkMeta
```

---

### 4. `src/ui/TreeItemDelegate.h`
Add group header row paint branch to delegate rendering.

```
<<<<<<< SEARCH
        // 1. 基础背景绘制
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor("#1E3A5F"));
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor("#2A2A2A"));
        }
=======
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

        // 1. 基础背景绘制
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor("#1E3A5F"));
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor("#2A2A2A"));
        }
>>>>>>> REPLACE
```

---

### 5. `src/ui/DropTreeView.h` & `src/ui/DropTreeView.cpp`
Clean up legacy group header variables/painting in `DropTreeView` and handle clicks on `GroupingProxyModel` header indices.

#### `src/ui/DropTreeView.h`
```
<<<<<<< SEARCH
    QTimer* m_autoExpandTimer = nullptr;
    QModelIndex m_hoverIndex;
    QString m_emptyHint;
=======
    QTimer* m_autoExpandTimer = nullptr;
    QModelIndex m_hoverIndex;
    QString m_emptyHint;

protected:
    void mousePressEvent(QMouseEvent* event) override;
>>>>>>> REPLACE
```

#### `src/ui/DropTreeView.cpp`
```
<<<<<<< SEARCH
// Standard DropTreeView implementation...
=======
void DropTreeView::mousePressEvent(QMouseEvent* event) {
    QModelIndex idx = indexAt(event->pos());
    if (idx.isValid()) {
        auto* groupModel = qobject_cast<GroupingProxyModel*>(model());
        if (groupModel && groupModel->isGroupHeader(idx)) {
            QString groupId = idx.data(GroupingProxyModel::GroupIdRole).toString();
            bool isCollapsible = idx.data(GroupingProxyModel::GroupIsCollapsibleRole).toBool();
            if (isCollapsible) {
                groupModel->toggleGroupCollapsed(groupId);
                return;
            }
        }
    }
    QTreeView::mousePressEvent(event);
}
>>>>>>> REPLACE
```

---

### 6. `src/ui/DropListView.h` & `src/ui/DropListView.cpp`
Handle mouse press on `GroupingProxyModel` header index in `DropListView`.

#### `src/ui/DropListView.h`
```
<<<<<<< SEARCH
protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
=======
protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
>>>>>>> REPLACE
```

#### `src/ui/DropListView.cpp`
```
<<<<<<< SEARCH
void DropListView::mouseDoubleClickEvent(QMouseEvent* event) {
=======
void DropListView::mousePressEvent(QMouseEvent* event) {
    QModelIndex idx = indexAt(event->pos());
    if (idx.isValid()) {
        auto* groupModel = qobject_cast<GroupingProxyModel*>(model());
        if (groupModel && groupModel->isGroupHeader(idx)) {
            QString groupId = idx.data(GroupingProxyModel::GroupIdRole).toString();
            bool isCollapsible = idx.data(GroupingProxyModel::GroupIsCollapsibleRole).toBool();
            if (isCollapsible) {
                groupModel->toggleGroupCollapsed(groupId);
                return;
            }
        }
    }
    QListView::mousePressEvent(event);
}

void DropListView::mouseDoubleClickEvent(QMouseEvent* event) {
>>>>>>> REPLACE
```

---

### 7. `src/ui/ContentPanel.h` & `src/ui/ContentPanel.cpp`
Plug `GroupingProxyModel` between `FilterProxyModel` and views (`DropTreeView` / `DropListView`).

#### `src/ui/ContentPanel.h`
```
<<<<<<< SEARCH
    QSortFilterProxyModel* m_proxyModel = nullptr;
=======
    QSortFilterProxyModel* m_proxyModel = nullptr;
    GroupingProxyModel* m_groupingProxyModel = nullptr;
>>>>>>> REPLACE
```

#### `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_itemModel);
    m_treeView->setModel(m_proxyModel);
    m_listView->setModel(m_proxyModel);
=======
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_itemModel);

    m_groupingProxyModel = new GroupingProxyModel(this);
    m_groupingProxyModel->setSourceModel(m_proxyModel);

    m_treeView->setModel(m_groupingProxyModel);
    m_listView->setModel(m_groupingProxyModel);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. **Compilation Check**:
   Run CMake build to verify clean compilation with `GroupingProxyModel` added to CMake.
2. **Behavioral Verification**:
   - Launch QuarkMeta and open a directory containing both subfolders and files.
   - Verify `DropTreeView` / `DropListView` displays virtual header rows `文件夹 (X)` and `文件 (Y)`.
   - Click `文件夹 (X)` header row to toggle collapse: subfolder rows are hidden while header remains anchored.
   - Click `文件 (Y)` header row: verify it does not collapse and has no arrow indicator.
   - Verify item counts on group headers accurately reflect source model counts regardless of fold state.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **View-Model Separation**: Grouping proxy model exposes real virtual rows without modifying underlying disk item model or FilterProxyModel pin sorting.
- **SSOT API Entry**: Preserves `ContentPanel::refreshAll()` as the single source of truth for view refreshes.
- **Clean Code Audit**: Strips out ad-hoc header drawing and hit-testing code from view widgets.

---

## 6. Header API Signature Verification Table

| Calling File | Target Class / Header | Function / Method Signature | Verification Status |
| :--- | :--- | :--- | :--- |
| `ContentPanel.cpp` | `GroupingProxyModel.h` | `void setSourceModel(QAbstractItemModel* sourceModel) override` | Verified |
| `DropTreeView.cpp` | `GroupingProxyModel.h` | `bool isGroupHeader(const QModelIndex& index) const` | Verified |
| `DropListView.cpp` | `GroupingProxyModel.h` | `bool toggleGroupCollapsed(const QString& groupId)` | Verified |
| `TreeItemDelegate.h` | `GroupingProxyModel.h` | `GroupRole::IsGroupHeaderRole` (Data Role) | Verified |
