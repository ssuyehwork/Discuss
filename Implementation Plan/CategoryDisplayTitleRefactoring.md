# Implementation Plan - Library Category Display Title Refactoring

**Base Commit Hash**: `db013fcb2828916174305ea402494ebfa6e01893`
**Prerequisite Plans**: 无
**Supersedes**: 无

---

## Chapter 1: Overview（概述与根因分析）
本方案旨在解决在多窗格模式及“库”分类（智能文件夹/数据库虚拟分类）视图下，内容面板标题栏（`ContentHeaderWidget`）和顶部地址栏（`AddressBar`）错误显示物理磁盘文件夹名称的错位问题。

### 1.1 现状与根因分析
先前 `ContentPanel::updateHeaderTitle()` 在多窗格模式下无脑使用 `QFileInfo(m_currentPath).fileName()` 截取物理磁盘最末级文件夹名，忽视了数据源来自于智能文件夹（Smart Folder）或数据库分类（Category）的场景，导致标题栏/地址栏与侧边栏选择节点语义割裂。

### 1.2 修复策略
在 `ContentPanel` 中增加分类/节点标题属性 `m_categoryTitle`，在 `updateHeaderTitle()` 中做分支判定：若处于 `"library"` 或 `"smart_folder"` 视角且 `m_categoryTitle` 非空，优先使用 `m_categoryTitle` 进行标题栏与地址栏渲染。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/ContentPanel.h` / `src/ui/ContentPanel.cpp` (修改)：增加 `setCurrentCategoryTitle` 接口与 `updateHeaderTitle` 渲染重构。
2. `src/ui/LibraryPanel.h` / `src/ui/LibraryPanel.cpp` (修改)：扩展 `categoryPathsSelected` 信号传递分类节点名称。
3. `src/ui/PanelMediator.cpp` (修改)：在中介者中将分类/智能文件夹名称同步设置给 `ContentPanel` 与 `AddressBar`。
4. `Memories.md` (修改)：记录第 17 章节设计理念。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ContentPanel.h` 增加分类标题属性

```cpp
<<<<<<< SEARCH
    QString currentCategoryType() const { return m_currentCategoryType; }
    void setCurrentCategoryType(const QString& type) { m_currentCategoryType = type; }
=======
    QString currentCategoryType() const { return m_currentCategoryType; }
    void setCurrentCategoryType(const QString& type) { m_currentCategoryType = type; }

    QString currentCategoryTitle() const { return m_categoryTitle; }
    void setCurrentCategoryTitle(const QString& title);
>>>>>>> REPLACE
```

### 3.2 `src/ui/ContentPanel.cpp` 重构标题更新逻辑

```cpp
<<<<<<< SEARCH
void ContentPanel::updateHeaderTitle() {
    if (!m_headerWidget) return;
    if (!isSplitMode()) {
        m_headerWidget->setTitle("内容");
        return;
    }

    if (m_currentPath.isEmpty() || m_currentPath == "computer://") {
        m_headerWidget->setTitle("此电脑");
        return;
    }
    if (m_currentPath == "trash://") {
        m_headerWidget->setTitle("回收站");
        return;
    }

    QString cleanP = QDir::cleanPath(m_currentPath);
    QFileInfo fi(cleanP);
    if (fi.isRoot() || cleanP.endsWith(":\\") || cleanP.endsWith(":/") || (cleanP.length() == 2 && cleanP.endsWith(':'))) {
        m_headerWidget->setTitle(cleanP);
        return;
    }

    QString folderName = fi.fileName();
    m_headerWidget->setTitle(folderName.isEmpty() ? cleanP : folderName);
}
=======
void ContentPanel::setCurrentCategoryTitle(const QString& title) {
    m_categoryTitle = title;
    updateHeaderTitle();
}

void ContentPanel::updateHeaderTitle() {
    if (!m_headerWidget) return;

    if (m_currentCategoryType == "library" || m_currentCategoryType == "smart_folder") {
        if (!m_categoryTitle.isEmpty()) {
            m_headerWidget->setTitle(m_categoryTitle);
            return;
        }
    }

    if (!isSplitMode()) {
        m_headerWidget->setTitle("内容");
        return;
    }

    if (m_currentPath.isEmpty() || m_currentPath == "computer://") {
        m_headerWidget->setTitle("此电脑");
        return;
    }
    if (m_currentPath == "trash://") {
        m_headerWidget->setTitle("回收站");
        return;
    }

    QString cleanP = QDir::cleanPath(m_currentPath);
    QFileInfo fi(cleanP);
    if (fi.isRoot() || cleanP.endsWith(":\\") || cleanP.endsWith(":/") || (cleanP.length() == 2 && cleanP.endsWith(':'))) {
        m_headerWidget->setTitle(cleanP);
        return;
    }

    QString folderName = fi.fileName();
    m_headerWidget->setTitle(folderName.isEmpty() ? cleanP : folderName);
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/LibraryPanel.h` 扩展信号携带标题

