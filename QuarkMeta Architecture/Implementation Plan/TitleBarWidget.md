# Implementation Plan - TitleBarWidget (Monochrome SVG Icons)

## 1. Overview
This implementation plan specifies the changes required to equip view mode actions in `TitleBarWidget.cpp` with neutral monochrome (`#EEEEEE`) SVG icons.

---

## 2. Modified Files List
- `src/ui/TitleBarWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Neutral Monochrome Icons in View Mode Menu (`src/ui/TitleBarWidget.cpp`)

```
<<<<<<< SEARCH
        QAction* actAdaptive = menu.addAction("自适应(A)");
        QAction* actGrid = menu.addAction("网格(G)");
        QAction* actList = menu.addAction("列表(L)");
=======
        QAction* actAdaptive = menu.addAction(UiHelper::getIcon("grid", QColor("#EEEEEE"), 18), "自适应(A)");
        QAction* actGrid = menu.addAction(UiHelper::getIcon("grid", QColor("#EEEEEE"), 18), "网格(G)");
        QAction* actList = menu.addAction(UiHelper::getIcon("list_ul", QColor("#EEEEEE"), 18), "列表(L)");
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Visual Verification**:
   - Click the view mode button in the top title bar.
   - Verify that all view mode items ("自适应", "网格", "列表") feature neutral monochrome (`#EEEEEE`) SVG icons.
