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
    m_listView->setStyleSheet("QListView { background: #1E1E1E; border: 1px solid #333333; color: #CCCCCC; }"
                              "QListView::item:selected { background: #3E3E42; color: #FFFFFF; }");
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
}

void ColumnViewWidget::appendColumn(const QString& path) {
    int newIdx = m_panes.size();
    ColumnViewPane* pane = new ColumnViewPane(path, m_container);
    pane->setProperty("paneIndex", newIdx);

    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        dismissSubColumns(paneIdx);
        appendColumn(folderPath);
        emit pathNavigated(folderPath);
    });

    m_panes.append(pane);
    m_layout->insertWidget(m_panes.size() - 1, pane);
    ensureWidgetVisible(pane);
}

} // namespace QuarkMeta
