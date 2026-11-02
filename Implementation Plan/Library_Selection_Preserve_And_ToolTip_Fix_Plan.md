# 库面板刷新状态保持与 ToolTipOverlay 绝对禁跟光标实施方案 (Library Selection Preserve & Non-Cursor ToolTip Plan)

语言：正文与注释只允许使用中文（标识符、代码块、标准 Git Merge Diff 标记与固定章节英文标题除外）。

---

## 物理证据清单 (Pre-Draft Evidence)
1. **物理读取文件列表**：
   - `QuarkMeta_Source/src/ui/LibraryPanel.h`：已读取，确认 `LibraryPanel` 类成员与 `m_treeView`、`m_model` 访问级别。
   - `QuarkMeta_Source/src/ui/LibraryPanel.cpp`：已读取，确认第 1168 行 `loadLibrary()` 内部在执行 `m_model->clear()` 清空模型前未记录选中节点 ID，重建树后未恢复选中高亮。
   - `QuarkMeta_Source/src/meta/LibraryService.cpp`：已读取，确认第 90~110 行构建上下文菜单动作时，硬编码传入了 `QCursor::pos()` 调用 `ToolTipOverlay::instance()->showText(...)`，严重违反 Memories.md 第 21 章规范。
2. **头文件原样签名核验**：
   - `void ToolTipOverlay::showToast(const QString& text, int timeout = 1500, const QColor& borderColor = QColor("#2ecc71"), QWidget* targetWidget = nullptr);`
   - `QStandardItem* LibraryPanel::findItemByNodeId(QStandardItem* parent, int nodeId);`
3. **既有 API 检索日志**：
   - `grep -rn "showToast" QuarkMeta_Source/src/ui/ToolTipOverlay.h` -> 确认直接复用 `ToolTipOverlay::instance()->showToast(text, timeout, borderColor)`。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与根因分析
1. **库选项卡分类刷新后丢失选中高亮根因**：
   - 在用户执行将项目“从‘库’中移除”等数据变更操作时，底层通过 `libraryChanged` 触发了 `LibraryPanel::loadLibrary()`；
   - 在 `LibraryPanel::loadLibrary()` 的开头，直接执行了 `m_model->clear()`，将整棵树的所有模型项释放并清空；
   - 在清空前，**完全没有记录当前活跃被选中的节点 ID（`savedNodeId`）与类型**；在全树节点重新生成挂载完成后，也**没有执行任何选择状态恢复逻辑**；
   - 这导致树模型重建后，原本选中的节点高亮直接消失（处于无选中状态），而右侧主内容区仍然在展示该分类内容，产生了严重的视图断层与视觉残缺；
   - **架构修复**：
     - 在 `loadLibrary()` 执行 `m_model->clear()` 之前，先获取当前选中项的 `QModelIndex currentIndex = m_treeView->currentIndex()`，提取其节点 ID `int savedNodeId = currentIndex.data(Qt::UserRole + 1).toInt()`；
     - 在全树重建并展开完毕后，若 `savedNodeId != 0`，递归检索该节点（`findItemByNodeId(m_model->invisibleRootItem(), savedNodeId)`）；
     - 检索成功后，调用 `m_treeView->setCurrentIndex(targetItem->index())` 恢复高亮选中状态。
2. **ToolTipOverlay 违规跟随鼠标光标移动显示根因**：
   - Memories.md 第 21 章节明确规定：“全软件内所有通过 ToolTipOverlay 触发的浮层反馈……严禁跟随鼠标光标位置（`QCursor::pos()`）进行漂移动态渲染，必须自动锚定当前活跃内容面板正中居中，距离顶部下方 10px”；
   - 但在 `LibraryService.cpp` 中（第 91、93、104、106 行），代码硬编码传入了 `QCursor::pos()` 触发 `showText`，导致提示框在屏幕边缘或跟随鼠标指针乱窜；
   - **架构修复**：
     - 将 `LibraryService.cpp` 中所有对 `ToolTipOverlay::instance()->showText(QCursor::pos(), ...)` 的调用彻底清除；
     - 统一替换为规范的 `ToolTipOverlay::instance()->showToast(...)`，完全由组件内部统筹计算面板居中锚定坐标，消除鼠标跟随。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `QuarkMeta_Source/src/ui/LibraryPanel.cpp`（修改：在 `loadLibrary()` 树清空前记录当前选中节点 ID，并在树重建完成后恢复选中高亮）
