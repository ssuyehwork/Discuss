#include "ColumnViewWidget.h"
#include "../core/DiskScanService.h"
#include "TreeItemDelegate.h"
#include "UiHelper.h"
#include <QFileInfo>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <QDir>
#include <QResizeEvent>

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
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new TreeItemDelegate(this, false, false));
    m_listView->setStyleSheet("QListView { background: #1E1E1E; border: none; border-right: 1px solid #2D2D2D; color: #CCCCCC; }"
                              "QListView::item:selected { background: #3E3E42; color: #FFFFFF; }");
    layout->addWidget(m_listView);

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
            }
        });
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
    m_layout->setAlignment(Qt::AlignLeft);

    setWidget(m_container);
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

} // namespace QuarkMeta
