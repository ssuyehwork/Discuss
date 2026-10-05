# Implementation Plan - ActionPasteTags Execution Unification (Paste Tags SSOT)

## 1. Overview
Currently, executing the "Paste Tags" (`ActionPasteTags`) action is implemented in two duplicated locations:
1. `ContentContextMenu::showMenu()` under `ContentPanel::ActionPasteTags` case (lines 830–845 of `src/ui/controllers/ContentContextMenu.cpp`).
2. `ContentKeyHandler::handleKeyPress()` under `Ctrl + Shift + V` hotkey branch (lines 408–427 of `src/ui/controllers/ContentKeyHandler.cpp`).

Both locations independently duplicate the following steps:
- Retrieving copied tags via `ClipboardService::instance().copiedTags()`.
- Displaying warning tooltips when clipboard contains no tags.
- Iterating over selected model indexes (`view->selectionModel()->selectedIndexes()`).
- Applying `model->setData(targetIdx, copiedTags, TagsRole)`.
- Displaying success feedback tooltips via `ToolTipOverlay`.

This duplication violates the **Single Source of Truth (SSOT)** rules specified in `AGENTS.md` (Section 2.4).

This implementation plan unifies the "Paste Tags" execution into a single SSOT entry point in `ClipboardService`:
- `ClipboardService::instance().executePasteTags(QAbstractItemView* view)`

Both `ContentKeyHandler` (`Ctrl + Shift + V`) and `ContentContextMenu` (`ActionPasteTags`) will delegate directly to this SSOT function.

---

