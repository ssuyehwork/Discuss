# Implementation Plan - LastOperationManager Execution Unification (F4 & Context Menu SSOT)

## 1. Overview
Currently, executing the "Repeat Last Operation" functionality is triggered in two separate locations:
1. **F4 Key Press Handler**: Implemented in `ContentKeyHandler::handleKeyPress()` (lines 389–418 of `src/ui/controllers/ContentKeyHandler.cpp`).
2. **Context Menu Handler**: Implemented in `ContentContextMenu::showMenu()` under `ContentPanel::ActionRepeatLastOp` case (lines 620–653 of `src/ui/controllers/ContentContextMenu.cpp`).

Both locations independently duplicate the logic of:
- Checking `LastOperationManager::instance().hasOperation()` and handling `MoveToFolder`.
- Iterating over selected model indexes (`view->selectionModel()->selectedIndexes()`).
- Filtering out section headers and non-column-0 indexes.
- Executing `model->setData()` for `SetRating`, `SetColor` (with `ShellIconManager::getFileIcon`), and `PasteTags`.
- Displaying feedback tooltips or performing operation feedback.

This physical duplication violates the **Single Source of Truth (SSOT)** contract set forth in `AGENTS.md` (Section 2.4).

This implementation plan unifies the execution logic into a single static method:
`ContentKeyHandler::executeRepeatLastOp(ContentPanel* panel, QAbstractItemView* view)`
Both `ContentKeyHandler` (F4 key) and `ContentContextMenu` (`ActionRepeatLastOp`) will delegate to this SSOT function.

---

