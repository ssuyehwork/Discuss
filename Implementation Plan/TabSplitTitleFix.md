# Implementation Plan - TabSplitTitleFix (分屏标签页标题全量保持与增量更新修复方案)

## Chapter 1: Overview（概述与根因分析）

### 1. 现象与问题分析
用户反馈在多窗格分屏标签页（如合并了 `Dual-mode version` 与 `resources` 两个窗格）场景下：
未激活标签页显示完整的合并标题 `Dual-mode version | resources`（图①）；但当点击激活该标签页后，标题突然变更为 `Dual-mode version | `，未激活的副窗格名称被意外擦除/丢弃（图②）。

### 2. 根因剖析 (Root Cause)
经过对 `src/ui/TabBarWidget.cpp` 源码的深度排查：
1. **未校验分屏状态全量覆盖**：当用户点击切换标签页时，`PanelMediator` 会向当前活动窗格发起导航，并同步调用 `TabBarWidget::updateCurrentTabTitle(displayPath, url)`。原 `updateCurrentTabTitle` 逻辑在接收到导航目录变更时，**完全没有校验当前标签页是否处于分屏模式（`splitState.isSplit`）**，直接将 `m_tabs[m_currentIndex].title` 赋值为单窗格目录名（`folderName`），硬生生覆盖冲掉了原本包含副窗格的合并标题（`Dual-mode version | resources`）。
2. **分屏路径列表未做空值过滤**：在 `updateSplitTabTitle` 中，遍历 `state.panePaths` 拼装 `nameList` 时，未拦截初始化或恢复过程中的空路径（`""`），导致 `cleanNameForTab("")` 拼出了多余的末尾 ` | ` 分隔符。

### 3. 重构设计与解决目标
1. **分屏状态智能感知与增量更新**：在 `updateCurrentTabTitle` 中引入 `splitState.isSplit` 校验：
   - 若当前标签页为分屏模式，精准更新 `splitState.panePaths` 中对应活动窗格（`activePaneIndex`）的最新路径；
   - 重新根据所有非空窗格路径重新拼装合并标题（格式为 `窗格1 | 窗格2`），绝对不再使用单窗格名称覆盖冲掉其他窗格名称；
2. **空路径防错过滤**：在 `updateSplitTabTitle` 与 `updateCurrentTabTitle` 的标题拼装循环中，显式跳过 `p.isEmpty()` 的空路径，确保分屏标题始终格式化为干净且完整的 `窗格1 | 窗格2`。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/TabBarWidget.cpp`
2. `Memories.md`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/TabBarWidget.cpp`

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
            if (!p.isEmpty()) {
                nameList.append(cleanNameForTab(p));
            }
        }
        if (!nameList.isEmpty()) {
            QString mergedTitle = nameList.join(" | ");
            m_tabs[m_currentIndex].title = mergedTitle;
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
            if (!p.isEmpty()) {
                nameList.append(cleanNameForTab(p));
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

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 编译命令
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### 2. 运行时验收标准
1. 启动 QuarkMeta。
2. 创建或切换至包含 2 个分屏窗格（例如 `Dual-mode version` 与 `resources`）的标签页。
3. 切换至其他标签页，观察标签页标题显示为 `Dual-mode version | resources`。
4. 点击该标签页重新激活它。
5. **期望结果 1**：激活后标签页标题稳定保持为 `Dual-mode version | resources`，副窗格名称 ` resources` 不会丢弃或抹除。
6. **期望结果 2**：标题文本中不会产生末尾无意义的 ` | ` 分隔符。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 `TabBarWidget::cleanNameForTab` 以及现有的 `TabSplitState` 窗格路径数据结构，不添加 redundant 造轮子接口。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生/视图副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 标签页分屏窗格路径 | `TabInfo::splitState.panePaths` | `TabInfo::title` (格式化 `A \| B`) | `updateCurrentTabTitle` → 增量拼装更新 `title` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- `updateSplitTabTitle(const TabSplitState& state)`：公开接口签名保持 100% 不变；
- `updateCurrentTabTitle(const QString& title, const QString& url)`：公开接口签名保持 100% 不变。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 仅在 `TabBarWidget.cpp` 内增加逻辑分支判定，不涉及任何头文件包含变动。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否物理阅读了涉及的 `.h`/`.cpp` 文件？ | 是 | `TabBarWidget.h`, `TabBarWidget.cpp` |
| 2 | SEARCH 块是否在目标文件中真实且唯一存在？ | 是 | 已通过验证匹配 |
| 3 | 调用的外部成员函数是否在 `.h` 中真实存在？ | 是 | `cleanNameForTab` 等内部静态函数 |
| 4 | 新增/重构接口的声明定义调用点是否一致？ | 是 | 签名未改变 |
| 5 | 头文件包含链改动后 `.cpp` 类型是否闭合？ | 是 | 类型均闭合 |
| 6 | 新建/删除文件是否在 `CMakeLists.txt` 中注册？ | 是 | 未增删文件 |
| 7 | 跨类调用的成员访问级别是否允许？ | 是 | 内部私有/公有方法 |
| 8 | 是否未触及方案范围外的文件？ | 是 | 仅针对 `TabBarWidget` 与 `Memories` |
| 9 | 方案中是否有未解答的假设或模糊需求？ | 否 | 需求与根因清晰 |
| 10 | 是否无内联 QSS 样式硬编码违规？ | 是 | 不涉及 QSS |
| 11 | 是否完整包含 10 大必备章节？ | 是 | 完整包含 10 大章节 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 修改点 | 既有行为/数值 | 新行为/数值 | 是否零篡改/符合用户共识 |
| :--- | :--- | :--- | :--- |
| 单窗格标签页导航 | 更新单目录名称为标题 | 保持完全相同 | 100% 行为一致 |
| 分屏标签页导航 | 错擦除副窗格，退化为单名称 | 保留多窗格路径，增量组装 `A \| B` 标题 | 修复缺陷，符合用户共识 |
