#include "DropTreeView.h"
#include "ViewDragDropHelper.h"
#include "models/SectionProxyModel.h"
#include "../core/ModelContract.h"
#include "ContentPanel.h"
#include <QPainter>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QAbstractProxyModel>
#include <QScrollBar>
#include <QStringList>
#include <QFileInfo>
#include "Logger.h"

namespace QuarkMeta {

// 声明式规则表：单一真理来源 (Single Source of Truth)
static const std::vector<ColumnPolicy> kFileListColumnPolicies = {
    { FileListColumn::Name,         0,   QHeaderView::Fixed, false },
    { FileListColumn::Status,       40,  QHeaderView::Fixed, true  },
    { FileListColumn::Rating,       100, QHeaderView::Fixed, false },
    { FileListColumn::Dimension,    100, QHeaderView::Fixed, false },
    { FileListColumn::Type,         60,  QHeaderView::Fixed, false },
    { FileListColumn::Size,         80,  QHeaderView::Fixed, false },
    { FileListColumn::CreatedDate,  130, QHeaderView::Fixed, false },
    { FileListColumn::ModifiedDate, 130, QHeaderView::Fixed, false },
};

DropTreeView::DropTreeView(QWidget* parent) : QTreeView(parent) {
    setHeader(new ContentHeaderView(Qt::Horizontal, this));
    setDragEnabled(true);
    setDropIndicatorShown(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    DragDropEventFilter::install(this);

    // 🚀【强力锁定 QPalette】：强制设定暗色 Base 与 AlternateBase，防止原生 Windows 调色板在交替行露白
    QPalette pal = palette();
    pal.setColor(QPalette::Base, QColor("#1E1E1E"));
    pal.setColor(QPalette::AlternateBase, QColor("#252526"));
    pal.setColor(QPalette::Text, QColor("#EEEEEE"));
    pal.setColor(QPalette::WindowText, QColor("#EEEEEE"));
    setPalette(pal);
    if (viewport()) {
        viewport()->setPalette(pal);
    }
}


void DropTreeView::setModel(QAbstractItemModel* model) {
    if (this->model()) {
        disconnect(this->model(), &QAbstractItemModel::modelReset, this, &DropTreeView::updateGroupHeaderSpanning);
        disconnect(this->model(), &QAbstractItemModel::layoutChanged, this, &DropTreeView::updateGroupHeaderSpanning);
        disconnect(this->model(), &QAbstractItemModel::rowsInserted, this, &DropTreeView::updateGroupHeaderSpanning);
    }
    QTreeView::setModel(model);
    if (model) {
        connect(model, &QAbstractItemModel::modelReset, this, &DropTreeView::updateGroupHeaderSpanning);
        connect(model, &QAbstractItemModel::layoutChanged, this, &DropTreeView::updateGroupHeaderSpanning);
        connect(model, &QAbstractItemModel::rowsInserted, this, &DropTreeView::updateGroupHeaderSpanning);
        updateGroupHeaderSpanning();
    }
}

void DropTreeView::updateGroupHeaderSpanning() {
    if (!model()) return;
    int total = model()->rowCount();
    for (int r = 0; r < total; ++r) {
        QModelIndex idx = model()->index(r, 0);
        if (idx.data(SectionHeaderRole).toBool()) {
            setFirstColumnSpanned(r, QModelIndex(), true);
        }
    }
}

void DropTreeView::mousePressEvent(QMouseEvent* event) {
    QModelIndex idx = indexAt(event->pos());
    if (idx.isValid() && idx.data(SectionHeaderRole).toBool()) {
        if (event->button() == Qt::LeftButton) {
            if (idx.data(SectionKindRole).toInt() == 1) {
                SectionProxyModel* secModel = nullptr;
                QAbstractItemModel* cur = model();
                while (cur) {
                    secModel = qobject_cast<SectionProxyModel*>(cur);
                    if (secModel) break;
                    auto* proxy = qobject_cast<QAbstractProxyModel*>(cur);
                    if (proxy) cur = proxy->sourceModel();
                    else break;
                }
                if (secModel) {
                    secModel->setFolderCollapsed(!secModel->isFolderCollapsed());
                }
            }
        }
        event->accept();
        return;
    }
    QTreeView::mousePressEvent(event);
}

void DropTreeView::startDrag(Qt::DropActions supportedActions) {
    ViewDragDropHelper::executeStartDrag(this, supportedActions);
}

void DropTreeView::applyColumnPolicies() {
    if (m_isApplyingPolicies) return;
    m_isApplyingPolicies = true;

    QHeaderView* hdr = header();
    if (!hdr) {
        m_isApplyingPolicies = false;
        return;
    }

    hdr->setMinimumSectionSize(0);

    int totalVisibleFixedWidth = 0;
    for (const auto& policy : kFileListColumnPolicies) {
        if (policy.alwaysHidden) continue;
        if (policy.column != FileListColumn::Name) {
            totalVisibleFixedWidth += policy.fixedWidth;
        }
    }

    int availWidth = viewport() ? viewport()->width() : width();
    int minPaneW = ContentPanel::kMinPaneWidth;
    int calculatedNameWidth = std::max(minPaneW, availWidth - totalVisibleFixedWidth);

    for (const auto& policy : kFileListColumnPolicies) {
        int colIdx = static_cast<int>(policy.column);
        if (policy.alwaysHidden) {
            setColumnHidden(colIdx, true);
            continue;
        }

        setColumnHidden(colIdx, false);
        hdr->setSectionResizeMode(colIdx, QHeaderView::Fixed);

        if (policy.column == FileListColumn::Name) {
            hdr->resizeSection(colIdx, calculatedNameWidth);
        } else {
            hdr->resizeSection(colIdx, policy.fixedWidth);
        }
    }

    m_isApplyingPolicies = false;
}

void DropTreeView::resizeEvent(QResizeEvent* event) {
    QTreeView::resizeEvent(event);
    applyColumnPolicies();
}

void DropTreeView::keyboardSearch(const QString& search) {
    Q_UNUSED(search);
}

void DropTreeView::paintEvent(QPaintEvent* event) {
    QTreeView::paintEvent(event);
    if (!m_emptyHint.isEmpty() && model() && model()->rowCount() == 0) {
        QPainter painter(viewport());
        painter.save();
        painter.setPen(QColor("#888888"));
        painter.setFont(QFont("Microsoft YaHei", 12));
        painter.drawText(viewport()->rect(), Qt::AlignCenter, m_emptyHint);
        painter.restore();
    }
}

} // namespace QuarkMeta
