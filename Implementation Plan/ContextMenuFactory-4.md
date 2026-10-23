# Implementation Plan: ContextMenuFactory Submenu Enhancement for Copy Name (ContextMenuFactory-4.md)

## 1. Overview
本实施方案旨在将右键菜单中的“复制名称”项升级为带右箭头 (`▶`) 的二级子菜单，并支持三种分隔符选项：
1. **分行名称**（使用 `\r\n` 换行符拼接，保持原有逻辑）；
2. **顿号分割名称**（使用中文顿号 `、` 拼接）；
3. **逗号分割名称**（使用逗号 `, ` 拼接）。

所有更改严格收敛于 `ContextMenuFactory` 真理源 (SSOT) 控制类中。所有既有调用方（内容面板 `ContentContextMenu`、侧边栏 `NavPanel`、收藏夹 `FavoritePanel`、地址栏 `AddressBar`、快速预览 `QuickLookWindow`）无需修改调用代码，自动平滑升级继承二级子菜单能力。

---

## 2. Modified Files List
* `src/ui/controllers/ContextMenuFactory.h`
* `src/ui/controllers/ContextMenuFactory.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/controllers/ContextMenuFactory.h`

<<<<<<< SEARCH
    /**
     * @brief 复制文件名列表至剪贴板 SSOT 入口 (统一 \r\n 换行符)
     */
    static bool copyNamesToClipboard(const QStringList& paths, bool showOverlay = true);
=======
    /**
     * @brief 文件名分隔符枚举类型
     */
    enum class NameDelimiter {
        NewLine,       ///< 分行名称 (\r\n)
        Enumeration,   ///< 顿号分割名称 (、)
        Comma          ///< 逗号分割名称 (, )
    };

    /**
     * @brief 复制文件名列表至剪贴板 SSOT 入口 (支持指定分隔符)
     */
    static bool copyNamesToClipboard(const QStringList& paths, NameDelimiter delimiter, bool showOverlay = true);

    /**
     * @brief 复制文件名列表至剪贴板 SSOT 入口重载 (默认使用 NewLine 分行)
     */
    static bool copyNamesToClipboard(const QStringList& paths, bool showOverlay = true);
>>>>>>> REPLACE

---

### File 2: `src/ui/controllers/ContextMenuFactory.cpp`

<<<<<<< SEARCH
bool ContextMenuFactory::copyNamesToClipboard(const QStringList& paths, bool showOverlay) {
    if (paths.isEmpty()) return false;

    QStringList names;
    for (const auto& p : paths) {
        if (!p.isEmpty()) {
            names << QFileInfo(p).fileName();
        }
    }
    if (names.isEmpty()) return false;

    QApplication::clipboard()->setText(names.join("\r\n"));
    if (showOverlay) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制名称至剪贴板", 1500, Style::SuccessGreen);
    }
    return true;
}
=======
bool ContextMenuFactory::copyNamesToClipboard(const QStringList& paths, NameDelimiter delimiter, bool showOverlay) {
    if (paths.isEmpty()) return false;

    QStringList names;
    for (const auto& p : paths) {
        if (!p.isEmpty()) {
            names << QFileInfo(p).fileName();
        }
    }
    if (names.isEmpty()) return false;

    QString sep;
    switch (delimiter) {
        case NameDelimiter::NewLine:
            sep = "\r\n";
            break;
        case NameDelimiter::Enumeration:
            sep = "、";
            break;
        case NameDelimiter::Comma:
            sep = ", ";
            break;
    }

    QApplication::clipboard()->setText(names.join(sep));
    if (showOverlay) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制名称至剪贴板", 1500, Style::SuccessGreen);
    }
    return true;
}

bool ContextMenuFactory::copyNamesToClipboard(const QStringList& paths, bool showOverlay) {
    return copyNamesToClipboard(paths, NameDelimiter::NewLine, showOverlay);
}
>>>>>>> REPLACE

