#include "ColumnViewWidget.h"
#include "ContentPanel.h"
#include "DualSectionPanel.h"
#include "../core/DiskScanService.h"
#include "../core/NavigationService.h"
#include "../meta/MetaCacheDecorator.h"
#include "DropListView.h"
#include "ColumnItemDelegate.h"
#include "UiHelper.h"
#include <QFileInfo>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <QDir>
#include <QResizeEvent>
#include <QScrollBar>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>

namespace QuarkMeta {

class ColumnBlankCanvasWidget : public QWidget {
public:
    explicit ColumnBlankCanvasWidget(ColumnViewWidget* columnView, ContentPanel* contentPanel, QWidget* parent = nullptr)
        : QWidget(parent), m_columnView(columnView), m_contentPanel(contentPanel) {
        setObjectName("ColumnBlankCanvasWidget");
        setAcceptDrops(true);
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, this, &ColumnBlankCanvasWidget::onContextMenuRequested);
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && m_columnView) {
            m_columnView->goUpColumn();
        }
        QWidget::mouseDoubleClickEvent(event);
    }

    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData() && event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
            m_isDragHover = true;
            update();
        }
    }

    void dragLeaveEvent(QDragLeaveEvent* event) override {
        m_isDragHover = false;
        update();
        QWidget::dragLeaveEvent(event);
    }

    void dropEvent(QDropEvent* event) override {
        m_isDragHover = false;
        update();
        if (m_contentPanel && m_columnView && m_columnView->rightmostPane()) {
            QString targetDir = m_columnView->rightmostPane()->currentPath();
            QStringList paths;
            for (const QUrl& url : event->mimeData()->urls()) {
                paths << url.toLocalFile();
            }
            if (!paths.isEmpty()) {
                m_contentPanel->onPathsDropped(paths, QModelIndex(), targetDir);
                event->acceptProposedAction();
            }
        }
    }

    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        if (m_isDragHover) {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            QColor highlightColor("#3498db");
            highlightColor.setAlphaF(0.35f);
            painter.fillRect(rect(), highlightColor);
            painter.setPen(QPen(QColor("#3498db"), 2, Qt::DashLine));
            painter.drawRect(rect().adjusted(1, 1, -1, -1));
        }
    }

