#pragma once

#include <QSortFilterProxyModel>
#include <QSet>
#include <QString>
#include "../FilterPanel.h"
#include "../../core/ItemRecord.h"

namespace QuarkMeta {

/**
 * @brief 独立的高级多维条件过滤与加权排序代理模型
 */
class FilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit FilterProxyModel(QObject* parent = nullptr);
    ~FilterProxyModel() override = default;

    FilterState currentFilter;

    void updateFilter();
    void setCachedDuplicatePaths(const QSet<QString>& paths);

    void setSortType(int type) { m_sortType = type; invalidate(); }
    void setSortOrder(Qt::SortOrder order) { m_sortOrder = order; invalidate(); }

    void setGroupHeadersEnabled(bool enabled);
    bool groupHeadersEnabled() const { return m_groupHeadersEnabled; }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    QSet<QString> m_cachedDuplicatePaths;
    int m_sortType = 0;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    bool m_groupHeadersEnabled = false;
    bool m_foldersCollapsed = false;
    bool m_filesCollapsed = false;
};

} // namespace QuarkMeta
