#include "FilterProxyModel.h"
#include "ItemModelBase.h"
#include "../UiHelper.h"
#include <QDateTime>
#include <QRegularExpression>
#include <cmath>

namespace QuarkMeta {

FilterProxyModel::FilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

void FilterProxyModel::setSortCriteria(int type, Qt::SortOrder order) {
    bool typeChanged = (m_sortType != type);
    m_sortType = type;

    if (sortColumn() != 0 || sortOrder() != order) {
        sort(0, order);
    } else if (typeChanged) {
        invalidate();
    }
}

void FilterProxyModel::updateFilter() {
    beginFilterChange();
    endFilterChange();
}

void FilterProxyModel::setCachedDuplicatePaths(const QSet<QString>& paths) {
    if (m_cachedDuplicatePaths == paths) return;
    m_cachedDuplicatePaths = paths;
    updateFilter();
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    Q_UNUSED(sourceParent);
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];

    return FilterMatcher::matches(record, currentFilter, m_cachedDuplicatePaths);
}

bool FilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
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

    // 🚀【强制优先级 1：文件夹优先】：文件夹在分区最顶部，无视升降序反转（不跨区）
    if (leftRec.isDir != rightRec.isDir) {
        return (sortOrder() == Qt::AscendingOrder) == leftRec.isDir;
    }

    // 🚀【强制优先级 2：置顶优先】：在各自分区内（文件夹区/文件区），置顶项永远在最顶部，无视升降序反转
    if (leftRec.pinned != rightRec.pinned) {
        return (sortOrder() == Qt::AscendingOrder) == leftRec.pinned;
    }

    auto compareNames = [](const ItemRecord& l, const ItemRecord& r) {
        return l.filename.localeAwareCompare(r.filename) < 0;
    };

    // 0: SortByName, 1: SortByCreateDate, 2: SortByModifyDate, 3: SortByExtension, 4: SortBySize, 5: SortByDimension, 6: SortByRating, 7: SortByAddedDate
    switch (m_sortType) {
        case 0: return compareNames(leftRec, rightRec);
        case 1:
            if (leftRec.ctime != rightRec.ctime) return leftRec.ctime < rightRec.ctime;
            return compareNames(leftRec, rightRec);
        case 2:
            if (leftRec.mtime != rightRec.mtime) return leftRec.mtime < rightRec.mtime;
            return compareNames(leftRec, rightRec);
        case 3: {
            int comp = leftRec.suffix.localeAwareCompare(rightRec.suffix);
            if (comp != 0) return comp < 0;
            return compareNames(leftRec, rightRec);
        }
        case 4: {
            long long lSize = leftRec.isDir ? -1 : leftRec.size;
            long long rSize = rightRec.isDir ? -1 : rightRec.size;
            if (lSize != rSize) return lSize < rSize;
            return compareNames(leftRec, rightRec);
        }
        case 5: {
            long long lDim = static_cast<long long>(leftRec.width) * leftRec.height;
            long long rDim = static_cast<long long>(rightRec.width) * rightRec.height;
            if (lDim != rDim) return lDim < rDim;
            return compareNames(leftRec, rightRec);
        }
        case 6:
            if (leftRec.rating != rightRec.rating) return leftRec.rating < rightRec.rating;
            return compareNames(leftRec, rightRec);
        case 7: {
            long long leftAdded = leftRec.added_at == 0 ? leftRec.ctime : leftRec.added_at;
            long long rightAdded = rightRec.added_at == 0 ? rightRec.ctime : rightRec.added_at;
            if (leftAdded != rightAdded) return leftAdded < rightAdded;
            return compareNames(leftRec, rightRec);
        }
        default:
            return compareNames(leftRec, rightRec);
    }
}

} // namespace QuarkMeta
