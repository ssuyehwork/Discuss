# ContentPaneSplitManager Implementation Plan - Drag Title Bar to Toggle Split Orientation

This implementation plan adds support for toggling multi-pane split orientation (Horizontal ↔ Vertical) by dragging the Content Panel header title bar:
- Dragging downward on the header bar in horizontal split mode (beyond `startDragDistance()`) switches the split orientation to Vertical.
- Dragging rightward on the header bar in vertical split mode (beyond `startDragDistance()`) switches the split orientation to Horizontal.
- Blue preview overlays indicate the target layout orientation during dragging.
- Height checks ensure vertical split mode is only applied when the container height can accommodate `paneCount * kMinPaneHeight`.

---

## 1. Overview
When operating in multi-pane split view, users need an intuitive, gesture-based mechanism to toggle between horizontal split (side-by-side columns) and vertical split (stacked rows).

Implementation details:
1. `ContentPanel.h` introduces `static constexpr int kMinPaneHeight = 230;` as the single source of truth for minimum pane height in vertical split mode.
2. `ContentHeaderWidget` detects mouse drag on the header bar (ignoring clicks on tool buttons) when in split mode and emits orientation drag signals (`orientationDragStarted`, `orientationDragUpdated`, `orientationDragEnded`, `orientationToggleRequested`).
3. `ContentPaneSplitManager` exposes `setSplitOrientation(Qt::Orientation target)`:
   - Evaluates whether target is different from `m_splitOrientation`.
   - Delegates from secondary pane to root pane.
   - Verifies container height when switching to `Qt::Vertical`: if total height < `paneCount() * kMinPaneHeight`, displays a `ToolTipOverlay` error ("内容区高度不足，无法垂直排列") and cancels the switch.
   - Updates `m_splitOrientation` and `m_paneSplitter->setOrientation(target)`.
   - Updates container minimum constraints (width constraint for Horizontal, height constraint for Vertical).
   - Accurately deducts `handleWidth * (count - 1)` and calls `redistributePaneSizes()`.
   - Notifies layout changes via debounced `notifyLayoutChanged()`.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/ContentHeaderWidget.h`
- `src/ui/ContentHeaderWidget.cpp`
- `src/ui/controllers/ContentPaneSplitManager.h`
- `src/ui/controllers/ContentPaneSplitManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.h`

```
<<<<<<< SEARCH
    static constexpr int kMinPaneWidth = 230;
    static constexpr int kMaxPanes = 4;
=======
    static constexpr int kMinPaneWidth = 230;
    static constexpr int kMinPaneHeight = 230;
    static constexpr int kMaxPanes = 4;
>>>>>>> REPLACE
```

### 3.2 `src/ui/ContentHeaderWidget.h`

```
<<<<<<< SEARCH
signals:
    void filterStateChanged(const FilterState& state);
    void recursiveToggled(bool recursive);
    void splitViewRequested();
=======
signals:
    void filterStateChanged(const FilterState& state);
    void recursiveToggled(bool recursive);
    void splitViewRequested();
    void orientationToggleRequested(Qt::Orientation target);
    void orientationDragStarted(Qt::Orientation target);
    void orientationDragUpdated(const QPoint& globalPos);
    void orientationDragEnded(bool apply);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
=======
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    FilterState m_filterState;
};
=======
    FilterState m_filterState;
    QPoint m_dragStartPos;
    bool m_isDraggingHeader = false;
};
>>>>>>> REPLACE
```

### 3.3 `src/ui/ContentHeaderWidget.cpp`

```
<<<<<<< SEARCH
#include <QEvent>
#include <QCursor>
#include <QStyle>
=======
#include <QEvent>
#include <QCursor>
#include <QStyle>
#include <QMouseEvent>
#include <QApplication>
#include "ContentPanel.h"
#include "controllers/ContentPaneSplitManager.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool ContentHeaderWidget::eventFilter(QObject* watched, QEvent* event) {
=======
void ContentHeaderWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QWidget* child = childAt(event->pos());
        if (!qobject_cast<QPushButton*>(child)) {
            ContentPanel* panel = qobject_cast<ContentPanel*>(parentWidget());
            if (!panel) panel = qobject_cast<ContentPanel*>(parentWidget()->parentWidget());
            if (panel && panel->isSplitMode()) {
                m_dragStartPos = event->pos();
                m_isDraggingHeader = false;
                event->accept();
                return;
            }
        }
    }
    QWidget::mousePressEvent(event);
}

