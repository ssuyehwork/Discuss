# Implementation Plan - MetaPanel UI Fine Details Fix (`metapanel-2.md`)

## 1. Overview
This implementation plan restores two exact UI details highlighted in user feedback:
1. **Path Text Field Cursor Position**: Call `m_pathEdit->setCursorPosition(0)` in `MetaPanel::updateInfo` so that the beginning of long path strings (e.g. drive letter `H:\...`) is displayed by default instead of scrolling to the tail end.
2. **Link Edit Button Vertical Separator**: Apply `border-left: 1px solid #3c3c3c` styling to the action tool button inside `m_linkEdit` to restore the distinct vertical divider line on the left side of the link icon button.

---

## 2. Modified Files List
- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/MetaPanel.cpp`
```
<<<<<<< SEARCH
    for (QToolButton* btn : m_linkEdit->findChildren<QToolButton*>()) {
        btn->setCursor(Qt::PointingHandCursor);
    }
=======
    for (QToolButton* btn : m_linkEdit->findChildren<QToolButton*>()) {
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QToolButton { border: none; border-left: 1px solid #3c3c3c; background: transparent; padding-left: 4px; padding-right: 4px; }"
            "QToolButton:hover { background: #3E3E42; }"
        );
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_pathEdit->setText(p);
=======
    m_pathEdit->setText(p);
    m_pathEdit->setCursorPosition(0);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Build application using CMake / Ninja.
2. Select any file in `ContentPanel` with a long path.
3. Observe `MetaPanel`'s physical path field: the path shows the start/drive letter (e.g., `H:\...`) rather than the end.
4. Observe the associated link field: the link icon button on the right displays a sharp 1px vertical divider line (`border-left`) separating it from the text box.
