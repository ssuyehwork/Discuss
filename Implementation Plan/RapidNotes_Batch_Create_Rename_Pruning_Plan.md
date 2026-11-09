# RapidNotes 彻底剥离批量创建与批量重命名实施方案

> 本方案严格遵循《AGENTS.md》实施界限与《Standard Specifications for Implementation Plan.md》十全大补章节规范，旨在彻底清除与纯数据库笔记架构相冲突的物理磁盘文件批量创建（`BatchCreateDialog`）与物理文件批量重命名（`BatchRenameDialog`）整条死代码调用链，全面收敛为数据库笔记新建与行内重命名模式。

---

## 前置：已读物理证据清单 (Gate 1 Evidence)

1. **物理读取文件列表**：
   - `RapidNotes_Source/RapidNotes/CMakeLists.txt`
   - `RapidNotes_Source/RapidNotes/src/ui/controllers/ContentContextMenu.cpp`
   - `RapidNotes_Source/RapidNotes/src/ui/controllers/ContentFileOpsHandler.cpp`
   - `RapidNotes_Source/RapidNotes/src/ui/controllers/ContentFileOpsHandler.h`
   - `RapidNotes_Source/RapidNotes/src/ui/controllers/ContentKeyHandler.cpp`
   - `RapidNotes_Source/RapidNotes/src/ui/ContentPanel.h`
   - `RapidNotes_Source/RapidNotes/src/ui/ContentPanel.cpp`
   - `RapidNotes_Source/RapidNotes/Memories_RapidNotes.md`

2. **核心调用链物理签名与行号核查**：
   - `ContentContextMenu.cpp:556`: 右键菜单添加 `ActionBatchCreate`
   - `ContentContextMenu.cpp:958`: 响应 `case ContentPanel::ActionBatchCreate:` 打开 `BatchCreateDialog`
   - `ContentContextMenu.cpp:884`: 右键菜单多选时调用 `m_panel->performBatchRename();`
   - `ContentKeyHandler.cpp:597`: F2 快捷键多选时调用 `m_panel->performBatchRename();`
   - `ContentPanel.h:66, 68`: 枚举 `ActionBatchRename` 与 `ActionBatchCreate`
   - `ContentPanel.h:184`: `void performBatchRename();`
   - `ContentPanel.cpp:831`: `void ContentPanel::performBatchRename() { ... }`
   - `ContentFileOpsHandler.h:18`: `void performBatchRename();`
   - `ContentFileOpsHandler.cpp:36`: `void ContentFileOpsHandler::performBatchRename() { ... }`
   - `CMakeLists.txt:412,443,453,568,571,581`: 注册的 10 个相关源码文件

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题根因分析
在 RapidNotes 从面向本地磁盘文件系统向纯数据库笔记管理（`inspiration.db`，由 `notes` 与 `categories` 表驱动）融合转型的过程中，原 QuarkMeta 移植过来的两组物理磁盘批量操作逻辑变成了死代码和冲突源：
1. **`BatchCreateDialog`**：根据前缀/后缀在磁盘物理目录调用 `QDir().mkpath()` 或 `QFile` 批量创建空白文件。在数据库模式下，创建笔记走 `Ctrl+N`，创建分类走 `Ctrl+Shift+N`，不再需要磁盘批量创建；
2. **`BatchRenameDialog`**：依赖 `BatchRenameEngine`、`BatchRenameService` 和 `BatchRenameCommand` 对物理路径列表执行操作系统层级的改名与文件监控抑制。在数据库模式下，笔记条目重命名遵循统一的 **行内就地重命名（F2 Inline Rename）**，两者底层语义严重冲突。

### 1.2 重构目标
1. 彻底物理删除 10 个物理磁盘批量操作相关的 `.h` / `.cpp` 源码文件；
2. 在 `CMakeLists.txt` 中同步移除这 10 个文件的注册，保持构建树纯净；
3. 清理 `ContentContextMenu`、`ContentKeyHandler`、`ContentFileOpsHandler` 与 `ContentPanel` 中的调用接口与菜单项，消除孤儿死代码。

---

## Chapter 2: Modified Files List（影响文件清单）