private:
    void onContextMenuRequested(const QPoint& pos) {
        if (m_contentPanel) {
            QPoint globalPos = mapToGlobal(pos);
            QAbstractItemView* view = m_contentPanel->activeItemView();
            if (view && view->viewport()) {
                QPoint viewPos = view->viewport()->mapFromGlobal(globalPos);
                m_contentPanel->onCustomContextMenuRequested(view, viewPos);
            } else {
                m_contentPanel->onCustomContextMenuRequested(nullptr, globalPos);
            }
        }
    }

    ColumnViewWidget* m_columnView = nullptr;
    ContentPanel* m_contentPanel = nullptr;
    bool m_isDragHover = false;
};

ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel) 
{
    setObjectName("ColumnViewPane");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(220);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 1, 1, 0);
    layout->setSpacing(0);

    m_paneScrollArea = new QScrollArea(this);
    m_paneScrollArea->setObjectName("ColumnPaneScrollArea");
    m_paneScrollArea->setWidgetResizable(true);
    m_paneScrollArea->setFrameShape(QFrame::NoFrame);
    m_paneScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_paneScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_paneScrollArea->setContextMenuPolicy(Qt::CustomContextMenu);

    m_model = new DiskItemModel(this);
    m_model->setCurrentPath(path);

    // 1. 文件夹专用代理模型 (仅放行文件夹)
    m_folderProxyModel = new FilterProxyModel(this);
    m_folderProxyModel->setSourceModel(m_model);
    FilterState folderOnlyFilter;
    folderOnlyFilter.showFolders = true;
    folderOnlyFilter.showFiles = false;
    m_folderProxyModel->currentFilter = folderOnlyFilter;

    // 2. 文件专用代理模型 (仅放行文件)
    m_fileProxyModel = new FilterProxyModel(this);
    m_fileProxyModel->setSourceModel(m_model);
    FilterState fileOnlyFilter;
    fileOnlyFilter.showFolders = false;
    fileOnlyFilter.showFiles = true;
    m_fileProxyModel->currentFilter = fileOnlyFilter;

    m_proxyModel = m_fileProxyModel; // 兼容对外 proxyModel()

    // 3. 子文件夹列表视图
    m_folderListView = new DropListView();
    m_folderListView->setObjectName("ColumnViewFolderList");
    m_folderListView->setFrameShape(QFrame::NoFrame);
    m_folderListView->setFocusPolicy(Qt::StrongFocus);
    m_folderListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderListView->setDragEnabled(true);
    m_folderListView->setAcceptDrops(true);
    m_folderListView->setDropIndicatorShown(true);
    m_folderListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderListView->setModel(m_folderProxyModel);
    m_folderListView->setItemDelegate(new ColumnItemDelegate(this));
    m_folderListView->hide();

    // 4. 普通文件列表视图
    m_listView = new DropListView();
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setFocusPolicy(Qt::StrongFocus);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setDropIndicatorShown(true);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setModel(m_fileProxyModel);
    m_listView->setItemDelegate(new ColumnItemDelegate(this));

    m_panel = new DualSectionPanel(m_folderListView, m_listView, m_folderProxyModel, m_fileProxyModel, this);
    m_panel->setContextMenuPolicy(Qt::CustomContextMenu);
    m_panel->setFocusPolicy(Qt::StrongFocus);
    m_panel->setAcceptDrops(true);

    m_paneScrollArea->setWidget(m_panel);
    layout->addWidget(m_paneScrollArea);

    auto handlePaneBlankContextMenu = [this](const QPoint& pos, QWidget* sourceWidget) {
        if (!m_contentPanel) return;
        QPoint globalPos = sourceWidget ? sourceWidget->mapToGlobal(pos) : QCursor::pos();
        DropListView* targetView = m_listView ? m_listView : m_folderListView;
        if (targetView && targetView->viewport()) {
            QPoint viewPos = targetView->viewport()->mapFromGlobal(globalPos);
            m_contentPanel->onCustomContextMenuRequested(targetView, viewPos);
        }
    };

    connect(m_paneScrollArea, &QWidget::customContextMenuRequested, this, [this, handlePaneBlankContextMenu](const QPoint& pos) {
        handlePaneBlankContextMenu(pos, m_paneScrollArea);
    });
    connect(m_panel, &QWidget::customContextMenuRequested, this, [this, handlePaneBlankContextMenu](const QPoint& pos) {
        handlePaneBlankContextMenu(pos, m_panel);
    });

    auto updateSectionCountsAndHints = [this]() {
        tryPendingSelection();
        int viewportH = m_paneScrollArea && m_paneScrollArea->viewport() ? m_paneScrollArea->viewport()->height() : 0;
        m_panel->updateSectionCounts(viewportH);

        int folderCount = m_folderProxyModel ? m_folderProxyModel->rowCount() : 0;
        int fileCount = m_fileProxyModel ? m_fileProxyModel->rowCount() : 0;

        if (m_folderListView && folderCount > 0 && m_folderListView->isVisible()) {
            int rowH = m_folderListView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int folderH = folderCount * rowH + 2;
            if (fileCount == 0) {
                m_folderListView->setFixedHeight(qMax(folderH, m_panel->folderViewMinHeight()));
            } else {
                m_folderListView->setFixedHeight(folderH);
            }
        }

        if (m_listView && fileCount > 0) {
            int rowH = m_listView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int fileH = fileCount * rowH + 2;
            m_listView->setFixedHeight(qMax(fileH, m_panel->fileViewMinHeight()));
        }
        update();
    };

    connect(m_panel, &DualSectionPanel::folderCollapseToggled, this, [updateSectionCountsAndHints](bool) {
        updateSectionCountsAndHints();
    });

    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, updateSectionCountsAndHints);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, updateSectionCountsAndHints);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, updateSectionCountsAndHints);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, updateSectionCountsAndHints);

    connect(m_folderListView, &DropListView::blankSpaceClicked, this, [this]() {
        int paneIdx = property("paneIndex").toInt();
        if (m_contentPanel && m_contentPanel->columnView()) {
            m_contentPanel->columnView()->activatePaneFromBlankClick(paneIdx);
        }
    });
    connect(m_listView, &DropListView::blankSpaceClicked, this, [this]() {
        int paneIdx = property("paneIndex").toInt();
        if (m_contentPanel && m_contentPanel->columnView()) {
            m_contentPanel->columnView()->activatePaneFromBlankClick(paneIdx);
        }
    });

    connect(m_folderListView, &DropListView::blankSpaceDoubleClicked, this, [this]() {
        int paneIdx = property("paneIndex").toInt();
        emit blankSpaceDoubleClicked(paneIdx);
    });
    connect(m_listView, &DropListView::blankSpaceDoubleClicked, this, [this]() {
        int paneIdx = property("paneIndex").toInt();
        emit blankSpaceDoubleClicked(paneIdx);
    });

    m_paneScrollArea->installEventFilter(this);
    m_panel->installEventFilter(this);
    m_folderListView->installEventFilter(this);
    m_listView->installEventFilter(this);

    if (m_contentPanel) {
        m_folderListView->installEventFilter(m_contentPanel);
        m_listView->installEventFilter(m_contentPanel);
        connect(m_folderListView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_folderListView, pos);
            }
        });
        connect(m_listView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_listView, pos);
            }
        });
        connect(m_folderListView, &DropListView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            if (m_contentPanel) {
                m_contentPanel->onPathsDropped(paths, targetIndex, m_path, m_folderProxyModel);
            }
        });
        connect(m_listView, &DropListView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            if (m_contentPanel) {
                m_contentPanel->onPathsDropped(paths, targetIndex, m_path, m_fileProxyModel);
            }
        });
    }

    // 选区互斥联动与信号广播
    connect(m_folderListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        if (m_folderListView->selectionModel()->hasSelection() && m_listView->selectionModel()) {
            QSignalBlocker blocker(m_listView->selectionModel());
            m_listView->clearSelection();
        }
        emit selectionChanged();
    });
    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        if (m_listView->selectionModel()->hasSelection() && m_folderListView->selectionModel()) {
            QSignalBlocker blocker(m_folderListView->selectionModel());
            m_folderListView->clearSelection();
        }
        emit selectionChanged();
    });

    // 文件夹点击（仅选中高亮，不清空右侧子列）
    connect(m_folderListView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        int paneIdx = property("paneIndex").toInt();
        emit folderClicked(itemPath, paneIdx);
    });

    // 文件点击（仅选中高亮，不清空右侧子列）
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderClicked(itemPath, paneIdx);
        } else {
            emit fileClicked(itemPath, paneIdx);
        }
    });

    // 文件夹双击（触发展开与挂载新列）
    connect(m_folderListView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (index.isValid()) {
            QString itemPath = index.data(PathRole).toString();
            int paneIdx = property("paneIndex").toInt();
            emit folderExpandRequested(itemPath, paneIdx);
        }
    });

    // 文件/子文件夹双击
    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (index.isValid()) {
            QString itemPath = index.data(PathRole).toString();
            bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
            int paneIdx = property("paneIndex").toInt();
            if (isDir) {
                emit folderExpandRequested(itemPath, paneIdx);
            } else if (m_contentPanel) {
                m_contentPanel->onDoubleClicked(index);
            }
        }
    });
}

