#pragma once

#include <QScrollArea>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QSet>
#include "models/FilterProxyModel.h"
#include "models/ItemModelBase.h"

namespace QuarkMeta {

class FolderSectionHeaderBar;
class FileSectionHeaderBar;

/**
 * @brief 双分区协同撑开滚动画布：封装文件夹+文件双分区、折叠联动、全高撑开与真实物理视口探测
 */
class SectionedScrollCanvas : public QScrollArea {
    Q_OBJECT

public:
    enum class CanvasType {
        Grid,
        List
    };

    explicit SectionedScrollCanvas(CanvasType type, ItemModelBase* sourceModel, const FilterState& initialFilter, QObject* eventFilter = nullptr, QWidget* parent = nullptr);
    ~SectionedScrollCanvas() override = default;

    CanvasType canvasType() const { return m_type; }
    QAbstractItemView* folderView() const { return m_folderView; }
    QAbstractItemView* fileView() const { return m_fileView; }
    FolderSectionHeaderBar* folderHeader() const { return m_folderHeader; }
    FileSectionHeaderBar* fileHeader() const { return m_fileHeader; }
    FilterProxyModel* folderProxyModel() const { return m_folderProxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_fileProxyModel; }

    void setSourceModel(ItemModelBase* model);
    void applyFilter(const FilterState& filter);
    void applySort(int column, Qt::SortOrder order);
    void updateSectionCounts();
    void updateZoom(int zoomLevel);
    void toggleFolderSectionCollapse();

    // 选区与真实物理视口探测（彻底解决全高撑开后的卡顿问题）
    QAbstractItemView* activeItemView() const;
    QModelIndexList getSelectedIndexes() const;
    void refreshVisibleThumbnails(ItemModelBase* model);

signals:
    void selectionChanged();
    void doubleClicked(const QModelIndex& index);
    void customContextMenuRequested(const QPoint& pos);
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex, QAbstractItemModel* sourceProxy);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void initViews(ItemModelBase* sourceModel, QObject* eventFilter);
    void setupConnections();

    CanvasType m_type;
    QWidget* m_containerWidget = nullptr;
    QVBoxLayout* m_layout = nullptr;
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    FileSectionHeaderBar* m_fileHeader = nullptr;
    QAbstractItemView* m_folderView = nullptr;
    QAbstractItemView* m_fileView = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
};

} // namespace QuarkMeta
