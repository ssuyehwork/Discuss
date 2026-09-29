#include "DualSectionPanel.h"
#include "FolderSectionWidget.h"
#include "Logger.h"
#include "../core/CoreController.h"
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
    // 具体"隐藏了多少项"的判断逻辑跟原 ColumnViewPane 实现保持一致，
    // 这里先给出统一入口，实际隐藏计数逻辑在接入阶段核对原实现后补全。
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
    // 注意：具体的"文件区固定高度撑开"数值计算（Grid用JustifiedView::totalHeight()，
    // List/Column用行数×行高），因为跟具体view类型（JustifiedView vs QTreeView vs QListView）
    // 强相关，保留在 SectionedScrollCanvas / ColumnViewPane 各自的
    // totalHeightChanged信号连接、以及本函数调用之后各自追加一行 setFixedHeight(qMax(计算值, computeFileViewMinHeight(...)))，
    // 不在本类里做，避免这个共享类反而要认识三种不同的view子类。
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

    QRect hostVpRect = hostViewport->rect();
    QSet<int> visibleRows;

    auto scanView = [&](QAbstractItemView* view, FilterProxyModel* proxy) {
        if (!view || !view->isVisible() || !view->viewport() || !proxy || proxy->rowCount() == 0) return;

        QWidget* subVp = view->viewport();
        // 将外层 QScrollArea 视口矩形投影到子视图真实的 viewport 坐标系
        QPoint topPoint = subVp->mapFromGlobal(hostViewport->mapToGlobal(hostVpRect.topLeft()));
        QPoint btmPoint = subVp->mapFromGlobal(hostViewport->mapToGlobal(hostVpRect.bottomRight()));

        // 完全在可视区域之外时直接跳过
        if (topPoint.y() >= subVp->height() || btmPoint.y() <= 0) return;

        // 限制在子视图可视区域内，并避开极端边缘 padding
        int sampleTopY = qBound(0, topPoint.y(), subVp->height() - 1);
        int sampleBtmY = qBound(0, btmPoint.y(), subVp->height() - 1);
        int sampleLeftX = qBound(0, qMax(topPoint.x(), 12), subVp->width() - 1);
        int sampleRightX = qBound(0, qMin(btmPoint.x(), subVp->width() - 12), subVp->width() - 1);

        QModelIndex topIdx = view->indexAt(QPoint(sampleLeftX, sampleTopY));
        QModelIndex btmIdx = view->indexAt(QPoint(sampleRightX, sampleBtmY));

        // 若直接采样点位于卡片间隙，向中心采样
        if (!topIdx.isValid()) {
            topIdx = view->indexAt(QPoint(subVp->width() / 2, sampleTopY));
        }
        if (!btmIdx.isValid()) {
            btmIdx = view->indexAt(QPoint(subVp->width() / 2, sampleBtmY));
        }

        int top = -1;
        int bottom = -1;

        if (topIdx.isValid()) {
            top = topIdx.row();
        }
        if (btmIdx.isValid()) {
            bottom = btmIdx.row();
        }

        // 🚨【关键修复】：若采样的边缘坐标刚好命中卡片间隙（padding），向中心步进扫描寻找视口内第一个/最后一个有效索引
        if (top == -1 || bottom == -1) {
            int stepY = 16;
            int currentY = sampleTopY;
            while (top == -1 && currentY <= sampleBtmY) {
                QModelIndex idx = view->indexAt(QPoint(subVp->width() / 2, currentY));
                if (idx.isValid()) {
                    top = idx.row();
                }
                currentY += stepY;
            }

            currentY = sampleBtmY;
            while (bottom == -1 && currentY >= sampleTopY) {
                QModelIndex idx = view->indexAt(QPoint(subVp->width() / 2, currentY));
                if (idx.isValid()) {
                    bottom = idx.row();
                }
                currentY -= stepY;
            }
        }

        // 只有当视口内确实扫描到有效的卡片行时，才计算加载范围（前扩 4 行，后扩 4 行缓冲）；
        // 如果视图完全为空或不在视口内部，绝对严禁退化为 0 ~ rowCount()-1 全量加载！
        if (top != -1 && bottom != -1) {
            int clampedTop = qMax(0, top - 4);
            int clampedBottom = qMin(proxy->rowCount() - 1, bottom + 4);
            for (int r = clampedTop; r <= clampedBottom; ++r) {
                QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
                if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
            }
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
