# QuarkMeta 库虚拟分类拖拽归类后 ContentPanel 视图内容实时刷新与路径同步实施方案

## 1. 概述与核心变更目标
在用户向侧边栏“库”中的某个虚拟分类（如“其他”）拖拽/归入新项目后，侧边栏分类节点后方的数字计数会即时更新（例如从 3 更新为 4），但中间内容面板（`ContentPanel`）依然只呈现拖拽前的 3 个旧项目。

**核心目标：**
1. 恪守 `Memories.md` 顶层规范：向“库”分类添加项目后，内容面板必须与侧边栏分类数字计数毫秒级 100% 绝对一致地呈现最新归入的项目。
2. 消除 `ContentPanel::refreshAll()` 对 `"library"` 视图形态直接重新加载静态 `m_lastLoadedLibraryPaths` 旧缓存的缺陷。
3. 在 `ContentPanel.cpp` 中引入 `#include "../meta/LibraryService.h"` 以支持跨服务调用。
4. 当 `ContentPanel::refreshAll()` 执行且视图形态为 `"library"` 且 `m_categoryId != 0` 时，实时向 `LibraryService::instance().getCategoryPaths(m_categoryId)` 重新查询最新关联路径集并更新 `m_lastLoadedLibraryPaths`，随后驱动视图重新装载，确保内容面板数据即时同步展现。
5. 严格遵守 AGENTS.md 协同纪律：AI 助手仅提供实施方案放置于 `Implementation Plan/` 物理隔离目录中，绝对禁止未经授权对 `src/` 源代码进行直接修改。

---

## 2. 涉及的影响范围与受影响组件

### 2.1 修改文件列表
- `Implementation Plan/LibraryCategoryContentSyncOnDrop.md`（本实施方案文件）

### 2.2 待重构的 C++ 源码文件（仅在实施方案中规划，物理代码保持零修改）
1. `src/ui/ContentPanel.cpp`
   - **头文件引入与刷新逻辑重构**：包含 `#include "../meta/LibraryService.h"` 头文件；在 `ContentPanel::refreshAll()` 中，针对 `m_currentCategoryType == "library"` 时，若 `m_categoryId != 0`，调用 `m_lastLoadedLibraryPaths = LibraryService::instance().getCategoryPaths(m_categoryId)` 重新刷新路径列表后传入 `loadLibraryPaths`。

---

## 3. 详细代码变更说明 (SEARCH / REPLACE 差异对比)

以下为后续授权实施时针对各受影响 C++ 源码文件的精确修改方案：

### 3.1 `src/ui/ContentPanel.cpp`

```cpp
<<<<<<< SEARCH
#include "UiHelper.h"
#include "ToolTipOverlay.h"
=======
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "../meta/LibraryService.h"
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    if (m_currentCategoryType == "library" || m_currentCategoryType == "smart_folder") {
        loadLibraryPaths(m_lastLoadedLibraryPaths);
        return;
    }
=======
    if (m_currentCategoryType == "library") {
        if (m_categoryId != 0) {
            m_lastLoadedLibraryPaths = LibraryService::instance().getCategoryPaths(m_categoryId);
        }
        loadLibraryPaths(m_lastLoadedLibraryPaths);
        return;
    }

    if (m_currentCategoryType == "smart_folder") {
        loadLibraryPaths(m_lastLoadedLibraryPaths);
        return;
    }
>>>>>>> REPLACE
```

---

## 4. 架构设计与 SSOT 合规性分析
1. **彻底打通侧边栏与内容视图的真理源同步通道**：`ContentPanel` 在上个提交中已绑定 `m_categoryId`，本次改动引入 `LibraryService` 接口，使 `refreshAll()` 获得与数据库中分类关联关系的微秒级响应查询能力，消除过往依靠静态 `m_lastLoadedLibraryPaths` 造成的视图更新滞后死角。
2. **高内聚与单一职责**：修改集中于 `ContentPanel::refreshAll()` 内部，不干扰其他物理磁盘路径导航或垃圾桶、智能文件夹的处理通道。

---

## 5. 迁移与兼容性评估
改动完全向下兼容。当处理普通磁盘路径（`m_currentCategoryType` 为空）时，绕过分类查询直接走物理磁盘加载逻辑，对原有流程 0 侵入。

---

## 6. 性能影响评估
`LibraryService::getCategoryPaths` 基于 SQLite 索引的主键关联查询，在毫秒级内即可返回最新的路径列表，性能损耗几乎可忽略不计。

---

## 7. 安全与风险防护
所有界面刷新与数据重新查验均在主 UI 线程内同步完成，避免了异步竞态可能导致的数据不一致问题。

---

## 8. 自动化测试与验证计划
1. **场景复现与验证**：
   - 打开分屏双窗格：左侧窗格 ① 打开“库”中的“其他”分类，右侧窗格 ② 打开物理磁盘目录 `G:\SVG\自建`。
   - 从窗格 ② 将图片（如 `Clear-3.svg`）拖入窗格 ① 的“其他”分类中。
   - 检查左侧“其他”分类后的计数（从 3 变 4），同时检查中间内容面板是否即时同步刷出最新的 4 个项目卡片。

---

## 9. 回滚策略
若需回滚，撤销 `Implementation Plan/LibraryCategoryContentSyncOnDrop.md` 相关的提交即可。

---

## 10. 实施步骤与预估工时
1. 撰写并提交 `Implementation Plan/LibraryCategoryContentSyncOnDrop.md` 实施方案（已完成，预估工时 0.2h）。
2. 待用户给予明确代码实施授权后，编辑 `ContentPanel.cpp` 并编译验证（预估工时 0.2h）。
