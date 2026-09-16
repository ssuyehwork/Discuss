# ColumnViewWidget-14.md: Column Width Decoupling & Rightmost Blank Canvas Column

## 1. Overview
This implementation plan systemically refactors `ColumnViewWidget` and `ColumnViewPane`:
1. Decouples column widths by removing hardcoded `setFixedWidth(230)` overrides on `ColumnViewPane` instances.
2. Introduces a first-class `ColumnBlankCanvasWidget` (`m_blankCanvasWidget`) appended at the rightmost edge of `ColumnViewWidget`'s container layout. The blank canvas guarantees a full-width column of interaction canvas space (e.g. 230px) regardless of scroll position or expanded pane count.
3. Empowers `ColumnBlankCanvasWidget` as a first-class interaction canvas:
   - **Drop Area**: External drag-and-drop onto the blank canvas deposits files into the rightmost pane's directory (`rightmostPane()->currentPath()`) with hover highlighting.
   - **Context Menu**: Right-clicking the blank canvas invokes `ContentPanel`'s blank-area context menu for the rightmost pane directory (New Folder, New File, Paste, Refresh).
   - **Double-Click Go Up**: Double-clicking the blank canvas executes `goUpColumn()`, closing sub-columns and moving up one level.
   - **Single-Click Deselect**: Single-clicking the blank canvas clears selections across all open panes.

---

## 2. Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ColumnViewWidget.h`

<<<<<<< SEARCH
    ContentPanel* m_contentPanel = nullptr;
    FilterState m_currentFilter;
    int m_activePaneIndex = -1;
    bool m_autoScrollToRight = false;
    QWidget* m_container = nullptr;
    QHBoxLayout* m_layout = nullptr;
    QList<ColumnViewPane*> m_panes;
};
=======
    void clearAllSelections();

    ContentPanel* m_contentPanel = nullptr;
    FilterState m_currentFilter;
    int m_activePaneIndex = -1;
    bool m_autoScrollToRight = false;
    QWidget* m_container = nullptr;
    QHBoxLayout* m_layout = nullptr;
    QList<ColumnViewPane*> m_panes;
    QWidget* m_blankCanvasWidget = nullptr;
};
>>>>>>> REPLACE

---

### File 2: `src/ui/ColumnViewWidget.cpp`

<<<<<<< SEARCH
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
=======
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
>>>>>>> REPLACE

---

<<<<<<< SEARCH
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
    m_layout->addWidget(pane);
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
=======
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
>>>>>>> REPLACE

---

<<<<<<< SEARCH
void ColumnViewWidget::clearOtherSelections(int activePaneIdx) {
    for (int i = activePaneIdx + 1; i < m_panes.size(); ++i) {
        m_panes[i]->clearSelection();
    }
}
=======
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
>>>>>>> REPLACE

---

<<<<<<< SEARCH
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = 230;
    for (auto* pane : m_panes) {
        pane->setFixedWidth(defaultWidth);
        pane->setMinimumWidth(defaultWidth);
        pane->setMaximumWidth(defaultWidth);
    }
}
=======
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = 230;
    for (auto* pane : m_panes) {
        pane->setMinimumWidth(defaultWidth);
    }
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### Verification Commands
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Verification Checklist
1. Open Column View mode in QuarkMeta.
2. Verify that column widths are decoupled and flexible without rigid `setFixedWidth(230)` constraints.
3. Verify that `ColumnBlankCanvasWidget` appears as a 230px canvas column at the rightmost edge of open panes.
4. Drag and drop external files onto the blank canvas; verify files drop into `rightmostPane()->currentPath()`.
5. Right-click the blank canvas; verify the blank area context menu pops up.
6. Double-click the blank canvas; verify `goUpColumn()` executes and collapses the rightmost sub-column.
7. Single-click the blank canvas; verify all items are deselected.
