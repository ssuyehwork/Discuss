# Implementation Plan - “打开项目”归一化重构方案 (OpenItemNormalization.md)

- **Base Commit Hash**: 064a8b932918e95757175b4cdbee5915839e14fa
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## Chapter 1: Overview

### 需求与重构目标
将 QuarkMeta 文件列表中的所有“打开项目”触发方式（双击、空格键、回车键、右键菜单“打开”）统一归一化归口至单一入口 `ContentPanel::activateIndex`：
1. **行为绝对一致**：
   - **可预览文件**：记录访问（`RecordAccess`）并调用 `QuickLookWindow::preview` 开启预览。
   - **不可预览文件**：直接截断放弃（严禁用系统默认程序打开）。
   - **文件夹**：进入文件夹（列视图展开下一列）。
2. **重构与收敛点**：
   - 将 `ContentPanel::onDoubleClicked` 统一更名为 `ContentPanel::activateIndex`。
   - 彻底删除 `ContentPanel.h` 中的 `requestQuickLook` 信号及所有中间判断分支。
   - 收敛 `PanelMediator.cpp` 中的 `fileActivated` 响应处理，删除 `QDesktopServices::openUrl` 兜底。
   - 将 `QuickLookWindow` 中的 `previewFile` 接口删除并统一收敛至 `preview`，并对外暴露 `currentPath()` 接口。
   - 清除 `QuickLookWindow.cpp` 中冗余的 `UNPREVIEWABLE_EXTS` 黑名单与对应分支，不可预览扩展名逻辑统一使用 `UiHelper::canPreviewFile`。
   - 调整 `QuickLookWindow` 上一个/下一个导航切换逻辑，跳过文件夹与不可预览文件。

---

## Chapter 2: Modified Files List

- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentKeyHandler.cpp`
- `src/ui/ColumnViewPane.cpp`
- `src/ui/PanelMediator.h`
- `src/ui/PanelMediator.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`
- `src/ui/QuickLookWindow.h`
- `src/ui/QuickLookWindow.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`

```
<<<<<<< SEARCH
    void requestQuickLook(const QString& path);
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void onDoubleClicked(const QModelIndex& index);
=======
    void activateIndex(const QModelIndex& index);
>>>>>>> REPLACE
```

---

### 2. `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    connect(jv, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
=======
    connect(jv, &QAbstractItemView::doubleClicked, this, &ContentPanel::activateIndex);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(tree, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
=======
    connect(tree, &QAbstractItemView::doubleClicked, this, &ContentPanel::activateIndex);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::onDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    QString path = index.data(PathRole).toString();
    if (path.isEmpty()) return;
    if (QFileInfo(path).isDir()) {
        emit directorySelected(path);
    } else {
        emit fileActivated(path);
    }
}
=======
void ContentPanel::activateIndex(const QModelIndex& index) {
    if (!index.isValid()) return;
    if (index.data(SectionHeaderRole).toBool()) return;
    QString path = index.data(PathRole).toString();
    if (path.isEmpty()) return;

    if (QFileInfo(path).isDir()) {
        if (m_currentViewMode == ColumnView && m_columnView) {
            ColumnViewPane* pane = m_columnView->activePane();
            int paneIdx = pane ? pane->property("paneIndex").toInt() : 0;
            m_columnView->expandFolder(path, paneIdx);
        } else {
            emit directorySelected(path);
        }
    } else {
        emit fileActivated(path);
    }
}
>>>>>>> REPLACE
```

---

### 3. `src/ui/controllers/ContentKeyHandler.cpp`

```
<<<<<<< SEARCH
    // 7. 空格键: QuickLook 预览
    if (keyEvent->key() == Qt::Key_Space) {
        QModelIndex idx = view->currentIndex();
        if (!idx.isValid() && view->selectionModel()) {
            auto selected = view->selectionModel()->selectedIndexes();
            if (!selected.isEmpty()) idx = selected.first();
        }

        if (idx.isValid()) {
            // 🚀 确保即使焦点处于列表视图非 Name 列，也能正确从 FileListColumn::Name sibling 获取完整 PathRole 属性
            QModelIndex nameIdx = idx.sibling(idx.row(), static_cast<int>(FileListColumn::Name));
            QString path = nameIdx.data(PathRole).toString();
            if (!path.isEmpty() && UiHelper::canPreviewFile(path)) {
                emit m_panel->requestQuickLook(path);
            }
        }
        return true;
    }

    // 8. 导航键
    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
        m_panel->onDoubleClicked(view->currentIndex());
        return true;
    }
