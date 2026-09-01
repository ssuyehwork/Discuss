# Implementation Plan: ThemeManager-5

## 1. Overview
精确恢复五大面板容器的卡片间 5px 实体物理切缝机制。
通过为容器（`#SidebarContainer`, `#FavoriteContainer`, `#EditorContainer`, `#MetadataContainer`, `#FilterContainer`）恢复 `margin: 0px 2px`，结合 1px 的 `QSplitter` 手柄（2px 物理 Margin + 1px 手柄 + 2px 物理 Margin = 5px 物理分割缝），同时保持 `Dual-mode version` 的背景色（`#1E1E1E`）与边框线（`1px solid #333333`）。

## 2. Modified Files List
- `src/ui/ThemeManager.cpp`
- `src/ui/MainWindow.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ThemeManager.cpp`
<<<<<<< SEARCH
        /* 🚀【全局唯一样式真理源】：100% 还原 Dual-mode version 视觉契约 */
        QSplitter {
            background: transparent;
            border: none;
        }
        QSplitter::handle:horizontal {
            background-color: #1E1E1E;
            width: 5px;
        }
        QSplitter::handle:horizontal:hover {
            background-color: #2A2A2A;
        }

        /* 2. 五大实体栏区卡片底板与 1px 精细边界 (Dual-mode 纯正还原) */
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E;
            border: 1px solid #333333;
            border-radius: 0px;
            margin: 0px;
            padding: 0px;
        }
=======
        /* 🚀【全局唯一样式真理源】：卡片 5px 实体物理切缝 + Dual-mode 深色风格 */
        QSplitter {
            background: transparent;
            border: none;
        }
        QSplitter::handle:horizontal {
            background-color: #1E1E1E;
            width: 1px;
        }
        QSplitter::handle:horizontal:hover {
            background-color: #378ADD;
        }

        /* 2. 五大实体栏区卡片底板（通过左右各 2px margin + 1px 分隔手柄构建 5px 物理缝隙） */
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E;
            border: 1px solid #333333;
            border-radius: 0px;
            margin: 0px 2px;
            padding: 0px;
        }
>>>>>>> REPLACE

### `src/ui/MainWindow.cpp`
<<<<<<< SEARCH
    // 100% 还原 Dual-mode version 原版 QSplitter 样式
    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(5); 
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; }"
        "QSplitter::handle:horizontal { background-color: #1E1E1E; width: 5px; }"
        "QSplitter::handle:horizontal:hover { background-color: %1; }"
        "#SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {"
        "  background-color: #1E1E1E;"
        "  border: 1px solid #333333;"
        "  margin: 0px;"
        "}"
=======
    // 5px 实体物理缝隙 (2px margin + 1px handle + 2px margin) + Dual-mode 深色样式
    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(1); 
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; }"
        "QSplitter::handle:horizontal { background-color: #1E1E1E; width: 1px; }"
        "QSplitter::handle:horizontal:hover { background-color: %1; }"
        "#SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {"
        "  background-color: #1E1E1E;"
        "  border: 1px solid #333333;"
        "  margin: 0px 2px;"
        "}"
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. 读取并核对修改后的 `ThemeManager.cpp` 与 `MainWindow.cpp`，确认 margin: 0px 2px 与 setHandleWidth(1) 正确结合。
2. 确认总体两卡片卡槽之间形成精确的 5px 物理分割高质感暗色缝隙。