2. `QuarkMeta_Source/src/meta/LibraryService.cpp`（修改：移除 `QCursor::pos()` 违规调用，统一改用标准的 `showToast`）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/LibraryPanel.cpp`

```diff
<<<<<<< SEARCH
void LibraryPanel::loadLibrary() {
    if (!m_model) return;
    m_isLoading = true;
    m_model->clear();
=======
void LibraryPanel::loadLibrary() {
    if (!m_model) return;

    // 记录刷新前选中的分类或智能文件夹节点 ID，刷新后精准恢复高亮选中
    int savedSelectedNodeId = 0;
    if (m_treeView && m_treeView->currentIndex().isValid()) {
        savedSelectedNodeId = m_treeView->currentIndex().data(Qt::UserRole + 1).toInt();
    }

    m_isLoading = true;
    m_model->clear();
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // 触发智能文件夹异步后台计算
    if (!smartFolders.isEmpty()) {
        m_currentCountReqId++;
        updateSmartFolderCountsAsync(m_currentCountReqId, smartFolders);
    }
=======
    // 触发智能文件夹异步后台计算
    if (!smartFolders.isEmpty()) {
        m_currentCountReqId++;
        updateSmartFolderCountsAsync(m_currentCountReqId, smartFolders);
    }

    // 恢复树节点的选中高亮状态，确保持续显示高亮
    if (savedSelectedNodeId != 0 && m_treeView) {
        QStandardItem* targetItem = findItemByNodeId(m_model->invisibleRootItem(), savedSelectedNodeId);
        if (targetItem) {
            m_treeView->setCurrentIndex(targetItem->index());
        }
    }
>>>>>>> REPLACE
```

---

### 3.2 `QuarkMeta_Source/src/meta/LibraryService.cpp`

```diff
<<<<<<< SEARCH
    if (anyInLibrary) {
        QIcon removeIcon = UiHelper::getIcon("close", QColor("#EEEEEE"), 18);
        QAction* action = parentMenu->addAction(removeIcon, "从“库”中移除");

        connect(action, &QAction::triggered, receiver ? receiver : parentMenu, [this, targetPaths]() {
            if (removePathsFromLibrary(targetPaths)) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已将 %1 个项目从库中移除").arg(targetPaths.size()), 1500, QColor("#2ecc71"));
            } else {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "移除失败", 1500, QColor("#e81123"));
            }
        });

        return action;
    } else {
        QIcon addIcon = UiHelper::getIcon("Database-1.svg", QColor("#EEEEEE"), 18);
        QAction* action = parentMenu->addAction(addIcon, "添加至“库”");

        connect(action, &QAction::triggered, receiver ? receiver : parentMenu, [this, targetPaths]() {
            if (addPathsToCategory(-2, targetPaths)) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已将 %1 个项目归入未分类").arg(targetPaths.size()), 1500, QColor("#2ecc71"));
            } else {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "归入失败", 1500, QColor("#e81123"));
            }
        });

        return action;
    }
