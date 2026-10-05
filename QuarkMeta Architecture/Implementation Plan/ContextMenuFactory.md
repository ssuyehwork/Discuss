# Implementation Plan - ContextMenuFactory Copy Path/Name SSOT Unification

## 1. Overview
Currently, the logic for copying file paths and file names to the system clipboard is scattered and implemented independently across multiple controller classes:
1. `ContextMenuFactory::buildCopyPathAction()` and `buildCopyNameAction()` (in `src/ui/controllers/ContextMenuFactory.cpp`, lines 32 & 47) using `\n` line separator.
2. `ContentKeyHandler::handleKeyPress()` (in `src/ui/controllers/ContentKeyHandler.cpp`, line 364) using `\r\n` line separator for Ctrl+C fallback.
3. `ContentContextMenu::showMenu()` under `ActionCopyName` (line 934) using `\r\n` and `ActionCopyPath` (line 952) using `\n`.

This inconsistency in line separators (`\r\n` vs `\n`) and duplicate calls to `QApplication::clipboard()->setText()` and `ToolTipOverlay` violates the **Single Source of Truth (SSOT)** rules specified in `AGENTS.md` (Section 2.4).

This implementation plan unifies path/name copying into two static SSOT entry points in `ContextMenuFactory`:
- `ContextMenuFactory::copyPathsToClipboard(const QStringList& paths, bool showOverlay = true)`
- `ContextMenuFactory::copyNamesToClipboard(const QStringList& paths, bool showOverlay = true)`

All callers (`buildCopyPathAction`, `buildCopyNameAction`, `ContentKeyHandler`, `ContentContextMenu`) will delegate to these two SSOT entry points using standard native separators and `\r\n` line breaks.

---

