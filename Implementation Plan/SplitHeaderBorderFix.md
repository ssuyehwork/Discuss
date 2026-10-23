# Implementation Plan - Split Header Border Fix

## Overview
This plan restores the `1px solid #333333` top border above `ContentHeaderWidget` in multi-pane split view mode.
In split view mode, the host `#EditorContainer` panel is marked with `isHostPanel="true"`. Previously, a CSS rule `#EditorContainer[isHostPanel="true"] #EditorContainer { border-top: none; }` removed the top border on child pane containers, causing the top divider line above the content header bar to disappear. Removing this override ensures every child pane container maintains a standard `1px solid #333333` border on all four edges, aligning visually with adjacent panels like NavPanel, FavoritePanel, MetaPanel, and FilterPanel.

---

## Modified Files List
- `resources/style.qss`

---

## Detailed Line-by-Line Changes

### 1. Update `resources/style.qss`

```diff
<<<<<<< SEARCH
#EditorContainer[isHostPanel="true"] {
    border: none;
}

#EditorContainer[isHostPanel="true"] #EditorContainer {
    border-top: none;
}
=======
#EditorContainer[isHostPanel="true"] {
    border: none;
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### Build Command
```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

### Verification Steps
1. Launch QuarkMeta.
2. Observe single-pane view: `ContentPanel` has a `1px #333333` top border above the title bar.
3. Split the pane using the split view button or drag-and-drop tab splitting.
4. Verify both the primary pane and secondary pane `ContentHeaderWidget` title bars display a clear `1px solid #333333` top border line that aligns perfectly with the sidebar and metadata panels.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- [x] Reuses existing `#EditorContainer` stylesheet definition.
- [x] Eliminates unnecessary CSS override rule without adding redundant inline styles or custom paint code.

---

## Header API Signature Verification
*(Not Applicable - pure QSS style adjustment)*

---

## Header Inclusion Chain & Type Completeness Check
*(Not Applicable - pure QSS style adjustment)*
