# Implementation Plan - C++ Inline StyleSheet Unification to resources/style.qss

## 1. Overview
Currently, several UI components directly invoke inline `setStyleSheet(...)` calls with hardcoded CSS strings, RGB/Hex color values (`#444444`, `#EEEEEE`, `#888888`), padding, and border attributes in C++ code:
1. `src/ui/MainWindow.cpp` (lines 446 & 452: separator line styling)
2. `src/ui/FilterPanel.cpp` (line 851: brand title label styling)
3. `src/ui/PresetTagsDialog.cpp` (lines 41, 47, 53, 59, 82, 92: dialog label, edit, tag container, and button styling)
4. `src/ui/FileCollisionDialog.cpp` (lines 53, 63, 77: collision dialog text, checkbox, and button styling)

This violates the **UI Style SSOT Contract** specified in `AGENTS.md` (Section 4.1):
> "禁止 C++ 内联样式硬编码：任何样式修改必须在 resources/style.qss 与 ThemeManager 框架下按规范实施，严禁在 C++ 控件代码中采用内联 setStyleSheet(...) 方式硬编码样式。"

In strict compliance with the **Zero-Value-Alteration Contract** (`AGENTS.md` Section 4.4):
- Every single property value (Hex colors, border thickness, border radius, padding, font size, margins) is preserved 100% exactly without changing any numeric value or color code.
- C++ controls are assigned clear `setObjectName(...)` selectors.
- All inline style declarations are migrated into `resources/style.qss`.

---

## 2. Modified Files List
1. `resources/style.qss` (Adding QSS rule declarations for dialogs, separators, and title labels)
2. `src/ui/MainWindow.cpp` (Replacing inline `setStyleSheet` calls on separator lines with `setObjectName("MenuSeparatorLine")`)
3. `src/ui/FilterPanel.cpp` (Replacing inline `setStyleSheet` on `m_titleLabel` with `setObjectName("FilterPanelTitleLabel")`)
4. `src/ui/PresetTagsDialog.cpp` (Replacing inline `setStyleSheet` calls on controls with object names)
5. `src/ui/FileCollisionDialog.cpp` (Replacing inline `setStyleSheet` calls on controls with object names)

---

## 3. Detailed Line-by-Line Changes

### File 1: `resources/style.qss`

<<<<<<< SEARCH
/* 批次三重构样式：悬浮浮层与历史面板外联 QSS 选择器 */
=======
/* 菜单分割线与标题层内联样式收拢 */
QFrame#MenuSeparatorLine {
    background-color: #444444;
    border: none;
}

QLabel#FilterPanelTitleLabel {
    color: #FFD700;
}

/* 预设标签弹窗 PresetTagsDialog 无损收拢 */
QLabel#PresetFolderLabel, QLabel#PresetTagLabel {
    color: #888888;
    font-size: 11px;
    font-weight: bold;
}

QLineEdit#PresetFolderNameEdit {
    background-color: #1E1E1E;
    border: 1px solid #3C3C3C;
    border-radius: 4px;
    color: #EEEEEE;
    font-size: 12px;
    padding: 4px;
}

QWidget#PresetTagContainer {
    background-color: #1E1E1E;
    border: 1px solid #3C3C3C;
    border-radius: 4px;
}

QPushButton#PresetBtnSave {
    background-color: #378ADD;
    color: #FFFFFF;
    border: none;
    border-radius: 4px;
    font-weight: bold;
}

QPushButton#PresetBtnSave:hover {
    background-color: #4A90E2;
}

QPushButton#PresetBtnCancel {
    background-color: #3E3E42;
    color: #CCCCCC;
    border: none;
    border-radius: 4px;
}

QPushButton#PresetBtnCancel:hover {
    background-color: #4E4E52;
}

/* 文件冲突弹窗 FileCollisionDialog 无损收拢 */
QLabel#CollisionTextLabel {
    color: #EEEEEE;
    font-size: 13px;
}

QCheckBox#CollisionApplyAllChk {
    color: #CCCCCC;
    font-size: 12px;
}

QPushButton#CollisionOptionBtn {
    background-color: #2D2D30;
    color: #EEEEEE;
    border: 1px solid #3E3E42;
    border-radius: 4px;
    padding: 8px 12px;
    text-align: left;
}

QPushButton#CollisionOptionBtn:hover {
    background-color: #3E3E42;
    border-color: #007ACC;
}

/* 批次三重构样式：悬浮浮层与历史面板外联 QSS 选择器 */
>>>>>>> REPLACE

---

