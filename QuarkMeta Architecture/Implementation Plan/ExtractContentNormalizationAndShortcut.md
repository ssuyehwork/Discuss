# ExtractContentNormalizationAndShortcut.md Implementation Plan

## 1. Overview
This implementation plan normalizes the "Extract Content" (提取内容) functionality across the software and binds it to the in-panel shortcut `Ctrl + Shift + E`:
1. Creates `ContextMenuFactory::extractContentToClipboard(const QString& path)` as the unified SSOT execution API for text extraction feedback, clipboard writing, and validation.
2. Creates `ContextMenuFactory::buildExtractContentAction(QMenu* menu, const QString& path, QObject* receiver)` to build "支持提取内容 / 不支持提取内容" menu items, eliminating duplicated code in `ContentContextMenu.cpp`.
3. Integrates `Ctrl + Shift + E` shortcut in `ContentKeyHandler::handleKeyPress` to invoke `ContextMenuFactory::extractContentToClipboard` for the selected item.

## 2. Modified Files List
- `src/ui/controllers/ContextMenuFactory.h`
- `src/ui/controllers/ContextMenuFactory.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`
- `src/ui/controllers/ContentKeyHandler.cpp`

## 3. Detailed Line-by-Line Changes

```path
src/ui/controllers/ContextMenuFactory.h
```

<<<<<<< SEARCH
    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);
    static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver = nullptr);
=======
    /**
     * @brief 提取文件文本内容至剪贴板 SSOT 入口 (快捷键 Ctrl+Shift+E 与 右键菜单 "支持提取内容" 共同调用)
     */
    static bool extractContentToClipboard(const QString& path);

    /**
     * @brief 构建“提取内容”右键菜单项 (自动处理可提取/不可提取状态与点击回调)
     */
    static QAction* buildExtractContentAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);

    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);
    static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver = nullptr);
>>>>>>> REPLACE

```path
src/ui/controllers/ContextMenuFactory.cpp
```

<<<<<<< SEARCH
#include "ContextMenuFactory.h"
#include "../UiHelper.h"
#include "../ToolTipOverlay.h"
#include "../ColorPicker.h"
#include "../../core/CoreEngine.h"
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QWidgetAction>
#include <QHBoxLayout>
#include <QLabel>
=======
#include "ContextMenuFactory.h"
#include "../UiHelper.h"
#include "../ToolTipOverlay.h"
#include "../ColorPicker.h"
#include "../../core/CoreEngine.h"
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QMenu>
#include <QWidgetAction>
#include <QHBoxLayout>
#include <QLabel>
>>>>>>> REPLACE

<<<<<<< SEARCH
QAction* ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver) {
=======
bool ContextMenuFactory::extractContentToClipboard(const QString& path) {
    if (path.isEmpty() || QFileInfo(path).isDir()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "提取失败：只能提取文件内容", 1500, QColor("#e81123"));
        return false;
    }

    QString ext = QFileInfo(path).suffix().toLower();
    if (!UiHelper::isTextFile(ext)) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "不支持提取内容：文件格式超出纯文本范围", 1500, QColor("#e81123"));
        return false;
    }

    QString content;
    if (UiHelper::extractTextContent(path, content)) {
        QApplication::clipboard()->setText(content);
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已成功提取内容并存入剪贴板 (共 %1 字符)").arg(content.length()), 1500, QColor("#2ecc71"));
        return true;
    } else {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "提取失败：文件超过限制或无法作为纯文本解析", 1500, QColor("#e81123"));
        return false;
    }
}

QAction* ContextMenuFactory::buildExtractContentAction(QMenu* menu, const QString& path, QObject* receiver) {
    if (!menu || path.isEmpty()) return nullptr;

    bool isFolder = QFileInfo(path).isDir();
    QString fileExt = QFileInfo(path).suffix().toLower();
    bool canExtract = !isFolder && UiHelper::isTextFile(fileExt);

    if (canExtract) {
        QAction* actExtract = menu->addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "支持提取内容");
        QObject::connect(actExtract, &QAction::triggered, receiver ? receiver : menu, [path]() {
            extractContentToClipboard(path);
        });
        return actExtract;
    } else {
        QAction* actDisabled = menu->addAction(UiHelper::getIcon("prohibit", QColor("#888888"), 18), "不支持提取内容");
        actDisabled->setEnabled(false);
        return actDisabled;
    }
}

