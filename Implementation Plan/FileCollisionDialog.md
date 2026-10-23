# Implementation Plan: FileCollisionDialog Refactoring (File Collision Resolution System)

## 1. Overview
This implementation plan refactors the file collision handling during clipboard paste and move operations to align with standard Windows File Explorer UX specifications:
- **Standard Action Terminology**: Adobe Bridge style dialog with "是否应用于全部文件？" checkbox and four capsule buttons: "自动解析" (AutoResolve), "替换" (Replace), "跳过" (Skip), and "取消" (Cancel).
- **Batch Conflict Execution**: Supports "为所有冲突执行此操作" (Apply to all conflicts) state persistence across multi-file operations.
- **Clean Architecture & Frameless Dialog Standard**: Standardizes `FileCollisionDialog` as a custom subclass of `FramelessDialog` adhering to QuarkMeta dark mode styling (`#252526` background, `#4E4E52` border) with command-link button blocks and path info header stacks.

## 2. Modified Files List
- `src/ui/FileCollisionDialog.h` (New Header)
- `src/ui/FileCollisionDialog.cpp` (New Implementation)
- `src/core/ClipboardService.h`
- `src/core/ClipboardService.cpp`
- `CMakeLists.txt`

## 3. Detailed Line-by-Line Changes

### 3.1 Register `FileCollisionDialog` in `CMakeLists.txt`

<<<<<<< SEARCH
    src/ui/DuplicateConflictDialog.cpp
=======
    src/ui/DuplicateConflictDialog.cpp
    src/ui/FileCollisionDialog.cpp
>>>>>>> REPLACE

<<<<<<< SEARCH
    src/ui/DuplicateConflictDialog.h
=======
    src/ui/DuplicateConflictDialog.h
    src/ui/FileCollisionDialog.h
>>>>>>> REPLACE

### 3.2 Add `FileCollisionDialog` Class Definition (`src/ui/FileCollisionDialog.h`)

```cpp
#ifndef FILECOLLISIONDIALOG_H
#define FILECOLLISIONDIALOG_H

#include "FramelessDialog.h"
#include <QCheckBox>
#include <QPushButton>

namespace QuarkMeta {

enum class CollisionResolveAction {
    Replace,
    Skip,
    KeepBoth,
    Cancel
};

class FileCollisionDialog : public FramelessDialog {
    Q_OBJECT

public:
    explicit FileCollisionDialog(const QString& sourceDir,
                                const QString& targetDir,
                                int conflictCount,
                                QWidget* parent = nullptr);

    CollisionResolveAction selectedAction() const;
    bool applyToAll() const;

private:
    void setupUi(const QString& sourceDir, const QString& targetDir, int conflictCount);

    CollisionResolveAction m_selectedAction = CollisionResolveAction::Cancel;
    QCheckBox* m_chkApplyToAll = nullptr;
};

} // namespace QuarkMeta

#endif // FILECOLLISIONDIALOG_H
```

### 3.3 Implement `FileCollisionDialog` (`src/ui/FileCollisionDialog.cpp`)

```cpp
#include "FileCollisionDialog.h"
#include "UiHelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace QuarkMeta {

FileCollisionDialog::FileCollisionDialog(const QString& sourceDir,
                                           const QString& targetDir,
                                           int conflictCount,
                                           QWidget* parent)
    : FramelessDialog(parent) {
    setWindowTitle("替换或跳过文件");
    setFixedSize(460, 320);
    setupUi(sourceDir, targetDir, conflictCount);
}

CollisionResolveAction FileCollisionDialog::selectedAction() const {
    return m_selectedAction;
}

bool FileCollisionDialog::applyToAll() const {
    return m_chkApplyToAll ? m_chkApplyToAll->isChecked() : false;
}

void FileCollisionDialog::setupUi(const QString& sourceDir, const QString& targetDir, int conflictCount) {
    QWidget* area = getContentArea();
    QVBoxLayout* layout = new QVBoxLayout(area);
    layout->setContentsMargins(24, 10, 24, 20);
    layout->setSpacing(14);

    QLabel* lblHeader = new QLabel(QString("正在将 %1 个项目从 %2 粘贴到 %3")
                                      .arg(conflictCount)
                                      .arg(sourceDir)
                                      .arg(targetDir), area);
    lblHeader->setWordWrap(true);
    lblHeader->setStyleSheet("color: #CCCCCC; font-size: 13px;");
    layout->addWidget(lblHeader);

    QLabel* lblSub = new QLabel(QString("目标包含 %1 个同名文件").arg(conflictCount), area);
    lblSub->setStyleSheet("color: #FFFFFF; font-size: 16px; font-weight: bold;");
    layout->addWidget(lblSub);

    auto createOptionBtn = [this, area](const QString& iconName, const QString& text, CollisionResolveAction action) {
        QPushButton* btn = new QPushButton(area);
        btn->setIcon(UiHelper::getIcon(iconName, QColor("#EEEEEE"), 18));
        btn->setText("  " + text);
        btn->setFixedHeight(42);
        btn->setStyleSheet(
            "QPushButton {"
            "  background-color: #2D2D30;"
            "  color: #FFFFFF;"
            "  border: 1px solid #3E3E42;"
            "  border-radius: 4px;"
            "  text-align: left;"
            "  padding-left: 16px;"
            "  font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #3E3E42;"
            "  border-color: #378ADD;"
            "}"
        );
        connect(btn, &QPushButton::clicked, this, [this, action]() {
            m_selectedAction = action;
            accept();
        });
        return btn;
    };

    layout->addWidget(createOptionBtn("copy", "自动解析（同时共存）", CollisionResolveAction::AutoResolve));
    layout->addWidget(createOptionBtn("paste", "替代", CollisionResolveAction::Replace));
    layout->addWidget(createOptionBtn("close", "取消", CollisionResolveAction::Cancel));

    m_chkApplyToAll = new QCheckBox("为所有冲突执行此操作", contentWidget());
    m_chkApplyToAll->setStyleSheet("QCheckBox { color: #CCCCCC; font-size: 13px; }");
    layout->addWidget(m_chkApplyToAll);
}

} // namespace QuarkMeta
```

## 4. Build & Verification Steps
1. Verify `FileCollisionDialog.h` and `FileCollisionDialog.cpp` match standard C++/Qt 6.x signatures.
2. Ensure CMake build config includes the new source files.
3. Test paste conflict prompt during duplicate file operations in QuarkMeta.
