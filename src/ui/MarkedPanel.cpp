#include "MarkedPanel.h"
#include "UiHelper.h"
#include "../meta/MarkedFolderService.h"
#include "ToolTipOverlay.h"
#include <QFileInfo>
#include <QDir>
#include <QMenu>
#include <QMouseEvent>

namespace QuarkMeta {

MarkedPanel::MarkedPanel(QWidget* parent) : QWidget(parent) {
    initUi();
    connect(&MarkedFolderService::instance(), &MarkedFolderService::markedFoldersChanged, this, &MarkedPanel::onMarkedFoldersChanged);
    loadMarkedFolders();
}

void MarkedPanel::initUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setObjectName("MarkedTreeWidget");
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(0);
    m_treeWidget->setIconSize(QSize(16, 16));
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    m_mainLayout->addWidget(m_treeWidget);

    connect(m_treeWidget, &QTreeWidget::itemClicked, this, &MarkedPanel::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &MarkedPanel::onContextMenuRequested);
}

void MarkedPanel::loadMarkedFolders() {
    m_treeWidget->clear();
    QStringList allList = MarkedFolderService::instance().getAllMarked();
    for (const QString& path : allList) {
        if (QFile::exists(path)) {
            bool active = MarkedFolderService::instance().isMarked(path);
            QTreeWidgetItem* item = createMarkedItem(path, active);
            m_treeWidget->addTopLevelItem(item);
        }
    }
}

QTreeWidgetItem* MarkedPanel::createMarkedItem(const QString& path, bool active) {
    QFileInfo fi(path);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, fi.fileName().isEmpty() ? path : fi.fileName());
    item->setData(0, Qt::UserRole, path);
    item->setData(0, Qt::UserRole + 1, active);
    item->setIcon(0, UiHelper::getIcon("Destination.svg", active ? QColor("#ff551c") : QColor("#555555"), 16));
    return item;
}

void MarkedPanel::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty()) {
        emit requestLocateFolder(path);
    }
}

void MarkedPanel::onContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    if (item) {
        QString path = item->data(0, Qt::UserRole).toString();
        bool active = item->data(0, Qt::UserRole + 1).toBool();

        if (active) {
            QAction* unmarkAct = menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#888888"), 16), "取消标记");
            connect(unmarkAct, &QAction::triggered, this, [path]() {
                MarkedFolderService::instance().removeMarked(path);
            });
        } else {
            QAction* markAct = menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 16), "标记为目的地");
            connect(markAct, &QAction::triggered, this, [path]() {
                MarkedFolderService::instance().addMarked(path);
            });
        }

        QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 16), "从标记历史中移除");
        connect(removeAct, &QAction::triggered, this, [path]() {
            MarkedFolderService::instance().removeFolderRecord(path);
        });
        menu.addSeparator();
    }

    QAction* unmarkAllAct = menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#888888"), 16), "取消所有标记");
    connect(unmarkAllAct, &QAction::triggered, this, []() {
        MarkedFolderService::instance().clearAll();
    });

    QAction* clearHistoryAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 16), "清空标记历史");
    connect(clearHistoryAct, &QAction::triggered, this, []() {
        MarkedFolderService::instance().clearAllFolderRecords();
    });

    menu.exec(m_treeWidget->mapToGlobal(pos));
}

void MarkedPanel::onMarkedFoldersChanged() {
    loadMarkedFolders();
}

void MarkedPanel::filterTreeNodes(const QString& keyword) {
    QString kw = keyword.trimmed().toLower();
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
        if (kw.isEmpty()) {
            item->setHidden(false);
        } else {
            bool match = item->text(0).toLower().contains(kw) || item->data(0, Qt::UserRole).toString().toLower().contains(kw);
            item->setHidden(!match);
        }
    }
}

} // namespace QuarkMeta