## 2. Modified Files List
1. `src/ui/controllers/ContentKeyHandler.h` (Public static interface declaration addition)
2. `src/ui/controllers/ContentKeyHandler.cpp` (Implementation of `executeRepeatLastOp` and simplification of F4 handler)
3. `src/ui/controllers/ContentContextMenu.cpp` (Delegation of `ActionRepeatLastOp` to `executeRepeatLastOp`)

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/controllers/ContentKeyHandler.h`

<<<<<<< SEARCH
    /**
     * @brief 执行快捷移入/重复移入到目标文件夹
     */
    static bool executeMoveToFolder(ContentPanel* panel, const QString& targetDir);
=======
    /**
     * @brief 执行快捷移入/重复移入到目标文件夹
     */
    static bool executeMoveToFolder(ContentPanel* panel, const QString& targetDir);

    /**
     * @brief 重复上一次操作 SSOT 统一入口 (快捷键 F4 与 右键菜单 "重复上一次操作" 共同调用)
     */
    static bool executeRepeatLastOp(ContentPanel* panel, QAbstractItemView* view);
>>>>>>> REPLACE

---

### File 2: `src/ui/controllers/ContentKeyHandler.cpp`

<<<<<<< SEARCH
    // 5. F4: 重复上一次操作 (星级 / 标记颜色 / 粘贴标签 / 快捷移入)
    if (keyEvent->key() == Qt::Key_F4) {
        if (!LastOperationManager::instance().hasOperation()) {
            return true;
        }

        LastOperationType type = LastOperationManager::instance().type();
        if (type == LastOperationType::MoveToFolder) {
            executeMoveToFolder(m_panel, LastOperationManager::instance().destination());
            return true;
        }

        QAbstractItemModel* model = view->model();
        if (!model) return true;
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& targetIdx : indexes) {
            if (targetIdx.column() == 0 && !targetIdx.data(SectionHeaderRole).toBool()) {
                if (type == LastOperationType::SetRating) {
                    model->setData(targetIdx, LastOperationManager::instance().rating(), RatingRole);
                } else if (type == LastOperationType::SetColor) {
                    QString colorVal = LastOperationManager::instance().color();
                    model->setData(targetIdx, colorVal, ColorRole);
                    QString path = targetIdx.data(PathRole).toString();
                    QIcon coloredIcon = ShellIconManager::getFileIcon(path, 128);
                    model->setData(targetIdx, coloredIcon, Qt::DecorationRole);
                } else if (type == LastOperationType::PasteTags) {
                    model->setData(targetIdx, LastOperationManager::instance().tags(), TagsRole);
                }
            }
        }
        return true;
    }
=======
    // 5. F4: 重复上一次操作 (星级 / 标记颜色 / 粘贴标签 / 快捷移入)
    if (keyEvent->key() == Qt::Key_F4) {
        executeRepeatLastOp(m_panel, view);
        return true;
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
bool ContentKeyHandler::executeMoveToFolder(ContentPanel* panel, const QString& targetDir) {
=======
bool ContentKeyHandler::executeRepeatLastOp(ContentPanel* panel, QAbstractItemView* view) {
    if (!LastOperationManager::instance().hasOperation()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "尚未记录任何可重复的操作", 1500, QColor("#e81123"));
        return false;
    }

    LastOperationType type = LastOperationManager::instance().type();
    if (type == LastOperationType::MoveToFolder) {
        return executeMoveToFolder(panel, LastOperationManager::instance().destination());
    }

    if (!view) return false;
    QAbstractItemModel* model = view->model();
    if (!model) return false;

    auto indexes = view->selectionModel()->selectedIndexes();
    int count = 0;
    for (const auto& idx : indexes) {
        if (idx.column() == 0 && !idx.data(SectionHeaderRole).toBool()) {
            if (type == LastOperationType::SetRating) {
                model->setData(idx, LastOperationManager::instance().rating(), RatingRole);
            } else if (type == LastOperationType::SetColor) {
                QString colorVal = LastOperationManager::instance().color();
                model->setData(idx, colorVal, ColorRole);
                QString itemPath = idx.data(PathRole).toString();
                QIcon coloredIcon = ShellIconManager::getFileIcon(itemPath, 128);
                model->setData(idx, coloredIcon, Qt::DecorationRole);
            } else if (type == LastOperationType::PasteTags) {
                model->setData(idx, LastOperationManager::instance().tags(), TagsRole);
            }
            count++;
        }
    }

    if (count > 0) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已对 %1 个项目重复执行上一次操作").arg(count), 1500, QColor("#2ecc71"));
    }
    return count > 0;
}

bool ContentKeyHandler::executeMoveToFolder(ContentPanel* panel, const QString& targetDir) {
>>>>>>> REPLACE

---

### File 3: `src/ui/controllers/ContentContextMenu.cpp`

<<<<<<< SEARCH
        case ContentPanel::ActionRepeatLastOp: {
            if (!LastOperationManager::instance().hasOperation()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "尚未记录任何可重复的操作", 1500, QColor("#e81123"));
                break;
            }
            LastOperationType type = LastOperationManager::instance().type();
            if (type == LastOperationType::MoveToFolder) {
                ContentKeyHandler::executeMoveToFolder(m_panel, LastOperationManager::instance().destination());
                break;
            }
            auto indexes = view->selectionModel()->selectedIndexes();
            QAbstractItemModel* model = view->model();
            int count = 0;
            for (const auto& idx : indexes) {
                if (idx.column() == 0 && model && !idx.data(SectionHeaderRole).toBool()) {
                    if (type == LastOperationType::SetRating) {
                        model->setData(idx, LastOperationManager::instance().rating(), RatingRole);
                    } else if (type == LastOperationType::SetColor) {
                        QString colorVal = LastOperationManager::instance().color();
                        model->setData(idx, colorVal, ColorRole);
                        QString itemPath = idx.data(PathRole).toString();
                        QIcon coloredIcon = ShellIconManager::getFileIcon(itemPath, 128);
                        model->setData(idx, coloredIcon, Qt::DecorationRole);
                    } else if (type == LastOperationType::PasteTags) {
                        model->setData(idx, LastOperationManager::instance().tags(), TagsRole);
                    }
                    count++;
                }
            }
            if (count > 0) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已对 %1 个项目重复执行上一次操作").arg(count), 1500, QColor("#2ecc71"));
            }
            break;
        }
=======
        case ContentPanel::ActionRepeatLastOp: {
            ContentKeyHandler::executeRepeatLastOp(m_panel, view);
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
1. **F4 Key Test**: Select multiple file items in Grid/List View, set a star rating or color tag on one, select another set of items, and press `F4`. Verify that the last operation is applied to all selected items with the overlay feedback tooltip.
2. **Context Menu Test**: Select items, right-click, select "重复上一次操作", and verify identical behavior.
3. **No Redundant Execution**: Ensure logic exists in a single SSOT location (`ContentKeyHandler::executeRepeatLastOp`).

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Reused SSOT Entry Points**:
  - `LastOperationManager::instance()`: Single Source of Truth for last recorded operation state.
  - `ContentKeyHandler::executeRepeatLastOp()`: Single Source of Truth for executing the last recorded operation.
  - `ToolTipOverlay::instance()->showText()`: Standard feedback overlay portal.
- **Anti-Redundancy**: Completely removed duplicate loop iteration, `setData` calls, and feedback tooltips from `ContentContextMenu.cpp` and `ContentKeyHandler.cpp` inline blocks.

---

## 6. Header API Signature Verification

| Class / Component | Function / Method Signature | Header File Path | Status |
| :--- | :--- | :--- | :--- |
| `LastOperationManager` | `static LastOperationManager& instance()` | `src/core/LastOperationManager.h` | Verified Existing |
| `LastOperationManager` | `bool hasOperation() const` | `src/core/LastOperationManager.h` | Verified Existing |
| `LastOperationManager` | `LastOperationType type() const` | `src/core/LastOperationManager.h` | Verified Existing |
| `LastOperationManager` | `int rating() const` | `src/core/LastOperationManager.h` | Verified Existing |
| `LastOperationManager` | `QString color() const` | `src/core/LastOperationManager.h` | Verified Existing |
| `LastOperationManager` | `QStringList tags() const` | `src/core/LastOperationManager.h` | Verified Existing |
| `LastOperationManager` | `QString destination() const` | `src/core/LastOperationManager.h` | Verified Existing |
| `ContentKeyHandler` | `static bool executeMoveToFolder(ContentPanel* panel, const QString& targetDir)` | `src/ui/controllers/ContentKeyHandler.h` | Verified Existing |
| `ContentKeyHandler` | `static bool executeRepeatLastOp(ContentPanel* panel, QAbstractItemView* view)` | `src/ui/controllers/ContentKeyHandler.h` | New Extension (Added) |
| `ToolTipOverlay` | `void showText(const QPoint& pos, const QString& text, int timeoutMs = 1500, const QColor& bgColor = QColor("#2ecc71"))` | `src/ui/widgets/ToolTipOverlay.h` | Verified Existing |
| `ShellIconManager` | `static QIcon getFileIcon(const QString& path, int size = 128)` | `src/utils/ShellIconManager.h` | Verified Existing |

---

## 7. Header Inclusion Chain & Type Completeness Check

- `src/ui/controllers/ContentKeyHandler.cpp`:
  - `#include "../../core/LastOperationManager.h"` (Already included)
  - `#include "../widgets/ToolTipOverlay.h"` (Already included)
  - `#include "../../utils/ShellIconManager.h"` (Already included)
  - `#include "../ContentPanel.h"` (Already included)
- `src/ui/controllers/ContentContextMenu.cpp`:
  - `#include "ContentKeyHandler.h"` (Already included line 26)
  - `#include "../../core/LastOperationManager.h"` (Already included line 34)

All type references (`ContentPanel`, `QAbstractItemView`, `LastOperationManager`, `ToolTipOverlay`, `ShellIconManager`) are fully defined in the compilation units of `ContentKeyHandler.cpp` and `ContentContextMenu.cpp`.
