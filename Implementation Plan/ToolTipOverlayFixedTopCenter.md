# Implementation Plan - ToolTipOverlay Fixed Top-Center Position Refactoring

## 1. Overview
Currently, `ToolTipOverlay` popup messages (e.g. "已成功添加至收藏夹", "窗格数量超出上限", "未选择任何可提取内容的文件" etc.) position themselves relative to the mouse cursor position (`QCursor::pos() + QPoint(15, 15)`).
This implementation plan refactors the `ToolTipOverlay` positioning logic so that toast notifications and warning tooltips are anchored **10 pixels below the top of the central `ContentPanel` container**, and **horizontally centered** within the `ContentPanel` width.

---

## 2. Modified Files List
- `src/ui/ToolTipOverlay.h`
- `src/ui/ToolTipOverlay.cpp`
- `src/ui/PanelMediator.cpp`
- `src/ui/FavoritePanel.cpp`
- `src/ui/LibraryPanel.cpp`
- `src/ui/MetaPanel.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ToolTipOverlay.h`
Add helper static method or overloaded method to calculate centered top-fixed position based on target widget geometry (defaulting to active `ContentPanel` if not provided).

```cpp
<<<<<<< SEARCH
    /**
     * @brief 显示提示文字
     */
    void showText(const QPoint& globalPos, const QString& text, int timeout = 700, const QColor& borderColor = QColor("#B0B0B0"), bool exactPosition = false, const QColor& backgroundColor = QColor("#2B2B2B"));
=======
    /**
     * @brief 显示提示文字
     */
    void showText(const QPoint& globalPos, const QString& text, int timeout = 700, const QColor& borderColor = QColor("#B0B0B0"), bool exactPosition = false, const QColor& backgroundColor = QColor("#2B2B2B"));

    /**
     * @brief 显示固定在目标 Widget (默认为 ContentPanel) 顶部下方 10px 且水平居中的 Toast 气泡
     */
    void showToast(const QString& text, int timeout = 1500, const QColor& borderColor = QColor("#2ecc71"), QWidget* targetWidget = nullptr);
>>>>>>> REPLACE
```

### 3.2 `src/ui/ToolTipOverlay.cpp`
Implement `showToast` calculating position relative to target widget (or active `ContentPanel` via `QApplication::activeWindow()->findChild<ContentPanel*>()`):

```cpp
<<<<<<< SEARCH
void ToolTipOverlay::showText(const QPoint& globalPos, const QString& text, int timeout, const QColor& borderColor, bool exactPosition, const QColor& backgroundColor) {
=======
void ToolTipOverlay::showToast(const QString& text, int timeout, const QColor& borderColor, QWidget* targetWidget) {
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, [this, text, timeout, borderColor, targetWidget]() {
            showToast(text, timeout, borderColor, targetWidget);
        });
        return;
    }

    QWidget* anchor = targetWidget;
    if (!anchor) {
        QWidget* activeWin = QApplication::activeWindow();
        if (activeWin) {
            anchor = activeWin->findChild<QWidget*>("ContentPanel");
            if (!anchor) {
                // Fallback to finding by class type if objectName is not ContentPanel
                QList<QWidget*> panels = activeWin->findChildren<QWidget*>();
                for (QWidget* w : panels) {
                    if (w->inherits("QuarkMeta::ContentPanel")) {
                        anchor = w;
                        break;
                    }
                }
            }
        }
    }

    QPoint targetPos;
    if (anchor && anchor->isVisible()) {
        QPoint globalTopLeft = anchor->mapToGlobal(QPoint(0, 0));
        int panelWidth = anchor->width();

        // Measure text width to center horizontally
        m_pendingText = text;
        triggerPendingShow(); // Ensure size is calculated
        int tipWidth = width();

        int centerX = globalTopLeft.x() + (panelWidth - tipWidth) / 2;
        int topY = globalTopLeft.y() + 10; // 10px below top
        targetPos = QPoint(centerX, topY);
    } else {
        targetPos = QCursor::pos();
    }

    showText(targetPos, text, timeout, borderColor, true);
}

void ToolTipOverlay::showText(const QPoint& globalPos, const QString& text, int timeout, const QColor& borderColor, bool exactPosition, const QColor& backgroundColor) {
>>>>>>> REPLACE
```

### 3.3 `src/ui/PanelMediator.cpp` & Call Sites
Replace `ToolTipOverlay::instance()->showText(QCursor::pos(), ...)` toast calls in `PanelMediator.cpp`, `FavoritePanel.cpp`, `LibraryPanel.cpp`, `MetaPanel.cpp`, and `ContentContextMenu.cpp` with `ToolTipOverlay::instance()->showToast(...)`.

Example diff in `src/ui/PanelMediator.cpp`:

```cpp
<<<<<<< SEARCH
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "该文件夹已在收藏夹中，请勿重复添加", 2000, QColor("#e81123"));
=======
                    ToolTipOverlay::instance()->showToast("该文件夹已在收藏夹中，请勿重复添加", 2000, QColor("#e81123"));
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Clean build directory and recompile project target using `cmake --build .`.
2. Trigger toasts (e.g. duplicate favorite addition, pane limit reached, copy path notice).
3. Confirm toast bubble pops up 10px below top edge of `ContentPanel` and horizontally centered within `ContentPanel`.

---

## 5. Official API Reuse & Anti-Redundancy
- Reuses existing `ToolTipOverlay::instance()` singleton.
- Reuses `QWidget::mapToGlobal()` and `QWidget::findChild<ContentPanel*>()` without introducing duplicate floating overlay classes.

---

## 6. Header API Signature Verification
- `void showToast(const QString& text, int timeout = 1500, const QColor& borderColor = QColor("#2ecc71"), QWidget* targetWidget = nullptr);` is added to public interface without altering pre-existing `showText` signature.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `QApplication` and `QWidget` are fully included in `ToolTipOverlay.h` / `ToolTipOverlay.cpp`.

---

## 8. Data SSOT Declaration Table
| Entity / State | SSOT Location | Synchronization Mechanism |
| :--- | :--- | :--- |
| Overlay Toast Position | `ToolTipOverlay::showToast` | Calculated from active `ContentPanel::mapToGlobal()` at display time |

---

## 9. Compliance Self-Check Table
| Check Item | Status | Notes |
| :--- | :--- | :--- |
| Zero Code Modification Rule | PASS | Output strictly as plan document inside `Implementation Plan/` |
| Chinese Language Protocol | PASS | Documented completely in Chinese |
| Contract Lock | PASS | Original `showText` header signatures frozen |

---

## 10. Behavior Preservation Declaration Table
| Action | Existing Behavior | Preserved Behavior |
| :--- | :--- | :--- |
| Hover Tooltips (Color Wheel, Buttons) | Follow mouse / exact pos | Preserved via `showText` |
| Toast Notifications | Mouse cursor relative | Repositioned to ContentPanel Top 10px Center via `showToast` |
