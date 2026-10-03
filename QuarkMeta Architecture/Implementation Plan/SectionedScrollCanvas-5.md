# SectionedScrollCanvas-5 Implementation Plan

## 1. Overview
This plan implements the single unified view architecture for List View and Grid View inside `SectionedScrollCanvas`, discarding `DualSectionPanel` completely.
- Instead of creating and maintaining two sub-views (`folderView` + `fileView`) and two models (`folderProxy` + `fileProxy`), `SectionedScrollCanvas` manages a **single unified view** (`m_view`) backed by a **single unified proxy model** (`m_proxyModel`).
- The `FolderSectionHeaderBar` is retained as a sticky/top section bar inside the canvas layout. Clicking it toggles `m_proxyModel->setFoldersCollapsed(!collapsed)`.
- When collapsed, folder rows disappear from `m_view`, and file rows naturally take up the entire area.
- The entire view scrolls smoothly under `SectionedScrollCanvas`''s own single vertical scrollbar without nested or split scrollbars.
- Existing public methods (`folderView()`, `fileView()`, `folderProxyModel()`, `fileProxyModel()`) are preserved as aliases pointing to the unified view and unified proxy model, maintaining strict contract lock compliance.

## 2. Modified Files List
- `src/ui/SectionedScrollCanvas.h`
- `src/ui/SectionedScrollCanvas.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/SectionedScrollCanvas.h`
```diff
<<<<<<< SEARCH
    CanvasType canvasType() const { return m_type; }
    QAbstractItemView* folderView() const;
    QAbstractItemView* fileView() const;
    FolderSectionHeaderBar* folderHeader() const;
    FileSectionHeaderBar* fileHeader() const;
    FilterProxyModel* folderProxyModel() const { return m_folderProxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_fileProxyModel; }

    void updateSectionCounts();
    void updateZoom(int zoomLevel);
    void toggleFolderSectionCollapse();

    QAbstractItemView* activeItemView() const;
    QModelIndexList getSelectedIndexes() const;
    void refreshVisibleThumbnails(ItemModelBase* model);
    void triggerVisibleScan();
=======
    CanvasType canvasType() const { return m_type; }
    QAbstractItemView* view() const { return m_view; }
    QAbstractItemView* folderView() const { return m_view; }
    QAbstractItemView* fileView() const { return m_view; }
    FolderSectionHeaderBar* folderHeader() const { return m_folderHeader; }
    FileSectionHeaderBar* fileHeader() const { return nullptr; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    FilterProxyModel* folderProxyModel() const { return m_proxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_proxyModel; }

    void updateSectionCounts();
    void updateZoom(int zoomLevel);
    void toggleFolderSectionCollapse();

    QAbstractItemView* activeItemView() const { return m_view; }
    QModelIndexList getSelectedIndexes() const;
    void refreshVisibleThumbnails(ItemModelBase* model);
    void triggerVisibleScan();
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    CanvasType m_type;
    DualSectionPanel* m_panel = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
    QTimer* m_scrollThumbTimer = nullptr;
};
=======
    CanvasType m_type;
    QWidget* m_container = nullptr;
    QVBoxLayout* m_layout = nullptr;
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    QAbstractItemView* m_view = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    QTimer* m_scrollThumbTimer = nullptr;
};
>>>>>>> REPLACE
```

