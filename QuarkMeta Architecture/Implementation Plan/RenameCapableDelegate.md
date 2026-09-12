# Implementation Plan: RenameCapableDelegate & Delegate Normalization

## 1. Overview
This implementation plan unifies the line-editing and rename functionality across all item views (Tree, Grid/Thumbnail, Column) by extracting a standalone `FileNameLineEdit` control and introducing an abstract base class `RenameCapableDelegate`.
By sealing `createEditor`, `setEditorData`, and `setModelData` with `override final` in `RenameCapableDelegate`, all view delegates automatically gain identical, robust rename capabilities without duplicating edit cycle or key filtering code.

## 2. Modified Files List
- `CMakeLists.txt`
- `src/ui/FileNameLineEdit.h` (New File)
- `src/ui/FileNameLineEdit.cpp` (New File)
- `src/ui/RenameCapableDelegate.h` (New File)
- `src/ui/RenameCapableDelegate.cpp` (New File)
- `src/ui/TreeItemDelegate.h`
- `src/ui/ThumbnailDelegate.h`
- `src/ui/ThumbnailDelegate.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt` (Register New Files for MOC)
<<<<<<< SEARCH
    src/ui/TreeItemDelegate.h
    src/ui/ThumbnailDelegate.h
=======
    src/ui/TreeItemDelegate.h
    src/ui/ThumbnailDelegate.h
    src/ui/FileNameLineEdit.h
    src/ui/RenameCapableDelegate.h
>>>>>>> REPLACE

<<<<<<< SEARCH
    src/ui/ThumbnailDelegate.cpp
    src/ui/TitleBarWidget.cpp
=======
    src/ui/ThumbnailDelegate.cpp
    src/ui/FileNameLineEdit.cpp
    src/ui/RenameCapableDelegate.cpp
    src/ui/TitleBarWidget.cpp
>>>>>>> REPLACE

### 3.2 `src/ui/FileNameLineEdit.h` (New File)
<<<<<<< SEARCH
=======
#pragma once

#include <QLineEdit>
#include <QFocusEvent>
#include <QKeyEvent>

namespace QuarkMeta {

/**
 * @brief Standalone LineEdit with autonomous selection and key navigation for file renaming.
 */
class FileNameLineEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit FileNameLineEdit(QWidget* parent = nullptr);
    void setIsFolder(bool isFolder);

protected:
    void focusInEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    bool m_isFolder = false;
};

} // namespace QuarkMeta
>>>>>>> REPLACE

### 3.3 `src/ui/FileNameLineEdit.cpp` (New File)
<<<<<<< SEARCH
=======
#include "FileNameLineEdit.h"

namespace QuarkMeta {

FileNameLineEdit::FileNameLineEdit(QWidget* parent) : QLineEdit(parent) {}

void FileNameLineEdit::setIsFolder(bool isFolder) {
    m_isFolder = isFolder;
}

void FileNameLineEdit::focusInEvent(QFocusEvent* event) {
    QLineEdit::focusInEvent(event);
    if (m_isFolder) {
        selectAll();
    } else {
        int lastDot = text().lastIndexOf('.');
        if (lastDot > 0) {
            setSelection(0, lastDot);
        } else {
            selectAll();
        }
    }
}

void FileNameLineEdit::keyPressEvent(QKeyEvent* event) {
    int key = event->key();
    if (key == Qt::Key_Up || key == Qt::Key_Down) {
        event->accept();
        return; // Consume Up/Down to prevent item view row shifting
    }
    if (key == Qt::Key_Left || key == Qt::Key_Right) {
        if (hasSelectedText()) {
            if (key == Qt::Key_Left) {
                setCursorPosition(0);
            } else {
                int lastDot = text().lastIndexOf('.');
                if (lastDot > 0) {
                    setCursorPosition(lastDot);
                } else {
                    setCursorPosition(text().length());
                }
            }
            deselect();
            event->accept();
            return;
        }
    }
    QLineEdit::keyPressEvent(event);
}

} // namespace QuarkMeta
>>>>>>> REPLACE

### 3.4 `src/ui/RenameCapableDelegate.h` (New File)
<<<<<<< SEARCH
=======
#pragma once

#include <QStyledItemDelegate>
#include "FileNameLineEdit.h"

namespace QuarkMeta {

/**
 * @brief Base Delegate enforcing unified inline rename logic across all view delegates.
 */
class RenameCapableDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override final;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override final;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override final;
};

} // namespace QuarkMeta
>>>>>>> REPLACE

### 3.5 `src/ui/RenameCapableDelegate.cpp` (New File)
<<<<<<< SEARCH
=======
#include "RenameCapableDelegate.h"
#include "../core/ModelContract.h"

namespace QuarkMeta {

QWidget* RenameCapableDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const {
    FileNameLineEdit* editor = new FileNameLineEdit(parent);
    bool isFolder = (index.data(TypeRole).toString() == "folder");
    editor->setIsFolder(isFolder);
    return editor;
}

void RenameCapableDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    QString value = index.model()->data(index, Qt::EditRole).toString();
    FileNameLineEdit* lineEdit = qobject_cast<FileNameLineEdit*>(editor);
    if (lineEdit) {
        lineEdit->setText(value);
    }
}

void RenameCapableDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (!lineEdit) return;
    QString newName = lineEdit->text().trimmed();
    if (!newName.isEmpty()) {
        model->setData(index, newName, Qt::EditRole);
    }
}

} // namespace QuarkMeta
>>>>>>> REPLACE

### 3.6 Refactoring `src/ui/TreeItemDelegate.h`
<<<<<<< SEARCH
#include "ThumbnailDelegate.h"
=======
#include "RenameCapableDelegate.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
class TreeItemDelegate : public QStyledItemDelegate {
=======
class TreeItemDelegate : public RenameCapableDelegate {
>>>>>>> REPLACE

### 3.7 Refactoring `src/ui/ThumbnailDelegate.h`
<<<<<<< SEARCH
#include <QStyledItemDelegate>
#include <QLineEdit> 

namespace QuarkMeta {

class FileNameLineEdit : public QLineEdit { 
    Q_OBJECT 
public: 
    explicit FileNameLineEdit(QWidget* parent = nullptr) : QLineEdit(parent) {} 
    void setIsFolder(bool isFolder) { m_isFolder = isFolder; } 
 
protected: 
    void focusInEvent(QFocusEvent* event) override { 
        QLineEdit::focusInEvent(event); // 先执行基类 Focus 事件 
        if (m_isFolder) { 
            selectAll(); 
        } else { 
            int lastDot = text().lastIndexOf('.'); 
            if (lastDot > 0) { 
                setSelection(0, lastDot); 
            } else { 
                selectAll(); 
            } 
        } 
    } 
 
private: 
    bool m_isFolder = false; 
}; 

class ThumbnailDelegate : public QStyledItemDelegate {
=======
#include "RenameCapableDelegate.h"

namespace QuarkMeta {

class ThumbnailDelegate : public RenameCapableDelegate {
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. Re-configure CMake build system:
   ```bash
   cmake -B build
   ```
2. Build the project:
   ```bash
   cmake --build build --config Debug
   ```
3. Run tests to confirm zero regressions in item view renaming and key navigation.
