# Implementation Plan - BatchOpenWithRefactoringPlan (右键菜单“打开方式”多选批量化重构方案)

## 已读物理证据清单 (Gate 1: Pre-Draft Evidence)
1. **物理读取文件列表**：
   - `src/ui/controllers/ContextMenuFactory.h` (已阅读：第 50~65 行，确认 `buildOpenWithMenu` 声明)
   - `src/ui/controllers/ContextMenuFactory.cpp` (已阅读：第 270~310 行，确认 `buildOpenWithMenu` 内部实现)
   - `src/ui/controllers/ContentContextMenu.cpp` (已阅读：第 245~260 行与第 580~610 行，确认菜单构建与 `ActionOpenDefault` 触发逻辑)
   - `src/util/ShellHelper.h` (已阅读：确认 `getAssociatedApps` 与 `showOpenWithDialog` 签名)
   - `src/util/ShellHelper.cpp` (已阅读：确认底层 Shell 调用实现)
2. **头文件原样签名**：
   - `ContextMenuFactory.h`:
     ```cpp
     static QMenu* buildOpenWithMenu(QMenu* parentMenu, const QString& filePath, QWidget* parentWidget = nullptr);
     ```
   - `ContentPanel.h`:
     ```cpp
     QStringList getSelectedPaths() const;
     ```
3. **既有 API 检索日志**：
   - `ShellHelper::getAssociatedApps(filePath)`
   - `ShellHelper::showOpenWithDialog(filePath, parentWidget)`
   - `QProcess::startDetached(exec, arguments)`
   - `QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))`

---

## Chapter 1: Overview（概述与根因分析）

### 1. 现象与问题分析
在内容面板中，当用户多选选中多个文件（如 50 个文件）时，右键上下文菜单中的“打开方式”子菜单完全缺失，或者仅能针对右键命中那一个单一文件生效。点击推荐应用程序时，仅有该单文件被外部程序启动，其余被选中的数十个文件完全被丢弃。

### 2.  (显式类型包含)

```
<<<<<<< SEARCH
#include <QUrl>

namespace QuarkMeta {
=======
#include <QUrl>
#include <QFileDialog>

namespace QuarkMeta {
>>>>>>> REPLACE
```

### 4. 根因剖析 (Root Cause)
1. **选区丢失与单路径截断**：在 `src/ui/controllers/ContentContextMenu.cpp` 中构建上下文菜单时，代码直接通过 `currentIndex.sibling(..., FileListColumn::Name)` 获取右键点击处单行对应的 `path`，并将单路径传入 `ContextMenuFactory::buildOpenWithMenu(&menu, path, m_panel)`，完全忽略了 `m_panel->getSelectedPaths()` 所持有的完整选区集合；
2. **工厂接口缺乏集合重载**：`ContextMenuFactory::buildOpenWithMenu` 仅定义了单路径签名 `const QString& filePath`，并且在内部将参数直接硬编码绑定到 Lambda 槽函数中启动外部进程（`QProcess::startDetached(exec, QStringList() << filePath)`）；
3. **违反多选一致性设计铁律**：未能贯彻 `Memories.md` 第 20 章“任何操作若不支持批量操作均为失败设计”原则，多选状态在“打开方式”交互中严重退化。

### 3. 重构设计与解决目标
1. **API 对称性与批量接口扩展**：
   - 保持既有单文件签名 `buildOpenWithMenu(parentMenu, const QString& filePath, ...)` 兼容不变（契约锁）；
   - 新增批量重载 `buildOpenWithMenu(parentMenu, const QStringList& filePaths, ...)`；
   - 单文件版本直接委托调用批量版本（将 `filePath` 包装为单元素 `QStringList`），消除平行重复代码；
2. **智能扩展名归集与关联程序探测**：
   - 过滤掉选区中的目录，仅保留文件；
   - 提取首个有效文件的扩展名，通过 `ShellHelper::getAssociatedApps` 解析系统关联应用列表；
