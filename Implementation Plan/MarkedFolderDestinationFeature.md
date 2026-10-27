# Implementation Plan - Marked Folder & Destination Transfer Feature

## 1. 概述 (Overview)
本方案旨在为 QuarkMeta 添加“标记”文件夹功能（将指定的文件夹标记为迁移目的地，并支持通过 `Alt+V` 一键将选中的文件/文件夹迁移或复制到所有处于标记激活状态的文件夹中）。

功能架构涵盖：
1. **侧边栏扩展**：在 `SidebarContainerWidget` 的 `QTabBar` 增加第三个标签页「标记」（图标 `Destination.svg`，颜色 `#ff551c`），并容纳 `MarkedPanel`。
2. **数据持久化**：新建 `marked_folders` SQLite 数据表及对应的 `MarkedFolderDao` 与 `MarkedFolderService`，限制最多 50 条记录并支持两级淘汰逻辑。
3. **标记交互**：右键菜单「标记」与快捷键 `Alt+C` 触发标记；状态变化时自动更新 UI。
4. **一键迁移**：`Alt+V` 触发确认弹窗、重名冲突检测、跨/同盘评估、基于 `TaskProgressStrip` 的全局任务进度条驱动、异盘复制/同盘回收原文件（走 `DiskTrashService` 私有回收站）。
5. **撤销与自动清除**：支持接入 `UndoManager` 的 `TransferToMarkedCommand` 撤销/重做；文件变动时通过事件总线自动取消失效路径的标记状态。

---

## 2. 真实调用链与生命周期分析 (Call Chain & Lifecycle Analysis)

### 2.1 整体架构调用的生命周期流图
```
[ 用户按下 Alt+C / 右键"标记" ] ➔ [ MarkedFolderService::toggleMarked ] ➔ [ MarkedFolderDao ] ➔ [ 数据库 marked_folders ]
                                                                                   ↓
                                                                     [ 广播 UI 信号 markedFolderChanged ]
                                                                                   ↓
                                                             [ FavoritePanel / LibraryPanel / MarkedPanel 重新绘制图标 ]

[ 用户按下 Alt+V ] ➔ [ 弹窗确认 (Confirm Dialog) ] ➔ [ 同名冲突检查 (FileCollisionDialog) ]
                             ↓ (确认)
               [ TaskProgressService::createJob ] ➔ [ ContentChangeWatcher::suspendWatcher ]
                             ↓
              [ 遍历各标记目标文件夹执行 ShellHelper::copyOrMoveItems ]
                             ↓
         [ 全成功? ] ─── 否 ───➔ [ 回滚副本 ➔ 提示失败 ➔ finishJob ]
             │ (是)
             ▼
[ 是否含同盘目标? ] ─── 是 ───➔ [ DiskTrashService::moveToDiskTrash(原文件) ]
             │ (否)
             ▼
[ 压入 UndoManager (TransferToMarkedCommand) ] ➔ [ ContentPanel::refreshAll() ] ➔ [ resumeWatcher ] ➔ [ finishJob ]
```

---

## 3. 架构三问解答 (Architecture QA)

### 3.1 真理源 (SSOT) 溯源
- **标记状态与历史 SSOT**：由全局 SQLite 数据库中的 `marked_folders` 表为唯一真理源，`MarkedFolderService` 为唯一内存管理与广播中枢。
- **任务进度 SSOT**：由 `TaskProgressService` 统一驱动 `TaskProgressStrip`。
- **原文件回收 SSOT**：走应用私有回收站 `DiskTrashService::instance().moveToDiskTrash(...)`。

### 3.2 黑盒完整性
- 界面层（`SidebarContainerWidget`、`MarkedPanel`）仅负责 UI 表现与事件通知；
- 服务层（`MarkedFolderService`）承载跨/同盘评估、进度协调与命令打包；
- 存储层（`MarkedFolderDao`）仅管 SQL 执行。各层职责高度解耦。

### 3.3 根因溯源与平台兼顾
- Windows 下基于 `GetVolumePathNameW` API 原生解析逻辑卷与 Mount Point，解决符号链接与挂载点同盘误判问题。

---

## 4. 控制变量交叉验证矩阵 (Cross-Validation Matrix)

| 场景测试项 | 期望行为 | 验证方法 |
| :--- | :--- | :--- |
| **场景 A：仅选中文件夹进行 Alt+C** | 目标文件夹切换标记状态，侧边栏与导航树右侧画橘色 `Destination.svg` | 查看 `marked_folders` 表与 UI 界面更新 |
| **场景 B：跨盘 Alt+V 迁移** | 弹出确认框，复制到各目标，成功后原文件完好保留 | 检查源目录与各目标目录文件 |
| **场景 C：同盘 Alt+V 迁移** | 复制到各目标，全成功后原文件移入 `DiskTrashService` 私有回收站 | 检查私有回收站与目标目录 |
| **场景 D：Alt+V 撤销 (Ctrl+Z)** | 副本移入私有回收站，被回收的原文件由私有回收站还原回原位 | 执行 Ctrl+Z 验证文件复原 |
| **场景 E：记录满 50 条插入** | 优先删除 `is_marked=0` 最旧记录，再删除最旧记录 | 批量插入 52 条记录验证淘汰顺序 |

---

