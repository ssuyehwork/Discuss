# QuarkMeta “库”虚拟分类与智能文件夹地址栏名称同步治理实施方案

## 1. 概述与核心变更目标
在用户选中侧边栏“库”（LibraryPanel）中的智能文件夹或虚拟分类（如“其他”、“图形”等）时，内容面板（`ContentPanel`）标头能够正常呈现分类标题，但顶部地址栏（`AddressBar`）在窗格激活切换或导航更新时，会错误地被覆盖显示为底层物理磁盘路径或旧数据路径，导致地址栏呈现与当前窗格展示内容严重脱节。

**核心目标：**
1. 恪守 `Memories.md` 顶层规范：在查看“库”分类或智能文件夹等虚拟分类数据时，地址栏与标题栏必须统一呈现逻辑分类/智能文件夹名称（如 `库 → 其他`）。
2. 在 `ContentPanel` 中提供统一的 `displayPath()` 接口，根据当前分类类型（`library` 或 `smart_folder`）优先返回逻辑分类路径与标题，替代直接返回物理磁盘路径的 `currentPath()`。
3. 在 `PanelMediator` 的窗格激活回调（`panelActivated`）以及地址栏路径同步逻辑中，统一调用 `activePanel->displayPath()`，彻底消除窗格切换与激活时地址栏被旧磁盘路径覆写退化的缺陷。

---

## 2. 涉及的影响范围与受影响组件

### 2.1 修改文件列表
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/PanelMediator.cpp`

---

## 3. 详细代码变更说明 (SEARCH / REPLACE 差异对比)

### 3.1 `src/ui/ContentPanel.h`

```cpp
<<<<<<< SEARCH
    QString currentCategoryTitle() const { return m_categoryTitle; }
=======
    QString currentCategoryTitle() const { return m_categoryTitle; }
    QString displayPath() const;
>>>>>>> REPLACE
```

### 3.2 `src/ui/ContentPanel.cpp`

```cpp
<<<<<<< SEARCH
void ContentPanel::setCurrentCategoryTitle(const QString& title) {
    m_categoryTitle = title;
    updateHeaderTitle();
}
=======
void ContentPanel::setCurrentCategoryTitle(const QString& title) {
    m_categoryTitle = title;
    updateHeaderTitle();
}

QString ContentPanel::displayPath() const {
    if (!m_categoryTitle.isEmpty() && (m_currentCategoryType == "library" || m_currentCategoryType == "smart_folder")) {
        return "库 → " + m_categoryTitle;
    }
    return m_currentPath;
}
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
void ContentPanel::setCurrentPath(const QString& path) {
    m_currentPath = path;
=======
void ContentPanel::setCurrentPath(const QString& path) {
    m_currentPath = path;
    m_currentCategoryType.clear();
    m_categoryTitle.clear();
>>>>>>> REPLACE
```

### 3.3 `src/ui/PanelMediator.cpp`

```cpp
<<<<<<< SEARCH
        connect(panel, &ContentPanel::panelActivated, this, [this, addressBar](ContentPanel* activePanel) {
            if (m_activeContentPanel != activePanel) {
                m_activeContentPanel = activePanel;
                emit activeContentPanelChanged(activePanel);
            }
            if (addressBar) {
                addressBar->setPath(activePanel->currentPath());
            }
        });
=======
        connect(panel, &ContentPanel::panelActivated, this, [this, addressBar](ContentPanel* activePanel) {
            if (m_activeContentPanel != activePanel) {
                m_activeContentPanel = activePanel;
                emit activeContentPanelChanged(activePanel);
            }
            if (addressBar) {
                addressBar->setPath(activePanel->displayPath());
            }
        });
>>>>>>> REPLACE
```

---

## 4. 架构设计与 SSOT 合规性分析
1. **单一真理源 (SSOT)**：`ContentPanel` 本身是视图与数据的载体，将分类显示标题与路径统一封装于 `ContentPanel::displayPath()`，避免控制层 `PanelMediator` 在多处重复硬编码 `"库 → " + name` 的字符串拼接，提升高内聚性。
2. **多窗格/分屏完美兼容**：通过 `displayPath()` 替代 `currentPath()`，在用户切换不同分屏窗格（`panelActivated`）时，能够精准恢复激活窗格真实的逻辑显示路径（无论是物理路径还是库分类路径）。

---

## 5. 迁移与兼容性评估
改动完全向下兼容，针对非库分类的普通磁盘路径，`displayPath()` 自动平滑退回返回 `m_currentPath`，对现有磁盘导航、收藏夹及历史记录无任何破坏性影响。

---

## 6. 性能影响评估
仅涉及简单的字符串类型判断与条件返回，逻辑开销在微秒（µs）级别，CPU 与内存开销为 0。

---

## 7. 安全与风险防护
所有状态改动均在主 UI 线程内同步进行，不涉及多线程并发竞争，无任何死锁或内存越界风险。

---

## 8. 自动化测试与验证计划
1. **交互验证**：
   - 打开侧边栏“库”面板，点击智能文件夹或分类（如“其他”）。
   - 检查顶部地址栏是否即时同步显示为 `库 → 其他`。
   - 在多窗格分屏模式下，在普通磁盘目录窗格与“库”分类窗格之间来回切换焦点，检查地址栏是否能够毫秒级无缝切换并精准呈现各自对应的路径/标题。
2. **退回验证**：
   - 从“库”分类点击左侧“目录导航”或“此电脑”进入普通磁盘文件夹，检查地址栏与 `m_categoryTitle` 是否正常重置恢复显示物理磁盘路径（如 `D:\Downloads`）。

---

## 9. 回滚策略
若需要回滚，只需撤销相关修改提交即可。

---

## 10. 实施步骤与预估工时
1. 撰写并提交实施方案。
2. 依据方案修改 `ContentPanel` 与 `PanelMediator` 并进行测试验证。
