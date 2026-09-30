#include "DualSectionPanel.h"
#include "FolderSectionWidget.h"
#include "JustifiedView.h"
#include "Logger.h"
#include "../core/CoreController.h"
#include <QScrollArea>
#include <QScrollBar>
#include <QElapsedTimer>
#include <QDebug>

namespace QuarkMeta {

DualSectionPanel::DualSectionPanel(QAbstractItemView* folderView, QAbstractItemView* fileView,
                                    FilterProxyModel* folderProxy, FilterProxyModel* fileProxy,
                                    QWidget* parent)
    : QWidget(parent), m_folderView(folderView), m_fileView(fileView),
      m_folderProxyModel(folderProxy), m_fileProxyModel(fileProxy) {

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->setAlignment(Qt::AlignTop);

    m_folderHeader = new FolderSectionHeaderBar(this);
    m_folderHeader->hide();
    m_layout->addWidget(m_folderHeader, 0);

    if (m_folderView) {
        m_folderView->setParent(this);
        m_folderView->hide();
        m_layout->addWidget(m_folderView, 0);
    }

    m_fileHeader = new FileSectionHeaderBar(this);
    m_fileHeader->hide();
    m_layout->addWidget(m_fileHeader, 0);

    if (m_fileView) {
        m_fileView->setParent(this);
        m_layout->addWidget(m_fileView, 1);
    }

    // 🚀 筛选后全隐藏提示（原 ColumnViewPane 独有，现统一给三种视图）
    m_emptyFilterHintLabel = new QLabel(this);
    m_emptyFilterHintLabel->setAlignment(Qt::AlignCenter);
    m_emptyFilterHintLabel->setWordWrap(true);
    m_emptyFilterHintLabel->setStyleSheet("color: #888888; font-size: 12px; padding: 16px;");
    m_emptyFilterHintLabel->hide();
    m_layout->addWidget(m_emptyFilterHintLabel, 0);

    connect(m_folderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderView && m_folderHeader->count() > 0) {
            m_folderView->setVisible(!collapsed);
            updateSectionCounts(m_lastHostViewportHeight);
            emit folderCollapseToggled(collapsed);
        }
    });

    if (m_folderView && m_folderView->selectionModel()) {
        connect(m_folderView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &DualSectionPanel::selectionChanged);
    }
    if (m_fileView && m_fileView->selectionModel()) {
        connect(m_fileView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &DualSectionPanel::selectionChanged);
    }
}

int DualSectionPanel::computeFileViewMinHeight(int hostViewportHeight) const {
    int used = 0;
    if (m_folderHeader && m_folderHeader->isVisible()) used += m_folderHeader->height();
    if (m_folderView && m_folderView->isVisible()) used += m_folderView->height();
    if (m_fileHeader && m_fileHeader->isVisible()) used += m_fileHeader->height();
    return qMax(0, hostViewportHeight - used);
}

int DualSectionPanel::computeFolderViewMinHeight(int hostViewportHeight) const {
    int used = 0;
    if (m_folderHeader && m_folderHeader->isVisible()) used += m_folderHeader->height();
    return qMax(0, hostViewportHeight - used);
}

void DualSectionPanel::updateEmptyFilterHint() {
    if (!m_emptyFilterHintLabel || !m_folderProxyModel || !m_fileProxyModel) return;
    bool folderEmpty = m_folderProxyModel->rowCount() == 0;
    bool fileEmpty = m_fileProxyModel->rowCount() == 0;

    if (folderEmpty && fileEmpty) {
        m_emptyFilterHintLabel->setText("所有内容已被筛选隐藏");
        m_emptyFilterHintLabel->show();
    } else {
        m_emptyFilterHintLabel->hide();
    }
}

void DualSectionPanel::updateSectionCounts(int hostViewportHeight) {
    m_lastHostViewportHeight = hostViewportHeight;
    if (!m_folderProxyModel || !m_fileProxyModel) return;

    int folderCount = m_folderProxyModel->rowCount();
    int fileCount = m_fileProxyModel->rowCount();

    if (m_folderHeader) {
        m_folderHeader->setCount(folderCount);
        m_folderHeader->setVisible(folderCount > 0);
    }
    if (m_folderView) {
        if (folderCount == 0) {
            m_folderView->hide();
        } else if (!m_folderHeader->isCollapsed()) {
            m_folderView->show();
        }
    }

    if (m_fileHeader) {
        m_fileHeader->setCount(fileCount);
        m_fileHeader->setVisible(fileCount > 0 && folderCount > 0);
    }
    if (m_fileView) {
        if (fileCount == 0) {
            m_fileView->hide();
        } else {
            m_fileView->show();
        }
    }

    updateEmptyFilterHint();
}

