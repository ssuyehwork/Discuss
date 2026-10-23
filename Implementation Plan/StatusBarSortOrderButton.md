# Implementation Plan - Status Bar Sort Order Toggle Button (StatusBarSortOrderButton.md)

## 1. Overview
This implementation plan adds a dedicated Sort Order toggle button (`m_btnToggleSortOrder`) on the status bar, located immediately to the left of the "Justified" (`m_btnToggleJustified`) view mode button, separated by a visual vertical line (`|`).

When the current sort order is Ascending (`Qt::AscendingOrder`):
- The button displays `arrow_up_long.svg`.
- The button stays checked/highlighted (`setChecked(true)`).

When the current sort order is Descending (`Qt::DescendingOrder`):
- The button displays `arrow_down_long.svg`.
- The button stays unchecked/normal (`setChecked(false)`).

Clicking the button toggles the sort order via `ContentPanel::setSortOrder`, which synchronously updates the model sort and updates the status bar highlights.

## 2. Modified Files List
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/MainWindow.h`
Add `m_btnToggleSortOrder` member variable to `MainWindow`.

```
<<<<<<< SEARCH
    QPushButton* m_btnToggleJustified = nullptr;
    QPushButton* m_btnToggleGrid = nullptr;
    QPushButton* m_btnToggleList = nullptr;
=======
    QPushButton* m_btnToggleSortOrder = nullptr;
    QPushButton* m_btnToggleJustified = nullptr;
    QPushButton* m_btnToggleGrid = nullptr;
    QPushButton* m_btnToggleList = nullptr;
>>>>>>> REPLACE
```

### 3.2 Update `src/ui/MainWindow.cpp`
In `setupStatusBar()`:
1. Initialize `m_btnToggleSortOrder` using `createSquareStatusBtn`.
2. Connect `m_btnToggleSortOrder`'s `clicked` signal to toggle sort order in `ContentPanel`.
3. Add a vertical separator line between `m_btnToggleSortOrder` and `m_btnToggleJustified`.
4. Add layout items in exact order: `m_btnToggleSortOrder`, `sepLineSort`, `m_btnToggleJustified`, `m_btnToggleGrid`, `m_btnToggleList`, `sepLine`, ...

In `updateStatusBarButtonHighlights()`:
1. Add `QSignalBlocker b11(m_btnToggleSortOrder);`.
2. Query `m_contentPanel->currentSortOrder()`.
3. If Ascending, set icon to `arrow_up_long.svg`, set tooltip "升序 (点击切换降序)", and set `setChecked(true)`.
4. If Descending, set icon to `arrow_down_long.svg`, set tooltip "降序 (点击切换升序)", and set `setChecked(false)`.

```
<<<<<<< SEARCH
    m_btnToggleJustified = createSquareStatusBtn("resize2", "自适应(A)");
    m_btnToggleGrid      = createSquareStatusBtn("gridgapm", "网格(G)");
    m_btnToggleList      = createSquareStatusBtn("list_ul", "列表(L)");
=======
    m_btnToggleSortOrder = createSquareStatusBtn("arrow_down_long", "排序方向 (降序)");
    m_btnToggleJustified = createSquareStatusBtn("resize2", "自适应(A)");
    m_btnToggleGrid      = createSquareStatusBtn("gridgapm", "网格(G)");
    m_btnToggleList      = createSquareStatusBtn("list_ul", "列表(L)");
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(m_btnToggleJustified, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::JustifiedViewMode);
            updateStatusBarButtonHighlights();
        }
    });
=======
    connect(m_btnToggleSortOrder, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            Qt::SortOrder current = m_contentPanel->currentSortOrder();
            Qt::SortOrder next = (current == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
            m_contentPanel->setSortOrder(next);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleJustified, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::JustifiedViewMode);
            updateStatusBarButtonHighlights();
        }
    });
>>>>>>> REPLACE

```
<<<<<<< SEARCH
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
=======
    QFrame* sepLineSort = new QFrame(m_statusBarWidget);
    sepLineSort->setFrameShape(QFrame::VLine);
    sepLineSort->setFixedWidth(1);
    sepLineSort->setFixedHeight(14);
    sepLineSort->setStyleSheet("background-color: #444444; border: none;");

    QFrame* sepLine = new QFrame(m_statusBarWidget);
    sepLine->setFrameShape(QFrame::VLine);
    sepLine->setFixedWidth(1);
    sepLine->setFixedHeight(14);
    sepLine->setStyleSheet("background-color: #444444; border: none;");

    statusL->setSpacing(4);
    statusL->addWidget(m_btnToggleSortOrder);
    statusL->addWidget(sepLineSort);
    statusL->addWidget(m_btnToggleJustified);
    statusL->addWidget(m_btnToggleGrid);
    statusL->addWidget(m_btnToggleList);
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
    QSignalBlocker b11(m_btnToggleSortOrder);

    if (m_contentPanel) {
        ContentPanel::ViewMode mode = m_contentPanel->currentViewMode();
        if (m_btnToggleJustified) m_btnToggleJustified->setChecked(mode == ContentPanel::JustifiedViewMode);
        if (m_btnToggleGrid)      m_btnToggleGrid->setChecked(mode == ContentPanel::GridView);
        if (m_btnToggleList)      m_btnToggleList->setChecked(mode == ContentPanel::ListView);

        Qt::SortOrder sortOrd = m_contentPanel->currentSortOrder();
        if (m_btnToggleSortOrder) {
            bool isAsc = (sortOrd == Qt::AscendingOrder);
            m_btnToggleSortOrder->setIcon(UiHelper::getIcon(isAsc ? "arrow_up_long" : "arrow_down_long", QColor("#EEEEEE"), 18));
            m_btnToggleSortOrder->setProperty("tooltipText", isAsc ? "升序 (点击切换降序)" : "降序 (点击切换升序)");
            m_btnToggleSortOrder->setChecked(isAsc);
        }
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps

1. **Build Verification**:
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```

2. **Functional & Visual Verification**:
   - Inspect the bottom status bar: verify that `m_btnToggleSortOrder` appears to the left of `m_btnToggleJustified` ("自适应"), separated by a vertical line `|`.
   - Toggle sorting order via right-click context menu or by clicking `m_btnToggleSortOrder`.
   - When sorting is Ascending (`Qt::AscendingOrder`):
     - Icon is `arrow_up_long.svg`.
     - Button is highlighted (`setChecked(true)`).
   - When sorting is Descending (`Qt::DescendingOrder`):
     - Icon is `arrow_down_long.svg`.
     - Button is normal (`setChecked(false)`).
