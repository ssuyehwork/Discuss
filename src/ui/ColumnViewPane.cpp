#include "ColumnViewPane.h"
#include "ContentPanel.h"
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
#include <QSignalBlocker>
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

    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setDynamicSortFilter(true);

    m_containerWidget = new QWidget(this);
    m_containerLayout = new QVBoxLayout(m_containerWidget);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_containerLayout->setSpacing(0);

    m_folderHeader = new FolderSectionHeaderBar(m_containerWidget);
    m_folderHeader->hide();
    m_containerLayout->addWidget(m_folderHeader);

    m_unifiedListView = new DropListView();
    m_unifiedListView->setObjectName("ColumnViewPaneListView");
    m_unifiedListView->setFrameShape(QFrame::NoFrame);
    m_unifiedListView->setFocusPolicy(Qt::StrongFocus);
    m_unifiedListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_unifiedListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_unifiedListView->setDragEnabled(true);
    m_unifiedListView->setAcceptDrops(true);
    m_unifiedListView->setDropIndicatorShown(true);
    m_unifiedListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_unifiedListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_unifiedListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_unifiedListView->setModel(m_proxyModel);
    m_unifiedListView->setItemDelegate(new ColumnItemDelegate(this));
    m_containerLayout->addWidget(m_unifiedListView);

    m_fileHeader = new FileSectionHeaderBar(m_containerWidget);
    m_fileHeader->hide();
    m_containerLayout->addWidget(m_fileHeader);

    m_paneScrollArea->setWidget(m_containerWidget);
    layout->addWidget(m_paneScrollArea);

    auto handlePaneBlankContextMenu = [this](const QPoint& pos, QWidget* sourceWidget) {
        if (!m_contentPanel) return;
        QPoint globalPos = sourceWidget ? sourceWidget->mapToGlobal(pos) : QCursor::pos();
        DropListView* targetView = m_unifiedListView;
        if (targetView && targetView->viewport()) {
            QPoint viewPos = targetView->viewport()->mapFromGlobal(globalPos);
            m_contentPanel->onCustomContextMenuRequested(targetView, viewPos);
        }
    };

    connect(m_paneScrollArea, &QWidget::customContextMenuRequested, this, [this, handlePaneBlankContextMenu](const QPoint& pos) {
        handlePaneBlankContextMenu(pos, m_paneScrollArea);
    });
    connect(m_containerWidget, &QWidget::customContextMenuRequested, this, [this, handlePaneBlankContextMenu](const QPoint& pos) {
        handlePaneBlankContextMenu(pos, m_containerWidget);
    });

    auto updateSectionCountsAndHints = [this]() {
        tryPendingSelection();
        int total = m_proxyModel ? m_proxyModel->rowCount() : 0;
        int folderCount = 0;
        int fileCount = 0;
        for (int i = 0; i < total; ++i) {
            QModelIndex idx = m_proxyModel->index(i, 0);
            bool isDir = idx.data(TypeRole).toString() == "folder" || idx.data(Qt::UserRole + 2).toBool();
            if (isDir) folderCount++;
            else fileCount++;
        }

        if (m_folderHeader) {
            m_folderHeader->setCount(folderCount);
            m_folderHeader->setVisible(folderCount > 0);
        }
        if (m_fileHeader) {
            m_fileHeader->setCount(fileCount);
            m_fileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }

        if (m_unifiedListView && total > 0) {
            int rowH = m_unifiedListView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int totalH = total * rowH + 2;
            m_unifiedListView->setFixedHeight(totalH);
        }
        update();
    };

    connect(m_proxyModel, &QAbstractItemModel::modelReset, this, updateSectionCountsAndHints);
    connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, updateSectionCountsAndHints);

    connect(m_unifiedListView, &DropListView::blankSpaceClicked, this, [this]() {
        int paneIdx = property("paneIndex").toInt();
        if (m_contentPanel && m_contentPanel->columnView()) {
            m_contentPanel->columnView()->activatePaneFromBlankClick(paneIdx);
        }
    });

    connect(m_unifiedListView, &DropListView::blankSpaceDoubleClicked, this, [this]() {
        int paneIdx = property("paneIndex").toInt();
        emit blankSpaceDoubleClicked(paneIdx);
    });

    if (m_paneScrollArea && m_paneScrollArea->verticalScrollBar()) {
        connect(m_paneScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            refreshVisibleThumbnails();
        });
    }

    m_paneScrollArea->installEventFilter(this);
    m_containerWidget->installEventFilter(this);
    m_unifiedListView->installEventFilter(this);
    if (m_unifiedListView->viewport()) m_unifiedListView->viewport()->installEventFilter(this);

    if (m_contentPanel) {
        m_unifiedListView->installEventFilter(m_contentPanel);
        m_paneScrollArea->installEventFilter(m_contentPanel);
        m_containerWidget->installEventFilter(m_contentPanel);

        connect(m_unifiedListView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            emit contextMenuRequested(m_unifiedListView, pos);
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_unifiedListView, pos);
            }
        });
        connect(m_unifiedListView, &DropListView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            emit pathsDroppedSignal(paths, targetIndex, m_path);
            if (m_contentPanel) {
                m_contentPanel->onPathsDropped(paths, targetIndex, m_path, m_proxyModel);
            }
        });
    }

    if (m_unifiedListView->selectionModel()) {
        connect(m_unifiedListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            emit selectionChanged();
        });
    }

    connect(m_unifiedListView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderClicked(itemPath, paneIdx);
        } else {
            emit fileClicked(itemPath, paneIdx);
        }
    });

    connect(m_unifiedListView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
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
}

