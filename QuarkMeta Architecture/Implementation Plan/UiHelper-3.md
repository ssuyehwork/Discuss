# UiHelper - Menu Factory & LineEdit Clear Button Helper (UiHelper-3.md)

## 1. Overview
Centralizes `QMenu` instantiation and styling into `UiHelper::createMenu(QWidget* parent = nullptr)` to eliminate repeated `UiHelper::applyMenuStyle(&menu)` boilerplate across 16+ call sites. Also provides `UiHelper::setupLineEditClearButton(QLineEdit* edit)` to standardize clear button behavior ("visible when text is non-empty, invisible when empty") across search bars and input fields.

## 2. Modified Files List
- `src/ui/UiHelper.h`
- `src/ui/UiHelper.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/UiHelper.h`
Add `createMenu` and `setupLineEditClearButton` static declarations.

```git
<<<<<<< SEARCH
    static void setupLineEditContextMenu(QLineEdit* edit);
=======
    static void setupLineEditContextMenu(QLineEdit* edit);
    static void showLineEditContextMenu(QLineEdit* edit, const QPoint& pos);
    static QMenu* createMenu(QWidget* parent = nullptr);
    static QAction* setupLineEditClearButton(QLineEdit* edit);
>>>>>>> REPLACE
```

### `src/ui/UiHelper.cpp`
Implement `createMenu` and `setupLineEditClearButton`.

```git
<<<<<<< SEARCH
void UiHelper::setupLineEditContextMenu(QLineEdit* edit) {
=======
QMenu* UiHelper::createMenu(QWidget* parent) {
    QMenu* menu = new QMenu(parent);
    applyMenuStyle(menu);
    return menu;
}

QAction* UiHelper::setupLineEditClearButton(QLineEdit* edit) {
    if (!edit) return nullptr;
    QAction* clearAction = edit->addAction(getIcon("close", QColor("#888888")), QLineEdit::TrailingPosition);
    clearAction->setVisible(!edit->text().isEmpty());
    QObject::connect(clearAction, &QAction::triggered, edit, &QLineEdit::clear);
    QObject::connect(edit, &QLineEdit::textChanged, edit, [clearAction](const QString& text) {
        clearAction->setVisible(!text.isEmpty());
    });
    return clearAction;
}

void UiHelper::setupLineEditContextMenu(QLineEdit* edit) {
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
```bash
cmake --build build
```
Verify that menus and clear buttons function smoothly with reduced code duplication.
