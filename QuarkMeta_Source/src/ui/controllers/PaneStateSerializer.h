#pragma once

#include "ContentPaneSplitManager.h"
#include "../TabBarWidget.h"

namespace QuarkMeta {

class PaneStateSerializer {
public:
    static PaneTreeNode exportNodeRecursive(ContentPaneSplitManager::PaneNode* node);

    static ContentPaneSplitManager::PaneNode* importNodeRecursive(
        const PaneTreeNode& treeNode, 
        QWidget* parentWidget, 
        ContentPanel* hostPanel, 
        QFrame*& primaryContainer);
};

} // namespace QuarkMeta
