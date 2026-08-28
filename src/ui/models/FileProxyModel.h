#pragma once

#include <QSortFilterProxyModel>
#include <QSet>
#include "../FilterPanel.h"
#include "../../core/ItemRecord.h"

namespace QuarkMeta {

class FileProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit FileProxyModel(QObject* parent = nullptr);
    ~FileProxyModel() override = default;

    FilterState currentFilter;

    void updateFilter();
    void setCachedDuplicatePaths(const QSet<QString>& paths);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    QSet<QString> m_cachedDuplicatePaths;
};

} // namespace QuarkMeta
