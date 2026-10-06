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

ContentPaneSplitManager::SplitEvaluationResult ContentPaneSplitManager::evaluateSplitDrop(const QPoint& pos, const QSize& refSize) {
    SplitEvaluationResult res;
    int w = refSize.width();
    int h = refSize.height();
    if (w <= 0 || h <= 0) return res;

    if (pos.x() < w * 0.25) {
        res.isValid = true;
        res.orientation = Qt::Horizontal;
        res.insertBefore = true;
        res.highlightRect = QRect(0, 0, w / 2, h);
    } else if (pos.x() > w * 0.75) {
        res.isValid = true;
        res.orientation = Qt::Horizontal;
        res.insertBefore = false;
        res.highlightRect = QRect(w / 2, 0, w / 2, h);
    } else if (pos.y() < h * 0.25) {
        res.isValid = true;
        res.orientation = Qt::Vertical;
        res.insertBefore = true;
        res.highlightRect = QRect(0, 0, w, h / 2);
    } else if (pos.y() > h * 0.75) {
        res.isValid = true;
        res.orientation = Qt::Vertical;
        res.insertBefore = false;
        res.highlightRect = QRect(0, h / 2, w, h / 2);
    }
    return res;
}

void ContentPaneSplitManager::splitPane(Qt::Orientation orientation, const QString& secondaryPath, bool insertBefore) {
    if (!m_panel) return;

    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->splitPane(orientation, secondaryPath, insertBefore);
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
        m_primaryPaneContainer->setMinimumWidth(ContentPanel::kMinPaneWidth);
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
        ContentPanel* root = rootPane();
        ContentPanel* activeTarget = root;
        if (m_activePaneForSplit) {
            activeTarget = m_activePaneForSplit;
        }
        if (!secondaryPath.isEmpty() && activeTarget) {
            activeTarget->loadDirectory(secondaryPath);
        }
        return;
    }

    QWidget* container = new QWidget(m_paneSplitter);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    ContentPanel* newPane = new ContentPanel(container);
    container->setMinimumWidth(ContentPanel::kMinPaneWidth);
    newPane->setIsSecondaryPane(true);
    if (newPane->m_splitManager) {
        newPane->m_splitManager->m_rootPane = m_panel;
    }
    newPane->setViewMode(m_panel->currentViewMode());

    connect(newPane, &ContentPanel::closePaneRequested, m_panel, [this, newPane]() {
        closePane(newPane);
    });
    connect(newPane, &ContentPanel::directorySelected, m_panel, [this, newPane](const QString& path) {
        newPane->setActivePane(true);
        newPane->loadDirectory(path);
        emit m_panel->dualPanePathsChanged(m_panel->currentPath(), path);
        emit newPane->panelActivated(newPane);
        notifyLayoutChanged();
    });

    layout->addWidget(newPane);
    if (insertBefore) {
        m_paneSplitter->insertWidget(0, container);
        m_paneContainers.prepend(container);
        m_panes.prepend(newPane);
    } else {
        m_paneSplitter->addWidget(container);
        m_paneContainers.append(container);
        m_panes.append(newPane);
    }

    newPane->loadDirectory(!secondaryPath.isEmpty() ? secondaryPath : m_panel->currentPath());

    redistributePaneSizes();

    emit m_panel->secondaryPaneCreated(newPane);

    updateContainerMinimumWidth();
    refreshActiveIndicators();
    notifyLayoutChanged();
}

void ContentPaneSplitManager::notifyLayoutChanged() {
    if (m_blockLayoutSignals) return;

    if (!m_layoutDebounceTimer) {
        m_layoutDebounceTimer = new QTimer(this);
        m_layoutDebounceTimer->setSingleShot(true);
        m_layoutDebounceTimer->setInterval(0);
        connect(m_layoutDebounceTimer, &QTimer::timeout, this, [this]() {
            if (!m_blockLayoutSignals) {
                emit layoutChanged();
            }
        });
    }

    if (!m_layoutDebounceTimer->isActive()) {
        m_layoutDebounceTimer->start();
    }
}

void ContentPaneSplitManager::updateContainerMinimumWidth() {
    if (m_primaryPaneContainer) {
        m_primaryPaneContainer->setMinimumWidth(ContentPanel::kMinPaneWidth);
    }
    for (QWidget* container : m_paneContainers) {
        if (container) {
            container->setMinimumWidth(ContentPanel::kMinPaneWidth);
        }
    }
}

void ContentPaneSplitManager::closePane(ContentPanel* pane) {
    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->closePane(pane);
        return;
    }

    ContentPanel* root = rootPane();
    if (root && root->m_splitManager && root->m_splitManager->m_activePaneForSplit == pane) {
        root->m_splitManager->m_activePaneForSplit = nullptr;
    }

    int idx = m_panes.indexOf(pane);
    if (idx < 0) {
        // 如果请求关闭的是主窗格（rootPane / m_panel），且存在副窗格，则由首个副窗格接管主窗格内容并销毁副窗格
        if (pane == m_panel && !m_panes.isEmpty()) {
            ContentPanel* firstSecondary = m_panes.first();
            QString secondaryPath = firstSecondary->currentPath();
            m_panel->setViewMode(firstSecondary->currentViewMode());
            m_panel->loadDirectory(secondaryPath, firstSecondary->isRecursive());
            closePane(firstSecondary);
        }
        return;
    }

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

    updateContainerMinimumWidth();
    refreshActiveIndicators();
    notifyLayoutChanged();
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

    if (!m_paneSplitter || m_paneSplitter->count() == 0) {
        state.panePaths.append(m_panel->currentPath());
        state.primaryIndex = 0;
        state.activePaneIndex = 0;
        return state;
    }

    for (int i = 0; i < m_paneSplitter->count(); ++i) {
        QWidget* w = m_paneSplitter->widget(i);
        if (w == m_primaryPaneContainer) {
            state.panePaths.append(m_panel->currentPath());
            state.primaryIndex = state.panePaths.size() - 1;
            if (m_activePaneForSplit == nullptr || m_activePaneForSplit == m_panel) {
                state.activePaneIndex = state.panePaths.size() - 1;
            }
        } else {
            int cIdx = m_paneContainers.indexOf(w);
            if (cIdx >= 0 && cIdx < m_panes.size() && m_panes[cIdx]) {
                ContentPanel* pane = m_panes[cIdx];
                state.panePaths.append(pane->currentPath());
                if (m_activePaneForSplit == pane) {
                    state.activePaneIndex = state.panePaths.size() - 1;
                }
            }
        }
    }

    return state;
}

