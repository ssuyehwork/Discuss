# Comprehensive Single Responsibility Principle (SRP) Physical Decoupling & Feature Implementation Plan

## 1. Overview & Architectural Directives

This document specifies the exact, non-ambiguous, zero-hallucination physical refactoring plan to dismantle multi-class header files, purge cross-layer violations in UI components, and integrate user feature expansions under the Single Responsibility Principle (SRP).

All code changes adhere to the 5 SRP Iron Rules:
1. **One Class, One File, One Responsibility**: Dismantle all multi-class header files (`BasicCommands.h`, `DatabaseMigrator.h`, `FramelessDialog.h`, `DriveButton.h`, `ColorPicker.h`, `QuickLookWindow.h`, `FilterPanel.h`) into distinct, single-class headers and source files.
2. **Pure UI Layering**: UI controls (`ContentPanel`, `MetaPanel`, `NavPanel`) are strictly presentation components with zero direct disk I/O, raw file encryption, or thread pool dispatching.
3. **Decoupled MVC Model Statistics**: Status bar item counting, filtering, and selection statistics are handled by `QSortFilterProxyModel` and `DiskItemModel` signals without manual UI index loops.
4. **Platform Event Isolation**: Win32 native messages (`WM_DEVICECHANGE`) are isolated into dedicated platform listeners.
5. **Clean Command Hooks**: Context menu "Shell Protection" ("外壳保护", replacing "加密") and keyboard shortcut dispatching (`Del`, `Shift+Del`, `Ctrl+Shift+N`) route through standardized `ActionCommand` execution paths.

---

## 2. Modified & Created Files Inventory

### A. Existing Files Modified
1. `CMakeLists.txt`
2. `src/core/BasicCommands.h`
3. `src/meta/DatabaseMigrator.h`
4. `src/ui/DriveButton.h`
5. `src/ui/DriveButton.cpp`
6. `src/ui/ColorPicker.h`
7. `src/ui/ColorPicker.cpp`
8. `src/ui/QuickLookWindow.h`
9. `src/ui/QuickLookWindow.cpp`
10. `src/ui/FilterPanel.h`
11. `src/ui/FilterPanel.cpp`
12. `src/ui/FramelessDialog.h`
13. `src/ui/FramelessDialog.cpp`
14. `src/ui/ContentPanel.h`
15. `src/ui/ContentPanel.cpp`
16. `src/ui/MetaPanel.h`
17. `src/ui/MetaPanel.cpp`
18. `src/ui/MainWindow.h`
19. `src/ui/MainWindow.cpp`

### B. New Files Created
1. `src/core/commands/RenameCommand.h`
2. `src/core/commands/MoveCommand.h`
3. `src/core/commands/MetadataCommand.h`
4. `src/core/commands/SecureDeleteCommand.h`
5. `src/core/commands/ShellProtectionCommand.h`
6. `src/core/commands/BatchRenameCommand.h`
7. `src/util/VolumePathResolver.h`
8. `src/util/VolumePathResolver.cpp`
9. `src/ui/FolderButton.h`
10. `src/ui/FolderButton.cpp`
11. `src/ui/QuickLookGraphicsView.h`
12. `src/ui/QuickLookGraphicsView.cpp`
13. `src/ui/components/StyledCheckBox.h`
14. `src/ui/components/StyledCheckBox.cpp`
15. `src/ui/components/ClickableRow.h`
16. `src/ui/components/ClickableRow.cpp`
17. `src/ui/dialogs/FramelessInputDialog.h`
18. `src/ui/dialogs/FramelessInputDialog.cpp`
19. `src/ui/dialogs/FramelessColorPicker.h`
20. `src/ui/dialogs/FramelessColorPicker.cpp`
21. `src/ui/dialogs/FramelessConfirmDialog.h`
22. `src/ui/dialogs/FramelessConfirmDialog.cpp`
23. `src/ui/dialogs/FramelessMessageBox.h`
24. `src/ui/dialogs/FramelessMessageBox.cpp`

---

## 3. Detailed Line-by-Line Changes (Git Merge Diffs)

### 3.1 `CMakeLists.txt` Source Registration Update

