# ContentPanel Dual-Pane Parallel Layout & 5px Separation Refactoring Plan

## Overview
This implementation plan refactors the dual-pane layout in `ContentPanel` to eliminate the outer wrapping border ("box-in-box" / 圈外圈内) and present the primary and secondary panes as two independent, parallel card panels separated by a 5px gap.

### Root Cause Analysis
Previously, when splitting `ContentPanel`:
1. The primary `ContentPanel` (`#EditorContainer`) retained its outer container border and kept `m_topBarWidget` at the top of the main layout.
2. `m_paneSplitter` was inserted below `m_topBarWidget`, containing the primary `m_viewStack` on the left and a new `ContentPanel` (`m_secondaryContentPanel`, also styled with `#EditorContainer`) on the right.
3. This resulted in the secondary panel being rendered as an inner bordered box nested inside the outer bordered box of the primary panel.

### Solution
1. **Parallel Primary Container (`m_primaryPaneContainer`)**: When splitting into dual-pane mode, both `m_topBarWidget` and `m_viewStack` of the primary panel are reparented into a dedicated `m_primaryPaneContainer` styled with `setObjectName("EditorContainer")`.
2. **Sibling Panes in `QSplitter`**: `m_paneSplitter` holds `m_primaryPaneContainer` (left/top) and `m_secondaryPaneContainer` (right/bottom) as equal siblings.
3. **Outer Container Transparency**: The outer `ContentPanel` sets its object name to `"ContentPanelHost"` (or applies `background: transparent; border: none;`) while split, removing the outer wrapping border.
4. **5px Handle Spacing**: `m_paneSplitter->setHandleWidth(5)` creates a clean 5px physical separation between the two parallel card panels.
5. **Seamless Unsplit Restoration**: When closing the secondary pane (`closeSecondaryPane()`), `m_topBarWidget` and `m_viewStack` are returned to `ContentPanel`'s `m_mainLayout`, and the outer `ContentPanel` restores its `#EditorContainer` styling.

---

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `resources/style.qss`

---

## Detailed Line-by-Line Changes

### File: `src/ui/ContentPanel.h`

<<<<<<< SEARCH
    QSplitter* m_paneSplitter = nullptr;
    QWidget* m_secondaryPaneContainer = nullptr;
    ContentPanel* m_secondaryContentPanel = nullptr;
=======
    QSplitter* m_paneSplitter = nullptr;
    QWidget* m_primaryPaneContainer = nullptr;
    QWidget* m_secondaryPaneContainer = nullptr;
    ContentPanel* m_secondaryContentPanel = nullptr;
>>>>>>> REPLACE

---

### File: `src/ui/ContentPanel.cpp`

<<<<<<< SEARCH
void ContentPanel::splitPane(Qt::Orientation orientation, const QString& secondaryPath) {
    if (m_isSplit && m_splitOrientation == orientation) {
        if (m_secondaryContentPanel && !secondaryPath.isEmpty()) {
            m_secondaryContentPanel->loadDirectory(secondaryPath);
        }
        return;
    }

    m_splitOrientation = orientation;
    m_isSplit = true;

    if (!m_paneSplitter) {
        m_paneSplitter = new QSplitter(m_splitOrientation, this);
        m_paneSplitter->setHandleWidth(2);
        m_mainLayout->removeWidget(m_viewStack);
        m_paneSplitter->addWidget(m_viewStack);

        m_secondaryPaneContainer = new QWidget(m_paneSplitter);
        QVBoxLayout* secLayout = new QVBoxLayout(m_secondaryPaneContainer);
        secLayout->setContentsMargins(0, 0, 0, 0);

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
        m_paneSplitter->setOrientation(m_splitOrientation);
        if (m_secondaryPaneContainer) {
            m_secondaryPaneContainer->show();
        }
    }

    if (m_secondaryContentPanel) {
        m_secondaryContentPanel->loadDirectory(!secondaryPath.isEmpty() ? secondaryPath : m_currentPath);
    }

    QList<int> sizes;
    int total = (orientation == Qt::Horizontal) ? width() : height();
    sizes << total / 2 << total / 2;
    m_paneSplitter->setSizes(sizes);

    if (m_isSplit) {
        QString p1 = m_currentPath;
        QString p2 = m_secondaryContentPanel ? m_secondaryContentPanel->currentPath() : QString();
        emit dualPanePathsChanged(p1, p2);
    }
}

