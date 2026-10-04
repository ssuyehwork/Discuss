#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ContentViewCoordinator.h"
#include "../ContentPanel.h"
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
            if (m_panel->columnView()->activePane()->listView()) {
                views << m_panel->columnView()->activePane()->listView();
            }
        }
    } else if (mode == ContentPanel::ListView) {
        if (m_panel->dropTreeView()) views << m_panel->dropTreeView();
    } else { // GridView / JustifiedViewMode
        if (m_panel->gridView()) views << m_panel->gridView();
    }
    return views;
}

QAbstractItemView* ContentViewCoordinator::activeItemView() const {
    if (!m_panel) return nullptr;

    auto mode = m_panel->currentViewMode();
    if (mode == ContentPanel::ColumnView) {
        if (m_panel->columnView() && m_panel->columnView()->activePane()) {
            return m_panel->columnView()->activePane()->listView();
        }
        return nullptr;
    }

    if (mode == ContentPanel::ListView) {
        return m_panel->dropTreeView();
    }

    return m_panel->gridView();
}

QSortFilterProxyModel* ContentViewCoordinator::getActiveProxyModel() const {
    if (!m_panel) return nullptr;

    QAbstractItemView* view = activeItemView();
    if (view && view->model()) {
        return qobject_cast<QSortFilterProxyModel*>(view->model());
    }
    return nullptr;
}

QModelIndexList ContentViewCoordinator::getSelectedIndexes() const {
    QModelIndexList res;
    if (!m_panel) return res;

    if (m_panel->currentViewMode() == ContentPanel::ColumnView) {
        if (m_panel->columnView() && m_panel->columnView()->activePane()) {
            for (auto* view : {m_panel->columnView()->activePane()->folderListView(), m_panel->columnView()->activePane()->listView()}) {
                if (view && view->selectionModel() && view->selectionModel()->hasSelection()) {
                    for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                        if (idx.column() == 0) res.append(idx);
                    }
                }
            }
        }
        return res;
    }

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

QStringList ContentViewCoordinator::getSelectedPaths() const {
    QStringList paths;
    for (const auto& idx : getSelectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) paths << p;
        }
    }
    return paths;
}

void ContentViewCoordinator::applyFilterStateToAllViews(const FilterState& state) {
    if (!m_panel) return;

    m_panel->applyFilters(state);
}

void ContentViewCoordinator::restoreSelections(const QSet<QString>& selectedPaths, bool isPendingEdit) {
    if (!m_panel || selectedPaths.isEmpty()) return;

    if (m_panel->currentViewMode() == ContentPanel::ColumnView) {
        if (m_panel->columnView() && m_panel->columnView()->rightmostPane()) {
            m_panel->columnView()->rightmostPane()->setPendingSelectPaths(selectedPaths, isPendingEdit);
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
            if (lastIdx.isValid()) {
                view->scrollTo(lastIdx);
                if (isPendingEdit) {
                    QPointer<QAbstractItemView> weakView(view);
                    QTimer::singleShot(0, m_panel, [weakView, lastIdx]() {
                        if (weakView && lastIdx.isValid()) {
                            weakView->setFocus();
                            weakView->setCurrentIndex(lastIdx);
                            weakView->edit(lastIdx);
                        }
                    });
                }
            }
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

void ContentViewCoordinator::updateGridSize(int zoomLevel) {
    Q_UNUSED(zoomLevel);
}

void ContentViewCoordinator::installActivationFilters() {
    if (!m_panel) return;

    auto installOnce = [this](QObject* obj) {
        if (!obj || m_filteredObjects.contains(obj)) return;
        obj->installEventFilter(m_panel);
        m_filteredObjects.insert(obj);
    };

    for (QAbstractItemView* view : currentActiveViews()) {
        if (!view) continue;
        installOnce(view);
        installOnce(view->viewport());
    }
}

} // namespace QuarkMeta
