# ContentPaneSplitManager-3 Implementation Plan - SSOT Fix for Split State & Single/Multi-Pane Focus Border

## 1. Overview
This plan fixes the SSOT architecture flaw in `ContentPaneSplitManager` where secondary (副) panes returned `false` for `isSplitMode()`, while completely resolving the single-pane outer border issue without breaking multi-pane focus highlights.

### Architectural Root Causes & SSOT Solution
1. **SSOT Split State Fragmentation (`isSplitMode`)**:
   - `m_isSplit` is only set to `true` on the **Root Pane**'s manager.
   - Secondary panes' `ContentPaneSplitManager` instances kept `m_isSplit == false`.
   - Calling `isSplitMode()` on a secondary pane returned `false`, breaking any split-dependent logic (including active pane focus borders).
   - **Fix**: Delegate `isSplitMode()` to `rootPane()`:
     ```cpp
     bool ContentPaneSplitManager::isSplitMode() const {
         if (m_rootPane && m_rootPane != m_panel && m_rootPane->m_splitManager) {
             return m_rootPane->m_splitManager->isSplitMode();
         }
         return m_isSplit;
     }
     ```
2. **QSS Base Container Border Inheritance**:
   - `#EditorContainer` inherited `border: 1px solid #333333` from the default container group in `resources/style.qss`.
   - **Fix**: Separate `#EditorContainer` in `resources/style.qss` to give it `border: none` by default in single-pane mode, while displaying `border: 1px solid #555555` when `activePane="true"` in split mode.
3. **Active Pane Highlight Logic (`setActivePane`)**:
   - Use `bool inSplit = isSplitMode();` so both Root and Secondary panes accurately perceive split mode, setting `activePane` to `true` when active during a split view, and `false` in single-pane view.

---

## 2. Modified Files List
- `resources/style.qss`
- `src/ui/controllers/ContentPaneSplitManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### `resources/style.qss`
```
<<<<<<< SEARCH
/* 核心容器样式 - 彻底消除 margin，由 QSplitter 5px 句柄原生提供栏间 5px 物理间距，确保面板精确呈现 230px 物理满额宽度 */
#SidebarContainer, #FavoriteContainer, #ListContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 0px;
    color: #EEEEEE;
    margin: 0px;
    padding: 0px;
}

#EditorContainer[activePane="true"], ContentPanel[activePane="true"] {
    border: 1px solid #555555;
}

#EditorContainer[isHostPanel="true"] {
    border: none;
}
=======
/* 核心容器样式 - 彻底消除 margin，由 QSplitter 5px 句柄原生提供栏间 5px 物理间距，确保面板精确呈现 230px 物理满额宽度 */
#SidebarContainer, #FavoriteContainer, #ListContainer, #MetadataContainer, #FilterContainer {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 0px;
    color: #EEEEEE;
    margin: 0px;
    padding: 0px;
}

/* 编辑器/内容面板基础独立样式：单窗格下默认无边框，呈现清爽无缝 UI */
#EditorContainer {
    background-color: #1E1E1E;
    border: none;
    border-radius: 0px;
    color: #EEEEEE;
    margin: 0px;
    padding: 0px;
}

/* 仅在多窗格拆分且面板激活时呈现 1px 高亮定焦边框 */
#EditorContainer[activePane="true"], ContentPanel[activePane="true"] {
    border: 1px solid #555555;
}

#EditorContainer[isHostPanel="true"] {
    border: none;
}
>>>>>>> REPLACE
```

### `src/ui/controllers/ContentPaneSplitManager.cpp`
```
<<<<<<< SEARCH
bool ContentPaneSplitManager::isSplitMode() const {
    return m_isSplit;
}
=======
bool ContentPaneSplitManager::isSplitMode() const {
    if (m_rootPane && m_rootPane != m_panel && m_rootPane->m_splitManager) {
        return m_rootPane->m_splitManager->isSplitMode();
    }
    return m_isSplit;
}
>>>>>>> REPLACE
```

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

    bool inSplit = isSplitMode();

    if (inSplit && m_primaryPaneContainer) {
        m_primaryPaneContainer->setProperty("activePane", active ? "true" : "false");
        m_primaryPaneContainer->style()->unpolish(m_primaryPaneContainer);
        m_primaryPaneContainer->style()->polish(m_primaryPaneContainer);
    } else {
        m_panel->setProperty("activePane", inSplit && active ? "true" : "false");
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
   - **Single Pane Mode**: Verify that in single pane mode, no outer border line (`border: none`) is displayed around `ContentPanel`.
   - **Multi-Pane Split Mode (Root Pane & Secondary Pane)**:
     - Open split view (2 or 3 panes).
     - Click Root Pane: Verify `activePane="true"` is set on `m_primaryPaneContainer` and `#555555` border is shown.
     - Click Secondary Pane: Verify `isSplitMode()` evaluates to `true` on secondary pane via `rootPane()`, setting `activePane="true"` and displaying `#555555` border on secondary pane.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **SSOT State Delegation**: Single SSOT source for `isSplitMode()` delegated through `rootPane()`.
- **Zero Duplication**: Secondary panes delegate query to `rootPane()` rather than maintaining duplicate, desynchronized `m_isSplit` flags.

---

## 6. Header API Signature Verification

| Class / Function Header Signature | File Origin | Result |
| :--- | :--- | :--- |
| `bool ContentPaneSplitManager::isSplitMode() const` | `src/ui/controllers/ContentPaneSplitManager.h:18` | Verified |
| `ContentPanel* ContentPaneSplitManager::rootPane() const` | `src/ui/controllers/ContentPaneSplitManager.h:23` | Verified |
| `void ContentPaneSplitManager::setActivePane(bool active)` | `src/ui/controllers/ContentPaneSplitManager.h:32` | Verified |

---

## 7. Header Inclusion Chain & Type Completeness Check

- `ContentPaneSplitManager.cpp` includes `<QStyle>` for `style()->unpolish()` / `style()->polish()`.
- `ContentPanel.h` is included in `ContentPaneSplitManager.cpp`.
- `m_splitManager` member access on `m_rootPane` is fully defined and valid.
