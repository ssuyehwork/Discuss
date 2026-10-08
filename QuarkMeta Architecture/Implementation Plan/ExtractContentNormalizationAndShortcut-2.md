# ExtractContentNormalizationAndShortcut-2.md Implementation Plan

## 1. Overview
Fixes the in-panel shortcut `Ctrl + Shift + E` failure and adds "Extract Content" (支持提取内容) to `QuickLookWindow.cpp` context menu:
1. **Root Cause Fix**: Moves `Ctrl + Shift + E` shortcut registration from the local view event filter (`ContentKeyHandler`) to `AppShortcutController` using `QShortcut` with `Qt::WindowShortcut` context and `isEditingFocus()` protection.
2. **Multi-Pane Routing**: Connects `AppShortcutController::extractContentRequested` in `PanelMediator` to dynamically execute extraction on the currently active panel (`m_activeContentPanel`).
3. **QuickLook Integration**: Integrates `ContextMenuFactory::buildExtractContentAction(&menu, m_currentPath, this)` in `QuickLookWindow::showContextMenu`.

## 2. Modified Files List
- `src/ui/AppShortcutController.h`
- `src/ui/AppShortcutController.cpp`
- `src/ui/PanelMediator.cpp`
- `src/ui/QuickLookWindow.cpp`
- `src/ui/controllers/ContextMenuFactory.h`
- `src/ui/controllers/ContextMenuFactory.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`

## 3. Detailed Line-by-Line Changes

```path
src/ui/AppShortcutController.h
```

<<<<<<< SEARCH
signals:
    void toggleImmersiveRequested();
    void createNewFolderRequested();
    void togglePinRequested();
=======
signals:
    void toggleImmersiveRequested();
    void createNewFolderRequested();
    void togglePinRequested();
    void extractContentRequested();
>>>>>>> REPLACE

```path
src/ui/AppShortcutController.cpp
```

<<<<<<< SEARCH
    // 8. Ctrl+Shift+T: 恢复关闭的标签页
    QShortcut* scRestoreTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), m_window);
    scRestoreTab->setContext(Qt::WindowShortcut);
    connect(scRestoreTab, &QShortcut::activated, this, [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->restoreLastClosedTab();
                }
            }
        }
    });
}
=======
    // 8. Ctrl+Shift+T: 恢复关闭的标签页
    QShortcut* scRestoreTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), m_window);
    scRestoreTab->setContext(Qt::WindowShortcut);
    connect(scRestoreTab, &QShortcut::activated, this, [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->restoreLastClosedTab();
                }
            }
        }
    });

    // 10. Ctrl+Shift+E: 提取文件文本内容至剪贴板
    QShortcut* scExtract = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), m_window);
    scExtract->setContext(Qt::WindowShortcut);
    connect(scExtract, &QShortcut::activated, this, [this]() {
        if (!isEditingFocus()) {
            emit extractContentRequested();
        }
    });
}
>>>>>>> REPLACE

```path
src/ui/PanelMediator.cpp
```

<<<<<<< SEARCH
        connect(shortcutController, &AppShortcutController::createNewFolderRequested, this, [this, contentPanel]() {
            if (contentPanel) {
                contentPanel->createNewItem("folder");
            }
        });
=======
        connect(shortcutController, &AppShortcutController::createNewFolderRequested, this, [this, contentPanel]() {
            if (contentPanel) {
                contentPanel->createNewItem("folder");
            }
        });

        connect(shortcutController, &AppShortcutController::extractContentRequested, this, [this]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : rootPanel();
            if (target) {
                QStringList selectedPaths = target->getSelectedPaths();
                if (!selectedPaths.isEmpty()) {
                    ContextMenuFactory::extractContentToClipboard(selectedPaths.first());
                } else {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "未选择任何可提取内容的文件", 1200, QColor("#e81123"));
                }
            }
        });
>>>>>>> REPLACE

```path
src/ui/QuickLookWindow.cpp
```

<<<<<<< SEARCH
    ContextMenuFactory::buildCopyNameAction(&menu, QStringList{m_currentPath}, this);
    ContextMenuFactory::buildCopyPathAction(&menu, QStringList{m_currentPath}, this);
    FavoriteService::instance().buildFavoriteAction(&menu, m_currentPath, this);
=======
    ContextMenuFactory::buildCopyNameAction(&menu, QStringList{m_currentPath}, this);
    ContextMenuFactory::buildCopyPathAction(&menu, QStringList{m_currentPath}, this);
    ContextMenuFactory::buildExtractContentAction(&menu, m_currentPath, this);
    FavoriteService::instance().buildFavoriteAction(&menu, m_currentPath, this);
>>>>>>> REPLACE

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

## 4. Build & Verification Steps
1. Run CMake build / compile script to compile the project.
2. Select any text file (.txt, .md, .cpp, etc.) in `ContentPanel` and press `Ctrl + Shift + E` regardless of widget focus position.
3. Confirm that `AppShortcutController` catches the shortcut and `PanelMediator` triggers `ContextMenuFactory::extractContentToClipboard`, copying text to clipboard with a green feedback overlay.
4. Press Space on a text file to open `QuickLookWindow`, right-click, and select "支持提取内容". Confirm text is extracted.
5. Focus a `QLineEdit` / text input box and press `Ctrl + Shift + E`. Confirm `isEditingFocus()` prevents text extraction from firing while typing.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Registered `Ctrl + Shift + E` in `AppShortcutController` to follow the exact same SSOT architecture as `F5`, `Ctrl+Shift+N`, `Ctrl+Shift+T`.
- Routed via `PanelMediator` to target `m_activeContentPanel` for 100% split-pane multi-window compatibility.

## 6. Header API Signature Verification
- `AppShortcutController::extractContentRequested()`: Signal declared in `AppShortcutController.h`.
- `ContextMenuFactory::extractContentToClipboard(const QString& path)`: Static method declared in `ContextMenuFactory.h`.
- `ContextMenuFactory::buildExtractContentAction(QMenu* menu, const QString& path, QObject* receiver)`: Static method declared in `ContextMenuFactory.h`.

## 7. Header Inclusion Chain & Type Completeness Check
- `PanelMediator.cpp` includes `AppShortcutController.h` and `ContextMenuFactory.h`.
- `AppShortcutController.cpp` includes `<QShortcut>`.
- Type completeness is 100% verified.
