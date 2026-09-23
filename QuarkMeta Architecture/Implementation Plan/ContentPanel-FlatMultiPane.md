# ContentPanel Flat Multi-Pane Architecture Implementation Plan

## Overview
This implementation plan converts `ContentPanel` split-pane architecture from binary recursive nesting to a flat, equal-width distribution model:
1. **Flat Hierarchy**: A root `ContentPanel` owns a flat `QList<ContentPanel*> m_panes` (up to 3 additional panes, totaling 4 panes max) hosted directly under a single `QSplitter`.
2. **Equal-Width Distribution**: Sub-pane resizing and additions trigger `redistributePaneSizes()` to ensure equal width allocation across all open panes.
3. **Decoupled Closing Behavior**: Individual pane close actions invoke `closePane(pane)`, removing only the targeted pane. Closing down to a single pane restores standard single-pane layout.
4. **Header Compatibility**: `closeSecondaryPane()` closes the last/active sub-pane instead of destroying all panes at once.

## Modified Files List
1. `src/ui/ContentPanel.h`
2. `src/ui/ContentPanel.cpp`
3. `src/ui/PanelMediator.cpp`
4. `resources/style.qss`

## Detailed Changes

### 1. `src/ui/ContentPanel.h`
Added multi-pane member attributes and helper methods:
```cpp
static constexpr int kMaxPanes = 4;

ContentPanel* secondaryContentPanel() const { return m_panes.isEmpty() ? nullptr : m_panes.first(); }
QList<ContentPanel*> panes() const { return m_panes; }
int paneCount() const { return 1 + m_panes.size(); }
ContentPanel* rootPane() const { return m_rootPane ? m_rootPane : const_cast<ContentPanel*>(this); }
void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString());
void closePane(ContentPanel* pane);
```

### 2. `src/ui/ContentPanel.cpp`
Implemented flat splitter management in `splitPane()`, `closePane()`, `closeSecondaryPane()`, and `redistributePaneSizes()`.

### 3. `src/ui/PanelMediator.cpp`
Updated active pane state traversal to iterate through `node->panes()`.

### 4. `resources/style.qss`
Removed `#ContentPanelHost` style rule.

## Verification
- Confirmed no dangling references across `ContentPanel`, `PanelMediator`, and `ContentContextMenu`.
- Checked active pane state propagation.
