# Implementation Plan Supplement - ContextMenuFactory-1 (MoveTo Submenu Normalization)

## 1. Overview
This implementation plan supplement updates the "移动到" (Move To) submenu specification in context menus to ensure that:
1. Submenu items display only the target folder's name (`QFileInfo(recentDir).fileName()`) instead of the full path string.
2. The full path is set as a Tooltip (`actMove->setToolTip(recentDir)`).
3. Root directories / drive roots (e.g. `C:\`) fall back to showing the full path if `fileName()` is empty.

---

## 2. Modified Files List
- `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/controllers/ContentContextMenu.cpp`

<<<<<<< SEARCH
                for (const QString& recentDir : recentFolders) {
                    QAction* actMove = moveMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 16), recentDir);
                    connect(actMove, &QAction::triggered, this, [performMoveTo, recentDir]() {
                        performMoveTo(recentDir);
                    });
                }
=======
                for (const QString& recentDir : recentFolders) {
                    QFileInfo dirInfo(recentDir);
                    QString displayName = dirInfo.fileName();
                    if (displayName.isEmpty()) {
                        displayName = recentDir; // 盘符或根目录降级显示原路径
                    }
                    QAction* actMove = moveMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 16), displayName);
                    actMove->setToolTip(recentDir);
                    connect(actMove, &QAction::triggered, this, [performMoveTo, recentDir]() {
                        performMoveTo(recentDir);
                    });
                }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Recompile project: `cmake --build build --config Debug`
2. Launch QuarkMeta app.
3. Right click on any file and open the "移动到" submenu.
4. Verify that items display concise folder names (e.g. "implementation plan") with full paths in tooltips.
