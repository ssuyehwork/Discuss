# QuarkMeta 布局管理器最小宽度防挤压与 QSS 分割线补全实施方案 (PanelLayoutManager-1.md)

## 1. Overview
本方案旨在根治 2 个 UI 布局与样式物理缺陷：
1. **构造初始化阶段 `isVisible()` 误判归零**：在主窗口 `show()` 之前，Qt 原生机制使 `m_panel->isVisible()` 恒定返回 `false`，导致 5 栏全开时 `minimumWidth` 被错误计算并降级锁为 465px，引发面板暴力挤压乱码叠压。通过改用 `!isHidden()` 恢复真实逻辑，硬锁定 5 栏最小宽度为 1180px。
2. **QSS 选择器错配与 handle 隐形空气墙**：将 QSS 选择器中的 `#ListContainer` 修正为 `#FavoriteContainer`，并为 `QSplitter::handle` 增加 `border-left: 1px solid <BorderColor>` 细边线，恢复清晰的分栏边界。

---

## 2. Modified Files List
1. **修改** `src/ui/PanelLayoutManager.cpp`
2. **修改** `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/PanelLayoutManager.cpp` 纠正最小宽度算式

```cpp
<<<<<<< SEARCH
void PanelLayoutManager::updateDynamicMinimumSize() {
    if (!m_mainWindow) return;

    int visibleCount = 0;
    if (m_navPanel && m_navPanel->isVisible()) visibleCount++;
    if (m_favoritePanel && m_favoritePanel->isVisible()) visibleCount++;
    if (m_contentPanel && m_contentPanel->isVisible()) visibleCount++;
    if (m_metaPanel && m_metaPanel->isVisible()) visibleCount++;
    if (m_filterPanel && m_filterPanel->isVisible()) visibleCount++;

    if (visibleCount <= 0) visibleCount = 1;

    int calculatedMinW = (visibleCount * kBasePanelWidth) + ((visibleCount - 1) * kSplitterHandleWidth) + 10;
    int finalMinW = qMax(465, calculatedMinW);

    m_mainWindow->setMinimumWidth(finalMinW);
}
=======
void PanelLayoutManager::updateDynamicMinimumSize() {
    if (!m_mainWindow) return;

    int visibleCount = 0;
    if (m_navPanel && !m_navPanel->isHidden()) visibleCount++;
    if (m_favoritePanel && !m_favoritePanel->isHidden()) visibleCount++;
    if (m_contentPanel && !m_contentPanel->isHidden()) visibleCount++;
    if (m_metaPanel && !m_metaPanel->isHidden()) visibleCount++;
    if (m_filterPanel && !m_filterPanel->isHidden()) visibleCount++;

    if (visibleCount <= 0) visibleCount = 1;

    int calculatedMinW = (visibleCount * kBasePanelWidth) + ((visibleCount - 1) * kSplitterHandleWidth) + 10;
    int finalMinW = qMax(465, calculatedMinW);

    m_mainWindow->setMinimumWidth(finalMinW);
}
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/MainWindow.cpp` QSS 修正与 QSplitter handle 分割线注入

```cpp
<<<<<<< SEARCH
            #SidebarContainer, #ListContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
                background-color: %1; border: 1px solid %2; border-radius: 0px;
            }
=======
            #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
                background-color: %1; border: 1px solid %2; border-radius: 0px;
            }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; }"
        "QSplitter::handle { background-color: %1; width: 5px; }"
        "QSplitter::handle:hover { background-color: %2; }" 
    ).arg(qssColor(BackgroundDeep)).arg(qssColor(BackgroundHover)));
=======
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; }"
        "QSplitter::handle { background-color: %1; border-left: 1px solid %2; width: 5px; }"
        "QSplitter::handle:hover { background-color: %3; }"
    ).arg(qssColor(BackgroundDeep)).arg(qssColor(BorderColor)).arg(qssColor(BackgroundHover)));
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **最小宽度强力约束验证**：
   - 5 栏全开时，将主窗口向左暴力拉窄，验证窗口会在 1180px 处被硬性挡住，界面绝对无法重叠叠压。
2. **分栏视觉分隔线验证**：
   - 检查收藏夹外框边框，以及每个栏区右侧 1px 细分割线是否清晰展现。
