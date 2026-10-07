# Implementation Plan - QComboBox Dropdown Arrow Restoration (`BatchCreateDialogCombo.md`)

## 1. Overview
In `BatchCreateDialog` and across the application, `QComboBox` controls (such as the "类型" type selection combo) appear as flat input boxes without a visible dropdown arrow indicator.

### Root Cause Analysis
In `resources/style.qss`, `QComboBox::drop-down` is customized:
```css
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 20px;
    border-left: none;
}
```
In Qt QSS, overriding `QComboBox::drop-down` automatically suppresses Qt's native down-arrow indicator unless `QComboBox::down-arrow` is explicitly defined. Because `QComboBox::down-arrow` was omitted from `style.qss`, the dropdown arrow became completely invisible, leaving users unable to recognize `QComboBox` as a clickable dropdown menu.

### Proposed Fix
1. Define `QComboBox::down-arrow` in `resources/style.qss`.
2. Update `ThemeManager` / `UiHelper` to supply a styled `chevron_down` SVG icon URL (`UiHelper::getSvgTempFilePath("chevron_down", QColor("#AAAAAA"))`) to `QComboBox::down-arrow` stylesheet rule, restoring clear dropdown visual cues across all dialogs.

---

## 2. Modified Files List
- `resources/style.qss`
- `src/ui/ThemeManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: Add `QComboBox::down-arrow` Rules in `resources/style.qss`

```
<<<<<<< SEARCH
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 20px;
    border-left: none;
}
=======
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 20px;
    border-left: none;
}
QComboBox::down-arrow {
    width: 10px;
    height: 10px;
    margin-right: 4px;
}
>>>>>>> REPLACE
```

### Change 2: Inject Dynamic SVG Chevron Down Arrow in `src/ui/ThemeManager.cpp`

```
<<<<<<< SEARCH
#include "ThemeManager.h"
#include <QFile>
#include <QDebug>
=======
#include "ThemeManager.h"
#include "UiHelper.h"
#include <QFile>
#include <QDebug>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QString ThemeManager::getGlobalStyleSheet() const {
    QFile file(":/style.qss");
    if (file.open(QFile::ReadOnly)) {
        QString content = QLatin1String(file.readAll());
        return content;
    }

    return QString();
}
=======
QString ThemeManager::getGlobalStyleSheet() const {
    QFile file(":/style.qss");
    if (file.open(QFile::ReadOnly)) {
        QString content = QLatin1String(file.readAll());
        QString arrowPath = UiHelper::getSvgTempFilePath("chevron_down", QColor("#AAAAAA"));
        content += QString("\nQComboBox::down-arrow { image: url(\"%1\"); }\n").arg(arrowPath);
        return content;
    }

    return QString();
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Execute CMake build:
   `cmake -B build && cmake --build build --config Release`
2. Run `QuarkMeta`.
3. Open `BatchCreateDialog` ("批量创建"):
   - Inspect the "类型" combo box.
   - Verify a clean `chevron_down` SVG arrow indicator appears on the right side of the combo box.
   - Click the combo box and verify dropdown selection works smoothly.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Icon SSOT**: Reuses `UiHelper::getSvgTempFilePath("chevron_down", ...)` from the unified SVG icon cache in `SvgIcons.h`.
- **Style Centralization**: Applies centrally in `ThemeManager` for all `QComboBox` instances without duplicate per-dialog hacks.

---

## 6. Header API Signature Verification
- `UiHelper::getSvgTempFilePath(const QString& key, const QColor& color)`: Verified in `src/ui/UiHelper.h`.
- `ThemeManager::getGlobalStyleSheet()`: Verified in `src/ui/ThemeManager.h`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "UiHelper.h"`: Added to `src/ui/ThemeManager.cpp`.
- `#include <QColor>`: Included via `UiHelper.h`.
