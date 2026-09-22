# Implementation Plan: ContentPanel-19.md (Secondary Pane Default Path Fallback Fix)

## 1. Overview
Fixes the fallback default path when splitting panes in `ContentPanel`. Previously, if `secondaryPath` was empty when splitting a pane, `splitPane()` defaulted to `"computer://"`. This implementation changes the fallback default path to `m_currentPath`, so splitting the current panel duplicates the active folder path by default into the secondary pane.

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.cpp`
Change fallback path in `m_secondaryContentPanel->loadDirectory(...)` from `"computer://"` to `m_currentPath`.

```
<<<<<<< SEARCH
    if (m_secondaryContentPanel) {
        m_secondaryContentPanel->loadDirectory(!secondaryPath.isEmpty() ? secondaryPath : "computer://");
    }
=======
    if (m_secondaryContentPanel) {
        m_secondaryContentPanel->loadDirectory(!secondaryPath.isEmpty() ? secondaryPath : m_currentPath);
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Run `cmake --build build` or `ninja -C build`.
2. Navigate to any folder (e.g., `T 图片`).
3. Drag a tab onto the content panel overlay or trigger split pane without passing a secondary path.
4. Verify that the secondary pane opens showing the active directory (`T 图片`) instead of "Computer" / drive icons.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **View Navigation SSOT**: Uses `m_secondaryContentPanel->loadDirectory(...)` with `m_currentPath`.

## 6. Header API Signature Verification
- `ContentPanel::currentPath() const` -> Verified existing in `src/ui/ContentPanel.h`.
- `ContentPanel::loadDirectory(const QString&, bool)` -> Verified existing in `src/ui/ContentPanel.h`.