void ContentPaneSplitManager::restoreSplitState(const TabSplitState& state) {
    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->restoreSplitState(state);
        return;
    }

    m_blockLayoutSignals = true;

    // 1. 关闭现有所有副窗格
    while (!m_panes.isEmpty()) {
        closePane(m_panes.last());
    }

    if (state.panePaths.isEmpty()) {
        updateContainerMinimumWidth();
        refreshActiveIndicators();
        m_blockLayoutSignals = false;
        notifyLayoutChanged();
        return;
    }

    int primIdx = qBound(0, state.primaryIndex, state.panePaths.size() - 1);

    // 2. 还原主窗格路径
    m_panel->loadDirectory(state.panePaths[primIdx]);

    // 3. 如果快照包含分屏且路径大于 1，则按屏幕物理顺序动态构建副窗格
    if (state.isSplit && state.panePaths.size() > 1) {
        m_splitOrientation = state.orientation;

        for (int i = primIdx - 1; i >= 0; --i) {
            splitPane(state.orientation, state.panePaths[i], true);
        }

        for (int i = primIdx + 1; i < state.panePaths.size(); ++i) {
            splitPane(state.orientation, state.panePaths[i], false);
        }
    }

    // 4. 恢复激活窗格
    int activeIdx = qBound(0, state.activePaneIndex, state.panePaths.size() - 1);
    if (activeIdx == primIdx) {
        m_panel->setActivePane(true);
    } else {
        int secIdx = (activeIdx < primIdx) ? activeIdx : (activeIdx - 1);
        if (secIdx >= 0 && secIdx < m_panes.size()) {
            m_panes[secIdx]->setActivePane(true);
        } else {
            m_panel->setActivePane(true);
        }
    }

    updateContainerMinimumWidth();
    refreshActiveIndicators();

    m_blockLayoutSignals = false;
    notifyLayoutChanged();
}

void ContentPaneSplitManager::setActivePane(bool active) {
    ContentPanel* root = rootPane();
    if (root && root->m_splitManager) {
        if (active) {
            root->m_splitManager->m_activePaneForSplit = m_panel;
        }
        root->m_splitManager->refreshActiveIndicators();
    }
}

void ContentPaneSplitManager::refreshActiveIndicators() {
    if (rootPane() != m_panel) {
        rootPane()->m_splitManager->refreshActiveIndicators();
        return;
    }

    auto setPaneActiveProperty = [](QWidget* widget, bool shown) {
        if (!widget) return;
        QString valStr = shown ? "true" : "false";
        if (widget->property("activePane").toString() != valStr) {
            widget->setProperty("activePane", valStr);
            if (widget->style()) {
                widget->style()->unpolish(widget);
                widget->style()->polish(widget);
            }
        }
    };

    bool isSplit = isSplitMode();

    // 根窗格激活判定
    bool rootActive = (m_activePaneForSplit == nullptr || m_activePaneForSplit == m_panel);
    bool rootShown = isSplit && rootActive;

    if (m_primaryPaneContainer) {
        setPaneActiveProperty(m_primaryPaneContainer, rootShown);
    }
    setPaneActiveProperty(m_panel, false);
    if (m_panel) {
        m_panel->updateHeaderTitle();
        if (m_panel->m_headerWidget) {
            m_panel->m_headerWidget->setActive(rootShown);
        }
    }

    // 副窗格激活判定
    for (ContentPanel* pane : m_panes) {
        if (!pane) continue;
        bool paneActive = (m_activePaneForSplit == pane);
        bool paneShown = isSplit && paneActive;

        pane->updateHeaderTitle();
        setPaneActiveProperty(pane, paneShown);
        if (pane->m_headerWidget) {
            pane->m_headerWidget->setActive(paneShown);
        }
    }
}

void ContentPaneSplitManager::updateDragOverlay(const QPoint& pos) {
    SplitEvaluationResult eval = evaluateSplitDrop(pos, m_panel->size());
    if (!eval.isValid) {
        hideDragOverlay();
        return;
    }

    if (!m_dragOverlayWidget) {
        m_dragOverlayWidget = new QWidget(m_panel);
        m_dragOverlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_dragOverlayWidget->setStyleSheet("background-color: rgba(0, 122, 255, 0.25); border: 2px solid #007AFF;");
    }

    m_dragOverlayWidget->setGeometry(eval.highlightRect);
    m_dragOverlayWidget->show();
    m_dragOverlayWidget->raise();
}

void ContentPaneSplitManager::hideDragOverlay() {
    if (m_dragOverlayWidget) {
        m_dragOverlayWidget->hide();
    }
}

} // namespace QuarkMeta