void ContentHeaderWidget::mouseMoveEvent(QMouseEvent* event) {
    if ((event->buttons() & Qt::LeftButton) && !m_dragStartPos.isNull()) {
        if ((event->pos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
            ContentPanel* panel = qobject_cast<ContentPanel*>(parentWidget());
            if (!panel) panel = qobject_cast<ContentPanel*>(parentWidget()->parentWidget());
            if (panel && panel->isSplitMode()) {
                Qt::Orientation currentOri = panel->splitManager()->exportSplitState().orientation;
                QPoint delta = event->pos() - m_dragStartPos;
                Qt::Orientation targetOri = currentOri;
                if (currentOri == Qt::Horizontal && delta.y() > QApplication::startDragDistance()) {
                    targetOri = Qt::Vertical;
                } else if (currentOri == Qt::Vertical && delta.x() > QApplication::startDragDistance()) {
                    targetOri = Qt::Horizontal;
                }

                if (!m_isDraggingHeader) {
                    m_isDraggingHeader = true;
                    emit orientationDragStarted(targetOri);
                }
                emit orientationDragUpdated(event->globalPosition().toPoint());
                event->accept();
                return;
            }
        }
    }
    QWidget::mouseMoveEvent(event);
}

void ContentHeaderWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_isDraggingHeader) {
            ContentPanel* panel = qobject_cast<ContentPanel*>(parentWidget());
            if (!panel) panel = qobject_cast<ContentPanel*>(parentWidget()->parentWidget());
            if (panel && panel->isSplitMode()) {
                Qt::Orientation currentOri = panel->splitManager()->exportSplitState().orientation;
                QPoint delta = event->pos() - m_dragStartPos;
                bool shouldToggle = false;
                Qt::Orientation targetOri = currentOri;

                if (currentOri == Qt::Horizontal && delta.y() > QApplication::startDragDistance()) {
                    shouldToggle = true;
                    targetOri = Qt::Vertical;
                } else if (currentOri == Qt::Vertical && delta.x() > QApplication::startDragDistance()) {
                    shouldToggle = true;
                    targetOri = Qt::Horizontal;
                }

                emit orientationDragEnded(shouldToggle);
                if (shouldToggle) {
                    emit orientationToggleRequested(targetOri);
                }
            }
            m_isDraggingHeader = false;
            m_dragStartPos = QPoint();
            event->accept();
            return;
        }
        m_dragStartPos = QPoint();
    }
    QWidget::mouseReleaseEvent(event);
}

