# Implementation Plan - PanelLayoutManager (Monochrome SVG Icons)

## 1. Overview
This implementation plan specifies the changes required to equip panel toggle menu items in `PanelLayoutManager.cpp` with neutral monochrome (`#EEEEEE`) SVG icons.

---

## 2. Modified Files List
- `src/ui/PanelLayoutManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Neutral Monochrome Icons in Panel Layout Menu (`src/ui/PanelLayoutManager.cpp`)

```
<<<<<<< SEARCH
    addToggleAction("显示目录导航", "nav", m_navPanel);
    addToggleAction("显示收藏夹", "favorite", m_favoritePanel);
    addToggleAction("显示内容区", "content", m_contentPanel, false);
    addToggleAction("显示元数据栏", "meta", m_metaPanel);
    addToggleAction("显示筛选栏", "filter", m_filterPanel);

    menu->addSeparator();
    QAction* resetAct = menu->addAction("重置分栏");
=======
    addToggleAction("显示目录导航", "nav", m_navPanel);
    addToggleAction("显示收藏夹", "favorite", m_favoritePanel);
    addToggleAction("显示内容区", "content", m_contentPanel, false);
    addToggleAction("显示元数据栏", "meta", m_metaPanel);
    addToggleAction("显示筛选栏", "filter", m_filterPanel);

    menu->addSeparator();
    QAction* resetAct = menu->addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "重置分栏");
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Visual Verification**:
   - Trigger the panel layout context menu.
   - Verify that all panel toggle and reset actions display neutral monochrome (`#EEEEEE`) SVG icons.