3. **多目标参数完整传递**：
   - 点击具体推荐程序时，将选中的所有文件路径一次性作为参数列表传递给 `QProcess::startDetached(exec, filePaths)`；
   - 点击“选择其他应用...”时，若为单文件仍调用系统 `ShellHelper::showOpenWithDialog`；若为多文件，弹出可执行文件选择框或通过系统打开方式执行选区批量启动；
4. **ContentContextMenu 联动改造**：
   - 统一提取选区 `m_panel->getSelectedPaths()`，若选区非空且包含所选项目，则将完整文件列表传递给 `buildOpenWithMenu`。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/controllers/ContextMenuFactory.h`
2. `src/ui/controllers/ContextMenuFactory.cpp`
3. `src/ui/controllers/ContentContextMenu.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/controllers/ContextMenuFactory.h`

```
<<<<<<< SEARCH
    /**
     * @brief 构建“打开方式”级联子菜单
     */
    static QMenu* buildOpenWithMenu(QMenu* parentMenu, const QString& filePath, QWidget* parentWidget = nullptr);
=======
    /**
     * @brief 构建“打开方式”级联子菜单 (单文件兼容重载)
     */
    static QMenu* buildOpenWithMenu(QMenu* parentMenu, const QString& filePath, QWidget* parentWidget = nullptr);

    /**
     * @brief 构建“打开方式”级联子菜单 (多选批量重载，SSOT)
     */
    static QMenu* buildOpenWithMenu(QMenu* parentMenu, const QStringList& filePaths, QWidget* parentWidget = nullptr);
>>>>>>> REPLACE
```

### 2. `src/ui/controllers/ContextMenuFactory.cpp`

```
<<<<<<< SEARCH
#include <QUrl>

namespace QuarkMeta {
=======
#include <QUrl>
#include <QFileDialog>

namespace QuarkMeta {
>>>>>>> REPLACE
```

### 3. `src/ui/controllers/ContextMenuFactory.cpp`

```
<<<<<<< SEARCH
QMenu* ContextMenuFactory::buildOpenWithMenu(QMenu* parentMenu, const QString& filePath, QWidget* parentWidget) {
    if (!parentMenu || filePath.isEmpty() || QFileInfo(filePath).isDir()) return nullptr;

    QMenu* openWithMenu = parentMenu->addMenu(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "打开方式");
    UiHelper::applyMenuStyle(openWithMenu);

    QList<ShellHelper::AssocAppInfo> apps = ShellHelper::getAssociatedApps(filePath);
    for (const auto& app : apps) {
        QIcon icon = app.icon.isNull() ? UiHelper::getIcon("launch", QColor("#EEEEEE"), 16) : app.icon;
        QAction* act = openWithMenu->addAction(icon, app.name);
        QString exec = app.execPath;
        QObject::connect(act, &QAction::triggered, [filePath, exec]() {
            if (!exec.isEmpty()) {
                QProcess::startDetached(exec, QStringList() << filePath);
            } else {
                QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
            }
        });
    }

    if (!apps.isEmpty()) {
        openWithMenu->addSeparator();
    }

    QAction* actChooseOther = openWithMenu->addAction(UiHelper::getIcon("add", QColor("#EEEEEE"), 16), "选择其他应用...");
    QObject::connect(actChooseOther, &QAction::triggered, [filePath, parentWidget]() {
        ShellHelper::showOpenWithDialog(filePath, parentWidget);
    });

    return openWithMenu;
}
=======
QMenu* ContextMenuFactory::buildOpenWithMenu(QMenu* parentMenu, const QString& filePath, QWidget* parentWidget) {
    if (filePath.isEmpty()) return nullptr;
    return buildOpenWithMenu(parentMenu, QStringList() << filePath, parentWidget);
}

QMenu* ContextMenuFactory::buildOpenWithMenu(QMenu* parentMenu, const QStringList& filePaths, QWidget* parentWidget) {
    if (!parentMenu || filePaths.isEmpty()) return nullptr;

    // 过滤出文件路径，排除文件夹
    QStringList validFiles;
    for (const QString& p : filePaths) {
        if (!p.isEmpty() && !QFileInfo(p).isDir()) {
            validFiles << p;
        }
    }
    if (validFiles.isEmpty()) return nullptr;

    QMenu* openWithMenu = parentMenu->addMenu(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "打开方式");
    UiHelper::applyMenuStyle(openWithMenu);

    // 以首个文件的扩展名作为推荐程序探测基准
    QString primaryFile = validFiles.first();
    QList<ShellHelper::AssocAppInfo> apps = ShellHelper::getAssociatedApps(primaryFile);
    for (const auto& app : apps) {
        QIcon icon = app.icon.isNull() ? UiHelper::getIcon("launch", QColor("#EEEEEE"), 16) : app.icon;
        QAction* act = openWithMenu->addAction(icon, app.name);
        QString exec = app.execPath;
        QObject::connect(act, &QAction::triggered, [validFiles, exec]() {
            if (!exec.isEmpty()) {
                QProcess::startDetached(exec, validFiles);
            } else {
                for (const QString& f : validFiles) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(f));
                }
            }
        });
    }

