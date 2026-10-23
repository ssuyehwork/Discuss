# MainWindow Implementation Plan

## Overview
This plan enforces the "7-select-1 exclusive single highlight" rule for status bar control buttons in `MainWindow` (`src/ui/MainWindow.cpp` & `src/ui/MainWindow.h`):
1. **7-Select-1 Exclusive Highlight Rule**: At any time, exactly ONE status bar button displays the checked/highlighted state. When "Reset Layout" (all panels shown) is active, only the Reset button is highlighted; individual panel buttons are kept unchecked/dark.
2. **Active Button Tracking**: Track `m_activeStatusBtn` upon button clicks so that in custom layout states, only the last interacted button is highlighted.
3. **Inverted Layout & Icon Size**: Preserve icon size `26x20`, inverted button layout order, and `ToolTipOverlay` single popup behavior.

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
    QPushButton* m_activeStatusBtn = nullptr;
>>>>>>> REPLACE
```

### 2. `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
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
=======
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
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    if (m_btnToggleFilter) m_btnToggleFilter->setChecked(filterVis);
    if (m_btnToggleMeta) m_btnToggleMeta->setChecked(metaVis);
    if (m_btnToggleFavorite) m_btnToggleFavorite->setChecked(favVis);
    if (m_btnToggleNav) m_btnToggleNav->setChecked(navVis);

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
=======
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
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `updateStatusBarButtonHighlights()` unchecks all status buttons before setting at most one checked.
2. Confirm that when all panels are visible (Reset state), only Reset Layout button is highlighted and all other buttons remain unhighlighted.
3. Confirm that clicking any single status bar button highlights ONLY that button.
