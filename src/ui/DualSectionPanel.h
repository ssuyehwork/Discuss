#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QLabel>
#include "models/FilterProxyModel.h"
#include "models/ItemModelBase.h"

namespace QuarkMeta {

class FolderSectionHeaderBar;
class FileSectionHeaderBar;

/**
 * @brief 文件夹区+文件区 双分区核心逻辑面板
 *
 * List / Grid / Column 三种视图模式共用的唯一实现：负责 header×2 + view×2 的排布、
 * 折叠联动、高度撑开（含"撑满宿主可视区剩余空间"）、选中聚合、可视区域缩略图按需刷新、
 * 筛选后全隐藏提示。
 *
 * 本类不创建 folderView/fileView（由外部按各自需要的具体类型创建后传入），
 * 也不拥有滚动能力（自己是普通QWidget）——滚动由宿主（一个QScrollArea）负责，
 * 本类只需要宿主在必要时告知"当前可视视口高度是多少"。
 */
class DualSectionPanel : public QWidget {
    Q_OBJECT
public:
    explicit DualSectionPanel(QAbstractItemView* folderView, QAbstractItemView* fileView,
                               FilterProxyModel* folderProxy, FilterProxyModel* fileProxy,
                               QWidget* parent = nullptr);
    ~DualSectionPanel() override = default;

    FolderSectionHeaderBar* folderHeader() const { return m_folderHeader; }
    FileSectionHeaderBar* fileHeader() const { return m_fileHeader; }
    QAbstractItemView* folderView() const { return m_folderView; }
    QAbstractItemView* fileView() const { return m_fileView; }

    // hostViewportHeight：宿主QScrollArea当前viewport()->height()，用于撑满剩余空间的计算
    void updateSectionCounts(int hostViewportHeight);
    void toggleFolderSectionCollapse();
    int fileViewMinHeight() const { return computeFileViewMinHeight(m_lastHostViewportHeight); }

    QAbstractItemView* activeItemView() const;
    QModelIndexList getSelectedIndexes() const;

    // hostViewport：宿主QScrollArea的viewport()，用于把物理可视矩形投影到子视图坐标系
    void refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport);

signals:
    void selectionChanged();
    void folderCollapseToggled(bool collapsed);

private:
    int computeFileViewMinHeight(int hostViewportHeight) const;
    void updateEmptyFilterHint();

    QVBoxLayout* m_layout = nullptr;
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    FileSectionHeaderBar* m_fileHeader = nullptr;
    QAbstractItemView* m_folderView = nullptr;
    QAbstractItemView* m_fileView = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
    QLabel* m_emptyFilterHintLabel = nullptr;
    int m_lastHostViewportHeight = 0;
};

} // namespace QuarkMeta
