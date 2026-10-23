# MainWindow Implementation Plan

## Overview
This plan updates the 3 view mode option buttons (`自适应(A)`, `网格(G)`, `列表(L)`) in `src/ui/MainWindow.cpp` to render as 1:1 standard square buttons (`22 x 22 px`, icon size `18 x 18 px`), matching the title bar button shape, while keeping the status bar panel layout buttons as rectangular (`28 x 20 px`, icon size `27 x 24 px`).

## Modified Files List
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
    m_btnToggleJustified = createStatusBtn("resize2", "自适应(A)");
    m_btnToggleGrid      = createStatusBtn("gridgapm", "网格(G)");
    m_btnToggleList      = createStatusBtn("list_ul", "列表(L)");
=======
    auto createSquareStatusBtn = [this](const QString& iconKey, const QString& tip) -> QPushButton* {
        QPushButton* btn = new QPushButton(m_statusBarWidget);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setAttribute(Qt::WA_Hover);
        btn->setFixedSize(22, 22);
        btn->setCheckable(true);
        btn->setIcon(UiHelper::getIcon(iconKey, QColor("#EEEEEE"), 18));
        btn->setIconSize(QSize(18, 18));
        btn->setObjectName("StatusBarControlBtn");
        btn->setProperty("tooltipText", tip);
        if (m_hoverFilter) {
            btn->installEventFilter(m_hoverFilter);
        }
        return btn;
    };

    m_btnToggleJustified = createSquareStatusBtn("resize2", "自适应(A)");
    m_btnToggleGrid      = createSquareStatusBtn("gridgapm", "网格(G)");
    m_btnToggleList      = createSquareStatusBtn("list_ul", "列表(L)");
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `src/ui/MainWindow.cpp` creates the 3 view mode buttons using `createSquareStatusBtn` with 22x22 fixed size and 18x18 icon size.
2. Confirm the 3 view mode buttons render as 1:1 square buttons, geometrically distinct from the 28x20 rectangular layout buttons on the right.
