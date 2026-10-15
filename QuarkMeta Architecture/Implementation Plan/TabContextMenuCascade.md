# Implementation Plan - Cascaded Tab Context Menu (New Tab & Merge into Existing Tab Split)

## 1. Overview
This implementation plan refactors the item context menu for folders in `src/ui/controllers/ContentContextMenu.cpp`:
1. Replaces the flat menu item **"在新标签页中打开"** with a sub-menu named **"标签页"**.
2. Adds two sub-options under **"标签页"**:
   - **"在新标签页中打开"**: Opens a separate new tab for the folder at the end of the tab bar.
   - **"在标签页中打开"**: A cascading sub-menu dynamically listing all open tabs.
3. Clicking on a tab under **"在标签页中打开 ▶ [Tab Name]"** switches to that target tab and merges the folder as a multi-pane split (`splitPane(Qt::Horizontal, path)`), exactly like dragging a folder onto a tab button.

---

## 2. Modified Files List
- `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/controllers/ContentContextMenu.cpp`
Update `showMenu` in `ContentContextMenu.cpp` to replace flat `ActionOpenInNewTab` with a cascaded submenu structure:

```cpp
<<<<<<< SEARCH
            if (isFolder) {
                menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE"), 18), "在新标签页中打开")->setData(ContentPanel::ActionOpenInNewTab);
            } else {
                menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
            }
=======
            if (isFolder) {
                QMenu* tabCascadeMenu = menu.addMenu(UiHelper::getIcon("add", QColor("#EEEEEE"), 18), "标签页");
                UiHelper::applyMenuStyle(tabCascadeMenu);

                tabCascadeMenu->addAction(UiHelper::getIcon("add", QColor("#EEEEEE"), 16), "在新标签页中打开")->setData(ContentPanel::ActionOpenInNewTab);

                QMenu* openInExistingTabMenu = tabCascadeMenu->addMenu(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 16), "在标签页中打开");
                UiHelper::applyMenuStyle(openInExistingTabMenu);

                TabBarWidget* tabBar = (m_panel && m_panel->window()) ? m_panel->window()->findChild<TabBarWidget*>() : nullptr;
                if (tabBar) {
                    int count = tabBar->tabCount();
                    for (int i = 0; i < count; ++i) {
                        QString tabTitle = tabBar->tabTitle(i);
                        QAction* actMergeTab = openInExistingTabMenu->addAction(UiHelper::getIcon("tab", QColor("#EEEEEE"), 16), tabTitle);
                        connect(actMergeTab, &QAction::triggered, this, [this, i, path]() {
                            if (!m_panel) return;
                            tabBar->setCurrentIndex(i, true);
                            m_panel->splitPane(Qt::Horizontal, path, false);
                        });
                    }
                }
            } else {
                menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
            }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile QuarkMeta target using CMake:
   `cmake --build build`
2. Launch QuarkMeta with 2 or 3 open tabs.
3. Right-click on a folder in the item list.
4. Verify that **"标签页 ▶"** appears below "打开文件夹".
5. Hover over **"标签页 ▶"** and confirm sub-options:
   - **"在新标签页中打开"**
   - **"在标签页中打开 ▶"** (lists all open tabs).
6. Click **"在标签页中打开 ▶ [Tab 2]"**.
7. Verify that:
   - The application switches to Tab 2.
   - The selected folder is horizontally split into a multi-pane layout inside Tab 2.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Multi-Pane SSOT**: Uses existing `m_panel->splitPane(Qt::Horizontal, path, false)` pipeline.
- **Tab Bar SSOT**: Queries open tab titles and indexes directly via `TabBarWidget`.

---

## 6. Header API Signature Verification
| Class / Function Signature | Declared Header File | Signature Match Verification |
| :--- | :--- | :--- |
| `int tabCount() const` | `src/ui/TabBarWidget.h` | 100% Exact Match |
| `QString tabTitle(int index) const` | `src/ui/TabBarWidget.h` | 100% Exact Match |
| `void setCurrentIndex(int index, bool forceNotify = false)` | `src/ui/TabBarWidget.h` | 100% Exact Match |

---

## 7. Header Inclusion Chain & Type Completeness Check
| Inclusion / Type Name | File | Purpose & Completeness Check |
| :--- | :--- | :--- |
| `#include "../TabBarWidget.h"` | `src/ui/controllers/ContentContextMenu.cpp` | Provides `TabBarWidget` class definition. |