### File 2: `src/ui/MainWindow.cpp`

<<<<<<< SEARCH
    sepLineSort->setStyleSheet("background-color: #444444; border: none;");
=======
    sepLineSort->setObjectName("MenuSeparatorLine");
>>>>>>> REPLACE

<<<<<<< SEARCH
    sepLine->setStyleSheet("background-color: #444444; border: none;");
=======
    sepLine->setObjectName("MenuSeparatorLine");
>>>>>>> REPLACE

---

### File 3: `src/ui/FilterPanel.cpp`

<<<<<<< SEARCH
    m_titleLabel->setStyleSheet(QString("color: %1;").arg(brandYellow.name()));
=======
    m_titleLabel->setObjectName("FilterPanelTitleLabel");
>>>>>>> REPLACE

---

### File 4: `src/ui/PresetTagsDialog.cpp`

<<<<<<< SEARCH
    folderNameLabel->setStyleSheet("color: #888; font-size: 11px; font-weight: bold;");
=======
    folderNameLabel->setObjectName("PresetFolderLabel");
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_folderNameEdit->setStyleSheet(
        "QLineEdit { background-color: #1E1E1E; border: 1px solid #3C3C3C; border-radius: 4px; color: #EEEEEE; font-size: 12px; padding: 4px; }"
    );
=======
    m_folderNameEdit->setObjectName("PresetFolderNameEdit");
>>>>>>> REPLACE

<<<<<<< SEARCH
    tagLabel->setStyleSheet("color: #888; font-size: 11px; font-weight: bold;");
=======
    tagLabel->setObjectName("PresetTagLabel");
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_tagContainer->setStyleSheet(
        "QWidget { background-color: #1E1E1E; border: 1px solid #3C3C3C; border-radius: 4px; }"
    );
=======
    m_tagContainer->setObjectName("PresetTagContainer");
>>>>>>> REPLACE

<<<<<<< SEARCH
    btnSave->setStyleSheet(
        "QPushButton { background-color: #378ADD; color: #FFFFFF; border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #4A90E2; }"
    );
=======
    btnSave->setObjectName("PresetBtnSave");
>>>>>>> REPLACE

<<<<<<< SEARCH
    btnCancel->setStyleSheet(
        "QPushButton { background-color: #3E3E42; color: #CCCCCC; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #4E4E52; }"
    );
=======
    btnCancel->setObjectName("PresetBtnCancel");
>>>>>>> REPLACE

---

### File 5: `src/ui/FileCollisionDialog.cpp`

<<<<<<< SEARCH
    textLabel->setStyleSheet("color: #EEEEEE; font-size: 13px;");
=======
    textLabel->setObjectName("CollisionTextLabel");
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_chkApplyToAll->setStyleSheet(
        "QCheckBox { color: #CCCCCC; font-size: 12px; }"
    );
=======
    m_chkApplyToAll->setObjectName("CollisionApplyAllChk");
>>>>>>> REPLACE

<<<<<<< SEARCH
        btn->setStyleSheet(
            "QPushButton { background-color: #2D2D30; color: #EEEEEE; border: 1px solid #3E3E42; border-radius: 4px; padding: 8px 12px; text-align: left; }"
            "QPushButton:hover { background-color: #3E3E42; border-color: #007ACC; }"
        );
=======
        btn->setObjectName("CollisionOptionBtn");
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### Build Command
```bash
cmake -B build -S .
cmake --build build --config Release
```

### Verification Methods
1. **Visual Verification**: Open `PresetTagsDialog`, `FileCollisionDialog`, `FilterPanel`, and main window menus. Confirm background colors, border colors, radii, paddings, and font sizes match 1:1 without any visual difference.
2. **Code Verification**: Confirm zero inline `setStyleSheet(...)` calls remain in the modified dialogs and components.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Reused SSOT Entry Points**: `resources/style.qss` as the Single Source of Truth for all UI styles.
- **Zero-Value-Alteration**: Every single property value (`#378ADD`, `#4A90E2`, `#1E1E1E`, `#3C3C3C`, `#2D2D30`, 4px radius, 11px/12px/13px font sizes) has been zero-value-copied into `resources/style.qss`.

---

## 6. Header API Signature Verification

| Class / Component | Function / Method Signature | Header File Path | Status |
| :--- | :--- | :--- | :--- |
| `QWidget` | `void setObjectName(const QString& name)` | `<QWidget>` | Qt Core Standard API |

---

## 7. Header Inclusion Chain & Type Completeness Check

No header inclusion chain breaks. All modified files continue using standard Qt widget headers.
