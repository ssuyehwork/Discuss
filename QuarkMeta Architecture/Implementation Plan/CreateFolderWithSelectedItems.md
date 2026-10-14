# Implementation Plan - Create Folder with Selected Items Context Menu Action

## 1. Overview
This implementation plan adds a new context menu option **"用所选项目新建文件夹(N)"** directly below **"添加至“库”"** in the item context menu (`ContentContextMenu`):
1. Creates a custom frameless dialog `CreateFolderWithItemsDialog` (`src/ui/dialogs/CreateFolderWithItemsDialog.h` / `.cpp`) matching the UI specification in Image 2 (blue circular `+` header icon, "添加文件夹" main title, "输入新的文件夹名称" subtitle, and "新的文件夹名称" primary blue action button).
2. Adds `ActionCreateFolderWithSelectedItems` enum value to `ContentPanel::ContextAction` in `src/ui/ContentPanel.h`.
3. Integrates the **"用所选项目新建文件夹(N)"** menu action into `src/ui/controllers/ContentContextMenu.cpp` placed directly below `LibraryService::instance().buildLibraryAction(...)`.
4. Upon confirmation, creates the specified folder and asynchronously moves (`DiskIoService`) all selected items into the new folder, updating metadata and triggering view refresh (`refreshAll()`).
5. Registers the new header and source files in `CMakeLists.txt`.

---

## 2. Modified Files List
- `CMakeLists.txt`
- `src/ui/ContentPanel.h`
- `src/ui/controllers/ContentContextMenu.cpp`
- `src/ui/dialogs/CreateFolderWithItemsDialog.h` (New File)
- `src/ui/dialogs/CreateFolderWithItemsDialog.cpp` (New File)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
Register `CreateFolderWithItemsDialog.h` and `CreateFolderWithItemsDialog.cpp` in `SOURCES` and `HEADERS`:

```cmake
<<<<<<< SEARCH
    src/ui/dialogs/CacheLocationDialog.h
    src/ui/dialogs/CacheLocationDialog.cpp
=======
    src/ui/dialogs/CacheLocationDialog.h
    src/ui/dialogs/CacheLocationDialog.cpp
    src/ui/dialogs/CreateFolderWithItemsDialog.h
    src/ui/dialogs/CreateFolderWithItemsDialog.cpp
>>>>>>> REPLACE
```

### 3.2 `src/ui/ContentPanel.h`
Add `ActionCreateFolderWithSelectedItems` value to `ContextAction` enum:

```cpp
<<<<<<< SEARCH
        ActionOpen, ActionOpenInNewTab, ActionOpenDefault, ActionShowInExplorer, ActionShowInQuarkMeta, ActionNewFolder, ActionNewMd, ActionNewTxt,
=======
        ActionOpen, ActionOpenInNewTab, ActionOpenDefault, ActionShowInExplorer, ActionShowInQuarkMeta, ActionNewFolder, ActionNewMd, ActionNewTxt, ActionCreateFolderWithSelectedItems,
>>>>>>> REPLACE
```

### 3.3 `src/ui/controllers/ContentContextMenu.cpp`
Include `CreateFolderWithItemsDialog.h`, add the menu item below `LibraryService::buildLibraryAction`, and handle the action execution:

```cpp
<<<<<<< SEARCH
#include "../dialogs/FramelessInputDialog.h"
=======
#include "../dialogs/FramelessInputDialog.h"
#include "../dialogs/CreateFolderWithItemsDialog.h"
>>>>>>> REPLACE
```

Add menu action right under `LibraryService::instance().buildLibraryAction`:

```cpp
<<<<<<< SEARCH
            QStringList selPathsItem = m_panel->getSelectedPaths();
            if (selPathsItem.isEmpty() && !path.isEmpty()) selPathsItem << path;
            LibraryService::instance().buildLibraryAction(&menu, selPathsItem, m_panel);

            menu.addSeparator();
=======
            QStringList selPathsItem = m_panel->getSelectedPaths();
            if (selPathsItem.isEmpty() && !path.isEmpty()) selPathsItem << path;
            LibraryService::instance().buildLibraryAction(&menu, selPathsItem, m_panel);

            QAction* actCreateFolderWithItems = menu.addAction(UiHelper::getIcon("folder", QColor("#EEEEEE"), 18), "用所选项目新建文件夹(N)");
            actCreateFolderWithItems->setData(ContentPanel::ActionCreateFolderWithSelectedItems);

            menu.addSeparator();
>>>>>>> REPLACE
```

Handle `ActionCreateFolderWithSelectedItems` in the action dispatch switch statement:

