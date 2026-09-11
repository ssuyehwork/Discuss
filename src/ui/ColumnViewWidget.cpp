#include "ColumnViewWidget.h"
#include "ContentPanel.h"
#include "../core/DiskScanService.h"
#include "DropListView.h"
#include "ColumnItemDelegate.h"
#include "UiHelper.h"
#include <QFileInfo>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <QDir>
#include <QResizeEvent>

namespace QuarkMeta {

ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel) 
{
    setMinimumWidth(220);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_model = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setDropIndicatorShown(true);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setModel(m_proxyModel);

    auto* delegate = new ColumnItemDelegate(this);
    m_listView->setItemDelegate(delegate);
    layout->addWidget(m_listView);

    if (m_contentPanel) {
        m_listView->installEventFilter(m_contentPanel);
        m_listView->viewport()->installEventFilter(m_contentPanel);
        connect(m_listView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
    }

    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ColumnViewPane::selectionChanged);

    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder");
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderSelected(itemPath, paneIdx);
        }
    });

    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder");
        int paneIdx = property("paneIndex").toInt();
        if (!isDir) {
            emit fileSelected(itemPath, paneIdx);
        }
    });

    loadDirectory();
}

void ColumnViewPane::setFilterState(const FilterState& state) {
    if (m_proxyModel) {
        m_proxyModel->currentFilter = state;
        m_proxyModel->updateFilter();
    }
}

void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    if (!m_proxyModel) return;
    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        QModelIndex idx = m_proxyModel->index(r, 0);
        if (idx.data(PathRole).toString() == targetPath) {
            m_listView->setCurrentIndex(idx);
            m_listView->scrollTo(idx);
            m_pendingSelectPath.clear();
            break;
        }
    }
}

void ColumnViewPane::clearSelection() {
    if (m_listView) {
        m_listView->clearSelection();
    }
}

void ColumnViewPane::loadDirectory() {
    QString path = m_path;
    QPointer<ColumnViewPane> weakSelf(this);
    (void)QtConcurrent::run([weakSelf, path]() {
        if (!weakSelf) return;
        std::vector<ItemRecord> items = DiskScanService::scanDirectory(path, false, std::function<bool()>());
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakSelf, items]() {
            if (weakSelf && weakSelf->m_model) {
                weakSelf->m_model->setRecords(items);
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
                // 触发图标与缩略图提取管线
                int count = weakSelf->m_model->rowCount();
                if (count > 0) {
                    QList<int> visibleRows;
                    visibleRows.reserve(count);
                    for (int r = 0; r < count; ++r) visibleRows.append(r);
                    weakSelf->m_model->loadThumbnailsForRows(visibleRows);
                }
                emit weakSelf->recordsLoaded(items);
            }
        });
    });
}

ColumnViewWidget::ColumnViewWidget(ContentPanel* contentPanel, QWidget* parent)
    : QScrollArea(parent), m_contentPanel(contentPanel) 
{
    setObjectName("ColumnViewScrollArea");
    setWidgetResizable(true);

    m_container = new QWidget(this);
    m_layout = new QHBoxLayout(m_container);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->setAlignment(Qt::AlignLeft);

    setWidget(m_container);
}

ColumnViewPane* ColumnViewWidget::activePane() const {
    if (m_activePaneIndex >= 0 && m_activePaneIndex < m_panes.size()) {
        return m_panes[m_activePaneIndex];
    }
    return m_panes.isEmpty() ? nullptr : m_panes.last();
}

QStringList ColumnViewWidget::getSelectedPaths() const {
    ColumnViewPane* pane = activePane();
    if (!pane || !pane->listView() || !pane->listView()->selectionModel()) return {};
    QStringList paths;
    for (const auto& idx : pane->listView()->selectionModel()->selectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) paths << p;
        }
    }
    return paths;
}

