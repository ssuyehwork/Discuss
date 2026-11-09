# 清除与移除按钮图标归一化实施方案 (Clear Button Icon Normalization Implementation Plan)

**方案版本**：v1.0  
**日期**：2026-11-09  
**需求描述**：全软件针对数据条目的清除、移除、清空历史及删除规则按钮/菜单项，统一将既有引用的 `"close"` 替换为 `"clear-2.svg"`，与顶层窗口及窗格关闭操作的纯叉号 `"close"` 严格语义分离。

---

## 已读物理证据清单（第 1 关：Gate 1）

| 文件 | 确认已读 | 关键物理签名/行号 |
|---|---|---|
| `QuarkMeta_Source/src/ui/AddressHistoryPanel.cpp` | ✅ | 第 101 行：`btnRemove->setIcon(UiHelper::getIcon("close", QColor("#555555"), 12));` |
| `QuarkMeta_Source/src/ui/SearchHistoryPanel.cpp` | ✅ | 第 106 行：`btnRemove->setIcon(UiHelper::getIcon("close", QColor("#555555"), 12));` |
| `QuarkMeta_Source/src/ui/components/TagPill.cpp` | ✅ | 第 24 行：`m_closeBtn->setIcon(UiHelper::getIcon("close", QColor("#FFFFFF"), 14));` |
| `QuarkMeta_Source/src/ui/dialogs/SmartFolderRuleRow.cpp` | ✅ | 第 44 行：`m_btnDelete->setIcon(UiHelper::getIcon("close", QColor("#CCCCCC"), 14));` |
| `QuarkMeta_Source/src/ui/MarkedPanel.cpp` | ✅ | 第 347 行：`removeAct = menu.addAction(UiHelper::getIcon("close", ...), "从标记历史中移除");` 及第 373 行：`clearHistoryAct = menu.addAction(UiHelper::getIcon("close", ...), "清空标记历史");` |
| `QuarkMeta_Source/src/ui/LibraryMenuBuilder.cpp` | ✅ | 第 86 行：`clearAct = menu->addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "清空快速访问");` |
| `QuarkMeta_Source/src/ui/SvgIcons.h` | ✅ | 第 3018 行已存在 `"clear-2.svg"`，第 3019 行已存在 `"clear-2"` |

**防重复造轮子与根因检索记录**：
- `SvgIcons::icons["clear-2.svg"]` 为全软件标准带圆底背景镂空交叉的清除图标真理源；
- 原有业务将“数据条目清除/移除/清空”与“窗口关闭”混用 `"close"`，导致语义混淆且视觉识别度不统一；
- 归一化后：窗口/标签页/对话框关闭保留 `"close"`，所有数据条目级别的清除/清空统一使用 `"clear-2.svg"`。

---

## Chapter 1: Overview（概述与根因分析）

### 设计目标
针对 QuarkMeta 中所有涉及“清除/移除数据条目、清空历史、删除规则”的按钮及动作项，将图标由原先的 `"close"` 统一修改为 `"clear-2.svg"`。
1. **语义归一化**：彻底解耦“关闭窗口/窗格 (`close`)”与“清除数据项 (`clear-2.svg`)”的视觉与逻辑语义；
2. **零篡改保证**：仅修改图标键名传参，控件尺寸、颜色、事件响应、信号槽绑定 100% 原样保留；
3. **接口契约冻结**：不改动任何类公开接口与头文件签名。

---

## Chapter 2: Modified Files List（影响文件清单）

| 操作类型 | 文件相对路径 |
|---|---|
| **修改** | `QuarkMeta_Source/src/ui/AddressHistoryPanel.cpp` |
| **修改** | `QuarkMeta_Source/src/ui/SearchHistoryPanel.cpp` |
| **修改** | `QuarkMeta_Source/src/ui/components/TagPill.cpp` |
| **修改** | `QuarkMeta_Source/src/ui/dialogs/SmartFolderRuleRow.cpp` |
| **修改** | `QuarkMeta_Source/src/ui/MarkedPanel.cpp` |
| **修改** | `QuarkMeta_Source/src/ui/LibraryMenuBuilder.cpp` |
| 不涉及 | `CMakeLists.txt`（无文件增删） |

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/AddressHistoryPanel.cpp` — 最近访问历史移除按钮图标更新

```
<<<<<<< SEARCH
            QPushButton* btnRemove = new QPushButton(row);
            btnRemove->setFixedSize(16, 16);
            btnRemove->setFlat(true);
            btnRemove->setIcon(UiHelper::getIcon("close", QColor("#555555"), 12));
            btnRemove->setIconSize(QSize(12, 12));
