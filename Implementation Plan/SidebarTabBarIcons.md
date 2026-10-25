# Implementation Plan - Sidebar Tab Bar Icons Integration

**Base Commit:** `1481e72e0d24c1e3d46be0db4a63a278d6cf052e`

---

## 1. Overview
This plan describes the addition of SVG icons to the `SidebarContainerWidget` tab bar (`QTabBar`).
The top tab bar in the sidebar contains two tabs: "收藏夹" (Bookmarks/Favorites) and "库" (Library).
To improve visual identity and interface consistency, we integrate neutral gray (`#888888`, size 16x16) SVG icons into both tabs using `UiHelper::getIcon`:
- Tab 0 ("收藏夹"): Uses icon key `"Favorite"` from `SvgIcons.h`.
- Tab 1 ("库"): Uses icon key `"Search-in-library"` from `SvgIcons.h`.

---

## 2. Modified Files List
- `src/ui/SidebarContainerWidget.cpp`: Update `SidebarContainerWidget::initUi()` to set `setIconSize(QSize(16, 16))` and assign icons to tab index 0 and 1 via `addTab`.

---

## 3. Detailed Line-by-Line Changes

### `src/ui/SidebarContainerWidget.cpp`

```diff
<<<<<<< SEARCH
    m_tabBar = new QTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->addTab("收藏夹");
    m_tabBar->addTab("库");
    m_tabBar->setCursor(Qt::PointingHandCursor);
=======
    m_tabBar = new QTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setIconSize(QSize(16, 16));
    m_tabBar->addTab(UiHelper::getIcon("Favorite", "#888888", 16), "收藏夹");
    m_tabBar->addTab(UiHelper::getIcon("Search-in-library", "#888888", 16), "库");
    m_tabBar->setCursor(Qt::PointingHandCursor);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Execute plan verification check via `python3 tools/plan_verify/verify_plan.py Implementation\ Plan/SidebarTabBarIcons.md`.
2. Ensure no C++ source code files under `src/` or `resources/` were modified during this phase.

---

## 5. Official API Reuse & Anti-Redundancy
- Reuses `UiHelper::getIcon(key, colorHex, size)` for rendering SVG icons from `SvgIcons.h` without creating duplicate pixmap wrappers or redundant resource files.
- Uses standard Qt `QTabBar::setIconSize` and `QTabBar::addTab(QIcon, QString)` methods.

---

## 6. Header API Signature Verification
- No `.h` headers are modified. `SidebarContainerWidget.h` public API signature remains strictly frozen and unchanged.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `SidebarContainerWidget.cpp` already includes `UiHelper.h` at the top (`#include "UiHelper.h"`).
- `UiHelper::getIcon` is fully accessible and declared in `UiHelper.h`.

---

## 8. Data SSOT Declaration Table
| Data Field | Storage Location | Access / Mutation Route | SSOT Guarantee |
| :--- | :--- | :--- | :--- |
| Sidebar Active Tab Index | `AppConfig` (`"Sidebar/CurrentTabIndex"`) | `SidebarContainerWidget::initUi()` reads index; `QTabBar::currentChanged` writes index | Single config key managed via `AppConfig` |

---

## 9. Compliance Self-Check Table
| Check Item | Status | Details |
| :--- | :--- | :--- |
| Anti-Patching Rule | PASS | Complete block search/replace provided without placeholder omitted lines. |
| Thread & IO Isolation | PASS | Icon loading and UI creation execute exclusively on the main GUI thread. |
| Non-reentrant Mutex Safety | PASS | No mutex or locks involved in QTabBar icon population. |
| Logic Isolation | PASS | Only tab icon visualization is modified; tab change signal logic and index handling are untouched. |
| Direct Code Modification Prohibition | PASS | Zero source files in `src/` modified. Changes reside purely in this Implementation Plan. |

---

## 10. Behavior Preservation Declaration Table
| Existing Behavior | Preserved? | Reason / Implementation Details |
| :--- | :--- | :--- |
| Tab Switch Signal & Config Persistence | Yes | `connect(m_tabBar, &QTabBar::currentChanged, ...)` logic remains unchanged. |
| Saved Index Restoration | Yes | `AppConfig::instance().getValue("Sidebar/CurrentTabIndex", 0)` read logic remains unchanged. |
