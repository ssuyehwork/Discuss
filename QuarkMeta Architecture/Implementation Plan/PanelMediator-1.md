# Implementation Plan - PanelMediator Computer Root Path Navigation Fix

## 1. Overview（概述与解决的问题）

### 架构三问回答
1. **真理源溯源 (SSOT)**：
   在 QuarkMeta 中，全软件对于“此电脑”根节点的统一标识符为 `computer://`。`NavigationService`、`NavPanel`、`AddressBar` 与 `BreadcrumbBar` 均以 `computer://` 作为真理源。
2. **黑盒完整性 (Black-box Integrity)**：
   `PanelMediator` 负责解耦与协调 `NavPanel`、`AddressBar` 和 `ContentPanel` 之间的导航路由。当导航到 `computer://` 时，`PanelMediator` 必须将精确的 `"computer://"` 字符串透传给 `ContentPanel::loadDirectory`，严禁擅自篡改或缩减为空字符串 `""`。
3. **根因溯源 (Root Cause Analysis)**：
   在 `PanelMediator.cpp` 中，当 `url == "computer://"` 时，原代码错误地调用了 `contentPanel->loadDirectory("")`（传入空字符串）。在列表/网格视图下，`ContentDataLoader` 内部有容错处理；但在列视图（ColumnView）模式下，`ColumnViewWidget::setRootPath(path)` 检测到 `path.isEmpty()` 直接静默 `return`，导致直接点击导航栏的“此电脑”时，列视图内容面板呈现一片空白，无法加载任何磁盘盘符分栏。

### 解决的核心问题
1. **统一导航路由**：修改 `PanelMediator.cpp`，将 `contentPanel->loadDirectory("")` 修正为 `contentPanel->loadDirectory("computer://")`，彻底根治列视图模式下点击“此电脑”后内容面板一片空白的缺陷。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/PanelMediator.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/PanelMediator.cpp`

```
<<<<<<< SEARCH
        if (contentPanel) {
            if (url == "computer://") {
                contentPanel->loadDirectory("");
            } else if (url == "trash://") {
                contentPanel->loadCategory("trash");
            } else {
                contentPanel->loadDirectory(url);
            }
        }
=======
        if (contentPanel) {
            if (url == "computer://") {
                contentPanel->loadDirectory("computer://");
            } else if (url == "trash://") {
                contentPanel->loadCategory("trash");
            } else {
                contentPanel->loadDirectory(url);
            }
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证命令
在终端运行以下 CMake 命令进行编译：
```bash
cmake -B build -S .
cmake --build build --config Release
```

### 4.2 逻辑功能验证
1. **列视图直接点击此电脑验证**：将视图模式切换为列视图（ColumnView），直接点击左侧“目录导航”下的“此电脑”（或地址栏切换至此电脑）。
2. **盘符与分栏渲染校验**：验证列视图内容面板第一列能否 100% 稳定显示出所有硬盘盘符（C:\、D:\、G:\ 等），且点击盘符后能在右侧顺利展开下一级文件夹分栏。
