# PanelLayoutManager Panel Visibility Persistence Implementation Plan

## 1. Overview
When clicking panel layout buttons in the status bar (e.g. "Hide Filter", "Hide Meta", "Hide Nav", "Hide Favorite"), `PanelLayoutManager::saveLayoutState()` writes `MainWindow/NavVisible`, `MainWindow/FavoriteVisible`, `MainWindow/MetaVisible`, and `MainWindow/FilterVisible` keys to `AppConfig`. However, `PanelLayoutManager::initLayout()` failed to read these persisted flags on application startup when not in immersive mode, causing hidden panels to always reappear upon restarting the application.

This plan restores panel visibility state reading in `PanelLayoutManager::initLayout()` to achieve 100% persistent panel layout state across restarts.

## 2. Modified Files List
- `src/ui/PanelLayoutManager.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/PanelLayoutManager.cpp`

```diff
<<<<<<< SEARCH
    bool isImmersive = AppConfig::instance().getValue("MainWindow/IsImmersiveMode", false).toBool();
    if (isImmersive) {
        if (m_navPanel) m_navPanel->setVisible(false);
        if (m_favoritePanel) m_favoritePanel->setVisible(false);
        if (m_metaPanel) m_metaPanel->setVisible(false);
        if (m_filterPanel) m_filterPanel->setVisible(false);
        emit panelVisibilityChanged("nav", false);
        emit panelVisibilityChanged("favorite", false);
        emit panelVisibilityChanged("meta", false);
        emit panelVisibilityChanged("filter", false);
    }

    // 同步恢复分栏尺寸，杜绝异步 singleShot(0) 造成的二次排版抽搐
=======
    bool isImmersive = AppConfig::instance().getValue("MainWindow/IsImmersiveMode", false).toBool();
    if (isImmersive) {
        if (m_navPanel) m_navPanel->setVisible(false);
        if (m_favoritePanel) m_favoritePanel->setVisible(false);
        if (m_metaPanel) m_metaPanel->setVisible(false);
        if (m_filterPanel) m_filterPanel->setVisible(false);
        emit panelVisibilityChanged("nav", false);
        emit panelVisibilityChanged("favorite", false);
        emit panelVisibilityChanged("meta", false);
        emit panelVisibilityChanged("filter", false);
    } else {
        bool navVis = AppConfig::instance().getValue("MainWindow/NavVisible", true).toBool();
        bool favVis = AppConfig::instance().getValue("MainWindow/FavoriteVisible", true).toBool();
        bool metaVis = AppConfig::instance().getValue("MainWindow/MetaVisible", true).toBool();
        bool filterVis = AppConfig::instance().getValue("MainWindow/FilterVisible", true).toBool();

        if (m_navPanel) m_navPanel->setVisible(navVis);
        if (m_favoritePanel) m_favoritePanel->setVisible(favVis);
        if (m_metaPanel) m_metaPanel->setVisible(metaVis);
        if (m_filterPanel) m_filterPanel->setVisible(filterVis);

        emit panelVisibilityChanged("nav", navVis);
        emit panelVisibilityChanged("favorite", favVis);
        emit panelVisibilityChanged("meta", metaVis);
        emit panelVisibilityChanged("filter", filterVis);
    }

    // 同步恢复分栏尺寸，杜绝异步 singleShot(0) 造成的二次排版抽搐
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Compile the application using CMake / MSVC build pipeline.
2. Launch QuarkMeta, click a layout button in the status bar (e.g. "隐藏筛选器" / "隐藏目录导航").
3. Close QuarkMeta and relaunch the application.
4. Verify that the hidden panels remain hidden and that the corresponding status bar buttons retain their checked/highlighted state.
