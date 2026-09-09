# MainWindow Implementation Plan

## Overview
This plan addresses the requirements for the status bar control buttons in `src/ui/MainWindow.cpp`:
1. **Update Icon Size**: Change `setIconSize` from `26 x 17 px` to `26 x 20 px` (`btn->setIconSize(QSize(26, 20));`).
2. **Invert Button Layout Order**: Reverse the placement order of status bar buttons in `setupStatusBar` so that Reset Layout and Preset Layout appear on the far left, followed by Navigation, Favorites, Content, Metadata, and Filter toggles.
3. **Fix Duplicate ToolTip Overlay**: Remove `btn->setToolTip(tip);` to prevent native Qt tooltip popups from overlapping with `ToolTipOverlay`.
4. **Independent Panel Visibility Toggling**: Verify and preserve the independent panel toggle signal connections (`m_btnToggleFilter`, `m_btnToggleMeta`, `m_btnToggleFavorite`, `m_btnToggleNav`) which call `m_panelLayoutManager->setPanelVisible(...)` on their respective panels without touching any other panel's state.

## Modified Files List
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
        btn->setIconSize(QSize(26, 17));
        btn->setObjectName("StatusBarControlBtn");
        btn->setProperty("tooltipText", tip);
        btn->setToolTip(tip);
=======
        btn->setIconSize(QSize(26, 20));
        btn->setObjectName("StatusBarControlBtn");
        btn->setProperty("tooltipText", tip);
>>>>>>> REPLACE
```

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

### Verified Independent Toggle Connection Slots (Preserved in `MainWindow.cpp`):
```cpp
    connect(m_btnToggleFilter, &QPushButton::clicked, this, [this]() {
        if (m_panelLayoutManager) {
            bool current = m_panelLayoutManager->isPanelVisible("filter");
            m_panelLayoutManager->setPanelVisible("filter", !current);
        }
    });

    connect(m_btnToggleMeta, &QPushButton::clicked, this, [this]() {
        if (m_panelLayoutManager) {
            bool current = m_panelLayoutManager->isPanelVisible("meta");
            m_panelLayoutManager->setPanelVisible("meta", !current);
        }
    });

    connect(m_btnToggleFavorite, &QPushButton::clicked, this, [this]() {
        if (m_panelLayoutManager) {
            bool current = m_panelLayoutManager->isPanelVisible("favorite");
            m_panelLayoutManager->setPanelVisible("favorite", !current);
        }
    });

    connect(m_btnToggleNav, &QPushButton::clicked, this, [this]() {
        if (m_panelLayoutManager) {
            bool current = m_panelLayoutManager->isPanelVisible("nav");
            m_panelLayoutManager->setPanelVisible("nav", !current);
        }
    });
```

## Build & Verification Steps
1. Inspect `src/ui/MainWindow.cpp` to verify that `setIconSize` is set to `(26, 20)`.
2. Ensure status bar control buttons display in the inverted sequence from left to right: Reset Layout, Preset Layout, Toggle Navigation, Toggle Favorites, Content Panel (Immersive), Toggle Metadata, Toggle Filter.
3. Confirm that hovering over status bar buttons displays only a single `ToolTipOverlay` popup without native Qt tooltip duplication.
4. Confirm that clicking each toggle button independently toggles its respective panel without affecting other panels.