### 删除的文件（共 10 个）
1. `RapidNotes_Source/RapidNotes/src/ui/BatchCreateDialog.h`
2. `RapidNotes_Source/RapidNotes/src/ui/BatchCreateDialog.cpp`
3. `RapidNotes_Source/RapidNotes/src/ui/CreateRuleRow.h`
4. `RapidNotes_Source/RapidNotes/src/ui/CreateRuleRow.cpp`
5. `RapidNotes_Source/RapidNotes/src/ui/BatchRenameDialog.h`
6. `RapidNotes_Source/RapidNotes/src/ui/BatchRenameDialog.cpp`
7. `RapidNotes_Source/RapidNotes/src/meta/BatchRenameEngine.h`
8. `RapidNotes_Source/RapidNotes/src/meta/BatchRenameEngine.cpp`
9. `RapidNotes_Source/RapidNotes/src/core/BatchRenameService.h`
10. `RapidNotes_Source/RapidNotes/src/core/BatchRenameService.cpp`
11. `RapidNotes_Source/RapidNotes/src/core/commands/BatchRenameCommand.h`

### 修改的文件（共 5 个）
1. `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 移除 10 个文件的编译源列表。
2. `RapidNotes_Source/RapidNotes/src/ui/ContentPanel.h` — 移除枚举与接口。
3. `RapidNotes_Source/RapidNotes/src/ui/ContentPanel.cpp` — 移除方法定义。
4. `RapidNotes_Source/RapidNotes/src/ui/controllers/ContentFileOpsHandler.h` 与 `.cpp` — 移除 `performBatchRename()` 声明、实现与包含。
5. `RapidNotes_Source/RapidNotes/src/ui/controllers/ContentKeyHandler.cpp` — 移除 F2 多选批量改名调用。
6. `RapidNotes_Source/RapidNotes/src/ui/controllers/ContentContextMenu.cpp` — 移除右键菜单中的批量创建和批量重命名动作。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 移除 BatchRenameService

```
<<<<<<< SEARCH
    src/core/BatchRenameService.cpp
    src/core/BatchRenameService.h
=======
>>>>>>> REPLACE
```

### 3.2 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 移除 BatchRenameCommand

```
<<<<<<< SEARCH
    src/core/commands/BatchRenameCommand.h
=======
>>>>>>> REPLACE
```

### 3.3 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 移除 BatchRenameEngine

```
<<<<<<< SEARCH
    src/meta/BatchRenameEngine.cpp
    src/meta/BatchRenameEngine.h
=======
>>>>>>> REPLACE
```

### 3.4 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 移除 BatchCreateDialog、BatchRenameDialog 与 CreateRuleRow

```
<<<<<<< SEARCH
    src/ui/BatchCreateDialog.cpp
    src/ui/BatchCreateDialog.h
    src/ui/BatchProgressDialog.h
    src/ui/BatchRenameDialog.cpp
    src/ui/BatchRenameDialog.h
=======
    src/ui/BatchProgressDialog.h
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    src/ui/CreateRuleRow.cpp
    src/ui/CreateRuleRow.h
=======
>>>>>>> REPLACE
```

### 3.5 `RapidNotes_Source/RapidNotes/src/ui/ContentPanel.h` — 移除 ActionBatchRename 与 ActionBatchCreate 枚举及接口

```
<<<<<<< SEARCH
        ActionBatchRename, ActionRename, ActionCopy, ActionCut, ActionDuplicate, ActionPaste, ActionCopyTags, ActionPasteTags, ActionRepeatLastOp, ActionDelete,
=======
        ActionRename, ActionCopy, ActionCut, ActionDuplicate, ActionPaste, ActionCopyTags, ActionPasteTags, ActionRepeatLastOp, ActionDelete,
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionBatchCreate, ActionFileCompare,
=======
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionFileCompare,
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void createNewItem(const QString& type);
    void performBatchRename();
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride = QString(), Qt::DropAction action = Qt::CopyAction);
=======
    void createNewItem(const QString& type);
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride = QString(), Qt::DropAction action = Qt::CopyAction);
>>>>>>> REPLACE
```

### 3.6 `RapidNotes_Source/RapidNotes/src/ui/ContentPanel.cpp` — 移除 performBatchRename 实现

```
<<<<<<< SEARCH
void ContentPanel::createNewItem(const QString& type) {
    if (m_fileOpsHandler) m_fileOpsHandler->createNewItem(type);
}

void ContentPanel::performBatchRename() {
    if (m_fileOpsHandler) m_fileOpsHandler->performBatchRename();
}