=======
    // 7. 空格键与回车键: 统一激活当前项（QuickLook 预览或打开文件夹）
    if (keyEvent->key() == Qt::Key_Space || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
        QModelIndex idx = view->currentIndex();
        if (!idx.isValid() && view->selectionModel()) {
            auto selected = view->selectionModel()->selectedIndexes();
            if (!selected.isEmpty()) idx = selected.first();
        }
        if (idx.isValid()) {
            QModelIndex nameIdx = idx.sibling(idx.row(), static_cast<int>(FileListColumn::Name));
            m_panel->activateIndex(nameIdx);
        }
        return true;
    }
>>>>>>> REPLACE
```

---

### 4. `src/ui/ColumnViewPane.cpp`

```
<<<<<<< SEARCH
    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (!index.isValid() || index.data(SectionHeaderRole).toBool()) return;
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderExpandRequested(itemPath, paneIdx);
        } else if (m_contentPanel) {
            m_contentPanel->onDoubleClicked(index);
        }
    });
=======
    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (m_contentPanel) {
            m_contentPanel->activateIndex(index);
        }
    });
>>>>>>> REPLACE
```

---

### 5. `src/ui/PanelMediator.h`

```
<<<<<<< SEARCH
    QString m_currentQuickLookPath;
=======
>>>>>>> REPLACE
```

---

### 6. `src/ui/PanelMediator.cpp`

```
<<<<<<< SEARCH
        // 1. QuickLook & 文件双击/空格打开与快速预览 (🚀 彻底修复副窗格不能预览的问题)
        connect(panel, &ContentPanel::requestQuickLook, this, [this](const QString& path) {
            m_currentQuickLookPath = path;
            QuickLookWindow::instance().previewFile(path);
        });

        connect(panel, &ContentPanel::fileActivated, this, [this](const QString& path) {
            AppCommand cmd;
            cmd.type = AppCommandType::RecordAccess;
            cmd.targetPaths << path;
            CoreEngine::instance().executeCommand(cmd);

            if (UiHelper::canPreviewFile(path)) {
                m_currentQuickLookPath = path;
                QuickLookWindow::instance().previewFile(path);
            } else {
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            }
        });