=======
            QPushButton* btnRemove = new QPushButton(row);
            btnRemove->setFixedSize(16, 16);
            btnRemove->setFlat(true);
            btnRemove->setIcon(UiHelper::getIcon("clear-2.svg", QColor("#555555"), 12));
            btnRemove->setIconSize(QSize(12, 12));
>>>>>>> REPLACE
```

### 3.2 `QuarkMeta_Source/src/ui/SearchHistoryPanel.cpp` — 搜索历史记录移除按钮图标更新

```
<<<<<<< SEARCH
            QPushButton* btnRemove = new QPushButton(row);
            btnRemove->setFixedSize(16, 16);
            btnRemove->setFlat(true);
            btnRemove->setIcon(UiHelper::getIcon("close", QColor("#555555"), 12));
            btnRemove->setIconSize(QSize(12, 12));
=======
            QPushButton* btnRemove = new QPushButton(row);
            btnRemove->setFixedSize(16, 16);
            btnRemove->setFlat(true);
            btnRemove->setIcon(UiHelper::getIcon("clear-2.svg", QColor("#555555"), 12));
            btnRemove->setIconSize(QSize(12, 12));
>>>>>>> REPLACE
```

### 3.3 `QuarkMeta_Source/src/ui/components/TagPill.cpp` — 标签药丸清除“×”按钮图标更新

```
<<<<<<< SEARCH
    m_closeBtn = new QPushButton(this); 
    m_closeBtn->setFixedSize(16, 16); 
    m_closeBtn->setCursor(Qt::PointingHandCursor); 
    m_closeBtn->setIcon(UiHelper::getIcon("close", QColor("#FFFFFF"), 14)); 
    m_closeBtn->setIconSize(QSize(12, 12));
=======
    m_closeBtn = new QPushButton(this); 
    m_closeBtn->setFixedSize(16, 16); 
    m_closeBtn->setCursor(Qt::PointingHandCursor); 
    m_closeBtn->setIcon(UiHelper::getIcon("clear-2.svg", QColor("#FFFFFF"), 14)); 
    m_closeBtn->setIconSize(QSize(12, 12));
>>>>>>> REPLACE
```

### 3.4 `QuarkMeta_Source/src/ui/dialogs/SmartFolderRuleRow.cpp` — 智能文件夹规则行删除按钮图标更新

```
<<<<<<< SEARCH
    m_btnDelete = new QPushButton(this);
    m_btnDelete->setFixedSize(24, 24);
    m_btnDelete->setIcon(UiHelper::getIcon("close", QColor("#CCCCCC"), 14));
    m_btnDelete->setObjectName("SmartFolderRowDeleteBtn");
=======
    m_btnDelete = new QPushButton(this);
    m_btnDelete->setFixedSize(24, 24);
    m_btnDelete->setIcon(UiHelper::getIcon("clear-2.svg", QColor("#CCCCCC"), 14));
    m_btnDelete->setObjectName("SmartFolderRowDeleteBtn");
