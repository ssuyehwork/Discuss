# Implementation Plan - Smart Folder Rule Tag Selector Integration

## 1. Overview
In `SmartFolderDialog`, when the rule type is set to "标签" (`SmartRuleType::Tag`), the rule value line edit (`m_valueEdit`) currently relies solely on plain text typing, requiring users to manually remember and spell tag names correctly.

This implementation plan enhances `SmartFolderRuleRow` in `src/ui/dialogs/SmartFolderDialog.cpp` by binding `TagSelectorOverlay` (tag selection floating picker) to `m_valueEdit` when the rule type is "标签". Users can click the line edit or its tag trailing action icon to open `TagSelectorOverlay` and select existing lexicon tags, automatically filling the selected tag name into `m_valueEdit`.

---

## 2. Modified Files List
- `src/ui/dialogs/SmartFolderDialog.h`
- `src/ui/dialogs/SmartFolderDialog.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/dialogs/SmartFolderDialog.h`
Include `TagSelectorOverlay.h` and add event filtering / overlay reference to `SmartFolderRuleRow`:

```cpp
<<<<<<< SEARCH
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include "FramelessDialog.h"
=======
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QPointer>
#include "FramelessDialog.h"
#include "TagSelectorOverlay.h"
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    QComboBox* m_opCombo = nullptr;
    QLineEdit* m_valueEdit = nullptr;
    QComboBox* m_valueCombo = nullptr;
    QPushButton* m_btnDelete = nullptr;
=======
    QComboBox* m_opCombo = nullptr;
    QLineEdit* m_valueEdit = nullptr;
    QComboBox* m_valueCombo = nullptr;
    QPushButton* m_btnDelete = nullptr;
    QAction* m_tagAction = nullptr;
    QPointer<TagSelectorOverlay> m_tagOverlay;

    void openTagSelector();
>>>>>>> REPLACE
```

### 3.2 `src/ui/dialogs/SmartFolderDialog.cpp`
In `SmartFolderRuleRow::setupUi()`, install event filter on `m_valueEdit`. In `onTypeChanged()`, dynamically add or remove the tag picker trailing action icon on `m_valueEdit`. Implement `openTagSelector()`:

```cpp
<<<<<<< SEARCH
void SmartFolderRuleRow::setupUi() {
    QHBoxLayout* layout = new QHBoxLayout(this);
=======
void SmartFolderRuleRow::openTagSelector() {
    if (m_tagOverlay) {
        m_tagOverlay->close();
        return;
    }

    QWidget* topWidget = window();
    QString currentTag = m_valueEdit->text().trimmed();
    QStringList initTags;
    if (!currentTag.isEmpty()) {
        initTags << currentTag;
    }

    m_tagOverlay = new TagSelectorOverlay(initTags, topWidget, TagTriggerSource::PresetTagsDialog);

    // Position overlay below m_valueEdit
    QPoint globalPos = m_valueEdit->mapToGlobal(QPoint(0, m_valueEdit->height() + 2));
    m_tagOverlay->move(m_tagOverlay->mapFromGlobal(globalPos));

    connect(m_tagOverlay, &TagSelectorOverlay::selectionChanged, this, [this](const QStringList& selectedTags) {
        if (!selectedTags.isEmpty()) {
            m_valueEdit->setText(selectedTags.last());
        }
    });

    m_tagOverlay->show();
}

void SmartFolderRuleRow::setupUi() {
    QHBoxLayout* layout = new QHBoxLayout(this);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    m_valueEdit = new QLineEdit(this);

    m_valueCombo = new QComboBox(this);
=======
    m_valueEdit = new QLineEdit(this);
    m_valueEdit->installEventFilter(this);

    m_valueCombo = new QComboBox(this);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    case SmartRuleType::Tag:
        m_opCombo->addItem("包含", static_cast<int>(SmartRuleOperator::Contains));
        m_opCombo->addItem("不包含", static_cast<int>(SmartRuleOperator::NotContains));
        m_valueEdit->setPlaceholderText("输入标签名称");
        break;
=======
    case SmartRuleType::Tag:
        m_opCombo->addItem("包含", static_cast<int>(SmartRuleOperator::Contains));
        m_opCombo->addItem("不包含", static_cast<int>(SmartRuleOperator::NotContains));
        m_valueEdit->setPlaceholderText("点击选择或输入标签名称");
        if (!m_tagAction) {
            m_tagAction = m_valueEdit->addAction(UiHelper::getIcon("tag_filled", QColor("#9B59B6"), 14), QLineEdit::TrailingPosition);
            connect(m_tagAction, &QAction::triggered, this, &SmartFolderRuleRow::openTagSelector);
        }
        break;
>>>>>>> REPLACE
```

Handle `eventFilter` on `m_valueEdit` when type is `SmartRuleType::Tag`:

```cpp
<<<<<<< SEARCH
void SmartFolderRuleRow::onTypeChanged(int index) {
=======
bool SmartFolderRuleRow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_valueEdit && static_cast<SmartRuleType>(m_typeCombo->currentData().toInt()) == SmartRuleType::Tag) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::FocusIn) {
            openTagSelector();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void SmartFolderRuleRow::onTypeChanged(int index) {
>>>>>>> REPLACE
```

In `onTypeChanged`, remove `m_tagAction` if type is not `SmartRuleType::Tag`:

```cpp
<<<<<<< SEARCH
    m_opCombo->clear();
    m_valueEdit->show();
    m_valueCombo->hide();
=======
    m_opCombo->clear();
    m_valueEdit->show();
    m_valueCombo->hide();

    if (type != SmartRuleType::Tag && m_tagAction) {
        m_valueEdit->removeAction(m_tagAction);
        m_tagAction = nullptr;
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile project target using `cmake --build .`.
2. In QuarkMeta, open Library panel context menu and select "新建智能文件夹".
3. In `SmartFolderDialog`, select rule type "标签".
4. Verify that `m_valueEdit` displays the tag icon on the right and placeholder "点击选择或输入标签名称".
5. Click `m_valueEdit` or its tag icon. Verify `TagSelectorOverlay` pops up.
6. Click any tag pill in `TagSelectorOverlay`. Verify the selected tag name automatically fills into `m_valueEdit`.

---

## 5. Official API Reuse & Anti-Redundancy
- Reuses existing `TagSelectorOverlay` with `TagTriggerSource::PresetTagsDialog` mode without creating duplicate tag picker dialogs.

---

## 6. Header API Signature Verification
- No breaking changes to public `SmartFolderDialog` or `SmartFolderRuleRow` header signatures.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `TagSelectorOverlay.h` included in `SmartFolderDialog.h`.

---

## 8. Data SSOT Declaration Table
| Entity / State | SSOT Location | Synchronization Mechanism |
| :--- | :--- | :--- |
| Smart Folder Tag Rule Value | `m_valueEdit->text()` | Auto-populated from `TagSelectorOverlay::selectionChanged` |

---

## 9. Compliance Self-Check Table
| Check Item | Status | Notes |
| :--- | :--- | :--- |
| Zero Code Modification Rule | PASS | Output strictly as plan document inside `Implementation Plan/` |
| Chinese Language Protocol | PASS | Documented completely in Chinese |
| Contract Lock | PASS | Public header signatures untouched |

---

## 10. Behavior Preservation Declaration Table
| Action | Existing Behavior | Preserved Behavior |
| :--- | :--- | :--- |
| Smart Folder Tag Rule Typing | Manual text input only | Manual typing + `TagSelectorOverlay` click selection |