=======
    if (anyInLibrary) {
        QIcon removeIcon = UiHelper::getIcon("close", QColor("#EEEEEE"), 18);
        QAction* action = parentMenu->addAction(removeIcon, "从“库”中移除");

        connect(action, &QAction::triggered, receiver ? receiver : parentMenu, [this, targetPaths]() {
            if (removePathsFromLibrary(targetPaths)) {
                ToolTipOverlay::instance()->showToast(QString("已将 %1 个项目从库中移除").arg(targetPaths.size()), 1500, QColor("#2ecc71"));
            } else {
                ToolTipOverlay::instance()->showToast("移除失败", 1500, QColor("#e81123"));
            }
        });

        return action;
    } else {
        QIcon addIcon = UiHelper::getIcon("Database-1.svg", QColor("#EEEEEE"), 18);
        QAction* action = parentMenu->addAction(addIcon, "添加至“库”");

        connect(action, &QAction::triggered, receiver ? receiver : parentMenu, [this, targetPaths]() {
            if (addPathsToCategory(-2, targetPaths)) {
                ToolTipOverlay::instance()->showToast(QString("已将 %1 个项目归入未分类").arg(targetPaths.size()), 1500, QColor("#2ecc71"));
            } else {
                ToolTipOverlay::instance()->showToast("归入失败", 1500, QColor("#e81123"));
            }
        });

        return action;
    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建命令
```bash
cmake -B build -S QuarkMeta_Source
cmake --build build --target QuarkMeta -j4
```

### 4.2 运行时验收标准
1. **库分类持续高亮选中**：
   - 切换到“库”选项卡，点击选中某一分类节点（例如“全部数据”或某一自定义分类）；
   - 右键选中该分类下的一个或多个项目，点击“从‘库’中移除”；
   - 移除成功后，观察左侧库选项卡分类树：分类计数完成刷新的同时，当前选中的分类节点**持续保持高亮选中状态**，没有出现高亮消失或丢失选中的问题。
2. **ToolTipOverlay 固定居中无鼠标跟随**：
   - 执行“从‘库’中移除”或“添加至‘库’”时，观察弹出的绿色成功反馈气泡；
   - 浮层气泡在活跃内容面板顶部正中位置水平居中呈现，**绝对不跟随鼠标光标移动或在光标附近漂移**，完全符合 Memories.md 第 21 章规范。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **复用 `LibraryPanel::findItemByNodeId`**：直接复用类内已有的递归查找节点函数，杜绝另起炉灶编写重复的遍历代码；
2. **复用 `ToolTipOverlay::showToast`**：直接复用标准的 Toast 接口，依托其内部已有的面板正中居中和标题栏下方 10px 坐标算法，消除重复位置计算。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 唯一数据源 | 派生副本/UI表现 | 同步与刷新方式 |
| :--- | :--- | :--- | :--- |
| 当前选中节点 ID | `LibraryPanel::m_treeView->currentIndex()` 对应模型的 `Qt::UserRole + 1` | `savedSelectedNodeId` 局部变量 | 刷新前提取，重建后立即通过 `setCurrentIndex` 恢复 |
| 反馈提示位置 | `ToolTipOverlay` 内部计算的活跃 `ContentPanel` 居中坐标 | 气泡位置 `QPoint` | 绝对禁止使用 `QCursor::pos()`，通过 `showToast` 自动完成计算 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部调用接口核查**：
   - `ToolTipOverlay::instance()->showToast(const QString&, int, const QColor&)` 签名在 `ToolTipOverlay.h` 中真实存在且为 `public`；
   - `LibraryPanel::findItemByNodeId` 在 `LibraryPanel.h` 中已声明为私有成员函数，可在 `LibraryPanel::loadLibrary` 中合法调用。
2. **访问级别**：全部合法，无跨类私有成员访问越权。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `LibraryPanel.cpp` 已包含：
   - `#include "LibraryPanel.h"`
   - `#include "ToolTipOverlay.h"`
2. `LibraryService.cpp` 已包含：
   - `#include "LibraryService.h"`
   - `#include "../ui/ToolTipOverlay.h"`
3. 未引入新类型，包含链完整闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 检查项 | 状态 | 说明 |
| :--- | :--- | :--- |
| 1. SEARCH 块原文真实存在且唯一匹配 | 满足 | 经 `tools/plan_verify/verify_plan.py` 自动化检测，所有 SEARCH 块均唯一匹配 |
| 2. 外部成员函数签名一致 | 满足 | `showToast` 签名与头文件完全一致 |
| 3. 新增接口全链核查 | 不涉及 | 本次不新增对外公开接口 |
| 4. #include 完整闭合 | 满足 | 头文件包含链完备 |
| 5. CMakeLists.txt 同步注册 | 不涉及 | 无新建或删除源码文件 |
| 6. 跨类访问控制权限合法 | 满足 | 无越权访问 |
| 7. 方案未触及范围外文件 | 满足 | 仅涉及 `LibraryPanel.cpp` 和 `LibraryService.cpp` |
| 8. 杜绝盲目修改/架构警告核查 | 满足 | 严格遵循 Memories.md 第 21 与 35 章节规范 |
| 9. 异步并发安全与生命周期 | 不涉及 | 本次不涉及多线程并发 |
| 10. 10 大必备章节完整性 | 满足 | 完整包含 Chapter 1 至 Chapter 10 |

### 自动化验证执行输出日志：
```text
=== [Plan Verification] Starting verification on: Implementation Plan/Library_Selection_Preserve_And_ToolTip_Fix_Plan.md ===
[+] Chapter completeness check (All 10 chapters present): PASSED
[+] Hallucinated API / Macro check: PASSED (None found)
[+] Total SEARCH/REPLACE blocks found: 3
[+] SEARCH block in QuarkMeta_Source/src/ui/LibraryPanel.cpp: Exactly 1 unique match.
[+] SEARCH block in QuarkMeta_Source/src/ui/LibraryPanel.cpp: Exactly 1 unique match.
[+] SEARCH block in QuarkMeta_Source/src/meta/LibraryService.cpp: Exactly 1 unique match.
[+] Physical source code matching: ALL PASSED
=== [Plan Verification] SUCCESS: Implementation Plan/Library_Selection_Preserve_And_ToolTip_Fix_Plan.md satisfies all verification rules! ===
```

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 交互与行为零篡改核查
- 库分类与智能文件夹原有的拖拽排序、重命名、置顶以及右键菜单功能保持 100% 行为一致；
- “从‘库’中移除”与“添加至‘库’”的数据事务处理逻辑保持不变。

### 10.2 样式数值零篡改核查
- Toast 动画超时时间 `1500` 毫秒保持完全一致；
- 成功边框色 `#2ecc71` 与失败边框色 `#e81123` 保持完全一致；
- 树控件的项间距、字体及图标大小保持完全一致。
