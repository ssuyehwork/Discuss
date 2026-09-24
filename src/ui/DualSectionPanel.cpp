#include "DualSectionPanel.h"
#include "FolderSectionWidget.h"
#include "Logger.h"
#include "../core/CoreController.h"
#include <QElapsedTimer>

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
        m_layout->addWidget(m_fileView, 0);
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

    QRect vpRect = hostViewport->rect();
    QSet<int> visibleRows;

    auto scanView = [&](QAbstractItemView* view, FilterProxyModel* proxy) {
        if (!view || !view->isVisible() || !proxy || proxy->rowCount() == 0) return;

        // 计算宿主 ScrollArea 视口映射到当前 item view 坐标系下的可视矩形
        QRect viewVpRect(
            view->mapFromGlobal(hostViewport->mapToGlobal(vpRect.topLeft())),
            view->mapFromGlobal(hostViewport->mapToGlobal(vpRect.bottomRight()))
        );

        int totalRows = proxy->rowCount();
        for (int r = 0; r < totalRows; ++r) {
            QModelIndex pIdx = proxy->index(r, 0);
            QRect itemRect = view->visualRect(pIdx);

            if (itemRect.isEmpty()) continue;

            // 只要卡片矩形与视图视口相交（无论在哪一列、瀑布流何种排布），均认定为可见！
            if (itemRect.intersects(viewVpRect)) {
                QModelIndex srcIdx = proxy->mapToSource(pIdx);
                if (srcIdx.isValid()) {
                    visibleRows.insert(srcIdx.row());
                }
            } else if (itemRect.top() > viewVpRect.bottom()) {
                // 已超出下方可视区域，由于布局自上而下，可提前结束遍历
                break;
            }
        }
    };

    scanView(m_folderView, m_folderProxyModel);
    scanView(m_fileView, m_fileProxyModel);

    if (!visibleRows.isEmpty()) {
        model->loadThumbnailsForRows(visibleRows.values());
    }
}

} // namespace QuarkMeta
