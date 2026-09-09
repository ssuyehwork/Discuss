# MainWindow Implementation Plan

## Overview
This plan adds three view mode option buttons (`自适应(A)`, `网格(G)`, `列表(L)`) to the status bar in `src/ui/MainWindow.h` and `src/ui/MainWindow.cpp`:
1. **View Mode Buttons Placement**: Positioned to the left of the `Reset Layout` button (the original leftmost status button), separated by a 1px vertical line (`|`).
   - Sequence from left to right:
     1. `m_btnToggleJustified` (`resize2`, "自适应(A)")
     2. `m_btnToggleGrid` (`gridgapm`, "网格(G)")
     3. `m_btnToggleList` (`list_ul`, "列表(L)")
     4. Vertical Line Separator (`|`, 1px wide, 14px high, color `#444444`)
     5. `m_btnResetLayout` (重置分栏 - original leftmost button)
     6. `m_btnPresetLayout` (三栏预设)
     7. `m_btnToggleNav` (隐藏目录导航)
     8. `m_btnToggleFavorite` (隐藏收藏栏)
     9. `m_btnContentPanel` (内容面板)
     10. `m_btnToggleMeta` (隐藏元数据面板)
     11. `m_btnToggleFilter` (隐藏筛选器 - far right)
2. **View Mode Actions**:
   - `m_btnToggleJustified`: Switches `m_contentPanel` to `JustifiedViewMode`.
   - `m_btnToggleGrid`: Switches `m_contentPanel` to `GridView`.
   - `m_btnToggleList`: Switches `m_contentPanel` to `ListView`.
3. **View Mode Highlighting**:
   - `updateStatusBarButtonHighlights()` synchronizes the checked state of the 3 view mode buttons with `m_contentPanel->currentViewMode()`.

## Modified Files List
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/MainWindow.h`

```diff
<<<<<<< SEARCH
    QPushButton* m_btnResetLayout = nullptr;
=======
    QPushButton* m_btnResetLayout = nullptr;
    QPushButton* m_btnToggleJustified = nullptr;
    QPushButton* m_btnToggleGrid = nullptr;
    QPushButton* m_btnToggleList = nullptr;
>>>>>>> REPLACE
```

### 2. `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
    m_btnToggleFilter   = createStatusBtn("隐藏筛选器", "切换筛选器面板 (显示/隐藏)");
=======
    m_btnToggleJustified = createStatusBtn("resize2", "自适应(A)");
    m_btnToggleGrid      = createStatusBtn("gridgapm", "网格(G)");
    m_btnToggleList      = createStatusBtn("list_ul", "列表(L)");

    m_btnToggleFilter   = createStatusBtn("隐藏筛选器", "切换筛选器面板 (显示/隐藏)");
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    connect(m_btnResetLayout, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", true);
        m_panelLayoutManager->setPanelVisible("favorite", true);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", true);
        m_panelLayoutManager->setPanelVisible("filter", true);
        m_panelLayoutManager->resetSplitterLayout();
        updateStatusBarButtonHighlights();
    });

    statusL->setSpacing(4);
    statusL->addWidget(m_btnResetLayout);
=======
    connect(m_btnToggleJustified, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::JustifiedViewMode);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleGrid, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::GridView);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleList, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::ListView);
            updateStatusBarButtonHighlights();
        }
    });

    if (m_contentPanel) {
        connect(m_contentPanel, &ContentPanel::viewModeChanged, this, [this](ContentPanel::ViewMode) {
            updateStatusBarButtonHighlights();
        });
    }

    connect(m_btnResetLayout, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", true);
        m_panelLayoutManager->setPanelVisible("favorite", true);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", true);
        m_panelLayoutManager->setPanelVisible("filter", true);
        m_panelLayoutManager->resetSplitterLayout();
        updateStatusBarButtonHighlights();
    });

    QFrame* sepLine = new QFrame(m_statusBarWidget);
    sepLine->setFrameShape(QFrame::VLine);
    sepLine->setFixedWidth(1);
    sepLine->setFixedHeight(14);
    sepLine->setStyleSheet("background-color: #444444; border: none;");

    statusL->setSpacing(4);
    statusL->addWidget(m_btnToggleJustified);
    statusL->addWidget(m_btnToggleGrid);
    statusL->addWidget(m_btnToggleList);
    statusL->addWidget(sepLine);
    statusL->addWidget(m_btnResetLayout);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    QSignalBlocker b7(m_btnResetLayout);

    if (m_btnToggleFilter)   m_btnToggleFilter->setChecked(false);
=======
    QSignalBlocker b7(m_btnResetLayout);
    QSignalBlocker b8(m_btnToggleJustified);
    QSignalBlocker b9(m_btnToggleGrid);
    QSignalBlocker b10(m_btnToggleList);

    if (m_contentPanel) {
        ContentPanel::ViewMode mode = m_contentPanel->currentViewMode();
        if (m_btnToggleJustified) m_btnToggleJustified->setChecked(mode == ContentPanel::JustifiedViewMode);
        if (m_btnToggleGrid)      m_btnToggleGrid->setChecked(mode == ContentPanel::GridView);
        if (m_btnToggleList)      m_btnToggleList->setChecked(mode == ContentPanel::ListView);
    }

    if (m_btnToggleFilter)   m_btnToggleFilter->setChecked(false);
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `src/ui/MainWindow.h` declares member variables for the 3 view mode buttons.
2. Confirm status bar layout order: `Justified`, `Grid`, `List`, `|` vertical line separator, `Reset Layout`, `Preset Layout`, `Toggle Nav`, `Toggle Favorite`, `Content Panel`, `Toggle Meta`, `Toggle Filter`.
3. Verify clicking each view mode button changes `m_contentPanel`'s view mode and updates button checked highlights.