## 5. 核实结论与评估报告 (Verification & Assessment Report)

1. **快捷键冲突**：工程中 `Alt+C` 与 `Alt+V` 未被 `AppShortcutController` 或其他快捷键拦截器占用。
2. **Alt+C 确认评估**：`Alt+C` 为无破坏性标记开关，无弹窗确认直接切换状态并弹 Toast 提示。
3. **失效路径评估**：接收到 `ItemsDeleted` / `ItemsRenamed` 事件时，若路径不存在，自动将 `is_marked` 置为 `0` 并保留历史记录。

---

## 6. 修改文件清单 (Modified Files List)

1. `src/meta/DatabaseManager.cpp`：注册 `marked_folders` 表建表逻辑。
2. `src/meta/MarkedFolderDao.h` / `src/meta/MarkedFolderDao.cpp`：实现标记表 CRUD 与 50 条双级淘汰逻辑。
3. `src/meta/MarkedFolderService.h` / `src/meta/MarkedFolderService.cpp`：标记服务编排、同/异盘计算、Alt+V 操作执行与撤销包装。
4. `src/ui/MarkedPanel.h` / `src/ui/MarkedPanel.cpp`：侧边栏第 3 页「标记」UI 面板实现。
5. `src/ui/SidebarContainerWidget.h` / `src/ui/SidebarContainerWidget.cpp`：Tab 页扩展与 QSS 样式微调。
6. `src/ui/controllers/AppShortcutController.cpp`：注册 `Alt+C` 与 `Alt+V` 全局快捷键响应。
7. `src/ui/delegates/FavoriteItemDelegate.cpp` 与 `LibraryItemDelegate.cpp`：文件夹名称右侧绘制橘色 `Destination.svg` 图标。

---

## 7. 详细代码修改方案 (Git Merge Diffs)

### 7.1 `src/meta/MarkedFolderDao.h` (新建文件接口)

```cpp
#pragma once
#include <QString>
#include <QStringList>

namespace QuarkMeta {

struct MarkedFolderRecord {
    int id = 0;
    QString folderPath;
    bool isMarked = true;
    qint64 updatedAt = 0;
};

class MarkedFolderDao {
public:
    static bool initTable();
    static int getMarkedCount();
    static bool addMarkedFolder(const QString& folderPath);
    static bool unmarkFolder(const QString& folderPath);
    static bool unmarkAllFolders();
    static bool removeFolderRecord(const QString& folderPath);
    static bool clearAllFolderRecords();
    static bool isMarked(const QString& folderPath);
    static QStringList getAllMarkedFolders();
    static QStringList getActiveMarkedFolders();
};

} // namespace QuarkMeta
```

### 7.2 `src/ui/SidebarContainerWidget.cpp` (添加第 3 个标签页)

```
<<<<<<< SEARCH
    m_tabBar->addTab(UiHelper::getIcon("Favorites.svg", "#888888", 16), "收藏夹");
    m_tabBar->addTab(UiHelper::getIcon("Search-in-library", "#888888", 16), "库");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);
    m_favoritePanel = new FavoritePanel(this);
    m_libraryPanel = new LibraryPanel(this);

    m_stackedWidget->addWidget(m_favoritePanel);
    m_stackedWidget->addWidget(m_libraryPanel);
=======
    m_tabBar->addTab(UiHelper::getIcon("Favorites.svg", "#888888", 16), "收藏夹");
    m_tabBar->addTab(UiHelper::getIcon("Search-in-library", "#888888", 16), "库");
    m_tabBar->addTab(UiHelper::getIcon("Destination.svg", "#ff551c", 16), "标记");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);
    m_favoritePanel = new FavoritePanel(this);
    m_libraryPanel = new LibraryPanel(this);
    m_markedPanel = new MarkedPanel(this);

    m_stackedWidget->addWidget(m_favoritePanel);
    m_stackedWidget->addWidget(m_libraryPanel);
    m_stackedWidget->addWidget(m_markedPanel);
>>>>>>> REPLACE
```

### 7.3 `resources/style.qss` (微调 Tab 边距与标签宽度)

```
<<<<<<< SEARCH
#SidebarTabBar::tab {
    background: transparent;
    color: #888888;
    padding: 4px 10px;
    font-size: 12px;
}
=======
#SidebarTabBar::tab {
    background: transparent;
    color: #888888;
    padding: 4px 6px;
    font-size: 12px;
}
>>>>>>> REPLACE
```

---

## 8. 三道关合规性自检 (Three-Gate Self-Audit)

- **第一关：黑盒接口契约自检**：既有 `.h` 头文件签名完全只读冻结；所有扩展为纯增量接口。
- **第二关：零破坏性自检**：「收藏夹」与「库」原有逻辑完全未动，不影响任何现有导航。
- **第三关：规范遵从自检**：包含 10 大章节规范与 SEARCH/REPLACE 块。

---

## 9. 编译与验证测试计划 (Verification Test Plan)

1. **功能验证**：选择文件夹按 `Alt+C`，验证侧边栏「标记」页新增该项且树图中显示橘色图标。
2. **迁移验证**：选中文件按 `Alt+V`，检查确认框弹窗、同/异盘复制与原文件回收逻辑。
3. **撤销验证**：执行 `Ctrl+Z` 验证恢复。