## 2. Modified Files List
1. `src/core/ClipboardService.h` (Public `executePasteTags` declaration with forward declaration `#include <QAbstractItemView>`)
2. `src/core/ClipboardService.cpp` (`executePasteTags` implementation)
3. `src/ui/controllers/ContentKeyHandler.cpp` (Delegation of `Ctrl + Shift + V` tag pasting to `ClipboardService::instance().executePasteTags(view)`)
4. `src/ui/controllers/ContentContextMenu.cpp` (Delegation of `ActionPasteTags` case to `ClipboardService::instance().executePasteTags(view)`)

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/core/ClipboardService.h`

<<<<<<< SEARCH
#include <QObject>
#include <QStringList>
#include <QWidget>

namespace QuarkMeta {

class ClipboardService : public QObject {
=======
#include <QObject>
#include <QStringList>
#include <QWidget>
#include <QAbstractItemView>

namespace QuarkMeta {

class ClipboardService : public QObject {
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 标签剪贴板方法
    void setCopiedTags(const QStringList& tags);
    QStringList copiedTags() const;
    bool hasCopiedTags() const;
    void clearCopiedTags();
=======
    // 标签剪贴板方法
    void setCopiedTags(const QStringList& tags);
    QStringList copiedTags() const;
    bool hasCopiedTags() const;
    void clearCopiedTags();

    /**
     * @brief 粘贴标签至目标视图选中项的 SSOT 统一入口 (包含校验、Model更新与 ToolTip 提示)
     */
    bool executePasteTags(QAbstractItemView* view);
>>>>>>> REPLACE

---

### File 2: `src/core/ClipboardService.cpp`

<<<<<<< SEARCH
#include "ClipboardService.h"
#include "../util/ShellHelper.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QThread>
=======
#include "ClipboardService.h"
#include "../util/ShellHelper.h"
#include "../ui/ToolTipOverlay.h"
#include "../core/ModelContract.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QThread>
#include <QCursor>
>>>>>>> REPLACE

<<<<<<< SEARCH
void ClipboardService::clearCopiedTags() {
    m_copiedTags.clear();
}
=======
void ClipboardService::clearCopiedTags() {
    m_copiedTags.clear();
}

bool ClipboardService::executePasteTags(QAbstractItemView* view) {
    if (!view) return false;

    if (m_copiedTags.isEmpty()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "剪贴板无有效标签", 1500, QColor("#e81123"));
        return false;
    }

    QAbstractItemModel* model = view->model();
    if (!model) return false;

    auto indexes = view->selectionModel()->selectedIndexes();
    int count = 0;
    for (const auto& targetIdx : indexes) {
        if (targetIdx.column() == 0 && !targetIdx.data(SectionHeaderRole).toBool()) {
            model->setData(targetIdx, m_copiedTags, TagsRole);
            count++;
        }
    }

    if (count > 0) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已将标签粘贴至 %1 个项目").arg(count), 1500, QColor("#2ecc71"));
    }
    return count > 0;
}
>>>>>>> REPLACE

---

### File 3: `src/ui/controllers/ContentKeyHandler.cpp`

<<<<<<< SEARCH
        if (keyEvent->key() == Qt::Key_V) {
            QStringList copiedTags = ClipboardService::instance().copiedTags();
            if (copiedTags.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "剪贴板无有效标签", 1500, QColor("#e81123"));
                return true;
            }
            QAbstractItemModel* model = view->model();
            auto indexes = view->selectionModel()->selectedIndexes();
            int count = 0;
            for (const auto& targetIdx : indexes) {
                if (model && targetIdx.column() == 0 && !targetIdx.data(SectionHeaderRole).toBool()) {
                    model->setData(targetIdx, copiedTags, TagsRole);
                    count++;
                }
            }
            if (count > 0) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已将标签粘贴至 %1 个项目").arg(count), 1500, QColor("#2ecc71"));
            }
            return true;
        }
=======
        if (keyEvent->key() == Qt::Key_V) {
            ClipboardService::instance().executePasteTags(view);
            return true;
        }
>>>>>>> REPLACE

---

### File 4: `src/ui/controllers/ContentContextMenu.cpp`

<<<<<<< SEARCH
        case ContentPanel::ActionPasteTags: {
            QStringList copiedTags = ClipboardService::instance().copiedTags();
            if (copiedTags.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "剪贴板无有效标签", 1500, QColor("#e81123"));
                break;
            }
            auto indexes = view->selectionModel()->selectedIndexes();
            QAbstractItemModel* model = view->model();
            int count = 0;
            for (const auto& idx : indexes) {
                if (idx.column() == 0 && model && !idx.data(SectionHeaderRole).toBool()) {
                    model->setData(idx, copiedTags, TagsRole);
                    count++;
                }
            }
            if (count > 0) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已将标签粘贴至 %1 个项目").arg(count), 1500, QColor("#2ecc71"));
            }
            break;
        }
=======
        case ContentPanel::ActionPasteTags: {
            ClipboardService::instance().executePasteTags(view);
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
1. **Shortcut `Ctrl + Shift + V` Test**: Copy tags using `Ctrl + Shift + C`, select items in Grid/List View, press `Ctrl + Shift + V`. Verify tags are applied and overlay tooltip displays item count.
2. **Context Menu Test**: Right-click selected items, choose "粘贴标签". Verify identical behavior and overlay tooltip.
3. **SSOT Centralization**: Confirm tag paste logic flows exclusively through `ClipboardService::instance().executePasteTags(view)`.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Reused SSOT Entry Points**:
  - `ClipboardService::instance().executePasteTags(view)`: Single Source of Truth for executing tag paste.
  - `ToolTipOverlay::instance()->showText()`: Standard feedback overlay portal.
- **Anti-Redundancy**: Completely removed duplicate loop iteration, `setData` calls, and feedback tooltips from `ContentContextMenu.cpp` and `ContentKeyHandler.cpp`.

---

## 6. Header API Signature Verification

| Class / Component | Function / Method Signature | Header File Path | Status |
| :--- | :--- | :--- | :--- |
| `ClipboardService` | `bool executePasteTags(QAbstractItemView* view)` | `src/core/ClipboardService.h` | New Addition |
| `ClipboardService` | `QStringList copiedTags() const` | `src/core/ClipboardService.h` | Verified Existing |

---

## 7. Header Inclusion Chain & Type Completeness Check

- `src/core/ClipboardService.h`:
  - Includes `<QAbstractItemView>` explicitly. Resolves MSVC C2061/C2511 type compilation requirements.
- `src/core/ClipboardService.cpp`:
  - Includes `"../ui/ToolTipOverlay.h"`, `"../core/ModelContract.h"`, `<QCursor>`.
- `src/ui/controllers/ContentKeyHandler.cpp`:
  - Includes `"../../core/ClipboardService.h"`.
- `src/ui/controllers/ContentContextMenu.cpp`:
  - Includes `"../../core/ClipboardService.h"`.
