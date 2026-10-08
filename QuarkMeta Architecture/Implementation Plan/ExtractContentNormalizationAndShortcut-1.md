# ExtractContentNormalizationAndShortcut-1.md Implementation Plan

## 1. Overview
Iteration on `ExtractContentNormalizationAndShortcut.md`: Adds the normalized "Extract Content" (支持提取内容) right-click context menu action into `QuickLookWindow.cpp` using the SSOT API `ContextMenuFactory::buildExtractContentAction`.

## 2. Modified Files List
- `src/ui/QuickLookWindow.cpp`

## 3. Detailed Line-by-Line Changes

```path
src/ui/QuickLookWindow.cpp
```

<<<<<<< SEARCH
    ContextMenuFactory::buildCopyNameAction(&menu, QStringList{m_currentPath}, this);
    ContextMenuFactory::buildCopyPathAction(&menu, QStringList{m_currentPath}, this);
    FavoriteService::instance().buildFavoriteAction(&menu, m_currentPath, this);
=======
    ContextMenuFactory::buildCopyNameAction(&menu, QStringList{m_currentPath}, this);
    ContextMenuFactory::buildCopyPathAction(&menu, QStringList{m_currentPath}, this);
    ContextMenuFactory::buildExtractContentAction(&menu, m_currentPath, this);
    FavoriteService::instance().buildFavoriteAction(&menu, m_currentPath, this);
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. Run CMake build / compile script to compile the project.
2. Launch the application, press Space on any text file to open `QuickLookWindow`.
3. Right-click inside `QuickLookWindow` to open the context menu.
4. Verify that "支持提取内容" appears in the menu and clicking it copies the text content to clipboard with a green feedback overlay.
5. Press Space on a non-text file or image, right-click, and verify that "不支持提取内容" is disabled with `"prohibit"` icon.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `ContextMenuFactory::buildExtractContentAction` SSOT menu builder.
- Maintained 100% behavior parity across ContentPanel context menus and QuickLookWindow context menu.

## 6. Header API Signature Verification
- `ContextMenuFactory::buildExtractContentAction(QMenu* menu, const QString& path, QObject* receiver)`: Static method declared in `ContextMenuFactory.h`.

## 7. Header Inclusion Chain & Type Completeness Check
- `QuickLookWindow.cpp` includes `"controllers/ContextMenuFactory.h"` on line 16.
- Type completeness for `QMenu` and `QAction` is 100% verified.
