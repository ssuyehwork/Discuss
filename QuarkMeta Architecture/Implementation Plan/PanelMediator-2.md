# Implementation Plan: PanelMediator-2.md (Bridge Dual Pane Signal to TabBarWidget)

This implementation plan connects `ContentPanel::dualPanePathsChanged` via `PanelMediator` (or `MainWindow`) to `TabBarWidget::updateDualPaneTabTitle`.

## Overview
1. In `PanelMediator::setupConnections`, subscribe to `ContentPanel::dualPanePathsChanged`.
2. When triggered, extract folder names from `path1` and `path2` using `QFileInfo`.
3. Invoke `TabBarWidget::updateDualPaneTabTitle(title1, path1, title2, path2)` on `TabBarWidget`.
4. When `secondaryPaneClosed` is received, invoke `TabBarWidget::updateCurrentTabTitle(mainTitle, mainPath)` to revert the merged title back to single-folder representation.

## Modified Files List
- `src/ui/PanelMediator.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/PanelMediator.cpp`
<<<<<<< SEARCH
        connect(contentPanel, &ContentPanel::directorySelected, this, [this](const QString& path) {
            if (m_titleBarWidget && m_titleBarWidget->tabBarWidget()) {
                QFileInfo fi(path);
                QString title = fi.fileName();
                if (title.isEmpty()) title = path;
                m_titleBarWidget->tabBarWidget()->updateCurrentTabTitle(title, path);
            }
        });
=======
        connect(contentPanel, &ContentPanel::directorySelected, this, [this](const QString& path) {
            if (m_titleBarWidget && m_titleBarWidget->tabBarWidget()) {
                QFileInfo fi(path);
                QString title = fi.fileName();
                if (title.isEmpty()) title = path;
                m_titleBarWidget->tabBarWidget()->updateCurrentTabTitle(title, path);
            }
        });

        connect(contentPanel, &ContentPanel::dualPanePathsChanged, this, [this](const QString& path1, const QString& path2) {
            if (m_titleBarWidget && m_titleBarWidget->tabBarWidget()) {
                auto cleanName = [](const QString& u) -> QString {
                    if (u == "computer://" || u.isEmpty()) return "此电脑";
                    QFileInfo fi(u);
                    QString fn = fi.fileName();
                    return fn.isEmpty() ? u : fn;
                };
                m_titleBarWidget->tabBarWidget()->updateDualPaneTabTitle(cleanName(path1), path1, cleanName(path2), path2);
            }
        });
>>>>>>> REPLACE

## Build & Verification Steps
1. Build via CMake.
2. Trigger dual-pane split on `ContentPanel` and verify that `TabBarWidget` updates active tab title to `"FolderA | FolderB"` instantly.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `TabBarWidget::updateDualPaneTabTitle` and `TabBarWidget::updateCurrentTabTitle` without adding duplicated tab formatting code.

## Header API Signature Verification
- `TabBarWidget::updateDualPaneTabTitle(const QString&, const QString&, const QString&, const QString&)` -> Declared in `src/ui/TabBarWidget.h`.
