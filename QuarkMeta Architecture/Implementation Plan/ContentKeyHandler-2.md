# Implementation Plan - ContentKeyHandler Ctrl+S Collapse/Expand & Event Filter Fix

## Overview
This implementation plan resolves the `Ctrl+S` collapse/expand failure and event handling issues across List, Grid, Justified, and Column views.

### Root Causes
1. **Focus Lost Gating in `ContentKeyHandler::handleKeyPress`**: `handleKeyPress` immediately returned `false` if `obj` was not a `QAbstractItemView` (`if (!view) return false;`). When `folderView` was hidden via `Ctrl+S`, `folderView` lost keyboard focus. Subsequent `Ctrl+S` key presses targeted the parent container (`DualSectionPanel` or `SectionedScrollCanvas`), causing `handleKeyPress` to reject the key event before reaching `if (keyEvent->key() == Qt::Key_S)`.
2. **Pure Folder View Gating (`fileCount == 0`)**: When a directory had 0 files and `folderView` was collapsed, no visible `QAbstractItemView` existed to receive focus or events.
3. **Event Filter Attachment Scope**: `SectionedScrollCanvas` installed `eventFilter` only on child view viewports, leaving `SectionedScrollCanvas`, `DualSectionPanel`, and header bars unfiltered.

## Modified Files List
- `src/ui/controllers/ContentKeyHandler.cpp`
- `src/ui/SectionedScrollCanvas.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/controllers/ContentKeyHandler.cpp`
Allow panel-level shortcuts (`Ctrl+S`, `Ctrl+Shift+N`, `Ctrl+Shift+V`, `Backspace`) to execute even if `obj` is not a `QAbstractItemView`.

<<<<<<< SEARCH
bool ContentKeyHandler::handleKeyPress(QObject* obj, QEvent* event) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (qobject_cast<QLineEdit*>(QApplication::focusWidget())) return false;

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
    if (!view) view = qobject_cast<QAbstractItemView*>(obj->parent());
    if (!view) return false;
=======
bool ContentKeyHandler::handleKeyPress(QObject* obj, QEvent* event) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (qobject_cast<QLineEdit*>(QApplication::focusWidget())) return false;

    // Panel-level global shortcuts (independent of QAbstractItemView focus)
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if ((keyEvent->modifiers() & Qt::ShiftModifier) && keyEvent->key() == Qt::Key_N) {
            m_panel->createNewItem("folder");
            return true;
        }
        if (keyEvent->key() == Qt::Key_S) {
            m_panel->toggleFolderSectionCollapse();
            return true;
        }
    }
    if (keyEvent->key() == Qt::Key_Backspace) {
        QDir dir(m_panel->currentPath());
        if (dir.cdUp()) emit m_panel->directorySelected(dir.absolutePath());
        return true;
    }

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
    if (!view) view = qobject_cast<QAbstractItemView*>(obj->parent());
    if (!view) return false;
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 6. Ctrl + S / C / X / V / Shift+N
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if ((keyEvent->modifiers() & Qt::ShiftModifier) && keyEvent->key() == Qt::Key_N) {
            m_panel->createNewItem("folder");
            return true;
        }
        if (keyEvent->key() == Qt::Key_S) {
            m_panel->toggleFolderSectionCollapse();
            return true;
        }
        if (keyEvent->key() == Qt::Key_C && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            ClipboardService::instance().copyItems(m_panel->getSelectedPaths());
            return true;
        }
=======
    // 6. Ctrl + C / X / V
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_C && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            ClipboardService::instance().copyItems(m_panel->getSelectedPaths());
            return true;
        }
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 8. 导航键
    if (keyEvent->key() == Qt::Key_Backspace) {
        QDir dir(m_panel->currentPath());
        if (dir.cdUp()) emit m_panel->directorySelected(dir.absolutePath());
        return true;
    }
    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
=======
    // 8. 导航键
    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
>>>>>>> REPLACE

---

### 2. `src/ui/SectionedScrollCanvas.cpp`
Install `eventFilter` on `SectionedScrollCanvas` and `DualSectionPanel` so shortcuts work even when clicking background canvas areas.

<<<<<<< SEARCH
    m_panel = new DualSectionPanel(folderView, fileView, m_folderProxyModel, m_fileProxyModel, this);
    m_panel->setContextMenuPolicy(Qt::CustomContextMenu);
    m_panel->setFocusPolicy(Qt::StrongFocus);
    m_panel->setAcceptDrops(true);
    setWidget(m_panel);

    setupConnections();
=======
    m_panel = new DualSectionPanel(folderView, fileView, m_folderProxyModel, m_fileProxyModel, this);
    m_panel->setContextMenuPolicy(Qt::CustomContextMenu);
    m_panel->setFocusPolicy(Qt::StrongFocus);
    m_panel->setAcceptDrops(true);
    setWidget(m_panel);

    if (eventFilter) {
        installEventFilter(eventFilter);
        if (viewport()) viewport()->installEventFilter(eventFilter);
        m_panel->installEventFilter(eventFilter);
    }

    setupConnections();
>>>>>>> REPLACE

---

## Build & Verification Steps
1. Perform CMake configuration and build using standard MSVC/Qt toolchain.
2. Open a folder containing both subfolders and files.
3. Press `Ctrl+S` to collapse the folder section. Verify that pressing `Ctrl+S` a second time successfully expands the folder section.
4. Open a folder containing folders only (`fileCount == 0`). Press `Ctrl+S` to collapse and press `Ctrl+S` again to expand. Confirm it expands properly without getting stuck or locked at 0px height.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `ContentPanel::toggleFolderSectionCollapse()` as the unified SSOT entry point.

## Header API Signature Verification
- `ContentPanel::toggleFolderSectionCollapse()` -> `void toggleFolderSectionCollapse()`
