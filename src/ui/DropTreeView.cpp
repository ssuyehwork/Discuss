#include "DropTreeView.h"
#include "../core/ModelContract.h"
#include "ContentPanel.h"
#include <QDrag>
#include <QPainter>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QPixmap>
#include <QAbstractProxyModel>
#include <QMimeData>
#include <QUrl>
#include <QDir>
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
    if (event->source() != this && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeView::dragEnterEvent(event);
    }
}

void DropTreeView::dragMoveEvent(QDragMoveEvent* event) {
    if (event->source() != this && event->mimeData()->hasUrls()) {
        // 物理同步：显式调用基类逻辑以激活放置指示器 (Drop Indicator)
        QTreeView::dragMoveEvent(event);
        event->acceptProposedAction();
    } else {
        QTreeView::dragMoveEvent(event);
    }
}

void DropTreeView::dropEvent(QDropEvent* event) {
    if (event->source() != this && event->mimeData()->hasUrls()) {
        QStringList paths;
        for (const QUrl& u : event->mimeData()->urls()) {
            if (u.isLocalFile()) {
                paths << QDir::toNativeSeparators(u.toLocalFile());
            }
        }
        QModelIndex idx = indexAt(event->position().toPoint());
        if (!paths.isEmpty()) {
            emit pathsDropped(paths, idx);
        }
        event->acceptProposedAction();
    } else {
        QTreeView::dropEvent(event);
    }
}

void DropTreeView::startDrag(Qt::DropActions supportedActions) {
    QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty()) return;

    // 核心增强：拦截并注入物理路径 QUrl
    QMimeData* mimeData = model()->mimeData(indexes);
    if (!mimeData) {
        mimeData = new QMimeData();
    }
    QList<QUrl> urls;
    for (const QModelIndex& idx : indexes) {
        if (idx.column() != 0) continue;
        
        // 2026-06-xx 工业级增强：优先从 PathRole 提取以规避 ContentPanel 中的角色冲突 (UserRole+1 为 Rating)
        QString path;
        QVariant pathVar = idx.data(PathRole);
        if (pathVar.isValid()) {
            path = pathVar.toString();
        } else {
            path = idx.data(Qt::UserRole + 1).toString();
        }
        
        if (!path.isEmpty() && QFileInfo::exists(path)) {
            urls << QUrl::fromLocalFile(path);
        }
    }
    
    if (!urls.isEmpty()) {
        mimeData->setUrls(urls);
    }

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    
    // 物理还原：消除卡片快照干扰，使用 1x1 透明像素
    QPixmap pix(1, 1);
    pix.fill(Qt::transparent);
    drag->setPixmap(pix);
    drag->setHotSpot(QPoint(0, 0));
    
    drag->exec(supportedActions | Qt::CopyAction, Qt::MoveAction);
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
