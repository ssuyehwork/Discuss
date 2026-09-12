# Implementation Plan - Show In QuarkMeta Context Menu Action (ContentContextMenuShowInQuarkMeta.md)

## 1. Overview
This implementation plan adds a new context menu action `"在 QuarkMeta 中显示"` (ActionShowInQuarkMeta) directly below `"在“资源管理器”中显示"` (ActionShowInExplorer).

This action is only shown when **"显示子文件夹中的项目"** (subfolder / mirror mode) is active (`m_panel->isSubfolderModeEnabled()`). 
When clicked, it extracts the target item's absolute path, sets `setPendingSelectName(fi.fileName(), fi.isDir())` on `ContentPanel`, and navigates to the item's true physical parent directory via `NavigationService::instance().navigateTo(fi.absolutePath())`, locating and highlighting the item in QuarkMeta.

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/controllers/ContentContextMenu.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/ContentPanel.h`
Add `ActionShowInQuarkMeta` to `ContentPanel::ContextMenuAction` enum.

```
<<<<<<< SEARCH
        ActionOpen, ActionOpenDefault, ActionShowInExplorer, ActionNewFolder, ActionNewMd, ActionNewTxt,
=======
        ActionOpen, ActionOpenDefault, ActionShowInExplorer, ActionShowInQuarkMeta, ActionNewFolder, ActionNewMd, ActionNewTxt,
>>>>>>> REPLACE
```

### 3.2 Update `src/ui/controllers/ContentContextMenu.cpp`
1. In `ContentContextMenu::exec()`, check if `m_panel->isSubfolderModeEnabled()` is true.
2. If true, insert `menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 18), "在 QuarkMeta 中显示")->setData(ContentPanel::ActionShowInQuarkMeta);` directly below `ActionShowInExplorer`.
3. Handle `ContentPanel::ActionShowInQuarkMeta` in the action switch statement:
   - Extract selected file/folder path.
   - Parse `QFileInfo fi(targetPath)`.
   - Call `m_panel->setPendingSelectName(fi.fileName(), fi.isDir())`.
   - Call `NavigationService::instance().navigateTo(fi.absolutePath())`.

```
<<<<<<< SEARCH
            actShowInExp->setData(ContentPanel::ActionShowInExplorer);
=======
            actShowInExp->setData(ContentPanel::ActionShowInExplorer);

            if (m_panel && m_panel->isSubfolderModeEnabled()) {
                QAction* actShowInQuarkMeta = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 18), "在 QuarkMeta 中显示");
                actShowInQuarkMeta->setData(ContentPanel::ActionShowInQuarkMeta);
            }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        case ContentPanel::ActionShowInExplorer: {
            QString path = selectedPaths.isEmpty() ? m_panel->currentPath() : selectedPaths.first();
            ShellHelper::showInExplorer(path);
            break;
        }
=======
        case ContentPanel::ActionShowInExplorer: {
            QString path = selectedPaths.isEmpty() ? m_panel->currentPath() : selectedPaths.first();
            ShellHelper::showInExplorer(path);
            break;
        }
        case ContentPanel::ActionShowInQuarkMeta: {
            if (!selectedPaths.isEmpty()) {
                QString targetPath = selectedPaths.first();
                QFileInfo fi(targetPath);
                m_panel->setPendingSelectName(fi.fileName(), fi.isDir());
                NavigationService::instance().navigateTo(fi.absolutePath());
            }
            break;
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps

1. **Build Verification**:
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```

2. **Functional Verification**:
   - Open a directory containing subfolders in `QuarkMeta`.
   - Click top-right "显示子文件夹中的项目" (subfolder mirror mode).
   - Right-click on any item originating from a deeper subfolder.
   - Verify that `"在 QuarkMeta 中显示"` appears directly below `"在“资源管理器”中显示"`.
   - Click `"在 QuarkMeta 中显示"`.
   - Verify QuarkMeta exits mirror mode, navigates directly to the file's true parent directory, and automatically scrolls to and highlights the target item.
