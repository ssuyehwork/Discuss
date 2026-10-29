# Implementation Plan - Selection Highlight Unification (全软件选中高亮颜色与覆盖关系归一化)

## Chapter 1: Overview（概述与根因分析）

### 1. 现象与问题分析
用户反馈侧边栏与内容区域选中高亮颜色及覆盖关系不一致：
1. 目录导航 (`NavPanel`) 与右侧内容树视图 (`TreeItemDelegate`) 选中时呈柔和的低饱和 15% 半透明蓝色背景，且图标与文本位于高亮背景之上面板层次分明。
2. 收藏夹 (`FavoritePanel`) 与库 (`LibraryPanel`) 之前使用了强烈的纯色不透明色块填充整行，导致视觉上产生色块遮挡或浮于顶层的不协调感。

### 2. 根因剖析 (Root Cause)
经过对绘图代理（Delegate）源码分析：
- **`TreeItemDelegate.h` (目录导航与内容区)**：选中时在底层绘制 `#378ADD` 叠加 15% 透明度的画刷，图标和文本随后在其上方绘制：
  ```cpp
  if (selected) {
      bg = QColor("#378ADD");
      bg.setAlphaF(0.15f);
  }
  ```
- **`FavoritePanel.cpp` & `LibraryPanel.cpp` (收藏夹与库)**：之前直接使用不透明纯色 `QColor("#37373D")` / `QColor("#378ADD")` 充填整行，缺少 15% 半透明平滑处理。

### 3. 重构与统一目标
将 `FavoriteItemDelegate` 和 `LibraryItemDelegate` 的选中背景绘制统一修改为 `#378ADD` 15% 半透明蓝色（`QColor bg("#378ADD"); bg.setAlphaF(0.15f);`），并在绘制文本和图标前最底层填充矩形，确保全软件所有侧边栏与内容视图高亮风格与图层覆盖关系 100% 绝对一致。

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
        QColor selBg("#378ADD");
        selBg.setAlphaF(0.15f);
        painter->fillRect(opt.rect, selBg);
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
        QColor selBg("#378ADD");
        selBg.setAlphaF(0.15f);
        painter->fillRect(opt.rect, selBg);
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
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### 2. 运行时验收标准
1. 启动 QuarkMeta。
2. 在“目录导航”、“收藏夹”和“库”之间切换点击节点。
3. 检查高亮背景，确认全软件所有侧边栏项选中高亮背景统一呈现 `#378ADD` 15% 半透明蓝色，且图标与文字位于背景之上，层级分明且无覆盖遮挡感。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 Qt 原生 `QStyle::State_Selected` 校验与 `QColor::setAlphaF(0.15f)` 半透明通道。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生/视图副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 选中高亮色值 | `TreeItemDelegate.h` (`#378ADD` 0.15 alpha) | `FavoriteItemDelegate` / `LibraryItemDelegate` | Delegate 底层自绘对齐 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- 无接口签名修改，仅为 Delegate 重绘逻辑数值对齐。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 未修改任何头文件包含。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否物理阅读了涉及的 `.h`/`.cpp` 文件？ | 是 | `FavoritePanel.cpp`, `LibraryPanel.cpp`, `TreeItemDelegate.h` |
| 2 | SEARCH 块是否在目标文件中真实且唯一存在？ | 是 | 已验证匹配 |
| 3 | 调用的外部成员函数是否在 `.h` 中真实存在？ | 是 | `QColor::setAlphaF` |
| 4 | 新增/重构接口的声明定义调用点是否一致？ | 是 | 不涉及新接口 |
| 5 | 头文件包含链改动后 `.cpp` 类型是否闭合？ | 是 | 未改动头文件包含 |
| 6 | 新建/删除文件是否在 `CMakeLists.txt` 中注册？ | 是 | 未新建或删除文件 |
| 7 | 跨类调用的成员访问级别是否允许？ | 是 | 内部绘制方法 |
| 8 | 是否未触及方案范围外的文件？ | 是 | 仅针对 2 个 Panel 文件与 Memories |
| 9 | 方案中是否有未解答的假设或模糊需求？ | 否 | 需求与逻辑已获用户明确认可 |
| 10 | 是否无内联 QSS 样式硬编码违规？ | 是 | 绘制透明度与色值对齐统一 |
| 11 | 是否完整包含 10 大必备章节？ | 是 | 完整包含 10 大章节 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 修改点 | 既有行为/数值 | 新行为/数值 | 是否零篡改/符合用户共识 |
| :--- | :--- | :--- | :--- |
| 收藏夹选中高亮 | 硬编码 `#37373D` / 不透明 | `#378ADD` (15% 透明度) | 100% 对齐 TreeItemDelegate 视觉，用户认可 |
| 库分类选中高亮 | 硬编码 `#37373D` / 不透明 | `#378ADD` (15% 透明度) | 100% 对齐 TreeItemDelegate 视觉，用户认可 |
