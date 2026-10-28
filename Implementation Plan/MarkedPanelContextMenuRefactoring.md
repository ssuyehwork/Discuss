# Implementation Plan - MarkedPanel Context Menu Refactoring ("标记" 面板右键菜单补全与图标单色化)

## 1. Chapter 1: Overview（概述与根因分析）
### 1.1 根因分析
在现有 `MarkedPanel`（“标记”侧边栏面板）的 `onContextMenuRequested` 函数中，右键菜单仅提供了 4 项基本操作（`标记为目标文件夹/取消标记`、`从标记历史中移除`、`取消所有标记`、`清空标记历史`），缺乏桌面级侧边栏的核心必备功能（打开文件夹、新 Tab/窗格打开、在资源管理器中显示、复制完整路径、刷新），且缺失了 **“取消其他标记”** （仅保留当前点击项为唯一标记目的地）的高频快捷选项。

同时，原有代码在构造 `标记为目标文件夹` 动作项时传入了 `#ff551c` 橙色图标（`UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 16)`），违背了 `Memories.md` 中关于“右键菜单所有矢量图标统一采用 `#EEEEEE` 中性单色”的绝对红线。

### 1.2 治理重构目标
1. **补全 `MarkedFolderService` 查询与操作 API**：在 `MarkedFolderService` 中补充 `keepOnlyMarked(const QString& targetPath)` 接口，支持“取消其他标记”动作；
2. **重构 `MarkedPanel::onContextMenuRequested` 菜单结构**：
   - **第一组（打开与定位）**：`打开文件夹`（触发 `requestLocateFolder`）、`在新标签页中打开`（通过 `TabBarWidget` 新建 Tab 打开）、`在“资源管理器”中显示`（调用 `ShellHelper::showInGraphicalShell`）；
   - **第二组（标记状态控制）**：`标记为目标文件夹` / `取消标记`、`取消其他标记`（仅当选中项为非激活标记或激活标记时有效，将其余所有标记取消）、`从标记历史中移除`；
   - **第三组（辅助工具组）**：`复制完整路径`（写剪贴板 `QGuiApplication::clipboard()->setText(...)`）、`刷新`（触发 `loadMarkedFolders`）；
   - **第四组（全局清理组）**：`取消所有标记`（`clearAll`）、`清空标记历史`（`clearAllFolderRecords`）；
3. **彻底单色化**：全局右键菜单图标一律强制配置为 `#EEEEEE`，杜绝任何彩色图标。

---

## 2. Chapter 2: Modified Files List（影响文件清单）

- `src/meta/MarkedFolderService.h`
- `src/meta/MarkedFolderService.cpp`
- `src/ui/MarkedPanel.h`
- `src/ui/MarkedPanel.cpp`

---

## 3. Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 补充 `MarkedFolderService` 接口声明与实现

#### 3.1.1 在 `src/meta/MarkedFolderService.h` 中添加 `keepOnlyMarked` 声明
```
<<<<<<< SEARCH
    bool clearAllFolderRecords();
    bool clearAll();
=======
    bool clearAllFolderRecords();
    bool clearAll();
    bool keepOnlyMarked(const QString& targetPath);
>>>>>>> REPLACE
```

#### 3.1.2 在 `src/meta/MarkedFolderService.cpp` 中添加 `keepOnlyMarked` 实现
```
<<<<<<< SEARCH
bool MarkedFolderService::clearAll() {
    bool ok = MarkedFolderDao::clearAllMarked();
    if (ok) {
        emit markedFoldersChanged();
    }
    return ok;
}
=======
bool MarkedFolderService::clearAll() {
    bool ok = MarkedFolderDao::clearAllMarked();
    if (ok) {
        emit markedFoldersChanged();
    }
    return ok;
}

bool MarkedFolderService::keepOnlyMarked(const QString& targetPath) {
    if (targetPath.isEmpty()) return false;
    bool ok = MarkedFolderDao::clearAllMarked();
    if (ok) {
        MarkedFolderDao::addMarked(targetPath);
        emit markedFoldersChanged();
    }
    return ok;
}
>>>>>>> REPLACE
```

---

### 3.2 重构 `MarkedPanel` 信号与右键菜单逻辑

#### 3.2.1 在 `src/ui/MarkedPanel.h` 中添加新信号
```
<<<<<<< SEARCH
signals:
    void requestLocateFolder(const QString& path);
=======
signals:
    void requestLocateFolder(const QString& path);
    void requestOpenInNewTab(const QString& path);
>>>>>>> REPLACE
```

#### 3.2.2 重构 `src/ui/MarkedPanel.cpp` 的 `onContextMenuRequested`

