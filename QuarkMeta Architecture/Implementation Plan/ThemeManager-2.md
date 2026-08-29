# 实施方案：全软件 QMenu 菜单选中高亮色还原为 #3E3E42 (ThemeManager-2.md)

## 1. Overview（概述与解决的问题）

### 1.1 核心问题说明
在全局主题管理器 `ThemeManager`（`src/ui/ThemeManager.cpp`）中，全软件 `QMenu`（包含托盘右键菜单与系统右键上下文菜单）的菜单项选中高亮背景色当前被配置为蓝色（`#378ADD`），偏离了原始版本 `Dual-mode version` 的暗深灰色（`#3E3E42`）高亮风格。

### 1.2 修复方案核心设计理念
1. **完全对标 `Dual-mode version` 视觉规范**：
   - 将 `QMenu::item:selected` 背景色统一替换为 `#3E3E42`。
   - 包含 `getGlobalStyleSheet()` 全局 QSS 字符串与 `applyMenuStyle()` 显式注入部分的双重修改。
2. **UI 记忆规范同步写入**：
   - 将 `QMenu` 菜单高亮色 `#3E3E42` 写入根目录 `Memories.md`，持久化规范约束。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/ThemeManager.cpp` - 修改全局 `QMenu::item:selected` 选中背景色为 `#3E3E42`。
2. `Memories.md` - 写入 QMenu 菜单高亮色色值规范。

---

## 3. Detailed Line-by-Line Changes（精准代码替换块）

### 3.1 修改 `src/ui/ThemeManager.cpp`

```
<<<<<<< SEARCH
        QMenu::item:selected {
            background-color: #378ADD;
            color: #FFFFFF;
        }
=======
        QMenu::item:selected {
            background-color: #3E3E42;
            color: #FFFFFF;
        }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        "QMenu::item:selected {"
        "   background-color: #378ADD;"
        "   color: #FFFFFF;"
        "}"
=======
        "QMenu::item:selected {"
        "   background-color: #3E3E42;"
        "   color: #FFFFFF;"
        "}"
>>>>>>> REPLACE
```

---

### 3.2 修改 `Memories.md`

```
<<<<<<< SEARCH
* `ThemeManager` (`src/ui/ThemeManager.h/cpp`) centralizes global UI theme and QSS stylesheet management (`ThemeManager::instance().initialize(&app)`), injecting unified dark theme QSS into `QApplication` (covering `QMainWindow`, `QDialog`, `QMenu`, `QScrollBar`, and input fields) and providing `applyMenuStyle` for popup menus (explicitly invoking `setStyleSheet` on `QMenu` to force Qt custom drawing and bypass Win32 native light `HMENU` rendering on Windows), eliminating fragmented QSS blocks from `MainWindow.cpp` and menu styling from `SvgIconRenderer.cpp`.
=======
* `ThemeManager` (`src/ui/ThemeManager.h/cpp`) centralizes global UI theme and QSS stylesheet management (`ThemeManager::instance().initialize(&app)`), injecting unified dark theme QSS into `QApplication` (covering `QMainWindow`, `QDialog`, `QMenu`, `QScrollBar`, and input fields) and providing `applyMenuStyle` for popup menus (explicitly invoking `setStyleSheet` on `QMenu` to force Qt custom drawing and bypass Win32 native light `HMENU` rendering on Windows, with `QMenu::item:selected` hover background color set to dark gray `#3E3E42` matching `Dual-mode version`), eliminating fragmented QSS blocks from `MainWindow.cpp` and menu styling from `SvgIconRenderer.cpp`.
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证
在沙盒 Bash 环境中执行构建命令：

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel 4
```

### 4.2 视觉与功能验证步骤
1. **菜单高亮色校验**：
   - 启动可执行文件 `./QuarkMeta`。
   - 右键点击图标唤出托盘菜单或空白处呼出上下文菜单。
   - 悬停鼠标移动到菜单项上，确认选中高亮背景色为标准的暗深灰色（`#3E3E42`），而非蓝色。
