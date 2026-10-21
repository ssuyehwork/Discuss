# ContextMenuEnhancements.md - 右键菜单“打开方式”替换与复制名称去重实施方案

## Chapter 1: Pre-Draft Evidence（第 1 关：起草前核查）

### 1. 已读文件清单
- `src/ui/controllers/ContentContextMenu.cpp`（已读取第 280~285 行）
- `src/ui/QuickLookWindow.cpp`（已读取第 500~505 行）
- `src/ui/controllers/ContextMenuFactory.cpp`（已读取第 180~210 行）

### 2. 物理头文件签名原文摘录
- `ContextMenuFactory.h`:
  `static bool copyNamesToClipboard(const QStringList& paths, NameDelimiter delimiter = NameDelimiter::NewLine, bool showOverlay = true);`

### 3. 既有 API 检索记录
- 使用 `grep` 查找到“用系统默认程序打开”在 `ContentContextMenu.cpp` 和 `QuickLookWindow.cpp` 中定义；“复制名称”在 `ContextMenuFactory.cpp` 中实现。

---

## Chapter 2: Overview（概述与解决的问题）

按照用户指示进行右键菜单体验优化：
1. **替换文案**：将“用系统默认程序打开”统一修改替换为符合操作系统习惯的“打开方式”。
2. **复制名称自动去重**：在 `ContextMenuFactory::copyNamesToClipboard` 遍历提取文件名时，加入去重逻辑（保持原有出现的物理顺序，同时消除重复文件名），避免在多选相同名称或重复条目时向剪贴板写入无意义的重复内容。

---

## Chapter 3: Modified Files List（影响文件清单）
1. `src/ui/controllers/ContentContextMenu.cpp` (将“用系统默认程序打开”文案替换为“打开方式”)
2. `src/ui/QuickLookWindow.cpp` (将“用系统默认程序打开”文案替换为“打开方式”)
3. `src/ui/controllers/ContextMenuFactory.cpp` (在 `copyNamesToClipboard` 中增加文件名去重逻辑)

---

## Chapter 4: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/controllers/ContentContextMenu.cpp`
```
<<<<<<< SEARCH
            } else {
                menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
            }
=======
            } else {
                menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "打开方式")->setData(ContentPanel::ActionOpenDefault);
            }
>>>>>>> REPLACE
```

### 2. `src/ui/QuickLookWindow.cpp`
```
<<<<<<< SEARCH
    QAction* actOpenDefault = menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开");
=======
    QAction* actOpenDefault = menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "打开方式");
>>>>>>> REPLACE
```

### 3. `src/ui/controllers/ContextMenuFactory.cpp`
```
<<<<<<< SEARCH
bool ContextMenuFactory::copyNamesToClipboard(const QStringList& paths, NameDelimiter delimiter, bool showOverlay) {
    if (paths.isEmpty()) return false;

    QStringList names;
    for (const auto& p : paths) {
        if (!p.isEmpty()) {
            names << QFileInfo(p).fileName();
        }
    }
    if (names.isEmpty()) return false;
=======
bool ContextMenuFactory::copyNamesToClipboard(const QStringList& paths, NameDelimiter delimiter, bool showOverlay) {
    if (paths.isEmpty()) return false;

    QStringList names;
    for (const auto& p : paths) {
        if (!p.isEmpty()) {
            QString name = QFileInfo(p).fileName();
            if (!name.isEmpty() && !names.contains(name)) {
                names << name;
            }
        }
    }
    if (names.isEmpty()) return false;
>>>>>>> REPLACE
```

---

## Chapter 5: Build & Verification Steps（编译命令与验证方法）

### 编译步骤
```bash
cmake -B build -G "Ninja"
cmake --build build --config Release
```

### 验证方法
1. **右键菜单项验证**：右键点击任意文件，核对菜单选项已变更为“打开方式”。
2. **复制名称去重验证**：选中多个包含相同文件名的卡片，右键选择“复制名称” -> “分行名称”并粘贴到记事本，确认重复的文件名已被自动过滤去重。

---

## Chapter 6: SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用自查）
- 恪守数据单源原则，直接在 `ContextMenuFactory::copyNamesToClipboard` 统一入口增加去重，所有调用复制名称的地方均能无缝自动享受去重逻辑。

---

## Chapter 7: Header API Signature Verification（头文件 API 物理签名核查表）

| 调用成员 / 函数 | 所在头文件 | 物理精确签名 | 核查状态 |
|---|---|---|---|
| `ContextMenuFactory::copyNamesToClipboard` | `src/ui/controllers/ContextMenuFactory.h` | `static bool copyNamesToClipboard(const QStringList& paths, NameDelimiter delimiter = NameDelimiter::NewLine, bool showOverlay = true);` | 物理存在一致 |

---

## Chapter 8: Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）
- `ContextMenuFactory.cpp` 包含链完整，无需变动头文件。

---

## Chapter 9: Compliance Self-Check Table（第 2 关：交付前自检）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | `ContextMenuEnhancements.md` 对应右键菜单优化模块 |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/ContextMenuEnhancements.md` |
| 所有 SEARCH 块代码在仓库真实存在 | 3.2 | 满足 | 第 4 章 SEARCH 块逐字存在于源码中 |
| 第 1~10 章齐全 | 3.4 | 满足 | 第 1~10 章齐全 |
| 已复用既有 SSOT 入口，无另起炉灶 | 2.4 | 满足 | 第 6 章 |

---

## Chapter 10: Zero-Value-Alteration Check（数值零篡改核对）
- 本方案不涉及任何 UI 尺寸或颜色等参数变动，填写：**不涉及**。