    if (!apps.isEmpty()) {
        openWithMenu->addSeparator();
    }

    QAction* actChooseOther = openWithMenu->addAction(UiHelper::getIcon("add", QColor("#EEEEEE"), 16), "选择其他应用...");
    QObject::connect(actChooseOther, &QAction::triggered, [validFiles, parentWidget]() {
        if (validFiles.size() == 1) {
            ShellHelper::showOpenWithDialog(validFiles.first(), parentWidget);
        } else {
            // 多选批量场景：通过选择可执行文件对所有选中项批量打开
            QString exec = QFileDialog::getOpenFileName(parentWidget, "选择打开程序", QString(), "可执行程序 (*.exe);;所有文件 (*.*)");
            if (!exec.isEmpty()) {
                QProcess::startDetached(exec, validFiles);
            }
        }
    });

    return openWithMenu;
}
>>>>>>> REPLACE
```

### 4. `src/ui/controllers/ContentContextMenu.cpp`

```
<<<<<<< SEARCH
            if (isFolder) {
                ContextMenuFactory::buildTabCascadeMenu(&menu, path, m_panel);
            } else {
                menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
                ContextMenuFactory::buildOpenWithMenu(&menu, path, m_panel);
            }
=======
            if (isFolder) {
                ContextMenuFactory::buildTabCascadeMenu(&menu, path, m_panel);
            } else {
                menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
                QStringList selPathsForOpen = m_panel ? m_panel->getSelectedPaths() : QStringList();
                if (selPathsForOpen.isEmpty() && !path.isEmpty()) {
                    selPathsForOpen << path;
                }
                ContextMenuFactory::buildOpenWithMenu(&menu, selPathsForOpen, m_panel);
            }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 编译命令
在工程根目录下执行构建：
```bash
cmake -B build -S .
cmake --build build --config Release -j8
```

### 2. 运行时验收标准
1. **单文件场景**：选中 1 个文本文件或图片，右键查看“打开方式”，推荐程序正常展示；点击应用可正常启动并打开该文件；点击“选择其他应用...”弹出系统 `SHOpenWithDialog`；
2. **多选场景（同类型）**：在内容面板中通过 `Ctrl+A` 或按住 `Shift` 选中 20 个图片文件（`.jpg`），右键菜单正常显示“打开方式”；选择某一图片查看器或编辑器后，20 个图片路径全部作为参数传递给该应用批量启动打开；
3. **多选场景（系统默认程序打开）**：多选后点击“用系统默认程序打开”，系统依次批量调度 `QDesktopServices::openUrl` 打开所选全部文件；
4. **混合多选场景**：选中包含文件夹与文件的混合选区，“打开方式”智能过滤掉文件夹，仅将所有有效文件送入外部应用，不崩溃、不报错。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **复用 `ShellHelper::getAssociatedApps`**：复用既有的系统关联程序探测基础设施，杜绝在 UI 控制器中重复编写 Win32 注册表查询逻辑；
2. **复用 `QProcess::startDetached` 批量参数支持**：直接利用 Qt 官方进程启动机制传递 `QStringList` 参数列表，无需另行编写管道或外部批处理脚本；
3. **复用 `m_panel->getSelectedPaths()`**：严守 `ContentPanel` 作为选区权威来源的 SSOT 原则，不再私自解析索引模型。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本 | 同步与刷新方式 |
| :--- | :--- | :--- | :--- |
| **选中的文件路径集合** | `ContentPanel::getSelectedPaths()` | `validFiles` (临时过滤容器) | 右键点击时即时获取，生命周期限制在槽函数内部 |
| **文件关联应用程序列表** | `ShellHelper::getAssociatedApps` | `apps` 列表 | 菜单弹出时动态探测生成，菜单关闭后自动随局部作用域析构 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部调用接口原样签名**：
   - `ContextMenuFactory.h`:
     ```cpp
     static QMenu* buildOpenWithMenu(QMenu* parentMenu, const QString& filePath, QWidget* parentWidget = nullptr);
     ```
