# Implementation Plan: 侧边栏历史选项卡单击交互优化与完备右键上下文菜单实施方案

Base Commit Hash: `2ed3357542a13cdfa996de90da4b0a0a52f91c3b`
Target File: `QuarkMeta_Source/src/ui/HistoryPanel.cpp`

本实施方案旨在严格遵循《SYSTEM_PROMPT.md》、《AGENTS.md》及《Memories.md》（章节 5、章节 24 与章节 26），在 `QuarkMeta_Source` 桌面应用中完整解决：
1. **侧边栏“历史”选项卡单击性能与体验优化**：去除左键单击自动定位主视图逻辑，避免频繁切换目录与缩略图重绘带来的性能开销，仅保留行内选中高亮；
2. **上下文右键菜单完备性补全**：全面补齐与系统资源管理器、收藏夹等同等权重的 10 大核心操作（默认程序打开、资源管理器显示、QuarkMeta 中显示、打开方式级联、收藏夹、库、复制、剪切、复制名称、复制完整路径），并完备保留历史面板特有的“从历史中移除”与“清空历史”。

---

## 前置：已读物理证据清单 (Pre-Draft Physical Evidence)

### 1. 物理读取文件列表与相对路径
- `QuarkMeta_Source/src/ui/HistoryPanel.h` (已确认无对外签名破坏，仅在内部 slots 调整)
- `QuarkMeta_Source/src/ui/HistoryPanel.cpp` (核心修改文件)
- `QuarkMeta_Source/src/ui/controllers/ContextMenuFactory.h` (上下文菜单工厂 SSOT 头文件)
- `QuarkMeta_Source/src/ui/controllers/ContextMenuFactory.cpp` (确认已有实现与调用规范)
- `QuarkMeta_Source/src/meta/HistoryService.h` (历史服务接口声明)
- `QuarkMeta_Source/src/meta/FavoriteService.h` (收藏夹服务接口声明)
- `QuarkMeta_Source/src/meta/LibraryService.h` (库服务接口声明)
- `QuarkMeta_Source/src/core/ClipboardService.h` (剪贴板服务接口声明)
- `Memories.md` (第 26 章交互规范真理源)

### 2. 头文件原样声明签名
- `ContextMenuFactory::buildShowInExplorerAction`:
  ```cpp
  static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, bool isBlankSpace = false, QObject* receiver = nullptr);
  ```
- `ContextMenuFactory::buildOpenWithMenu`:
  ```cpp
  static QMenu* buildOpenWithMenu(QMenu* parentMenu, const QStringList& filePaths, QWidget* parentWidget = nullptr);
  ```
- `ContextMenuFactory::buildCopyPathAction`:
  ```cpp
  static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
  ```
- `ContextMenuFactory::buildCopyNameAction`:
  ```cpp
  static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
  ```
- `FavoriteService::buildFavoriteAction`:
  ```cpp
  QAction* buildFavoriteAction(QMenu* parentMenu, const QString& path, QObject* receiver = nullptr);
  ```
- `LibraryService::buildLibraryAction`:
  ```cpp
  QAction* buildLibraryAction(QMenu* parentMenu, const QStringList& targetPaths, QObject* receiver = nullptr);
  ```
- `ClipboardService::copyItems` / `cutItems`:
  ```cpp
  void copyItems(const QStringList& paths);
  void cutItems(const QStringList& paths);
  ```
- `HistoryService::recordFileAccess`:
  ```cpp
  bool recordFileAccess(const QString& path);
  ```

### 3. 既有 API 检索日志 (Search-First 证据)
- 检索 `buildShowInExplorerAction`：在 `ContextMenuFactory.h` 第 52 行命中，严禁手写资源管理器跳转代码；
- 检索 `buildOpenWithMenu`：在 `ContextMenuFactory.h` 第 64 行命中，复用既有打开方式级联菜单；
- 检索 `buildCopyNameAction` / `buildCopyPathAction`：在 `ContextMenuFactory.h` 第 53-54 行命中，统一复用；
- 检索 `buildFavoriteAction` / `buildLibraryAction`：在 `FavoriteService.h` 第 23 行及 `LibraryService.h` 第 25 行命中。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题根因与背景
在 `QuarkMeta_Source/src/ui/HistoryPanel.cpp` 中：
```cpp
void HistoryPanel::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty()) {
        emit requestLocateFile(path);
    }
}
```
当用户仅仅用鼠标左键单击历史项目以便在侧边栏做选中或查看时，`onItemClicked` 立即无差别发射了 `requestLocateFile(path)` 信号。该信号经由 `NavPanel` 传递给 `PanelMediator`，直接驱动主内容区 `ContentPanel` 重新解析物理父目录、读取所有文件项、并发提取缩略图，甚至引起视口强制重排与滚动。对于频繁翻看历史列表的用户而言，带来了极其昂贵的无用开销。

