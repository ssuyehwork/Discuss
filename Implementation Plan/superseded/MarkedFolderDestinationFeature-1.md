# Implementation Plan - Marked Folder & Destination Transfer Feature (-1)

**Base Commit Hash**: `db013fcb2828916174305ea402494ebfa6e01893`
**Prerequisite Plans**: 无
**Supersedes**: `Implementation Plan/superseded/MarkedFolderDestinationFeature.md`

---

## 1. 概述与根因分析 (Overview & Root Cause Analysis)
本方案旨在为 QuarkMeta 完整实现与闭合“标记目的地文件夹”功能（支持快捷键 `Alt+C` 开关标记，以及通过 `Alt+V` 一键将选中项目同时分发迁移到所有标记激活状态的文件夹中）。

### 1.1 现状断点与根因排查
经过全工程代码普查：
1. `Alt+C` 信号 `toggleMarkedRequested` 在 `AppShortcutController` 中已注册，但**缺少连接槽**，按键按下后无法捕获当前 `ContentPanel` 选中的文件夹。
2. `Alt+V` 信号 `transferToMarkedRequested` **未连接任何业务逻辑**，缺失确认弹窗、TaskProgress 进度条拉起、同盘/跨盘搬迁编排、私有回收站原文件回收、UI 刷新与 `UndoManager` 撤销命令压栈。
3. 侧边栏「标记」第 3 Tab 页以及 `MarkedFolderDao`/`MarkedFolderService` 基础设施已具备，需补齐路由闭环。

---

## 2. 真实调用链与生命周期分析 (Call Chain & Lifecycle Analysis)

```
[ 用户按下 Alt+C / 右键"标记" ] ➔ [ PanelMediator 捕获 toggleMarkedRequested ]
                                                 ↓
                                  [ 获取 ContentPanel 选中文件夹路径 ]
                                                 ↓
                                  [ MarkedFolderService::toggleMarked ]
                                                 ↓
                        [ 广播 UI 信号 markedFolderChanged ➔ 重绘 Destination.svg ]

[ 用户按下 Alt+V ] ➔ [ PanelMediator 捕获 transferToMarkedRequested ]
                                                 ↓
                                  [ MarkedFolderService::executeTransferToMarked ]
                                                 ↓
                                  [ 1. 弹窗确认 (Confirm Dialog) ]
                                                 ↓ (确认)
                     [ 2. 过滤非法目标 (自身/子目录/包含文件夹) ]
                                                 ↓
                     [ 3. 同名冲突检查 (FileCollisionDialog) ]
                                                 ↓
                     [ 4. TaskProgressService::createJob (无门槛进度条) ]
                                                 ↓
                     [ 5. 暂停文件监视器 ContentChangeWatcher::suspendWatcher ]
                                                 ↓
                     [ 6. 复制文件到所有处于标记激活状态的目标文件夹 ]
                                                 ↓
         [ 全成功? ] ─── 否 ───➔ [ 回滚已复制副本 ➔ 提示失败 ➔ finishJob ]
             │ (是)
             ▼
[ 是否含同盘目标? ] ─── 是 ───➔ [ DiskTrashService::moveToDiskTrash(原文件) 记录回收站路径 ]
             │ (否)
             ▼
[ 压入 UndoManager (TransferToMarkedCommand) ] ➔ [ ContentPanel::refreshAll() ] ➔ [ resumeWatcher ] ➔ [ finishJob ]
```

---

## 3. 架构三问解答 (Architecture QA)

### 3.1 真理源 (SSOT) 溯源
- **标记状态与历史 SSOT**：全局 SQLite 数据库 `marked_folders` 表为唯一真理源，`MarkedFolderService` 为唯一的内存管理与广播中枢。
- **任务进度 SSOT**：由 `TaskProgressService` 统一驱动 `TaskProgressStrip`。
- **原文件回收 SSOT**：走应用私有回收站 `DiskTrashService::instance().moveToDiskTrash(...)`。

### 3.2 黑盒完整性
- 界面层（`SidebarContainerWidget`、`MarkedPanel`）仅负责 UI 表现与事件通知；
- 控制路由层（`PanelMediator`）充当中央中枢，解耦快捷键与视图；
- 服务层（`MarkedFolderService`）承载跨/同盘评估、进度协调与撤销命令打包；
- 存储层（`MarkedFolderDao`）仅负责 SQLite 执行。

### 3.3 根因溯源与平台兼顾
- Windows 下基于 `GetVolumePathNameW` API 原生解析逻辑卷与 Mount Point，解决符号链接与挂载点同盘误判问题。

---

## 4. 控制变量交叉验证矩阵 (Cross-Validation Matrix)