```
<<<<<<< SEARCH
void MarkedPanel::onContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    if (item) {
        QString path = item->data(0, Qt::UserRole).toString();
        bool active = item->data(0, Qt::UserRole + 1).toBool();

        if (active) {
            QAction* unmarkAct = menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#888888"), 16), "取消标记");
            connect(unmarkAct, &QAction::triggered, this, [path]() {
                MarkedFolderService::instance().removeMarked(path);
            });
        } else {
            QAction* markAct = menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 16), "标记为目标文件夹");
            connect(markAct, &QAction::triggered, this, [path]() {
                MarkedFolderService::instance().addMarked(path);
            });
        }

        QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 16), "从标记历史中移除");
        connect(removeAct, &QAction::triggered, this, [path]() {
            MarkedFolderService::instance().removeFolderRecord(path);
        });
        menu.addSeparator();
    }

    QAction* unmarkAllAct = menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#888888"), 16), "取消所有标记");
    connect(unmarkAllAct, &QAction::triggered, this, []() {
        MarkedFolderService::instance().clearAll();
    });

    QAction* clearHistoryAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 16), "清空标记历史");
    connect(clearHistoryAct, &QAction::triggered, this, []() {
        MarkedFolderService::instance().clearAllFolderRecords();
    });

    menu.exec(m_treeWidget->mapToGlobal(pos));
}
=======
void MarkedPanel::onContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget ? m_treeWidget->itemAt(pos) : nullptr;
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    if (item) {
        QString path = item->data(0, Qt::UserRole).toString();
        bool active = item->data(0, Qt::UserRole + 1).toBool();

        // 1. 打开与定位组
        QAction* openAct = menu.addAction(UiHelper::getIcon("folder", QColor("#EEEEEE"), 16), "打开文件夹");
        connect(openAct, &QAction::triggered, this, [this, path]() {
            emit requestLocateFolder(path);
        });

        QAction* newTabAct = menu.addAction(UiHelper::getIcon("Folder-add", QColor("#EEEEEE"), 16), "在新标签页中打开");
        connect(newTabAct, &QAction::triggered, this, [this, path]() {
            emit requestOpenInNewTab(path);
        });

        QAction* showInExplorerAct = menu.addAction(UiHelper::getIcon("Search-in-library", QColor("#EEEEEE"), 16), "在“资源管理器”中显示");
        connect(showInExplorerAct, &QAction::triggered, this, [path]() {
            ShellHelper::showInGraphicalShell(path);
        });

        menu.addSeparator();

        // 2. 标记状态控制组
        if (active) {
            QAction* unmarkAct = menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#EEEEEE"), 16), "取消标记");
            connect(unmarkAct, &QAction::triggered, this, [path]() {
                MarkedFolderService::instance().removeMarked(path);
            });
        } else {
            QAction* markAct = menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#EEEEEE"), 16), "标记为目标文件夹");
            connect(markAct, &QAction::triggered, this, [path]() {
                MarkedFolderService::instance().addMarked(path);
            });
        }

        QAction* keepOnlyAct = menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#EEEEEE"), 16), "取消其他标记");
        connect(keepOnlyAct, &QAction::triggered, this, [path]() {
            MarkedFolderService::instance().keepOnlyMarked(path);
        });

        QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 16), "从标记历史中移除");
        connect(removeAct, &QAction::triggered, this, [path]() {
            MarkedFolderService::instance().removeFolderRecord(path);
        });

        menu.addSeparator();

        // 3. 辅助工具组
        QAction* copyPathAct = menu.addAction(UiHelper::getIcon("document-attach-outline", QColor("#EEEEEE"), 16), "复制完整路径");
        connect(copyPathAct, &QAction::triggered, this, [path]() {
            if (QClipboard* cb = QGuiApplication::clipboard()) {
                cb->setText(QDir::toNativeSeparators(path));
            }
        });

        QAction* refreshAct = menu.addAction(UiHelper::getIcon("check", QColor("#EEEEEE"), 16), "刷新");
        connect(refreshAct, &QAction::triggered, this, [this]() {
            loadMarkedFolders();
        });

        menu.addSeparator();
    }

    // 4. 全局清理组
    QAction* unmarkAllAct = menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#EEEEEE"), 16), "取消所有标记");
    connect(unmarkAllAct, &QAction::triggered, this, []() {
        MarkedFolderService::instance().clearAll();
    });

    QAction* clearHistoryAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 16), "清空标记历史");
    connect(clearHistoryAct, &QAction::triggered, this, []() {
        MarkedFolderService::instance().clearAllFolderRecords();
    });

    menu.exec(m_treeWidget->mapToGlobal(pos));
}
>>>>>>> REPLACE
```