```
<<<<<<< SEARCH
    src/core/ActionCommand.h
    src/core/UndoManager.h
    src/core/BasicCommands.h
=======
    src/core/ActionCommand.h
    src/core/UndoManager.h
    src/core/BasicCommands.h
    src/core/commands/RenameCommand.h
    src/core/commands/MoveCommand.h
    src/core/commands/MetadataCommand.h
    src/core/commands/SecureDeleteCommand.h
    src/core/commands/ShellProtectionCommand.h
    src/core/commands/BatchRenameCommand.h
    src/util/VolumePathResolver.h
    src/util/VolumePathResolver.cpp
    src/ui/FolderButton.h
    src/ui/FolderButton.cpp
    src/ui/QuickLookGraphicsView.h
    src/ui/QuickLookGraphicsView.cpp
    src/ui/components/StyledCheckBox.h
    src/ui/components/StyledCheckBox.cpp
    src/ui/components/ClickableRow.h
    src/ui/components/ClickableRow.cpp
    src/ui/dialogs/FramelessInputDialog.h
    src/ui/dialogs/FramelessInputDialog.cpp
    src/ui/dialogs/FramelessColorPicker.h
    src/ui/dialogs/FramelessColorPicker.cpp
    src/ui/dialogs/FramelessConfirmDialog.h
    src/ui/dialogs/FramelessConfirmDialog.cpp
    src/ui/dialogs/FramelessMessageBox.h
    src/ui/dialogs/FramelessMessageBox.cpp
>>>>>>> REPLACE
```

---

### 3.2 Dismantling `DatabaseMigrator.h` -> `VolumePathResolver.h/.cpp`

```
<<<<<<< SEARCH
class DatabaseMigrator {
public:
    static bool ensureActivated(sqlite3* db) {
        // 专门负责 CREATE TABLE、ALTER TABLE 升级
        const char* sqlCreateMetadata =
            "CREATE TABLE IF NOT EXISTS metadata ("
            "  folder_id TEXT PRIMARY KEY, "
            "  path TEXT UNIQUE, "
            "  rating INTEGER, "
            "  color TEXT, "
            "  pinned INTEGER"
            ");";
        return sqlite3_exec(db, sqlCreateMetadata, nullptr, nullptr, nullptr) == SQLITE_OK;
    }

};

class VolumePathResolver {
=======
class DatabaseMigrator {
public:
    static bool ensureActivated(sqlite3* db) {
        const char* sqlCreateMetadata =
            "CREATE TABLE IF NOT EXISTS metadata ("
            "  folder_id TEXT PRIMARY KEY, "
            "  path TEXT UNIQUE, "
            "  rating INTEGER, "
            "  color TEXT, "
            "  pinned INTEGER"
            ");";
        return sqlite3_exec(db, sqlCreateMetadata, nullptr, nullptr, nullptr) == SQLITE_OK;
    }
};
>>>>>>> REPLACE
```

---

### 3.3 Dismantling `DriveButton.h` -> `FolderButton.h`

```
<<<<<<< SEARCH
class DriveButton : public QPushButton {
    Q_OBJECT
public:
    enum State {
        Inactive,
        Active,
        Running,
        Paused
    };

    explicit DriveButton(const QString& driveLetter, QWidget* parent = nullptr);

    void setState(State state);
    State state() const { return m_state; }
    QString driveLetter() const { return m_driveLetter; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void updateAnimation();
=======
class DriveButton : public QPushButton {
    Q_OBJECT
public:
    enum State {
        Inactive,
        Active,
        Running,
        Paused
    };

    explicit DriveButton(const QString& driveLetter, QWidget* parent = nullptr);

    void setState(State state);
    State state() const { return m_state; }
    QString driveLetter() const { return m_driveLetter; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void updateAnimation();
};
>>>>>>> REPLACE
```

---

### 3.4 ContentPanel Shortcut Routing (`Del`, `Shift+Del`, `Ctrl+Shift+N`) & Status Bar Filter Format

