#pragma once

#include <QSortFilterProxyModel>
#include <QSet>
#include "../FilterPanel.h"
#include "../../core/ItemRecord.h"

namespace QuarkMeta {

class FolderProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit FolderProxyModel(QObject* parent = nullptr);
    ~FolderProxyModel() override = default;

    FilterState currentFilter;

    void updateFilter();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
};

} // namespace QuarkMeta