void ColumnViewPane::setActive(bool active) {
    if (m_isActive != active) {
        m_isActive = active;
        update();
    }
}

void ColumnViewPane::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    if (m_isActive) {
        painter.setPen(QPen(QColor("#3498db"), 1));
        painter.drawLine(0, 0, width(), 0);
    }
    if (m_folderListView && m_folderListView->isVisible()) {
        int folderBottom = m_folderListView->y() + m_folderListView->height();
        if (folderBottom >= height()) {
            painter.setPen(QPen(QColor("#3498db"), 1));
            painter.drawLine(0, height() - 1, width(), height() - 1);
        }
    }
}

bool ColumnViewPane::eventFilter(QObject* obj, QEvent* event) {
    if (event && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && (obj == m_paneScrollArea || obj == m_panel)) {
            int paneIdx = property("paneIndex").toInt();
            if (m_contentPanel && m_contentPanel->columnView()) {
                m_contentPanel->columnView()->activatePaneFromBlankClick(paneIdx);
            }
        }
    }

    if (event && event->type() == QEvent::KeyPress && (obj == m_folderListView || obj == m_listView)) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        int paneIdx = property("paneIndex").toInt();
        if (keyEvent->key() == Qt::Key_Right || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            DropListView* view = qobject_cast<DropListView*>(obj);
            if (view && view->currentIndex().isValid()) {
                QModelIndex idx = view->currentIndex();
                QString itemPath = idx.data(PathRole).toString();
                bool isDir = (idx.data(TypeRole).toString() == "folder") || idx.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
                if (isDir && !itemPath.isEmpty()) {
                    emit folderExpandRequested(itemPath, paneIdx);
                    return true;
                }
            }
        } else if (keyEvent->key() == Qt::Key_Left) {
            if (paneIdx > 0 && m_contentPanel && m_contentPanel->columnView()) {
                m_contentPanel->columnView()->focusPane(paneIdx - 1);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

DropListView* ColumnViewPane::listView() const { return m_listView; }
DropListView* ColumnViewPane::folderListView() const { return m_folderListView; }
FolderSectionHeaderBar* ColumnViewPane::folderHeader() const { return m_panel ? m_panel->folderHeader() : nullptr; }

void ColumnViewPane::refreshVisibleThumbnails() {
    if (m_panel && m_model && m_paneScrollArea && m_paneScrollArea->viewport()) {
        m_panel->refreshVisibleThumbnails(m_model, m_paneScrollArea->viewport());
    }
}

void ColumnViewPane::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_panel && m_paneScrollArea && m_paneScrollArea->viewport()) {
        int viewportH = m_paneScrollArea->viewport()->height();
        m_panel->updateSectionCounts(viewportH);
        int folderCount = m_folderProxyModel ? m_folderProxyModel->rowCount() : 0;
        int fileCount = m_fileProxyModel ? m_fileProxyModel->rowCount() : 0;
        if (m_folderListView && folderCount > 0) {
            int rowH = m_folderListView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int folderH = folderCount * rowH + 2;
            if (fileCount == 0) {
                m_folderListView->setFixedHeight(qMax(folderH, m_panel->folderViewMinHeight()));
            } else {
                m_folderListView->setFixedHeight(folderH);
            }
        }
        if (m_listView && fileCount > 0) {
            int rowH = m_listView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int fileH = fileCount * rowH + 2;
            m_listView->setFixedHeight(qMax(fileH, m_panel->fileViewMinHeight()));
        }
    }
    update();
}