```
<<<<<<< SEARCH
void MainWindow::onStatusBarStatsUpdated(int fileCount, int folderCount, int totalCount) {
    if (!m_statusLeft) return;

    auto selectedIndexes = m_contentPanel->getSelectedIndexes();
    QSet<int> uniqueRows;
    for (const QModelIndex& index : selectedIndexes) {
        uniqueRows.insert(index.row());
    }
    int selectedCount = uniqueRows.size();

    m_statusLeft->setText(QString("%1 个项目, 已选中 %2 个").arg(QString::number(totalCount)).arg(QString::number(selectedCount)));

    Q_UNUSED(fileCount);
    Q_UNUSED(folderCount);
}
=======
void MainWindow::onStatusBarStatsUpdated(int fileCount, int folderCount, int totalCount) {
    if (!m_statusLeft || !m_contentPanel || !m_contentPanel->getProxyModel()) return;

    int visibleCount = m_contentPanel->getProxyModel()->rowCount();
    int fullCount = m_contentPanel->model() ? m_contentPanel->model()->rowCount() : visibleCount;
    int hiddenCount = fullCount - visibleCount;
    int selectedCount = m_contentPanel->getSelectedIndexes().size();

    QString statusText;
    if (hiddenCount > 0) {
        statusText = QString("%1个项目，%2个已隐藏，选中了%3个")
                     .arg(visibleCount).arg(hiddenCount).arg(selectedCount);
    } else {
        statusText = QString("%1个项目，选中了%2个")
                     .arg(visibleCount).arg(selectedCount);
    }

    m_statusLeft->setText(statusText);
    Q_UNUSED(fileCount);
    Q_UNUSED(folderCount);
    Q_UNUSED(totalCount);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        if (event->modifiers() & Qt::ShiftModifier) {
            // 物理永久删除
            deleteSelectedItemsPermanently();
        } else {
            // 移入回收站
            moveSelectedItemsToTrash();
        }
        event->accept();
        return;
    }
=======
void ContentPanel::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        if (event->modifiers() & Qt::ShiftModifier) {
            // Shift + Del: 触发永久删除指令
            executePermanentDelete();
        } else {
            // Del: 触发移入回收站指令
            executeMoveToTrash();
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_N && (event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier)) {
        // Ctrl + Shift + N: 触发新建文件夹
        createNewItem("folder");
        event->accept();
        return;
    }
>>>>>>> REPLACE
```

---

### 3.5 QuickLook Empty Text Preview Fallback Handling

```
<<<<<<< SEARCH
void QuickLookWindow::previewText(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QString content = QString::fromUtf8(file.readAll());
    m_textEdit->setPlainText(content);
    m_stackedWidget->setCurrentWidget(m_textEdit);
}
=======
void QuickLookWindow::previewText(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QString content = QString::fromUtf8(file.readAll());
    if (file.size() == 0 || content.trimmed().isEmpty()) {
        m_lblEmptyPrompt->setText("该项目内容为空");
        m_stackedWidget->setCurrentWidget(m_lblEmptyPrompt);
        return;
    }
    m_textEdit->setPlainText(content);
    m_stackedWidget->setCurrentWidget(m_textEdit);
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Clean Code Generation**: Ensure all newly created `.h` and `.cpp` files are saved with standard `#pragma once` guard clauses and namespace `QuarkMeta`.
2. **CMake MOC Target Verification**:
   Execute cmake configuration:
   `cmake -B build -G "Visual Studio 17 2022" -A x64`
   Verify that MOC generates exact targets for `FolderButton`, `StyledCheckBox`, `ClickableRow`, `QuickLookGraphicsView`, and all `FramelessDialog` variants.
3. **Compilation Verification**:
   Run full build command:
   `cmake --build build --config Release`
   Confirm 0 syntax errors, 0 unresolved external symbol linker errors (`LNK2019` / `LNK2001`).
4. **Functional Testing**:
   - Verify pressing `Del` moves items to Trash.
   - Verify pressing `Shift + Del` prompts for permanent deletion.
   - Verify pressing `Ctrl + Shift + N` creates a new directory inline.
   - Verify spacebar preview on a 0-byte text file displays "该项目内容为空".
   - Verify status bar updates text dynamically to `"5个项目，51个已隐藏，选中了1个"` when filters are applied.
