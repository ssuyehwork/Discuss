#include "ContentPaneSplitManager.h"
#include "../ContentPanel.h"
#include "../ContentHeaderWidget.h"
#include "../TabBarWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStyle>
#include <QDir>

namespace QuarkMeta {

ContentPaneSplitManager::ContentPaneSplitManager(ContentPanel* panel)
    : QObject(panel), m_panel(panel)
{
}

ContentPanel* ContentPaneSplitManager::secondaryContentPanel() const {
    return m_panes.isEmpty() ? nullptr : m_panes.first();
}

ContentPanel* ContentPaneSplitManager::rootPane() const {
    return m_rootPane ? m_rootPane : m_panel;
}

bool ContentPaneSplitManager::isSplitMode() const {
    return m_isSplit;
}

void ContentPaneSplitManager::splitPane(Qt::Orientation orientation, const QString& secondaryPath) {
    if (!m_panel) return;

    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->splitPane(orientation, secondaryPath);
        return;
    }

    m_splitOrientation = orientation;

    if (!m_paneSplitter) {
        m_isSplit = true;
        m_panel->setProperty("isHostPanel", "true");
        m_panel->style()->unpolish(m_panel);
        m_panel->style()->polish(m_panel);

        m_paneSplitter = new QSplitter(m_splitOrientation, m_panel);
        m_paneSplitter->setHandleWidth(5);
        m_paneSplitter->setChildrenCollapsible(false);

        m_primaryPaneContainer = new QFrame(m_paneSplitter);
        m_primaryPaneContainer->setObjectName("EditorContainer");
        m_primaryPaneContainer->setAttribute(Qt::WA_StyledBackground, true);
        m_primaryPaneContainer->setMinimumWidth(230);
        QVBoxLayout* primLayout = new QVBoxLayout(m_primaryPaneContainer);
        primLayout->setContentsMargins(0, 0, 0, 0);
        primLayout->setSpacing(0);

        if (m_panel->m_headerWidget) {
            m_panel->m_mainLayout->removeWidget(m_panel->m_headerWidget);
            primLayout->addWidget(m_panel->m_headerWidget);
        }
        if (m_panel->m_viewStack) {
            m_panel->m_mainLayout->removeWidget(m_panel->m_viewStack);
            primLayout->addWidget(m_panel->m_viewStack, 1);
        }

        m_paneSplitter->addWidget(m_primaryPaneContainer);
        m_panel->m_mainLayout->addWidget(m_paneSplitter, 1);
    } else {
        m_isSplit = true;
        m_panel->setProperty("isHostPanel", "true");
        m_panel->style()->unpolish(m_panel);
        m_panel->style()->polish(m_panel);
        m_paneSplitter->setOrientation(m_splitOrientation);
        if (m_primaryPaneContainer) {
            if (m_panel->m_headerWidget && m_primaryPaneContainer->layout()) {
                m_panel->m_mainLayout->removeWidget(m_panel->m_headerWidget);
                m_primaryPaneContainer->layout()->addWidget(m_panel->m_headerWidget);
            }
            if (m_panel->m_viewStack && m_primaryPaneContainer->layout()) {
                m_panel->m_mainLayout->removeWidget(m_panel->m_viewStack);
                if (QVBoxLayout* primVBox = qobject_cast<QVBoxLayout*>(m_primaryPaneContainer->layout())) {
                    primVBox->addWidget(m_panel->m_viewStack, 1);
                } else {
                    m_primaryPaneContainer->layout()->addWidget(m_panel->m_viewStack);
                }
            }
            m_primaryPaneContainer->show();
        }
        m_paneSplitter->show();
    }

    if (paneCount() >= ContentPanel::kMaxPanes) {
        ContentPanel* target = m_activePaneForSplit ? m_activePaneForSplit : m_panel;
        if (!secondaryPath.isEmpty()) {
            target->loadDirectory(secondaryPath);
        }
        return;
    }

    QWidget* container = new QWidget(m_paneSplitter);
    container->setMinimumWidth(230);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    ContentPanel* newPane = new ContentPanel(container);
    newPane->setIsSecondaryPane(true);
    if (newPane->m_splitManager) {
        newPane->m_splitManager->m_rootPane = m_panel;
    }
    newPane->setViewMode(m_panel->currentViewMode());

    connect(newPane, &ContentPanel::closePaneRequested, m_panel, [this, newPane]() {
        closePane(newPane);
    });
    connect(newPane, &ContentPanel::directorySelected, m_panel, [this, newPane](const QString& path) {
        newPane->loadDirectory(path);
        emit m_panel->dualPanePathsChanged(m_panel->currentPath(), path);
    });

    layout->addWidget(newPane);
    m_paneSplitter->addWidget(container);

    m_paneContainers.append(container);
    m_panes.append(newPane);

    newPane->loadDirectory(!secondaryPath.isEmpty() ? secondaryPath : m_panel->currentPath());

    redistributePaneSizes();

    emit m_panel->secondaryPaneCreated(newPane);
}

