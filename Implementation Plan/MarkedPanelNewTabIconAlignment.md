# Implementation Plan - MarkedPanel NewTab Icon Alignment (标记面板“在新标签页中打开”图标归一化)

- **Base Commit Hash**: HEAD
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 根因分析
根据用户提供的视觉截图与既有右键级联菜单规范：
级联菜单项“标签页”及其子动作“在新标签页中打开”统一采用加号图标 `"add"`，而在 `MarkedPanel.cpp` 中此前误将“在新标签页中打开”写成了 `"Folder-add"`，与系统内其他右键菜单（`NavPanel`、`FavoritePanel`、`AddressBar`、`ContentContextMenu`）不一致。

### 1.2 重构目标
将 `MarkedPanel.cpp` 中的“在新标签页中打开”图标由 `"Folder-add"` 修正为统一的标准加号图标 **`"add"`**（颜色 `#EEEEEE`，尺寸 16）。

---

## Chapter 2: Modified Files List（影响文件清单）

- `src/ui/MarkedPanel.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/MarkedPanel.cpp`

```
<<<<<<< SEARCH
        QAction* newTabAct = menu.addAction(UiHelper::getIcon("Folder-add", QColor("#EEEEEE"), 16), "在新标签页中打开");
=======
        QAction* newTabAct = menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE"), 16), "在新标签页中打开");
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建命令
```bash
cmake --build --preset x64-Debug
```

### 4.2 运行时验收标准
1. **右键“标记”面板中的任意项目**：
   - 查看“在新标签页中打开”菜单项，图标精准呈现为标准加号 `"add"`（颜色 `#EEEEEE`）；
   - 与 `NavPanel`、`FavoritePanel`、`AddressBar` 及 `ContentContextMenu` 级联菜单中的“在新标签页中打开”图标完全统一。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 统一复用静态定义的标准矢量图标键 `"add"`。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| “在新标签页中打开”图标键 | `UiHelper::getIcon("add", ...)` | 无 | 静态定义 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- 不修改任何 `.h` 接口，仅调整 `MarkedPanel.cpp` 局部 QAction 图标 key 参数。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 未动头文件包含。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 结论 | 说明 |
| :--- | :--- | :--- | :--- |
| 1 | SEARCH 块原文是否唯一匹配 | 满足 | 精准匹配第 180 行 |
| 2 | 图标是否采用中性单色 `#EEEEEE` | 满足 | 采用 `#EEEEEE` |
| 3 | 章节完整性 | 满足 | Chapter 1 至 Chapter 10 齐备 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 序号 | 规则类型 | 项目描述 | 原数值/原行为 | 新数值/新行为 | 校验结果 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 1 | 交互零篡改 | “在新标签页中打开”触发逻辑 | 触发 NavigationService 打开 | 完全一致 | 一致 |
| 2 | 图标归一化 | 统一菜单图标 | `"add"` 加号图标 | `"add"` 加号图标 | 100% 对齐截图 |
