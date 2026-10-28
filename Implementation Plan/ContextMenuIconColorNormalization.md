# 右键菜单图标颜色中性单色归一化实施方案 (ContextMenuIconColorNormalization.md)

### 已读物理证据清单
1. **物理读取文件列表**：
   - `src/meta/MarkedFolderService.cpp` (只读确认 `buildMarkAction` 中 `Destination.svg` 传入了 `#ff551c` 橙色图标)
   - `src/ui/LibraryPanel.cpp` (只读确认 `onCategoryContextMenu` 中 `unpinAct` 传入了 `#F39C12`，`presetTagAct` 传入了 `#9B59B6`)
   - `src/ui/FavoritePanel.cpp` (只读确认 `onFavoriteContextMenu` 中 `actAutoRec` 传入了 `#2ecc71` 动态彩色)
2. **头文件原样签名**：
   - `UiHelper::getIcon(const QString& key, const QColor& color, int size)`
3. **既有 API 检索日志**：
   - `grep -rn "addAction.*getIcon" src/`

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题描述与根因分析
根据 `Memories.md` 第 1.3 节与第 16.1 节的顶层视觉红线：“**无彩色单色绝对统一（绝对禁止例外）：全局所有右键菜单与上下文菜单项图标统一且仅可采用同一个中性单色设计 (`#EEEEEE`)，绝对禁止在右键菜单任何图标上出现或使用任何特殊色彩/着色（包括但不限于 `#ff551c`、红、黄、蓝、绿等），绝对不存在任何例外场景（例如‘标记为目标文件夹’项也必须使用 `#EEEEEE` 中性单色）**”。

现有代码中存在 3 处违背该红线规则的右键菜单图标着色遗留点：
1. `MarkedFolderService::buildMarkAction` 在为右键上下文菜单添加“标记为目标文件夹”/“取消标记”项时，使用了 `#ff551c` 品牌橙/置顶橙图标。
2. `LibraryPanel::onCategoryContextMenu` 在为分类右键菜单添加“从‘快速访问’移除”与“设置预设标签”项时，分别使用了 `#F39C12` 与 `#9B59B6` 彩色图标。
3. `FavoritePanel::onFavoriteContextMenu` 在为收藏项右键菜单添加“关闭/开启自动递归”项时，使用了 `#2ecc71` 动态绿色图标。

### 1.2 重构目标
将上述所有右键上下文菜单项的图标色彩统一更正为唯一合法的 **`QColor("#EEEEEE")`** 中性单色，确保全系统右键菜单视觉风格严肃沉静、100% 遵守红线契约。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/meta/MarkedFolderService.cpp` (修改)
2. `src/ui/LibraryPanel.cpp` (修改)
3. `src/ui/FavoritePanel.cpp` (修改)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/meta/MarkedFolderService.cpp`

```
<<<<<<< SEARCH
    QString label = allMarked ? "取消标记" : "标记为目标文件夹";
    QIcon icon = UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 18);

    QAction* action = parentMenu->addAction(icon, label);
=======
    QString label = allMarked ? "取消标记" : "标记为目标文件夹";
    QIcon icon = UiHelper::getIcon("Destination.svg", QColor("#EEEEEE"), 18);

    QAction* action = parentMenu->addAction(icon, label);
>>>>>>> REPLACE
```

### 3.2 `src/ui/LibraryPanel.cpp`

```
<<<<<<< SEARCH
    // 绘制置顶/取消置顶菜单项
    if (isPinned) {
        QAction* unpinAct = menu.addAction(UiHelper::getIcon("pin_vertical", QColor("#F39C12")), "从“快速访问”移除");
        connect(unpinAct, &QAction::triggered, this, [this, nodeId]() {
            onPinCategoryTriggered(nodeId, false);
        });
    } else {
        QAction* pinAct = menu.addAction(UiHelper::getIcon("pin_tilted", QColor("#EEEEEE")), "添加至“快速访问”");
        connect(pinAct, &QAction::triggered, this, [this, nodeId]() {
            onPinCategoryTriggered(nodeId, true);
        });
    }

    menu.addSeparator();

    QAction* newSubCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建子分类");
    connect(newSubCatAct, &QAction::triggered, this, [this, nodeId]() {
        createAndEditCategory(nodeId);
    });

    QAction* presetTagAct = menu.addAction(UiHelper::getIcon("tag_filled", QColor("#9B59B6")), "设置预设标签");
=======
    // 绘制置顶/取消置顶菜单项
    if (isPinned) {
        QAction* unpinAct = menu.addAction(UiHelper::getIcon("pin_vertical", QColor("#EEEEEE")), "从“快速访问”移除");
        connect(unpinAct, &QAction::triggered, this, [this, nodeId]() {
            onPinCategoryTriggered(nodeId, false);
        });
    } else {
        QAction* pinAct = menu.addAction(UiHelper::getIcon("pin_tilted", QColor("#EEEEEE")), "添加至“快速访问”");
        connect(pinAct, &QAction::triggered, this, [this, nodeId]() {
            onPinCategoryTriggered(nodeId, true);
        });
    }

    menu.addSeparator();

    QAction* newSubCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建子分类");
    connect(newSubCatAct, &QAction::triggered, this, [this, nodeId]() {
        createAndEditCategory(nodeId);
    });

    QAction* presetTagAct = menu.addAction(UiHelper::getIcon("tag_filled", QColor("#EEEEEE")), "设置预设标签");
>>>>>>> REPLACE
```

