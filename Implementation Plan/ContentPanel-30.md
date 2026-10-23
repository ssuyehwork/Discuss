# Implementation Plan - ContentPanel Full-Closed-Loop Cross-View Navigation & Path Sync

## 1. Overview（概述与解决的问题）

### 1.1 问题背景与深度审查结论
在先前编制的 `ContentPanel-29.md` 方案基础之上，经过对调用链、状态机生命周期以及全局导航联动体系的深度二次审查，定位到 3 个若不实施闭环治理就会导致**“顾此失彼、按下葫芦浮起瓢”**的深水区隐患：
1. **切入列视图时的“暴力重置与闪烁”隐患（过度重建）**：
   - 若切入列视图时无条件调用 `setRootPath(m_currentPath)`，由于 `setRootPath` 内部强制执行 `clearAllColumns()`，会导致原本在列视图中已经展开的多层级列栈与滚动位置被全部当场抹杀，每次切换都要从根目录重新走一遍硬盘扫描；
   - **闭环防守**：切入列视图前必须先做 `m_columnView->containsPath(m_currentPath)` 命中判定。若已包含当前路径（例如用户仅是临时切到网格视图看一眼缩略图后立即切回），完好保留既有展开列栈与滚动位置，**绝对禁止暴力清空已展开的列**，做到 0 销毁、0 闪烁！
2. **切出列视图后的“全局导航脱节”隐患（地址栏/Tab 标题失步）**：
   - 若离开列视图时仅在本地更新了 `m_currentPath` 并调用 `loadDirectory`，虽然内容区载入了新目录，但全局中介者 `NavigationService` 并未收到显式通知，会导致顶部的地址栏（`AddressBar`）、左侧的目录树（`NavPanel`）以及 Tab 标签页标题停留在旧路径上；
   - **闭环防守**：在更新路径后，显式发射 `emit directorySelected(m_currentPath)`，无缝驱动 `NavigationService::instance().navigateTo(m_currentPath)`，确保内容区、地址栏、目录树、Tab 标题及历史记录栈 100% 全局原子对齐！
3. **选区快照时序错位隐患（时序倒置）**：
   - 原代码中采集选区快照 `m_selectionState` 发生在函数入口第 565 行；若在后面才更新 `m_currentPath`，会导致快照中记录的 `currentFolder` 还是旧路径，从而在恢复选区时因路径不匹配造成选区丢失；
   - **闭环防守**：**必须在函数进入的第 0 毫秒**，在选区快照采集之前，先完成从列视图最右侧列权威路径的提取与同步，确保选区与目录 100% 匹配。

### 1.2 架构三问回答 (Architecture 3-Question Answers)
1. **SSOT 真理源溯源**：当前活跃文件夹路径由 `m_currentPath` 维持，在列视图模式下以最右侧列 `m_columnView->rightmostPane()->currentPath()` 为绝对真理源，切换瞬间原子反哺给全局；
2. **黑盒完整性**：所有状态流转与时序调度严格封装在 `ContentPanel::setViewMode` 内部，对外公开接口 `ContentPanel.h` 保持 100% 只读冻结；
3. **根因溯源**：彻底清除 `rowCount() == 0` 的错误阻断守卫，消除时序错位，建立“按需重建 + 全局通知”的闭环状态机。

---

## 2. Modified Files List（影响文件清单）

| 文件路径 | 修改类型 | 说明 |
| :--- | :--- | :--- |
| `src/ui/ContentPanel.cpp` | 逻辑闭环重构 | 彻底重构 `setViewMode` 的时序逻辑：第 0 毫秒路径提取、列视图包含性防重置检测、无条件载入数据及全局导航联动通知 |

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ContentPanel.cpp` — 视图切换全闭环状态机重构

```
<<<<<<< SEARCH
    // 1. 在原视图中上报并更新 SelectionState (SSOT)，修正临时对象迭代器野指针闪退
    m_selectionState.currentFolder = m_currentPath;
    QStringList selList = getSelectedPaths();
    m_selectionState.selectedPaths = QSet<QString>(selList.begin(), selList.end());
    if (!m_selectionState.selectedPaths.isEmpty()) {
        m_selectionState.focusedPath = *m_selectionState.selectedPaths.begin();
    }

    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_listCanvas);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            QString targetPath = !m_selectionState.focusedPath.isEmpty() ? m_selectionState.focusedPath : m_currentPath;
            m_columnView->setRootPath(targetPath);
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView);
        if (fjv) fjv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridCanvas);
    }

    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_diskModel || m_diskModel->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }
