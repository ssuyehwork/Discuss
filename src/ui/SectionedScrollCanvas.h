#pragma once

#include <QScrollArea>
#include <QAbstractItemView>
#include <QSet>
#include <QTimer>
#include "models/FilterProxyModel.h"
#include "models/ItemModelBase.h"

namespace QuarkMeta {

class FolderSectionHeaderBar;
class FileSectionHeaderBar;
class DualSectionPanel;

/**
 * @brief 双分区协同撑开滚动画布：List/Grid 专属外壳，内部持有唯一的 DualSectionPanel 核心逻辑
 */
class SectionedScrollCanvas : public QScrollArea {
    Q_OBJECT

public:
    enum class CanvasType {
        Grid,
        List
    };

    explicit SectionedScrollCanvas(CanvasType type, FilterProxyModel* folderProxy, FilterProxyModel* fileProxy, QObject* eventFilter = nullptr, QWidget* parent = nullptr);
    ~SectionedScrollCanvas() override = default;

    CanvasType canvasType() const { return m_type; }
    QAbstractItemView* folderView() const;
    QAbstractItemView* fileView() const;
    FolderSectionHeaderBar* folderHeader() const;
    FileSectionHeaderBar* fileHeader() const;
    FilterProxyModel* folderProxyModel() const { return m_folderProxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_fileProxyModel; }

    void updateSectionCounts();
    void updateZoom(int zoomLevel);
    void toggleFolderSectionCollapse();

    QAbstractItemView* activeItemView() const;
    QModelIndexList getSelectedIndexes() const;
    void refreshVisibleThumbnails(ItemModelBase* model);

signals:
    void selectionChanged();
    void doubleClicked(const QModelIndex& index);
    void customContextMenuRequested(const QPoint& pos);
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex, QAbstractItemModel* sourceProxy);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QAbstractItemView* createFolderView(QObject* eventFilter);
    QAbstractItemView* createFileView(QObject* eventFilter);
    void setupConnections();

    CanvasType m_type;
    DualSectionPanel* m_panel = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
    QTimer* m_scrollThumbTimer = nullptr;
};

} // namespace QuarkMeta
