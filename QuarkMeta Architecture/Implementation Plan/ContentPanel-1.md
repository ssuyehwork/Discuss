# ContentPanel Implementation Plan (ContentPanel-1.md)

## Overview
本方案旨在彻底解决从分栏视图（`ViewModeColumn`）切换回网格视图（`GridView`）、列表视图（`ListView`）或瀑布流视图（`JustifiedView`）时，因主数据模型 `m_diskModel` 未能即时装载当前路径数据而导致内容区域错误显示“没有可显示的项目”的脱节问题。

## Modified Files List
1. `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/ContentPanel.cpp`
在 `setViewMode` 切换视图模式时，增加对主模型 `m_diskModel` 实际装载状态与 `m_currentPath` 的权威校验。若发现前一视图模式为分栏视图且主模型处于未装载或数据为空状态，则自动触发 `loadDirectory(m_currentPath, m_isRecursive)` 进行自愈重载：

```diff
<<<<<<< SEARCH
void ContentPanel::setViewMode(ViewMode mode) {
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else if (mode == ViewModeColumn) {
        if (m_columnView) {
            m_columnView->setRootPath(m_currentPath);
        }
        m_viewStack->setCurrentWidget(m_columnView);
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridView);
    }

    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    updateGridSize();
    emit viewModeChanged(mode);
    emit zoomLevelChanged(m_zoomLevel);

    if (m_visibleTimer) m_visibleTimer->start();
}
=======
void ContentPanel::setViewMode(ViewMode mode) {
    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else if (mode == ViewModeColumn) {
        if (m_columnView) {
            m_columnView->setRootPath(m_currentPath);
        }
        m_viewStack->setCurrentWidget(m_columnView);
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridView);
    }

    // 🚀【自愈数据同步机制】：若从分栏视图切回网格/列表/瀑布流视图，且主模型处于空装载状态，自动自愈驱动 loadDirectory
    if (oldMode == ViewModeColumn && mode != ViewModeColumn) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_diskModel || m_diskModel->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }

    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    updateGridSize();
    emit viewModeChanged(mode);
    emit zoomLevelChanged(m_zoomLevel);

    if (m_visibleTimer) m_visibleTimer->start();
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. 编译项目：`cmake --build build`
2. 启动应用，首先切换至“分栏视图（列视图）”模式；
3. 点击“收藏夹”或“目录导航”中的任意非空文件夹（例如 `G:\C++\QuarkMeta\QuarkMeta\Z 作废`）；
4. 确认分栏视图顺利级叠展开并显示该文件夹里的所有子文件/子文件夹；
5. 点击右下角视图切换栏中的“网格视图”或“列表视图”按钮；
6. 验证：切换后，内容区域**立即正确刷出**该文件夹内的真实数据项目，绝对不再出现“没有可显示的项目”虚假提示！
