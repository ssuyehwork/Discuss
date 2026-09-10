#include "ColumnViewWidget.h"
#include "../core/DiskScanService.h"
#include "UiHelper.h"
#include <QFileInfo>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>

namespace QuarkMeta {

ColumnViewPane::ColumnViewPane(const QString& path, QWidget* parent)
    : QWidget(parent), m_path(path) 
{
    setFixedWidth(230);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_titleLabel = new QLabel(QFileInfo(path).fileName().isEmpty() ? path : QFileInfo(path).fileName(), this);
    m_titleLabel->setStyleSheet("font-weight: bold; padding: 4px; color: #EEEEEE; background: #252526;");
    layout->addWidget(m_titleLabel);

    m_model = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new QListView(this);
    m_listView->setModel(m_proxyModel);
    m_listView->setStyleSheet("QListView { background: #1E1E1E; border: 1px solid #333333; color: #CCCCCC; outline: none; }"
                              "QListView::item:selected { background: #3E3E42; color: #FFFFFF; outline: none; }");
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
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);
    m_layout->addStretch();

    setWidget(m_container);
}

void ColumnViewWidget::setRootPath(const QString& path) {
    clearAllColumns();
    if (path.isEmpty()) return;

    // Build path stack from root to target path
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

    // Append columns recursively for each ancestor
    for (int i = 0; i < pathStack.size(); ++i) {
        const QString& p = pathStack[i];
        ColumnViewPane* pane = appendColumn(p);
        if (i > 0 && i - 1 < m_panes.size() - 1) {
            // Highlight the selected child folder in the parent pane
            m_panes[i - 1]->selectItemByPath(p);
        }
    }
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
    m_layout->insertWidget(m_panes.size() - 1, pane);
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

} // namespace QuarkMeta