此外，历史项右键菜单（`onContextMenuRequested`）目前极其简陋，仅有“从历史中移除”和“清空历史”，缺少了用户高频使用的“系统打开”、“资源管理器定位”、“在 QuarkMeta 中显示”、“打开方式”、“添加至收藏夹/库”、“复制/剪切/复制路径/复制名称”等一系列基础生产力动作。

### 1.2 重构设计目标
1. **取消单击定位**：清空 `onItemClicked` 中发射 `requestLocateFile` 的逻辑，使左键单击仅保留 QTreeWidget 自身的视觉选中高亮（基于全局规范 `rgba(55, 138, 221, 0.15)`），无任何多余 I/O 与视图重绘；
2. **完备右键上下文菜单**：
   - 菜单完全受控于 `UiHelper::applyMenuStyle(&menu)`；
   - 所有菜单项图标严格采用 `#EEEEEE` 统一中性色；
   - 顺序编排并严格复用现有 SSOT 接口与工厂方法：
     - 用系统默认程序打开 (`QDesktopServices::openUrl` + `HistoryService::recordFileAccess`)
     - 在“资源管理器”中显示 (`ContextMenuFactory::buildShowInExplorerAction`)
     - 在 QuarkMeta 中显示 (`emit requestLocateFile(path)`)
     - 打开方式 (`ContextMenuFactory::buildOpenWithMenu`)
     - [分隔符]
     - 添加至收藏夹 (`FavoriteService::instance().buildFavoriteAction`)
     - 添加至“库” (`LibraryService::instance().buildLibraryAction`)
     - [分隔符]
     - 复制（Ctrl+C） (`ClipboardService::instance().copyItems`)
     - 剪切（Ctrl+X） (`ClipboardService::instance().cutItems`)
     - 复制名称 (`ContextMenuFactory::buildCopyNameAction`)
     - 复制完整路径 (`ContextMenuFactory::buildCopyPathAction`)
     - [分隔符]
     - 从历史中移除 (`HistoryService::instance().removeHistory`)
     - 清空历史 (`HistoryService::instance().clearAllHistory`)

---

## Chapter 2: Modified Files List（影响文件清单）

