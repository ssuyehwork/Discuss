#include "FolderProxyModel.h"
#include "../ContentPanel.h"
#include <QDateTime>

namespace QuarkMeta {

FolderProxyModel::FolderProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

void FolderProxyModel::updateFilter() {
    beginFilterChange();
    endFilterChange();
}

bool FolderProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& /*sourceParent*/) const {
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];

    // 🚀【绝对物理隔离 1】：仅接受文件夹，从源头彻底剔除所有文件！
    if (!record.isDir) {
        return false;
    }

    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

    if (!currentFilter.showFolders) {
        return false;
    }

    // 关键词过滤
    if (!currentFilter.keyword.isEmpty()) {
        const QString& kw = currentFilter.keyword;
        if (!record.filename.contains(kw, Qt::CaseInsensitive)) {
            return false;
        }
    }

    return true;
}

bool FolderProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return QSortFilterProxyModel::lessThan(source_left, source_right);

    const auto& records = sourceModelPtr->allRecords();
    int leftRow = source_left.row();
    int rightRow = source_right.row();
    if (leftRow < 0 || leftRow >= static_cast<int>(records.size()) || 
        rightRow < 0 || rightRow >= static_cast<int>(records.size())) {
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }

    const auto& leftRec = records[leftRow];
    const auto& rightRec = records[rightRow];

    // 🚀【文件夹专属置顶】：仅在文件夹内部优先置顶，互不跨界
    bool leftPinned = leftRec.pinned || leftRec.encrypted;
    bool rightPinned = rightRec.pinned || rightRec.encrypted;
    if (leftPinned != rightPinned) {
        return (sortOrder() == Qt::AscendingOrder) ? leftPinned : rightPinned;
    }

    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    ContentPanel::SortType sType = contentPanel ? contentPanel->currentSortType() : ContentPanel::SortByName;

    auto compareNames = [](const ItemRecord& l, const ItemRecord& r) {
        return l.filename.localeAwareCompare(r.filename) < 0;
    };

    switch (sType) {
        case ContentPanel::SortByName: return compareNames(leftRec, rightRec);
        case ContentPanel::SortByCreateDate:
            if (leftRec.ctime != rightRec.ctime) return leftRec.ctime < rightRec.ctime;
            return compareNames(leftRec, rightRec);
        case ContentPanel::SortByModifyDate:
            if (leftRec.mtime != rightRec.mtime) return leftRec.mtime < rightRec.mtime;
            return compareNames(leftRec, rightRec);
        default:
            return compareNames(leftRec, rightRec);
    }
}

} // namespace QuarkMeta