```cpp
<<<<<<< SEARCH
        case ContentPanel::ActionNewFolder:
            m_panel->createNewItem("folder");
            break;
=======
        case ContentPanel::ActionNewFolder:
            m_panel->createNewItem("folder");
            break;
        case ContentPanel::ActionCreateFolderWithSelectedItems: {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (selectedPaths.isEmpty() && !path.isEmpty()) {
                selectedPaths << path;
            }
            if (!selectedPaths.isEmpty()) {
                CreateFolderWithItemsDialog dlg(currentPath, selectedPaths, m_panel);
                if (dlg.exec() == QDialog::Accepted) {
                    m_panel->refreshAll();
                }
            }
            break;
        }
>>>>>>> REPLACE
```

### 3.4 New Header File `src/ui/dialogs/CreateFolderWithItemsDialog.h`
Create header for the custom frameless dialog:

```cpp
#pragma once

#include "FramelessDialog.h"
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>

namespace QuarkMeta {

class CreateFolderWithItemsDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit CreateFolderWithItemsDialog(const QString& currentDir, const QStringList& selectedPaths, QWidget* parent = nullptr);
    ~CreateFolderWithItemsDialog() override = default;

private slots:
    void onCreateAndMigrate();

private:
    void setupUi();

    QString m_currentDir;
    QStringList m_selectedPaths;
    QLineEdit* m_nameEdit{nullptr};
    QPushButton* m_btnCancel{nullptr};
    QPushButton* m_btnCreate{nullptr};
};

} // namespace QuarkMeta
```

### 3.5 New Source File `src/ui/dialogs/CreateFolderWithItemsDialog.cpp`
Implement custom dialog UI layout (matching Image 2) and file migration logic:

```cpp
#include "dialogs/CreateFolderWithItemsDialog.h"
#include "UiHelper.h"
#include "../ToolTipOverlay.h"
#include "../SvgIconRenderer.h"
#include "../../util/DiskIoService.h"
#include "../../core/LastOperationManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QPointer>

namespace QuarkMeta {

CreateFolderWithItemsDialog::CreateFolderWithItemsDialog(const QString& currentDir, const QStringList& selectedPaths, QWidget* parent)
    : FramelessDialog(parent), m_currentDir(currentDir), m_selectedPaths(selectedPaths) {
    setWindowTitle("添加文件夹");
    resize(460, 220);
    setMinimumSize(420, 200);
    setupUi();
}

void CreateFolderWithItemsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(contentArea());
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // 1. 顶部 Header 区域：左侧蓝色加号图标，右侧主副标题
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(14);

    QLabel* iconLabel = new QLabel(this);
    iconLabel->setFixedSize(48, 48);

    // 渲染蓝色圆形背景加号图标
    QPixmap circlePixmap(48, 48);
    circlePixmap.fill(Qt::transparent);
    {
        QPainter p(&circlePixmap);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor("#3498db"));
        p.setPen(Qt::NoPen);
        p.drawEllipse(0, 0, 48, 48);

        QIcon addIcon = UiHelper::getIcon("add", Qt::white, 24);
        addIcon.paint(&p, QRect(12, 12, 24, 24));
    }
    iconLabel->setPixmap(circlePixmap);
    headerLayout->addWidget(iconLabel);

    auto* titleTextLayout = new QVBoxLayout();
    titleTextLayout->setSpacing(4);

    QLabel* titleLabel = new QLabel("添加文件夹", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #EEEEEE;");

    QLabel* subtitleLabel = new QLabel("输入新的文件夹名称", this);
    QFont subFont = subtitleLabel->font();
    subFont.setPointSize(9);
    subtitleLabel->setFont(subFont);
    subtitleLabel->setStyleSheet("color: #AAAAAA;");

    titleTextLayout->addWidget(titleLabel);
    titleTextLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleTextLayout);
    headerLayout->addStretch();

    mainLayout->addLayout(headerLayout);

    // 2. 单行文件夹名称输入框
    m_nameEdit = new QLineEdit("新建文件夹", this);
    m_nameEdit->setFixedHeight(38);
    m_nameEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #1E1E1E;
            color: #EEEEEE;
            border: 1px solid #3498db;
            border-radius: 6px;
            padding: 0 12px;
            font-size: 13px;
        }
        QLineEdit:focus {
            border: 1.5px solid #3498db;
        }
    )");
    UiHelper::setupLineEditContextMenu(m_nameEdit);
    connect(m_nameEdit, &QLineEdit::returnPressed, this, &CreateFolderWithItemsDialog::onCreateAndMigrate);
    mainLayout->addWidget(m_nameEdit);

    mainLayout->addStretch();

    // 3. 底部按钮区域：左侧取消，右侧蓝底主按钮
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    m_btnCancel = new QPushButton("取消", this);
    m_btnCancel->setFixedSize(120, 36);
    m_btnCancel->setCursor(Qt::PointingHandCursor);
    m_btnCancel->setStyleSheet(R"(
        QPushButton {
            background-color: #2D2D2D;
            color: #EEEEEE;
            border: 1px solid #3E3E3E;
            border-radius: 6px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #383838;
        }
    )");
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    m_btnCreate = new QPushButton("新的文件夹名称", this);
    m_btnCreate->setFixedHeight(36);
    m_btnCreate->setMinimumWidth(150);
    m_btnCreate->setCursor(Qt::PointingHandCursor);
    m_btnCreate->setStyleSheet(R"(
        QPushButton {
            background-color: #3498db;
            color: #FFFFFF;
            border: none;
            border-radius: 6px;
            font-size: 13px;
            font-weight: bold;
            padding: 0 16px;
        }
        QPushButton:hover {
            background-color: #2980b9;
        }
    )");
    connect(m_btnCreate, &QPushButton::clicked, this, &CreateFolderWithItemsDialog::onCreateAndMigrate);

    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnCreate);
    mainLayout->addLayout(btnLayout);

    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

void CreateFolderWithItemsDialog::onCreateAndMigrate() {
    QString folderName = m_nameEdit->text().trimmed();
    if (folderName.isEmpty()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "文件夹名称不能为空", 1500, QColor("#e74c3c"));
        return;
    }

    QDir baseDir(m_currentDir);
    QString targetFolderPath = baseDir.filePath(folderName);

    // 自动避重：若文件夹已存在，增加递增编号 (例如 新建文件夹 (2))
    int counter = 2;
    while (QDir(targetFolderPath).exists()) {
        targetFolderPath = baseDir.filePath(QString("%1 (%2)").arg(folderName).arg(counter++));
    }

    if (!baseDir.mkpath(targetFolderPath)) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "创建文件夹失败", 1500, QColor("#e74c3c"));
        return;
    }

    // 执行所选项目移动（Cut/Move）入新创建的文件夹
    if (!m_selectedPaths.isEmpty()) {
        LastOperationManager::instance().recordMoveToFolder(targetFolderPath);

        DiskIoContext ioCtx;
        ioCtx.sources = m_selectedPaths;
        ioCtx.destination = targetFolderPath;
        ioCtx.isMove = true;

        QPointer<QWidget> parentWidget(parentWidget());
        DiskIoService::instance().executeAsync(ioCtx, [parentWidget, count = m_selectedPaths.size()](bool success) {
            QMetaObject::invokeMethod(QCoreApplication::instance(), [success, count]() {
                if (success) {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已创建文件夹并将 %1 个项目移动至新文件夹").arg(count), 2000, QColor("#2ecc71"));
                } else {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "移动文件时出错，请重试", 2000, QColor("#e81123"));
                }
            });
        });
    }

    accept();
}

} // namespace QuarkMeta
```