本次方案仅影响 1 个业务源文件，无需新建或删除任何文件：
1. `QuarkMeta_Source/src/ui/HistoryPanel.cpp` (修改)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/HistoryPanel.cpp`

<<<<<<< SEARCH
#include "HistoryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "../meta/HistoryService.h"
#include "../util/ThumbnailPipelineService.h"
#include "../util/ColorPaletteEngine.h"
=======
#include "HistoryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "controllers/ContextMenuFactory.h"
#include "../core/ClipboardService.h"
#include "../meta/FavoriteService.h"
#include "../meta/HistoryService.h"
#include "../meta/LibraryService.h"
#include "../util/ThumbnailPipelineService.h"
#include "../util/ColorPaletteEngine.h"
#include <QDesktopServices>
>>>>>>> REPLACE

<<<<<<< SEARCH
void HistoryPanel::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty()) {
        emit requestLocateFile(path);
    }
}

void HistoryPanel::onContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    QMenu menu(this);

    if (item) {
        QString path = item->data(0, Qt::UserRole).toString();
        QAction* removeAct = menu.addAction("从历史中移除");
        connect(removeAct, &QAction::triggered, this, [path]() {
            HistoryService::instance().removeHistory(path);
        });
        menu.addSeparator();
    }

    QAction* clearAct = menu.addAction("清空历史");
    connect(clearAct, &QAction::triggered, this, []() {
        HistoryService::instance().clearAllHistory();
    });

    menu.exec(m_treeWidget->mapToGlobal(pos));
}
=======
void HistoryPanel::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(item);
    Q_UNUSED(column);
    // 单击仅由 QTreeWidget 处理选中高亮，不发射定位信号以彻底杜绝额外目录载入与缩略图解析开销
}

void HistoryPanel::onContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    if (item) {
        QString path = item->data(0, Qt::UserRole).toString();
        if (!path.isEmpty()) {
            // 1. 用系统默认程序打开
            QAction* actOpenDefault = menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开");
            connect(actOpenDefault, &QAction::triggered, this, [path]() {
                if (QFileInfo::exists(path)) {
                    HistoryService::instance().recordFileAccess(path);
                    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                }
            });

            // 2. 在“资源管理器”中显示
            ContextMenuFactory::buildShowInExplorerAction(&menu, path, false, this);

            // 3. 在 QuarkMeta 中显示
            QAction* actShowInApp = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 18), "在 QuarkMeta 中显示");
            connect(actShowInApp, &QAction::triggered, this, [this, path]() {
                emit requestLocateFile(path);
            });

            // 4. 打开方式 (级联子菜单)
            ContextMenuFactory::buildOpenWithMenu(&menu, QStringList{path}, this);

            menu.addSeparator();

            // 5. 添加至收藏夹
            FavoriteService::instance().buildFavoriteAction(&menu, path, this);

            // 6. 添加至“库”
            LibraryService::instance().buildLibraryAction(&menu, QStringList{path}, this);

            menu.addSeparator();

            // 7. 复制（Ctrl+C）
            QAction* actCopy = menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "复制（Ctrl+C）");
            connect(actCopy, &QAction::triggered, this, [path]() {
                ClipboardService::instance().copyItems({path});
            });

            // 8. 剪切（Ctrl+X）
            QAction* actCut = menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切（Ctrl+X）");
            connect(actCut, &QAction::triggered, this, [path]() {
                ClipboardService::instance().cutItems({path});
            });

            // 9. 复制名称
            ContextMenuFactory::buildCopyNameAction(&menu, {path}, this);

            // 10. 复制完整路径
            ContextMenuFactory::buildCopyPathAction(&menu, {path}, this);

            menu.addSeparator();

            // 11. 从历史中移除
            QAction* removeAct = menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "从历史中移除");
            connect(removeAct, &QAction::triggered, this, [path]() {
                HistoryService::instance().removeHistory(path);
            });

            menu.addSeparator();
        }
    }

    // 12. 清空历史 (项菜单末尾或空白处菜单)
    QAction* clearAct = menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "清空历史");
    connect(clearAct, &QAction::triggered, this, []() {
        HistoryService::instance().clearAllHistory();
    });

    menu.exec(m_treeWidget->mapToGlobal(pos));
}
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译验证步骤
```bash
# 检查语法与工程构建依赖
cmake --build build --target QuarkMeta
```

### 4.2 运行时交互验收标准
1. **左键单击测试**：
   - 打开侧边栏“目录导航”容器下的“历史”选项卡；
   - 鼠标左键单击历史列表中任意一个历史文件；
   - **预期现象**：该历史文件项呈现半透明蓝色选中高亮，右侧主内容区（`ContentPanel`）没有任何视角切换、目录重新加载或缩略图重新载入，保持原状。
2. **右键上下文菜单完备性测试**：
   - 鼠标右键点击某个历史文件项；
   - **预期现象**：弹出带有暗色样式的右键菜单，所有图标颜色统一为 `#EEEEEE`，完整包含以下 12 个操作：
     - “用系统默认程序打开”：点击可成功调用系统关联软件打开文件；
     - “在“资源管理器”中显示”：点击后在 Windows 资源管理器中打开对应目录并选中该文件；
     - “在 QuarkMeta 中显示”：点击后主内容区跳转定位至该文件所在目录并选中高亮；
     - “打开方式”：悬停可展开系统推荐程序与“选择其他应用...”；
     - “添加至收藏夹”：点击可正确将对应文件加入收藏夹；
     - “添加至“库””：点击可正确弹出入库/分类操作；
     - “复制（Ctrl+C）”：点击后在外部资源管理器或 QuarkMeta 其他目录中按 Ctrl+V 可粘贴该文件；
     - “剪切（Ctrl+X）”：点击后可在目标文件夹粘贴并完成移动；
     - “复制名称”：点击后剪贴板获得纯文件名；
     - “复制完整路径”：点击后剪贴板获得文件的绝对物理路径；
     - “从历史中移除”：点击后仅从历史列表中删除该条目；
     - “清空历史”：点击后清空整个历史记录列表。
3. **右键空白处测试**：
   - 在历史列表空白区域右键单击；
   - **预期现象**：仅显示“清空历史”项。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **资源管理器定位 SSOT**：
   复用 `ContextMenuFactory::buildShowInExplorerAction(&menu, path, false, this)`，严禁另起炉灶手写 `QProcess` 或 Win32 Explorer 打开逻辑。
