#include "ColumnViewWidget.h"
#include "ColumnItemDelegate.h"
#include "ContentPanel.h"
#include "../core/DiskScanService.h"
#include "../core/ModelContract.h"

#include <QVBoxLayout>
#include <QApplication>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrent>

namespace QuarkMeta {

// ---------------------------------------------------------------------------
// ColumnViewPane Implementation
// ---------------------------------------------------------------------------

ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel) {

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_model = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new QListView(this);
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new ColumnItemDelegate(this));

    layout->addWidget(m_listView);

    // 选中变化直接连 ContentPanel
    if (m_contentPanel) {
        connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged,
                m_contentPanel, &ContentPanel::onSelectionChanged);

        m_listView->installEventFilter(m_contentPanel);
        m_listView->viewport()->installEventFilter(m_contentPanel);
        connect(m_listView, &QListView::customContextMenuRequested,
                m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
    }

    connect(m_listView, &QListView::clicked, this, &ColumnViewPane::onClicked);
    connect(m_listView, &QListView::doubleClicked, this, &ColumnViewPane::onDoubleClicked);

    setFixedWidth(240);
    loadDirectory();
}

void ColumnViewPane::loadDirectory() {
    if (m_path.isEmpty()) return;
    QString scanPath = m_path;

    QtConcurrent::run([this, scanPath]() {
        bool showHidden = m_contentPanel ? m_contentPanel->currentFilter().showHidden : false;
        std::vector<ItemRecord> rawItems = DiskScanService::scanDirectory(scanPath, false, []() { return true; });

        std::vector<ItemRecord> items;
        if (!showHidden) {
            items.reserve(rawItems.size());
            for (const auto& item : rawItems) {
                if (!item.isHidden) {
                    items.push_back(item);
                }
            }
        } else {
            items = std::move(rawItems);
        }

        QMetaObject::invokeMethod(this, [this, items]() {
            if (m_model) {
                m_model->setRecords(items);
            }
        }, Qt::QueuedConnection);
    });
}

void ColumnViewPane::selectItemByPath(const QString& itemPath) {
    if (!m_proxyModel || itemPath.isEmpty()) return;
    for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
        QModelIndex proxyIdx = m_proxyModel->index(i, 0);
        if (proxyIdx.data(PathRole).toString() == itemPath) {
            if (m_listView && m_listView->selectionModel()) {
                m_listView->scrollTo(proxyIdx);
                m_listView->setCurrentIndex(proxyIdx);
                m_listView->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            break;
        }
    }
}

void ColumnViewPane::onClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    QString targetPath = index.data(PathRole).toString();
    bool isFolder = (index.data(TypeRole).toString() == "folder");

    if (isFolder) {
        emit folderSelected(targetPath, this);
    } else {
        emit fileSelected(targetPath, this);
    }
}

void ColumnViewPane::onDoubleClicked(const QModelIndex& index) {
    if (!index.isValid() || !m_contentPanel) return;
    QString targetPath = index.data(PathRole).toString();
    bool isFolder = (index.data(TypeRole).toString() == "folder");

    if (isFolder) {
        emit m_contentPanel->directorySelected(targetPath);
    } else {
        emit m_contentPanel->fileActivated(targetPath);
    }
}

// ---------------------------------------------------------------------------
// ColumnViewWidget Implementation
// ---------------------------------------------------------------------------

ColumnViewWidget::ColumnViewWidget(ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_contentPanel(contentPanel) {

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_container = new QWidget(m_scrollArea);
    m_containerLayout = new QHBoxLayout(m_container);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_containerLayout->setSpacing(1);
    m_containerLayout->addStretch(1);

    m_container->setLayout(m_containerLayout);
    m_scrollArea->setWidget(m_container);

    mainLayout->addWidget(m_scrollArea);
}

void ColumnViewWidget::setRootPath(const QString& path) {
    m_rootPath = path;
    qDeleteAll(m_panes);
    m_panes.clear();

    if (!path.isEmpty() && path != "computer://") {
        appendColumn(path);
    }
}

void ColumnViewWidget::appendColumn(const QString& path) {
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);

    connect(pane, &ColumnViewPane::folderSelected, this, &ColumnViewWidget::onFolderSelected);
    connect(pane, &ColumnViewPane::fileSelected, this, &ColumnViewWidget::onFileSelected);

    // 插入到 layout Stretch 之前
    m_containerLayout->insertWidget(m_containerLayout->count() - 1, pane);
    m_panes.append(pane);

    // 自动向右滚动到底
    QMetaObject::invokeMethod(this, [this]() {
        if (m_scrollArea && m_scrollArea->horizontalScrollBar()) {
            m_scrollArea->horizontalScrollBar()->setValue(m_scrollArea->horizontalScrollBar()->maximum());
        }
    }, Qt::QueuedConnection);
}

void ColumnViewWidget::dismissSubColumns(ColumnViewPane* targetPane) {
    int idx = m_panes.indexOf(targetPane);
    if (idx < 0) return;

    while (m_panes.size() > idx + 1) {
        ColumnViewPane* lastPane = m_panes.takeLast();
        m_containerLayout->removeWidget(lastPane);
        delete lastPane;
    }
}

ColumnViewPane* ColumnViewWidget::activePane() const {
    QWidget* focusWidget = QApplication::focusWidget();
    for (auto* pane : m_panes) {
        if (pane->listView() && focusWidget &&
            (focusWidget == pane->listView() || pane->listView()->isAncestorOf(focusWidget))) {
            return pane;
        }
    }
    return m_panes.isEmpty() ? nullptr : m_panes.last();
}

void ColumnViewWidget::refreshActiveColumn() {
    ColumnViewPane* active = activePane();
    if (active) {
        active->loadDirectory();
    }
}

void ColumnViewWidget::updateMetadataForPath(const QString& path) {
    for (auto* pane : m_panes) {
        if (pane && pane->model()) {
            pane->model()->updateRecordMetadata(path);
            if (pane->listView() && pane->listView()->viewport()) {
                pane->listView()->viewport()->update();
            }
        }
    }
}

void ColumnViewWidget::onFolderSelected(const QString& folderPath, ColumnViewPane* pane) {
    dismissSubColumns(pane);
    appendColumn(folderPath);
}

void ColumnViewWidget::onFileSelected(const QString& /*filePath*/, ColumnViewPane* pane) {
    dismissSubColumns(pane);
}

} // namespace QuarkMeta