bool ContentHeaderWidget::eventFilter(QObject* watched, QEvent* event) {
>>>>>>> REPLACE
```

### 3.4 `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    m_headerWidget = new ContentHeaderWidget(this);
    m_headerWidget->setFilterState(m_currentFilter);

    connect(m_headerWidget, &ContentHeaderWidget::splitViewRequested, this, [this]() {
        if (!m_splitManager) return;
        m_splitManager->splitPane(Qt::Horizontal);
    });
=======
    m_headerWidget = new ContentHeaderWidget(this);
    m_headerWidget->setFilterState(m_currentFilter);

    connect(m_headerWidget, &ContentHeaderWidget::splitViewRequested, this, [this]() {
        if (!m_splitManager) return;
        m_splitManager->splitPane(Qt::Horizontal);
    });

    connect(m_headerWidget, &ContentHeaderWidget::orientationToggleRequested, this, [this](Qt::Orientation target) {
        if (!m_splitManager) return;
        m_splitManager->setSplitOrientation(target);
    });

    connect(m_headerWidget, &ContentHeaderWidget::orientationDragStarted, this, [this](Qt::Orientation target) {
        if (!m_splitManager) return;
        m_splitManager->updateOrientationPreviewOverlay(target);
    });

    connect(m_headerWidget, &ContentHeaderWidget::orientationDragEnded, this, [this](bool apply) {
        Q_UNUSED(apply);
        if (!m_splitManager) return;
        m_splitManager->hideOrientationPreviewOverlay();
    });
>>>>>>> REPLACE
```

### 3.5 `src/ui/controllers/ContentPaneSplitManager.h`

```
<<<<<<< SEARCH
    void restoreSplitState(const struct TabSplitState& state);
    void updateDragOverlay(const QPoint& pos);
    void hideDragOverlay();
    void updateContainerMinimumWidth();
=======
    void setSplitOrientation(Qt::Orientation target);
    void restoreSplitState(const struct TabSplitState& state);
    void updateDragOverlay(const QPoint& pos);
    void hideDragOverlay();
    void updateOrientationPreviewOverlay(Qt::Orientation target);
    void hideOrientationPreviewOverlay();
    void updateContainerMinimumWidth();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QWidget* m_dragOverlayWidget = nullptr;
    Qt::Orientation m_splitOrientation = Qt::Horizontal;
=======
    QWidget* m_dragOverlayWidget = nullptr;
    QWidget* m_orientationPreviewWidget = nullptr;
    Qt::Orientation m_splitOrientation = Qt::Horizontal;
>>>>>>> REPLACE
```

### 3.6 `src/ui/controllers/ContentPaneSplitManager.cpp`

```
<<<<<<< SEARCH
#include "../ContentHeaderWidget.h"
#include "../TabBarWidget.h"
#include <QHBoxLayout>
=======
#include "../ContentHeaderWidget.h"
#include "../TabBarWidget.h"
#include "ToolTipOverlay.h"
#include <QHBoxLayout>
#include <QCursor>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPaneSplitManager::updateContainerMinimumWidth() {
    if (m_primaryPaneContainer) {
        m_primaryPaneContainer->setMinimumWidth(ContentPanel::kMinPaneWidth);
    }
    for (QWidget* container : m_paneContainers) {
        if (container) {
            container->setMinimumWidth(ContentPanel::kMinPaneWidth);
        }
    }
}
=======
void ContentPaneSplitManager::updateContainerMinimumWidth() {
    bool isVert = (m_splitOrientation == Qt::Vertical);
    int minW = isVert ? 0 : ContentPanel::kMinPaneWidth;
    int minH = isVert ? ContentPanel::kMinPaneHeight : 0;

    if (m_primaryPaneContainer) {
        m_primaryPaneContainer->setMinimumWidth(minW);
        m_primaryPaneContainer->setMinimumHeight(minH);
    }
    for (QWidget* container : m_paneContainers) {
        if (container) {
            container->setMinimumWidth(minW);
            container->setMinimumHeight(minH);
        }
    }
}

void ContentPaneSplitManager::setSplitOrientation(Qt::Orientation target) {
    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->setSplitOrientation(target);
        return;
    }

    if (m_splitOrientation == target) return;

    int count = paneCount();
    if (target == Qt::Vertical && m_paneSplitter) {
        int availH = m_paneSplitter->height();
        int reqH = count * ContentPanel::kMinPaneHeight;
        if (availH > 0 && availH < reqH) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), "内容区高度不足，无法垂直排列", 2000, QColor("#e81123"));
            return;
        }
    }

    m_splitOrientation = target;
    if (m_paneSplitter) {
        m_paneSplitter->setOrientation(target);
    }

    updateContainerMinimumWidth();
    redistributePaneSizes();
    notifyLayoutChanged();
}