void ColumnViewPane::setFilterState(const FilterState& state) {
    if (m_folderProxyModel) {
        FilterState s = state;
        s.showFolders = true;
        s.showFiles = false;
        m_folderProxyModel->currentFilter = s;
        m_folderProxyModel->updateFilter();
    }
    if (m_fileProxyModel) {
        FilterState s = state;
        s.showFolders = false;
        s.showFiles = true;
        m_fileProxyModel->currentFilter = s;
        m_fileProxyModel->updateFilter();
    }
}

void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    tryPendingSelection();
}

void ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths) {
    m_pendingSelectPaths = paths;
    m_pendingSelectPath.clear();
    tryPendingSelection();
}

void ColumnViewPane::applySort(int sortType, Qt::SortOrder sortOrder) {
    if (m_folderProxyModel) {
        m_folderProxyModel->setSortType(sortType);
        m_folderProxyModel->sort(0, sortOrder);
    }
    if (m_fileProxyModel) {
        m_fileProxyModel->setSortType(sortType);
        m_fileProxyModel->sort(0, sortOrder);
    }
}

void ColumnViewPane::tryPendingSelection() {
    if (!m_fileProxyModel || !m_listView) return;

    if (!m_pendingSelectPaths.isEmpty()) {
        QSet<QString> normalizedPending;
        normalizedPending.reserve(m_pendingSelectPaths.size());
        for (const QString& p : m_pendingSelectPaths) {
            normalizedPending.insert(QDir::toNativeSeparators(QDir::cleanPath(p)).toLower());
        }

        // 1. 尝试在普通文件代理中选择
        QItemSelection fileSel;
        QModelIndex lastFileIdx;
        if (m_fileProxyModel->rowCount() > 0) {
            for (int r = 0; r < m_fileProxyModel->rowCount(); ++r) {
                QModelIndex idx = m_fileProxyModel->index(r, 0);
                QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString())).toLower();
                if (normalizedPending.contains(itemPath)) {
                    fileSel.select(idx, idx);
                    lastFileIdx = idx;
                }
            }
        }

        // 2. 尝试在文件夹代理中选择
        QItemSelection folderSel;
        QModelIndex lastFolderIdx;
        if (m_folderProxyModel && m_folderProxyModel->rowCount() > 0) {
            for (int r = 0; r < m_folderProxyModel->rowCount(); ++r) {
                QModelIndex idx = m_folderProxyModel->index(r, 0);
                QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString())).toLower();
                if (normalizedPending.contains(itemPath)) {
                    folderSel.select(idx, idx);
                    lastFolderIdx = idx;
                }
            }
        }

        bool matchedAny = false;
        if (!fileSel.isEmpty() && m_listView->selectionModel()) {
            QSignalBlocker blocker(m_listView->selectionModel());
            m_listView->selectionModel()->select(fileSel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastFileIdx.isValid()) {
                m_listView->selectionModel()->setCurrentIndex(lastFileIdx, QItemSelectionModel::NoUpdate);
                m_listView->scrollTo(lastFileIdx, QAbstractItemView::PositionAtCenter);
            }
            matchedAny = true;
        }
        if (!folderSel.isEmpty() && m_folderListView && m_folderListView->selectionModel()) {
            QSignalBlocker blocker(m_folderListView->selectionModel());
            m_folderListView->selectionModel()->select(folderSel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastFolderIdx.isValid()) {
                m_folderListView->selectionModel()->setCurrentIndex(lastFolderIdx, QItemSelectionModel::NoUpdate);
                m_folderListView->scrollTo(lastFolderIdx, QAbstractItemView::PositionAtCenter);
            }
            matchedAny = true;
        }

        if (matchedAny) {
            m_pendingSelectPaths.clear();
            m_pendingSelectPath.clear();
            emit selectionChanged();
            return;
        }
    }

    if (!m_pendingSelectPath.isEmpty()) {
        QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(m_pendingSelectPath));
        QString targetName = QFileInfo(cleanTarget).fileName();

        // 优先在文件夹代理中寻找
        if (m_folderProxyModel && m_folderListView) {
            for (int r = 0; r < m_folderProxyModel->rowCount(); ++r) {
                QModelIndex idx = m_folderProxyModel->index(r, 0);
                QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
                QString itemName = QFileInfo(itemPath).fileName();

                if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0 ||
                    (!targetName.isEmpty() && QString::compare(itemName, targetName, Qt::CaseInsensitive) == 0)) {
                    if (m_folderListView->selectionModel()) {
                        QSignalBlocker blocker(m_folderListView->selectionModel());
                        m_folderListView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                        m_folderListView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
                    }
                    m_folderListView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
                    m_pendingSelectPath.clear();
                    emit selectionChanged();
                    return;
                }
            }
        }

        // 次选在文件代理中寻找
        if (m_fileProxyModel && m_listView) {
            for (int r = 0; r < m_fileProxyModel->rowCount(); ++r) {
                QModelIndex idx = m_fileProxyModel->index(r, 0);
                QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
                QString itemName = QFileInfo(itemPath).fileName();

                if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0 ||
                    (!targetName.isEmpty() && QString::compare(itemName, targetName, Qt::CaseInsensitive) == 0)) {
                    if (m_listView->selectionModel()) {
                        QSignalBlocker blocker(m_listView->selectionModel());
                        m_listView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                        m_listView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
                    }
                    m_listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
                    m_pendingSelectPath.clear();
                    emit selectionChanged();
                    return;
                }
            }
        }
    }
}

