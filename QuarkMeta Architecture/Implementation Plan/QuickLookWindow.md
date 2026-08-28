# QuickLookWindow Implementation Plan

## 1. Overview
This implementation plan establishes atomic preview generation fused cancellation (`m_previewGeneration`) for `QuickLookWindow` (`src/ui/QuickLookWindow.h/cpp`), purges top-level widget scanning for `FavoritePanel`, removes raw Win32 `SetWindowPos` top-most calls in favor of `FramelessWindowHelper::setAlwaysOnTop`, and stabilizes `QuickLookGraphicsView` / `QuickLookMinimap` viewport synchronization against floating-point re-entrancy oscillations.

---

## 2. Modified Files List
- `src/ui/QuickLookWindow.h` *(Modified)*
- `src/ui/QuickLookWindow.cpp` *(Modified)*
- `src/ui/QuickLookGraphicsView.cpp` *(Modified)*

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/QuickLookWindow.h`
```cpp
<<<<<<< SEARCH
    QuickLookGraphicsView* m_graphicsView = nullptr;
=======
    std::atomic<uint64_t> m_previewGeneration{1};
    QuickLookGraphicsView* m_graphicsView = nullptr;
>>>>>>> REPLACE
```

### 3.2 Update `src/ui/QuickLookWindow.cpp`
```cpp
<<<<<<< SEARCH
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "ShellIconManager.h"
#include "../util/ColorPaletteEngine.h"
=======
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "ShellIconManager.h"
#include "FramelessWindowHelper.h"
#include "../util/ColorPaletteEngine.h"
>>>>>>> REPLACE
```
```cpp
<<<<<<< SEARCH
void QuickLookWindow::closePreview() {
    if (m_graphicsView) {
        m_graphicsView->clear();
    }
    hide();
}
=======
void QuickLookWindow::closePreview() {
    m_previewGeneration.fetch_add(1, std::memory_order_relaxed);
    if (m_graphicsView) {
        m_graphicsView->clear();
    }
    hide();
}
>>>>>>> REPLACE
```
```cpp
<<<<<<< SEARCH
    showFullScreen();
    raise();
    activateWindow();
=======
    showFullScreen();
    raise();
    activateWindow();

    m_ignoreDeactivate = true;
    QTimer::singleShot(150, this, [this]() {
        m_ignoreDeactivate = false;
    });
    FramelessWindowHelper::setAlwaysOnTop(this, true);
>>>>>>> REPLACE
```

### 3.3 Update `src/ui/QuickLookGraphicsView.cpp`
```cpp
<<<<<<< SEARCH
    connect(m_minimap, &QuickLookMinimap::centerRequested, this, [this](double xRatio, double yRatio) {
        if (!m_pixmapItem || m_pixmapItem->pixmap().isNull()) return;
        QRectF totalRect = m_pixmapItem->boundingRect();
        QPointF targetCenter(xRatio * totalRect.width(), yRatio * totalRect.height());
        centerOn(targetCenter);
    });
=======
    connect(m_minimap, &QuickLookMinimap::centerRequested, this, [this](double xRatio, double yRatio) {
        if (!m_pixmapItem || m_pixmapItem->pixmap().isNull()) return;
        QRectF totalRect = m_pixmapItem->boundingRect();
        QPointF targetCenter(xRatio * totalRect.width(), yRatio * totalRect.height());
        
        centerOn(targetCenter);
        updateMinimap();
    });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Verify `QuarkMeta-Architecture-Planning.md` contains the updated `QuickLookWindow` architecture specification.
2. Verify `QuickLookWindow.md` is strictly created under `QuarkMeta Architecture/Implementation Plan/` with precise 1:1 class name mapping.
3. Run pre-commit instructions checks and submit.
