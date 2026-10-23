# SearchController Implementation Plan - Reactive Clear Button Fix

This implementation plan details the exact changes needed in `SearchController` and `resources/style.qss` to guarantee that the search edit clear button (×) is strictly hidden when the input text is empty, preventing persistent icon visibility artifacts in empty search states.

---

## 1. Overview & Problem Statement
* **Issue**: The global search line edit (`QLineEdit#SearchEdit`) displays a persistent clear button icon (×) even when the input field is empty (with placeholder text "搜索...").
* **Root Cause**: `m_searchEdit->setClearButtonEnabled(true)` relies on Qt's internal `QToolButton` visibility heuristics, which can be overridden or rendered visible when styling or focus state changes occur in custom dark QSS themes.
* **Solution**: Replace `setClearButtonEnabled(true)` with a explicit `QAction` added at `QLineEdit::TrailingPosition`. Connect `QLineEdit::textChanged` to dynamically toggle action visibility (`setVisible(!text.isEmpty())`), enforcing the "Reactive Clear Button Contract".

---

## 2. Modified Files List
- `src/ui/SearchController.cpp`
- `resources/style.qss`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/SearchController.cpp`

```git
<<<<<<< SEARCH
    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setFixedSize(230, 32);
    m_searchEdit->addAction(UiHelper::getIcon("search", TextMuted), QLineEdit::LeadingPosition);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setObjectName("SearchEdit");
=======
    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setFixedSize(230, 32);
    m_searchEdit->addAction(UiHelper::getIcon("search", TextMuted), QLineEdit::LeadingPosition);
    
    QAction* clearAction = m_searchEdit->addAction(UiHelper::getIcon("close", TextMuted), QLineEdit::TrailingPosition);
    clearAction->setVisible(false);
    connect(clearAction, &QAction::triggered, m_searchEdit, &QLineEdit::clear);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [clearAction](const QString& text) {
        clearAction->setVisible(!text.isEmpty());
    });

    m_searchEdit->setObjectName("SearchEdit");
>>>>>>> REPLACE
```

---

### 3.2 `resources/style.qss`

```git
<<<<<<< SEARCH
QLineEdit#SearchEdit {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 6px;
    color: #EEEEEE;
    padding-left: 4px;
    padding-right: 24px;
    font-size: 12px;
}
=======
QLineEdit#SearchEdit {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 6px;
    color: #EEEEEE;
    padding-left: 4px;
    padding-right: 24px;
    font-size: 12px;
}
QLineEdit#SearchEdit QToolButton {
    border: none;
    background: transparent;
}
QLineEdit#SearchEdit QToolButton:hover {
    background-color: #3E3E42;
    border-radius: 3px;
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Verify `SearchController.cpp` uses the explicit `QAction` with `clearAction->setVisible(!text.isEmpty())`.
2. Confirm `SearchController-1.md` exists in `QuarkMeta Architecture/Implementation Plan/`.
3. Confirm `QuarkMeta-Architecture-Planning.md` contains the Reactive Clear Button Contract in Chapter 1.
