#include "DropTreeView.h"
#include "ViewDragDropHelper.h"
#include "../core/ModelContract.h"
#include "ContentPanel.h"
#include <QPainter>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QAbstractProxyModel>
#include <QStringList>
#include <QFileInfo>
#include "Logger.h"

namespace QuarkMeta {

// 声明式规则表：单一真理来源 (Single Source of Truth)
static const std::vector<ColumnPolicy> kFileListColumnPolicies = {
    { FileListColumn::Name,         0,   QHeaderView::Stretch, 0,   false }, // 始终显示并拉伸
    { FileListColumn::Status,       40,  QHeaderView::Fixed,   0,   true  }, // 恒定隐藏
    { FileListColumn::Rating,       100, QHeaderView::Fixed,   350, false }, // >=350px
    { FileListColumn::Dimension,    100, QHeaderView::Fixed,   480, false }, // >=480px
    { FileListColumn::Type,         60,  QHeaderView::Fixed,   600, false }, // >=600px
    { FileListColumn::Size,         80,  QHeaderView::Fixed,   600, false }, // >=600px
    { FileListColumn::ModifiedDate, 130, QHeaderView::Fixed,   720, false }, // >=720px
};

DropTreeView::DropTreeView(QWidget* parent) : QTreeView(parent) {
    setHeader(new ContentHeaderView(Qt::Horizontal, this));
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);

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

void DropTreeView::dragEnterEvent(QDragEnterEvent* event) {
    if (!ViewDragDropHelper::handleDragEnter(this, event)) {
        QTreeView::dragEnterEvent(event);
    }
}

void DropTreeView::dragMoveEvent(QDragMoveEvent* event) {
    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        QTreeView::dragMoveEvent(event);
    }
}

void DropTreeView::dropEvent(QDropEvent* event) {
    QStringList paths;
    QModelIndex targetIdx;
    if (ViewDragDropHelper::handleDrop(this, event, paths, targetIdx)) {
        emit pathsDropped(paths, targetIdx);
    } else {
        QTreeView::dropEvent(event);
    }
}

void DropTreeView::startDrag(Qt::DropActions supportedActions) {
    ViewDragDropHelper::executeStartDrag(this, supportedActions);
}

void DropTreeView::applyColumnPolicies() {
    QHeaderView* hdr = header();
    if (!hdr) return;

    int currentWidth = viewport() ? viewport()->width() : width();

    for (const auto& policy : kFileListColumnPolicies) {
        int colIdx = static_cast<int>(policy.column);
        bool shouldHide = policy.alwaysHidden || (policy.minContainerWidth > 0 && currentWidth < policy.minContainerWidth);
        
        setColumnHidden(colIdx, shouldHide);
        hdr->setSectionResizeMode(colIdx, policy.resizeMode);
        if (policy.fixedWidth > 0) {
            hdr->resizeSection(colIdx, policy.fixedWidth);
        }
    }
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
