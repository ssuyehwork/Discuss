# Single Responsibility Principle Architecture Refactoring & Feature Expansion Plan

## 1. Overview & Problem Definition
To prevent architectural degradation and high maintenance costs as new features are added, this implementation plan defines the line-by-line physical decoupling of 14 multi-class/overloaded source files following the Single Responsibility Principle (SRP). It also establishes clean hooks for:
1. Shell protection ("外壳保护", formerly "加密") context menu command.
2. `Del` (Move to Trash), `Shift+Del` (Permanent Delete), `Ctrl+Shift+N` (New Folder) keyboard shortcut dispatching.
3. QuickLook empty text preview fallback ("该项目内容为空").
4. FilterPanel hidden items status bar counter format (`"5个项目，51个已隐藏，选中了1个"`).

## 2. Modified Files & Target Structural Split

### A. Multi-Class File Splitting (One File, One Class)
- `src/core/BasicCommands.h` -> Split into `src/core/commands/`: `RenameCommand.h/.cpp`, `MoveCommand.h/.cpp`, `MetadataCommand.h/.cpp`, `SecureDeleteCommand.h/.cpp`, `ShellProtectionCommand.h/.cpp`, `BatchRenameCommand.h/.cpp`.
- `src/meta/DatabaseMigrator.h` -> Split into `DatabaseMigrator.h/.cpp` and `src/util/VolumePathResolver.h/.cpp`.
- `src/ui/FramelessDialog.h` -> Split into `src/ui/dialogs/`: `FramelessDialog.h/.cpp`, `FramelessInputDialog.h/.cpp`, `FramelessColorPicker.h/.cpp`, `FramelessConfirmDialog.h/.cpp`, `FramelessMessageBox.h/.cpp`.
- `src/ui/DriveButton.h` -> Split into `DriveButton.h/.cpp` and `FolderButton.h/.cpp`.
- `src/ui/ColorPicker.h` -> Split into `src/ui/components/color/`: `SvPicker.h/.cpp`, `HueSlider.h/.cpp`, `ColorPicker.h/.cpp`, `ColorStripPicker.h/.cpp`.
- `src/ui/QuickLookWindow.h` -> Split into `QuickLookGraphicsView.h/.cpp` and `QuickLookWindow.h/.cpp`.
- `src/ui/FilterPanel.h` -> Split into `StyledCheckBox.h/.cpp`, `ClickableRow.h/.cpp`, `FilterState.h`, `FilterPanel.h/.cpp`.

### B. UI Pure Layering & Service Decoupling
- `src/ui/ContentPanel.h/.cpp`: Remove direct `SecureFileEraser`, `EncryptionManager`, and `QtConcurrent` disk scanning. Forward file system actions to `CoreEngine` ActionCommands.
- `src/ui/MetaPanel.h/.cpp`: Eliminate direct `.QuarkMeta.json` disk reads and dynamic `setStyleSheet` polish storms.
- `src/ui/MainWindow.h/.cpp`: Move `WM_DEVICECHANGE` Win32 message handling to `DeviceWatcher`.
- `src/ui/NavPanel.cpp`: Relocate `QtConcurrent` disk scanning logic to `DiskNavigatorService`.

## 3. Detailed Line-by-Line Changes & Design Specifications

### 3.1 Status Bar Filtered Item Counter
```cpp
// In MainWindow.cpp / ContentPanel.cpp status updates:
int visibleCount = m_contentPanel->getProxyModel()->rowCount();
int totalCount = m_contentPanel->model() ? m_contentPanel->model()->rowCount() : visibleCount;
int hiddenCount = totalCount - visibleCount;
int selectedCount = m_contentPanel->getSelectedIndexes().size();

QString statusMsg;
if (hiddenCount > 0) {
    statusMsg = QString("%1 个项目, %2 个已隐藏, 已选中 %3 个")
        .arg(visibleCount).arg(hiddenCount).arg(selectedCount);
} else {
    statusMsg = QString("%1 个项目, 已选中 %2 个")
        .arg(visibleCount).arg(selectedCount);
}
m_statusLeft->setText(statusMsg);
```

### 3.2 QuickLook Empty Text Fallback
```cpp
// In QuickLookWindow.cpp text preview renderer:
if (file.size() == 0 || content.trimmed().isEmpty()) {
    showEmptyStateWidget("该项目内容为空");
    return;
}
```

## 4. Build & Verification Steps
1. Update `CMakeLists.txt` to register all newly created `.h` and `.cpp` files under their respective modules.
2. Run clean build: `cmake -B build && cmake --build build`.
3. Verify that zero compilation errors occur and MOC correctly processes all `Q_OBJECT` macros.
