# ColumnItemDelegate In-Place Renaming Unification Implementation Plan

This implementation plan details the precise changes required to harmonize the in-place renaming editor experience in `ColumnItemDelegate` with QuarkMeta's application-wide standards (`QuarkMeta-Architecture-Planning.md`).

## Overview
Currently, `ColumnItemDelegate` relies on Qt's default `QStyledItemDelegate::createEditor` (which creates a standard `QLineEdit`). This causes three major issues:
1. **Lack of Extension Protection**: When renaming a file in Column View, the entire filename including extension is selected, leading to accidental deletion of file extensions.
2. **Native Right-Click Menu Violation**: Default `QLineEdit` presents Windows/Qt default context menus instead of QuarkMeta's dark-themed exclusive context menu.
3. **Inconsistent Navigation Keys**: Up/Down and Left/Right key behaviors during in-place editing do not match the smart cursor positioning and navigation guards present in Grid/Tree views.

This plan integrates `FileNameLineEdit` into `ColumnItemDelegate` and handles precise geometry and model data synchronization.

---

## Modified Files List
- `src/ui/ColumnItemDelegate.h`

---

## Detailed Line-by-Line Changes

### File: `src/ui/ColumnItemDelegate.h`

```git
<<<<<<< SEARCH
#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include "UiHelper.h"
#include "../core/ModelContract.h"

namespace QuarkMeta {

class ColumnItemDelegate : public QStyledItemDelegate {
public:
    explicit ColumnItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}
=======
#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QLineEdit>
#include <QKeyEvent>
#include "UiHelper.h"
#include "ThumbnailDelegate.h"
#include "../core/ModelContract.h"

namespace QuarkMeta {

class ColumnItemDelegate : public QStyledItemDelegate {
public:
    explicit ColumnItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(option);
        FileNameLineEdit* editor = new FileNameLineEdit(parent);
        editor->setObjectName("ColumnItemEditor");
        bool isFolder = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool();
        editor->setIsFolder(isFolder);
        editor->installEventFilter(const_cast<ColumnItemDelegate*>(this));
        return editor;
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(index);
        QRect r = option.rect;
        r.adjust(32, 1, -22, -1);
        editor->setGeometry(r);
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        QString value = index.model()->data(index, Qt::EditRole).toString();
        FileNameLineEdit* lineEdit = qobject_cast<FileNameLineEdit*>(editor);
        if (lineEdit) {
            lineEdit->setText(value);
        }
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
        if (!lineEdit) return;
        QString newName = lineEdit->text().trimmed();
        if (!newName.isEmpty()) {
            model->setData(index, newName, Qt::EditRole);
        }
    }

    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            QLineEdit* editor = qobject_cast<QLineEdit*>(obj);
            if (editor) {
                int key = keyEvent->key();
                if (key == Qt::Key_Up || key == Qt::Key_Down) {
                    keyEvent->accept();
                    return true;
                }
                if (key == Qt::Key_Left || key == Qt::Key_Right) {
                    if (editor->hasSelectedText()) {
                        if (key == Qt::Key_Left) {
                            editor->setCursorPosition(0);
                        } else {
                            QString val = editor->text();
                            int lastDot = val.lastIndexOf('.');
                            if (lastDot > 0) {
                                editor->setCursorPosition(lastDot);
                            } else {
                                editor->setCursorPosition(val.length());
                            }
                        }
                        editor->deselect();
                        keyEvent->accept();
                        return true;
                    }
                    return false;
                }
            }
        }
        return QStyledItemDelegate::eventFilter(obj, event);
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **CMake Build Verification**:
   ```bash
   cmake --build build --config Debug
   ```
2. **Functional Verification**:
   - Launch QuarkMeta application and switch to Column View mode (Miller Columns).
   - Select a file in any active column and press `F2` (or click "Rename" from right-click context menu).
   - Verify that:
     - Only the base filename is selected (extension is protected and unselected).
     - Up/Down direction keys do not cause view selection drift while editing.
     - Pressing Left arrow moves cursor to beginning of filename; pressing Right arrow moves cursor directly before the extension dot.
     - Right-clicking inside the active in-place edit box invokes QuarkMeta's exclusive dark context menu.
