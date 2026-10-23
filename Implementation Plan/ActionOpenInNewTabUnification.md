# Implementation Plan - ActionOpenInNewTab Decoupling & SSOT Routing Unification

## 1. Overview
Currently, executing the "Open in New Tab" (`ActionOpenInNewTab`) action from the context menu (lines 580–590 of `src/ui/controllers/ContentContextMenu.cpp`) performs direct UI tree traversal and unvalidated down-drilling:
`m_panel->window()->findChild<TitleBarWidget*>()->tabBar()->openOrFocusTab(targetPath)`

This implementation:
1. Violates the **Dependency Lock** rule (`SYSTEM_PROMPT.md` Rule 2) by performing unvalidated UI widget hierarchy down-drilling from a Controller into a specific Top-Level Window child widget (`TitleBarWidget` / `TabBarWidget`).
2. Directly couples `ContentContextMenu` to specific UI layout implementations, breaking modularity and testability.

This implementation plan decouples tab creation by introducing a unified SSOT entry point in `NavigationService`:
- `NavigationService::instance().openInNewTab(const QString& path)`

`PanelMediator` (the central mediator) connects `NavigationService::requestOpenInNewTab` directly to `TabBarWidget::openOrFocusTab()`. `ContentContextMenu` delegates cleanly to `NavigationService::instance().openInNewTab(targetPath)`, completely eliminating `findChild<TitleBarWidget*>` down-drilling.

---

