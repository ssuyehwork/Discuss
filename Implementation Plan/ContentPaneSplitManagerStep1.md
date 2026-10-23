# ContentPaneSplitManager Step 1 Implementation Plan

## 1. Overview
本方案为【窗格树架构重构】的第一步实施方案，主要完成以下核心内容：
1. **窗格树底层数据结构（Pane Tree Infrastructure）**：建立 `PaneNode` 与 `PaneTreeNode` 结构，支持叶节点（`ContentPanel` 窗格容器）与分割节点（`QSplitter`）的任意深度嵌套。
2. **规范化展平机制（Tree Normalization）**：实现 `normalizeTree`，自动将仅剩 1 个子节点的分割节点用子节点替换，并将同向嵌套的子分割节点平铺展开合并到父节点中。
3. **统一深度优先遍历顺序（Depth-First Traversal Guarantee）**：锁定深度优先遍历顺序（水平方向从左到右，垂直方向从上到下），所有 `panes()` 列表、导出路径、标签页名称拼接等全量走统一遍历通道。
4. **状态导出与恢复兼容（State Export & Legacy Import Protocol）**：支持窗格树序列化 `exportSplitState` 与 `restoreSplitState`；旧配置（仅有 `panePaths` 和 `orientation` 字段）自动转换为单层分割树，保证 100% 向上兼容。
5. **尺寸分配与硬性字面量清理**：新窗格加入或移动时，被分割的窗格将其一半尺寸让给新窗格；严禁使用数字字面量，全工程统一引用 `ContentPanel::kMaxPanes`、`ContentPanel::kMinPaneWidth` (230) 与 `ContentPanel::kMinPaneHeight` (230)。

---

## 2. Modified Files List
- `src/ui/controllers/ContentPaneSplitManager.h`
- `src/ui/controllers/ContentPaneSplitManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/controllers/ContentPaneSplitManager.h`

```
<<<<<<< SEARCH
    // 窗格树节点数据结构
    struct PaneNode {
        bool isSplitter = false;
        Qt::Orientation orientation = Qt::Horizontal;
        QPointer<QSplitter> splitter = nullptr;
        QPointer<QWidget> container = nullptr;
        ContentPanel* panel = nullptr;
        bool isPrimary = false;
        QList<PaneNode*> children;
        PaneNode* parent = nullptr;
    };

    PaneNode* m_rootNode = nullptr;
=======
public:
    // 窗格树节点数据结构
    struct PaneNode {
        bool isSplitter = false;
        Qt::Orientation orientation = Qt::Horizontal;
        QPointer<QSplitter> splitter = nullptr;
        QPointer<QWidget> container = nullptr;
        ContentPanel* panel = nullptr;
        bool isPrimary = false;
        QList<PaneNode*> children;
        PaneNode* parent = nullptr;
    };

    PaneNode* rootNode() const { return m_rootNode; }

private:
    PaneNode* m_rootNode = nullptr;
>>>>>>> REPLACE
```

### File 2: `src/ui/controllers/ContentPaneSplitManager.cpp`

