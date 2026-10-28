# QuarkMeta 右键菜单图标颜色规范化与“在新标签页中打开”图标统一治理实施方案

## 1. 概述与核心变更目标
针对系统右键上下文菜单（ContextMenu）中部分菜单项存在违规彩色着色（如“标记为目标文件夹”项硬编码使用 `#ff551c`，部分面板使用 `#F39C12`、`#9B59B6`、`#2ecc71` 等高彩度着色）以及“在新标签页中打开”菜单项图标命名不规范（错用通用 `add` 图标而非专用的 `Folder-add` 图标）的问题，本实施方案旨在对全软件所有右键上下文菜单进行图标颜色与语义图标规范化重构。

**核心目标：**
1. 恪守 `Memories.md` 第 1.3 节规则 3 红线：全局所有右键上下文菜单图标必须 100% 统一使用 **`#EEEEEE`** 中性单色，彻底抹除任何特殊彩着色（包括 `#ff551c`）。
2. 恪守 `Memories.md` 第 1.3 节规则 8 规范：全软件右键菜单中所有“在新标签页中打开”项，矢量图标统一下沉并规范绑定为 **`Folder-add`**。
3. 严格遵循 AGENTS.md 1.0 节铁律：AI 助手仅输出实施方案放置于 `Implementation Plan/` 目录，绝对禁止未经授权直接修改 `src/` 中的 C++ 源代码。

---

## 2. 涉及的影响范围与受影响组件

### 2.1 修改文件列表
- `Memories.md`（已新增 1.3 节规则 8 关于“在新标签页中打开”图标规范的契约）
- `Implementation Plan/ContextMenuIconColorNormalization.md`（本实施方案文件）

### 2.2 待重构的 C++ 源码文件（仅在实施方案中规划，物理代码保持零修改）
1. `src/service/MarkedFolderService.cpp`
   - **问题**：`createMarkedFolderMenu` 中对“标记为目标文件夹”项（`actMark`）硬编码传入物理彩色 `#ff551c`。
   - **重构规划**：移除 `#ff551c` 硬编码，统一传入 `#EEEEEE` 中性单色。
2. `src/ui/LibraryPanel.cpp`
   - **问题**：右键菜单中“在新标签页中打开”项使用通用 `add` 图标且颜色为 `#F39C12`；“固定到顶端”项使用 `#9B59B6`。
   - **重构规划**：将“在新标签页中打开”图标由 `add` 改为 `Folder-add`，颜色统一修正为 `#EEEEEE`；将“固定到顶端”图标颜色统一修正为 `#EEEEEE`。
3. `src/ui/FavoritePanel.cpp`
   - **问题**：右键菜单中“在新标签页中打开”项使用 `#2ecc71` 彩色着色。
   - **重构规划**：将图标颜色统一修正为 `#EEEEEE`，确保图标使用 `Folder-add`。
4. `src/ui/MarkedPanel.cpp`
   - **问题**：“在新标签页中打开”菜单项图标键为 `add`。
   - **重构规划**：将图标键修正为 `Folder-add`，颜色维持 `#EEEEEE`。

---

## 3. 详细代码变更说明 (SEARCH / REPLACE 差异对比)

以下为后续授权实施时针对各受影响 C++ 源码文件的精确修改方案：

### 3.1 `src/service/MarkedFolderService.cpp`

```cpp
<<<<<<< SEARCH
        QAction* actMark = menu->addAction(UiHelper::getIcon("Destination", "#ff551c"), QObject::tr("标记为目标文件夹"));
=======
        QAction* actMark = menu->addAction(UiHelper::getIcon("Destination", "#EEEEEE"), QObject::tr("标记为目标文件夹"));
>>>>>>> REPLACE
```

### 3.2 `src/ui/LibraryPanel.cpp`