<<<<<<< SEARCH
QAction* ContextMenuFactory::buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        copyNamesToClipboard(paths, true);
    });
    return action;
}
=======
QAction* ContextMenuFactory::buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QMenu* subMenu = menu->addMenu(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称");
    UiHelper::applyMenuStyle(subMenu);

    QObject* target = receiver ? receiver : menu;

    QAction* actNewLine = subMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "分行名称");
    QObject::connect(actNewLine, &QAction::triggered, target, [paths]() {
        copyNamesToClipboard(paths, NameDelimiter::NewLine, true);
    });

    QAction* actEnum = subMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "顿号分割名称");
    QObject::connect(actEnum, &QAction::triggered, target, [paths]() {
        copyNamesToClipboard(paths, NameDelimiter::Enumeration, true);
    });

    QAction* actComma = subMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "逗号分割名称");
    QObject::connect(actComma, &QAction::triggered, target, [paths]() {
        copyNamesToClipboard(paths, NameDelimiter::Comma, true);
    });

    return subMenu->menuAction();
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### 构建步骤
使用 CMake 和 MSVC (或 Ninja) 增量编译项目：
```bash
cmake --build build --config Debug --target QuarkMeta
```

### 验证步骤
1. 启动 QuarkMeta 应用程序；
2. 选定多个文件/文件夹项目，右键唤出内容面板上下文菜单；
3. 检查“复制名称”项是否带有右箭头 `▶` 并在悬停时弹出二级子菜单；
4. 验证二级子菜单包含三个子项：`分行名称`、`顿号分割名称`、`逗号分割名称`；
5. 依次点击三个子项，粘贴至文本编辑器：
   - 点击 `分行名称`：验证粘贴内容使用 `\r\n` 换行分隔；
   - 点击 `顿号分割名称`：验证粘贴内容使用顿号 `、` 分隔；
   - 点击 `逗号分割名称`：验证粘贴内容使用逗号 `, ` 分隔；
6. 检查侧边栏（NavPanel）、收藏夹（FavoritePanel）、地址栏面包屑（AddressBar）右键菜单中的“复制名称”，确认均已同步支持此二级子菜单功能。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

| 自查项 | 结论 | 说明 |
| :--- | :--- | :--- |
| **是否复用既有 SSOT 通道？** | **是** | 所有修改仅作用于 `ContextMenuFactory::buildCopyNameAction` 与 `copyNamesToClipboard`，没有新建平行实现 |
| **是否存在另起炉灶行为？** | **否** | 所有调用方无需任何代码变动，直接继承 SSOT 改造成果 |
| **是否遵守零内联硬编码？** | **是** | 二级子菜单统一调用 `UiHelper::applyMenuStyle(subMenu)` 应用全局暗色 QSS 样式 |
| **签名向后兼容性** | **100% 保持** | 重载函数 `copyNamesToClipboard(paths, showOverlay)` 保证原有直接调用不会产生编译断裂 |

---

## 6. Header API Signature Verification

| 被调用的类/函数 | `.h` 文件真实物理签名 | 核查状态 |
| :--- | :--- | :--- |
| `UiHelper::getIcon` | `static QIcon getIcon(const QString& iconKey, const QColor& color = QColor(), int size = 16);` | 物理存在，匹配 |
| `UiHelper::applyMenuStyle` | `static void applyMenuStyle(QMenu* menu);` | 物理存在，匹配 |
| `QMenu::addMenu` | `QMenu* addMenu(const QIcon& icon, const QString& title);` | Qt 标准接口，匹配 |
| `QMenu::menuAction` | `QAction* menuAction() const;` | Qt 标准接口，匹配 |
| `ToolTipOverlay::showText` | `void showText(const QPoint& pos, const QString& text, int durationMs = 1500, const QColor& bgColor = QColor());` | 物理存在，匹配 |

---

## 7. Header Inclusion Chain & Type Completeness Check

| 修改文件 | 新增/引用的类与枚举 | 头文件包含状态 |
| :--- | :--- | :--- |
| `ContextMenuFactory.h` | `NameDelimiter` (枚举) | 定义于 `ContextMenuFactory.h` 本身，类型完整 |
| `ContextMenuFactory.cpp` | `QMenu`, `QAction`, `QApplication`, `QClipboard`, `ToolTipOverlay`, `UiHelper` | `#include` 链路完整，无隐式不完整类型隐患 |