```
<<<<<<< SEARCH
void ContentPaneSplitManager::normalizeTree(PaneNode* node) {
    if (!node) return;

    if (node->isSplitter) {
        for (int i = node->children.size() - 1; i >= 0; --i) {
            normalizeTree(node->children[i]);
        }

        // 规范化 1：展平相同方向的子 Splitter 节点，并重建 QSplitter 挂载关系
        QList<PaneNode*> newChildren;
        for (PaneNode* child : node->children) {
            if (child->isSplitter && child->orientation == node->orientation) {
                if (node->splitter && child->splitter) {
                    int childIdx = node->splitter->indexOf(child->splitter);
                    for (int g = 0; g < child->children.size(); ++g) {
                        PaneNode* grandChild = child->children[g];
                        grandChild->parent = node;
                        newChildren.append(grandChild);

                        QWidget* gWidget = grandChild->isSplitter ? static_cast<QWidget*>(grandChild->splitter.data()) : grandChild->container.data();
                        if (gWidget) {
                            if (childIdx >= 0) {
                                node->splitter->insertWidget(childIdx + g, gWidget);
                            } else {
                                node->splitter->addWidget(gWidget);
                            }
                        }
                    }
                    child->children.clear();
                    child->splitter->deleteLater();
                    delete child;
                }
            } else {
                newChildren.append(child);
            }
        }
        node->children = newChildren;

        // 规范化 2：如果 Splitter 只剩 1 个子节点，替换为该子节点，并将组件正确重挂至上级
        if (node->children.size() == 1) {
            PaneNode* soleChild = node->children.first();
            soleChild->parent = node->parent;

            QWidget* soleWidget = soleChild->isSplitter ? static_cast<QWidget*>(soleChild->splitter.data()) : soleChild->container.data();

            if (node->parent) {
                int idx = node->parent->children.indexOf(node);
                if (idx >= 0) {
                    node->parent->children[idx] = soleChild;
                    if (node->parent->splitter && soleWidget) {
                        node->parent->splitter->replaceWidget(idx, soleWidget);
                    }
                }
            } else {
                m_rootNode = soleChild;
                if (m_panel && m_panel->m_mainLayout && soleWidget) {
                    if (node->splitter) {
                        m_panel->m_mainLayout->removeWidget(node->splitter);
                    }
                    m_panel->m_mainLayout->addWidget(soleWidget, 1);
                    soleWidget->show();
                }
            }

            node->children.clear();
            if (node->splitter) {
                node->splitter->deleteLater();
            }
            delete node;
        }
    }
}
=======
void ContentPaneSplitManager::normalizeTree(PaneNode* node) {
    if (!node) return;

    if (node->isSplitter) {
        for (int i = node->children.size() - 1; i >= 0; --i) {
            normalizeTree(node->children[i]);
        }

        // 规范化 1：展平相同方向的子 Splitter 节点，并重建 QSplitter 挂载关系
        QList<PaneNode*> newChildren;
        for (PaneNode* child : node->children) {
            if (child && child->isSplitter && child->orientation == node->orientation) {
                if (node->splitter && child->splitter) {
                    int childIdx = node->splitter->indexOf(child->splitter);
                    for (int g = 0; g < child->children.size(); ++g) {
                        PaneNode* grandChild = child->children[g];
                        if (!grandChild) continue;
                        grandChild->parent = node;
                        newChildren.append(grandChild);

                        QWidget* gWidget = grandChild->isSplitter ? static_cast<QWidget*>(grandChild->splitter.data()) : grandChild->container.data();
                        if (gWidget) {
                            if (childIdx >= 0) {
                                node->splitter->insertWidget(childIdx + g, gWidget);
                            } else {
                                node->splitter->addWidget(gWidget);
                            }
                        }
                    }
                    child->children.clear();
                    child->splitter->deleteLater();
                    delete child;
                }
            } else if (child) {
                newChildren.append(child);
            }
        }
        node->children = newChildren;

        // 规范化 2：如果 Splitter 只剩 1 个子节点，替换为该子节点，并将组件正确重挂至上级
        if (node->children.size() == 1) {
            PaneNode* soleChild = node->children.first();
            if (soleChild) {
                soleChild->parent = node->parent;

                QWidget* soleWidget = soleChild->isSplitter ? static_cast<QWidget*>(soleChild->splitter.data()) : soleChild->container.data();

                if (node->parent) {
                    int idx = node->parent->children.indexOf(node);
                    if (idx >= 0) {
                        node->parent->children[idx] = soleChild;
                        if (node->parent->splitter && soleWidget) {
                            node->parent->splitter->replaceWidget(idx, soleWidget);
                        }
                    }
                } else {
                    m_rootNode = soleChild;
                    if (m_panel && m_panel->m_mainLayout && soleWidget) {
                        if (node->splitter) {
                            m_panel->m_mainLayout->removeWidget(node->splitter);
                        }
                        m_panel->m_mainLayout->addWidget(soleWidget, 1);
                        soleWidget->show();
                    }
                }

                node->children.clear();
                if (node->splitter) {
                    node->splitter->deleteLater();
                }
                delete node;
            }
        }
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. **CMake 构建与编译**：
   在终端运行项目构建命令，验证核心重构组件编译通过：
   ```bash
   cmake --build build
   ```
2. **结构规范化校验**：
   在单窗格创建多个分屏，验证连续水平/垂直分屏后，`normalizeTree` 正常展平同向子节点，关闭部分窗格后无僵尸 `QSplitter` 残留。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- [x] **复用统一遍历接口**：`panes()` 100% 走唯一的深度优先遍历函数 `collectPanesDepthFirst`。
- [x] **严禁字面量**：使用 `ContentPanel::kMaxPanes`、`ContentPanel::kMinPaneWidth` (230) 与 `ContentPanel::kMinPaneHeight` (230)，绝无硬编码字面量。

---

## 6. Header API Signature Verification
- `ContentPaneSplitManager::panes() const` ➔ `QList<ContentPanel*>`（匹配 `ContentPaneSplitManager.h`）
- `ContentPaneSplitManager::exportSplitState() const` ➔ `TabSplitState`（匹配 `ContentPaneSplitManager.h`）
- `ContentPaneSplitManager::restoreSplitState(const TabSplitState&)` ➔ `void`（匹配 `ContentPaneSplitManager.h`）

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "ContentPaneSplitManager.h"` 包含 `<QSplitter>`、`<QList>`、`<QPointer>` 及 `"../TabBarWidget.h"`，类型声明完整。
