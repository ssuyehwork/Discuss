# Implementation Plan - Context Menu Open in New Tab & TitleBar Drag-Drop Open

This plan details the changes required to:
1. Add an "在新标签页中打开" (Open in New Tab) option in the right-click context menu between "打开文件夹" / "打开" and "在“资源管理器”中显示".
2. Ensure folder colors are rendered as solid folder SVG icons on tabs when opened.
3. Enable dragging and dropping folders onto the TitleBar or TabBar to open them in a new tab (or switch to the tab if already opened).

## Overview
1. **Context Menu Action**:
   - In `ContentPanel.h`: Add `ActionOpenInNewTab` to `ContextAction` enum.
   - In `ContentContextMenu.cpp`: Add "在新标签页中打开" action in the context menu for folders and drive roots.
   - In `ContentPanel.cpp`: Handle `ActionOpenInNewTab` by adding a tab on `TabBarWidget` or switching to it if already open.
2. **TabBar Helper & Drag-and-Drop**:
   - In `TabBarWidget.h / .cpp`: Add `openOrFocusTab(const QString& path)` helper method. If `path` is already open in a tab, switch to it; otherwise call `addTab(folderName, path, true)`.
   - In `TitleBarWidget.h / .cpp` & `TabBarWidget.h / .cpp`: Enable drag and drop (`setAcceptDrops(true)`). Implement `dragEnterEvent` & `dropEvent` to process dropped folder paths.

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`
- `src/ui/TabBarWidget.h`
- `src/ui/TabBarWidget.cpp`
- `src/ui/TitleBarWidget.h`
- `src/ui/TitleBarWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`
<<<<<<< SEARCH
    enum ContextAction {
        ActionOpen, ActionOpenDefault, ActionShowInExplorer, ActionShowInQuarkMeta, ActionNewFolder, ActionNewMd, ActionNewTxt,
=======
    enum ContextAction {
        ActionOpen, ActionOpenInNewTab, ActionOpenDefault, ActionShowInExplorer, ActionShowInQuarkMeta, ActionNewFolder, ActionNewMd, ActionNewTxt,
>>>>>>> REPLACE

### 2. `src/ui/controllers/ContentContextMenu.cpp`
<<<<<<< SEARCH
        if (isDriveRoot) {
            menu.addAction(UiHelper::getIcon("open", QColor("#EEEEEE"), 18), "打开")->setData(ContentPanel::ActionOpen);
            ContextMenuFactory::buildShowInExplorerAction(&menu, path, m_panel);
=======
        if (isDriveRoot) {
            menu.addAction(UiHelper::getIcon("open", QColor("#EEEEEE"), 18), "打开")->setData(ContentPanel::ActionOpen);
            menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE"), 18), "在新标签页中打开")->setData(ContentPanel::ActionOpenInNewTab);
            ContextMenuFactory::buildShowInExplorerAction(&menu, path, m_panel);
>>>>>>> REPLACE

<<<<<<< SEARCH
        } else {
            menu.addAction(UiHelper::getIcon(isFolder ? "folder" : "open", QColor("#EEEEEE"), 18), isFolder ? "打开文件夹" : "打开")->setData(ContentPanel::ActionOpen);
            if (!isFolder) {
                menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
            }
            ContextMenuFactory::buildShowInExplorerAction(&menu, path, m_panel);
=======
        } else {
            menu.addAction(UiHelper::getIcon(isFolder ? "folder" : "open", QColor("#EEEEEE"), 18), isFolder ? "打开文件夹" : "打开")->setData(ContentPanel::ActionOpen);
            if (isFolder) {
                menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE"), 18), "在新标签页中打开")->setData(ContentPanel::ActionOpenInNewTab);
            } else {
                menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
            }
            ContextMenuFactory::buildShowInExplorerAction(&menu, path, m_panel);
>>>>>>> REPLACE

<<<<<<< SEARCH
        case ContentPanel::ActionOpen:
            m_panel->onDoubleClicked(currentIndex);
            break;
=======
        case ContentPanel::ActionOpen:
            m_panel->onDoubleClicked(currentIndex);
            break;
        case ContentPanel::ActionOpenInNewTab: {
            QString targetPath = path;
            if (targetPath.isEmpty()) {
                targetPath = m_panel->getSelectedPaths().value(0);
            }
            if (!targetPath.isEmpty()) {
                if (m_panel->window()) {
                    TitleBarWidget* titleBar = m_panel->window()->findChild<TitleBarWidget*>();
                    if (titleBar && titleBar->tabBar()) {
                        titleBar->tabBar()->openOrFocusTab(targetPath);
                    }
                }
            }
            break;
        }
>>>>>>> REPLACE

### 3. `src/ui/TabBarWidget.h`
<<<<<<< SEARCH
    void updateCurrentTabTitle(const QString& title, const QString& url);

    void selectNextTab();
=======
    void updateCurrentTabTitle(const QString& title, const QString& url);
    void openOrFocusTab(const QString& path);

    void selectNextTab();
>>>>>>> REPLACE

<<<<<<< SEARCH
protected:
    void showTabContextMenu(int index, const QPoint& globalPos);
=======
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    void showTabContextMenu(int index, const QPoint& globalPos);
>>>>>>> REPLACE

### 4. `src/ui/TabBarWidget.cpp`
<<<<<<< SEARCH
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QDir>
=======
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
>>>>>>> REPLACE

<<<<<<< SEARCH
TabBarWidget::TabBarWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("TabBarWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(30);
=======
TabBarWidget::TabBarWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("TabBarWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setAcceptDrops(true);
    setFixedHeight(30);
>>>>>>> REPLACE

<<<<<<< SEARCH
void TabBarWidget::selectPreviousTab() {
    if (m_tabs.isEmpty()) return;
    int prevIdx = (m_currentIndex - 1 + m_tabs.size()) % m_tabs.size();
    setCurrentIndex(prevIdx, true);
}
=======
void TabBarWidget::selectPreviousTab() {
    if (m_tabs.isEmpty()) return;
    int prevIdx = (m_currentIndex - 1 + m_tabs.size()) % m_tabs.size();
    setCurrentIndex(prevIdx, true);
}

void TabBarWidget::openOrFocusTab(const QString& rawPath) {
    if (rawPath.isEmpty()) return;
    QString cleanTarget = QDir::cleanPath(rawPath);

    for (int i = 0; i < m_tabs.size(); ++i) {
        if (QDir::cleanPath(m_tabs[i].url) == cleanTarget) {
            setCurrentIndex(i, true);
            return;
        }
    }

    QFileInfo fi(cleanTarget);
    QString title = fi.fileName();
    if (title.isEmpty()) title = cleanTarget;
    addTab(title, cleanTarget, true);
}

void TabBarWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QWidget::dragEnterEvent(event);
    }
}

void TabBarWidget::dropEvent(QDropEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            QString path = url.toLocalFile();
            if (!path.isEmpty() && QFileInfo(path).isDir()) {
                openOrFocusTab(path);
                event->acceptProposedAction();
                return;
            }
        }
    }
    QWidget::dropEvent(event);
}
>>>>>>> REPLACE

### 5. `src/ui/TitleBarWidget.h`
<<<<<<< SEARCH
protected:
    void initUi(HoverEventFilter* hoverFilter);
=======
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    void initUi(HoverEventFilter* hoverFilter);
>>>>>>> REPLACE

### 6. `src/ui/TitleBarWidget.cpp`
<<<<<<< SEARCH
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QSignalBlocker>
=======
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QSignalBlocker>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
>>>>>>> REPLACE

<<<<<<< SEARCH
TitleBarWidget::TitleBarWidget(QWidget* parent, HoverEventFilter* hoverFilter)
    : QWidget(parent) {
    setObjectName("TitleBar");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(34);
    initUi(hoverFilter);
}
=======
TitleBarWidget::TitleBarWidget(QWidget* parent, HoverEventFilter* hoverFilter)
    : QWidget(parent) {
    setObjectName("TitleBar");
    setAttribute(Qt::WA_StyledBackground, true);
    setAcceptDrops(true);
    setFixedHeight(34);
    initUi(hoverFilter);
}

void TitleBarWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QWidget::dragEnterEvent(event);
    }
}

void TitleBarWidget::dropEvent(QDropEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            QString path = url.toLocalFile();
            if (!path.isEmpty() && QFileInfo(path).isDir()) {
                if (m_tabBar) {
                    m_tabBar->openOrFocusTab(path);
                }
                event->acceptProposedAction();
                return;
            }
        }
    }
    QWidget::dropEvent(event);
}
>>>>>>> REPLACE

## Build & Verification Steps
1. Build the project using CMake & MSVC / Ninja.
2. Run application and verify:
   - Right click a folder / drive: verify "在新标签页中打开" is present between "打开" and "在“资源管理器”中显示".
   - Click "在新标签页中打开": verify a new tab opens displaying the folder name, and the address bar updates.
   - Drag and drop a folder onto the title bar or tab bar: verify a new tab opens (or switches to the existing tab if already open).

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `TabBarWidget::openOrFocusTab` helper for deduplicating opened tabs.
- Reused `NavigationService::instance().navigateTo(path)` via `currentTabChanged` signal.

## Header API Signature Verification
- `TabBarWidget::openOrFocusTab(const QString& path)`
- `ContentPanel::ContextAction::ActionOpenInNewTab`
