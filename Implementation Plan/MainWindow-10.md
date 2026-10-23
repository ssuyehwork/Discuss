# MainWindow Implementation Plan

## Overview
This plan inverts the placement order of status bar control buttons on the bottom-right in `src/ui/MainWindow.cpp` so that they are added in reversed sequence from left to right:
1. `m_btnResetLayout` (重置分栏 - far left)
2. `m_btnPresetLayout` (显示收藏栏+内容面板+筛选器)
3. `m_btnToggleNav` (隐藏目录导航)
4. `m_btnToggleFavorite` (隐藏收藏栏)
5. `m_btnContentPanel` (内容面板)
6. `m_btnToggleMeta` (隐藏元数据面板)
7. `m_btnToggleFilter` (隐藏筛选器 - far right)

## Modified Files List
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
    statusL->setSpacing(4);
    statusL->addWidget(m_btnToggleFilter);
    statusL->addWidget(m_btnToggleMeta);
    statusL->addWidget(m_btnContentPanel);
    statusL->addWidget(m_btnToggleFavorite);
    statusL->addWidget(m_btnToggleNav);
    statusL->addWidget(m_btnPresetLayout);
    statusL->addWidget(m_btnResetLayout);
=======
    statusL->setSpacing(4);
    statusL->addWidget(m_btnResetLayout);
    statusL->addWidget(m_btnPresetLayout);
    statusL->addWidget(m_btnToggleNav);
    statusL->addWidget(m_btnToggleFavorite);
    statusL->addWidget(m_btnContentPanel);
    statusL->addWidget(m_btnToggleMeta);
    statusL->addWidget(m_btnToggleFilter);
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `src/ui/MainWindow.cpp` layout button adding order: Reset Layout (far left) to Filter Toggle (far right).
2. Confirm button icon size is 26x20 px and tooltips trigger single `ToolTipOverlay` popups without native Qt tooltip duplication.
