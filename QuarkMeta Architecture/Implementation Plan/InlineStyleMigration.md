# Implementation Plan - Inline `setStyleSheet` Migration to QSS (`InlineStyleMigration.md`)

## 1. Overview
This implementation plan covers the pure migration of hardcoded inline `setStyleSheet` usages from C++ source files into `resources/style.qss`, in strict accordance with `AGENTS.md` 4.1 & 4.4 rules.

### Migration Rules & Zero-Value-Alteration Contract
- **Zero Value Modification**: Every property name, RGB/Hex color, px border, radius, and opacity value is migrated 1:1 without changing any numerical values.
- **Selector Scoping**: QSS selectors use specific object names (`#OrientationPreviewWidget`, `#DragOverlayWidget`, `#FilterItemDot`, `#ColorPickerPreviewBlock`) to prevent style leakage or un-intended inheritance.
- **No Refactoring**: Pure style migration without altering widget lifecycle, layout, or behavioral code.

---

## 2. Modified Files List
- `resources/style.qss`
- `src/ui/controllers/ContentPaneSplitManager.cpp`
- `src/ui/FilterPanel.cpp`
- `src/ui/ColorPicker.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: Add QSS Rules for Overlay Widgets, Filter Dots, and Color Preview Blocks in `resources/style.qss`

```
<<<<<<< SEARCH
/* 导航树形视图分支宽度静态定义 */
QTreeView#NavTreeView::branch {
    width: 20px;
}
=======
/* 导航树形视图分支宽度静态定义 */
QTreeView#NavTreeView::branch {
    width: 20px;
}

/* 分屏拖拽与方向预览遮罩层静态样式 */
QWidget#OrientationPreviewWidget, QWidget#DragOverlayWidget {
    background-color: rgba(0, 122, 255, 0.25);
    border: 2px solid #007AFF;
}

/* 筛选面板颜色图标点静态基础样式 */
QLabel#FilterItemDot {
    border-radius: 5px;
}

/* 颜色拾取器预览方块基础样式 */
QWidget#ColorPickerPreviewBlock {
    border-radius: 4px;
    border: 1px solid #333333;
}
>>>>>>> REPLACE
```

### Change 2: Remove Inline `setStyleSheet` from `src/ui/controllers/ContentPaneSplitManager.cpp`

```
<<<<<<< SEARCH
    if (!m_orientationPreviewWidget) {
        m_orientationPreviewWidget = new QWidget(m_panel);
        m_orientationPreviewWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_orientationPreviewWidget->setStyleSheet("background-color: rgba(0, 122, 255, 0.25); border: 2px solid #007AFF;");
    }
=======
    if (!m_orientationPreviewWidget) {
        m_orientationPreviewWidget = new QWidget(m_panel);
        m_orientationPreviewWidget->setObjectName("OrientationPreviewWidget");
        m_orientationPreviewWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(m_panel);
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_dragOverlayWidget->setStyleSheet("background-color: rgba(0, 122, 255, 0.25); border: 2px solid #007AFF;");
    }
=======
    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(m_panel);
        m_dragOverlayWidget->setObjectName("DragOverlayWidget");
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
>>>>>>> REPLACE
```

### Change 3: Remove Inline `setStyleSheet` from `src/ui/FilterPanel.cpp`

```
<<<<<<< SEARCH
    if (dotColor.isValid() && dotColor != Qt::transparent) {
        QLabel* dot = new QLabel(row);
        dot->setObjectName("FilterItemDot");
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QString("background: %1;").arg(dotColor.name()));
        rl->addWidget(dot);
    }
=======
    if (dotColor.isValid() && dotColor != Qt::transparent) {
        QLabel* dot = new QLabel(row);
        dot->setObjectName("FilterItemDot");
        dot->setFixedSize(10, 10);
        QPalette pal = dot->palette();
        pal.setColor(QPalette::Window, dotColor);
        dot->setAutoFillBackground(true);
        dot->setPalette(pal);
        rl->addWidget(dot);
    }
>>>>>>> REPLACE
```

### Change 4: Remove Inline `setStyleSheet` from `src/ui/ColorPicker.cpp`

```
<<<<<<< SEARCH
void ColorPicker::updatePreview() {
    m_previewBlock->setStyleSheet(QString("background: %1;").arg(m_color.name()));
    m_hexEdit->setText(m_color.name().toUpper());
}
=======
void ColorPicker::updatePreview() {
    QPalette pal = m_previewBlock->palette();
    pal.setColor(QPalette::Window, m_color);
    m_previewBlock->setAutoFillBackground(true);
    m_previewBlock->setPalette(pal);
    m_hexEdit->setText(m_color.name().toUpper());
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Execute CMake build in project root:
   `cmake -B build && cmake --build build --config Release`
2. Run `QuarkMeta` application.
3. Test Drag & Drop Pane Split:
   - Drag a content tab/pane to split. Verify blue overlay rectangle (`rgba(0, 122, 255, 0.25)`) displays identical visuals without inline QSS.
4. Test Color Filter & Color Picker:
   - Open FilterPanel and ColorPicker. Verify dot colors and preview blocks display accurate colors.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Style SSOT**: All static styling rules are centralized in `resources/style.qss`. Dynamic RGB/Hex background colors use `QPalette` / `QPainter` instead of string-concatenated inline QSS.
- **Zero Redundancy**: Removed all string allocations for inline QSS.

---

## 6. Header API Signature Verification
- `QWidget::setObjectName(const QString& name)`: Standard QWidget API.
- `QWidget::setPalette(const QPalette& palette)`: Standard QWidget API.
- `QWidget::setAutoFillBackground(bool enabled)`: Standard QWidget API.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include <QPalette>`: Present in `ColorPicker.cpp` and `FilterPanel.cpp` header chains.
