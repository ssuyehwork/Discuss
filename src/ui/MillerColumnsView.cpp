#include "MillerColumnsView.h"
#include "models/DiskItemModel.h"
#include "models/FilterProxyModel.h"
#include "../core/ModelContract.h"
#include "../core/DiskScanService.h"
#include "../meta/MetaCacheDecorator.h"
#include "UiHelper.h"
#include "StyleLibrary.h"

#include <QPainter>
#include <QScrollBar>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QtConcurrent>

namespace QuarkMeta {

// =========================================================
// 1. MillerColumnDelegate: 精致渲染右箭头 (>) 与文件/目录图标
// =========================================================

MillerColumnDelegate::MillerColumnDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

QSize MillerColumnDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
    return QSize(220, 28);
}

void MillerColumnDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    bool isSelected = (option.state & QStyle::State_Selected);
    bool isHover = (option.state & QStyle::State_MouseOver);

    // 1. 背景底色
    if (isSelected) {
        painter->fillRect(option.rect, QColor("#094771")); // VS Code / Finder 风格深蓝高亮
    } else if (isHover) {
        painter->fillRect(option.rect, QColor("#2A2D2E"));
    }

    // 2. 数据提取
    QString type = index.data(TypeRole).toString();
    QString path = index.data(PathRole).toString();
    bool isFolder = (type == "folder") || QFileInfo(path).isDir();
    QString name = index.data(Qt::DisplayRole).toString();

    // 3. 绘制图标 (16x16)
    int iconSize = 16;
    int leftMargin = 10;
    QRect iconRect(option.rect.left() + leftMargin, option.rect.top() + (option.rect.height() - iconSize) / 2, iconSize, iconSize);

    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (icon.isNull()) {
        icon = isFolder ? UiHelper::getIcon("folder_filled", Style::BrandOrange) : UiHelper::getIcon("file", QColor("#CCCCCC"));
    }
    icon.paint(painter, iconRect);

    // 4. 绘制文件夹右侧级联展开箭头 (>)
    int rightMargin = 8;
    if (isFolder) {
        int arrowSize = 12;
        QRect arrowRect(option.rect.right() - rightMargin - arrowSize, option.rect.top() + (option.rect.height() - arrowSize) / 2, arrowSize, arrowSize);
        QIcon arrowIcon = UiHelper::getIcon("chevron_right", isSelected ? QColor("#FFFFFF") : QColor("#777777"), arrowSize);
        arrowIcon.paint(painter, arrowRect);
        rightMargin += (arrowSize + 4);
    }

    // 5. 绘制文件/目录名称
    QRect textRect = option.rect;
    textRect.setLeft(iconRect.right() + 8);
    textRect.setRight(option.rect.right() - rightMargin);

    painter->setPen(isSelected ? QColor("#FFFFFF") : QColor("#CCCCCC"));
    QFont f = option.font;
    f.setPointSize(9);
    painter->setFont(f);

    QFontMetrics fm(f);
    QString elidedText = fm.elidedText(name, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    painter->restore();
}


// =========================================================
// 2. MillerColumnPane: 独立纵列加载与选区响应
// =========================================================

MillerColumnPane::MillerColumnPane(const QString& path, QWidget* parent)
    : QWidget(parent), m_path(path) {
    initUi();
    loadDataAsync();
}

void MillerColumnPane::initUi() {
    setFixedWidth(230);
    setStyleSheet("QWidget { background-color: #1E1E1E; border-right: 1px solid #2B2B2B; }");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listView = new QListView(this);
    m_listView->setObjectName("MillerColumnListView");
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setItemDelegate(new MillerColumnDelegate(this));
    m_listView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_listView->setStyleSheet("QListView#MillerColumnListView { background: transparent; border: none; outline: none; }");

    m_diskModel = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_diskModel);
    m_listView->setModel(m_proxyModel);

    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        QModelIndexList selected = m_listView->selectionModel()->selectedIndexes();
        if (selected.isEmpty()) return;

        QModelIndex proxyIdx = selected.first();
        QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
        QString itemPath = srcIdx.data(PathRole).toString();
        bool isDir = (srcIdx.data(TypeRole).toString() == "folder") || QFileInfo(itemPath).isDir();

        emit itemSelected(proxyIdx, itemPath, isDir);
    });

    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        QModelIndex srcIdx = m_proxyModel->mapToSource(index);
        QString itemPath = srcIdx.data(PathRole).toString();
        if (!QFileInfo(itemPath).isDir()) {
            emit itemDoubleClicked(itemPath);
        }
    });

    layout->addWidget(m_listView);
}