在 `src/ui/MarkedPanel.cpp` 头部引入所需头文件：
```
<<<<<<< SEARCH
#include <QFileInfo>
#include <QDir>
#include <QMenu>
#include <QMouseEvent>
#include <QMimeData>
#include <QUrl>
=======
#include <QFileInfo>
#include <QDir>
#include <QMenu>
#include <QMouseEvent>
#include <QMimeData>
#include <QUrl>
#include <QClipboard>
#include <QGuiApplication>
#include "../util/ShellHelper.h"
>>>>>>> REPLACE
```

---

## 4. Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建命令
```bash
cmake --build build --config Debug --target QuarkMeta -j8
```

### 4.2 运行时验收标准
1. **“取消其他标记”验证**：
   - 标记 3 个目标文件夹（如 A、B、C）；
   - 右键点击文件夹 A，选择“取消其他标记”；
   - 验证只有文件夹 A 保持标记状态，B 和 C 自动取消标记，图标更新。
2. **完整菜单与单色图标验证**：
   - 右键点击标记项，弹出的菜单包含全部 4 组 9 个选项；
   - 确认菜单所有项图标均为中性单色 `#EEEEEE`，`标记为目标文件夹` 的 `#ff551c` 橙色已被成功替换。
3. **“在资源管理器中显示”与“复制完整路径”验证**：
   - 点击“在‘资源管理器’中显示”，系统顺利打开目标物理路径；
   - 点击“复制完整路径”，粘贴板成功写入绝对路径。

---

## 5. Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **复用 `ShellHelper::showInGraphicalShell`**：调用官方 Win32 唤起接口；
2. **复用 `MarkedFolderDao::clearAllMarked()` 与 `addMarked()`**：在 `keepOnlyMarked` 中直接复用既有原子 DAO 事务，杜绝另起炉灶。

---

## 6. Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本 / 展示层 | 同步方向与契约 |
| :--- | :--- | :--- | :--- |
| **标记目的地集合** | `marked_folders` 数据库表 / `MarkedFolderService` | `MarkedPanel` QTreeWidget 项状态与图标 | 事件广播 `markedFoldersChanged` 全量重载刷平 |

---

## 7. Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. `MarkedFolderService::keepOnlyMarked(const QString& targetPath)` 声明于 `MarkedFolderService.h`，`public` 方法；
2. `MarkedPanel::requestOpenInNewTab(const QString& path)` 声明于 `MarkedPanel.h` `signals:` 区域，访问级别完全受控。

---

## 8. Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. 在 `MarkedPanel.cpp` 中包含了 `<QClipboard>`、`<QGuiApplication>` 与 `ShellHelper.h`，确保类型完备。

---

## 9. Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 结论 | 位置与证据 |
| :---: | :--- | :---: | :--- |
| 1 | 实施方案放置于隔离目录 `Implementation Plan/` | **满足** | 本文档放置在 `Implementation Plan/MarkedPanelContextMenuRefactoring.md` |
| 2 | SEARCH 块原文在目标文件中真实存在且唯一匹配 | **满足** | 已精准匹配 `MarkedFolderService` 与 `MarkedPanel` 原文 |
| 3 | 方案调用的外部成员函数在 `.h` 中真实存在且签名一致 | **满足** | 已校验 `ShellHelper::showInGraphicalShell` 等方法 |
| 4 | 方案新增接口声明、定义与调用点 100% 一致 | **满足** | `keepOnlyMarked` 声明、定义与调用点完美匹配 |
| 5 | 头文件包含链闭合完整 | **满足** | 包含了 `ShellHelper.h` 与 Qt 基础头文件 |
| 6 | CMakeLists.txt 全覆盖 | **满足** | 无增删文件 |
| 7 | 遵照 10 大必备章节结构要求 | **满足** | Chapter 1 至 Chapter 10 齐备 |
| 8 | 遵循 Memories.md 右键菜单单色铁律 | **满足** | 彻底去除了 `#ff551c` 彩色图标 |

---

## 10. Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 维度 | 原设计规范 / 数值 | 本方案设计 / 数值 | 判定结果 |
| :--- | :--- | :--- | :---: |
| **右键菜单背景/高亮** | 暗色统一 `#252526` / `#3E3E42` | 暗色统一 `#252526` / `#3E3E42` | **100% 保持** |
| **图标与文本间距** | `10px` | `10px` | **100% 保持** |
| **右键菜单图标色彩** | 部分橙色 `#ff551c`（违规） | 100% 统一中性单色 `#EEEEEE` | **按规范修正** |