=======
        // 1. 文件双击/空格/回车/右键菜单统一文件激活动作
        connect(panel, &ContentPanel::fileActivated, this, [this](const QString& path) {
            if (!UiHelper::canPreviewFile(path)) {
                return;
            }
            AppCommand cmd;
            cmd.type = AppCommandType::RecordAccess;
            cmd.targetPaths << path;
            CoreEngine::instance().executeCommand(cmd);

            QuickLookWindow::instance().preview(path);
        });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(&QuickLookWindow::instance(), &QuickLookWindow::prevRequested, this, [this, contentPanel]() {
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (!target) return;
        QString prev = target->getAdjacentFilePath(m_currentQuickLookPath, -1);
        if (!prev.isEmpty()) {
            m_currentQuickLookPath = prev;
            QuickLookWindow::instance().previewFile(prev);
            target->selectAndScrollToPath(prev);
        }
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::nextRequested, this, [this, contentPanel]() {
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (!target) return;
        QString next = target->getAdjacentFilePath(m_currentQuickLookPath, 1);
        if (!next.isEmpty()) {
            m_currentQuickLookPath = next;
            QuickLookWindow::instance().previewFile(next);
            target->selectAndScrollToPath(next);
        }
    });

    // QuickLook 改星级 -> 同步更新内容面板卡片
    connect(&QuickLookWindow::instance(), &QuickLookWindow::ratingRequested, this, [this, metaPanel, contentPanel](int rating) {
        if (m_currentQuickLookPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetRating;
        cmd.targetPaths << m_currentQuickLookPath;
        cmd.params["rating"] = rating;
        CoreEngine::instance().executeCommand(cmd);

        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (metaPanel) metaPanel->setRating(rating, false);
        if (target) target->updateItemMetadata(m_currentQuickLookPath);
    });

    // QuickLook 改颜色 -> 同步更新内容面板卡片
    connect(&QuickLookWindow::instance(), &QuickLookWindow::colorRequested, this, [this, metaPanel, contentPanel](const QString& color) {
        if (m_currentQuickLookPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths << m_currentQuickLookPath;
        cmd.params["color"] = color;
        CoreEngine::instance().executeCommand(cmd);

        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (metaPanel) metaPanel->setColor(color, false);
        if (target) target->updateItemMetadata(m_currentQuickLookPath);
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::deleteRequested, this, [this, contentPanel](const QString& path) {
        if (path.isEmpty()) return;
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;

        if (TrashService::instance().moveToTrash({path}, target)) {
            if (target) {
                QString next = target->getAdjacentFilePath(path, 1);
                if (!next.isEmpty()) {
                    m_currentQuickLookPath = next;
                    QuickLookWindow::instance().previewFile(next);
                } else {
                    QString prev = target->getAdjacentFilePath(path, -1);
                    if (!prev.isEmpty()) {
                        m_currentQuickLookPath = prev;
                        QuickLookWindow::instance().previewFile(prev);
                    } else {
                        QuickLookWindow::instance().closePreview();
                    }
                }
                target->refreshAll();
            }
        }
    });
=======
    connect(&QuickLookWindow::instance(), &QuickLookWindow::prevRequested, this, [this, contentPanel]() {
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (!target) return;
        QString curPath = QuickLookWindow::instance().currentPath();
        int delta = -1;
        while (true) {
            QString prev = target->getAdjacentFilePath(curPath, delta);
            if (prev.isEmpty()) break;
            if (!QFileInfo(prev).isDir() && UiHelper::canPreviewFile(prev)) {
                QuickLookWindow::instance().preview(prev);
                target->selectAndScrollToPath(prev);
                break;
            }
            delta--;
        }
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::nextRequested, this, [this, contentPanel]() {
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (!target) return;
        QString curPath = QuickLookWindow::instance().currentPath();
        int delta = 1;
        while (true) {
            QString next = target->getAdjacentFilePath(curPath, delta);
            if (next.isEmpty()) break;
            if (!QFileInfo(next).isDir() && UiHelper::canPreviewFile(next)) {
                QuickLookWindow::instance().preview(next);
                target->selectAndScrollToPath(next);
                break;
            }
            delta++;
        }
    });

    // QuickLook 改星级 -> 同步更新内容面板卡片
    connect(&QuickLookWindow::instance(), &QuickLookWindow::ratingRequested, this, [this, metaPanel, contentPanel](int rating) {
        QString curPath = QuickLookWindow::instance().currentPath();
        if (curPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetRating;
        cmd.targetPaths << curPath;
        cmd.params["rating"] = rating;
        CoreEngine::instance().executeCommand(cmd);

        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (metaPanel) metaPanel->setRating(rating, false);
        if (target) target->updateItemMetadata(curPath);
    });

    // QuickLook 改颜色 -> 同步更新内容面板卡片
    connect(&QuickLookWindow::instance(), &QuickLookWindow::colorRequested, this, [this, metaPanel, contentPanel](const QString& color) {
        QString curPath = QuickLookWindow::instance().currentPath();
        if (curPath.isEmpty()) return;

        AppCommand cmd;
        cmd.type = AppCommandType::SetColor;
        cmd.targetPaths << curPath;
        cmd.params["color"] = color;
        CoreEngine::instance().executeCommand(cmd);

        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
        if (metaPanel) metaPanel->setColor(color, false);
        if (target) target->updateItemMetadata(curPath);
    });

    connect(&QuickLookWindow::instance(), &QuickLookWindow::deleteRequested, this, [this, contentPanel](const QString& path) {
        if (path.isEmpty()) return;
        ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;

        if (TrashService::instance().moveToTrash({path}, target)) {
            if (target) {
                int delta = 1;
                QString nextValid;
                while (true) {
                    QString candidate = target->getAdjacentFilePath(path, delta);
                    if (candidate.isEmpty()) break;
                    if (!QFileInfo(candidate).isDir() && UiHelper::canPreviewFile(candidate)) {
                        nextValid = candidate;
                        break;
                    }
                    delta++;
                }

                if (!nextValid.isEmpty()) {
                    QuickLookWindow::instance().preview(nextValid);
                } else {
                    delta = -1;
                    QString prevValid;
                    while (true) {
                        QString candidate = target->getAdjacentFilePath(path, delta);
                        if (candidate.isEmpty()) break;
                        if (!QFileInfo(candidate).isDir() && UiHelper::canPreviewFile(candidate)) {
                            prevValid = candidate;
                            break;
                        }
                        delta--;
                    }
                    if (!prevValid.isEmpty()) {
                        QuickLookWindow::instance().preview(prevValid);
                    } else {
                        QuickLookWindow::instance().closePreview();
                    }
                }
                target->refreshAll();
            }
        }
    });
>>>>>>> REPLACE
```

---

### 7. `src/ui/controllers/ContentContextMenu.cpp`

```
<<<<<<< SEARCH
        case ContentPanel::ActionOpen:
            m_panel->onDoubleClicked(currentIndex);
            break;
=======
        case ContentPanel::ActionOpen:
            m_panel->activateIndex(currentIndex);
            break;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        case ContentPanel::ActionOpenDefault: {
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString filePath = idx.data(PathRole).toString();
                    if (!filePath.isEmpty() && QFileInfo::exists(filePath)) {
                        MetadataManager::instance().recordAccess(filePath.toStdWString());
                        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
                    }
                }
            }
            break;
        }
=======
        case ContentPanel::ActionOpenDefault: {
            auto indexes = view->selectionModel()->selectedIndexes();
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString filePath = idx.data(PathRole).toString();
                    if (!filePath.isEmpty() && QFileInfo::exists(filePath)) {
                        targetPaths << filePath;
                        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
                    }
                }
            }
            if (!targetPaths.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::RecordAccess;
                cmd.targetPaths = targetPaths;
                CoreEngine::instance().executeCommand(cmd);
            }
            break;
        }
>>>>>>> REPLACE
```

---

### 8. `src/ui/QuickLookWindow.h`

```
<<<<<<< SEARCH
    void previewFile(const QString& path);
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void preview(const QString& filePath);
=======
    void preview(const QString& filePath);
    QString currentPath() const { return m_currentPath; }
>>>>>>> REPLACE
```

---

### 9. `src/ui/QuickLookWindow.cpp`

```
<<<<<<< SEARCH
// 静态文件分类后缀定义 (音视频格式并入黑名单进行系统大图标降级预览，不直接播放)
static const QSet<QString> UNPREVIEWABLE_EXTS = {
    "zip", "rar", "7z", "tar", "gz", "bz2", "xz", "exe", "dll", "msi", "sys", "iso", "dmg", "pkg", "bin", "lnk",
    "mp4", "m4v", "mov", "avi", "mkv", "wmv", "flv", "webm", "3gp", "mp3", "wav", "wma", "flac", "aac", "ogg", "m4a", "ape"
};
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void QuickLookWindow::previewFile(const QString& path) {
    preview(path);
}

void QuickLookWindow::preview(const QString& filePath) {
    m_currentPath = filePath;
    QFileInfo fi(filePath);
    m_titleLabel->setText(fi.fileName());
    m_infoLabel->setObjectName("QLInfoLabel");

    QString ext = fi.suffix().toLower();

    if (UiHelper::isGraphicsFile(ext)) {
        renderImage(filePath);
    } else if (UNPREVIEWABLE_EXTS.contains(ext)) {
        // 直接提示不支持，显示其系统图标
        m_graphicsView->hide();
        m_textEdit->hide();

        m_graphicsView->clear();
        m_textEdit->clear();

        QIcon fileIcon = ShellIconManager::getFileIcon(filePath, 256);
        QPixmap pix = fileIcon.pixmap(256, 256);
        m_graphicsView->setPixmap(pix);
        m_graphicsView->show();

        m_infoLabel->setText("该文件类型暂不支持预览");
        m_infoLabel->setObjectName("QLInfoLabelWarn");
    } else {
        renderText(filePath);
    }

    showFullScreen();
    raise();
    activateWindow();

    m_ignoreDeactivate = true;
    QTimer::singleShot(150, this, [this]() {
        m_ignoreDeactivate = false;
    });
    FramelessWindowHelper::setAlwaysOnTop(this, true);
}
=======
void QuickLookWindow::preview(const QString& filePath) {
    m_currentPath = filePath;
    QFileInfo fi(filePath);
    m_titleLabel->setText(fi.fileName());
    m_infoLabel->setObjectName("QLInfoLabel");

    QString ext = fi.suffix().toLower();

    if (UiHelper::isGraphicsFile(ext)) {
        renderImage(filePath);
    } else {
        renderText(filePath);
    }

    showFullScreen();
    raise();
    activateWindow();

    m_ignoreDeactivate = true;
    QTimer::singleShot(150, this, [this]() {
        m_ignoreDeactivate = false;
    });
    FramelessWindowHelper::setAlwaysOnTop(this, true);
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps

1. **构建验证**：执行 `cmake --build build` 确保全部修改点无语法或符号编译错误。
2. **逻辑核验**：
   - 双击/空格/回车/右键菜单“打开”可预览文件（如图片/文本/PDF）：记录访问并调出 QuickLook 预览。
   - 双击/空格/回车/右键菜单“打开”不可预览文件（如 `.exe`/`.zip`）：直接拦截放弃，无动作，绝不调用 `QDesktopServices::openUrl`。
   - 右键菜单“用系统默认程序打开”：触发 `RecordAccess` 命令并调用系统默认程序打开。
   - QuickLook 内部按下箭头/上箭头（prev/next）：自动跳过文件夹及不可预览文件，定位至可预览文件的下一张/上一张。

---

## Chapter 5: Official API Reuse & Anti-Redundancy

- 统一将“打开”行为归口至 `ContentPanel::activateIndex` 和 `PanelMediator.cpp` 中的 `fileActivated` 监听处理，彻底消除多头拦截与重复预览判断逻辑。

---

## Chapter 6: Data SSOT Declaration Table

| 交互行为 | 归一化 SSOT 函数入口 | 处理下行转发 |
| :--- | :--- | :--- |
| **双击/空格/回车/右键“打开”** | `ContentPanel::activateIndex` | 文件夹 → `directorySelected` / 列展开；文件 → `fileActivated` |
| **激活文件动作** | `PanelMediator` 的 `fileActivated` 槽 | `UiHelper::canPreviewFile` 校验 → `RecordAccess` 命令 → `QuickLookWindow::preview` |

---

## Chapter 7: Interface & Visibility Consistency Check

- **`ContentPanel::activateIndex(const QModelIndex& index)`**: `public`
- **`QuickLookWindow::currentPath() const`**: `public`
- 被删除冗余接口：`ContentPanel::requestQuickLook` (信号), `QuickLookWindow::previewFile` (函数)

---

## Chapter 8: Header Chain & Inclusion Closure Check

- 修改触及头文件只涉及方法声明增删，依赖链完全闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 检查项 | 是否满足 | 说明 |
| :--- | :--- | :--- |
| 1. 代码文件零直接修改 | 满足 | 方案物理存储于 `Implementation Plan/` |
| 2. 交互与右键菜单完全保留 | 满足 | “用系统默认程序打开”菜单功能完全保留，仅修改记录访问方式 |
| 3. 无新增其他功能 | 满足 | 严格局限于“打开项目”归一化 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table

- 保留右键菜单“用系统默认程序打开”功能本身。
- 保留列视图样式（蓝线、列宽）及既有重命名交互规则。