void DualSectionPanel::toggleFolderSectionCollapse() {
    if (m_folderHeader && m_folderHeader->isVisible() && m_folderHeader->count() > 0) {
        m_folderHeader->setCollapsed(!m_folderHeader->isCollapsed());
    }
}

QAbstractItemView* DualSectionPanel::activeItemView() const {
    if (m_folderView && (m_folderView->hasFocus() ||
        (m_folderView->selectionModel() && m_folderView->selectionModel()->hasSelection()))) {
        return m_folderView;
    }
    return m_fileView;
}

QModelIndexList DualSectionPanel::getSelectedIndexes() const {
    QModelIndexList res;
    for (auto* view : {m_folderView, m_fileView}) {
        if (view && view->selectionModel() && view->selectionModel()->hasSelection()) {
            for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                if (idx.column() == 0) res.append(idx);
            }
        }
    }
    return res;
}

void DualSectionPanel::refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport) {
    if (!model || !hostViewport || CoreController::isShuttingDown()) return;

    auto* scrollArea = qobject_cast<QScrollArea*>(hostViewport->parent());
    int scrollY = (scrollArea && scrollArea->verticalScrollBar()) ? scrollArea->verticalScrollBar()->value() : 0;
    int vpHeight = hostViewport->height();

    QSet<int> visibleRows;

    auto scanView = [&](QAbstractItemView* view, FilterProxyModel* proxy) {
        if (!view || !view->isVisible() || !proxy || proxy->rowCount() == 0) return;

        QString viewTag = (view == m_folderView) ? "FolderView" : "FileView";
        auto* jv = qobject_cast<JustifiedView*>(view);

        if (jv) {
            if (!jv->isLayoutReady()) {
                qDebug().noquote() << QString("[THUMB_TRACE] [%1] Geometry not ready (dirty or unpopulated), scan deferred.").arg(viewTag);
                return;
            }

            int topInContent = scrollY - view->y();
            int bottomInContent = topInContent + vpHeight;

            QList<int> proxyRows = jv->rowsInRange(topInContent, bottomInContent);
            if (proxyRows.isEmpty()) {
                qDebug().noquote() << QString("[THUMB_TRACE] [%1] Geometry scan: no intersecting rows in range [%2, %3].")
                    .arg(viewTag).arg(topInContent).arg(bottomInContent);
                return;
            }

            for (int r : proxyRows) {
                QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
                if (srcIdx.isValid()) {
                    visibleRows.insert(srcIdx.row());
                } else {
                    qDebug().noquote() << QString("[THUMB_TRACE] [%1] Skip row %2: invalid source index.").arg(viewTag).arg(r);
                }
            }

            qDebug().noquote() << QString("[THUMB_TRACE] [%1] Geometry scan: visible rows [%2, %3], total visible: %4")
                .arg(viewTag).arg(proxyRows.first()).arg(proxyRows.last()).arg(proxyRows.size());
        } else {
            // 传统 TreeView 换算
            int topInContent = scrollY - view->y();
            int bottomInContent = topInContent + vpHeight;

            int firstVisible = -1, lastVisible = -1, visibleCount = 0;
            int rowCount = proxy->rowCount();

            for (int r = 0; r < rowCount; ++r) {
                QModelIndex pIdx = proxy->index(r, 0);
                QRect rRect = view->visualRect(pIdx);

                if (!rRect.isValid() || rRect.isEmpty()) {
                    qDebug().noquote() << QString("[THUMB_TRACE] [%1] Skip row %2: invalid visualRect.").arg(viewTag).arg(r);
                    continue;
                }

                if (rRect.bottom() < topInContent) continue;
                if (rRect.top() > bottomInContent) break;

                if (firstVisible == -1) firstVisible = r;
                lastVisible = r;
                visibleCount++;

                QModelIndex srcIdx = proxy->mapToSource(pIdx);
                if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
            }

            qDebug().noquote() << QString("[THUMB_TRACE] [%1] Geometry scan: visible rows [%2, %3], total visible: %4")
                .arg(viewTag).arg(firstVisible).arg(lastVisible).arg(visibleCount);
        }
    };

    scanView(m_folderView, m_folderProxyModel);
    scanView(m_fileView, m_fileProxyModel);

    if (!visibleRows.isEmpty()) {
        qDebug() << "[THUMB_TRACE] DualSectionPanel::refreshVisibleThumbnails - Submitting" << visibleRows.size() << "rows to loadThumbnailsForRows.";
        model->loadThumbnailsForRows(visibleRows.values());
    } else {
        qDebug() << "[THUMB_TRACE] DualSectionPanel::refreshVisibleThumbnails - No visible rows found in viewport sampling.";
    }
}

} // namespace QuarkMeta