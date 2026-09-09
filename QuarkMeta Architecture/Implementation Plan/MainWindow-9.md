# MainWindow Implementation Plan

## Overview
This plan configures the status bar control buttons in `src/ui/MainWindow.cpp` per the user's re-defined semantics and sequence:
1. **Button Order (Left to Right)**:
   1. `m_btnToggleFilter` (隐藏筛选器.svg - far left)
   2. `m_btnToggleMeta` (隐藏元数据面板.svg)
   3. `m_btnContentPanel` (内容面板.svg)
   4. `m_btnToggleFavorite` (隐藏收藏栏.svg)
   5. `m_btnToggleNav` (隐藏目录导航.svg)
   6. `m_btnPresetLayout` (显示收藏栏+内容面板+筛选器.svg)
   7. `m_btnResetLayout` (重置分栏.svg - **positioned on the far right**)
2. **Button Actions**:
   - `m_btnToggleFilter`: Hides filter panel, displays all other 4 panels (nav, favorite, content, meta).
   - `m_btnToggleMeta`: Hides meta panel, displays all other 4 panels.
   - `m_btnContentPanel`: Hides all side panels, displays content panel only (immersive mode).
   - `m_btnToggleFavorite`: Hides favorite panel, displays all other 4 panels.
   - `m_btnToggleNav`: Hides nav panel, displays all other 4 panels.
   - `m_btnPresetLayout`: Applies 3-panel preset (favorite or nav + content + filter).
   - `m_btnResetLayout`: Displays all 5 panels (reset mode).
3. **1-to-1 Exclusive Button Highlighting**:
   - `updateStatusBarButtonHighlights()` evaluates the active panel layout configuration and highlights EXACTLY ONE corresponding status bar button. When Reset (all panels shown) is active, ONLY `m_btnResetLayout` (far right) is highlighted.

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
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", true);
        m_panelLayoutManager->setPanelVisible("favorite", true);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", true);
        m_panelLayoutManager->setPanelVisible("filter", false);
        updateStatusBarButtonHighlights();
    });

    connect(m_btnToggleMeta, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", true);
        m_panelLayoutManager->setPanelVisible("favorite", true);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", false);
        m_panelLayoutManager->setPanelVisible("filter", true);
        updateStatusBarButtonHighlights();
    });

    connect(m_btnContentPanel, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", false);
        m_panelLayoutManager->setPanelVisible("favorite", false);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", false);
        m_panelLayoutManager->setPanelVisible("filter", false);
        updateStatusBarButtonHighlights();
    });

    connect(m_btnToggleFavorite, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", true);
        m_panelLayoutManager->setPanelVisible("favorite", false);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", true);
        m_panelLayoutManager->setPanelVisible("filter", true);
        updateStatusBarButtonHighlights();
    });

    connect(m_btnToggleNav, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", false);
        m_panelLayoutManager->setPanelVisible("favorite", true);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", true);
        m_panelLayoutManager->setPanelVisible("filter", true);
        updateStatusBarButtonHighlights();
    });

    connect(m_btnPresetLayout, &QPushButton::clicked, this, [this]() {
        QString presetLeft = AppConfig::instance().getValue("MainWindow/PresetLeftPanel", "favorite").toString();
        applyPresetLayout(presetLeft);
    });

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

    if (isImm || (!navVis && !favVis && !metaVis && !filterVis)) {
        if (m_btnContentPanel) m_btnContentPanel->setChecked(true);
    } else if (navVis && favVis && metaVis && filterVis) {
        if (m_btnResetLayout) m_btnResetLayout->setChecked(true);
    } else if (navVis && favVis && metaVis && !filterVis) {
        if (m_btnToggleFilter) m_btnToggleFilter->setChecked(true);
    } else if (navVis && favVis && !metaVis && filterVis) {
        if (m_btnToggleMeta) m_btnToggleMeta->setChecked(true);
    } else if (navVis && !favVis && metaVis && filterVis) {
        if (m_btnToggleFavorite) m_btnToggleFavorite->setChecked(true);
    } else if (!navVis && favVis && metaVis && filterVis) {
        if (m_btnToggleNav) m_btnToggleNav->setChecked(true);
    } else if (((favVis && !navVis) || (navVis && !favVis)) && !metaVis && filterVis) {
        if (m_btnPresetLayout) m_btnPresetLayout->setChecked(true);
    }
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `src/ui/MainWindow.cpp` status bar button layout order: Filter Toggle (far left) to Reset Layout (far right).
2. Confirm button icon size is 26x20 px and tooltips trigger single `ToolTipOverlay` popups without native Qt tooltip duplication.
3. Confirm that when Reset (all panels shown) is active, ONLY the Reset Layout button on the far right is highlighted.