```cpp
<<<<<<< SEARCH
    QAction* openNewTabAct = menu.addAction(UiHelper::getIcon("add", "#F39C12"), tr("在新标签页中打开"));
    connect(openNewTabAct, &QAction::triggered, this, [this, index]() {
        QString path = m_model->filePath(index);
        emit requestOpenInNewTab(path);
    });

    QAction* pinAct = menu.addAction(UiHelper::getIcon("pin", "#9B59B6"), isPinned ? tr("取消固定") : tr("固定到顶端"));
=======
    QAction* openNewTabAct = menu.addAction(UiHelper::getIcon("Folder-add", "#EEEEEE"), tr("在新标签页中打开"));
    connect(openNewTabAct, &QAction::triggered, this, [this, index]() {
        QString path = m_model->filePath(index);
        emit requestOpenInNewTab(path);
    });

    QAction* pinAct = menu.addAction(UiHelper::getIcon("pin", "#EEEEEE"), isPinned ? tr("取消固定") : tr("固定到顶端"));
>>>>>>> REPLACE
```

### 3.3 `src/ui/FavoritePanel.cpp`

```cpp
<<<<<<< SEARCH
    QAction* openNewTabAct = menu.addAction(UiHelper::getIcon("Folder-add", "#2ecc71"), tr("在新标签页中打开"));
=======
    QAction* openNewTabAct = menu.addAction(UiHelper::getIcon("Folder-add", "#EEEEEE"), tr("在新标签页中打开"));
>>>>>>> REPLACE
```

### 3.4 `src/ui/MarkedPanel.cpp`

```cpp
<<<<<<< SEARCH
    QAction* actOpenNewTab = menu.addAction(UiHelper::getIcon("add", "#EEEEEE"), tr("在新标签页中打开"));
=======
    QAction* actOpenNewTab = menu.addAction(UiHelper::getIcon("Folder-add", "#EEEEEE"), tr("在新标签页中打开"));
>>>>>>> REPLACE
```

---

## 4. 架构设计与 SSOT 合规性分析
1. **中性单色绝对统一**：全系统所有右键菜单项图标统一且仅使用 `#EEEEEE`，杜绝任何特殊彩色，完全符合 `Memories.md` 1.3 节规则 3 的极致沉静深色暗黑美学。
2. **图标语义精准化**：“在新标签页中打开”统一使用 `Folder-add` 矢量图标，避免混淆通用加号与标签页/文件夹新建语义，符合 `Memories.md` 1.3 节规则 2 语义精准契约。
3. **物理隔离红线**：当前阶段不进行任何 `src/` 代码修改，所有方案均收录在 `Implementation Plan/` 目录中。

---

## 5. 迁移与兼容性评估
本次重构仅涉及 UI 右键菜单图标的颜色与图标 key 规范化调整，不改变任何信号槽连接或底层业务逻辑，不存在向下兼容性风险与数据库/配置文件迁移问题。

---

## 6. 性能影响评估
修改仅为图标渲染时的颜色参数与 SVG 名称查找，对 CPU、内存与磁盘 I/O 零附加开销，性能影响为 0ms。

---

## 7. 安全与风险防护
所有改动点均为只读式的 UI 样式修饰，不影响任何底层文件系统读写、数据库事务或线程通信，风险极低。

---

## 8. 自动化测试与验证计划
1. **代码审查验证**：检查所有 `src/` 目录下 `QMenu` 创建与 `UiHelper::getIcon` 调用，确保右键菜单路径中的图标颜色参数全部为 `#EEEEEE`。
2. **UI 视觉审查**：分别在“标记”面板、库面板、收藏夹面板与内容面板右键弹出菜单，确认所有图标均为亮灰色 `#EEEEEE`，无任何彩色突出项，且“在新标签页中打开”呈现为带有加号的文件夹矢量图标。

---

## 9. 回滚策略
如需回滚，只需在 Git 中撤销对 `Memories.md` 与 `Implementation Plan/ContextMenuIconColorNormalization.md` 的修改提交即可。

---

## 10. 实施步骤与预估工时
1. 在 `Memories.md` 中记录规范（已完成，工时 0.1h）。
2. 撰写并提交 `Implementation Plan/ContextMenuIconColorNormalization.md` 实施方案（已完成，工时 0.2h）。
3. 待用户明确授权并要求代码实施后，按照方案中的 SEARCH/REPLACE 差异应用修改并编译验证（预估工时 0.2h）。