void ColumnViewWidget::setActivePaneIndex(int newIndex) {
    if (newIndex < 0 || newIndex >= m_panes.size()) return;
    m_activePaneIndex = newIndex;
    for (int i = 0; i < m_panes.size(); ++i) {
        if (m_panes[i]) {
            m_panes[i]->setActive(i == newIndex);
        }
    }
}

void ColumnViewWidget::activatePaneFromBlankClick(int paneIndex) {
    if (paneIndex >= 0 && paneIndex < m_panes.size()) {
        setActivePaneIndex(paneIndex);
        ColumnViewPane* pane = m_panes[paneIndex];
        if (pane) {
            pane->clearSelection();
            clearOtherSelections(paneIndex);
            focusPane(paneIndex);
            emit selectionChanged();
            emit pathNavigated(pane->currentPath());
        }
    }
}

void ColumnViewPane::clearSelection() {
    if (m_folderListView && m_folderListView->selectionModel()) {
        QSignalBlocker blocker(m_folderListView->selectionModel());
        m_folderListView->clearSelection();
    } else if (m_folderListView) {
        m_folderListView->clearSelection();
    }

    if (m_listView && m_listView->selectionModel()) {
        QSignalBlocker blocker(m_listView->selectionModel());
        m_listView->clearSelection();
    } else if (m_listView) {
        m_listView->clearSelection();
    }
}