QModelIndexList ColumnViewWidget::getSelectedIndexes() const {
    ColumnViewPane* pane = activePane();
    if (!pane || !pane->listView() || !pane->listView()->selectionModel()) return {};
    return pane->listView()->selectionModel()->selectedIndexes();
}

void ColumnViewWidget::applyFilterState(const FilterState& state) {
    m_currentFilter = state;
    for (auto* pane : m_panes) {
        pane->setFilterState(state);
    }
}

void ColumnViewWidget::setRootPath(const QString& path) {
    clearAllColumns();
    if (path.isEmpty()) return;

    // 1. 拆分完整的祖先路径栈
    QList<QString> pathStack;
    QDir dir(path);
    QString curr = dir.absolutePath();

    while (!curr.isEmpty()) {
        pathStack.prepend(curr);
        QDir parentDir(curr);
        if (!parentDir.cdUp() || parentDir.absolutePath() == curr) {
            break;
        }
        curr = parentDir.absolutePath();
    }

    // 2. 逐层展开列，并在父列中高亮选中对应的子项
    for (int i = 0; i < pathStack.size(); ++i) {
        const QString& p = pathStack[i];
        appendColumn(p);
        if (i > 0 && i - 1 < m_panes.size() - 1) {
            m_panes[i - 1]->selectItemByPath(p);
        }
    }
    updatePaneWidths();
}

void ColumnViewWidget::clearAllColumns() {
    dismissSubColumns(-1);
}

void ColumnViewWidget::dismissSubColumns(int fromIndex) {
    while (m_panes.size() > fromIndex + 1) {
        ColumnViewPane* pane = m_panes.takeLast();
        m_layout->removeWidget(pane);
        pane->deleteLater();
    }
    updatePaneWidths();
}

ColumnViewPane* ColumnViewWidget::appendColumn(const QString& path) {
    int newIdx = m_panes.size();
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    pane->setProperty("paneIndex", newIdx);
    pane->setFilterState(m_currentFilter);

    connect(pane, &ColumnViewPane::recordsLoaded, this, [this, pane](const std::vector<ItemRecord>& records) {
        if (pane == activePane()) {
            emit activeColumnRecordsChanged(records);
        }
    });

    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        m_activePaneIndex = pane->property("paneIndex").toInt();
        emit selectionChanged();
        if (pane->model()) {
            emit activeColumnRecordsChanged(pane->model()->allRecords());
        }
    });

    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        m_activePaneIndex = paneIdx;
        dismissSubColumns(paneIdx);
        // 保持父列高亮：仅清空 paneIdx 右侧深层列的选区，保留 paneIdx 及其左侧父列的高亮
        for (int i = paneIdx + 1; i < m_panes.size(); ++i) {
            m_panes[i]->clearSelection();
        }
        appendColumn(folderPath);
    });

    connect(pane, &ColumnViewPane::fileSelected, this, [this](const QString& filePath, int paneIdx) {
        m_activePaneIndex = paneIdx;
        dismissSubColumns(paneIdx);
        clearOtherSelections(paneIdx);
        emit pathNavigated(filePath);
    });

    m_panes.append(pane);
    m_layout->addWidget(pane);
    updatePaneWidths();
    ensureWidgetVisible(pane);
    return pane;
}

void ColumnViewWidget::clearOtherSelections(int activePaneIdx) {
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i != activePaneIdx) {
            m_panes[i]->clearSelection();
        }
    }
}

void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = 240;
    for (auto* pane : m_panes) {
        pane->setFixedWidth(defaultWidth);
        pane->setMinimumWidth(defaultWidth);
        pane->setMaximumWidth(defaultWidth);
    }
}

void ColumnViewWidget::resizeEvent(QResizeEvent* event) {
    QScrollArea::resizeEvent(event);
    updatePaneWidths();
}

void ColumnViewWidget::refreshActiveColumn() {
    ColumnViewPane* pane = activePane();
    if (pane) {
        pane->loadDirectory();
    }
}

} // namespace QuarkMeta