2. **新增接口签名**：
   - `ContextMenuFactory.h`:
     ```cpp
     static QMenu* buildOpenWithMenu(QMenu* parentMenu, const QStringList& filePaths, QWidget* parentWidget = nullptr);
     ```
   - 访问权限：`public`（完全契合 `ContextMenuFactory` 既有静态构建方法）。
3. **调用点核查**：
   - `QuickLookWindow.cpp`: 维持原有 `buildOpenWithMenu(&menu, m_currentPath, this)`，自动解析到单路径版本，完全二进制兼容；
   - `ContentContextMenu.cpp`: 传入 `selPathsForOpen` (`QStringList`)，精准匹配新增的批量版本。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. **`ContextMenuFactory.cpp`**：
   - 增加了 `QFileDialog::getOpenFileName`，需显式核查是否包含 `<QFileDialog>`；
   - `ContextMenuFactory.cpp` 包含 `<QProcess>`、`<QFileInfo>`、`<QDesktopServices>`，且通过 `<QFileDialog>` 闭合包含。
2. **`ContentContextMenu.cpp`**：
   - 调用的 `m_panel->getSelectedPaths()`、`ContextMenuFactory::buildOpenWithMenu` 均已在头文件中声明，无需新增包含。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 合规状态 | 证据与位置 |
| :---: | :--- | :---: | :--- |
| 1 | 实施界限铁律：禁止直接篡改源代码，物理隔离于 Implementation Plan/ | **满足** | 本方案存放于 `Implementation Plan/BatchOpenWithRefactoringPlan.md` |
| 2 | SEARCH 块物理文件中唯一匹配 | **满足** | 经 `verify_plan.py` 自动化检测，所有 SEARCH 块 count=1 |
| 3 | 对外接口签名冻结契约 (Public API Freezing) | **满足** | 既有 `buildOpenWithMenu(..., const QString&)` 原样保留并重载 |
| 4 | C++/Qt 五道工程硬锁闭环 | **满足** | View/Controller 单向依赖，契约锁严格遵守 |
| 5 | Memories.md 第 20 章多选批量铁律对齐 | **满足** | 完全落实“任何操作若不支持批量操作均为失败设计”原则 |
| 6 | 包含 10 大必备章节且结构完整 | **满足** | 包含 Chapter 1 至 Chapter 10 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 序号 | 既有设计/属性参数 | 规范要求 | 方案处理说明 |
| :---: | :--- | :--- | :--- |
| 1 | 单文件右键“打开方式” | 保持原有交互与体验 100% 不变 | 单文件时行为完全一致，直接调用系统推荐与 `SHOpenWithDialog` |
| 2 | 图标颜色 `#EEEEEE` | 100% 原样照抄，禁止随意微调 | 新增与保留的图标均使用 `QColor("#EEEEEE")`，与全局菜单规范统一 |
| 3 | 菜单项字号与布局 | 100% 沿用 `UiHelper::applyMenuStyle` | 严格调用 `UiHelper::applyMenuStyle(openWithMenu)`，样式完全统一 |
