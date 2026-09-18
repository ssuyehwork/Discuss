#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ContentViewCoordinator.h"
#include "../ContentPanel.h"
#include "../FolderSectionWidget.h"
#include "../DropJustifiedView.h"
#include "../DropTreeView.h"
#include "../DropListView.h"
#include "../ColumnViewWidget.h"
#include "../JustifiedView.h"
#include "../models/DiskItemModel.h"
#include "../models/FilterProxyModel.h"
#include "../../core/CoreController.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QSignalBlocker>

namespace QuarkMeta {

ContentViewCoordinator::ContentViewCoordinator(ContentPanel* panel)
    : QObject(panel), m_panel(panel) {
}

QList<QAbstractItemView*> ContentViewCoordinator::currentActiveViews() const {
    QList<QAbstractItemView*> views;
    if (!m_panel) return views;

    auto mode = m_panel->currentViewMode();
    if (mode == ContentPanel::ColumnView) {
        if (m_panel->columnView() && m_panel->columnView()->activePane()) {
            if (m_panel->columnView()->activePane()->folderListView()) {
                views << m_panel->columnView()->activePane()->folderListView();
            }
            if (m_panel->columnView()->activePane()->listView()) {
                views << m_panel->columnView()->activePane()->listView();
            }
        }
    } else if (mode == ContentPanel::ListView) {
        if (m_panel->m_folderTreeView) views << m_panel->m_folderTreeView;
        if (m_panel->m_treeView) views << m_panel->m_treeView;
    } else { // GridView / JustifiedViewMode
        if (m_panel->m_folderGridView) views << m_panel->m_folderGridView;
        if (m_panel->m_gridView) views << m_panel->m_gridView;
    }
    return views;
}

QAbstractItemView* ContentViewCoordinator::activeItemView() const {
    if (!m_panel) return nullptr;

    auto mode = m_panel->currentViewMode();
    if (mode == ContentPanel::ColumnView) {
        if (m_panel->columnView() && m_panel->columnView()->activePane()) {
            DropListView* folderV = m_panel->columnView()->activePane()->folderListView();
            if (folderV && (folderV->hasFocus() || (folderV->selectionModel() && folderV->selectionModel()->hasSelection()))) {
                return folderV;
            }
            return m_panel->columnView()->activePane()->listView();
        }
        return nullptr;
    }

    if (mode == ContentPanel::ListView) {
        if (m_panel->m_folderTreeView && (m_panel->m_folderTreeView->hasFocus() ||
            (m_panel->m_folderTreeView->selectionModel() && m_panel->m_folderTreeView->selectionModel()->hasSelection()))) {
            return m_panel->m_folderTreeView;
        }
        return m_panel->m_treeView;
    }

    // GridView / JustifiedViewMode
    if (m_panel->m_folderGridView && (m_panel->m_folderGridView->hasFocus() ||
        (m_panel->m_folderGridView->selectionModel() && m_panel->m_folderGridView->selectionModel()->hasSelection()))) {
        return m_panel->m_folderGridView;
    }
    return m_panel->m_gridView;
}

QModelIndexList ContentViewCoordinator::getSelectedIndexes() const {
    QModelIndexList res;
    QList<QAbstractItemView*> views = currentActiveViews();
    for (auto* view : views) {
        if (view && view->selectionModel() && view->selectionModel()->hasSelection()) {
            for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                if (idx.column() == 0) {
                    res.append(idx);
                }
            }
        }
    }
    return res;
}

void ContentViewCoordinator::restoreSelections(const QSet<QString>& selectedPaths, bool isPendingEdit) {
    if (!m_panel || selectedPaths.isEmpty()) return;

    if (m_panel->currentViewMode() == ContentPanel::ColumnView) {
        if (m_panel->columnView() && m_panel->columnView()->rightmostPane()) {
            m_panel->columnView()->rightmostPane()->setPendingSelectPaths(selectedPaths);
        }
        return;
    }

    QList<QAbstractItemView*> views = currentActiveViews();
    for (auto* view : views) {
        if (!view || !view->selectionModel()) continue;
        QSortFilterProxyModel* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!proxy) proxy = m_panel->getActiveProxyModel();
        DiskItemModel* diskModel = m_panel->diskModel();

        if (diskModel && proxy) {
            QSignalBlocker blocker(view->selectionModel());
            QItemSelection sel;
            QModelIndex lastIdx;
            const auto& recs = diskModel->allRecords();
            for (size_t i = 0; i < recs.size(); ++i) {
                if (selectedPaths.contains(recs[i].path)) {
                    QModelIndex pIdx = proxy->mapFromSource(diskModel->index(static_cast<int>(i), 0));
                    if (pIdx.isValid()) { sel.select(pIdx, pIdx); lastIdx = pIdx; }
                }
            }
            view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastIdx.isValid()) { view->scrollTo(lastIdx); if (isPendingEdit) view->edit(lastIdx); }
        }
    }
}

void ContentViewCoordinator::refreshVisibleThumbnails() {
    if (!m_panel || !m_panel->model() || CoreController::isShuttingDown()) return;

    QList<QAbstractItemView*> views = currentActiveViews();
    QSet<int> visibleRows;

    for (auto* view : views) {
        if (!view || !view->viewport()) continue;
        auto* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!proxy || proxy->rowCount() == 0) continue;

        QRect vpRect = view->viewport()->rect();
        QModelIndex topIdx = view->indexAt(vpRect.topLeft());
        QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

        // 绝对照抄原数值：缓冲前后 4 行
        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 4) : proxy->rowCount() - 1;

        for (int r = top; r <= bottom; ++r) {
            QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
    }

    if (!visibleRows.isEmpty()) {
        m_panel->model()->loadThumbnailsForRows(visibleRows.values());
    }
}

