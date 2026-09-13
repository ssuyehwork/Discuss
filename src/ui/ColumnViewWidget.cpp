#include "ColumnViewWidget.h"
#include "ColumnItemDelegate.h"
#include "DropListView.h"
#include "ContentPanel.h"
#include "../core/DiskScanService.h"
#include "../core/ModelContract.h"

#include <QVBoxLayout>
#include <QApplication>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>

namespace QuarkMeta {

// ---------------------------------------------------------------------------
// ColumnViewPane Implementation
// ---------------------------------------------------------------------------

ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel) {
    setObjectName("ColumnViewPane");
    setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_model = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new ColumnItemDelegate(this));

    layout->addWidget(m_listView);

    // 选中变化驱动焦点设置，而后触发 ContentPanel::onSelectionChanged 与 selectionChanged 广播
    if (m_listView->selectionModel()) {
        connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            if (m_listView && !m_listView->selectionModel()->selectedIndexes().isEmpty()) {
                m_listView->setFocus();
            }
            emit selectionChanged();
            if (m_contentPanel) {
                m_contentPanel->onSelectionChanged();
            }
        });
    }

    if (m_contentPanel) {
        m_listView->installEventFilter(m_contentPanel);
        m_listView->viewport()->installEventFilter(m_contentPanel);
        connect(m_listView, &QListView::customContextMenuRequested,
                m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
        connect(m_listView, &DropListView::pathsDropped,
                m_contentPanel, &ContentPanel::onPathsDropped);
    }

    connect(m_listView, &QListView::clicked, this, &ColumnViewPane::onClicked);
    connect(m_listView, &QListView::doubleClicked, this, &ColumnViewPane::onDoubleClicked);

    setFixedWidth(240);
    loadDirectory();
}

void ColumnViewPane::loadDirectory() {
    if (m_path.isEmpty()) return;
    QString scanPath = m_path;

    QThreadPool::globalInstance()->start([this, scanPath]() {
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
                if (!m_pendingSelectPath.isEmpty()) {
                    tryPendingSelection();
                }
                // 触发图标与缩略图提取管线
                int count = m_model->rowCount();
                if (count > 0) {
                    QList<int> visibleRows;
                    visibleRows.reserve(count);
                    for (int r = 0; r < count; ++r) visibleRows.append(r);
                    m_model->loadThumbnailsForRows(visibleRows);
                }
                emit recordsLoaded(items);
            }
        }, Qt::QueuedConnection);
    });
}

void ColumnViewPane::selectItemByPath(const QString& itemPath) {
    m_pendingSelectPath = itemPath;
    tryPendingSelection();
}

void ColumnViewPane::tryPendingSelection() {
    if (m_pendingSelectPath.isEmpty() || !m_proxyModel || !m_listView) return;

    QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(m_pendingSelectPath));
    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        QModelIndex idx = m_proxyModel->index(r, 0);
        QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));

        if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0) {
            m_listView->setCurrentIndex(idx);
            if (m_listView->selectionModel()) {
                m_listView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            m_listView->scrollTo(idx, QAbstractItemView::EnsureVisible);
            m_pendingSelectPath.clear();
            break;
        }
    }
}

void ColumnViewPane::clearSelection() {
    if (m_listView && m_listView->selectionModel()) {
        m_listView->selectionModel()->clearSelection();
    }
}

void ColumnViewWidget::clearOtherSelections(ColumnViewPane* currentPane) {
    for (auto* pane : m_panes) {
        if (pane != currentPane && pane && pane->listView() && pane->listView()->selectionModel()) {
            QSignalBlocker blocker(pane->listView()->selectionModel());
            pane->listView()->clearSelection();
        }
    }
}

void ColumnViewPane::setFilterState(const FilterState& state) {
    if (m_proxyModel) {
        m_proxyModel->currentFilter = state;
        m_proxyModel->updateFilter();
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
    m_scrollArea->setObjectName("ColumnViewScrollArea");
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_container = new QWidget(m_scrollArea);
    m_containerLayout = new QHBoxLayout(m_container);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_containerLayout->setSpacing(0);
    m_containerLayout->addStretch(1);

    m_container->setLayout(m_containerLayout);
    m_scrollArea->setWidget(m_container);

    mainLayout->addWidget(m_scrollArea);
}

void ColumnViewWidget::scrollToRightmostPane() {
    QMetaObject::invokeMethod(this, [this]() {
        if (m_scrollArea && m_scrollArea->horizontalScrollBar()) {
            m_scrollArea->horizontalScrollBar()->setValue(m_scrollArea->horizontalScrollBar()->maximum());
        }
        if (!m_panes.isEmpty() && m_panes.last()) {
            m_scrollArea->ensureWidgetVisible(m_panes.last(), 0, 0);
        }
    }, Qt::QueuedConnection);
}

bool ColumnViewWidget::containsPath(const QString& path) const {
    if (path.isEmpty()) return false;
    QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(path));
    for (auto* pane : m_panes) {
        if (pane) {
            QString panePath = QDir::toNativeSeparators(QDir::cleanPath(pane->path()));
            if (QString::compare(panePath, cleanTarget, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
    }
    return false;
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
    m_rootPath = path;
    clearAllColumns();
    if (path.isEmpty() || path == "computer://") return;

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
}

void ColumnViewWidget::clearAllColumns() {
    dismissSubColumns(-1);
}

void ColumnViewWidget::appendColumn(const QString& path) {
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    pane->setFilterState(m_currentFilter);

    connect(pane, &ColumnViewPane::selectionChanged, this, [this]() {
        emit selectionChanged();
    });
    
    connect(pane, &ColumnViewPane::folderSelected, this, &ColumnViewWidget::onFolderSelected);
    connect(pane, &ColumnViewPane::fileSelected, this, &ColumnViewWidget::onFileSelected);

    if (pane->model()) {
        emit activeColumnRecordsChanged(pane->model()->allRecords());
    }

    // 插入到 layout Stretch 之前
    m_containerLayout->insertWidget(m_containerLayout->count() - 1, pane);
    m_panes.append(pane);

    scrollToRightmostPane();
}

void ColumnViewWidget::dismissSubColumns(ColumnViewPane* targetPane) {
    int idx = m_panes.indexOf(targetPane);
    if (idx < 0) return;
    dismissSubColumns(idx);
}

void ColumnViewWidget::dismissSubColumns(int fromIndex) {
    while (m_panes.size() > fromIndex + 1) {
        ColumnViewPane* pane = m_panes.takeLast();
        m_containerLayout->removeWidget(pane);
        delete pane;
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
    int idx = m_panes.indexOf(pane);
    dismissSubColumns(pane);
    // 保持父列高亮：仅清空当前列右侧深层列的选择，绝对保留当前列及其左侧父列的高亮
    if (idx >= 0) {
        for (int i = idx + 1; i < m_panes.size(); ++i) {
            m_panes[i]->clearSelection();
        }
    }
    appendColumn(folderPath);
    if (pane && pane->model()) {
        emit activeColumnRecordsChanged(pane->model()->allRecords());
    }
}

void ColumnViewWidget::onFileSelected(const QString& /*filePath*/, ColumnViewPane* pane) {
    dismissSubColumns(pane);
    if (pane && pane->model()) {
        emit activeColumnRecordsChanged(pane->model()->allRecords());
    }
}

} // namespace QuarkMeta
