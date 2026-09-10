#include "ColumnViewWidget.h"
#include "../core/DiskScanService.h"
#include "ColumnItemDelegate.h"
#include "UiHelper.h"
#include <QFileInfo>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <QDir>

namespace QuarkMeta {

ColumnViewPane::ColumnViewPane(const QString& path, QWidget* parent)
    : QWidget(parent), m_path(path) 
{
    setMinimumWidth(220);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_model = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new QListView(this);
    m_listView->setFocusPolicy(Qt::NoFocus);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new ColumnItemDelegate(this));
    m_listView->setStyleSheet("QListView { background: #1E1E1E; border: none; border-right: 1px solid #2D2D2D; color: #CCCCCC; outline: none; }"
                              "QListView::item:selected { background: #3E3E42; color: #FFFFFF; outline: none; }"
                              "QListView::item:focus { outline: none; border: none; }");
    layout->addWidget(m_listView);

    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(Qt::UserRole + 1).toString();
        bool isDir = index.data(Qt::UserRole + 2).toBool();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderSelected(itemPath, paneIdx);
        } else {
            emit fileSelected(itemPath, paneIdx);
        }
    });

    loadDirectory();
}

void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    if (!m_proxyModel) return;
    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        QModelIndex idx = m_proxyModel->index(r, 0);
        if (idx.data(Qt::UserRole + 1).toString() == targetPath) {
            m_listView->setCurrentIndex(idx);
            m_listView->scrollTo(idx);
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
                // 异步扫描完成后加载前 50 行项目的缩略图并更新视图
                int count = weakSelf->m_model->rowCount();
                QList<int> rowsToLoad;
                for (int i = 0; i < qMin(count, 50); ++i) {
                    rowsToLoad.append(i);
                }
                weakSelf->m_model->loadThumbnailsForRows(rowsToLoad);
            }
        });
    });

    connect(m_model, &DiskItemModel::dataChanged, this, [this](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
        Q_UNUSED(topLeft); Q_UNUSED(bottomRight);
        if (m_listView && m_listView->viewport()) {
            m_listView->viewport()->update();
        }
    });
}

ColumnViewWidget::ColumnViewWidget(QWidget* parent)
    : QScrollArea(parent) 
{
    setWidgetResizable(true);
    setStyleSheet("QScrollArea { background: #181818; border: none; }");

    m_container = new QWidget(this);
    m_layout = new QHBoxLayout(m_container);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    setWidget(m_container);
}

void ColumnViewWidget::setRootPath(const QString& path) {
    clearAllColumns();
    if (path.isEmpty()) return;

    appendColumn(path);
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
    ColumnViewPane* pane = new ColumnViewPane(path, m_container);
    pane->setProperty("paneIndex", newIdx);

    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        dismissSubColumns(paneIdx);
        clearOtherSelections(paneIdx);
        appendColumn(folderPath);
        emit pathNavigated(folderPath);
    });

    connect(pane, &ColumnViewPane::fileSelected, this, [this](const QString& filePath, int paneIdx) {
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
    int availableWidth = width();
    if (availableWidth <= 0) availableWidth = 800;

    int colCount = m_panes.size();
    int defaultWidth = 230;

    if (colCount * defaultWidth < availableWidth) {
        for (int i = 0; i < colCount - 1; ++i) {
            m_panes[i]->setFixedWidth(defaultWidth);
        }
        m_panes.last()->setMinimumWidth(availableWidth - (colCount - 1) * defaultWidth);
        m_panes.last()->setMaximumWidth(QWIDGETSIZE_MAX);
    } else {
        for (auto* pane : m_panes) {
            pane->setFixedWidth(defaultWidth);
        }
    }
}

} // namespace QuarkMeta
