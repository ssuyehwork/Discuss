# Implementation Plan - ContentPanel Cross-ViewMode Active Folder Sync & Restoration

## 1. Overview（概述与解决的问题）

### 1.1 问题背景与根因定位
在当前版本的四大内容视图（自适应视图 JustifiedView、网格视图 GridView、列表视图 ListView 与列视图 ColumnView）切换过程中，存在**严重的“最后一次打开的文件夹路径”失步脱节 Bug**：
1. **从【列视图】切回【网格/列表/自适应视图】时，最新浏览路径完全丢失**：
   - 用户在列视图（Miller Columns）中双击子文件夹展开至深层目录（如 `D:/A/B/C/New`），当前真正活跃的目录是最右侧列 `m_columnView->rightmostPane()->currentPath()`；
   - 但在 [`ContentPanel::setViewMode`](file:///g:/C++/QuarkMeta/QuarkMeta/src/ui/ContentPanel.cpp#L560) 中，完全没有从 `m_columnView` 提取这个最新路径更新给 `m_currentPath`；
   - 更加致命的是，代码在第 595 行加入了一道自作聪明的阻断守卫：
     ```cpp
     if (!m_diskModel || m_diskModel->rowCount() == 0) {
         loadDirectory(m_currentPath, m_isRecursive);
     }
     ```
     只要在切入列视图之前，网格视图曾加载过任何旧数据（`rowCount() > 0`），该条件**永远为 false**，直接将 `loadDirectory` 强行阻断！切回网格后直接展示远古旧文件夹的内容，列视图打开的新目录彻底被丢弃。
2. **从【网格/列表/自适应视图】切到【列视图】时，焦点项篡改了真实路径**：
   - 代码使用 `QString targetPath = !m_selectionState.focusedPath.isEmpty() ? m_selectionState.focusedPath : m_currentPath;`；
   - 若用户在网格视图中仅仅鼠标单击选中了一个子文件夹（未双击进入），`focusedPath` 就变成了该子文件夹；
   - 列视图在 `setRootPath` 时自作主张把该子文件夹展开成了最右侧列，破坏了当前正在浏览的实际文件夹真理源（SSOT）。

### 1.2 架构契约溯源 (Memories.md SSOT)
根目录下 [`Memories.md`](file:///g:/C++/QuarkMeta/QuarkMeta/Memories.md#L261-L264) 第 9 章第 8 条《分栏视图与主视图模式切换数据模型无缝同步契约》已作为全系统最高真理源明确约定：
> **“当用户从分栏视图切换至网格、列表或瀑布流视图时，`ContentPanel::setViewMode` 必须精准提取 `m_columnView` 最右侧列/激活列（`rightmostPane()` / `activePane()`）的最新实际文件夹路径更新至 `m_currentPath`，并触发自愈重载（`loadDirectory(m_currentPath)`），保障切换回其他视图时深层目录真实数据项 0 毫秒同步呈现，彻底消除弹回旧路径或‘显示无项目’的虚假空状态缺陷。”**

### 1.3 重构目标
严格落实 Memories.md 架构契约：
1. **离开 ColumnView 时**：精准提取最右侧列/激活列真实路径，同步更新 `m_currentPath`，彻底物理删除 `rowCount() == 0` 的脑残守卫，无条件调用 `loadDirectory(m_currentPath, m_isRecursive)` 载入最新数据；
2. **进入 ColumnView 时**：绝对以当前视图真正打开的文件夹 `m_currentPath` 为真理源构建祖先列栈，并在最右侧列中按需高亮定位 `focusedPath`，绝不篡改主目录结构。

---

## 2. Modified Files List（影响文件清单）

| 文件路径 | 修改类型 | 说明 |
| :--- | :--- | :--- |
| `src/ui/ContentPanel.cpp` | 逻辑修复 | 修复 `setViewMode` 中列视图与其他视图之间双向文件夹路径同步与强制重载逻辑 |

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ContentPanel.cpp` — 修复 `setViewMode` 跨视图路径同步与数据原子重载

```
<<<<<<< SEARCH
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
    // 🚀【SSOT 路径真理源同步】：离开列视图时，优先提取列视图最右侧列/激活列真实展开的最新目录
    if (oldMode == ColumnView && m_columnView) {
        ColumnViewPane* rPane = m_columnView->rightmostPane();
        if (!rPane) rPane = m_columnView->activePane();
        if (rPane && !rPane->currentPath().isEmpty()) {
            m_currentPath = rPane->currentPath();
            m_selectionState.currentFolder = m_currentPath;
        }
    }

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_listCanvas);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            // 🚀【SSOT 契约】：绝对以当前视图真正打开的文件夹 m_currentPath 为终点构建祖先列栈
            m_columnView->setRootPath(m_currentPath);
            if (!m_selectionState.focusedPath.isEmpty() && m_columnView->rightmostPane()) {
                m_columnView->rightmostPane()->selectItemByPath(m_selectionState.focusedPath);
            }
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView);
        if (fjv) fjv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridCanvas);
    }

    // 🚀【彻底清除脑残守卫】：从列视图切回其他视图时，无条件原子级载入列视图最新目录，坚决杜绝旧数据残留
    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty()) {
            loadDirectory(m_currentPath, m_isRecursive);
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
1. **测试从列视图 -> 网格/列表视图无缝同步**：
   - 切换至列视图（ColumnView）；
   - 在列视图中连续双击子文件夹深入 3~5 层（例如从 `D:/` 深入到 `D:/Projects/QuarkMeta/src/ui`）；
   - 点击顶部视图切换按钮切回“网格视图”或“列表视图”；
   - **验证点**：网格/列表视图立即且准确展示 `D:/Projects/QuarkMeta/src/ui` 下的所有真实文件与文件夹，绝不弹回根目录或显示旧目录。
2. **测试从网格/列表视图 -> 列视图无缝同步**：
   - 在网格视图中双击进入某个文件夹（例如 `D:/Downloads`）；
   - 单击选中其中的某一个子项（例如 `test.zip`）；
   - 点击视图切换按钮切至“列视图”；
   - **验证点**：列视图最右侧列精准停留在 `D:/Downloads`，且最右侧列中 `test.zip` 项被正确高亮选中，绝不会错误地把子文件夹当作新列展开。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（防另起炉灶自查）

| 检查项 | 结论 |
| :--- | :--- |
| 是否复用既有 SSOT 通道 | ✅ 严格复用既有 `loadDirectory()` 官方通道与 `m_columnView->rightmostPane()` |
| 是否彻底清除伪守卫死代码 | ✅ 彻底物理清除 `!m_diskModel || m_diskModel->rowCount() == 0` 脑残阻断逻辑 |
| 是否破坏对外公开接口签名 | ❌ 严守【契约锁】，`ContentPanel.h` 100% 保持只读未改 |
| 是否引入平台 Hack | ❌ 纯基于标准 Qt 状态机流转与数据加载，零平台级侵入 |

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 类名 | 函数调用签名 | 核查状态 | 头文件物理位置 |
| :--- | :--- | :--- | :--- |
| `ColumnViewWidget` | `ColumnViewPane* rightmostPane() const` | ✅ 已验证 | `src/ui/ColumnViewWidget.h` L30 |
| `ColumnViewWidget` | `ColumnViewPane* activePane() const` | ✅ 已验证 | `src/ui/ColumnViewWidget.h` L29 |
| `ColumnViewWidget` | `void setRootPath(const QString& path)` | ✅ 已验证 | `src/ui/ColumnViewWidget.h` L26 |
| `ColumnViewPane` | `QString currentPath() const` | ✅ 已验证 | `src/ui/ColumnViewPane.h` L23 |
| `ColumnViewPane` | `void selectItemByPath(const QString& targetPath)` | ✅ 已验证 | `src/ui/ColumnViewPane.h` L29 |
| `ContentPanel` | `void loadDirectory(const QString& path, bool recursive = false)` | ✅ 已验证 | `src/ui/ContentPanel.h` L82 |
