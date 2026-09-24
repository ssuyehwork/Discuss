#include "ColumnViewPane.h"
#include "ContentPanel.h"
#include "DualSectionPanel.h"
#include "ColumnViewWidget.h"
#include "ColumnItemDelegate.h"
#include "../core/DiskScanService.h"
#include "../meta/MetaCacheDecorator.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

namespace QuarkMeta {

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

    m_folderProxyModel = new FilterProxyModel(this);
    m_folderProxyModel->setSourceModel(m_model);
    FilterState folderOnlyFilter;
    folderOnlyFilter.showFolders = true;
    folderOnlyFilter.showFiles = false;
    m_folderProxyModel->currentFilter = folderOnlyFilter;

    m_fileProxyModel = new FilterProxyModel(this);
    m_fileProxyModel->setSourceModel(m_model);
    FilterState fileOnlyFilter;
    fileOnlyFilter.showFolders = false;
    fileOnlyFilter.showFiles = true;
    m_fileProxyModel->currentFilter = fileOnlyFilter;

    m_proxyModel = m_fileProxyModel;

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

    if (m_paneScrollArea && m_paneScrollArea->verticalScrollBar()) {
        connect(m_paneScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            refreshVisibleThumbnails();
        });
    }

    m_paneScrollArea->installEventFilter(this);
    m_panel->installEventFilter(this);
    m_folderListView->installEventFilter(this);
    m_listView->installEventFilter(this);

    if (m_contentPanel) {
        m_folderListView->installEventFilter(m_contentPanel);
        m_listView->installEventFilter(m_contentPanel);
        connect(m_folderListView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            emit contextMenuRequested(m_folderListView, pos);
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_folderListView, pos);
            }
        });
        connect(m_listView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            emit contextMenuRequested(m_listView, pos);
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_listView, pos);
            }
        });
        connect(m_folderListView, &DropListView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            emit pathsDroppedSignal(paths, targetIndex, m_path);
            if (m_contentPanel) {
                m_contentPanel->onPathsDropped(paths, targetIndex, m_path, m_folderProxyModel);
            }
        });
        connect(m_listView, &DropListView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            emit pathsDroppedSignal(paths, targetIndex, m_path);
            if (m_contentPanel) {
                m_contentPanel->onPathsDropped(paths, targetIndex, m_path, m_fileProxyModel);
            }
        });
    }

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

    connect(m_folderListView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        int paneIdx = property("paneIndex").toInt();
        emit folderClicked(itemPath, paneIdx);
    });

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

    connect(m_folderListView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (index.isValid()) {
            QString itemPath = index.data(PathRole).toString();
            int paneIdx = property("paneIndex").toInt();
            emit folderExpandRequested(itemPath, paneIdx);
        }
    });

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
    // 【架构红线 - 蓝色顶部焦点提示线设计约束】
    // 此处绘制的顶部蓝线（颜色 #3498db，宽度 1px）是"列视图（Column View）中活跃列"的焦点视觉标识。
    // ─────────────────────────────────────────────────────────────────────────────────
    // ✅ 唯一合法应用范围：ColumnViewPane（列视图中的单列窗格），当 m_isActive == true 时绘制。
    // ❌ 严禁应用范围：
    //    - ContentPanel（内容面板/窗格）     → 其活跃状态由 ContentPaneSplitManager::setActivePane
    //                                          通过 QSS property "activePane" 独立管理（border样式）。
    //    - ContentHeaderWidget（面板标题栏） → 其活跃状态由 ContentHeaderWidget::setActive
    //                                          通过 QSS property "activePane" 独立管理。
    //    - 任何其他非 ColumnViewPane 类型的 Widget。
    // ─────────────────────────────────────────────────────────────────────────────────
    // 两条"活跃"状态机（列焦点 vs 窗格焦点）完全正交，绝对不可混用或合并。
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
            m_contentPanel->columnView()->activePane() == this) {
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
                int count = weakSelf->m_model->rowCount();
                if (count > 0) {
                    weakSelf->refreshVisibleThumbnails();
                }
                emit weakSelf->recordsLoaded(weakSelf->m_model->allRecords());
            }
        });
    });
}

} // namespace QuarkMeta
