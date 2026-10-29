# Implementation Plan - Selection Highlight Unification (全软件选中高亮颜色归一化与统一)

## Chapter 1: Overview（概述与根因分析）

### 1. 现象与问题分析
用户反馈侧边栏与内容区域选中高亮颜色不一致：内容区与导航树视图呈现亮蓝色/淡蓝灰色高亮，而侧边栏（“收藏夹” `FavoritePanel` 与“库” `LibraryPanel`）选中时呈现暗灰色 (`#37373D`) 高亮，破坏了 UI 整体一致性。

### 2. 根因剖析 (Root Cause)
经过对全软件绘图代理（Delegate）源码与 QSS 样式的全局查重比对：
- **内容区与导航树**：`TreeItemDelegate.h` 与 `ColumnItemDelegate.cpp` 中选中的背景色均使用了统一的主题蓝色 `#378ADD`（或 `#378ADD` 叠加透明度）。
- **侧边栏 (收藏夹与库)**：`FavoritePanel.cpp` (`FavoriteItemDelegate::paint`) 和 `LibraryPanel.cpp` (`LibraryItemDelegate::paint`) 在判断 `opt.state & QStyle::State_Selected` 时，硬编码写死了背景绘制为暗灰色 `QColor("#37373D")`：
  ```cpp
  if (opt.state & QStyle::State_Selected) {
      painter->fillRect(opt.rect, QColor("#37373D")); // 👈 导致侧边栏选中高亮呈灰色的根源
  }
  ```

### 3. 重构与统一目标
将 `FavoriteItemDelegate` 和 `LibraryItemDelegate` 里的 `QColor("#37373D")` 修改统一为主题蓝色 **`QColor("#378ADD")`**，使全软件所有侧边栏（导航树、收藏夹、库）与右侧内容区域的选中高亮色彩 100% 绝对统一。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/FavoritePanel.cpp`
2. `src/ui/LibraryPanel.cpp`
3. `Memories.md`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
    // 背景高亮
    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor("#37373D"));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }
=======
    // 背景高亮
    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor("#378ADD"));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }
>>>>>>> REPLACE
```

### 2. `src/ui/LibraryPanel.cpp`

```
<<<<<<< SEARCH
    if (isDropTarget) {
        QColor dropBg("#3498db");
        dropBg.setAlphaF(0.35f);
        painter->fillRect(opt.rect, dropBg);
    } else if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor("#37373D"));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }
=======
    if (isDropTarget) {
        QColor dropBg("#3498db");
        dropBg.setAlphaF(0.35f);
        painter->fillRect(opt.rect, dropBg);
    } else if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor("#378ADD"));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 编译命令
在 Sandbox 环境运行标准构建命令：
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### 2. 运行时验收标准
1. 启动 QuarkMeta。
2. 依次点击左侧“收藏夹”中的任意项目、“库”中的分类项目以及“目录导航”中的文件夹项。
3. 观察选中高亮底色，确认全软件所有侧边栏与内容项选中高亮统一呈现 **`#378ADD`** 主题蓝色，彻底无暗灰色 `#37373D` 不一致现象。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 Qt 原生 `QStyle::State_Selected` 状态校验与 `QPainter::fillRect` 绘制通道。
- 零新增私有控件或另起炉灶类，仅修正底层数值与色彩定义。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生/视图副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 选中高亮色值 | `StyleLibrary.h` / 全局 QSS `#378ADD` | `FavoriteItemDelegate` / `LibraryItemDelegate` | SSOT 静态定义 ➔ Delegate 绘制渲染 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- 无接口签名修改，仅为内部 `paint` 方法重绘逻辑中的色值更正。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 未修改任何 `#include` 头文件包含，头文件包含链闭合且完全保持原样。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否物理阅读了涉及的 `.h`/`.cpp` 文件？ | 是 | `FavoritePanel.cpp`, `LibraryPanel.cpp` |
| 2 | SEARCH 块是否在目标文件中真实且唯一存在？ | 是 | 已通过 `grep` 验证 |
| 3 | 调用的外部成员函数是否在 `.h` 中真实存在？ | 是 | `QPainter::fillRect`, `QStyle::State_Selected` |
| 4 | 新增/重构接口的声明定义调用点是否一致？ | 是 | 不涉及新接口 |
| 5 | 头文件包含链改动后 `.cpp` 类型是否闭合？ | 是 | 未改动头文件包含 |
| 6 | 新建/删除文件是否在 `CMakeLists.txt` 中注册？ | 是 | 未新建或删除文件 |
| 7 | 跨类调用的成员访问级别是否允许？ | 是 | 均为类内部私有/受保护绘图方法 |
| 8 | 是否未触及方案范围外的文件？ | 是 | 严格限定在 2 个修改文件内 |
| 9 | 方案中是否有未解答的假设或模糊需求？ | 否 | 需求已与用户明确并达成共识 |
| 10 | 是否无内联 QSS 样式硬编码违规？ | 是 | C++ 代理绘图使用 `QColor` 标准画刷 |
| 11 | 是否完整包含 10 大必备章节？ | 是 | 本方案包含全部 10 大章节 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 修改点 | 既有行为/数值 | 新行为/数值 | 是否零篡改/符合用户共识 |
| :--- | :--- | :--- | :--- |
| 收藏夹选中背景色 | `#37373D` (暗灰色) | `#378ADD` (全局统一高亮蓝) | 符合用户统一高亮蓝共识 |
| 库分类选中背景色 | `#37373D` (暗灰色) | `#378ADD` (全局统一高亮蓝) | 符合用户统一高亮蓝共识 |