=======
    // 🚀【闭环防线 1：第 0 毫秒时序校正与权威路径提取】
    // 若原视图为列视图，在采集选区快照前，优先提取最右侧列（最新展开的真实目录）作为权威路径
    if (m_currentViewMode == ColumnView && m_columnView) {
        ColumnViewPane* rPane = m_columnView->rightmostPane();
        if (!rPane) rPane = m_columnView->activePane();
        if (rPane && !rPane->currentPath().isEmpty()) {
            m_currentPath = rPane->currentPath();
        }
    }

    // 1. 在原视图中上报并更新 SelectionState (SSOT)，修正临时对象迭代器野指针闪退
    m_selectionState.currentFolder = m_currentPath;
    QStringList selList = getSelectedPaths();
    m_selectionState.selectedPaths = QSet<QString>(selList.begin(), selList.end());
    if (!m_selectionState.selectedPaths.isEmpty()) {
        m_selectionState.focusedPath = *m_selectionState.selectedPaths.begin();
    }

    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_listCanvas);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            // 🚀【闭环防线 2：防过度重建与闪烁】
            // 仅在列视图尚未包含当前路径时才执行 setRootPath 构建新列栈；
            // 若已包含（例如切去网格后切回），完好保留既有展开列栈与滚动条位置，零销毁、零闪烁！
            if (!m_columnView->containsPath(m_currentPath)) {
                m_columnView->setRootPath(m_currentPath);
            }
            m_viewStack->setCurrentWidget(m_columnView);
            m_columnView->scrollToRightmostPane();
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView);
        if (fjv) fjv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridCanvas);
    }

    // 🚀【闭环防线 3：切出列视图时无条件载入数据，并驱动全局导航原子对齐】
    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty()) {
            loadDirectory(m_currentPath, m_isRecursive);
            emit directorySelected(m_currentPath);
        }
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译命令
```bash
cmake --build build --config Release
```

### 4.2 验证步骤
1. **验证列视图切回网格视图的数据与全局对齐**：
   - 处于列视图下，展开 `D:/Projects/QuarkMeta/src/ui`；
   - 点击切换到“网格视图”；
   - **核验标准**：
     - 网格视图立即呈现 `src/ui` 下的全部真实文件；
     - 顶部地址栏（AddressBar）文字正确显示为 `.../src/ui`；
     - 顶部 Tab 标签标题同步更新为 `ui`；
     - 左侧导航树高亮定位至 `ui` 节点。
2. **验证网格视图切回列视图的 0 闪烁与列状态保持**：
   - 在上一步完成的基础上，不移动目录，直接点击切回“列视图”；
   - **核验标准**：
     - 列视图瞬间呈现，**绝不发生**屏幕白闪或清空重新加载；
     - 原本展开的 `Projects -> QuarkMeta -> src -> ui` 多层列完好无损地保持在屏幕上；
     - 水平滚动条保持定位在最右侧 `ui` 列。
3. **验证跨目录切入列视图的正确展开**：
   - 在网格视图中导航至全新路径（例如 `C:/Windows/System32`）；
   - 点击切入“列视图”；
   - **核验标准**：列视图平滑从 `C:` 逐级展开至 `System32`，最右侧列为 `System32`。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（防另起炉灶自查）

| 检查项 | 结论 |
| :--- | :--- |
| 是否复用既有 SSOT 通道 | ✅ 严格复用 `emit directorySelected` 既有中介者广播通道与 `containsPath` |
| 是否彻底清除伪守卫死代码 | ✅ 彻底物理清除 `rowCount() == 0` 脑残阻断逻辑 |
| 是否破坏对外公开接口签名 | ❌ 严守【契约锁】，`ContentPanel.h` 100% 保持只读未改 |
| 是否引入平台 Hack | ❌ 纯基于标准 Qt 状态机与信号槽流转，零平台级侵入 |

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 类名 | 函数调用签名 | 核查状态 | 头文件物理位置 |
| :--- | :--- | :--- | :--- |
| `ColumnViewWidget` | `ColumnViewPane* rightmostPane() const` | ✅ 已验证 | `src/ui/ColumnViewWidget.h` L30 |
| `ColumnViewWidget` | `ColumnViewPane* activePane() const` | ✅ 已验证 | `src/ui/ColumnViewWidget.h` L29 |
| `ColumnViewWidget` | `bool containsPath(const QString& path) const` | ✅ 已验证 | `src/ui/ColumnViewWidget.h` L34 |
| `ColumnViewWidget` | `void setRootPath(const QString& path)` | ✅ 已验证 | `src/ui/ColumnViewWidget.h` L26 |
| `ColumnViewWidget` | `void scrollToRightmostPane()` | ✅ 已验证 | `src/ui/ColumnViewWidget.h` L39 |
| `ColumnViewPane` | `QString currentPath() const` | ✅ 已验证 | `src/ui/ColumnViewPane.h` L23 |
| `ContentPanel` | `void loadDirectory(const QString& path, bool recursive = false)` | ✅ 已验证 | `src/ui/ContentPanel.h` L82 |
| `ContentPanel` | `void directorySelected(const QString& path)` (Signal) | ✅ 已验证 | `src/ui/ContentPanel.h` L176 |
