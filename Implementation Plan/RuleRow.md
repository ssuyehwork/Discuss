# RuleRow & CreateRuleRow Controls Styling Implementation Plan

## 1. Overview
During the QSS refactoring, inline stylesheet styles in `RuleRow.cpp` and `CreateRuleRow.cpp` were moved to `resources/style.qss`. However, critical CSS attributes (`border`, `color`, `font-size`, `font-weight`) for `QPushButton#RuleDeleteBtn` were accidentally stripped and replaced with `border: none;`. This caused text clipping and border collapse on the add/remove buttons (`+` and `-`) in the Batch Create and Batch Rename dialogs, resulting in distorted rendering (e.g. `+` rendering as `├`).

This implementation plan restores the exact compact control styling in `resources/style.qss` without modifying any C++ public API signatures or structural code.

## 2. Modified Files List
- `resources/style.qss`

## 3. Detailed Line-by-Line Changes

### `resources/style.qss`

```diff
<<<<<<< SEARCH
QComboBox#RuleCombo {
    background: #1E1E1E;
    border: 1px solid #444;
    border-radius: 4px;
    padding: 2px 5px;
    color: #EEE;
}
QComboBox#RuleCombo::drop-down {
    border: none;
}
QLineEdit#RuleTextEdit, QSpinBox#RuleStartSpin {
    background: #1E1E1E;
    border: 1px solid #444;
    border-radius: 4px;
    padding: 2px 5px;
    color: #EEE;
}
QPushButton#RuleDeleteBtn {
    background: transparent;
    border: none;
    border-radius: 3px;
}
QPushButton#RuleDeleteBtn:hover {
    background-color: #3E3E42;
}
=======
QComboBox#RuleCombo {
    background: #1E1E1E;
    border: 1px solid #444;
    border-radius: 4px;
    padding: 2px 5px;
    color: #EEE;
}
QComboBox#RuleCombo::drop-down {
    border: none;
    width: 20px;
}
QComboBox#RuleCombo QAbstractItemView {
    background-color: #2D2D2D;
    border: 1px solid #444;
    selection-background-color: #3E3E42;
    selection-color: white;
    color: #EEE;
    outline: 0;
}
QComboBox#RuleCombo QAbstractItemView::item {
    height: 22px;
    padding: 2px;
}
QLineEdit#RuleTextEdit, QSpinBox#RuleStartSpin {
    background: #1E1E1E;
    border: 1px solid #444;
    border-radius: 4px;
    padding: 2px 5px;
    color: #EEE;
}
QPushButton#RuleDeleteBtn {
    background: transparent;
    border: 1px solid #434343;
    border-radius: 2px;
    color: #888;
    font-weight: bold;
    font-size: 14px;
    padding: 0px;
    text-align: center;
}
QPushButton#RuleDeleteBtn:hover {
    background-color: #3E3E42;
    color: #EEE;
    border-color: #666;
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build the application using CMake / MSVC build pipeline.
2. Open the Batch Create (`批量创建`) dialog and Batch Rename (`批量重命名`) dialog.
3. Verify that the add (`+`) and remove (`-`) buttons render with a clean 1px border (`#434343`), crisp 14px centered text without clipping, and turn light grey with `#3E3E42` background on hover.
4. Verify that rule type dropdowns (`QComboBox#RuleCombo`) render dark-themed item popup menus with proper selection highlights.
