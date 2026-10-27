#include "HistoryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "../meta/HistoryService.h"
#include "../util/ThumbnailPipelineService.h"
#include "../util/ColorPaletteEngine.h"
#include <QFileInfo>
#include <QDir>
#include <QFileIconProvider>
#include <QDrag>
#include <QMimeData>
#include <QUrl>
#include <QMouseEvent>
#include <QHeaderView>

namespace QuarkMeta {

HistoryPanel::HistoryPanel(QWidget* parent) : QWidget(parent) {
    initUi();

    connect(&HistoryService::instance(), &HistoryService::historyItemAdded, this, &HistoryPanel::onHistoryItemAdded);
    connect(&HistoryService::instance(), &HistoryService::historyItemRemoved, this, &HistoryPanel::onHistoryItemRemoved);
    connect(&HistoryService::instance(), &HistoryService::historyCleared, this, &HistoryPanel::onHistoryCleared);
}

void HistoryPanel::initUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setObjectName("HistoryTreeWidget");
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(0);
    m_treeWidget->setIconSize(QSize(16, 16));
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeWidget->setDragEnabled(true);
    m_treeWidget->setMouseTracking(true);

    m_mainLayout->addWidget(m_treeWidget);

    connect(m_treeWidget, &QTreeWidget::itemClicked, this, &HistoryPanel::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &HistoryPanel::onContextMenuRequested);

    if (m_treeWidget->viewport()) {
        m_treeWidget->viewport()->installEventFilter(this);
    }
}

void HistoryPanel::loadHistory() {
    if (m_isLoaded) return;
    m_isLoaded = true;

    m_treeWidget->clear();
    auto list = HistoryService::instance().getAllHistory();
    for (const auto& rec : list) {
        if (QFile::exists(rec.filePath)) {
            QTreeWidgetItem* item = createHistoryItem(rec.filePath);
            m_treeWidget->addTopLevelItem(item);
        }
    }
}

QTreeWidgetItem* HistoryPanel::createHistoryItem(const QString& path) {
    QFileInfo fi(path);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, fi.fileName());
    item->setData(0, Qt::UserRole, path);

    // 普通文件优先展示系统/通用图标
    QFileIconProvider provider;
    QIcon sysIcon = provider.icon(fi);
    if (!sysIcon.isNull()) {
        item->setIcon(0, sysIcon);
    } else {
        item->setIcon(0, UiHelper::getIcon("file", "#888888", 16));
    }

    // 图形图片类文件异步提取缩略图回填
    if (ColorPaletteEngine::isGraphicsFile(path)) {
        updateItemThumbnail(item, path);
    }

    return item;
}

void HistoryPanel::updateItemThumbnail(QTreeWidgetItem* item, const QString& path) {
    ThumbnailPipelineService::instance().loadBatchAsync({path}, 32, [this, item](const QString&, const QPixmap& pix) {
        if (!pix.isNull() && item && item->treeWidget()) {
            item->setIcon(0, QIcon(pix));
        }
    });
}

void HistoryPanel::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty()) {
        emit requestLocateFile(path);
    }
}

void HistoryPanel::onContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    QMenu menu(this);

    if (item) {
        QString path = item->data(0, Qt::UserRole).toString();
        QAction* removeAct = menu.addAction("从历史中移除");
        connect(removeAct, &QAction::triggered, this, [path]() {
            HistoryService::instance().removeHistory(path);
        });
        menu.addSeparator();
    }

    QAction* clearAct = menu.addAction("清空历史");
    connect(clearAct, &QAction::triggered, this, []() {
        HistoryService::instance().clearAllHistory();
    });

    menu.exec(m_treeWidget->mapToGlobal(pos));
}

void HistoryPanel::onHistoryItemAdded(const QString& path) {
    if (!m_isLoaded) return;

    // 检查是否存在现有 Item
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == path) {
            // 顶置现有 Item
            m_treeWidget->takeTopLevelItem(i);
            m_treeWidget->insertTopLevelItem(0, item);
            return;
        }
    }

    // 新建 Item 插入第 0 行
    QTreeWidgetItem* newItem = createHistoryItem(path);
    m_treeWidget->insertTopLevelItem(0, newItem);

    // 保持最多 100 项
    if (m_treeWidget->topLevelItemCount() > 100) {
        delete m_treeWidget->takeTopLevelItem(m_treeWidget->topLevelItemCount() - 1);
    }
}

void HistoryPanel::onHistoryItemRemoved(const QString& path) {
    if (!m_isLoaded) return;
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == path) {
            delete m_treeWidget->takeTopLevelItem(i);
            break;
        }
    }
}

void HistoryPanel::onHistoryCleared() {
    m_treeWidget->clear();
    m_isLoaded = false;
    loadHistory();
}

void HistoryPanel::filterTreeNodes(const QString& keyword) {
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

bool HistoryPanel::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_treeWidget->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->buttons() & Qt::LeftButton) {
                QTreeWidgetItem* item = m_treeWidget->itemAt(mouseEvent->pos());
                if (item) {
                    QString path = item->data(0, Qt::UserRole).toString();
                    if (!path.isEmpty()) {
                        ToolTipOverlay::hideTip();
                        QDrag* drag = new QDrag(m_treeWidget);
                        QMimeData* mimeData = new QMimeData();
                        mimeData->setUrls({QUrl::fromLocalFile(path)});
                        drag->setMimeData(mimeData);
                        drag->exec(Qt::CopyAction);
                        return true;
                    }
                }
            } else {
                QTreeWidgetItem* item = m_treeWidget->itemAt(mouseEvent->pos());
                if (item) {
                    QString path = item->data(0, Qt::UserRole).toString();
                    if (!path.isEmpty()) {
                        QString nativePath = QDir::toNativeSeparators(path);
                        if (m_lastHoveredPath != nativePath) {
                            m_lastHoveredPath = nativePath;
                            ToolTipOverlay::instance()->showText(QCursor::pos(), nativePath, 0);
                        }
                    } else {
                        m_lastHoveredPath.clear();
                        ToolTipOverlay::hideTip();
                    }
                } else {
                    m_lastHoveredPath.clear();
                    ToolTipOverlay::hideTip();
                }
            }
        } else if (event->type() == QEvent::Leave) {
            m_lastHoveredPath.clear();
            ToolTipOverlay::hideTip();
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace QuarkMeta
