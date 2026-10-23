# Implementation Plan - LineEdit Clear Button Normalization (`ClearButtonNormalization.md`)

## 1. Overview
In FilterPanel's sub-components (`CreateDateGroup.cpp`, `ModifyDateGroup.cpp`, `FileSizeGroup.cpp`, and `FileTypeGroup.cpp`), `setClearButtonEnabled(true)` was previously called on `QLineEdit` instances. This caused Qt to render the native OS clear button with a grey circular background and hard grey borders, directly violating `Memories.md` section 2.1 ("不得使用操作系统原生带硬灰边的原生 ClearButton 样式毁坏暗黑主题沉浸感").

### Proposed Solution
1. Remove all `setClearButtonEnabled(true)` calls across FilterPanel group widgets.
2. Replace them with trailing `QAction` clear buttons using `UiHelper::getIcon("close", QColor("#888888"))`, matching the SSOT implementation in `SearchController.cpp`.
3. Connect the trailing `QAction::triggered` signal to `QLineEdit::clear` and toggle action visibility dynamically based on `QLineEdit::textChanged`.

---

## 2. Modified Files List
- `src/ui/CreateDateGroup.cpp`
- `src/ui/ModifyDateGroup.cpp`
- `src/ui/FileSizeGroup.cpp`
- `src/ui/FileTypeGroup.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: Replace Native Clear Button in `src/ui/CreateDateGroup.cpp`

```
<<<<<<< SEARCH
    editCreateDate->setClearButtonEnabled(true);
=======
    QAction* clearAct = editCreateDate->addAction(UiHelper::getIcon("close", QColor("#888888")), QLineEdit::TrailingPosition);
    clearAct->setVisible(false);
    connect(clearAct, &QAction::triggered, editCreateDate, &QLineEdit::clear);
    connect(editCreateDate, &QLineEdit::textChanged, this, [clearAct](const QString& text) {
        clearAct->setVisible(!text.isEmpty());
    });
>>>>>>> REPLACE
```

### Change 2: Replace Native Clear Button in `src/ui/ModifyDateGroup.cpp`

```
<<<<<<< SEARCH
    editModifyDate->setClearButtonEnabled(true);
=======
    QAction* clearAct = editModifyDate->addAction(UiHelper::getIcon("close", QColor("#888888")), QLineEdit::TrailingPosition);
    clearAct->setVisible(false);
    connect(clearAct, &QAction::triggered, editModifyDate, &QLineEdit::clear);
    connect(editModifyDate, &QLineEdit::textChanged, this, [clearAct](const QString& text) {
        clearAct->setVisible(!text.isEmpty());
    });
>>>>>>> REPLACE
```

### Change 3: Replace Native Clear Buttons in `src/ui/FileSizeGroup.cpp`

```
<<<<<<< SEARCH
    minEdit->setClearButtonEnabled(true);

    maxEdit->setClearButtonEnabled(true);
=======
    QAction* clearMinAct = minEdit->addAction(UiHelper::getIcon("close", QColor("#888888")), QLineEdit::TrailingPosition);
    clearMinAct->setVisible(false);
    connect(clearMinAct, &QAction::triggered, minEdit, &QLineEdit::clear);
    connect(minEdit, &QLineEdit::textChanged, this, [clearMinAct](const QString& text) {
        clearMinAct->setVisible(!text.isEmpty());
    });

    QAction* clearMaxAct = maxEdit->addAction(UiHelper::getIcon("close", QColor("#888888")), QLineEdit::TrailingPosition);
    clearMaxAct->setVisible(false);
    connect(clearMaxAct, &QAction::triggered, maxEdit, &QLineEdit::clear);
    connect(maxEdit, &QLineEdit::textChanged, this, [clearMaxAct](const QString& text) {
        clearMaxAct->setVisible(!text.isEmpty());
    });
>>>>>>> REPLACE
```

### Change 4: Replace Native Clear Button in `src/ui/FileTypeGroup.cpp`

```
<<<<<<< SEARCH
    editType->setClearButtonEnabled(true);
=======
    QAction* clearAct = editType->addAction(UiHelper::getIcon("close", QColor("#888888")), QLineEdit::TrailingPosition);
    clearAct->setVisible(false);
    connect(clearAct, &QAction::triggered, editType, &QLineEdit::clear);
    connect(editType, &QLineEdit::textChanged, this, [clearAct](const QString& text) {
        clearAct->setVisible(!text.isEmpty());
    });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Execute CMake build:
   `cmake -B build && cmake --build build --config Release`
2. Run `QuarkMeta`.
3. Open FilterPanel ("筛选面板") and test input boxes:
   - Type text in `CreateDate`, `ModifyDate`, `FileSize`, and `FileType` input fields.
   - Verify the trailing clear button appears as a clean SVG `close` icon with no grey circular background or hard grey borders.
   - Click the clear button and verify text is cleared and focus remains on the input field.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Icon SSOT**: Reuses `UiHelper::getIcon("close", QColor("#888888"))` matching `SearchController.cpp` for 100% SVG icon consistency.
- **Memories.md Compliance**: Completely eliminates native OS clear button styles across all filter controls.

---

## 6. Header API Signature Verification
- `QLineEdit::addAction(const QIcon& icon, QLineEdit::ActionPosition position)`: Standard Qt QLineEdit API.
- `UiHelper::getIcon(const QString& name, const QColor& color, int size)`: Verified in `src/ui/UiHelper.h`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "UiHelper.h"`: Included in `CreateDateGroup.cpp`, `ModifyDateGroup.cpp`, `FileSizeGroup.cpp`, `FileTypeGroup.cpp`.
- `#include <QAction>`: Present in Qt header chain via `<QLineEdit>`.