void ContentPaneSplitManager::updateOrientationPreviewOverlay(Qt::Orientation target) {
    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->updateOrientationPreviewOverlay(target);
        return;
    }

    if (!m_paneSplitter) return;

    if (!m_orientationPreviewWidget) {
        m_orientationPreviewWidget = new QWidget(m_panel);
        m_orientationPreviewWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_orientationPreviewWidget->setStyleSheet("background-color: rgba(0, 122, 255, 0.25); border: 2px solid #007AFF;");
    }

    m_orientationPreviewWidget->setGeometry(m_paneSplitter->geometry());
    m_orientationPreviewWidget->show();
    m_orientationPreviewWidget->raise();
}

void ContentPaneSplitManager::hideOrientationPreviewOverlay() {
    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->hideOrientationPreviewOverlay();
        return;
    }

    if (m_orientationPreviewWidget) {
        m_orientationPreviewWidget->hide();
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPaneSplitManager::redistributePaneSizes() {
    if (!m_paneSplitter) return;
    int count = paneCount();
    if (count <= 1) return;
    int total = (m_splitOrientation == Qt::Horizontal) ? m_panel->width() : m_panel->height();
    int each = total / count;
    QList<int> sizes;
    for (int i = 0; i < count; ++i) {
        sizes << each;
    }
    m_paneSplitter->setSizes(sizes);
}
=======
void ContentPaneSplitManager::redistributePaneSizes() {
    if (!m_paneSplitter) return;
    int count = paneCount();
    if (count <= 1) return;
    int handleW = m_paneSplitter->handleWidth();
    int total = (m_splitOrientation == Qt::Horizontal)
        ? (m_paneSplitter->width() - handleW * (count - 1))
        : (m_paneSplitter->height() - handleW * (count - 1));
    if (total <= 0) total = (m_splitOrientation == Qt::Horizontal) ? m_panel->width() : m_panel->height();
    int each = total / count;
    QList<int> sizes;
    for (int i = 0; i < count; ++i) {
        sizes << each;
    }
    m_paneSplitter->setSizes(sizes);
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
Since Qt6 dependencies are not available in the headless sandbox, compilation is skipped.
Functional verification checklist for native build:
1. Split view into 2, 3, or 4 panes in horizontal mode.
2. Drag down on the header title bar of any pane beyond `startDragDistance()`.
3. Verify blue preview overlay covers the splitter area during drag.
4. Release left mouse button. Verify all panes rearrange vertically from top to bottom while keeping order and focus intact.
5. Drag right on the header title bar in vertical split mode beyond `startDragDistance()`.
6. Release left mouse button. Verify panes rearrange back horizontally from left to right.
7. Verify tool buttons (split, toggle hidden, layers) click normally without triggering drag.
8. Switch tabs or restart app and verify split orientation is properly saved and restored in `TabSplitState`.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Orientation Control SSOT**: `ContentPaneSplitManager::setSplitOrientation` is the single source of truth for orientation mutations.
- **Pane Dimensions SSOT**: Uses `ContentPanel::kMinPaneWidth` (230) and `ContentPanel::kMinPaneHeight` (230).
- **Layout Event SSOT**: Reuses debounced `notifyLayoutChanged()`.

---

## 6. Header API Signature Verification
- `ContentPanel::kMinPaneHeight` -> `ContentPanel.h`
- `ContentHeaderWidget::orientationToggleRequested(Qt::Orientation target)` -> `ContentHeaderWidget.h`
- `ContentPaneSplitManager::setSplitOrientation(Qt::Orientation target)` -> `ContentPaneSplitManager.h`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "ToolTipOverlay.h"` and `#include <QCursor>` added to `ContentPaneSplitManager.cpp`.
- `#include "controllers/ContentPaneSplitManager.h"` added to `ContentHeaderWidget.cpp`.
- All types (`ContentPanel`, `ContentHeaderWidget`, `ContentPaneSplitManager`, `Qt::Orientation`, `QMouseEvent`) have complete type definitions.
