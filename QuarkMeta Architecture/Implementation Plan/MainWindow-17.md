# Implementation Plan - MainWindow-17: Restore Missing Column View Button in Status Bar

## 1. Overview
The "Column View" (列视图) feature is fully supported in `ContentPanel` (`ContentPanel::ViewModeColumn`) and selectable via the top `TitleBarWidget` menu. However, the status bar in `MainWindow.cpp` only instantiated three view mode toggle buttons (`m_btnToggleJustified`, `m_btnToggleGrid`, and `m_btnToggleList`). While `m_btnToggleColumn` was declared in `MainWindow.h`, it was never instantiated or added to the status bar layout, and its button highlight sync was missing in `updateStatusBarButtonHighlights()`.

This implementation plan restores `m_btnToggleColumn` in `MainWindow.cpp`, connects its click event to switch to `ContentPanel::ViewModeColumn`, adds it to the status bar layout, and synchronizes its highlight state.

## 2. Modified Files List
- `src/ui/MainWindow.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/MainWindow.cpp`

```
<<<<<<< SEARCH
    m_btnToggleJustified = createSquareStatusBtn("resize2", "自适应(A)");
    m_btnToggleGrid      = createSquareStatusBtn("gridgapm", "网格(G)");
    m_btnToggleList      = createSquareStatusBtn("list_ul", "列表(L)");

    m_btnToggleFilter   = createStatusBtn("隐藏筛选器", "切换筛选器面板 (显示/隐藏)");
=======
    m_btnToggleJustified = createSquareStatusBtn("resize2", "自适应(A)");
    m_btnToggleGrid      = createSquareStatusBtn("gridgapm", "网格(G)");
    m_btnToggleList      = createSquareStatusBtn("list_ul", "列表(L)");
    m_btnToggleColumn    = createSquareStatusBtn("column_view", "列(C)");

    m_btnToggleFilter   = createStatusBtn("隐藏筛选器", "切换筛选器面板 (显示/隐藏)");
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(m_btnToggleList, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::ListView);
            updateStatusBarButtonHighlights();
        }
    });

    if (m_contentPanel) {
=======
    connect(m_btnToggleList, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::ListView);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleColumn, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::ViewModeColumn);
            updateStatusBarButtonHighlights();
        }
    });

    if (m_contentPanel) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    statusL->setSpacing(4);
    statusL->addWidget(m_btnToggleJustified);
    statusL->addWidget(m_btnToggleGrid);
    statusL->addWidget(m_btnToggleList);
    statusL->addWidget(sepLine);
=======
    statusL->setSpacing(4);
    statusL->addWidget(m_btnToggleJustified);
    statusL->addWidget(m_btnToggleGrid);
    statusL->addWidget(m_btnToggleList);
    statusL->addWidget(m_btnToggleColumn);
    statusL->addWidget(sepLine);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QSignalBlocker b8(m_btnToggleJustified);
    QSignalBlocker b9(m_btnToggleGrid);
    QSignalBlocker b10(m_btnToggleList);

    if (m_contentPanel) {
        ContentPanel::ViewMode mode = m_contentPanel->currentViewMode();
        if (m_btnToggleJustified) m_btnToggleJustified->setChecked(mode == ContentPanel::JustifiedViewMode);
        if (m_btnToggleGrid)      m_btnToggleGrid->setChecked(mode == ContentPanel::GridView);
        if (m_btnToggleList)      m_btnToggleList->setChecked(mode == ContentPanel::ListView);
    }
=======
    QSignalBlocker b8(m_btnToggleJustified);
    QSignalBlocker b9(m_btnToggleGrid);
    QSignalBlocker b10(m_btnToggleList);
    QSignalBlocker b11(m_btnToggleColumn);

    if (m_contentPanel) {
        ContentPanel::ViewMode mode = m_contentPanel->currentViewMode();
        if (m_btnToggleJustified) m_btnToggleJustified->setChecked(mode == ContentPanel::JustifiedViewMode);
        if (m_btnToggleGrid)      m_btnToggleGrid->setChecked(mode == ContentPanel::GridView);
        if (m_btnToggleList)      m_btnToggleList->setChecked(mode == ContentPanel::ListView);
        if (m_btnToggleColumn)    m_btnToggleColumn->setChecked(mode == ContentPanel::ViewModeColumn);
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps

1. Configure project with CMake:
   `cmake -B build -G "Ninja"`
2. Build project:
   `cmake --build build`
3. Launch application and verify that:
   - The status bar at the bottom right displays 4 view mode buttons (Adaptive, Grid, List, Column).
   - Clicking the Column View button (`m_btnToggleColumn`) switches the content area to multi-column navigation view (`ColumnViewWidget`).
   - The Column View button highlights when active, and switching via the top menu updates the status bar button highlight synchronously.