void ContentPanel::closeSecondaryPane() {
    if (!m_isSplit) return;

    m_isSplit = false;
    if (m_secondaryPaneContainer) {
        m_secondaryPaneContainer->hide();
    }
    emit secondaryPaneClosed();
    emit directorySelected(m_currentPath);
}
=======
void ContentPanel::splitPane(Qt::Orientation orientation, const QString& secondaryPath) {
    if (m_isSplit && m_splitOrientation == orientation) {
        if (m_secondaryContentPanel && !secondaryPath.isEmpty()) {
            m_secondaryContentPanel->loadDirectory(secondaryPath);
        }
        return;
    }

    m_splitOrientation = orientation;
    m_isSplit = true;

    // Remove outer border styling from main host panel while split
    setObjectName("ContentPanelHost");
    style()->unpolish(this);
    style()->polish(this);

    if (!m_paneSplitter) {
        m_paneSplitter = new QSplitter(m_splitOrientation, this);
        m_paneSplitter->setHandleWidth(5);
        m_paneSplitter->setChildrenCollapsible(false);

        // 1. Primary pane container
        m_primaryPaneContainer = new QWidget(m_paneSplitter);
        m_primaryPaneContainer->setObjectName("EditorContainer");
        QVBoxLayout* primLayout = new QVBoxLayout(m_primaryPaneContainer);
        primLayout->setContentsMargins(0, 0, 0, 0);
        primLayout->setSpacing(0);

        if (m_topBarWidget) {
            m_mainLayout->removeWidget(m_topBarWidget);
            primLayout->addWidget(m_topBarWidget);
        }
        if (m_viewStack) {
            m_mainLayout->removeWidget(m_viewStack);
            primLayout->addWidget(m_viewStack, 1);
        }

        m_paneSplitter->addWidget(m_primaryPaneContainer);

        // 2. Secondary pane container
        m_secondaryPaneContainer = new QWidget(m_paneSplitter);
        QVBoxLayout* secLayout = new QVBoxLayout(m_secondaryPaneContainer);
        secLayout->setContentsMargins(0, 0, 0, 0);
        secLayout->setSpacing(0);

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
        m_paneSplitter->setOrientation(m_splitOrientation);
        if (m_primaryPaneContainer) {
            m_primaryPaneContainer->show();
        }
        if (m_secondaryPaneContainer) {
            m_secondaryPaneContainer->show();
        }
        if (m_paneSplitter) {
            m_paneSplitter->show();
        }
    }

    if (m_secondaryContentPanel) {
        m_secondaryContentPanel->loadDirectory(!secondaryPath.isEmpty() ? secondaryPath : m_currentPath);
    }

    QList<int> sizes;
    int total = (orientation == Qt::Horizontal) ? width() : height();
    sizes << total / 2 << total / 2;
    m_paneSplitter->setSizes(sizes);

    if (m_isSplit) {
        QString p1 = m_currentPath;
        QString p2 = m_secondaryContentPanel ? m_secondaryContentPanel->currentPath() : QString();
        emit dualPanePathsChanged(p1, p2);
    }
}

void ContentPanel::closeSecondaryPane() {
    if (!m_isSplit) return;

    m_isSplit = false;

    if (m_paneSplitter) {
        m_paneSplitter->hide();
    }

    if (m_primaryPaneContainer) {
        m_primaryPaneContainer->layout()->removeWidget(m_topBarWidget);
        m_primaryPaneContainer->layout()->removeWidget(m_viewStack);
    }

    if (m_topBarWidget) {
        m_mainLayout->addWidget(m_topBarWidget);
        m_topBarWidget->show();
    }
    if (m_viewStack) {
        m_mainLayout->addWidget(m_viewStack, 1);
        m_viewStack->show();
    }

    // Restore standard EditorContainer styling for single-pane mode
    setObjectName("EditorContainer");
    style()->unpolish(this);
    style()->polish(this);

    emit secondaryPaneClosed();
    emit directorySelected(m_currentPath);
}
>>>>>>> REPLACE

---

### File: `resources/style.qss`

<<<<<<< SEARCH
QFrame#EditorContainer {
    background-color: #1a1a1a;
    border: 1px solid #333333;
    border-radius: 8px;
}
=======
QFrame#EditorContainer, QWidget#EditorContainer {
    background-color: #1a1a1a;
    border: 1px solid #333333;
    border-radius: 8px;
}

QFrame#ContentPanelHost, QWidget#ContentPanelHost {
    background-color: transparent;
    border: none;
}

QSplitter::handle {
    background-color: transparent;
}
>>>>>>> REPLACE

---

## Build & Verification Steps
1. Build the target using CMake / Ninja:
   `cmake --build build`
2. Run the application and open dual-pane mode (via dragging or context menu split).
3. Verify that the primary pane and secondary pane are rendered as two independent parallel card panels with identical borders and a clean 5px gap between them.
4. Verify that closing the secondary pane restores single-pane mode without visual artifacts.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `splitPane()` and `closeSecondaryPane()` behavior.
- Reuses `#EditorContainer` styling contract for both parallel sub-containers.

---

## Header API Signature Verification
- `ContentPanel::splitPane(Qt::Orientation, const QString&)`
- `ContentPanel::closeSecondaryPane()`
- `QSplitter::setHandleWidth(int)`
- `QWidget::setObjectName(const QString&)`