void ContentPanel::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride, Qt::DropAction action) {
=======
void ContentPanel::createNewItem(const QString& type) {
    if (m_fileOpsHandler) m_fileOpsHandler->createNewItem(type);
}

void ContentPanel::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride, Qt::DropAction action) {
>>>>>>> REPLACE
```

### 3.7 `RapidNotes_Source/RapidNotes/src/ui/controllers/ContentFileOpsHandler.h` — 移除 performBatchRename 声明

```
<<<<<<< SEARCH
    void createNewItem(const QString& type);
    void performBatchRename();
    bool resolvePasteDestination();
=======
    void createNewItem(const QString& type);
    bool resolvePasteDestination();
>>>>>>> REPLACE
```

### 3.8 `RapidNotes_Source/RapidNotes/src/ui/controllers/ContentFileOpsHandler.cpp` — 移除 BatchRenameDialog 包含与实现

```
<<<<<<< SEARCH
#include "../ToolTipOverlay.h"
#include "../BatchRenameDialog.h"
#include "../FileCollisionDialog.h"
=======
#include "../ToolTipOverlay.h"
#include "../FileCollisionDialog.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentFileOpsHandler::performBatchRename() {
    if (!m_panel) return;
    std::vector<std::wstring> originalPaths;
    for (const auto& idx : m_panel->getSelectedIndexes()) {
        if (idx.column() == static_cast<int>(FileListColumn::Name)) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) {
                originalPaths.push_back(QDir::toNativeSeparators(p).toStdWString());
            }
        }
    }
    if (originalPaths.empty()) return;

    // 收集根窗格与所有子窗格（去重）
    QList<ContentPanel*> targetPanes;
    ContentPanel* root = m_panel->rootPane();
    if (root) {
        targetPanes.append(root);
        for (ContentPanel* pane : root->panes()) {
            if (pane && !targetPanes.contains(pane)) {
                targetPanes.append(pane);
            }
        }
    } else if (m_panel) {
        targetPanes.append(m_panel);
    }

    struct WatchSuppressor {
        QList<ContentPanel*> panes;
        explicit WatchSuppressor(const QList<ContentPanel*>& p) : panes(p) {
            for (ContentPanel* cp : panes) {
                if (cp) cp->beginWatchSuppress();
            }
        }
        ~WatchSuppressor() {
            for (ContentPanel* cp : panes) {
                if (cp) cp->endWatchSuppress();
            }
        }
    } suppressor(targetPanes);

    BatchRenameDialog dlg(originalPaths, m_panel);
    if (dlg.exec() == QDialog::Accepted) {
        for (ContentPanel* cp : targetPanes) {
            if (cp) cp->refreshAll();
        }
    }
}