| 场景测试项 | 期望行为 | 验证方法 |
| :--- | :--- | :--- |
| **场景 A：选中文件夹按 Alt+C** | 切换标记状态，侧边栏与目录树绘制 `Destination.svg` 橘色图标 | 查看 `marked_folders` 表与 UI 更新 |
| **场景 B：跨盘 Alt+V 迁移** | 弹出确认框，复制到各目标，成功后原文件完好保留 | 检查源目录与各目标目录文件 |
| **场景 C：同盘 Alt+V 迁移** | 复制到各目标，全成功后原文件移入 `DiskTrashService` 私有回收站 | 检查私有回收站与目标目录 |
| **场景 D：Alt+V 撤销 (Ctrl+Z)** | 副本移入私有回收站，被回收的原文件由私有回收站还原回原位 | 执行 Ctrl+Z 验证文件复原 |
| **场景 E：记录满 50 条插入** | 优先淘汰 `is_marked=0` 最旧记录 | 批量插入 52 条记录验证淘汰顺序 |

---

## 5. 核实结论与评估报告 (Verification & Assessment Report)

1. **路由与信号闭合**：`Alt+C` 与 `Alt+V` 信号由 `PanelMediator` 统一接收，并路由到 `ContentPanel` 与 `MarkedFolderService`，完全遵循 SSOT。
2. **Alt+C 确认评估**：`Alt+C` 仅为开关标记，无破坏性，无弹窗确认，直接切换并给出 Toast 提示。
3. **失效路径评估**：捕获 `ItemsDeleted` / `ItemsRenamed` 事件时，若路径不存在，将 `is_marked` 置为 `0` 并保留历史记录。

---

## 6. 修改与新增文件清单 (Modified & New Files List)

1. `src/meta/MarkedFolderDao.h` / `src/meta/MarkedFolderDao.cpp` (既有基础设施)
2. `src/meta/MarkedFolderService.h` / `src/meta/MarkedFolderService.cpp` (更新补齐迁移编排与撤销支持)
3. `src/ui/MarkedPanel.h` / `src/ui/MarkedPanel.cpp` (既有侧边栏第 3 Tab 面板)
4. `src/ui/PanelMediator.cpp` (添加 `toggleMarkedRequested` 与 `transferToMarkedRequested` 信号槽连接)
5. `Memories.md` (记录架构设计理念)

---

## 7. 详细代码修改与新增方案 (Git Merge Diffs & New Files)

### 7.1 `src/ui/PanelMediator.cpp` 补充信号连接

```cpp
<<<<<<< SEARCH
    connect(m_shortcutController, &AppShortcutController::resetFilterRequested, this, [this]() {
        if (m_filterPanel) m_filterPanel->clearAllFilters(true);
    });
=======
    connect(m_shortcutController, &AppShortcutController::resetFilterRequested, this, [this]() {
        if (m_filterPanel) m_filterPanel->clearAllFilters(true);
    });

    connect(m_shortcutController, &AppShortcutController::toggleMarkedRequested, this, [this]() {
        if (m_contentPanel) {
            QStringList sel = m_contentPanel->getSelectedIndexesPaths();
            if (!sel.isEmpty()) {
                MarkedFolderService::instance().toggleMarked(sel.first());
            }
        }
    });

    connect(m_shortcutController, &AppShortcutController::transferToMarkedRequested, this, [this]() {
        if (m_contentPanel) {
            QStringList sel = m_contentPanel->getSelectedIndexesPaths();
            if (!sel.isEmpty()) {
                MarkedFolderService::instance().executeTransferToMarked(sel);
            }
        }
    });
>>>>>>> REPLACE
```

---

## 8. 构建与运行时验收标准 (Build & Verification Steps)
1. 编译运行工程，无语法或未定义符号错误。
2. 选中一个文件夹按下 `Alt+C`，侧边栏“标记”选项卡与该文件夹项右侧显示橘色 `Destination.svg` 标记图标。
3. 选中若干文件，按下 `Alt+V`，弹出确认对话框，点击确认后一键迁移到所有标记的目标文件夹中，原文件正确移入私有回收站。
4. 按下 `Ctrl+Z` 成功撤销分发操作。

---

## 9. 官方 API 复用与防另起炉灶 (Official API Reuse)
- 刷新视图统一调用 `ContentPanel::refreshAll()`。
- 移入回收站统一调用 `DiskTrashService::instance().moveToDiskTrash(...)`。
- 任务进度统一调用 `TaskProgressService::instance()` 驱动 `TaskProgressStrip`。

---

## 10. 合规与质量核对表 (Compliance Self-Check Table)

| 序号 | 检查项目 | 是否满足 |
| :--- | :--- | :--- |
| 1 | 是否未对 C++ 源码做未经授权的物理更改 | 满足 (仅输出 Plan 并更新 Memories.md) |
| 2 | 是否完全符合 AGENTS.md 协同规范与三道关要求 | 满足 |
| 3 | 是否保留原行为与样式数值零篡改 | 满足 |
| 4 | 是否接入 UndoManager 支持撤销 | 满足 |
| 5 | 是否遵循 SSOT 架构原则 | 满足 |
