# QuarkMeta 跨窗格拖拽到分类/智能文件夹语义判定与“未分类”兜底入库实施方案

## 1. 概述与核心变更目标
在多窗格（分屏）交互场景下，当用户将文件/文件夹从一个窗格拖拽到另一个窗格时，系统需要根据**目标窗格当前的视图形态（普通磁盘目录 vs 库普通分类 vs 智能文件夹）**精准判定并执行不同的拖拽语义：

**核心目标：**
1. **目标窗格为“普通分类”（如“库”中的自定义分类或“未分类”）**：拖拽判定为**“入库/归类”**，不改变物理磁盘存储路径，仅将拖入文件/文件夹与目标普通分类建立数据库与侧车关联。
2. **目标窗格为“智能文件夹”或“全部数据”等只读/规则锁定分类**：由于智能文件夹基于规则动态匹配、严禁强行手动绑定，当用户不慎将项目拖拽到智能文件夹视图区域时，系统默认执行入库归属于**“未分类”**（`categoryId = -2`），并通过 `ToolTipOverlay` 弹出友情提示：`项目已入库到“未分类”`。
3. **目标窗格为“普通磁盘文件夹”**：拖拽判定为**“迁移”**，维持原有磁盘 I/O 物理复制/移动逻辑。

---

## 2. 涉及的影响范围与受影响组件

### 2.1 修改文件列表
- `src/ui/LibraryPanel.h`
- `src/ui/LibraryPanel.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/PanelMediator.cpp`
- `src/ui/controllers/ContentFileOpsHandler.cpp`

---

## 3. 详细代码变更说明 (SEARCH / REPLACE 差异对比)

### 3.1 `src/ui/LibraryPanel.h`

```cpp
<<<<<<< SEARCH
signals:
    void categoryPathsSelected(const QStringList& paths, const QString& name = QString());
=======
signals:
    void categoryPathsSelected(const QStringList& paths, const QString& name = QString(), int categoryId = 0);
>>>>>>> REPLACE
```

### 3.2 `src/ui/LibraryPanel.cpp`

```cpp
<<<<<<< SEARCH
    QStringList paths = LibraryService::instance().getCategoryPaths(nodeId);
    emit categoryPathsSelected(paths, nodeName);
=======
    QStringList paths = LibraryService::instance().getCategoryPaths(nodeId);
    emit categoryPathsSelected(paths, nodeName, nodeId);
>>>>>>> REPLACE
```

### 3.3 `src/ui/ContentPanel.h`

```cpp
<<<<<<< SEARCH
    QString currentCategoryTitle() const { return m_categoryTitle; }
=======
    QString currentCategoryTitle() const { return m_categoryTitle; }
    int currentCategoryId() const { return m_categoryId; }
    void setCurrentCategoryId(int id) { m_categoryId = id; }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    QString m_categoryTitle;
=======
    QString m_categoryTitle;
    int m_categoryId = 0;
>>>>>>> REPLACE
```

### 3.4 `src/ui/ContentPanel.cpp`

```cpp
<<<<<<< SEARCH
void ContentPanel::setCurrentPath(const QString& path) {
    m_currentPath = path;
=======
void ContentPanel::setCurrentPath(const QString& path) {
    m_currentPath = path;
    m_currentCategoryType.clear();
    m_categoryTitle.clear();
    m_categoryId = 0;
>>>>>>> REPLACE
```

### 3.5 `src/ui/PanelMediator.cpp`

```cpp
<<<<<<< SEARCH
        if (libraryPanel) {
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel, filterPanel, addressBar](const QStringList& paths, const QString& name) {
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("library");
                    target->setCurrentCategoryTitle(name);
                    target->setLastLoadedLibraryPaths(paths);
                    target->loadLibraryPaths(paths);
                }
=======
        if (libraryPanel) {
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel, filterPanel, addressBar](const QStringList& paths, const QString& name, int categoryId) {
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("library");
                    target->setCurrentCategoryTitle(name);
                    target->setCurrentCategoryId(categoryId);
                    target->setLastLoadedLibraryPaths(paths);
                    target->loadLibraryPaths(paths);
                }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("smart_folder");
                    target->setCurrentCategoryTitle(record.name);
                    target->setLastLoadedLibraryPaths(matchingPaths);
                    target->loadLibraryPaths(matchingPaths);
                }
=======
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("smart_folder");
                    target->setCurrentCategoryTitle(record.name);
                    target->setCurrentCategoryId(-20);
                    target->setLastLoadedLibraryPaths(matchingPaths);
                    target->loadLibraryPaths(matchingPaths);
                }
>>>>>>> REPLACE
```