bool ContentFileOpsHandler::resolvePasteDestination() {
=======
bool ContentFileOpsHandler::resolvePasteDestination() {
>>>>>>> REPLACE
```

### 3.9 `RapidNotes_Source/RapidNotes/src/ui/controllers/ContentKeyHandler.cpp` — 移除 F2 快捷键多选调用

```
<<<<<<< SEARCH
        QStringList selectedPaths = m_panel->getSelectedPaths();
        if (selectedPaths.size() > 1) {
            m_panel->performBatchRename();
        } else {
=======
        QStringList selectedPaths = m_panel->getSelectedPaths();
        if (selectedPaths.size() > 1) {
            // 数据库笔记模型多选时不执行物理批量改名
        } else {
>>>>>>> REPLACE
```

### 3.10 `RapidNotes_Source/RapidNotes/src/ui/controllers/ContentContextMenu.cpp` — 移除包含与菜单动作

```
<<<<<<< SEARCH
#include "../FavoritePanel.h"
#include "../BatchRenameDialog.h"
#include "../BatchCreateDialog.h"
#include "../FramelessDialog.h"
=======
#include "../FavoritePanel.h"
#include "../FramelessDialog.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                menu.addSeparator();
                menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE")), "批量创建项目...")->setData(ContentPanel::ActionBatchCreate);
                menu.addSeparator();
=======
                menu.addSeparator();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        case ContentPanel::ActionBatchRename:
        case ContentPanel::ActionRename: {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (selectedPaths.size() > 1) {
                m_panel->performBatchRename();
            } else {
                view->edit(currentIndex);
            }
            break;
        }
=======
        case ContentPanel::ActionRename: {
            view->edit(currentIndex);
            break;
        }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        case ContentPanel::ActionBatchCreate: {
            BatchCreateDialog dlg(currentPath, m_panel);
            if (dlg.exec() == QDialog::Accepted) {
                m_panel->refreshAll();
            }
            break;
        }
        case ContentPanel::ActionRestore: {
=======
        case ContentPanel::ActionRestore: {
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译验收步骤
1. 执行 CMake 构建：
   ```cmd
   cmake -B build
   cmake --build build --config Release
   ```
2. 验证结果：
   - 10 个物理磁盘文件剥离后，工程编译顺利，无任何未定义符号（Unresolved External Symbols）；
   - CMake 生成树整洁，无多余冗余编译单元。

### 4.2 运行时验收标准
| 测试场景 | 操作步骤 | 预期效果 |
|---|---|---|
| 右键菜单检查 | 在空白区域或文件卡片上右键打开上下文菜单 | “批量创建项目...”菜单项已彻底消失，菜单布局整洁干练 |
| 多选重命名检查 | 在列表中多选多条笔记，观察右键菜单与快捷键 | 无“批量重命名”动作，单条笔记按下 F2 正常进入行内重命名编辑模式 |
| 新建笔记与分类 | 按下 Ctrl+N 或 Ctrl+Shift+N | 纯数据库新建流程正常触发，不受物理磁盘文件剥离影响 |

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 功能 | 复用 SSOT | 模块位置 |
|---|---|---|
| 行内重命名编辑 | `ItemModelBase::setData(index, title, Qt::EditRole)` | `ItemModelBase.h` |
| 笔记创建 | `DatabaseManager::instance().addNote` | `DatabaseManager.h` |
| 分类创建 | `DatabaseManager::instance().createCategory` | `DatabaseManager.h` |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 来源 | 消费点 | 说明 |
|---|---|---|---|
| 笔记标题 | `notes.title` | 行内就地编辑 | 消除基于物理文件名的间接同步 |
| 分类层级 | `categories` 表 | 分类树渲染 | 消除物理目录递归扫描 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 调用 | 物理签名位置 | 访问级别 |
|---|---|---|
| `ContentPanel::createNewItem` | `ContentPanel.h:183` | ✅ public |
| `ContentFileOpsHandler::createNewItem` | `ContentFileOpsHandler.h:17` | ✅ public |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

| 类型 | 状态 | 说明 |
|---|---|---|
| `BatchRenameDialog` | 已移除 | 头文件与调用点完全移除，无残留符号 |
| `BatchCreateDialog` | 已移除 | 头文件与调用点完全移除，无残留符号 |

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 说明 |
|---|---|---|---|
| 1 | 方案已通过需求确认 | ✅ 满足 | 用户明确指示需要创建实施方案 |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见前置证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 经由 plan_checker 工具验证通过 |
| 4 | 新增接口声明、定义、调用点一致 | ✅ 不涉及 | 纯代码与死调用链剥离 |
| 5 | 被修改的 `#include` 闭合 | ✅ 满足 | 彻底消除孤儿头文件 |
| 6 | 新建/删除文件在 CMakeLists 同步 | ✅ 满足 | 同步移除 10 个源码文件条目 |
| 7 | 跨类调用访问级别符合要求 | ✅ 满足 | 保持既有权限 |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 严格限制在清单内 |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | 根除物理文件重命名引起的 I/O 监听阻塞 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 整条物理文件调用链完整拔除 |
| 11 | 遵循 Memories_RapidNotes.md 规范 | ✅ 满足 | 已同步更新第 19 章里程碑 6 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 不涉及 | 未调用 ToolTipOverlay |
| 13 | 100% 语义化矢量图标 | ✅ 不涉及 | 移除冗余菜单图标 |
| 14 | 异步回调按值捕获 | ✅ 不涉及 | 无异步改动 |
| 15 | 无 Lambda 循环引用 | ✅ 不涉及 | 无循环引用 |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 不涉及 | 纯 UI 与控制器清理 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 不涉及 | 移除既有信号槽连接 |
| 18 | 对外 `.h` 签名冻结 | ✅ 满足 | 内部控制器方法清理，不影响对外公共组件 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明
| 既有功能 | 本方案影响 | 结论 |
|---|---|---|
| 单条笔记行内就地重命名 | F2 快捷键对选中的单条笔记正常进入行内编辑框 | ✅ 原样保留 |
| 正常分类切换与右键菜单 | 移除物理文件创建与改名项后，菜单更加精简响应更快 | ✅ 原样保留并优化 |

### 数值零篡改声明
| 数值参数 | 当前设定 | 结论 |
|---|---|---|
| 右键菜单图标尺寸 | 16x16 | ✅ 100% 原样照抄 |