2. **打开方式级联子菜单 SSOT**：
   复用 `ContextMenuFactory::buildOpenWithMenu(&menu, QStringList{path}, this)`，与主内容区右键完全一致。
3. **名称与路径复制 SSOT**：
   复用 `ContextMenuFactory::buildCopyNameAction` 与 `ContextMenuFactory::buildCopyPathAction`，完全对齐系统规范。
4. **收藏与库管理 SSOT**：
   复用 `FavoriteService::instance().buildFavoriteAction` 与 `LibraryService::instance().buildLibraryAction`。
5. **剪贴板操作 SSOT**：
   复用 `ClipboardService::instance().copyItems` 与 `cutItems`，确保支持剪贴板跨窗口与系统交互。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 / 状态 | 唯一 SSOT 数据源 | 派生副本 / 观察者 | 同步与刷新机制 |
| :--- | :--- | :--- | :--- |
| 历史记录列表 | `HistoryService` (`UI/RecentFilesHistory` in `AppConfig`) | `HistoryPanel::m_treeWidget` | 信号 `historyItemAdded`, `historyItemRemoved`, `historyCleared` 响应式刷新 |
| 选区高亮状态 | `QTreeWidget::selectionModel` | View 界面渲染 | 原生 `SingleSelection` 机制管理，单击无外部副作用 |
| 剪贴板文件 | `ClipboardService` (Windows 原生 OLE 剪贴板) | 外部 Explorer / QuarkMeta ContentPanel | `copyItems` / `cutItems` 即时写入系统剪贴板 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 外部调用接口签名物理核查
- `ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, bool isBlankSpace = false, QObject* receiver = nullptr)`:
  - 声明文件：`QuarkMeta_Source/src/ui/controllers/ContextMenuFactory.h:52`
  - 访问控制：`public static`，完全一致。
- `ContextMenuFactory::buildOpenWithMenu(QMenu* parentMenu, const QStringList& filePaths, QWidget* parentWidget = nullptr)`:
  - 声明文件：`QuarkMeta_Source/src/ui/controllers/ContextMenuFactory.h:64`
  - 访问控制：`public static`，完全一致。
- `ContextMenuFactory::buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr)`:
  - 声明文件：`QuarkMeta_Source/src/ui/controllers/ContextMenuFactory.h:54`
  - 访问控制：`public static`，完全一致。
- `ContextMenuFactory::buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr)`:
  - 声明文件：`QuarkMeta_Source/src/ui/controllers/ContextMenuFactory.h:53`
  - 访问控制：`public static`，完全一致。
- `FavoriteService::buildFavoriteAction(QMenu* parentMenu, const QString& path, QObject* receiver = nullptr)`:
  - 声明文件：`QuarkMeta_Source/src/meta/FavoriteService.h:23`
  - 访问控制：`public`，完全一致。
- `LibraryService::buildLibraryAction(QMenu* parentMenu, const QStringList& targetPaths, QObject* receiver = nullptr)`:
  - 声明文件：`QuarkMeta_Source/src/meta/LibraryService.h:25`
  - 访问控制：`public`，完全一致。
- `ClipboardService::copyItems(const QStringList& paths)` & `cutItems`:
  - 声明文件：`QuarkMeta_Source/src/core/ClipboardService.h:16-17`
  - 访问控制：`public`，完全一致。

### 7.2 新增或修改接口全链核查
本方案未新增任何类接口，亦未修改既有 `.h` 签名，`HistoryPanel.h` 保持 100% 冻结只读。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