void ContentViewCoordinator::updateListSectionCounts() {
    if (!m_panel || !m_panel->m_folderProxyModel || !m_panel->m_fileProxyModel) return;
    int folderCount = m_panel->m_folderProxyModel->rowCount();
    int fileCount = m_panel->m_fileProxyModel->rowCount();

    if (m_panel->m_listFolderHeader) {
        m_panel->m_listFolderHeader->setCount(folderCount);
        m_panel->m_listFolderHeader->setVisible(folderCount > 0);
    }
    if (m_panel->m_folderTreeView) {
        if (folderCount == 0) {
            m_panel->m_folderTreeView->hide();
        } else {
            bool collapsed = m_panel->m_listFolderHeader ? m_panel->m_listFolderHeader->isCollapsed() : false;
            m_panel->m_folderTreeView->setVisible(!collapsed);
            // 绝对照抄原数值：sizeHintForRow 默认 30，偏移量 + 2
            int rowH = m_panel->m_folderTreeView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 30;
            int hdrH = (m_panel->m_folderTreeView->header() && m_panel->m_folderTreeView->header()->isVisible()) ? m_panel->m_folderTreeView->header()->height() : 0;
            int folderH = folderCount * rowH + hdrH + 2;
            m_panel->m_folderTreeView->setFixedHeight(folderH);
        }
    }
    if (m_panel->m_listFileHeader) {
        m_panel->m_listFileHeader->setCount(fileCount);
        m_panel->m_listFileHeader->setVisible(fileCount > 0 && folderCount > 0);
    }
    // 补齐 AllViewsCoExpansion.md 核心契约：文件列表视图全高撑开（绝对照抄行高与边距参数）
    if (m_panel->m_treeView) {
        if (fileCount == 0) {
            m_panel->m_treeView->hide();
        } else {
            m_panel->m_treeView->show();
            int rowH = m_panel->m_treeView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 30;
            int hdrH = (m_panel->m_treeView->header() && m_panel->m_treeView->header()->isVisible()) ? m_panel->m_treeView->header()->height() : 0;
            int fileH = fileCount * rowH + hdrH + 2;
            m_panel->m_treeView->setFixedHeight(fileH);
        }
    }
}

void ContentViewCoordinator::updateGridSectionCounts() {
    if (!m_panel || !m_panel->m_folderProxyModel || !m_panel->m_fileProxyModel) return;
    int folderCount = m_panel->m_folderProxyModel->rowCount();
    int fileCount = m_panel->m_fileProxyModel->rowCount();

    if (m_panel->m_gridFolderHeader) {
        m_panel->m_gridFolderHeader->setCount(folderCount);
        m_panel->m_gridFolderHeader->setVisible(folderCount > 0);
    }
    if (m_panel->m_folderGridView) {
        if (folderCount == 0) {
            m_panel->m_folderGridView->hide();
        } else {
            bool collapsed = m_panel->m_gridFolderHeader ? m_panel->m_gridFolderHeader->isCollapsed() : false;
            m_panel->m_folderGridView->setVisible(!collapsed);
            if (auto* fjv = qobject_cast<JustifiedView*>(m_panel->m_folderGridView)) {
                m_panel->m_folderGridView->setFixedHeight(fjv->totalHeight());
            }
        }
    }
    if (m_panel->m_gridFileHeader) {
        m_panel->m_gridFileHeader->setCount(fileCount);
        m_panel->m_gridFileHeader->setVisible(fileCount > 0 && folderCount > 0);
    }
    // 补齐 AllViewsCoExpansion.md 核心契约：文件网格视图全高撑开
    if (m_panel->m_gridView) {
        if (fileCount == 0) {
            m_panel->m_gridView->hide();
        } else {
            m_panel->m_gridView->show();
            if (auto* jv = qobject_cast<JustifiedView*>(m_panel->m_gridView)) {
                m_panel->m_gridView->setFixedHeight(jv->totalHeight());
            }
        }
    }
}

void ContentViewCoordinator::updateGridSize(int zoomLevel) {
    if (!m_panel || !m_panel->m_viewStack) return;

    if (m_panel->m_viewStack->currentWidget() == m_panel->m_gridScrollArea) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_panel->m_gridView)) {
            jv->setTargetRowHeight(zoomLevel);
        }
        if (auto* fjv = qobject_cast<JustifiedView*>(m_panel->m_folderGridView)) {
            fjv->setTargetRowHeight(zoomLevel);
        }
    } else if (m_panel->m_viewStack->currentWidget() == m_panel->m_listScrollArea) {
        if (auto* dropTree = qobject_cast<DropTreeView*>(m_panel->m_treeView)) {
            if (auto* hdr = qobject_cast<ContentHeaderView*>(dropTree->header())) {
                hdr->setZoomLevel(zoomLevel);
            }
        }
        // 绝对照抄原数值：qMax(16, zoomLevel - 8)
        if (m_panel->m_treeView) {
            m_panel->m_treeView->setIconSize(QSize(qMax(16, zoomLevel - 8), qMax(16, zoomLevel - 8)));
            m_panel->m_treeView->doItemsLayout();
        }
    }
}

} // namespace QuarkMeta
