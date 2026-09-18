#include "ColumnViewWidget.h"
#include "ContentPanel.h"
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
        setFixedWidth(230);
        setAcceptDrops(true);
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, this, &ColumnBlankCanvasWidget::onContextMenuRequested);
    }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && m_columnView) {
            m_columnView->clearAllSelections();
        }
        QWidget::mousePressEvent(event);
    }

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
            m_contentPanel->onCustomContextMenuRequested(mapToGlobal(pos));
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
    layout->setContentsMargins(0, 0, 1, 0);
    layout->setSpacing(0);

    m_paneScrollArea = new QScrollArea(this);
    m_paneScrollArea->setObjectName("ColumnPaneScrollArea");
    m_paneScrollArea->setWidgetResizable(true);
    m_paneScrollArea->setFrameShape(QFrame::NoFrame);
    m_paneScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_paneScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_canvasWidget = new QWidget(m_paneScrollArea);
    m_canvasWidget->setObjectName("ColumnPaneCanvasWidget");
    QVBoxLayout* canvasLayout = new QVBoxLayout(m_canvasWidget);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->setSpacing(0);

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

    // 3. 顶部子文件夹折叠条
    m_folderHeader = new FolderSectionHeaderBar(m_canvasWidget);
    m_folderHeader->hide();
    canvasLayout->addWidget(m_folderHeader);

    // 4. 子文件夹列表视图
    m_folderListView = new DropListView(m_canvasWidget);
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
    canvasLayout->addWidget(m_folderListView);

    connect(m_folderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderListView && m_folderHeader->count() > 0) {
            m_folderListView->setVisible(!collapsed);
        }
    });

    // 5. 内容文件区分界条
    m_fileHeader = new FileSectionHeaderBar(m_canvasWidget);
    m_fileHeader->hide();
    canvasLayout->addWidget(m_fileHeader);

    // 6. 普通文件列表视图
    m_listView = new DropListView(m_canvasWidget);
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
    canvasLayout->addWidget(m_listView);

    m_emptyFilterHintLabel = new QLabel(m_canvasWidget);
    m_emptyFilterHintLabel->setAlignment(Qt::AlignCenter);
    m_emptyFilterHintLabel->setWordWrap(true);
    m_emptyFilterHintLabel->setStyleSheet("color: #888888; font-size: 12px; padding: 16px;");
    m_emptyFilterHintLabel->hide();
    canvasLayout->addWidget(m_emptyFilterHintLabel);

    canvasLayout->addStretch(1);

    m_paneScrollArea->setWidget(m_canvasWidget);
    layout->addWidget(m_paneScrollArea);

    auto updateSectionCountsAndHints = [this]() {
        tryPendingSelection();
        int folderCount = m_folderProxyModel ? m_folderProxyModel->rowCount() : 0;
        int fileCount = m_fileProxyModel ? m_fileProxyModel->rowCount() : 0;

        if (m_folderHeader) {
            m_folderHeader->setCount(folderCount);
        }
        if (m_folderListView) {
            if (folderCount == 0) {
                m_folderListView->hide();
            } else {
                bool collapsed = m_folderHeader ? m_folderHeader->isCollapsed() : false;
                m_folderListView->setVisible(!collapsed);
                int folderH = qMax(28, folderCount * 28 + 4);
                m_folderListView->setFixedHeight(folderH);
            }
        }
        if (m_fileHeader) {
            m_fileHeader->setCount(fileCount);
            m_fileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
        if (m_listView) {
            if (fileCount == 0) {
                m_listView->hide();
            } else {
                m_listView->show();
                int fileH = qMax(28, fileCount * 28 + 4);
                m_listView->setFixedHeight(fileH);
            }
        }

        if (!m_model || !m_emptyFilterHintLabel) return;
        int fullCount = m_model->rowCount();
        int visibleCount = folderCount + fileCount;
        int hiddenCount = fullCount - visibleCount;

        if (fullCount > 0 && visibleCount == 0) {
            m_emptyFilterHintLabel->setText(QString("所有内容已被筛选隐藏 (%1 个项目)").arg(hiddenCount));
            m_emptyFilterHintLabel->show();
            if (m_listView) m_listView->hide();
        } else {
            m_emptyFilterHintLabel->hide();
            if (m_listView && fileCount > 0) m_listView->show();
        }
        update();
    };

    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, updateSectionCountsAndHints);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, updateSectionCountsAndHints);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, updateSectionCountsAndHints);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, updateSectionCountsAndHints);

    connect(m_folderListView, &DropListView::blankSpaceDoubleClicked, this, [this]() {
        int paneIdx = property("paneIndex").toInt();
        emit blankSpaceDoubleClicked(paneIdx);
    });
    connect(m_listView, &DropListView::blankSpaceDoubleClicked, this, [this]() {
        int paneIdx = property("paneIndex").toInt();
        emit blankSpaceDoubleClicked(paneIdx);
    });

    if (m_contentPanel) {
        m_folderListView->installEventFilter(m_contentPanel);
        m_listView->installEventFilter(m_contentPanel);
        connect(m_folderListView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
        connect(m_listView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
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

    // 文件夹点击
    connect(m_folderListView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        int paneIdx = property("paneIndex").toInt();
        emit folderSelected(itemPath, paneIdx);
    });

    // 文件点击
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderSelected(itemPath, paneIdx);
        } else {
            emit fileSelected(itemPath, paneIdx);
        }
    });

    connect(m_folderListView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (m_contentPanel && index.isValid()) {
            m_contentPanel->onDoubleClicked(index);
        }
    });
    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (m_contentPanel && index.isValid()) {
            m_contentPanel->onDoubleClicked(index);
        }
    });
}