### 8.1 显式 `#include` 闭合性分析
在 `QuarkMeta_Source/src/ui/HistoryPanel.cpp` 头部显式补充了以下包含：
- `#include "controllers/ContextMenuFactory.h"`：提供 `ContextMenuFactory` 全套静态构建函数完整定义；
- `#include "../core/ClipboardService.h"`：提供 `ClipboardService::instance()` 完整声明；
- `#include "../meta/FavoriteService.h"`：提供 `FavoriteService::instance()` 完整声明；
- `#include "../meta/LibraryService.h"`：提供 `LibraryService::instance()` 完整声明；
- `#include <QDesktopServices>`：提供系统默认程序启动能力；
所有类型均为直接显式包含，不依赖任何二级间接头文件传递。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 状态 | 实施方案具体位置 / 说明 |
| :---: | :--- | :---: | :--- |
| 1 | 方案是否物理隔离在 `Implementation Plan/` 目录下 | 满足 | `Implementation Plan/History_Panel_Click_And_ContextMenu_Implementation_Plan.md` |
| 2 | 是否使用纯中文进行阐述与注释说明 | 满足 | 全文使用中文进行架构与技术阐述 |
| 3 | 是否包含全部 10 大必备章节规范 | 满足 | 完整包含 Chapter 1 至 Chapter 10 |
| 4 | 前置是否包含“已读物理证据清单” | 满足 | 前置完整列出读取文件、原样签名与 grep 检索证据 |
| 5 | SEARCH 块是否在目标文件中物理存在且唯一匹配 | 满足 | 经过自动化脚本物理全词校验，100% 唯一匹配 |
| 6 | 对外公开 `.h` 接口签名是否处于只读冻结状态 | 满足 | `HistoryPanel.h` 零修改，签名无任何破坏 |
| 7 | 是否杜绝 Win32 强夺焦点等平台级 Hack | 满足 | 零平台 Hack，基于标准 Qt 信号槽与 QMenu |
| 8 | 是否杜绝在 C++ 代码中内联 `setStyleSheet` | 满足 | 统一复用 `UiHelper::applyMenuStyle` 与系统 QSS |
| 9 | 上下文菜单图标颜色是否严格统一为 `#EEEEEE` | 满足 | 所有菜单项图标色严格统一使用 `#EEEEEE` |
| 10 | 是否复用既有 SSOT 菜单构建工厂（防另起炉灶） | 满足 | 严格复用 `ContextMenuFactory` / `FavoriteService` / `LibraryService` |
| 11 | 是否避免在异步循环内重复实例化原子变量 | 不涉及 | 本方案未引入多线程并发循环 |
| 12 | 异步 Lambda 是否避免按引用捕获局部栈变量 | 满足 | 所有 Lambda 均使用值捕获（如 `[path]`、`[this, path]`） |
| 13 | 递归 Lambda 是否避免捕获自身 shared_ptr | 不涉及 | 无递归 Lambda 结构 |
| 14 | 是否避免在 for 循环中并发 dispatch 顺序操作 | 不涉及 | 无多目的地顺序文件写操作 |
| 15 | 新引入的类型是否包含显式 `#include` 头文件 | 满足 | 显式包含了 `ContextMenuFactory.h`, `ClipboardService.h` 等 |
| 16 | CMakeLists.txt 是否完整同步注册与清理 | 满足 | 无新建/删除文件，保持既有 CMake 配置 |
| 17 | 是否完成用户设计理念向 `Memories.md` 的归档 | 满足 | 已归档至 `Memories.md` 第 26 章 |
| 18 | 是否运行自动化验证工具并附带全绿日志 | 满足 | 见下文自动化脚本运行输出日志 |

### 自动化验证工具运行输出日志
```
PASS: All 10 required chapters present.
PASS: Block 1 in QuarkMeta_Source/src/ui/HistoryPanel.cpp uniquely matches.
PASS: Block 2 in QuarkMeta_Source/src/ui/HistoryPanel.cpp uniquely matches.
```

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 行为与交互零篡改声明
| 既有功能 / 交互模式 | 方案处理方式 | 行为变化说明 |
| :--- | :--- | :--- |
| 历史记录双击/拖拽功能 | 原封不动保留 | 保持既有拖拽创建 QDrag 与原生行为 |
| 历史记录悬浮 ToolTip 显示 | 原封不动保留 | 保持 `ToolTipOverlay::showText` 路径提示 |
| 历史记录列表从历史中移除 / 清空历史 | 原封不动保留 | 菜单项与槽函数逻辑原封不动延续，仅增补图标与样式 |
| 单击交互方式 | 行为优化 | 取消了之前误触发的全局文件定位跳转，仅保留行选中高亮 |
| 右键菜单呈现方式 | 功能增强补全 | 补齐 10 项缺失的标准操作，统一暗色视觉呈现 |

### 10.2 样式数值零篡改声明
| 属性项 | 既有规范数值 | 方案使用数值 | 偏差 |
| :--- | :--- | :--- | :---: |
| 上下文菜单图标色彩 | `#EEEEEE` (Memories.md 规范) | `QColor("#EEEEEE")` | 0 |
| 上下文菜单图标尺寸 | `18px` (标准菜单图标规格) | `18` | 0 |
| 选中行背景颜色 | `rgba(55, 138, 221, 0.15)` | `rgba(55, 138, 221, 0.15)` (QSS) | 0 |
