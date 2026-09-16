# Implementation Plan - ContentPanel ViewMode Selection Synchronization

## 1. Overview
修补 `ContentPanel::setViewMode` 在网格、自适应、列表与分栏视图模式之间切换时，原视图中已选中的项目在新视图中丢失选中高亮状态的缺陷。
在 `setViewMode` 执行模式切换前，提取当前活动视图的选中路径集合 `selectedPaths`；在新视图激活后，将选区同步恢复至新视图的 `selectionModel()` 中，并高亮聚焦，确保全视图模式间切换时选择集无缝无损联动。

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### File: `src/ui/ContentPanel.cpp`
在 `ContentPanel::setViewMode` 中保存 `selectedPaths`，并在新视图切换完成后同步恢复选中高亮。

```
<<<<<<< SEARCH
    if (m_currentViewMode == mode) {
        return;
    }
    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            m_columnView->setRootPath(m_currentPath);
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridView);
    }

    // 🚀【自愈数据同步机制】：若从分栏视图切回网格/列表/瀑布流视图，且主模型处于空装载状态，自动自愈驱动 loadDirectory
    if (oldMode == ColumnView && mode != ColumnView) {
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
=======
    if (m_currentViewMode == mode) {
        return;
    }
    QStringList savedSelectedPaths = getSelectedPaths();
    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            m_columnView->setRootPath(m_currentPath);
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridView);
    }

    // 🚀【自愈数据同步机制】：若从分栏视图切回网格/列表/瀑布流视图，且主模型处于空装载状态，自动自愈驱动 loadDirectory
    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_diskModel || m_diskModel->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }

    // 🚀【视图切换选区无损同步】：在新激活的视图中同步恢复之前的选中高亮与聚焦位置
    if (!savedSelectedPaths.isEmpty()) {
        for (const QString& selPath : savedSelectedPaths) {
            selectAndScrollToPath(selPath);
        }
    }

    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    updateGridSize();
    emit viewModeChanged(mode);
    emit zoomLevelChanged(m_zoomLevel);

    if (m_visibleTimer) m_visibleTimer->start();
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
```bash
# 1. 配置并构建 CMake 项目
cmake -B build -S .
cmake --build build --config Release

# 2. 验证路径与测试
# - 在网格/自适应视图模式下，选中某个文件或文件夹
# - 切换视图模式至列表视图或分栏视图
# - 校验新视图中原选中项是否保持高亮选中状态且滚动聚焦可见
```

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT 入口复用**：已复用既有的 `getSelectedPaths()` 与 `selectAndScrollToPath(...)` 入口完成选区提取与还原。
- **无重复实现**：统一在 `setViewMode` 调度中心完成切换同步。

## 6. Header API Signature Verification
| 类名 / 模块名 | 调用的成员/数据角色 | 物理头文件签名 |
| :--- | :--- | :--- |
| `ContentPanel` | `getSelectedPaths` | `QStringList getSelectedPaths() const;` |
| `ContentPanel` | `selectAndScrollToPath` | `void selectAndScrollToPath(const QString& path);` |
