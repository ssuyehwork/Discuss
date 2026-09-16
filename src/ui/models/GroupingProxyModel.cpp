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