### 3.2 `src/ui/SectionedScrollCanvas.cpp`
```diff
<<<<<<< SEARCH
#include "DualSectionPanel.h"
#include "FolderSectionWidget.h"
=======
#include "FolderSectionWidget.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
SectionedScrollCanvas::SectionedScrollCanvas(CanvasType type, FilterProxyModel* folderProxy, FilterProxyModel* fileProxy, QObject* eventFilter, QWidget* parent)
    : QScrollArea(parent), m_type(type), m_folderProxyModel(folderProxy), m_fileProxyModel(fileProxy) {
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);

    QAbstractItemView* folderView = createFolderView(eventFilter);
    QAbstractItemView* fileView = createFileView(eventFilter);

    m_panel = new DualSectionPanel(folderView, fileView, m_folderProxyModel, m_fileProxyModel, this);
    m_panel->setContextMenuPolicy(Qt::CustomContextMenu);
    m_panel->setFocusPolicy(Qt::StrongFocus);
    m_panel->setAcceptDrops(true);
    setWidget(m_panel);

    if (eventFilter) {
        installEventFilter(eventFilter);
        if (viewport()) viewport()->installEventFilter(eventFilter);
        m_panel->installEventFilter(eventFilter);
    }


    setupConnections();

    m_scrollThumbTimer = new QTimer(this);
    m_scrollThumbTimer->setSingleShot(true);
    m_scrollThumbTimer->setInterval(60);
    connect(m_scrollThumbTimer, &QTimer::timeout, this, [this]() {
        if (m_folderProxyModel && m_folderProxyModel->sourceModel()) {
            if (auto* diskModel = qobject_cast<ItemModelBase*>(m_folderProxyModel->sourceModel())) {
                m_panel->refreshVisibleThumbnails(diskModel, viewport());
            }
        }
    });

    if (verticalScrollBar()) {
        connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            triggerVisibleScan();
        });
    }
}
=======
SectionedScrollCanvas::SectionedScrollCanvas(CanvasType type, FilterProxyModel* folderProxy, FilterProxyModel* fileProxy, QObject* eventFilter, QWidget* parent)
    : QScrollArea(parent), m_type(type), m_proxyModel(folderProxy ? folderProxy : fileProxy) {
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);

    m_container = new QWidget(this);
    m_container->setObjectName("SectionedScrollCanvasContainer");
    m_container->setContextMenuPolicy(Qt::CustomContextMenu);
    m_container->setFocusPolicy(Qt::StrongFocus);
    m_container->setAcceptDrops(true);

    m_layout = new QVBoxLayout(m_container);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->setAlignment(Qt::AlignTop);

    m_folderHeader = new FolderSectionHeaderBar(m_container);
    m_folderHeader->hide();
    m_layout->addWidget(m_folderHeader, 0);

    m_view = createFileView(eventFilter);
    if (m_view) {
        m_view->setParent(m_container);
        m_layout->addWidget(m_view, 1);
    }

    setWidget(m_container);

    if (eventFilter) {
        installEventFilter(eventFilter);
        if (viewport()) viewport()->installEventFilter(eventFilter);
        m_container->installEventFilter(eventFilter);
    }

    setupConnections();

    m_scrollThumbTimer = new QTimer(this);
    m_scrollThumbTimer->setSingleShot(true);
    m_scrollThumbTimer->setInterval(60);
    connect(m_scrollThumbTimer, &QTimer::timeout, this, [this]() {
        if (m_proxyModel && m_proxyModel->sourceModel()) {
            if (auto* diskModel = qobject_cast<ItemModelBase*>(m_proxyModel->sourceModel())) {
                refreshVisibleThumbnails(diskModel);
            }
        }
    });

    if (verticalScrollBar()) {
        connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            triggerVisibleScan();
        });
    }
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
QAbstractItemView* SectionedScrollCanvas::folderView() const { return m_panel->folderView(); }
QAbstractItemView* SectionedScrollCanvas::fileView() const { return m_panel->fileView(); }
FolderSectionHeaderBar* SectionedScrollCanvas::folderHeader() const { return m_panel->folderHeader(); }
FileSectionHeaderBar* SectionedScrollCanvas::fileHeader() const { return m_panel->fileHeader(); }

void SectionedScrollCanvas::setupConnections() {
    connect(m_panel, &DualSectionPanel::selectionChanged, this, &SectionedScrollCanvas::selectionChanged);
    connect(m_panel, &DualSectionPanel::folderCollapseToggled, this, [this](bool) {
        updateSectionCounts();
    });

    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();

    if (m_type == CanvasType::Grid) {
        if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
            connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int) {
                updateSectionCounts();
            });
            connect(fjv, &JustifiedView::layoutFinished, this, &SectionedScrollCanvas::triggerVisibleScan);
        }
        if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
            connect(jv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_fileProxyModel && m_fileProxyModel->rowCount() > 0) {
                    m_panel->fileView()->setFixedHeight(qMax(height, m_panel->fileViewMinHeight()));
                }
            });
            connect(jv, &JustifiedView::layoutFinished, this, &SectionedScrollCanvas::triggerVisibleScan);
        }
    }

    auto onModelChanged = [this]() {
        updateSectionCounts();
        triggerVisibleScan();
    };
    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);

    connect(folderView, &QAbstractItemView::doubleClicked, this, &SectionedScrollCanvas::doubleClicked);
    connect(fileView, &QAbstractItemView::doubleClicked, this, &SectionedScrollCanvas::doubleClicked);

    connect(this, &QScrollArea::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    connect(m_panel, &QWidget::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    connect(folderView, &QAbstractItemView::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    connect(fileView, &QAbstractItemView::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);

    if (m_type == CanvasType::Grid) {
        if (auto* dropFolder = qobject_cast<DropJustifiedView*>(folderView)) {
            connect(dropFolder, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_folderProxyModel);
            });
        }
        if (auto* dropFile = qobject_cast<DropJustifiedView*>(fileView)) {
            connect(dropFile, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_fileProxyModel);
            });
        }
    } else {
        if (auto* dropFolder = qobject_cast<DropTreeView*>(folderView)) {
            connect(dropFolder, &DropTreeView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_folderProxyModel);
            });
        }
        if (auto* dropFile = qobject_cast<DropTreeView*>(fileView)) {
            connect(dropFile, &DropTreeView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_fileProxyModel);
            });
        }
    }
}
=======
void SectionedScrollCanvas::setupConnections() {
    if (m_folderHeader) {
        connect(m_folderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
            if (m_proxyModel) {
                m_proxyModel->setFoldersCollapsed(collapsed);
            }
            updateSectionCounts();
        });
    }

    if (m_view && m_view->selectionModel()) {
        connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SectionedScrollCanvas::selectionChanged);
    }

    if (m_type == CanvasType::Grid) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_view)) {
            connect(jv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_view) {
                    m_view->setFixedHeight(qMax(height, viewport()->height()));
                }
            });
            connect(jv, &JustifiedView::layoutFinished, this, &SectionedScrollCanvas::triggerVisibleScan);
        }
    }

    auto onModelChanged = [this]() {
        updateSectionCounts();
        triggerVisibleScan();
    };
    if (m_proxyModel) {
        connect(m_proxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
        connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);
    }

    if (m_view) {
        connect(m_view, &QAbstractItemView::doubleClicked, this, &SectionedScrollCanvas::doubleClicked);
        connect(m_view, &QAbstractItemView::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    }

    connect(this, &QScrollArea::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    if (m_container) {
        connect(m_container, &QWidget::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    }

    if (m_type == CanvasType::Grid) {
        if (auto* dropView = qobject_cast<DropJustifiedView*>(m_view)) {
            connect(dropView, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_proxyModel);
            });
        }
    } else {
        if (auto* dropView = qobject_cast<DropTreeView*>(m_view)) {
            connect(dropView, &DropTreeView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_proxyModel);
            });
        }
    }
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void SectionedScrollCanvas::updateSectionCounts() {
    m_panel->updateSectionCounts(viewport()->height());
    if (!m_folderProxyModel || !m_fileProxyModel) return;

    int folderCount = m_folderProxyModel->rowCount();
    int fileCount = m_fileProxyModel->rowCount();
    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();

    if (folderView && folderCount > 0 && folderView->isVisible()) {
        if (m_type == CanvasType::Grid) {
            if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
                int baseH = fjv->totalHeight();
                if (fileCount == 0) {
                    folderView->setFixedHeight(qMax(baseH, m_panel->folderViewMinHeight()));
                } else {
                    folderView->setFixedHeight(baseH);
                }
            }
        } else {
            auto* tv = static_cast<QTreeView*>(folderView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            int baseH = folderCount * rowH + hdrH + 2;
            if (fileCount == 0) {
                folderView->setFixedHeight(qMax(baseH, m_panel->folderViewMinHeight()));
            } else {
                folderView->setFixedHeight(baseH);
            }
            folderView->updateGeometry();
        }
    }

    if (fileView && fileCount > 0) {
        if (m_type == CanvasType::Grid) {
            if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
                int targetH = qMax(jv->totalHeight(), m_panel->fileViewMinHeight());
                if (fileView->height() != targetH) {
                    fileView->setFixedHeight(targetH);
                }
            }
        } else {
            auto* tv = static_cast<QTreeView*>(fileView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            int targetH = qMax(fileCount * rowH + hdrH + 2, m_panel->fileViewMinHeight());
            if (fileView->height() != targetH) {
                fileView->setFixedHeight(targetH);
                fileView->updateGeometry();
            }
        }
    }
}
=======
void SectionedScrollCanvas::updateSectionCounts() {
    if (!m_proxyModel) return;

    int folderCount = 0;
    int fileCount = 0;
    if (auto* src = qobject_cast<ItemModelBase*>(m_proxyModel->sourceModel())) {
        const auto& recs = src->allRecords();
        for (const auto& r : recs) {
            if (r.isDir) folderCount++;
            else fileCount++;
        }
    }

    if (m_folderHeader) {
        m_folderHeader->setCount(folderCount);
        m_folderHeader->setVisible(folderCount > 0 && m_proxyModel->currentFilter.showFolders);
    }

    int totalVisible = m_proxyModel->rowCount();
    if (m_view && totalVisible > 0) {
        if (m_type == CanvasType::Grid) {
            if (auto* jv = qobject_cast<JustifiedView*>(m_view)) {
                int targetH = qMax(jv->totalHeight(), viewport()->height());
                if (m_view->height() != targetH) {
                    m_view->setFixedHeight(targetH);
                }
            }
        } else {
            auto* tv = static_cast<QTreeView*>(m_view);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            int targetH = qMax(totalVisible * rowH + hdrH + 2, viewport()->height());
            if (m_view->height() != targetH) {
                m_view->setFixedHeight(targetH);
                m_view->updateGeometry();
            }
        }
    }
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void SectionedScrollCanvas::updateZoom(int zoomLevel) {
    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();
    if (m_type == CanvasType::Grid) {
        if (auto* jv = qobject_cast<JustifiedView*>(fileView)) jv->setTargetRowHeight(zoomLevel);
        if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) fjv->setTargetRowHeight(zoomLevel);
    } else {
        QSize iconSize(qMax(16, zoomLevel - 8), qMax(16, zoomLevel - 8));
        if (auto* folderTree = qobject_cast<DropTreeView*>(folderView)) {
            folderTree->setIconSize(iconSize);
            folderTree->doItemsLayout();
        }
        if (auto* fileTree = qobject_cast<DropTreeView*>(fileView)) {
            if (auto* hdr = qobject_cast<ContentHeaderView*>(fileTree->header())) {
                hdr->setZoomLevel(zoomLevel);
            }
            fileTree->setIconSize(iconSize);
            fileTree->doItemsLayout();
        }
        updateSectionCounts();
    }
}

void SectionedScrollCanvas::toggleFolderSectionCollapse() {
    m_panel->toggleFolderSectionCollapse();
}

QAbstractItemView* SectionedScrollCanvas::activeItemView() const {
    return m_panel->activeItemView();
}

QModelIndexList SectionedScrollCanvas::getSelectedIndexes() const {
    return m_panel->getSelectedIndexes();
}

void SectionedScrollCanvas::refreshVisibleThumbnails(ItemModelBase* model) {
    m_panel->refreshVisibleThumbnails(model, viewport());
}
=======
void SectionedScrollCanvas::updateZoom(int zoomLevel) {
    if (m_type == CanvasType::Grid) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_view)) {
            jv->setTargetRowHeight(zoomLevel);
        }
    } else {
        QSize iconSize(qMax(16, zoomLevel - 8), qMax(16, zoomLevel - 8));
        if (auto* tree = qobject_cast<DropTreeView*>(m_view)) {
            if (auto* hdr = qobject_cast<ContentHeaderView*>(tree->header())) {
                hdr->setZoomLevel(zoomLevel);
            }
            tree->setIconSize(iconSize);
            tree->doItemsLayout();
        }
        updateSectionCounts();
    }
}

void SectionedScrollCanvas::toggleFolderSectionCollapse() {
    if (m_folderHeader && m_folderHeader->isVisible() && m_folderHeader->count() > 0) {
        m_folderHeader->setCollapsed(!m_folderHeader->isCollapsed());
    }
}

QModelIndexList SectionedScrollCanvas::getSelectedIndexes() const {
    QModelIndexList res;
    if (m_view && m_view->selectionModel() && m_view->selectionModel()->hasSelection()) {
        for (const auto& idx : m_view->selectionModel()->selectedIndexes()) {
            if (idx.column() == 0) res.append(idx);
        }
    }
    return res;
}

void SectionedScrollCanvas::refreshVisibleThumbnails(ItemModelBase* model) {
    if (!model || !m_view || !m_proxyModel || !viewport()) return;

    int scrollY = verticalScrollBar() ? verticalScrollBar()->value() : 0;
    int vpHeight = viewport()->height();
    QSet<int> visibleRows;

    if (auto* jv = qobject_cast<JustifiedView*>(m_view)) {
        if (!jv->isLayoutReady()) return;
        int topInContent = scrollY - m_view->y();
        int bottomInContent = topInContent + vpHeight;
        QList<int> proxyRows = jv->rowsInRange(topInContent, bottomInContent);
        for (int r : proxyRows) {
            QModelIndex srcIdx = m_proxyModel->mapToSource(m_proxyModel->index(r, 0));
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
    } else {
        int topInContent = scrollY - m_view->y();
        int bottomInContent = topInContent + vpHeight;
        int rowCount = m_proxyModel->rowCount();
        for (int r = 0; r < rowCount; ++r) {
            QModelIndex pIdx = m_proxyModel->index(r, 0);
            QRect rRect = m_view->visualRect(pIdx);
            if (!rRect.isValid() || rRect.isEmpty()) continue;
            if (rRect.bottom() < topInContent) continue;
            if (rRect.top() > bottomInContent) break;
            QModelIndex srcIdx = m_proxyModel->mapToSource(pIdx);
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
    }

    if (!visibleRows.isEmpty()) {
        model->loadThumbnailsForRows(visibleRows.values());
    }
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void SectionedScrollCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        auto* folderView = m_panel->folderView();
        auto* fileView = m_panel->fileView();
        if (folderView && folderView->selectionModel()) folderView->selectionModel()->clearSelection();
        if (fileView && fileView->selectionModel()) fileView->selectionModel()->clearSelection();
    }
    QScrollArea::mousePressEvent(event);
}
=======
void SectionedScrollCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_view && m_view->selectionModel()) {
            m_view->selectionModel()->clearSelection();
        }
    }
    QScrollArea::mousePressEvent(event);
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
        if (!paths.isEmpty()) {
            emit pathsDropped(paths, QModelIndex(), m_fileProxyModel);
            event->acceptProposedAction();
            return;
        }
=======
        if (!paths.isEmpty()) {
            emit pathsDropped(paths, QModelIndex(), m_proxyModel);
            event->acceptProposedAction();
            return;
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build via CMake:
   ```powershell
   cmake -B build -S .
   cmake --build build --config Release
   ```
2. Verify that List View (`DropTreeView`) and Grid View (`DropJustifiedView`) render in single-view layout.
3. Verify that scrolling operates via a single unified scrollbar.
4. Verify clicking the folder header toggles folder collapse seamlessly.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- `folderView()` / `fileView()` maintained as aliases to `m_view` to prevent any broken callers.
- Reuses `FolderSectionHeaderBar` directly without intermediate wrapping container.
- `DualSectionPanel` completely removed from `SectionedScrollCanvas`.

## 6. Header API Signature Verification
- `SectionedScrollCanvas::view() const`: Verified.
- `SectionedScrollCanvas::folderView() const`: Preserved signature.
- `SectionedScrollCanvas::fileView() const`: Preserved signature.
- `SectionedScrollCanvas::folderHeader() const`: Preserved signature.

## 7. Header Inclusion Chain & Type Completeness Check
- Removed `#include "DualSectionPanel.h"`.
- All types (`FolderSectionHeaderBar`, `DropTreeView`, `DropJustifiedView`) explicitly included.