void ColumnViewPane::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    if (m_folderListView && m_folderListView->isVisible()) {
        int folderBottom = m_folderListView->y() + m_folderListView->height();
        if (folderBottom >= height()) {
            QPainter painter(this);
            painter.setPen(QPen(QColor("#3498db"), 1));
            painter.drawLine(0, height() - 1, width(), height() - 1);
        }
    }
}

void ColumnViewPane::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
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

void ColumnViewPane::clearSelection() {
    if (m_folderListView) {
        m_folderListView->clearSelection();
    }
    if (m_listView) {
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
            items = DiskScanService::scanDirectory(path, recursive, std::function<bool()>());
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
                // 触发图标与缩略图提取管线
                int count = weakSelf->m_model->rowCount();
                if (count > 0) {
                    QList<int> visibleRows;
                    visibleRows.reserve(count);
                    for (int r = 0; r < count; ++r) visibleRows.append(r);
                    weakSelf->m_model->loadThumbnailsForRows(visibleRows);
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
    m_layout->addWidget(m_blankCanvasWidget);

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
        m_activePaneIndex = 0;
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

    m_activePaneIndex = m_panes.size() - 1;

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
    m_activePaneIndex = newIdx;

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
        m_activePaneIndex = pane->property("paneIndex").toInt();
        emit selectionChanged();
        if (rightmostPane() && rightmostPane()->model()) {
            emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
        }
    });

    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        if (paneIdx + 1 < m_panes.size() &&
            QDir::cleanPath(m_panes[paneIdx + 1]->currentPath()) == QDir::cleanPath(folderPath)) {
            dismissSubColumns(paneIdx + 1);
            m_activePaneIndex = paneIdx + 1;
            emit selectionChanged();
            return;
        }

        dismissSubColumns(paneIdx);
        // 保持父列高亮：仅清空 paneIdx 右侧深层列的选区，保留 paneIdx 及其左侧父列的高亮
        for (int i = paneIdx + 1; i < m_panes.size(); ++i) {
            m_panes[i]->clearSelection();
        }
        appendColumn(folderPath);
        emit pathNavigated(folderPath);
        if (m_contentPanel) {
            m_contentPanel->recalculateAndEmitStats();
        }
    });

    connect(pane, &ColumnViewPane::fileSelected, this, [this](const QString& filePath, int paneIdx) {
        m_activePaneIndex = paneIdx;
        dismissSubColumns(paneIdx);
        clearOtherSelections(paneIdx);
        emit pathNavigated(filePath);
    });

    m_panes.append(pane);
    if (m_blankCanvasWidget) {
        m_layout->removeWidget(m_blankCanvasWidget);
    }
    m_layout->addWidget(pane);
    if (m_blankCanvasWidget) {
        m_layout->addWidget(m_blankCanvasWidget);
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

void ColumnViewWidget::clearOtherSelections(int activePaneIdx) {
    for (int i = activePaneIdx + 1; i < m_panes.size(); ++i) {
        m_panes[i]->clearSelection();
    }
}

void ColumnViewWidget::updateParentHighlights() {
    for (int i = 0; i < m_panes.size(); ++i) {
        ColumnViewPane* parentPane = m_panes[i];
        if (!parentPane || !parentPane->proxyModel()) continue;

        ColumnViewPane* childPane = (i + 1 < m_panes.size()) ? m_panes[i + 1] : nullptr;
        QString childPath = childPane ? QDir::toNativeSeparators(QDir::cleanPath(childPane->currentPath())) : "";
        FilterProxyModel* model = parentPane->proxyModel();

        for (int r = 0; r < model->rowCount(); ++r) {
            QModelIndex idx = model->index(r, 0);
            QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
            bool isExpandedParent = !childPath.isEmpty() && (QString::compare(itemPath, childPath, Qt::CaseInsensitive) == 0);
            model->setData(idx, isExpandedParent, IsParentExpandedRole);
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
                m_activePaneIndex = m_panes.size() - 1;
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
            m_activePaneIndex = m_panes.size() - 1;
            emit pathNavigated(newActive->currentPath());
            emit selectionChanged();
        }
    }
}

} // namespace QuarkMeta
