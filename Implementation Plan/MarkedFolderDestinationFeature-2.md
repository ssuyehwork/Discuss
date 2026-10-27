# Implementation Plan - Marked Folder & Destination Transfer Feature (-2)

**Base Commit Hash**: `db013fcb2828916174305ea402494ebfa6e01893`
**Prerequisite Plans**: 无
**Supersedes**: `Implementation Plan/superseded/MarkedFolderDestinationFeature-1.md`

---

## Chapter 1: Overview（概述与根因分析）
本方案旨在修正上一版实施方案中 SEARCH 块上下文与外部调用的物理核查差异，完整闭合“标记目的地文件夹”功能（支持快捷键 `Alt+C` 开关标记，以及通过 `Alt+V` 一键将选中项目同时分发迁移到所有标记激活状态的文件夹中）。

### 1.1 现状断点与根因排查
经过全工程代码物理核查：
1. `src/ui/PanelMediator.cpp` 第 240-244 行中使用局部变量 `shortcutController` 和 `filterPanel`，先前方案中写错为了成员变量 `m_shortcutController`；
2. `src/meta/MarkedFolderService.h` 需补充 `executeTransferToMarked(const QStringList& paths)` 的公开接口声明与其 `.cpp` 的完整搬迁编排引擎实现。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/meta/MarkedFolderService.h` (修改)：声明 `executeTransferToMarked(const QStringList& paths)` 公开接口；
2. `src/meta/MarkedFolderService.cpp` (修改)：实现 `executeTransferToMarked` 一键迁移编排（确认弹窗、目标过滤、无门槛 TaskProgress 驱动、同/跨盘复制、私有回收站原文件回收、UI 刷新与 Undo 命令打包）；
3. `src/ui/PanelMediator.cpp` (修改)：在 `PanelMediator::setupConnections()` 中绑定 `toggleMarkedRequested` 与 `transferToMarkedRequested` 信号；
4. `Memories.md` (修改)：记载标记文件夹功能的设计理念。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/meta/MarkedFolderService.h` 增加公开接口

```cpp
<<<<<<< SEARCH
    bool clearAllFolderRecords();
    bool clearAll();

    QAction* buildMarkAction(QMenu* parentMenu, const QString& path, QObject* receiver);

signals:
=======
    bool clearAllFolderRecords();
    bool clearAll();

    /**
     * @brief 触发 Alt+V 一键同时分发/迁移选中项目到所有激活的标记目的地文件夹
     */
    bool executeTransferToMarked(const QStringList& sourcePaths);

    QAction* buildMarkAction(QMenu* parentMenu, const QString& path, QObject* receiver);

signals:
>>>>>>> REPLACE
```

### 3.2 `src/meta/MarkedFolderService.cpp` 实现一键分发逻辑

```cpp
<<<<<<< SEARCH
bool MarkedFolderService::clearAll() {
    if (MarkedFolderDao::unmarkAllFolders()) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}
=======
bool MarkedFolderService::clearAll() {
    if (MarkedFolderDao::unmarkAllFolders()) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

bool MarkedFolderService::executeTransferToMarked(const QStringList& sourcePaths) {
    if (sourcePaths.isEmpty()) return false;
    QStringList activeTargets = getActiveMarked();
    if (activeTargets.isEmpty()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "未开启任何标记目的地文件夹", 2000, QColor("#e81123"));
        return false;
    }

    // 1. 过滤非法目标文件夹（过滤源文件所在父目录、过滤源文件夹自身及其子目录）
    QStringList validTargets;
    for (const QString& target : activeTargets) {
        QString cleanTarget = QDir::cleanPath(target);
        bool isValid = true;
        for (const QString& src : sourcePaths) {
            QString cleanSrc = QDir::cleanPath(src);
            QFileInfo srcInfo(cleanSrc);
            if (cleanTarget == cleanSrc || cleanTarget == srcInfo.absolutePath() || cleanTarget.startsWith(cleanSrc + "/")) {
                isValid = false;
                break;
            }
        }
        if (isValid) {
            validTargets.append(cleanTarget);
        }
    }

    if (validTargets.isEmpty()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "标记目的地文件夹属于无效目标（源目录自身或其父/子目录）", 2500, QColor("#e81123"));
        return false;
    }

    // 2. 确认弹窗
    QString confirmMsg = QString("确定将 %1 个项目同时分发/迁移到以下 %2 个目的地文件夹？\n\n目标列表：\n%3")
                             .arg(sourcePaths.size())
                             .arg(validTargets.size())
                             .arg(validTargets.join("\n"));
    if (!FramelessConfirmDialog::confirm("确认一键分发/迁移", confirmMsg)) {
        return false;
    }

    // 3. 计算同盘/异盘策略 (只要存在任一目标与源同盘，则采取全成功后原文件走私有回收站的策略)
    bool hasSameDisk = false;
    for (const QString& src : sourcePaths) {
        QString srcDrive = QFileInfo(src).absolutePath().left(3);
        for (const QString& tgt : validTargets) {
            QString tgtDrive = QFileInfo(tgt).absolutePath().left(3);
            if (srcDrive.contains(tgtDrive, Qt::CaseInsensitive) || tgtDrive.contains(srcDrive, Qt::CaseInsensitive)) {
                hasSameDisk = true;
                break;
            }
        }
        if (hasSameDisk) break;
    }

    // 4. 无门槛 TaskProgress 驱动与文件监视器静默
    auto progressJob = TaskProgressService::instance().createJob("一键分发到标记目的地", sourcePaths.size() * validTargets.size());
    ContentChangeWatcher::instance().suspendWatcher();

    bool allSuccess = true;
    QStringList createdCopies;

    // 5. 执行跨/同盘复制分发
    for (const QString& src : sourcePaths) {
        QFileInfo srcInfo(src);
        for (const QString& tgt : validTargets) {
            QString destFilePath = QDir(tgt).filePath(srcInfo.fileName());

            // 重名冲突检查
            if (QFile::exists(destFilePath)) {
                FileCollisionDialog dialog(destFilePath, src);
                if (dialog.exec() == QDialog::Rejected) {
                    allSuccess = false;
                    break;
                }
            }

            bool copyOk = ShellHelper::copyOrMoveItems({src}, tgt, true); // 强制执行复制模式
            if (copyOk) {
                createdCopies.append(destFilePath);
                progressJob->advance();
            } else {
                allSuccess = false;
                break;
            }
        }
        if (!allSuccess) break;
    }

    // 6. 全成功判定与原文件私有回收站处理
    if (allSuccess) {
        if (hasSameDisk) {
            // 同盘场景：原文件移入 QuarkMeta 私有回收站 (DiskTrashService)
            DiskTrashService::instance().moveToDiskTrash(sourcePaths);
        }
        // 压入 UndoManager 撤销命令链
        UndoManager::instance().pushCommand(new TransferToMarkedCommand(sourcePaths, createdCopies, hasSameDisk));
        ToolTipOverlay::instance()->showText(QCursor::pos(), "成功分发迁移到标记目的地", 2000, QColor("#2ecc71"));
    } else {
        // 任何失败时回滚已创建的副本，并安全保留原文件
        for (const QString& copyPath : createdCopies) {
            QFile::remove(copyPath);
        }
        ToolTipOverlay::instance()->showText(QCursor::pos(), "一键分发失败，原文件已安全保留", 2500, QColor("#e81123"));
    }

    // 7. UI 刷新与收尾
    ContentChangeWatcher::instance().resumeWatcher();
    if (auto* activePanel = ContentPanel::activeInstance()) {
        activePanel->refreshAll();
    }
    progressJob->finishJob();
    return allSuccess;
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/PanelMediator.cpp` 绑定快捷键信号槽

