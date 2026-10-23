# MainWindow Implementation Plan

## Overview
This plan simplifies status bar control button highlight logic in `MainWindow` (`src/ui/MainWindow.cpp` & `src/ui/MainWindow.h`):
1. **Direct Independent Toggle Highlighting**: Each panel toggle button (`m_btnToggleNav`, `m_btnToggleFavorite`, `m_btnToggleMeta`, `m_btnToggleFilter`) directly reflects its own panel's visibility state (`setChecked(vis)`). If the panel is visible, its button is highlighted; if hidden, it is dark.
2. **Clean Layout Sync**: `updateStatusBarButtonHighlights()` sets each button's checked state independently based on `isPanelVisible(panelName)`, without complex mutual exclusion state tracking.
3. **Inverted Layout & Icon Size**: Preserves `setIconSize(QSize(26, 20))`, inverted button layout order, and `ToolTipOverlay` single popup behavior.

## Modified Files List
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/MainWindow.h`

```diff
<<<<<<< SEARCH
    QPushButton* m_btnResetLayout = nullptr;
    QPushButton* m_activeStatusBtn = nullptr;
=======
    QPushButton* m_btnResetLayout = nullptr;
>>>>>>> REPLACE
```

### 2. `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
    connect(m_btnToggleFilter, &QPushButton::clicked, this, [this]() {
        m_activeStatusBtn = m_btnToggleFilter;
        if (m_panelLayoutManager) {
            bool current = m_panelLayoutManager->isPanelVisible("filter");
            m_panelLayoutManager->setPanelVisible("filter", !current);
        }
    });

    connect(m_btnToggleMeta, &QPushButton::clicked, this, [this]() {
        m_activeStatusBtn = m_btnToggleMeta;
        if (m_panelLayoutManager) {
            bool current = m_panelLayoutManager->isPanelVisible("meta");
            m_panelLayoutManager->setPanelVisible("meta", !current);
        }
    });

    connect(m_btnContentPanel, &QPushButton::clicked, this, [this]() {
        m_activeStatusBtn = m_btnContentPanel;
        if (m_panelLayoutManager) {
            m_panelLayoutManager->toggleImmersiveMode();
        }
    });

    connect(m_btnToggleFavorite, &QPushButton::clicked, this, [this]() {
        m_activeStatusBtn = m_btnToggleFavorite;
        if (m_panelLayoutManager) {
            bool current = m_panelLayoutManager->isPanelVisible("favorite");
            m_panelLayoutManager->setPanelVisible("favorite", !current);
        }
    });

    connect(m_btnToggleNav, &QPushButton::clicked, this, [this]() {
        m_activeStatusBtn = m_btnToggleNav;
        if (m_panelLayoutManager) {
            bool current = m_panelLayoutManager->isPanelVisible("nav");
            m_panelLayoutManager->setPanelVisible("nav", !current);
        }
    });

    connect(m_btnPresetLayout, &QPushButton::clicked, this, [this]() {
        m_activeStatusBtn = m_btnPresetLayout;
        QString presetLeft = AppConfig::instance().getValue("MainWindow/PresetLeftPanel", "favorite").toString();
        applyPresetLayout(presetLeft);
    });

    connect(m_btnResetLayout, &QPushButton::clicked, this, [this]() {
        m_activeStatusBtn = m_btnResetLayout;
        if (m_panelLayoutManager) {
            m_panelLayoutManager->resetSplitterLayout();
        }
    });
=======
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

    connect(m_btnContentPanel, &QPushButton::clicked, this, [this]() {
        if (m_panelLayoutManager) {
            m_panelLayoutManager->toggleImmersiveMode();
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

    connect(m_btnPresetLayout, &QPushButton::clicked, this, [this]() {
        QString presetLeft = AppConfig::instance().getValue("MainWindow/PresetLeftPanel", "favorite").toString();
        applyPresetLayout(presetLeft);
    });

    connect(m_btnResetLayout, &QPushButton::clicked, this, [this]() {
        if (m_panelLayoutManager) {
            m_panelLayoutManager->resetSplitterLayout();
        }
    });
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    if (m_btnToggleFilter)   m_btnToggleFilter->setChecked(false);
    if (m_btnToggleMeta)     m_btnToggleMeta->setChecked(false);
    if (m_btnContentPanel)   m_btnContentPanel->setChecked(false);
    if (m_btnToggleFavorite) m_btnToggleFavorite->setChecked(false);
    if (m_btnToggleNav)      m_btnToggleNav->setChecked(false);
    if (m_btnPresetLayout)   m_btnPresetLayout->setChecked(false);
    if (m_btnResetLayout)    m_btnResetLayout->setChecked(false);

    if (isImm) {
        if (m_btnContentPanel) m_btnContentPanel->setChecked(true);
    } else if (navVis && favVis && metaVis && filterVis) {
        if (m_btnResetLayout) m_btnResetLayout->setChecked(true);
    } else if ((favVis || navVis) && !metaVis && filterVis && !(favVis && navVis)) {
        if (m_btnPresetLayout) m_btnPresetLayout->setChecked(true);
    } else if (m_activeStatusBtn) {
        m_activeStatusBtn->setChecked(true);
    }
=======
    if (m_btnToggleFilter)   m_btnToggleFilter->setChecked(filterVis);
    if (m_btnToggleMeta)     m_btnToggleMeta->setChecked(metaVis);
    if (m_btnToggleFavorite) m_btnToggleFavorite->setChecked(favVis);
    if (m_btnToggleNav)      m_btnToggleNav->setChecked(navVis);

    if (isImm) {
        if (m_btnContentPanel) m_btnContentPanel->setChecked(true);
        if (m_btnResetLayout) m_btnResetLayout->setChecked(false);
        if (m_btnPresetLayout) m_btnPresetLayout->setChecked(false);
    } else {
        if (m_btnContentPanel) m_btnContentPanel->setChecked(false);

        bool allDefault = navVis && favVis && metaVis && filterVis;
        if (m_btnResetLayout) m_btnResetLayout->setChecked(allDefault);

        bool isPreset3 = (favVis || navVis) && !metaVis && filterVis && !(favVis && navVis);
        if (m_btnPresetLayout) m_btnPresetLayout->setChecked(isPreset3);
    }
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `updateStatusBarButtonHighlights()` sets `setChecked(panelVis)` for each toggle button independently.
2. Ensure status bar control buttons display in the inverted sequence from left to right: Reset Layout, Preset Layout, Toggle Navigation, Toggle Favorites, Content Panel (Immersive), Toggle Metadata, Toggle Filter.