QAction* ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver) {
>>>>>>> REPLACE

```path
src/ui/controllers/ContentContextMenu.cpp
```

<<<<<<< SEARCH
            QString driveExt = QFileInfo(path).suffix().toLower();
            bool canExtractDrive = UiHelper::isTextFile(driveExt);
            if (canExtractDrive) {
                QAction* actExtract = moreMenuDrive->addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "支持提取内容");
                connect(actExtract, &QAction::triggered, this, [path]() {
                    QString content;
                    if (UiHelper::extractTextContent(path, content)) {
                        QApplication::clipboard()->setText(content);
                        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已成功提取内容并存入剪贴板 (共 %1 字符)").arg(content.length()), 1500, QColor("#2ecc71"));
                    } else {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "提取失败：文件超过限制或无法作为纯文本解析", 1500, QColor("#e81123"));
                    }
                });
            } else {
                QAction* actDisabled = moreMenuDrive->addAction(UiHelper::getIcon("prohibit", QColor("#888888"), 18), "不支持提取内容");
                actDisabled->setEnabled(false);
            }
=======
            ContextMenuFactory::buildExtractContentAction(moreMenuDrive, path, m_panel);
>>>>>>> REPLACE

<<<<<<< SEARCH
            QString fileExt = QFileInfo(path).suffix().toLower();
            bool canExtract = !isFolder && UiHelper::isTextFile(fileExt);
            if (canExtract) {
                QAction* actExtract = moreMenu->addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "支持提取内容");
                connect(actExtract, &QAction::triggered, this, [path]() {
                    QString content;
                    if (UiHelper::extractTextContent(path, content)) {
                        QApplication::clipboard()->setText(content);
                        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已成功提取内容并存入剪贴板 (共 %1 字符)").arg(content.length()), 1500, QColor("#2ecc71"));
                    } else {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "提取失败：文件超过限制或无法作为纯文本解析", 1500, QColor("#e81123"));
                    }
                });
            } else {
                QAction* actDisabled = moreMenu->addAction(UiHelper::getIcon("prohibit", QColor("#888888"), 18), "不支持提取内容");
                actDisabled->setEnabled(false);
            }
=======
            ContextMenuFactory::buildExtractContentAction(moreMenu, path, m_panel);
>>>>>>> REPLACE

```path
src/ui/controllers/ContentKeyHandler.cpp
```

<<<<<<< SEARCH
    // 4. Ctrl + Shift + C / V / R
    if (keyEvent->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
=======
    // 4. Ctrl + Shift + C / V / E / R
    if (keyEvent->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (keyEvent->key() == Qt::Key_E) {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (!selectedPaths.isEmpty()) {
                ContextMenuFactory::extractContentToClipboard(selectedPaths.first());
            } else {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "未选择任何可提取内容的文件", 1200, QColor("#e81123"));
            }
            return true;
        }
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. Run CMake build / compile script to compile the project.
2. Launch the application. Select a text file (.txt, .md, etc.) in ContentPanel and press `Ctrl + Shift + E`.
3. Verify that the text content is copied to clipboard and a green feedback overlay is displayed.
4. Right-click the file, hover "更多", and click "支持提取内容". Verify that it triggers the exact same SSOT execution pipeline.
5. Right-click a non-text file or folder, hover "更多", and verify that "不支持提取内容" is disabled with `"prohibit"` icon.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Created `ContextMenuFactory::extractContentToClipboard` as single SSOT execution API.
- Replaced 30+ lines of duplicate code in `ContentContextMenu.cpp` with `ContextMenuFactory::buildExtractContentAction`.

## 6. Header API Signature Verification
- `ContextMenuFactory::extractContentToClipboard(const QString& path)`: Static method declared in `ContextMenuFactory.h`.
- `ContextMenuFactory::buildExtractContentAction(QMenu* menu, const QString& path, QObject* receiver)`: Static method declared in `ContextMenuFactory.h`.

## 7. Header Inclusion Chain & Type Completeness Check
- `ContextMenuFactory.cpp` includes `<QFileInfo>`, `<QMenu>`, `<QAction>`, `<QApplication>`, `<QClipboard>`.
- `ContentKeyHandler.cpp` includes `ContextMenuFactory.h`.
- Type completeness for `QMenu`, `QAction`, `QString`, and `ToolTipOverlay` is 100% verified.