void ContentPaneSplitManager::closePane(ContentPanel* pane) {
    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->closePane(pane);
        return;
    }

    if (m_activePaneForSplit == pane) {
        m_activePaneForSplit = nullptr;
    }

    int idx = m_panes.indexOf(pane);
    if (idx < 0) return;

    QWidget* container = m_paneContainers.takeAt(idx);
    m_panes.removeAt(idx);
    if (container) {
        container->deleteLater();
    }

    if (m_panes.isEmpty()) {
        m_isSplit = false;
        m_panel->setProperty("isHostPanel", "false");
        m_panel->style()->unpolish(m_panel);
        m_panel->style()->polish(m_panel);
        if (m_paneSplitter) {
            m_paneSplitter->hide();
        }
        if (m_primaryPaneContainer) {
            m_primaryPaneContainer->layout()->removeWidget(m_panel->m_headerWidget);
            m_primaryPaneContainer->layout()->removeWidget(m_panel->m_viewStack);
        }
        if (m_panel->m_headerWidget) {
            m_panel->m_mainLayout->addWidget(m_panel->m_headerWidget);
            m_panel->m_headerWidget->show();
        }
        if (m_panel->m_viewStack) {
            m_panel->m_mainLayout->addWidget(m_panel->m_viewStack, 1);
            m_panel->m_viewStack->show();
        }
        emit m_panel->secondaryPaneClosed();
        emit m_panel->directorySelected(m_panel->currentPath());
    } else {
        redistributePaneSizes();
    }
}

void ContentPaneSplitManager::closeSecondaryPane() {
    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->closeSecondaryPane();
        return;
    }

    if (m_panes.isEmpty()) return;

    ContentPanel* target = m_activePaneForSplit ? m_activePaneForSplit : m_panes.last();
    if (target == m_panel) {
        target = m_panes.last();
    }
    closePane(target);
}

void ContentPaneSplitManager::redistributePaneSizes() {
    if (!m_paneSplitter) return;
    int count = paneCount();
    if (count <= 1) return;
    int total = (m_splitOrientation == Qt::Horizontal) ? m_panel->width() : m_panel->height();
    int each = total / count;
    QList<int> sizes;
    for (int i = 0; i < count; ++i) {
        sizes << each;
    }
    m_paneSplitter->setSizes(sizes);
}

TabSplitState ContentPaneSplitManager::exportSplitState() const {
    TabSplitState state;
    if (rootPane() != m_panel) {
        return rootPane()->m_splitManager->exportSplitState();
    }

    state.isSplit = m_isSplit;
    state.orientation = m_splitOrientation;
    state.panePaths.append(m_panel->currentPath());

    for (int i = 0; i < m_panes.size(); ++i) {
        if (m_panes[i]) {
            state.panePaths.append(m_panes[i]->currentPath());
            if (m_activePaneForSplit == m_panes[i]) {
                state.activePaneIndex = i + 1;
            }
        }
    }

    if (m_activePaneForSplit == m_panel || m_activePaneForSplit == nullptr) {
        state.activePaneIndex = 0;
    }

    return state;
}

void ContentPaneSplitManager::restoreSplitState(const TabSplitState& state) {
    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->restoreSplitState(state);
        return;
    }

    // 1. 关闭现有所有副窗格
    while (!m_panes.isEmpty()) {
        closePane(m_panes.last());
    }

    // 2. 还原主窗格路径
    if (!state.panePaths.isEmpty()) {
        m_panel->loadDirectory(state.panePaths.first());
    }

    // 3. 如果快照包含分屏且路径大于 1，则动态构建副窗格
    if (state.isSplit && state.panePaths.size() > 1) {
        m_splitOrientation = state.orientation;
        for (int i = 1; i < state.panePaths.size(); ++i) {
            splitPane(state.orientation, state.panePaths[i]);
        }
    }

    // 4. 恢复激活窗格
    if (state.activePaneIndex == 0 || m_panes.isEmpty()) {
        m_panel->setActivePane(true);
    } else if (state.activePaneIndex - 1 < m_panes.size()) {
        m_panes[state.activePaneIndex - 1]->setActivePane(true);
    }
}

void ContentPaneSplitManager::setActivePane(bool active) {
    if (active && rootPane()) {
        rootPane()->m_splitManager->m_activePaneForSplit = m_panel;
    }

    if (m_isSplit && m_primaryPaneContainer) {
        m_primaryPaneContainer->setProperty("activePane", active ? "true" : "false");
        m_primaryPaneContainer->style()->unpolish(m_primaryPaneContainer);
        m_primaryPaneContainer->style()->polish(m_primaryPaneContainer);
    } else {
        m_panel->setProperty("activePane", active ? "true" : "false");
        m_panel->style()->unpolish(m_panel);
        m_panel->style()->polish(m_panel);
    }

    if (m_panel->m_headerWidget) {
        m_panel->m_headerWidget->setActive(active);
    }
}

void ContentPaneSplitManager::updateDragOverlay(const QPoint& pos) {
    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(m_panel);
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_dragOverlayWidget->setStyleSheet("background-color: rgba(0, 122, 255, 0.25); border: 2px solid #007AFF;");
    }

    int w = m_panel->width();
    int h = m_panel->height();

    if (pos.x() > w * 0.75) {
        m_dragOverlayWidget->setGeometry(w / 2, 0, w / 2, h);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else if (pos.x() < w * 0.25) {
        m_dragOverlayWidget->setGeometry(0, 0, w / 2, h);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else if (pos.y() > h * 0.75) {
        m_dragOverlayWidget->setGeometry(0, h / 2, w, h / 2);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else if (pos.y() < h * 0.25) {
        m_dragOverlayWidget->setGeometry(0, 0, w, h / 2);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else {
        hideDragOverlay();
    }
}

void ContentPaneSplitManager::hideDragOverlay() {
    if (m_dragOverlayWidget) {
        m_dragOverlayWidget->hide();
    }
}

} // namespace QuarkMeta
