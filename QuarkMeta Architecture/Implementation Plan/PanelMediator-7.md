# Implementation Plan - PanelMediator-7.md

## Overview
修复新建标签页（Tab）时内容视口不更新、多窗格状态清理不彻底以及路径导航时 Tab 标题未双向同步的问题。

具体根因分析：
1. **多窗格恢复断裂 (ContentPaneSplitManager::restoreSplitState)**：新建标签页时其 `TabSplitState` 的 `panePaths` 默认为空。`restoreSplitState` 在清理原有副窗格后，由于 `state.panePaths.isEmpty()` 成立而提前 `return`，未将根主窗格的 `m_headerWidget` 与 `m_viewStack` 重新挂载至 `m_mainLayout`，导致从分屏标签页切换到新建单窗格标签页时，主内容区组件处于未挂载或隐藏状态，视口卡死在旧界面。
2. **导航双向同步缺失 (PanelMediator::setupConnections)**：`NavigationService::currentUrlChanged` 广播触发时，未调用 `titleBar->tabBar()->updateCurrentTabTitle`，导致在内容面板中导航切换路径时，顶部 Tab 标签页的标题与 URL 无法随之同步更新。

---

## Modified Files List
- `src/ui/controllers/ContentPaneSplitManager.cpp`
- `src/ui/PanelMediator.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/controllers/ContentPaneSplitManager.cpp`
在 `restoreSplitState` 方法中，补齐 `state.panePaths.isEmpty()` 场景下的根主窗格布局重置与还原逻辑：

```
<<<<<<< SEARCH
    if (state.panePaths.isEmpty()) {
        updateContainerMinimumWidth();
        refreshActiveIndicators();
        m_blockLayoutSignals = false;
        notifyLayoutChanged();
        return;
    }
=======
    if (state.panePaths.isEmpty()) {
        if (m_primaryPaneContainer) {
            if (m_panel->m_headerWidget && m_primaryPaneContainer->layout()) {
                m_primaryPaneContainer->layout()->removeWidget(m_panel->m_headerWidget);
            }
            if (m_panel->m_viewStack && m_primaryPaneContainer->layout()) {
                m_primaryPaneContainer->layout()->removeWidget(m_panel->m_viewStack);
            }
            m_primaryPaneContainer->deleteLater();
            m_primaryPaneContainer = nullptr;
        }
        if (m_panel->m_headerWidget) {
            m_panel->m_mainLayout->addWidget(m_panel->m_headerWidget);
            m_panel->m_headerWidget->show();
        }
        if (m_panel->m_viewStack) {
            m_panel->m_mainLayout->addWidget(m_panel->m_viewStack, 1);
            m_panel->m_viewStack->show();
        }
        updateContainerMinimumWidth();
        refreshActiveIndicators();
        m_blockLayoutSignals = false;
        notifyLayoutChanged();
        return;
    }
>>>>>>> REPLACE
```

### 2. `src/ui/PanelMediator.cpp`
在 `NavigationService::currentUrlChanged` 连接槽函数中，添加对 `TabBarWidget::updateCurrentTabTitle` 的同步驱动：

```
<<<<<<< SEARCH
        if (addressBar) addressBar->setPath(displayPath);
        if (navPanel) navPanel->selectPath(url == "computer://" ? "" : url);
=======
        if (addressBar) addressBar->setPath(displayPath);
        if (navPanel) navPanel->selectPath(url == "computer://" ? "" : url);
        if (titleBar && titleBar->tabBar()) {
            titleBar->tabBar()->updateCurrentTabTitle(displayPath, url);
        }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 编译步骤
```bash
cmake -B build
cmake --build build --config Release
```

### 验证方法
1. **新建标签页测试**：运行 QuarkMeta，点击标题栏 “+” 按钮或按 `Ctrl+T`，新建一个标签页。验证顶部 Tab 显示“此电脑”，同时主内容面板成功切换并加载“此电脑” (`computer://`) 驱动器列表。
2. **多窗格/分屏下新建标签页测试**：在一个标签页中开启双窗格/多窗格分屏后，点击 “+” 新建标签页。验证旧分屏状态被清理，新建标签页以纯净单窗格呈现“此电脑”。
3. **导航标题双向同步测试**：在内容面板或侧边栏点击双击进入子文件夹。验证顶部 Tab 标签页的标题与图标实时更新为当前文件夹名称。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **TabBar 标题更新入口**：严格复用 `TabBarWidget::updateCurrentTabTitle(displayPath, url)` 既有 API，未另起炉灶。
- **导航与工作区重置入口**：严格复用 `NavigationService::navigateTo(url)` 与 `ContentPaneSplitManager::restoreSplitState(state)` 既有 SSOT 入口，未手写局部重复加载逻辑。

---

## Header API Signature Verification
| 类名 / 接口 | 物理签名核查 (.h 源头) | 结论 |
| :--- | :--- | :--- |
| `TabBarWidget::updateCurrentTabTitle` | `void updateCurrentTabTitle(const QString& title, const QString& url);` | 100% 映射一致 |
| `NavigationService::currentUrlChanged` | `void currentUrlChanged(const QString& url, const QString& displayPath);` | 100% 映射一致 |
| `ContentPaneSplitManager::restoreSplitState` | `void restoreSplitState(const TabSplitState& state);` | 100% 映射一致 |

---

## Header Inclusion Chain & Type Completeness Check
- `PanelMediator.cpp` 已具备 `#include "TabBarWidget.h"` 与 `#include "TitleBarWidget.h"`，类型定义完整，无头文件断裂风险。
- `ContentPaneSplitManager.cpp` 包含内部成员 `m_panel`（`ContentPanel*`），其 `m_mainLayout`、`m_headerWidget` 与 `m_viewStack` 指针类型在内部作用域已完整定义。
