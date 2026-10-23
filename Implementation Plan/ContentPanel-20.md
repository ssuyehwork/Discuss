# Implementation Plan: ContentPanel-20.md (Dual Pane Paths Signal Notification)

This implementation plan adds the `dualPanePathsChanged` signal to `ContentPanel` and ensures that whenever split mode is activated, deactivated, or whenever either the primary or secondary pane navigates to a new directory, `ContentPanel` emits `dualPanePathsChanged(path1, path2)`.

## Overview
1. Add signal `dualPanePathsChanged(const QString& path1, const QString& path2)` to `ContentPanel.h`.
2. Implement helper `emitDualPanePathsChanged()` in `ContentPanel.cpp` that extracts current paths of primary and secondary panes and emits `dualPanePathsChanged`.
3. Call `emitDualPanePathsChanged()` in `splitPane()`, `closeSecondaryPane()`, and when `directorySelected` is emitted in primary or secondary pane.

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`
<<<<<<< SEARCH
    void statusBarStatsUpdated(int fileCount, int folderCount, int totalCount);
    void statusBarMessageReady(const QString& message);
=======
    void statusBarStatsUpdated(int fileCount, int folderCount, int totalCount);
    void statusBarMessageReady(const QString& message);
    void dualPanePathsChanged(const QString& path1, const QString& path2);
>>>>>>> REPLACE

### 2. `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
        m_secondaryContentPanel = new ContentPanel(m_secondaryPaneContainer);
        secLayout->addWidget(m_secondaryContentPanel);

        m_paneSplitter->addWidget(m_secondaryPaneContainer);
        m_mainLayout->addWidget(m_paneSplitter, 1);

        emit secondaryPaneCreated(m_secondaryContentPanel);
    } else {
=======
        m_secondaryContentPanel = new ContentPanel(m_secondaryPaneContainer);
        secLayout->addWidget(m_secondaryContentPanel);

        m_paneSplitter->addWidget(m_secondaryPaneContainer);
        m_mainLayout->addWidget(m_paneSplitter, 1);

        connect(m_secondaryContentPanel, &ContentPanel::directorySelected, this, [this](const QString&) {
            if (m_isSplit) {
                QString p1 = m_currentPath;
                QString p2 = m_secondaryContentPanel ? m_secondaryContentPanel->currentPath() : QString();
                emit dualPanePathsChanged(p1, p2);
            }
        });

        emit secondaryPaneCreated(m_secondaryContentPanel);
    } else {
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (m_secondaryContentPanel) {
        m_secondaryContentPanel->loadDirectory(!secondaryPath.isEmpty() ? secondaryPath : m_currentPath);
    }
}
=======
    if (m_secondaryContentPanel) {
        m_secondaryContentPanel->loadDirectory(!secondaryPath.isEmpty() ? secondaryPath : m_currentPath);
    }

    if (m_isSplit) {
        QString p1 = m_currentPath;
        QString p2 = m_secondaryContentPanel ? m_secondaryContentPanel->currentPath() : QString();
        emit dualPanePathsChanged(p1, p2);
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_isSplit = false;
    emit secondaryPaneClosed();
}
=======
    m_isSplit = false;
    emit secondaryPaneClosed();
    emit directorySelected(m_currentPath);
}
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_currentPath = path;
    m_selectionState.currentFolder = path;
    emit directorySelected(path);
=======
    m_currentPath = path;
    m_selectionState.currentFolder = path;
    emit directorySelected(path);
    if (m_isSplit) {
        QString p1 = m_currentPath;
        QString p2 = m_secondaryContentPanel ? m_secondaryContentPanel->currentPath() : QString();
        emit dualPanePathsChanged(p1, p2);
    }
>>>>>>> REPLACE

## Build & Verification Steps
1. Build via CMake to ensure no compilation errors or missing symbols.
2. Check that `dualPanePathsChanged` is emitted when `splitPane()` is called and when either pane navigates.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses existing `ContentPanel::currentPath()` and `directorySelected` signals without creating redundant state variables.

## Header API Signature Verification
- `ContentPanel::dualPanePathsChanged(const QString&, const QString&)` -> Signal added to `src/ui/ContentPanel.h`.
