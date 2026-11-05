#include "ContentSortController.h"
#include "../models/FilterProxyModel.h"
#include "../../core/AppConfig.h"

namespace RapidNotes {

ContentSortController::ContentSortController(QObject* parent) : QObject(parent) {}

void ContentSortController::setSortType(SortType type) {
    if (m_sortType != type) {
        m_sortType = type;
        emit sortCriteriaChanged(m_sortType, m_sortOrder);
    }
}

void ContentSortController::setSortOrder(Qt::SortOrder order) {
    if (m_sortOrder != order) {
        m_sortOrder = order;
        emit sortCriteriaChanged(m_sortType, m_sortOrder);
    }
}

void ContentSortController::setSortCriteria(SortType type, Qt::SortOrder order) {
    if (m_sortType != type || m_sortOrder != order) {
        m_sortType = type;
        m_sortOrder = order;
        emit sortCriteriaChanged(m_sortType, m_sortOrder);
    }
}

void ContentSortController::setCurrentFolderPath(const QString& folderPath) {
    m_currentFolderPath = folderPath;
}

FileListColumn ContentSortController::columnForSortType(SortType type) {
    switch (type) {
        case SortType::SortByName: return FileListColumn::ColName;
        case SortType::SortByCreateDate: return FileListColumn::ColCreateDate;
        case SortType::SortByModifyDate: return FileListColumn::ColModifyDate;
        case SortType::SortByExtension: return FileListColumn::ColType;
        case SortType::SortBySize: return FileListColumn::ColSize;
        case SortType::SortByDimension: return FileListColumn::ColDimension;
        case SortType::SortByRating: return FileListColumn::ColRating;
        case SortType::SortByAddedDate: return FileListColumn::ColCreateDate;
        default: return FileListColumn::ColName;
    }
}

SortType ContentSortController::sortTypeForColumn(FileListColumn col) {
    switch (col) {
        case FileListColumn::ColName: return SortType::SortByName;
        case FileListColumn::ColCreateDate: return SortType::SortByCreateDate;
        case FileListColumn::ColModifyDate: return SortType::SortByModifyDate;
        case FileListColumn::ColType: return SortType::SortByExtension;
        case FileListColumn::ColSize: return SortType::SortBySize;
        case FileListColumn::ColDimension: return SortType::SortByDimension;
        case FileListColumn::ColRating: return SortType::SortByRating;
        default: return SortType::SortByName;
    }
}

void ContentSortController::applySortToModel(QSortFilterProxyModel* proxyModel) {
    if (!proxyModel) return;
    int col = static_cast<int>(columnForSortType(m_sortType));
    proxyModel->sort(col, m_sortOrder);
}

void ContentSortController::loadFromConfig() {}
void ContentSortController::saveToConfig() {}

} // namespace RapidNotes
