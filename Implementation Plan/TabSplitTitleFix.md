# Implementation Plan - TabSplitTitleFix (分屏标签页标题全量保持、空路径拆分防错与宽度自适应修复方案)

## Chapter 1: Overview（概述与根因分析）

### 1. 现象与问题分析
用户反馈在多窗格分屏标签页（如合并了 `Dual-mode version` 与 `resources` 两个窗格）场景下：
1. 标签页激活或切换后，标题尾部拼接出了多余的 ` | ` 分隔符（呈现为 `resources | `）。
2. 在标题栏标签页未满、空间充裕的情况下，激活状态下的标签页依然会出现 `...` 省略号截断现象。

### 2. 根因剖析 (Root Cause)
经过对 `src/ui/TabBarWidget.cpp` 与 `src/ui/controllers/ContentPaneSplitManager.cpp` 源码的深度排查，确认存在以下三重根因与违规点：
1. **导出分屏状态未过滤空/无效路径**：`ContentPaneSplitManager::exportSplitState()` 在遍历所有窗格 (`allPanes`) 导出路径时，直接调用 `state.panePaths.append(p ? p->currentPath() : "")`。在窗格初始化或尚未完成目录装载前，`p->currentPath()` 返回空字符串 `""`，导致 `state.panePaths` 混入了空路径条目。
2. **标题拼装未做 Blank 校验生成悬空 `|`**：`TabBarWidget::updateSplitTabTitle` 与 `updateCurrentTabTitle` 在遍历 `state.panePaths` 执行 `cleanNameForTab` 时，没有对空/空白字符串进行过滤，导致 `nameList` 组装出了 `["resources", ""]`，经 `nameList.join(" | ")` 拼接后直接输出了带末尾悬空分隔符的错乱标题 `resources | `。
3. **人工硬编码 300px 宽度上限引发误截断**：在 `TabBarWidget::applyDiffUpdate()` 中，计算标签自然所需宽度时使用 `int needed = qBound(120, textW + 64, 300);` 硬编码了 `300px` 的上限约束。当标签页处于分屏模式或激活加粗字体时，`textW + 64` 很容易突破 300px。即使标题栏剩余大量空白空间，标签按钮宽度仍被死死限制在 300px，导致 `TabItemButton::updateElidedTitle()` 在可利用空间充裕时依然触发 `fm.elidedText(...)` 绘制 `...` 省略号。这直接违反了 `Memories.md` 第 19.1 节第 3 条“空间充裕时必须 100% 完整展示标题名称，绝对严禁人为固定低宽度上限而触发 `...` 省略号截断”的红线契约！

### 3. 重构设计与解决目标
1. **源头过滤空路径与智能分屏判定**：在 `ContentPaneSplitManager::exportSplitState()` 中仅收集非空路径；若有效非空路径数小于 2，自动将 `state.isSplit` 置为 `false`。
2. **标题格式化全流程空白清洗**：在 `TabBarWidget::updateSplitTabTitle` 与 `updateCurrentTabTitle` 中对 `panePaths` 的每个路径项进行 `p.trimmed().isEmpty()` 校验，确保 `nameList` 只包含有效的目录名称，从根本上杜绝末尾悬空 ` | ` 的产生。
3. **解锁 600px 宽度上限，实现空间充裕时零截断**：将 `applyDiffUpdate()` 中的宽度上限提升对齐至 `TabItemButton` 的 `maximumWidth` (`600px`)：`int needed = qBound(120, textW + 64, 600);`。在标题栏空间未满时，允许标签页根据完整标题长短自然伸展，消除人工 300px 锁死的 `...` 截断。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/controllers/ContentPaneSplitManager.cpp`
2. `src/ui/TabBarWidget.cpp`
3. `Memories.md`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/controllers/ContentPaneSplitManager.cpp`

```
<<<<<<< SEARCH
    state.isSplit = (allPanes.size() > 1);
    state.orientation = splitOrientation();

    for (int i = 0; i < allPanes.size(); ++i) {
        ContentPanel* p = allPanes[i];
        state.panePaths.append(p ? p->currentPath() : "");
        if (p == m_panel) {
            state.primaryIndex = i;
        }
        if (m_activePaneForSplit == p || (m_activePaneForSplit == nullptr && p == m_panel)) {
            state.activePaneIndex = i;
        }
    }
=======
    state.orientation = splitOrientation();

    for (int i = 0; i < allPanes.size(); ++i) {
        ContentPanel* p = allPanes[i];
        QString path = p ? p->currentPath().trimmed() : "";
        if (!path.isEmpty()) {
            state.panePaths.append(path);
        }
        if (p == m_panel) {
            state.primaryIndex = state.panePaths.isEmpty() ? 0 : state.panePaths.size() - 1;
        }
        if (m_activePaneForSplit == p || (m_activePaneForSplit == nullptr && p == m_panel)) {
            state.activePaneIndex = state.panePaths.isEmpty() ? 0 : state.panePaths.size() - 1;
        }
    }
    state.isSplit = (state.panePaths.size() > 1);
>>>>>>> REPLACE
```