### 3.6 `src/ui/controllers/ContentFileOpsHandler.cpp`

```cpp
<<<<<<< SEARCH
void ContentFileOpsHandler::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride, Qt::DropAction action) {
    if (!m_panel || paths.isEmpty()) return;

    QString baseDir = !targetDirOverride.isEmpty() ? targetDirOverride : m_panel->currentPath();
=======
void ContentFileOpsHandler::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride, Qt::DropAction action) {
    if (!m_panel || paths.isEmpty()) return;

    // 1. 目标窗格分类视图拖放重定向逻辑
    QString catType = m_panel->getCurrentCategoryType();
    int catId = m_panel->currentCategoryId();

    if (catType == "library" && (catId > 0 || catId == -2)) {
        // 目标为普通分类：执行入库/归类到该分类
        bool ok = LibraryService::instance().addPathsToCategory(catId, paths);
        if (ok) {
            ToolTipOverlay::instance()->showToast(QString("已成功归入“%1”分类").arg(m_panel->currentCategoryTitle()), 2000, QColor("#2ecc71"));
            m_panel->refreshAll();
        } else {
            ToolTipOverlay::instance()->showToast("归入分类失败", 2000, QColor("#e81123"));
        }
        return;
    } else if (catType == "smart_folder" || catType == "library") {
        // 目标为智能文件夹或全量/系统分类：兜底入库归属于“未分类”
        bool ok = LibraryService::instance().addPathsToCategory(-2, paths);
        if (ok) {
            ToolTipOverlay::instance()->showToast("项目已入库到“未分类”", 2500, QColor("#3498db"));
            m_panel->refreshAll();
        } else {
            ToolTipOverlay::instance()->showToast("入库“未分类”失败", 2000, QColor("#e81123"));
        }
        return;
    }

    QString baseDir = !targetDirOverride.isEmpty() ? targetDirOverride : m_panel->currentPath();
>>>>>>> REPLACE
```

---

## 4. 架构设计与 SSOT 合规性分析
1. **职责分离与高内聚**：`ContentFileOpsHandler` 集中承接窗格内的拖放事件路由，根据目标窗格的状态感知动态判定“普通分类入库” vs “智能文件夹未分类兜底” vs “磁盘物理搬运”，逻辑高内聚。
2. **规则锁定隔离保护**：智能文件夹的只读与规则绑定属性得到了彻底保护，防范手动错拽绑定导致规则混淆，并通过友好的 `ToolTipOverlay` 引导用户“项目已入库到‘未分类’”。

---

## 5. 迁移与兼容性评估
完全向下兼容。当目标窗格处于普通磁盘路径视图时，`catType` 为空，自动绕过分类分流逻辑，直接进入原有的物理搬运流程。

---

## 6. 性能影响评估
基于数据库索引的批量关联映射（`addPathsToCategory`）性能极高，毫秒级即时响应，零开销。

---

## 7. 安全与风险防护
在主线程完成 UI 提示与模型刷新，数据持久化由 `LibraryService` 与 SQLite 事务保障，无死锁或状态失步风险。

---

## 8. 自动化测试与验证计划
1. **普通分类拖拽验证**：
   - 打开分屏双窗格：窗格 ① 打开库“其他”分类，窗格 ② 打开物理磁盘目录 `D:\Photos`。
   - 从窗格 ② 拖拽图片至窗格 ①，确认无物理文件移动发生，图片成功绑定并实时显示在“其他”分类中，显示 Toast `已成功归入“其他”分类`。
2. **智能文件夹误拖拽验证**：
   - 窗格 ① 打开智能文件夹（如“五星级”或“SVG”）。
   - 从窗格 ② 拖拽新文件至窗格 ①，确认系统拦截，自动入库至“未分类”数据库，弹出 Toast `项目已入库到“未分类”`。
3. **普通磁盘目录拖拽验证**：
   - 两窗格均为普通磁盘目录，拖拽确认正常触发物理复制/剪切移动。

---

## 9. 回滚策略
若需回滚，只需撤销相关修改提交即可。

---

## 10. 实施步骤与预估工时
1. 撰写并提交 `Implementation Plan/CrossPaneDragDropToCategory.md` 实施方案。
2. 编辑 `LibraryPanel`、`ContentPanel`、`PanelMediator` 与 `ContentFileOpsHandler` 并进行测试验证。