void ColumnViewPane::loadDirectory() {
    QString path = m_path;
    bool recursive = false;
    if (m_contentPanel && m_contentPanel->isRecursive()) {
        if (m_contentPanel->columnView() &&
            m_contentPanel->columnView()->rightmostPane() == this) {
            recursive = true;
        }
    }
    QPointer<ColumnViewPane> weakSelf(this);
    (void)QtConcurrent::run([weakSelf, path, recursive]() {
        if (!weakSelf) return;
        std::vector<ItemRecord> items;
        if (path.isEmpty() || path == "computer://") {
            for (const QFileInfo& drive : QDir::drives()) {
                items.push_back(ItemRecord::create(drive.absolutePath()));
            }
        } else {
            items = DiskScanService::scanDirectory(path, recursive, [weakSelf]() {
                return weakSelf != nullptr;
            });
        }
        MetaCacheDecorator::decorate(items);
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakSelf, items]() {
            if (weakSelf && weakSelf->m_model) {
                weakSelf->m_model->setRecords(items);
                if (weakSelf->m_contentPanel) {
                    weakSelf->applySort(static_cast<int>(weakSelf->m_contentPanel->currentSortType()),
                                        weakSelf->m_contentPanel->currentSortOrder());
                }
                if (!weakSelf->m_pendingSelectPaths.isEmpty()) {
                    weakSelf->tryPendingSelection();
                } else if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
                // 触发图标与缩略图提取管线 (支持按需几何视口探测)
                int count = weakSelf->m_model->rowCount();
                if (count > 0) {
                    weakSelf->refreshVisibleThumbnails();
                }
                emit weakSelf->recordsLoaded(weakSelf->m_model->allRecords());
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

    m_blankCanvasWidget = new ColumnBlankCanvasWidget(this, m_contentPanel, m_container);
    m_blankCanvasWidget->show();

    setWidget(m_container);

    // 监听背景留白区域的双击事件，用于触发右侧空白区域双击回退
    viewport()->installEventFilter(this);
    m_container->installEventFilter(this);
    if (m_contentPanel) {
        installEventFilter(m_contentPanel);
        viewport()->installEventFilter(m_contentPanel);
        m_container->installEventFilter(m_contentPanel);
    }

    if (horizontalScrollBar()) {
        connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this](int min, int max) {
            Q_UNUSED(min);
            if (m_autoScrollToRight) {
                horizontalScrollBar()->setValue(max);
                m_autoScrollToRight = false;
            }
        });
    }
}

void ColumnViewWidget::scrollToRightmostPane() {
    m_autoScrollToRight = true;
    QMetaObject::invokeMethod(this, [this]() {
        if (horizontalScrollBar()) {
            horizontalScrollBar()->setValue(horizontalScrollBar()->maximum());
        }
        if (!m_panes.isEmpty() && m_panes.last()) {
            ensureWidgetVisible(m_panes.last(), 0, 0);
        }
    }, Qt::QueuedConnection);
}

ColumnViewPane* ColumnViewWidget::activePane() const {
    if (m_activePaneIndex >= 0 && m_activePaneIndex < m_panes.size()) {
        return m_panes[m_activePaneIndex];
    }
    return m_panes.isEmpty() ? nullptr : m_panes.last();
}

ColumnViewPane* ColumnViewWidget::rightmostPane() const {
    return m_panes.isEmpty() ? nullptr : m_panes.last();
}

void ColumnViewWidget::focusPane(int paneIndex) {
    if (paneIndex >= 0 && paneIndex < m_panes.size()) {
        setActivePaneIndex(paneIndex);
        ColumnViewPane* pane = m_panes[paneIndex];
        if (pane) {
            DropListView* view = pane->listView();
            if (pane->folderListView() && pane->folderListView()->selectionModel() && pane->folderListView()->selectionModel()->hasSelection()) {
                view = pane->folderListView();
            }
            if (view) view->setFocus();
        }
    }
}

bool ColumnViewWidget::containsPath(const QString& path) const {
    if (path.isEmpty()) return false;
    QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(path));
    for (auto* pane : m_panes) {
        if (pane) {
            QString panePath = QDir::toNativeSeparators(QDir::cleanPath(pane->currentPath()));
            if (QString::compare(panePath, cleanTarget, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
    }
    return false;
}

QStringList ColumnViewWidget::getSelectedPaths() const {
    ColumnViewPane* pane = activePane();
    if (!pane) return {};
    QStringList paths;
    if (pane->folderListView() && pane->folderListView()->selectionModel()) {
        for (const auto& idx : pane->folderListView()->selectionModel()->selectedIndexes()) {
            if (idx.column() == 0) {
                QString p = idx.data(PathRole).toString();
                if (!p.isEmpty()) paths << p;
            }
        }
    }
    if (pane->listView() && pane->listView()->selectionModel()) {
        for (const auto& idx : pane->listView()->selectionModel()->selectedIndexes()) {
            if (idx.column() == 0) {
                QString p = idx.data(PathRole).toString();
                if (!p.isEmpty()) paths << p;
            }
        }
    }
    return paths;
}

QModelIndexList ColumnViewWidget::getSelectedIndexes() const {
    ColumnViewPane* pane = activePane();
    if (!pane) return {};
    if (pane->folderListView() && pane->folderListView()->selectionModel() &&
        pane->folderListView()->selectionModel()->hasSelection()) {
        return pane->folderListView()->selectionModel()->selectedIndexes();
    }
    if (pane->listView() && pane->listView()->selectionModel()) {
        return pane->listView()->selectionModel()->selectedIndexes();
    }
    return {};
}

void ColumnViewWidget::applyFilterState(const FilterState& state) {
    m_currentFilter = state;
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            FilterState parentFilter;
            parentFilter.showHidden = m_currentFilter.showHidden;
            m_panes[i]->setFilterState(parentFilter);
        }
    }
}

void ColumnViewWidget::applySort(int sortType, Qt::SortOrder sortOrder) {
    for (auto* pane : m_panes) {
        if (pane) {
            pane->applySort(sortType, sortOrder);
        }
    }
}

void ColumnViewWidget::setRootPath(const QString& path) {
    clearAllColumns();
    if (path.isEmpty()) return;

    if (path == "computer://") {
        appendColumn("computer://");
        setActivePaneIndex(0);
        updatePaneWidths();
        return;
    }

    QString targetFilePath;
    QString dirPath = path;
    QFileInfo info(path);
    if (info.exists() && !info.isDir()) {
        targetFilePath = path;
        dirPath = info.absolutePath();
    }

    // 1. 拆分完整的祖先路径栈
    QList<QString> pathStack;
    QDir dir(dirPath);
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

    if (!targetFilePath.isEmpty() && !m_panes.isEmpty()) {
        m_panes.last()->selectItemByPath(targetFilePath);
    }

    setActivePaneIndex(m_panes.size() - 1);

    updatePaneWidths();
    updateParentHighlights();
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
    updateParentHighlights();
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            FilterState parentFilter;
            parentFilter.showHidden = m_currentFilter.showHidden;
            m_panes[i]->setFilterState(parentFilter);
        }
    }
    if (rightmostPane() && rightmostPane()->model()) {
        emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
    }
}