---

## 4. Build & Verification Steps
1. Recompile QuarkMeta target using CMake:
   `cmake --build build`
2. Launch QuarkMeta and select one or more items (files or folders).
3. Right-click selected items to open the context menu.
4. Verify that **"用所选项目新建文件夹(N)"** appears directly below **"添加至“库”"**.
5. Click **"用所选项目新建文件夹(N)"** (or press key `N`).
6. Confirm that the dialog appears matching Image 2 layout (blue circular `+` icon, "添加文件夹" title, "输入新的文件夹名称" label, "新的文件夹名称" primary button).
7. Enter a folder name and click "新的文件夹名称".
8. Verify that:
   - The new folder is created in the current directory.
   - All selected items are safely moved inside the new folder.
   - The view refreshes automatically.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **File Movement SSOT**: All file movement operations route through `DiskIoService::instance().executeAsync(...)`.
- **Icon Reuse**: Uses `UiHelper::getIcon("add", ...)` and `UiHelper::getIcon("folder", ...)` without hardcoding duplicative SVGs.

---

## 6. Header API Signature Verification
| Class / Function Signature | Declared Header File | Signature Match Verification |
| :--- | :--- | :--- |
| `explicit CreateFolderWithItemsDialog(const QString& currentDir, const QStringList& selectedPaths, QWidget* parent = nullptr)` | `src/ui/dialogs/CreateFolderWithItemsDialog.h` | 100% Exact Match |
| `ActionCreateFolderWithSelectedItems` | `src/ui/ContentPanel.h` | 100% Exact Match |

---

## 7. Header Inclusion Chain & Type Completeness Check
| Inclusion / Type Name | File | Purpose & Completeness Check |
| :--- | :--- | :--- |
| `#include "FramelessDialog.h"` | `src/ui/dialogs/CreateFolderWithItemsDialog.h` | Provides `FramelessDialog` base class. |
| `#include "dialogs/CreateFolderWithItemsDialog.h"` | `src/ui/controllers/ContentContextMenu.cpp` | Provides `CreateFolderWithItemsDialog` class definition. |
| `#include "../../util/DiskIoService.h"` | `src/ui/dialogs/CreateFolderWithItemsDialog.cpp` | Provides `DiskIoService` and `DiskIoContext`. |
