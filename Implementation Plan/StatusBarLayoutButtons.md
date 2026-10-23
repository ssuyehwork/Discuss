# Implementation Plan - StatusBarLayoutButtons.md

## Overview
This implementation plan documents adding a set of layout control buttons to the right side of the main status bar in `MainWindow`.
The buttons correspond to the SVG icons in `resources/08-09-2026 1231/` and are arranged in the following order (from left to right at the far right of the status bar):
1. `隐藏筛选器.svg` (Toggles FilterPanel)
2. `隐藏元数据面板.svg` (Toggles MetaPanel)
3. `内容面板.svg` (Toggles Immersive Mode / Solo Content Panel)
4. `隐藏收藏栏.svg` (Toggles FavoritePanel)
5. `隐藏目录导航.svg` (Toggles NavPanel)
6. `显示收藏栏+内容面板+筛选器.svg` (Applies 3-panel preset; right-click menu toggles left panel between NavPanel & FavoritePanel)
7. `重置分栏.svg` (Resets all splitter sizes and default visibility)

Persistent highlight state (`setChecked(true)`) is maintained for active buttons and synchronizes dynamically with keyboard shortcuts (Tab key) and panel visibility changes.

## Modified Files List
- `resources/style.qss`
- `src/ui/SvgIcons.h`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### `resources/style.qss`

<<<<<<< SEARCH
/* 底部状态栏 */
#StatusBar {
    background-color: #252525;
    border-top: 1px solid #333333;
}
=======
/* 底部状态栏 */
#StatusBar {
    background-color: #252525;
    border-top: 1px solid #333333;
}

/* 状态栏右侧分栏控制按钮 */
QPushButton#StatusBarControlBtn {
    background: transparent;
    border: none;
    border-radius: 4px;
    padding: 0;
    outline: none;
}
QPushButton#StatusBarControlBtn:hover {
    background: #3E3E42;
}
QPushButton#StatusBarControlBtn:pressed, QPushButton#StatusBarControlBtn:checked {
    background: #3E3E42;
    border: 1px solid #555555;
}
>>>>>>> REPLACE

### `src/ui/MainWindow.h`

<<<<<<< SEARCH
    void updateStatusBar();

    TitleBarWidget* m_titleBarWidget = nullptr;
=======
    void updateStatusBar();
    void applyPresetLayout(const QString& leftPanel);
    void updateStatusBarButtonHighlights();

    TitleBarWidget* m_titleBarWidget = nullptr;
>>>>>>> REPLACE
<<<<<<< SEARCH
    // 底部状态栏
    QLabel* m_statusLeft = nullptr;
    QWidget* m_statusBarWidget = nullptr;
    TaskProgressToolBar* m_taskProgressToolBar = nullptr;
=======
    // 底部状态栏
    QLabel* m_statusLeft = nullptr;
    QWidget* m_statusBarWidget = nullptr;
    TaskProgressToolBar* m_taskProgressToolBar = nullptr;

    QPushButton* m_btnToggleFilter = nullptr;
    QPushButton* m_btnToggleMeta = nullptr;
    QPushButton* m_btnContentPanel = nullptr;
    QPushButton* m_btnToggleFavorite = nullptr;
    QPushButton* m_btnToggleNav = nullptr;
    QPushButton* m_btnPresetLayout = nullptr;
    QPushButton* m_btnResetLayout = nullptr;
>>>>>>> REPLACE

### `src/ui/MainWindow.cpp`

<<<<<<< SEARCH
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QTimer>
#include <QCloseEvent>
=======
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QTimer>
#include <QCloseEvent>
#include <QPushButton>
#include <QMenu>
#include <QActionGroup>
#include <QSignalBlocker>
>>>>>>> REPLACE
<<<<<<< SEARCH
    connect(m_panelMediator, &PanelMediator::statusMessageRequested, this, [this](const QString& msg) {
        if (m_statusLeft) m_statusLeft->setText(msg);
    });
}
=======
    connect(m_panelMediator, &PanelMediator::statusMessageRequested, this, [this](const QString& msg) {
        if (m_statusLeft) m_statusLeft->setText(msg);
    });

    connect(m_panelLayoutManager, &PanelLayoutManager::panelVisibilityChanged, this, [this](const QString&, bool) {
        updateStatusBarButtonHighlights();
    });
    connect(m_panelLayoutManager, &PanelLayoutManager::layoutResetCompleted, this, [this]() {
        updateStatusBarButtonHighlights();
    });
    updateStatusBarButtonHighlights();
}
>>>>>>> REPLACE

## Build & Verification Steps
1. Rebuild the application using CMake (`cmake --build build`).
2. Run the application and inspect the bottom right of the status bar.
3. Test left-clicking each button to verify panel visibility toggling and layout resets.
4. Test right-clicking `m_btnPresetLayout` (`显示收藏栏+内容面板+筛选器.svg`) to verify the context menu for toggling between Favorite and Nav panels.
5. Press `Tab` key to enter immersive mode and verify that the `内容面板` button becomes persistently highlighted (`:checked`).
