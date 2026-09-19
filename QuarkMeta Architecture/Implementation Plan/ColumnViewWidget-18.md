# Implementation Plan - ColumnViewWidget Column Pane Blank Area Context Menu

This plan updates `ColumnViewPane` in `src/ui/ColumnViewWidget.cpp` so that right-clicking in the blank area of any column pane (`ColumnViewPane`, `m_canvasWidget`, or `m_paneScrollArea`) triggers the custom context menu for that column's current path.

## Overview
- **Problem**: 
  Right-clicking on file/folder list items in a column pane triggers `onCustomContextMenuRequested`, and right-clicking the trailing canvas `ColumnBlankCanvasWidget` triggers the context menu. However, right-clicking on the blank space underneath items inside a column pane (`ColumnViewPane` / `m_canvasWidget` / `m_paneScrollArea`) does not trigger the context menu because custom context menu policy and connections were not set on the pane widgets themselves.
- **Solution**:
  1. Set `setContextMenuPolicy(Qt::CustomContextMenu)` on `ColumnViewPane`, `m_paneScrollArea`, and `m_canvasWidget`.
  2. Connect `customContextMenuRequested` on these pane widgets to trigger `m_contentPanel->onCustomContextMenuRequested(mapToGlobal(pos))` for the pane's directory path.

---

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
    m_paneScrollArea = new QScrollArea(this);
    m_paneScrollArea->setObjectName("ColumnPaneScrollArea");
    m_paneScrollArea->setWidgetResizable(true);
    m_paneScrollArea->setFrameShape(QFrame::NoFrame);
    m_paneScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_paneScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_canvasWidget = new QWidget(m_paneScrollArea);
    m_canvasWidget->setObjectName("ColumnPaneCanvasWidget");
=======
    setContextMenuPolicy(Qt::CustomContextMenu);

    m_paneScrollArea = new QScrollArea(this);
    m_paneScrollArea->setObjectName("ColumnPaneScrollArea");
    m_paneScrollArea->setWidgetResizable(true);
    m_paneScrollArea->setFrameShape(QFrame::NoFrame);
    m_paneScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_paneScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_paneScrollArea->setContextMenuPolicy(Qt::CustomContextMenu);

    m_canvasWidget = new QWidget(m_paneScrollArea);
    m_canvasWidget->setObjectName("ColumnPaneCanvasWidget");
    m_canvasWidget->setContextMenuPolicy(Qt::CustomContextMenu);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (m_contentPanel) {
        m_folderListView->installEventFilter(m_contentPanel);
        m_listView->installEventFilter(m_contentPanel);
        connect(m_folderListView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
        connect(m_listView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
=======
    if (m_contentPanel) {
        m_folderListView->installEventFilter(m_contentPanel);
        m_listView->installEventFilter(m_contentPanel);
        connect(m_folderListView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
        connect(m_listView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);

        auto onPaneContextMenu = [this](const QPoint& pos) {
            QWidget* senderWidget = qobject_cast<QWidget*>(sender());
            QPoint globalPos = senderWidget ? senderWidget->mapToGlobal(pos) : QCursor::pos();
            m_contentPanel->onCustomContextMenuRequested(globalPos);
        };
        connect(this, &QWidget::customContextMenuRequested, this, onPaneContextMenu);
        connect(m_paneScrollArea, &QWidget::customContextMenuRequested, this, onPaneContextMenu);
        connect(m_canvasWidget, &QWidget::customContextMenuRequested, this, onPaneContextMenu);
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **Verify Diff Precision**:
   Run the diff checker script:
   ```bash
   python3 /home/jules/self_created_tools/check_plan_diff.py "QuarkMeta Architecture/Implementation Plan/ColumnViewWidget-18.md"
   ```

2. **Build Verification**:
   Build the Qt application with CMake:
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --config Debug -j$(nproc)
   ```

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **`ContentPanel::onCustomContextMenuRequested`**: Reused existing SSOT entry for context menus across all view modes without duplicating menu creation code.

---

## Header API Signature Verification

- `QWidget::setContextMenuPolicy(Qt::ContextMenuPolicy)`: Standard Qt QWidget method.
- `ContentPanel::onCustomContextMenuRequested(const QPoint&)`: Existing method declared in `ContentPanel.h`.
