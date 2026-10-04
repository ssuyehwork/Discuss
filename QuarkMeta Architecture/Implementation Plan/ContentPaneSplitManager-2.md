# ContentPaneSplitManager-2 Implementation Plan - Eliminate Outer Border in Single Pane Mode

## 1. Overview
This plan fixes the issue where an outer white/light-gray border line (`border: 1px solid #333333` / `#555555`) erroneously surrounds the main content area in **Single Pane (un-split) mode** (`m_isSplit == false`).

### Root Cause Analysis & Architecture Rationale
1. **Base QSS Inheritance**: In `resources/style.qss`, `#EditorContainer` inherits from the common container group:
   ```css
   #SidebarContainer, #FavoriteContainer, #ListContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
       background-color: #1E1E1E;
       border: 1px solid #333333; /* ⚠️ Default base container border */
       border-radius: 0px;
       color: #EEEEEE;
       margin: 0px;
       padding: 0px;
   }
   ```
   In single-pane mode, even if `activePane="false"`, `#EditorContainer` falls back to `border: 1px solid #333333`, drawing a 1px border rectangle around the content area.
2. **Multi-Pane View Requirement (`m_isSplit == true`)**: Active pane highlight (`border: 1px solid #555555`) is only necessary when multiple panes exist (`m_isSplit == true`) so users can distinguish the currently focused pane.
3. **Single Pane View Requirement (`m_isSplit == false`)**: In single pane mode, the content panel should have `border: none` for a seamless integrated UI without redundant outer borders.

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
        // 单窗格未拆分模式下，绝不触发 activePane 定焦边框
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
   - **Single Pane Mode**: Verify that in single pane mode, `#EditorContainer` has `border: none`. Clicking or focusing the content panel does not draw any outer border line around `ContentPanel`.
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