bool ColumnViewPane::eventFilter(QObject* obj, QEvent* event) {
    if (event && event->type() == QEvent::Wheel) {
        auto* wEvent = static_cast<QWheelEvent*>(event);
        if (!(wEvent->modifiers() & Qt::ControlModifier)) {
            if (m_paneScrollArea && m_paneScrollArea->verticalScrollBar() && m_paneScrollArea->verticalScrollBar()->isVisible()) {
                int delta = wEvent->angleDelta().y();
                if (delta == 0) delta = wEvent->pixelDelta().y();
                if (delta != 0) {
                    QScrollBar* sb = m_paneScrollArea->verticalScrollBar();
                    int step = sb->singleStep();
                    if (step <= 0) step = 20;
                    int scrollAmount = (delta / 120.0) * step * 3;
                    if (scrollAmount == 0) scrollAmount = (delta > 0 ? -step : step);
                    else scrollAmount = -scrollAmount;
                    sb->setValue(sb->value() + scrollAmount);
                    return true;
                }
            }
        }
    }

    if (event && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && (obj == m_paneScrollArea || obj == m_containerWidget)) {
            int paneIdx = property("paneIndex").toInt();
            if (m_contentPanel && m_contentPanel->columnView()) {
                m_contentPanel->columnView()->activatePaneFromBlankClick(paneIdx);
            }
        }
    }

    if (event && event->type() == QEvent::KeyPress && obj == m_unifiedListView) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        int paneIdx = property("paneIndex").toInt();
        if (keyEvent->key() == Qt::Key_Right || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (m_unifiedListView && m_unifiedListView->currentIndex().isValid()) {
                QModelIndex idx = m_unifiedListView->currentIndex();
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

void ColumnViewPane::refreshVisibleThumbnails() {
}

void ColumnViewPane::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

void ColumnViewPane::setFilterState(const FilterState& state) {
    if (m_proxyModel) {
        m_proxyModel->currentFilter = state;
        m_proxyModel->updateFilter();
    }
}

void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    tryPendingSelection();
}

void ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths, bool edit) {
    m_pendingSelectPaths = paths;
    m_pendingSelectPath.clear();
    m_isPendingEdit = edit;
    tryPendingSelection();
}

void ColumnViewPane::applySort(int sortType, Qt::SortOrder sortOrder) {
    if (m_proxyModel) {
        m_proxyModel->setSortType(sortType);
        m_proxyModel->sort(0, sortOrder);
    }
}

void ColumnViewPane::tryPendingSelection() {
    if (!m_proxyModel || !m_unifiedListView) return;

    if (!m_pendingSelectPaths.isEmpty()) {
        QSet<QString> normalizedPending;
        normalizedPending.reserve(m_pendingSelectPaths.size());
        for (const QString& p : m_pendingSelectPaths) {
            normalizedPending.insert(QDir::toNativeSeparators(QDir::cleanPath(p)).toLower());
        }

        QItemSelection sel;
        QModelIndex lastIdx;
        if (m_proxyModel->rowCount() > 0) {
            for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
                QModelIndex idx = m_proxyModel->index(r, 0);
                QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString())).toLower();
                if (normalizedPending.contains(itemPath)) {
                    sel.select(idx, idx);
                    lastIdx = idx;
                }
            }
        }

        if (!sel.isEmpty() && m_unifiedListView->selectionModel()) {
            QSignalBlocker blocker(m_unifiedListView->selectionModel());
            m_unifiedListView->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastIdx.isValid()) {
                m_unifiedListView->selectionModel()->setCurrentIndex(lastIdx, QItemSelectionModel::NoUpdate);
                m_unifiedListView->scrollTo(lastIdx, QAbstractItemView::PositionAtCenter);
            }
            m_pendingSelectPaths.clear();
            m_pendingSelectPath.clear();
            if (m_isPendingEdit && lastIdx.isValid()) {
                m_isPendingEdit = false;
                QPointer<DropListView> weakView(m_unifiedListView);
                QTimer::singleShot(0, this, [weakView, lastIdx]() {
                    if (weakView && lastIdx.isValid()) {
                        weakView->setFocus();
                        weakView->setCurrentIndex(lastIdx);
                        weakView->edit(lastIdx);
                    }
                });
            }
            emit selectionChanged();
            return;
        }
    }

    if (!m_pendingSelectPath.isEmpty()) {
        QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(m_pendingSelectPath));
        QString targetName = QFileInfo(cleanTarget).fileName();

        if (m_proxyModel && m_unifiedListView) {
            for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
                QModelIndex idx = m_proxyModel->index(r, 0);
                QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
                QString itemName = QFileInfo(itemPath).fileName();

                if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0 ||
                    (!targetName.isEmpty() && QString::compare(itemName, targetName, Qt::CaseInsensitive) == 0)) {
                    if (m_unifiedListView->selectionModel()) {
                        QSignalBlocker blocker(m_unifiedListView->selectionModel());
                        m_unifiedListView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                        m_unifiedListView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
                    }
                    m_unifiedListView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
                    m_pendingSelectPath.clear();
                    emit selectionChanged();
                    return;
                }
            }
        }
    }
}

void ColumnViewPane::clearSelection() {
    if (m_unifiedListView && m_unifiedListView->selectionModel()) {
        QSignalBlocker blocker(m_unifiedListView->selectionModel());
        m_unifiedListView->clearSelection();
    } else if (m_unifiedListView) {
        m_unifiedListView->clearSelection();
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