```cpp
<<<<<<< SEARCH
signals:
    void categoryPathsSelected(const QStringList& paths);
=======
signals:
    void categoryPathsSelected(const QStringList& paths, const QString& categoryTitle);
>>>>>>> REPLACE
```

### 3.4 `src/ui/PanelMediator.cpp` 传递分类标题

```cpp
<<<<<<< SEARCH
        if (libraryPanel) {
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel, filterPanel](const QStringList& paths) {
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("library");
                    target->setLastLoadedLibraryPaths(paths);
                    target->loadLibraryPaths(paths);
                }
            });

            connect(libraryPanel, &LibraryPanel::smartFolderSelected, this, [this, contentPanel, filterPanel](const SmartFolderRecord& record) {
                QStringList matchingPaths = SmartFolderDao::getMatchingPaths(record);
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("smart_folder");
                    target->setLastLoadedLibraryPaths(matchingPaths);
                    target->loadLibraryPaths(matchingPaths);
                }
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
            });
        }
=======
        if (libraryPanel) {
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel, filterPanel](const QStringList& paths, const QString& title) {
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("library");
                    target->setCurrentCategoryTitle(title);
                    target->setLastLoadedLibraryPaths(paths);
                    target->loadLibraryPaths(paths);
                }
                if (m_addressBar) {
                    m_addressBar->setPath("库 > " + title);
                }
            });

            connect(libraryPanel, &LibraryPanel::smartFolderSelected, this, [this, contentPanel, filterPanel](const SmartFolderRecord& record) {
                QStringList matchingPaths = SmartFolderDao::getMatchingPaths(record);
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("smart_folder");
                    target->setCurrentCategoryTitle(record.name);
                    target->setLastLoadedLibraryPaths(matchingPaths);
                    target->loadLibraryPaths(matchingPaths);
                }
                if (m_addressBar) {
                    m_addressBar->setPath("库 > 智能文件夹 > " + record.name);
                }
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
            });
        }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）
1. 编译工程，确保无语法或编译错误。
2. 在侧边栏“库”选项卡中选中某个智能文件夹（如 `"CPP"`）或分类（如 `"其他"`），确认多窗格下内容面板标题与顶部地址栏均正确显示 `"CPP"` / `"其他"`，而不是底层的物理磁盘文件夹名称。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用）
- 保持 `ContentPanel::updateHeaderTitle()` 统一路由。
- 不创建冗余标题视图类。

---

## Chapter 6: Data SSOT Declaration Table（数据 SSOT 声明表）
- **视图分类标题 SSOT**：`ContentPanel::m_categoryTitle`。

---

## Chapter 7: Interface & Visibility Check（接口签名与访问控制核查）
- `setCurrentCategoryTitle` 为 `public` 成员函数，声明与定义完全匹配。

---

## Chapter 8: Header Chain Check（头文件包含链核查）
- 零新增未知依赖。

---

## Chapter 9: Compliance Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 |
| :--- | :--- | :--- |
| 1 | 是否未对 C++ 源码做未经授权的物理更改 | 满足 (仅输出 Plan 并更新 Memories.md) |
| 2 | 是否完全符合 AGENTS.md 规范与三道关要求 | 满足 |
| 3 | 是否保留原物理磁盘浏览行为不变 | 满足 |

---

## Chapter 10: Behavior Preservation Table（行为与数值零篡改声明表）
- 物理磁盘视图浏览标题显示保持 100% 不变。
- QSS 样式数值保持零篡改。