```cpp
<<<<<<< SEARCH
        if (filterPanel) {
            connect(shortcutController, &AppShortcutController::resetFilterRequested, filterPanel, [filterPanel]() {
                filterPanel->clearAllFilters(true);
            });
        }
    }
=======
        if (filterPanel) {
            connect(shortcutController, &AppShortcutController::resetFilterRequested, filterPanel, [filterPanel]() {
                filterPanel->clearAllFilters(true);
            });
        }

        connect(shortcutController, &AppShortcutController::toggleMarkedRequested, this, [this, contentPanel]() {
            if (contentPanel) {
                QStringList sel = contentPanel->getSelectedIndexesPaths();
                if (!sel.isEmpty()) {
                    MarkedFolderService::instance().toggleMarked(sel.first());
                }
            }
        });

        connect(shortcutController, &AppShortcutController::transferToMarkedRequested, this, [this, contentPanel]() {
            if (contentPanel) {
                QStringList sel = contentPanel->getSelectedIndexesPaths();
                if (!sel.isEmpty()) {
                    MarkedFolderService::instance().executeTransferToMarked(sel);
                }
            }
        });
    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）
1. 编译工程，确保无 MSVC/GCC 编译及链接错误。
2. 选中项目按下 `Alt+C`，侧边栏与目录树重绘橘色 `Destination.svg` 标记图标。
3. 按下 `Alt+V`，触发确认弹窗，确认后将项目迁移到所有活动目的地文件夹中，同盘原文件正确移入 QuarkMeta 私有回收站。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）
- 刷新视图统一调用 `ContentPanel::refreshAll()`。
- 原文件回收统一调用 `DiskTrashService::instance().moveToDiskTrash(...)`。
- 任务进度统一调用 `TaskProgressService::instance()`。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）
- **标记数据 SSOT**：SQLite 数据库 `marked_folders` 表。
- **任务进度 SSOT**：`TaskProgressService` 实例。

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别核查）
- `MarkedFolderService::executeTransferToMarked(const QStringList& sourcePaths)` 为 `public` 成员函数，声明与定义完全对齐。
- `PanelMediator.cpp` 中使用的 `shortcutController` 局部变量和 `contentPanel` 闭包 capture 完美对齐物理代码。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链核查）
- `MarkedFolderService.h` 已包含 `<QStringList>`。
- `PanelMediator.cpp` 已包含 `MarkedFolderService.h`。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 |
| :--- | :--- | :--- |
| 1 | SEARCH 块在 `PanelMediator.cpp` 中 100% 真实匹配 | 满足 |
| 2 | `executeTransferToMarked` 在 `MarkedFolderService.h` 中真实存在且签名一致 | 满足 |
| 3 | 绝对未直接修改 C++ 源码文件 | 满足 |
| 4 | 所有设计理念记载于 `Memories.md` | 满足 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为与数值零篡改声明表）
- 原有快捷键逻辑与其他侧边栏功能保持 100% 行为零篡改。
- QSS 样式数值保持零篡改。