void MillerColumnPane::loadDataAsync() {
    QString p = m_path;
    QPointer<MillerColumnPane> weakThis(this);

    (void)QtConcurrent::run([weakThis, p]() {
        std::function<bool()> cancelCheck = nullptr;
        std::vector<ItemRecord> records = DiskScanService::scanDirectory(p, false, cancelCheck);
        MetaCacheDecorator::decorate(records);

        QMetaObject::invokeMethod(qApp, [weakThis, records = std::move(records)]() mutable {
            if (!weakThis) return;
            weakThis->m_diskModel->setRecords(records);
        });
    });
}


// =========================================================
// 3. MillerColumnsView: 级联滚动控制器与选区同步穿透
// =========================================================

MillerColumnsView::MillerColumnsView(QWidget* parent)
    : QAbstractItemView(parent) {
    setObjectName("MillerColumnsView");
    setStyleSheet("QWidget#MillerColumnsView { background-color: #1E1E1E; border: none; }");

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("QScrollArea { background-color: #1E1E1E; border: none; }");

    m_containerWidget = new QWidget(m_scrollArea);
    m_containerWidget->setStyleSheet("QWidget { background-color: #1E1E1E; }");

    m_containerLayout = new QHBoxLayout(m_containerWidget);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_containerLayout->setSpacing(0);
    m_containerLayout->addStretch(1);

    m_scrollArea->setWidget(m_containerWidget);
    rootLayout->addWidget(m_scrollArea);
}

void MillerColumnsView::setRootPath(const QString& rootPath) {
    if (m_rootPath == rootPath && !m_panes.isEmpty()) return;
    m_rootPath = rootPath;

    truncateColumnsAfter(-1);
    if (!m_rootPath.isEmpty() && m_rootPath != "computer://") {
        appendColumn(m_rootPath, -1);
    }
}

void MillerColumnsView::appendColumn(const QString& folderPath, int parentPaneIndex) {
    truncateColumnsAfter(parentPaneIndex);

    MillerColumnPane* pane = new MillerColumnPane(folderPath, m_containerWidget);
    int currentPaneIndex = m_panes.size();
    m_panes.append(pane);

    m_containerLayout->insertWidget(m_containerLayout->count() - 1, pane);

    connect(pane, &MillerColumnPane::itemSelected, this, [this, currentPaneIndex, pane](const QModelIndex& proxyIdx, const QString& itemPath, bool isDir) {
        // 核心归一化：将活跃列的 Index 赋给本容器，打通 ContentPanel::getSelectedIndexes()
        m_currentActiveIndex = proxyIdx;
        if (selectionModel()) {
            selectionModel()->setCurrentIndex(proxyIdx, QItemSelectionModel::ClearAndSelect);
        }

        if (isDir) {
            appendColumn(itemPath, currentPaneIndex);
            emit directoryNavigated(itemPath);
        } else {
            truncateColumnsAfter(currentPaneIndex);
            emit fileSelected(itemPath);
        }
    });

    connect(pane, &MillerColumnPane::itemDoubleClicked, this, [this](const QString& itemPath) {
        emit fileActivated(itemPath);
        emit doubleClicked(m_currentActiveIndex);
    });

    scrollToRightmost();
}

void MillerColumnsView::truncateColumnsAfter(int paneIndex) {
    while (m_panes.size() > paneIndex + 1) {
        MillerColumnPane* pane = m_panes.takeLast();
        m_containerLayout->removeWidget(pane);
        pane->deleteLater();
    }
}

void MillerColumnsView::scrollToRightmost() {
    QTimer::singleShot(50, this, [this]() {
        if (m_scrollArea && m_scrollArea->horizontalScrollBar()) {
            m_scrollArea->horizontalScrollBar()->setValue(m_scrollArea->horizontalScrollBar()->maximum());
        }
    });
}

QModelIndexList MillerColumnsView::selectedIndexes() const {
    if (m_currentActiveIndex.isValid()) {
        return { m_currentActiveIndex };
    }
    return {};
}

// 虚拟契约实现
QRect MillerColumnsView::visualRect(const QModelIndex&) const { return QRect(); }
void MillerColumnsView::scrollTo(const QModelIndex&, ScrollHint) {}
QModelIndex MillerColumnsView::indexAt(const QPoint&) const { return m_currentActiveIndex; }
QModelIndex MillerColumnsView::moveCursor(CursorAction, Qt::KeyboardModifiers) { return m_currentActiveIndex; }
int MillerColumnsView::horizontalOffset() const { return 0; }
int MillerColumnsView::verticalOffset() const { return 0; }
bool MillerColumnsView::isIndexHidden(const QModelIndex&) const { return false; }
void MillerColumnsView::setSelection(const QRect&, QItemSelectionModel::SelectionFlags) {}
QRegion MillerColumnsView::visualRegionForSelection(const QItemSelection&) const { return QRegion(); }

} // namespace QuarkMeta