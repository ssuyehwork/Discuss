#ifndef GROUPINGPROXYMODEL_H 
#define GROUPINGPROXYMODEL_H 
 
#include <QAbstractProxyModel> 
#include <QString> 
#include <QVector> 
#include <QSet>
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
    enum Roles { 
        IsGroupHeaderRole = GroupRole::IsGroupHeaderRole, 
        GroupIdRole = GroupRole::GroupIdRole, 
        GroupTitleRole = GroupRole::GroupTitleRole, 
        GroupIsCollapsibleRole = GroupRole::GroupIsCollapsibleRole, 
        GroupIsCollapsedRole = GroupRole::GroupIsCollapsedRole, 
        GroupItemCountRole = GroupRole::GroupItemCountRole 
    }; 
 
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
