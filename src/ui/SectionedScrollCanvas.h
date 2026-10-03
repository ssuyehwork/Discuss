#pragma once

#include <QScrollArea>
#include <QAbstractItemView>
#include <QVBoxLayout>
#include <QSet>
#include <QTimer>
#include "models/FilterProxyModel.h"
#include "models/ItemModelBase.h"

namespace QuarkMeta {

class FolderSectionHeaderBar;
class FileSectionHeaderBar;

/**
 * @brief 单视图组装滚动画布：List/Grid 专属外壳，直接持有单 unifiedView 与标头
 */
class SectionedScrollCanvas : public QScrollArea {
    Q_OBJECT

public:
    enum class CanvasType {
        Grid,
        List
    };

    explicit SectionedScrollCanvas(CanvasType type, FilterProxyModel* proxyModel, QObject* eventFilter = nullptr, QWidget* parent = nullptr);
    ~SectionedScrollCanvas() override = default;

    CanvasType canvasType() const { return m_type; }
    QAbstractItemView* unifiedView() const { return m_unifiedView; }

    // 契约锁保护：为现有调用方保留向下兼容接口
    QAbstractItemView* folderView() const { return m_unifiedView; }
    QAbstractItemView* fileView() const { return m_unifiedView; }
    FolderSectionHeaderBar* folderHeader() const { return m_folderHeader; }
    FileSectionHeaderBar* fileHeader() const { return m_fileHeader; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    FilterProxyModel* folderProxyModel() const { return m_proxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_proxyModel; }

    void updateSectionCounts();
    void updateZoom(int zoomLevel);
    void toggleFolderSectionCollapse();

    QAbstractItemView* activeItemView() const;
    QModelIndexList getSelectedIndexes() const;
    void refreshVisibleThumbnails(ItemModelBase* model);
    void triggerVisibleScan();

signals:
    void selectionChanged();
    void doubleClicked(const QModelIndex& index);
    void customContextMenuRequested(const QPoint& pos);
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex, QAbstractItemModel* sourceProxy);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QAbstractItemView* createUnifiedView(QObject* eventFilter);
    void setupConnections();

    CanvasType m_type;
    QWidget* m_containerWidget = nullptr;
    QVBoxLayout* m_containerLayout = nullptr;
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    FileSectionHeaderBar* m_fileHeader = nullptr;
    QAbstractItemView* m_unifiedView = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    QTimer* m_scrollThumbTimer = nullptr;
};

} // namespace QuarkMeta
