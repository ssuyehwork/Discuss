# Implementation Plan - ColumnViewWidget Minimum Container Padding & Focus Retention

This plan updates `ColumnViewWidget.cpp` so that clicking in the trailing blank canvas area preserves column selections and item highlights (preventing loss of focus/highlights), while ensuring the `QScrollArea` container always retains at least a 230px minimum trailing blank region.

## Overview
- **Problem**: 
  1. Clicking `ColumnBlankCanvasWidget` calls `clearAllSelections()`, clearing all selections across all columns and unhighlighting parent columns. Clicking outside in `QScrollArea` container does not clear selections.
  2. The trailing blank canvas area was constrained with `setFixedWidth(230)` instead of providing a minimum 230px canvas that expands with the container while preserving column highlights.
- **Solution**:
  1. Remove `clearAllSelections()` from `ColumnBlankCanvasWidget::mousePressEvent` so clicking trailing blank space preserves column selection and focus.
  2. Configure `ColumnBlankCanvasWidget` with `setMinimumWidth(230)` and `setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding)` with layout stretch factor `1` to fill remaining space while maintaining at least 230px container trailing padding.

---

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
        setObjectName("ColumnBlankCanvasWidget");
        setFixedWidth(230);
        setAcceptDrops(true);
=======
        setObjectName("ColumnBlankCanvasWidget");
        setMinimumWidth(230);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setAcceptDrops(true);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && m_columnView) {
            m_columnView->clearAllSelections();
        }
        QWidget::mousePressEvent(event);
    }
=======
    void mousePressEvent(QMouseEvent* event) override {
        // Do not clear column selections on click to preserve active column focus and parent highlights
        QWidget::mousePressEvent(event);
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_panes.append(pane);
    if (m_blankCanvasWidget) {
        m_layout->removeWidget(m_blankCanvasWidget);
    }
    m_layout->addWidget(pane);
    if (m_blankCanvasWidget) {
        m_layout->addWidget(m_blankCanvasWidget);
    }
=======
    m_panes.append(pane);
    if (m_blankCanvasWidget) {
        m_layout->removeWidget(m_blankCanvasWidget);
    }
    m_layout->addWidget(pane);
    if (m_blankCanvasWidget) {
        m_layout->addWidget(m_blankCanvasWidget, 1);
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **Verify Diff Precision**:
   Run the diff checker script:
   ```bash
   python3 /home/jules/self_created_tools/check_plan_diff.py "QuarkMeta Architecture/Implementation Plan/ColumnViewWidget-17.md"
   ```

2. **Build Verification**:
   Build the Qt application with CMake:
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --config Debug -j$(nproc)
   ```

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **`refreshAll()` / `loadDirectory()`**: Not altered; column loading remains unified through `ColumnViewPane::loadDirectory()`.
- **Focus & Selection Retention**: Removing `clearAllSelections()` directly on mouse press adheres to the principle that clicking trailing padding retains active selection and parent highlight states.

---

## Header API Signature Verification

- `ColumnBlankCanvasWidget::mousePressEvent(QMouseEvent* event)`: Override from `QWidget`.
- `ColumnViewWidget::clearAllSelections()`: Existing member function in `ColumnViewWidget.h`.
- `QWidget::setMinimumWidth(int)` / `QWidget::setSizePolicy(...)`: Standard Qt QWidget methods.