### 3.3 `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
            bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
            QAction* actAutoRec = menu.addAction(UiHelper::getIcon("layers", isAutoRec ? QColor("#2ecc71") : QColor("#EEEEEE"), 18),
                                                 isAutoRec ? "关闭自动递归" : "开启自动递归");
=======
            bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
            QAction* actAutoRec = menu.addAction(UiHelper::getIcon("layers", QColor("#EEEEEE"), 18),
                                                 isAutoRec ? "关闭自动递归" : "开启自动递归");
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建命令
使用 CMake 与 MSVC 进行项目增量构建：
```bash
cmake --build --preset x64-Debug
```

### 4.2 运行时验收标准
1. **右键文件/文件夹**：在网格、列表或分栏视图中右键任意文件/文件夹，弹出上下文菜单，“标记为目标文件夹”项图标应呈现统一的中性灰白 `#EEEEEE` 色，不再为橙色 `#ff551c`。
2. **右键库分类项**：在“库”面板中右键任意已固定至快速访问的分类或普通分类，弹出菜单中的“从‘快速访问’移除”与“设置预设标签”项图标均呈现统一的 `#EEEEEE` 单色。
3. **右键收藏夹项**：在“收藏夹”面板中右键任意开启自动递归的文件夹，弹出菜单中的“关闭自动递归”项图标呈现统一的 `#EEEEEE` 单色。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- **图标生成通道**：继续统一通过官方静态通道 `UiHelper::getIcon(key, color, size)` 获取图标。
- **菜单应用通道**：继续统一调用 `UiHelper::applyMenuStyle(&menu)` 应用全局样式，不另起炉灶。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 右键菜单图标样式 | `UiHelper::getIcon` + `#EEEEEE` | 无 | 静态控制，零副本 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- 本重构仅调整 `MarkedFolderService.cpp`、`LibraryPanel.cpp` 及 `FavoritePanel.cpp` 函数内部创建 `QAction` 时传递的 `QColor` 参数，不增加、删除或改变任何类的方法成员及 `.h` 接口签名。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 未修改任何文件头的 `#include` 指令，头文件包含链保持绝对完整。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 结论 | 位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否通过第 0 关并与用户达成共识 | 满足 | 探讨阶段已明确“右键菜单统一中性单色 #EEEEEE”并获得用户“达成共识”与“批准” |
| 2 | 已读物理证据清单是否完整包含物理读日志 | 满足 | 见方案开头已读证据清单 |
| 3 | SEARCH 块原文是否在当前目标文件中真实存在且唯一匹配 | 满足 | 精准匹配目标文件行 |
| 4 | 修改与新增的代码是否符合 C++17 规范 | 满足 | Qt/C++17 语法 |
| 5 | 是否引入 Win32 强抢焦点或不合规的事件过滤器 | 满足 | 零引入 |
| 6 | 是否篡改既有对外公开的 `.h` 接口签名 | 满足 | 未修改任何 `.h` 签名 |
| 7 | 是否修改 `resources/style.qss` 之外的内联样式 | 满足 | 零内联样式修改 |
| 8 | 是否包含完整 10 大章节 | 满足 | 包含 Chapter 1 至 Chapter 10 |
| 9 | 自动化校验脚本测试日志 | 满足 | 见下文 verify_plan.py 验证结果 |

### 自动化验证工具输出日志
```bash
python3 tools/plan_verify/verify_plan.py "Implementation Plan/ContextMenuIconColorNormalization.md"
[PASS] Plan satisfies all chapter and SEARCH/REPLACE format specifications.
```

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 序号 | 规则类型 | 项目描述 | 原数值/原行为 | 新数值/新行为 | 校验结果 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 1 | 交互零篡改 | 右键菜单功能逻辑 | 触发“标记为目标文件夹”、“移除快速访问”、“设置预设标签”、“切换自动递归” | 完全一致，零行为改变 | 一致 |
| 2 | 数值零篡改 | 统一右键菜单图标颜色 | `#EEEEEE` 中性单色 | `#EEEEEE` 中性单色 | 严格遵循 Memories.md 红线 |
