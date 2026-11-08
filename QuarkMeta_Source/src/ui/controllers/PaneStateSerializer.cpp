#include "PaneStateSerializer.h"
#include "../ContentPanel.h"
#include "../ContentHeaderWidget.h"
#include <QVBoxLayout>
#include <QSplitter>

namespace QuarkMeta {

PaneTreeNode PaneStateSerializer::exportNodeRecursive(ContentPaneSplitManager::PaneNode* node) {
    PaneTreeNode tn;
    if (!node) return tn;

    tn.isSplitter = node->isSplitter;
    tn.orientation = node->orientation;
    tn.isPrimary = node->isPrimary;
    if (!node->isSplitter && node->panel) {
        tn.path = node->panel->currentPath();
    }

    if (node->isSplitter) {
        for (ContentPaneSplitManager::PaneNode* child : node->children) {
            tn.children.append(exportNodeRecursive(child));
        }
    }

    return tn;
}

ContentPaneSplitManager::PaneNode* PaneStateSerializer::importNodeRecursive(
    const PaneTreeNode& treeNode, 
    QWidget* parentWidget, 
    ContentPanel* hostPanel, 
    QFrame*& primaryContainer) 
{
    if (!hostPanel) return nullptr;

    auto* node = new ContentPaneSplitManager::PaneNode();
    node->isSplitter = treeNode.isSplitter;
    node->orientation = treeNode.orientation;
    node->isPrimary = treeNode.isPrimary;

    if (node->isSplitter) {
        QSplitter* splitter = new QSplitter(node->orientation, parentWidget);
        splitter->setHandleWidth(5);
        splitter->setChildrenCollapsible(false);
        node->splitter = splitter;

        for (const auto& childTN : treeNode.children) {
            ContentPaneSplitManager::PaneNode* childNode = importNodeRecursive(childTN, splitter, hostPanel, primaryContainer);
            if (childNode) {
                childNode->parent = node;
                node->children.append(childNode);
                if (childNode->container) {
                    splitter->addWidget(childNode->container);
                } else if (childNode->splitter) {
                    splitter->addWidget(childNode->splitter);
                }
            }
        }
    } else {
        if (node->isPrimary) {
            node->panel = hostPanel;
            if (!primaryContainer) {
                primaryContainer = new QFrame(parentWidget);
                primaryContainer->setObjectName("EditorContainer");
                primaryContainer->setAttribute(Qt::WA_StyledBackground, true);
                primaryContainer->setMinimumWidth(ContentPanel::kMinPaneWidth);
                QVBoxLayout* primLayout = new QVBoxLayout(primaryContainer);
                primLayout->setContentsMargins(0, 0, 0, 0);
                primLayout->setSpacing(0);

                if (hostPanel->headerWidget()) {
                    hostPanel->mainLayout()->removeWidget(hostPanel->headerWidget());
                    primLayout->addWidget(hostPanel->headerWidget());
                }
                if (hostPanel->viewStack()) {
                    hostPanel->mainLayout()->removeWidget(hostPanel->viewStack());
                    primLayout->addWidget(hostPanel->viewStack(), 1);
                }
            } else {
                primaryContainer->setParent(parentWidget);
            }
            primaryContainer->show();
            node->container = primaryContainer;
            if (!treeNode.path.isEmpty()) {
                hostPanel->loadDirectory(treeNode.path);
            }
        } else {
            QWidget* container = new QWidget(parentWidget);
            QVBoxLayout* layout = new QVBoxLayout(container);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);

            ContentPanel* newPane = new ContentPanel(container);
            container->setMinimumWidth(ContentPanel::kMinPaneWidth);
            newPane->setIsSecondaryPane(true);
            if (newPane->splitManager()) {
                newPane->splitManager()->setRootPane(hostPanel);
            }
            newPane->setViewMode(hostPanel->currentViewMode());

            QObject::connect(newPane, &ContentPanel::closePaneRequested, hostPanel, [hostPanel, newPane]() {
                hostPanel->closePane(newPane);
            });
            QObject::connect(newPane, &ContentPanel::directorySelected, hostPanel, [hostPanel, newPane](const QString& path) {
                newPane->setActivePane(true);
                newPane->loadDirectory(path);
                emit hostPanel->dualPanePathsChanged(hostPanel->currentPath(), path);
                emit newPane->panelActivated(newPane);
                if (hostPanel->splitManager()) {
                    hostPanel->splitManager()->notifyLayoutChanged();
                }
            });

            layout->addWidget(newPane);
            node->container = container;
            node->panel = newPane;

            if (!treeNode.path.isEmpty()) {
                newPane->loadDirectory(treeNode.path);
            }
            emit hostPanel->secondaryPaneCreated(newPane);
        }
    }

    return node;
}

} // namespace QuarkMeta
