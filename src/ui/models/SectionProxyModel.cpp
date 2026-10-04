#include "SectionProxyModel.h"
#include "../../core/ModelContract.h"
#include <algorithm>

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
    m_sourceToProxyMap.clear();
    m_folderSourceRows.clear();
    m_fileSourceRows.clear();

    if (sourceModel()) {
        int total = sourceModel()->rowCount();
        m_sourceToProxyMap.fill(-1, total);
        for (int i = 0; i < total; ++i) {
            QModelIndex srcIdx = sourceModel()->index(i, 0);
            QString typeStr = srcIdx.data(TypeRole).toString();
            if (typeStr == "folder") {
                m_folderSourceRows.append(i);
            } else {
                m_fileSourceRows.append(i);
            }
        }
    }

    m_folderCount = m_folderSourceRows.size();
    m_fileCount = m_fileSourceRows.size();

    if (!sourceModel() || (m_folderCount == 0 && m_fileCount == 0)) {
        endResetModel();
        return;
    }

    // 1. Folder Header
    if (m_folderCount > 0) {
        m_mapping.append({RowType::FolderHeader, -1});
        if (!m_folderCollapsed) {
            for (int srcRow : m_folderSourceRows) {
                m_mapping.append({RowType::FolderItem, srcRow});
            }
        }
    }

    // 2. File Header & File Items
    if (m_fileCount > 0) {
        if (m_folderCount > 0) {
            m_mapping.append({RowType::FileHeader, -1});
        }
        for (int srcRow : m_fileSourceRows) {
            m_mapping.append({RowType::FileItem, srcRow});
        }
    }

    // Build O(1) reverse index from sourceRow to proxyRow
    for (int pRow = 0; pRow < m_mapping.size(); ++pRow) {
        int sRow = m_mapping.at(pRow).sourceRow;
        if (sRow >= 0 && sRow < m_sourceToProxyMap.size()) {
            m_sourceToProxyMap[sRow] = pRow;
        }
    }

    endResetModel();
}

void SectionProxyModel::updateCounts() {
    // Counts updated during rebuildMapping
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
    if (srcRow >= 0 && srcRow < m_sourceToProxyMap.size()) {
        int pRow = m_sourceToProxyMap.at(srcRow);
        if (pRow >= 0 && pRow < m_mapping.size()) {
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

    if (role == SectionKindRole) {
        if (entry.type == RowType::FolderHeader) return 1;
        if (entry.type == RowType::FileHeader) return 2;
        return 0;
    }

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

    if (role == SectionRowRole) {
        if (entry.type == RowType::FolderItem) {
            auto it = std::find(m_folderSourceRows.begin(), m_folderSourceRows.end(), entry.sourceRow);
            if (it != m_folderSourceRows.end()) {
                return static_cast<int>(std::distance(m_folderSourceRows.begin(), it));
            }
        }
        if (entry.type == RowType::FileItem) {
            auto it = std::find(m_fileSourceRows.begin(), m_fileSourceRows.end(), entry.sourceRow);
            if (it != m_fileSourceRows.end()) {
                return static_cast<int>(std::distance(m_fileSourceRows.begin(), it));
            }
        }
        return -1;
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
        return false;
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
        return Qt::ItemIsEnabled;
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
    if (m_folderCollapsed == collapsed || m_folderCount == 0) return;

    if (collapsed) {
        beginRemoveRows(QModelIndex(), 1, m_folderCount);
        m_folderCollapsed = true;
        m_mapping.clear();
        m_mapping.append({RowType::FolderHeader, -1});
        if (m_fileCount > 0) {
            if (m_folderCount > 0) m_mapping.append({RowType::FileHeader, -1});
            for (int srcRow : m_fileSourceRows) {
                m_mapping.append({RowType::FileItem, srcRow});
            }
        }
        endRemoveRows();
    } else {
        beginInsertRows(QModelIndex(), 1, m_folderCount);
        m_folderCollapsed = false;
        m_mapping.clear();
        m_mapping.append({RowType::FolderHeader, -1});
        for (int srcRow : m_folderSourceRows) {
            m_mapping.append({RowType::FolderItem, srcRow});
        }
        if (m_fileCount > 0) {
            if (m_folderCount > 0) m_mapping.append({RowType::FileHeader, -1});
            for (int srcRow : m_fileSourceRows) {
                m_mapping.append({RowType::FileItem, srcRow});
            }
        }
        endInsertRows();
    }

    QModelIndex headerIdx = index(0, 0);
    emit dataChanged(headerIdx, headerIdx, {SectionCollapsedRole, SectionHeaderTextRole});
}

void SectionProxyModel::onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
    if (!topLeft.isValid() || !bottomRight.isValid()) return;

    int minProxyRow = -1;
    int maxProxyRow = -1;

    for (int r = topLeft.row(); r <= bottomRight.row(); ++r) {
        if (r >= 0 && r < m_sourceToProxyMap.size()) {
            int pRow = m_sourceToProxyMap.at(r);
            if (pRow >= 0) {
                if (minProxyRow == -1 || pRow < minProxyRow) minProxyRow = pRow;
                if (maxProxyRow == -1 || pRow > maxProxyRow) maxProxyRow = pRow;
            }
        }
    }

    if (minProxyRow != -1 && maxProxyRow != -1) {
        QModelIndex pTopLeft = index(minProxyRow, topLeft.column());
        QModelIndex pBottomRight = index(maxProxyRow, bottomRight.column());
        emit dataChanged(pTopLeft, pBottomRight, roles);
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
    QModelIndexList proxyList;
    QModelIndexList sourceList;
    for (int r = 0; r < m_mapping.size(); ++r) {
        if (m_mapping[r].sourceRow >= 0) {
            QModelIndex pIdx = createIndex(r, 0);
            QModelIndex sIdx = mapToSource(pIdx);
            if (sIdx.isValid()) {
                proxyList.append(pIdx);
                sourceList.append(sIdx);
            }
        }
    }
    m_layoutChangeProxyIndexes = proxyList;
    m_layoutChangeSourceIndexes = sourceList;
    emit layoutAboutToBeChanged();
}

void SectionProxyModel::onSourceLayoutChanged() {
    rebuildMapping();
    if (!m_layoutChangeProxyIndexes.isEmpty()) {
        QModelIndexList newProxyIndexes;
        for (const QModelIndex& sIdx : m_layoutChangeSourceIndexes) {
            newProxyIndexes.append(mapFromSource(sIdx));
        }
        changePersistentIndexList(m_layoutChangeProxyIndexes, newProxyIndexes);
    }
    m_layoutChangeProxyIndexes.clear();
    m_layoutChangeSourceIndexes.clear();
    emit layoutChanged();
}

} // namespace QuarkMeta
