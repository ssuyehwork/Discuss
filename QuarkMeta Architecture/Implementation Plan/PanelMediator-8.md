# Implementation Plan - PanelMediator-8.md

## Overview
修复新建标签页（Tab）时内容视口不更新、多窗格/单窗格状态还原时的死锁与组件未挂载问题，以及路径导航时 Tab 标题未双向同步的问题。

彻底纠正与自检项：
在 `PanelMediator-7.md` 中，企图在 `state.panePaths.isEmpty()` 时直接 `return`，这会导致从多分屏标签页切换到新建单窗格标签页时，清理旧窗格后主视口组件（`m_headerWidget` 与 `m_viewStack`）依然残留处于被销毁的 `m_primaryPaneContainer` 中而无法显示，引发视口卡死。
在 `PanelMediator-8.md` 中，彻底移除盲目的 `state.panePaths.isEmpty()` 提前 `return`，统一在非分屏树分支中重置主窗格布局（清理 `m_primaryPaneContainer` 并将 `m_headerWidget` 与 `m_viewStack` 挂回 `m_mainLayout`），确保无论有无 `panePaths`，单窗格视口均能正常挂载与呈现。

---

## Modified Files List
- `src/ui/controllers/ContentPaneSplitManager.cpp`
- `src/ui/PanelMediator.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/controllers/ContentPaneSplitManager.cpp`
彻底重构 `restoreSplitState` 方法中的单窗格布局重置分支，移除提前 `return`：

```
<<<<<<< SEARCH
    if (state.panePaths.isEmpty()) {
        updateContainerMinimumWidth();
        refreshActiveIndicators();
        m_blockLayoutSignals = false;
        notifyLayoutChanged();
        return;
    }

    // 2. 如果序列化数据包含分屏树结构（isSplit == true），则按树层次还原；否则为单窗格
    if (state.isSplit && (state.rootNode.isSplitter || !state.rootNode.children.isEmpty())) {
        m_rootNode = importNodeRecursive(state.rootNode, m_panel);
        if (m_rootNode) {
            if (m_rootNode->splitter) {
                m_panel->m_mainLayout->addWidget(m_rootNode->splitter, 1);
                m_rootNode->splitter->show();
            } else if (m_rootNode->container) {
                m_panel->m_mainLayout->addWidget(m_rootNode->container, 1);
                m_rootNode->container->show();
            }
        }
    } else {
        // 兼容与普通单窗格/旧平铺配置恢复
        int primIdx = qBound(0, state.primaryIndex, state.panePaths.size() - 1);
        if (!state.panePaths.isEmpty()) {
            m_panel->loadDirectory(state.panePaths[primIdx]);
        }
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

        if (state.isSplit && state.panePaths.size() > 1) {
            for (int i = primIdx - 1; i >= 0; --i) {
                splitPane(state.orientation, state.panePaths[i], true);
            }
            for (int i = primIdx + 1; i < state.panePaths.size(); ++i) {
                splitPane(state.orientation, state.panePaths[i], false);
            }
        }
    }
=======
    // 2. 如果序列化数据包含分屏树结构（isSplit == true），则按树层次还原；否则为单窗格
    if (state.isSplit && (state.rootNode.isSplitter || !state.rootNode.children.isEmpty())) {
        m_rootNode = importNodeRecursive(state.rootNode, m_panel);
        if (m_rootNode) {
            if (m_rootNode->splitter) {
                m_panel->m_mainLayout->addWidget(m_rootNode->splitter, 1);
                m_rootNode->splitter->show();
            } else if (m_rootNode->container) {
                m_panel->m_mainLayout->addWidget(m_rootNode->container, 1);
                m_rootNode->container->show();
            }
        }
    } else {
        // 兼容与普通单窗格/新建标签页/旧平铺配置恢复
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

        if (!state.panePaths.isEmpty()) {
            int primIdx = qBound(0, state.primaryIndex, state.panePaths.size() - 1);
            m_panel->loadDirectory(state.panePaths[primIdx]);

            if (state.isSplit && state.panePaths.size() > 1) {
                for (int i = primIdx - 1; i >= 0; --i) {
                    splitPane(state.orientation, state.panePaths[i], true);
                }
                for (int i = primIdx + 1; i < state.panePaths.size(); ++i) {
                    splitPane(state.orientation, state.panePaths[i], false);
                }
            }
        }
    }
>>>>>>> REPLACE
```

### 2. `src/ui/PanelMediator.cpp`
在 `NavigationService::currentUrlChanged` 广播槽函数中，补充调用 `titleBar->tabBar()->updateCurrentTabTitle`：

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
1. **新建标签页加载验证**：运行 QuarkMeta，点击标题栏 “+” 按钮或按 `Ctrl+T`。验证新建标签页不仅顶部显示“此电脑”，并且主视口无死锁、无卡死，平滑加载并呈现“此电脑” (`computer://`) 驱动器网格。
2. **多窗格切新建标签页验证**：在一个 Tab 中开启双窗格分屏，然后点击 “+” 新建标签页。验证原双窗格被完全清理，新 Tab 正常呈现单窗格“此电脑”视角，切换回原 Tab 能够完美还原双窗格。
3. **导航双向同步验证**：在工作区双击进入任意子文件夹或点击侧边栏，验证顶部 Tab 标签页标题与图标实时同步更替。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- 严格复用 `TabBarWidget::updateCurrentTabTitle(displayPath, url)` 与 `ContentPanel::loadDirectory`，未另起炉灶。

---

## Header API Signature Verification
| 类名 / 接口 | 物理签名核查 (.h 源头) | 结论 |
| :--- | :--- | :--- |
| `TabBarWidget::updateCurrentTabTitle` | `void updateCurrentTabTitle(const QString& title, const QString& url);` | 100% 映射一致 |
| `NavigationService::currentUrlChanged` | `void currentUrlChanged(const QString& url, const QString& displayPath);` | 100% 映射一致 |
| `ContentPaneSplitManager::restoreSplitState` | `void restoreSplitState(const TabSplitState& state);` | 100% 映射一致 |

---

## Header Inclusion Chain & Type Completeness Check
- 类型定义完整，无头文件断裂隐患。