ColumnViewPane* ColumnViewWidget::appendColumn(const QString& path) {
    int newIdx = m_panes.size();
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    pane->setProperty("paneIndex", newIdx);

    connect(pane, &ColumnViewPane::blankSpaceDoubleClicked, this, [this](int paneIdx) {
        goUpColumnFromIndex(paneIdx);
    });
    if (m_contentPanel) {
        pane->applySort(static_cast<int>(m_contentPanel->currentSortType()), m_contentPanel->currentSortOrder());
    }
    pane->loadDirectory();

    connect(pane, &ColumnViewPane::recordsLoaded, this, [this, pane](const std::vector<ItemRecord>& records) {
        if (pane == rightmostPane()) {
            emit activeColumnRecordsChanged(records);
            if (m_contentPanel) {
                m_contentPanel->recalculateAndEmitStats();
            }
        }
    });

    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        setActivePaneIndex(pane->property("paneIndex").toInt());
        emit selectionChanged();
        if (rightmostPane() && rightmostPane()->model()) {
            emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
        }
    });

    connect(pane, &ColumnViewPane::folderClicked, this, [this, pane](const QString&, int paneIdx) {
        setActivePaneIndex(paneIdx);
        clearOtherSelections(paneIdx);
        emit selectionChanged();
        if (pane) {
            emit pathNavigated(pane->currentPath());
        }
    });

    connect(pane, &ColumnViewPane::fileClicked, this, [this, pane](const QString&, int paneIdx) {
        setActivePaneIndex(paneIdx);
        clearOtherSelections(paneIdx);
        emit selectionChanged();
        if (pane) {
            emit pathNavigated(pane->currentPath());
        }
    });

    auto handleFolderExpand = [this](const QString& folderPath, int paneIdx) {
        if (paneIdx + 1 < m_panes.size() &&
            QDir::cleanPath(m_panes[paneIdx + 1]->currentPath()) == QDir::cleanPath(folderPath)) {
            setActivePaneIndex(paneIdx + 1);
            focusPane(paneIdx + 1);
            emit selectionChanged();
            return;
        }

        dismissSubColumns(paneIdx);
        clearOtherSelections(paneIdx);
        appendColumn(folderPath);
        emit pathNavigated(folderPath);
        if (m_contentPanel) {
            m_contentPanel->recalculateAndEmitStats();
        }
    };

    connect(pane, &ColumnViewPane::folderExpandRequested, this, handleFolderExpand);
    connect(pane, &ColumnViewPane::folderSelected, this, handleFolderExpand);

    connect(pane, &ColumnViewPane::fileSelected, this, [this](const QString& filePath, int paneIdx) {
        setActivePaneIndex(paneIdx);
        emit selectionChanged();
        emit pathNavigated(filePath);
    });

    m_panes.append(pane);
    m_layout->addWidget(pane);
    setActivePaneIndex(newIdx);
    updatePaneWidths();
    updateParentHighlights();
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            FilterState parentFilter;
            parentFilter.showHidden = m_currentFilter.showHidden;
            m_panes[i]->setFilterState(parentFilter);
        }
    }
    scrollToRightmostPane();
    return pane;
}