## 2. Modified Files List
1. `src/ui/controllers/ContextMenuFactory.h` (Public static SSOT function declarations)
2. `src/ui/controllers/ContextMenuFactory.cpp` (SSOT implementations and action builder refactoring)
3. `src/ui/controllers/ContentKeyHandler.cpp` (Delegation of Ctrl+C fallback path copying)
4. `src/ui/controllers/ContentContextMenu.cpp` (Delegation of `ActionCopyName` and `ActionCopyPath`)

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/controllers/ContextMenuFactory.h`

<<<<<<< SEARCH
class ContextMenuFactory {
public:
    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);
    static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver = nullptr);
};
=======
class ContextMenuFactory {
public:
    /**
     * @brief 复制路径列表至剪贴板 SSOT 入口 (统一原生路径风格与 \r\n 换行符)
     */
    static bool copyPathsToClipboard(const QStringList& paths, bool showOverlay = true);

    /**
     * @brief 复制文件名列表至剪贴板 SSOT 入口 (统一 \r\n 换行符)
     */
    static bool copyNamesToClipboard(const QStringList& paths, bool showOverlay = true);

    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);
    static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver = nullptr);
};
>>>>>>> REPLACE

---

### File 2: `src/ui/controllers/ContextMenuFactory.cpp`

<<<<<<< SEARCH
QAction* ContextMenuFactory::buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制完整路径");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        QStringList cleanPaths;
        for (const auto& p : paths) {
            cleanPaths << QDir::toNativeSeparators(p);
        }
        QApplication::clipboard()->setText(cleanPaths.join("\n"));
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制路径至剪贴板", 1500, Style::SuccessGreen);
    });
    return action;
}

QAction* ContextMenuFactory::buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        QStringList names;
        for (const auto& p : paths) {
            names << QFileInfo(p).fileName();
        }
        QApplication::clipboard()->setText(names.join("\n"));
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制名称至剪贴板", 1500, Style::SuccessGreen);
    });
    return action;
}
=======
bool ContextMenuFactory::copyPathsToClipboard(const QStringList& paths, bool showOverlay) {
    if (paths.isEmpty()) return false;

    QStringList cleanPaths;
    for (const auto& p : paths) {
        if (!p.isEmpty()) {
            cleanPaths << QDir::toNativeSeparators(p);
        }
    }
    if (cleanPaths.isEmpty()) return false;

    QApplication::clipboard()->setText(cleanPaths.join("\r\n"));
    if (showOverlay) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制路径至剪贴板", 1500, Style::SuccessGreen);
    }
    return true;
}

bool ContextMenuFactory::copyNamesToClipboard(const QStringList& paths, bool showOverlay) {
    if (paths.isEmpty()) return false;

    QStringList names;
    for (const auto& p : paths) {
        if (!p.isEmpty()) {
            names << QFileInfo(p).fileName();
        }
    }
    if (names.isEmpty()) return false;

    QApplication::clipboard()->setText(names.join("\r\n"));
    if (showOverlay) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制名称至剪贴板", 1500, Style::SuccessGreen);
    }
    return true;
}

QAction* ContextMenuFactory::buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制完整路径");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        copyPathsToClipboard(paths, true);
    });
    return action;
}

QAction* ContextMenuFactory::buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        copyNamesToClipboard(paths, true);
    });
    return action;
}
>>>>>>> REPLACE

---

### File 3: `src/ui/controllers/ContentKeyHandler.cpp`

<<<<<<< SEARCH
            // Fallback: 复制绝对路径
            QStringList paths;
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& selIdx : indexes) {
                if (selIdx.column() == 0) paths << QDir::toNativeSeparators(selIdx.data(PathRole).toString());
            }
            if (!paths.isEmpty()) QApplication::clipboard()->setText(paths.join("\r\n"));
            return true;
=======
            // Fallback: 复制绝对路径
            QStringList paths;
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& selIdx : indexes) {
                if (selIdx.column() == 0) paths << selIdx.data(PathRole).toString();
            }
            ContextMenuFactory::copyPathsToClipboard(paths, false);
            return true;
>>>>>>> REPLACE

---

### File 4: `src/ui/controllers/ContentContextMenu.cpp`

<<<<<<< SEARCH
        case ContentPanel::ActionCopyName: {
            QModelIndexList indexes = m_panel->getSelectedIndexes();
            QStringList targetNames;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetNames << QFileInfo(p).fileName();
                }
            }
            if (targetNames.isEmpty() && !path.isEmpty()) {
                targetNames << QFileInfo(path).fileName();
            }
            if (!targetNames.isEmpty()) {
                QApplication::clipboard()->setText(targetNames.join("\r\n"));
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制文件名到剪贴板", 1200, QColor("#2ecc71"));
            }
            break;
        }
        case ContentPanel::ActionCopyPath: {
            QModelIndexList indexes = m_panel->getSelectedIndexes();
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetPaths << QDir::toNativeSeparators(p);
                }
            }
            if (targetPaths.isEmpty() && !path.isEmpty()) {
                targetPaths << QDir::toNativeSeparators(path);
            }
            if (!targetPaths.isEmpty()) {
                QApplication::clipboard()->setText(targetPaths.join("\n"));
            }
            break;
        }
=======
        case ContentPanel::ActionCopyName: {
            QModelIndexList indexes = m_panel->getSelectedIndexes();
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetPaths << p;
                }
            }
            if (targetPaths.isEmpty() && !path.isEmpty()) {
                targetPaths << path;
            }
            ContextMenuFactory::copyNamesToClipboard(targetPaths, true);
            break;
        }
        case ContentPanel::ActionCopyPath: {
            QModelIndexList indexes = m_panel->getSelectedIndexes();
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetPaths << p;
                }
            }
            if (targetPaths.isEmpty() && !path.isEmpty()) {
                targetPaths << path;
            }
            ContextMenuFactory::copyPathsToClipboard(targetPaths, true);
            break;
        }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### Build Command
```bash
cmake -B build -S .
cmake --build build --config Release
```

### Verification Methods
1. **Copy Path Test**: Select multiple files in Grid/List view, right click and choose "复制完整路径" or press Ctrl+C without selected tags. Paste into Notepad or text editor and verify paths are separated by `\r\n`.
2. **Copy Name Test**: Right click and select "复制名称". Verify names are separated by `\r\n`.
3. **SSOT Centralization**: Confirm all clipboard operations for paths and names flow exclusively through `ContextMenuFactory::copyPathsToClipboard` and `ContextMenuFactory::copyNamesToClipboard`.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Reused SSOT Entry Points**:
  - `ContextMenuFactory::copyPathsToClipboard(...)`
  - `ContextMenuFactory::copyNamesToClipboard(...)`
  - `ToolTipOverlay::instance()->showText(...)`
- **Anti-Redundancy**: Removed duplicate `QApplication::clipboard()->setText(...)` calls and line join operations from `ContentKeyHandler.cpp` and `ContentContextMenu.cpp`.

---

## 6. Header API Signature Verification

| Class / Component | Function / Method Signature | Header File Path | Status |
| :--- | :--- | :--- | :--- |
| `ContextMenuFactory` | `static bool copyPathsToClipboard(const QStringList& paths, bool showOverlay = true)` | `src/ui/controllers/ContextMenuFactory.h` | New Addition |
| `ContextMenuFactory` | `static bool copyNamesToClipboard(const QStringList& paths, bool showOverlay = true)` | `src/ui/controllers/ContextMenuFactory.h` | New Addition |
| `ContextMenuFactory` | `static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr)` | `src/ui/controllers/ContextMenuFactory.h` | Verified Existing |
| `ContextMenuFactory` | `static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr)` | `src/ui/controllers/ContextMenuFactory.h` | Verified Existing |

---

## 7. Header Inclusion Chain & Type Completeness Check

- `src/ui/controllers/ContextMenuFactory.h`:
  - Contains `#include <QStringList>`.
- `src/ui/controllers/ContextMenuFactory.cpp`:
  - Includes `<QApplication>`, `<QClipboard>`, `<QDir>`, `<QFileInfo>`, `"../ToolTipOverlay.h"`, `"../StyleLibrary.h"`.
- `src/ui/controllers/ContentKeyHandler.cpp`:
  - Includes `"ContextMenuFactory.h"`.
- `src/ui/controllers/ContentContextMenu.cpp`:
  - Includes `"ContextMenuFactory.h"`.