>>>>>>> REPLACE
```

### 3.5 `QuarkMeta_Source/src/ui/MarkedPanel.cpp` — 标记历史移除图标更新

```
<<<<<<< SEARCH
        QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 16), "从标记历史中移除");
        connect(removeAct, &QAction::triggered, this, [selectedPaths]() {
=======
        QAction* removeAct = menu.addAction(UiHelper::getIcon("clear-2.svg", QColor("#EEEEEE"), 16), "从标记历史中移除");
        connect(removeAct, &QAction::triggered, this, [selectedPaths]() {
>>>>>>> REPLACE
```

### 3.6 `QuarkMeta_Source/src/ui/MarkedPanel.cpp` — 清空标记历史图标更新

```
<<<<<<< SEARCH
    QAction* clearHistoryAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 16), "清空标记历史");
    connect(clearHistoryAct, &QAction::triggered, this, []() {
=======
    QAction* clearHistoryAct = menu.addAction(UiHelper::getIcon("clear-2.svg", QColor("#EEEEEE"), 16), "清空标记历史");
    connect(clearHistoryAct, &QAction::triggered, this, []() {
>>>>>>> REPLACE
```

### 3.7 `QuarkMeta_Source/src/ui/LibraryMenuBuilder.cpp` — 快速访问右键菜单“清空快速访问”图标更新

```
<<<<<<< SEARCH
    if (kind == LibraryNodeKind::SectionQuickAccess) {
        QAction* clearAct = menu->addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "清空快速访问");
        QObject::connect(clearAct, &QAction::triggered, [actions]() {
=======
    if (kind == LibraryNodeKind::SectionQuickAccess) {
        QAction* clearAct = menu->addAction(UiHelper::getIcon("clear-2.svg", QColor("#EEEEEE")), "清空快速访问");
        QObject::connect(clearAct, &QAction::triggered, [actions]() {
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 编译验证步骤
```bash
# 因沙盒环境中不包含 Qt6 库，跳过实际编译步骤
```

### 运行时验收标准

| 场景 | 操作步骤 | 预期结果 |
|---|---|---|
| 最近访问列表清除按钮 | 打开最近访问路径列表（`AddressHistoryPanel`），观察每一行右侧按钮 | 按钮图标变为带有实心背景的 `clear-2.svg`，点击后正常移除该行记录 |
| 搜索历史清除按钮 | 打开搜索历史面板（`SearchHistoryPanel`），观察搜索词右侧按钮 | 按钮图标正确渲染为 `clear-2.svg`，点击后正常删除搜索记录 |
| 标签药丸清除按钮 | 在属性面板或内容区查看 `TagPill` 标签控件 | 药丸右侧的“×”符号统一渲染为 `clear-2.svg`，悬停与点击删除功能正常 |
| 规则行删除按钮 | 打开智能文件夹规则编辑对话框 | 每一行规则右侧的删除按钮图标显示为 `clear-2.svg` |
| 清空菜单项 | 右键点击标记历史条目或快速访问分类 | 菜单中的“从标记历史中移除”、“清空标记历史”、“清空快速访问”左侧图标统一为 `clear-2.svg` |

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 功能 | 复用的官方 SSOT 入口 | 位置 |
|---|---|---|
| 矢量图标渲染引擎 | `UiHelper::getIcon(key, color, size)` | `QuarkMeta_Source/src/ui/UiHelper.h` |
| 矢量图标真理源字典 | `SvgIcons::icons["clear-2.svg"]` | `QuarkMeta_Source/src/ui/SvgIcons.h` |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 来源 | 派生/消费点 | 同步方向 |
|---|---|---|---|
| 清除按钮图标数据 | `SvgIcons::icons["clear-2.svg"]` | `UiHelper::getIcon` | 只读查询 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 调用 | 物理签名位置 | 访问级别 |
|---|---|---|
| `UiHelper::getIcon` | `UiHelper.h`: `static QIcon getIcon(const QString& name, const QColor& color, int size = 16);` | ✅ public |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

| 类型/API | `#include` 状态 |
|---|---|
| `UiHelper::getIcon` | 所有 6 个修改文件头部均已包含 `"UiHelper.h"` 或 `"../UiHelper.h"` ✅ |

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 位置/说明 |
|---|---|---|---|
| 1 | 方案已通过需求确认 | ✅ 满足 | 用户明确下达指令并达成共识（采用方案 2） |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见已读物理证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 逐文件核对真实存在且全文件唯一匹配 |
| 4 | 新增接口声明、定义、调用点签名一致 | ✅ 不涉及 | 仅修改已有 API 传入的图标键名实参 |
| 5 | 被修改的 `#include` 间接包含链闭合 | ✅ 满足 | 无 `#include` 增删 |
| 6 | 新建/删除文件已在 CMakeLists.txt 同步 | ✅ 不涉及 | 无文件增删 |
| 7 | 跨类调用访问级别均为 public | ✅ 满足 | `UiHelper::getIcon` 为 public 静态方法 |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 严格限制在清单内的 6 个 UI 文件 |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | 复用既有高效渲染通道 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 实现全软件层面的语义归一化 |
| 11 | 遵循 QuarkMeta_Source/Memories_QuarkMeta.md 规范 | ✅ 满足 | 符合清除与关闭语义归一化铁律 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 不涉及 | 未调用 ToolTipOverlay |
| 13 | 100% 语义化矢量图标 | ✅ 满足 | 纯 SVG 矢量定义 |
| 14 | 异步回调按值捕获 | ✅ 不涉及 | 纯 UI 图标设置 |
| 15 | 无 Lambda 循环引用 | ✅ 不涉及 | 保持既有 Lambda 结构 |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 不涉及 | 全部在 UI 主线程执行 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 不涉及 | 无信号槽语法改动 |
| 18 | 对外 `.h` 签名冻结 | ✅ 满足 | 不改动任何头文件接口签名 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明

| 既有行为 | 本方案影响 | 结论 |
|---|---|---|
| 按钮点击触发删除/清除历史与规则 | 仅替换按钮的图标图形，信号槽连接与业务逻辑完全不变 | ✅ 100% 原样保留 |
| 窗口、窗格及标签页的关闭操作 | 顶层 `close` 图标保持不变，不产生连带副作用 | ✅ 100% 原样保留 |

### 数值零篡改声明

| 数值类型 | 值 | 来源对齐 |
|---|---|---|
| 按钮尺寸 | `16x16`, `24x24` | 100% 原样照抄 |
| 图标尺寸 | `12`, `14`, `16` | 100% 原样照抄 |
| 颜色十六进制值 | `#555555`, `#FFFFFF`, `#CCCCCC`, `#EEEEEE` | 100% 原样照抄 |
