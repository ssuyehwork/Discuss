#pragma once

#include <QObject>
#include <QAbstractItemView>
#include <QModelIndexList>
#include <QSet>
#include <QString>

namespace QuarkMeta {

class ContentPanel;
class DiskItemModel;

/**
 * @brief 视图与视口协调器：负责 ContentPanel 的多视图选区收集、视口几何探测与分组折叠高度维护
 */
class ContentViewCoordinator : public QObject {
    Q_OBJECT

public:
    explicit ContentViewCoordinator(ContentPanel* panel);
    ~ContentViewCoordinator() override = default;

    // 视图探测
    QList<QAbstractItemView*> currentActiveViews() const;
    QAbstractItemView* activeItemView() const;

    // 选区与焦点计算
    QModelIndexList getSelectedIndexes() const;
    void restoreSelections(const QSet<QString>& selectedPaths, bool isPendingEdit);

    // 缩略图视口行号探测与触发
    void refreshVisibleThumbnails();

    // 分区高度与统计同步（原样移植，零数值变动）
    void updateListSectionCounts();
    void updateGridSectionCounts();

    // 视图缩放几何适配
    void updateGridSize(int zoomLevel);

private:
    ContentPanel* m_panel = nullptr;
};

} // namespace QuarkMeta
