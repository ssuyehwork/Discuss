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
    m_folderSourceRows.clear();
    m_fileSourceRows.clear();

    if (sourceModel()) {
        int total = sourceModel()->rowCount();
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

    endResetModel();
}

void SectionProxyModel::updateCounts() {
    // Rebuild mapping maintains counts
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
    if (srcIdx.isValid() && sourceModel()->setData(srcIdx, value, role)) {
        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
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