### 2. `src/ui/TabBarWidget.cpp`

```
<<<<<<< SEARCH
void TabBarWidget::updateSplitTabTitle(const TabSplitState& state) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    m_tabs[m_currentIndex].splitState = state;

    if (state.isSplit && !state.panePaths.isEmpty()) {
        QStringList nameList;
        for (const QString& p : state.panePaths) {
            nameList.append(cleanNameForTab(p));
        }
        QString mergedTitle = nameList.join(" | ");
        m_tabs[m_currentIndex].title = mergedTitle;
        int primIdx = qBound(0, state.primaryIndex, state.panePaths.size() - 1);
        m_tabs[m_currentIndex].url = state.panePaths[primIdx];
    } else if (!state.panePaths.isEmpty()) {
        m_tabs[m_currentIndex].title = cleanNameForTab(state.panePaths.first());
        m_tabs[m_currentIndex].url = state.panePaths.first();
    }

    if (m_currentIndex < m_tabWidgets.size()) {
        m_tabWidgets[m_currentIndex]->setTabTitle(m_tabs[m_currentIndex].title);
    }
    saveStateToConfig();
}
=======
void TabBarWidget::updateSplitTabTitle(const TabSplitState& state) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    m_tabs[m_currentIndex].splitState = state;

    if (state.isSplit && !state.panePaths.isEmpty()) {
        QStringList nameList;
        for (const QString& p : state.panePaths) {
            QString trimmedP = p.trimmed();
            if (!trimmedP.isEmpty()) {
                QString name = cleanNameForTab(trimmedP);
                if (!name.trimmed().isEmpty()) {
                    nameList.append(name);
                }
            }
        }
        if (!nameList.isEmpty()) {
            m_tabs[m_currentIndex].title = nameList.join(" | ");
        }
        int primIdx = qBound(0, state.primaryIndex, state.panePaths.size() - 1);
        m_tabs[m_currentIndex].url = state.panePaths[primIdx];
    } else if (!state.panePaths.isEmpty()) {
        m_tabs[m_currentIndex].title = cleanNameForTab(state.panePaths.first());
        m_tabs[m_currentIndex].url = state.panePaths.first();
    }

    if (m_currentIndex < m_tabWidgets.size()) {
        m_tabWidgets[m_currentIndex]->setTabTitle(m_tabs[m_currentIndex].title);
    }
    saveStateToConfig();
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void TabBarWidget::updateCurrentTabTitle(const QString& title, const QString& url) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    QString folderName = title;
    if (url == "computer://" || url.isEmpty()) {
        folderName = "此电脑";
    } else if (url.startsWith("library://") || url.startsWith("smartfolder://")) {
        folderName = title;
    } else if (title.contains("/") || title.contains("\\")) {
        QString cleanPath = QDir::cleanPath(url);
        QFileInfo fi(cleanPath);
        folderName = fi.fileName();
        if (folderName.isEmpty()) {
            folderName = cleanPath;
        }
    }

    m_tabs[m_currentIndex].title = folderName.isEmpty() ? "此电脑" : folderName;
    m_tabs[m_currentIndex].url = url;

    applyDiffUpdate();
    saveStateToConfig();
}
=======
void TabBarWidget::updateCurrentTabTitle(const QString& title, const QString& url) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    QString folderName = title;
    if (url == "computer://" || url.isEmpty()) {
        folderName = "此电脑";
    } else if (url.startsWith("library://") || url.startsWith("smartfolder://")) {
        folderName = title;
    } else if (title.contains("/") || title.contains("\\")) {
        QString cleanPath = QDir::cleanPath(url);
        QFileInfo fi(cleanPath);
        folderName = fi.fileName();
        if (folderName.isEmpty()) {
            folderName = cleanPath;
        }
    }

    if (folderName.isEmpty()) folderName = "此电脑";

    auto& currentTab = m_tabs[m_currentIndex];
    if (currentTab.splitState.isSplit && !currentTab.splitState.panePaths.isEmpty()) {
        int activeIdx = qBound(0, currentTab.splitState.activePaneIndex, currentTab.splitState.panePaths.size() - 1);
        currentTab.splitState.panePaths[activeIdx] = url;

        QStringList nameList;
        for (const QString& p : currentTab.splitState.panePaths) {
            QString trimmedP = p.trimmed();
            if (!trimmedP.isEmpty()) {
                QString name = cleanNameForTab(trimmedP);
                if (!name.trimmed().isEmpty()) {
                    nameList.append(name);
                }
            }
        }
        if (!nameList.isEmpty()) {
            currentTab.title = nameList.join(" | ");
        } else {
            currentTab.title = folderName;
        }
        currentTab.url = url;
    } else {
        currentTab.title = folderName;
        currentTab.url = url;
    }

    applyDiffUpdate();
    saveStateToConfig();
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QVector<int> naturalWidths(targetCount);
    int totalNaturalWidth = 0;
    for (int i = 0; i < targetCount; ++i) {
        QFontMetrics fm(m_tabWidgets[i]->font());
        int textW = fm.horizontalAdvance(m_tabs[i].title);
        int needed = qBound(120, textW + 64, 300);
        naturalWidths[i] = needed;
        totalNaturalWidth += needed;
    }
=======
    QVector<int> naturalWidths(targetCount);
    int totalNaturalWidth = 0;
    for (int i = 0; i < targetCount; ++i) {
        QFontMetrics fm(m_tabWidgets[i]->font());
        int textW = fm.horizontalAdvance(m_tabs[i].title);
        int needed = qBound(120, textW + 64, 600);
        naturalWidths[i] = needed;
        totalNaturalWidth += needed;
    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 编译命令
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### 2. 运行时验收标准
1. 启动 QuarkMeta。
2. 创建分屏标签页（如包含 `Dual-mode version` 与 `resources`）。
3. **期望结果 1**：在标题栏空间未满时，标签页根据完整名称自然伸展（可突破 300px 直至 600px），完整显示 `Dual-mode version | resources`，**无任何 `...` 省略号**。
4. 点击该标签页进行激活与操作。
5. **期望结果 2**：绝对不会出现悬空末尾 ` | `（如 `resources | `），标题始终为规范干净的组合名称。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 `TabBarWidget::cleanNameForTab` 与 `QString::trimmed` 过滤，解锁 `applyDiffUpdate()` 中对齐 `TabItemButton::maximumWidth` 的 600px 伸展限制，完全杜绝造轮子。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生/视图副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 标签页分屏窗格路径 | `TabInfo::splitState.panePaths` | `TabInfo::title` (格式化 `A \| B`) | `ContentPaneSplitManager::exportSplitState` 过滤空值 → `updateCurrentTabTitle` 增量更新 |
| 标签页自然计算宽度 | `naturalWidths[i]` | `tabBtn->setFixedWidth` | `applyDiffUpdate` 空间充裕时 100% 渲染自然宽度 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- `exportSplitState()`：公开接口签名保持 100% 不变；
- `updateSplitTabTitle(const TabSplitState& state)`：公开接口签名保持 100% 不变；
- `updateCurrentTabTitle(const QString& title, const QString& url)`：公开接口签名保持 100% 不变；
- `applyDiffUpdate()`：内部私有绘制计算逻辑。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 仅在 `ContentPaneSplitManager.cpp` 与 `TabBarWidget.cpp` 内更新逻辑，不涉及任何头文件包含变动。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否物理阅读了涉及的 `.h`/`.cpp` 文件？ | 是 | `ContentPaneSplitManager.cpp`, `TabBarWidget.cpp` |
| 2 | SEARCH 块是否在目标文件中真实且唯一存在？ | 是 | 已通过验证匹配 |
| 3 | 调用的外部成员函数是否在 `.h` 中真实存在？ | 是 | `cleanNameForTab` 等内部静态函数 |
| 4 | 新增/重构接口的声明定义调用点是否一致？ | 是 | 签名未改变 |
| 5 | 头文件包含链改动后 `.cpp` 类型是否闭合？ | 是 | 类型均闭合 |
| 6 | 新建/删除文件是否在 `CMakeLists.txt` 中注册？ | 是 | 未增删文件 |
| 7 | 跨类调用的成员访问级别是否允许？ | 是 | 内部私有/公有方法 |
| 8 | 是否未触及方案范围外的文件？ | 是 | 仅针对目标文件与 `Memories` |
| 9 | 方案中是否有未解答的假设或模糊需求？ | 否 | 需求与根因清晰 |
| 10 | 是否无内联 QSS 样式硬编码违规？ | 是 | 不涉及 QSS |
| 11 | 是否完整包含 10 大必备章节？ | 是 | 完整包含 10 大章节 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 修改点 | 既有行为/数值 | 新行为/数值 | 是否零篡改/符合用户共识 |
| :--- | :--- | :--- | :--- |
| 标签页计算宽度上限 | 硬编码人工 300px 锁死 | 解锁至 600px（对齐 TabItemButton 物理上限） | 100% 遵从 Memories 19.1 条款 3 零截断规范 |
| 分屏空路径过滤 | 裸追加 `""` 导致悬空 `|` | 严格过滤 `trimmed().isEmpty()` 空值 | 彻底根治末尾 `|` |