void ColumnViewWidget::clearAllSelections() {
    for (auto* pane : m_panes) {
        if (pane) {
            pane->clearSelection();
        }
    }
    emit selectionChanged();
}

void ColumnViewWidget::toggleFolderSectionCollapse() {
    ColumnViewPane* pane = activePane();
    if (!pane) pane = rightmostPane();
    if (pane && pane->folderHeader() && pane->folderHeader()->isVisible() && pane->folderHeader()->count() > 0) {
        pane->folderHeader()->setCollapsed(!pane->folderHeader()->isCollapsed());
    }
}

void ColumnViewWidget::clearOtherSelections(int activePaneIdx) {
    for (int i = activePaneIdx + 1; i < m_panes.size(); ++i) {
        m_panes[i]->clearSelection();
    }
}

void ColumnViewWidget::updateParentHighlights() {
    for (int i = 0; i < m_panes.size(); ++i) {
        ColumnViewPane* parentPane = m_panes[i];
        if (!parentPane) continue;

        ColumnViewPane* childPane = (i + 1 < m_panes.size()) ? m_panes[i + 1] : nullptr;
        QString childPath = childPane ? QDir::toNativeSeparators(QDir::cleanPath(childPane->currentPath())) : "";

        for (FilterProxyModel* model : {parentPane->folderProxyModel(), parentPane->fileProxyModel()}) {
            if (!model) continue;
            for (int r = 0; r < model->rowCount(); ++r) {
                QModelIndex idx = model->index(r, 0);
                QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
                bool isExpandedParent = !childPath.isEmpty() && (QString::compare(itemPath, childPath, Qt::CaseInsensitive) == 0);
                model->setData(idx, isExpandedParent, IsParentExpandedRole);
            }
        }
    }
}

void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = 230;
    for (auto* pane : m_panes) {
        pane->setFixedWidth(defaultWidth);
        pane->setMinimumWidth(defaultWidth);
        pane->setMaximumWidth(defaultWidth);
    }

    int totalPanesWidth = m_panes.size() * defaultWidth;
    int containerHeight = m_container ? m_container->height() : viewport()->height();
    int blankWidth = qMax(230, viewport()->width() - totalPanesWidth);
    if (m_blankCanvasWidget) {
        m_blankCanvasWidget->setGeometry(totalPanesWidth, 0, blankWidth, qMax(containerHeight, viewport()->height()));
    }
    if (m_container) {
        m_container->setMinimumWidth(totalPanesWidth + blankWidth);
    }
}

void ColumnViewWidget::resizeEvent(QResizeEvent* event) {
    QScrollArea::resizeEvent(event);
    updatePaneWidths();
}

bool ColumnViewWidget::eventFilter(QObject* obj, QEvent* event) {
    if ((obj == viewport() || obj == m_container) && event && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent && mouseEvent->button() == Qt::LeftButton) {
            goUpColumnFromIndex(m_panes.size() - 1);
            return true;
        }
    }
    return QScrollArea::eventFilter(obj, event);
}

void ColumnViewWidget::refreshActiveColumn() {
    ColumnViewPane* pane = activePane();
    if (pane) {
        pane->loadDirectory();
    }
}

void ColumnViewWidget::refreshAllColumns() {
    for (auto* pane : m_panes) {
        if (pane) {
            pane->loadDirectory();
        }
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

void ColumnViewWidget::goUpColumn() {
    goUpColumnFromIndex(m_panes.size() - 1);
}

void ColumnViewWidget::goUpColumnFromIndex(int paneIndex) {
    if (m_panes.isEmpty()) {
        NavigationService::instance().goUp();
        return;
    }

    if (paneIndex >= m_panes.size() - 1) {
        // 双击发生在最右侧列或背景留白处：逐级关闭最右侧列
        if (m_panes.size() > 1) {
            dismissSubColumns(m_panes.size() - 2);
            ColumnViewPane* newActive = rightmostPane();
            if (newActive) {
                setActivePaneIndex(m_panes.size() - 1);
                emit pathNavigated(newActive->currentPath());
                emit selectionChanged();
            }
        } else {
            // 仅剩最后一列（如盘符根目录 G:/）时，降级退回“此电脑”(computer://)
            NavigationService::instance().goUp();
        }
    } else if (paneIndex >= 0) {
        // 双击发生在中间父列的空白处：裁撤该列右侧的所有子列并保持当前父列高亮
        dismissSubColumns(paneIndex);
        ColumnViewPane* newActive = rightmostPane();
        if (newActive) {
            setActivePaneIndex(m_panes.size() - 1);
            emit pathNavigated(newActive->currentPath());
            emit selectionChanged();
        }
    }
}

} // namespace QuarkMeta
