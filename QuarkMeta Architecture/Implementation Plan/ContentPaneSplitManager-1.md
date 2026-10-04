# ContentPaneSplitManager-1 Implementation Plan - Suppress Active Focus Border in Single Pane Mode

## 1. Overview
This plan fixes the issue where the active focus border (`border: 1px solid #555555`) is erroneously displayed in **Single Pane (un-split) mode** (`m_isSplit == false`).

### Design & Architecture Rationale
- **Multi-Pane View (`m_isSplit == true`)**: The active pane requires a clear focus highlight (`activePane="true"`) so users know which pane currently receives keyboard shortcuts, address bar navigation, and command routing.
- **Single Pane View (`m_isSplit == false`)**: Only one pane exists, rendering a focus border redundant and visually intrusive. In single pane mode, `activePane` must strictly evaluate to `"false"` for `m_panel`.

---

## 2. Modified Files List
- `src/ui/controllers/ContentPaneSplitManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### `src/ui/controllers/ContentPaneSplitManager.cpp`
```
<<<<<<< SEARCH
void ContentPaneSplitManager::setActivePane(bool active) {
    if (active && rootPane()) {
        rootPane()->m_splitManager->m_activePaneForSplit = m_panel;
    }

    if (m_isSplit && m_primaryPaneContainer) {
        m_primaryPaneContainer->setProperty("activePane", active ? "true" : "false");
        m_primaryPaneContainer->style()->unpolish(m_primaryPaneContainer);
        m_primaryPaneContainer->style()->polish(m_primaryPaneContainer);
    } else {
        m_panel->setProperty("activePane", active ? "true" : "false");
        m_panel->style()->unpolish(m_panel);
        m_panel->style()->polish(m_panel);
    }

    if (m_panel->m_headerWidget) {
        m_panel->m_headerWidget->setActive(active);
    }
}
=======
void ContentPaneSplitManager::setActivePane(bool active) {
    if (active && rootPane()) {
        rootPane()->m_splitManager->m_activePaneForSplit = m_panel;
    }

    if (m_isSplit && m_primaryPaneContainer) {
        m_primaryPaneContainer->setProperty("activePane", active ? "true" : "false");
        m_primaryPaneContainer->style()->unpolish(m_primaryPaneContainer);
        m_primaryPaneContainer->style()->polish(m_primaryPaneContainer);
    } else {
        m_panel->setProperty("activePane", m_isSplit && active ? "true" : "false");
        m_panel->style()->unpolish(m_panel);
        m_panel->style()->polish(m_panel);
    }

    if (m_panel->m_headerWidget) {
        m_panel->m_headerWidget->setActive(active);
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Check**:
   ```bash
   cmake -B build -S .
   cmake --build build --config Debug
   ```
2. **Behavior Verification**:
   - **Single Pane Mode**: Verify that in single pane mode, clicking or focusing the content panel does not draw the `#555555` focus border around `ContentPanel`.
   - **Multi-Pane Split Mode**: Trigger a split view (2 or more panes). Verify that clicking a pane sets `activePane="true"` on the container and draws the active focus border around the currently active pane.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **SSOT State Preservation**: `ContentPaneSplitManager` remains the SSOT for `m_isSplit` and active pane routing.
- **No Parallel Logic**: Utilizes existing QSS property polishing mechanism (`unpolish` / `polish`) without creating duplicate properties or custom paint overrides.

---

## 6. Header API Signature Verification

| Class / Function Header Signature | File Origin | Result |
| :--- | :--- | :--- |
| `void ContentPaneSplitManager::setActivePane(bool active)` | `src/ui/controllers/ContentPaneSplitManager.h:32` | Verified |
| `bool ContentPaneSplitManager::isSplitMode() const` | `src/ui/controllers/ContentPaneSplitManager.h:18` | Verified |
| `ContentPanel* ContentPaneSplitManager::rootPane() const` | `src/ui/controllers/ContentPaneSplitManager.h:23` | Verified |
| `void ContentHeaderWidget::setActive(bool active)` | `src/ui/ContentHeaderWidget.h:26` | Verified |

---

## 7. Header Inclusion Chain & Type Completeness Check

- `ContentPaneSplitManager.cpp` includes `<QStyle>` for `style()->unpolish()` / `style()->polish()`.
- `ContentPanel` and `ContentHeaderWidget` headers are included in `ContentPaneSplitManager.cpp`.
- All types and signatures are completely verified.