## 2. Modified Files List
1. `src/core/NavigationService.h` (Public method `openInNewTab` and signal `requestOpenInNewTab` declaration)
2. `src/core/NavigationService.cpp` (Implementation of `openInNewTab`)
3. `src/ui/PanelMediator.cpp` (Connecting `requestOpenInNewTab` to `TabBarWidget::openOrFocusTab`)
4. `src/ui/controllers/ContentContextMenu.cpp` (`ActionOpenInNewTab` refactoring to delegate to `NavigationService::instance().openInNewTab`)

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/core/NavigationService.h`

<<<<<<< SEARCH
    // 核心导航调度接口
    void navigateTo(const QString& rawUrl, bool recordHistory = true);
    void goBack();
    void goForward();
    void goUp();
    void refresh();
=======
    // 核心导航调度接口
    void navigateTo(const QString& rawUrl, bool recordHistory = true);
    void openInNewTab(const QString& rawPath);
    void goBack();
    void goForward();
    void goUp();
    void refresh();
>>>>>>> REPLACE

<<<<<<< SEARCH
signals:
    /**
     * @brief 全局统一路径变更信号 (驱动各子面板单向加载数据)
     * @param url 标准协议 URL (如 file://C:/Users 或 computer://)
     * @param displayPath 适合 UI 面包屑展示的文本 (如 C:\Users 或 此电脑)
     */
    void currentUrlChanged(const QString& url, const QString& displayPath);
=======
signals:
    /**
     * @brief 全局请求在标签页打开路径信号
     */
    void requestOpenInNewTab(const QString& url);

    /**
     * @brief 全局统一路径变更信号 (驱动各子面板单向加载数据)
     * @param url 标准协议 URL (如 file://C:/Users 或 computer://)
     * @param displayPath 适合 UI 面包屑展示的文本 (如 C:\Users 或 此电脑)
     */
    void currentUrlChanged(const QString& url, const QString& displayPath);
>>>>>>> REPLACE

---

### File 2: `src/core/NavigationService.cpp`

<<<<<<< SEARCH
void NavigationService::navigateTo(const QString& rawUrl, bool recordHistory) {
=======
void NavigationService::openInNewTab(const QString& rawPath) {
    if (rawPath.isEmpty()) return;
    QString normUrl = normalizeUrl(rawPath);
    emit requestOpenInNewTab(normUrl);
}

void NavigationService::navigateTo(const QString& rawUrl, bool recordHistory) {
>>>>>>> REPLACE

---

### File 3: `src/ui/PanelMediator.cpp`

<<<<<<< SEARCH
void PanelMediator::bindSignals() {
    if (!m_navPanel || !m_favoritePanel || !m_contentPanel || !m_metaPanel) {
        return;
    }
=======
void PanelMediator::bindSignals() {
    if (!m_navPanel || !m_favoritePanel || !m_contentPanel || !m_metaPanel) {
        return;
    }

    // 🚀【Tab页在新标签中打开路由】：从 NavigationService 接收在 Tab 栏打开请求，解耦控制器下钻
    connect(&NavigationService::instance(), &NavigationService::requestOpenInNewTab, this, [this](const QString& url) {
        if (m_titleBar && m_titleBar->tabBar()) {
            m_titleBar->tabBar()->openOrFocusTab(url);
        }
    });
>>>>>>> REPLACE

---

### File 4: `src/ui/controllers/ContentContextMenu.cpp`

<<<<<<< SEARCH
        case ContentPanel::ActionOpenInNewTab: {
            QString targetPath = path;
            if (targetPath.isEmpty()) {
                QStringList selected = m_panel->getSelectedPaths();
                if (!selected.isEmpty()) targetPath = selected.first();
            }
            if (!targetPath.isEmpty()) {
                if (m_panel->window()) {
                    TitleBarWidget* titleBar = m_panel->window()->findChild<TitleBarWidget*>();
                    if (titleBar && titleBar->tabBar()) {
                        titleBar->tabBar()->openOrFocusTab(targetPath);
                    }
                }
            }
            break;
        }
=======
        case ContentPanel::ActionOpenInNewTab: {
            QString targetPath = path;
            if (targetPath.isEmpty()) {
                QStringList selected = m_panel->getSelectedPaths();
                if (!selected.isEmpty()) targetPath = selected.first();
            }
            if (!targetPath.isEmpty()) {
                NavigationService::instance().openInNewTab(targetPath);
            }
            break;
        }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### Build Command
```bash
cmake -B build -S .
cmake --build build --config Release
```

### Verification Methods
1. **Open in New Tab Test**: Right-click any folder item in Grid or List view and select "在新标签页中打开".
2. **Tab Bar Response**: Verify a new tab is created in `TabBarWidget` with the target folder loaded, without any `findChild` widget down-drilling in C++ controller code.
3. **Decoupling Verification**: Confirm `ContentContextMenu.cpp` delegates cleanly through `NavigationService::instance().openInNewTab(targetPath)`.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Reused SSOT Entry Points**:
  - `NavigationService::instance().openInNewTab(targetPath)`: Single Source of Truth for requesting new tab navigation.
  - `PanelMediator`: Single Source of Truth for connecting core domain events/signals to UI components.
- **Anti-Redundancy**: Completely removed `findChild<TitleBarWidget*>()` UI tree traversal from `ContentContextMenu.cpp`.

---

## 6. Header API Signature Verification

| Class / Component | Function / Method Signature | Header File Path | Status |
| :--- | :--- | :--- | :--- |
| `NavigationService` | `void openInNewTab(const QString& rawPath)` | `src/core/NavigationService.h` | New Addition |
| `NavigationService` | `void requestOpenInNewTab(const QString& url)` | `src/core/NavigationService.h` | New Signal Addition |
| `TabBarWidget` | `void openOrFocusTab(const QString& path)` | `src/ui/TabBarWidget.h` | Verified Existing |

---

## 7. Header Inclusion Chain & Type Completeness Check

- `src/core/NavigationService.h`:
  - Contains `#include <QString>`.
- `src/core/NavigationService.cpp`:
  - Implements `openInNewTab`.
- `src/ui/PanelMediator.cpp`:
  - Includes `"../core/NavigationService.h"`, `"TitleBarWidget.h"`, `"TabBarWidget.h"`.
- `src/ui/controllers/ContentContextMenu.cpp`:
  - Includes `"../../core/NavigationService.h"`. No longer needs down-drilling headers.
