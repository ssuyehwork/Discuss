# Implementation Plan: ThemeManager-4

## 1. Overview
严格按照原版 `Dual-mode version` 的视觉契约与调色板，完全还原全应用的深色背景与边框颜色。
彻底清除此前误引入的 `#141414` 深黑槽背景、`#2A2A2A` 边框及 `margin: 0px 2px`，100% 还原为 `Dual-mode version` 的原版色彩：
- 主背景与面板背景：`#1E1E1E` (`BackgroundDeep`)
- 容器标题栏背景：`#252526` (`BackgroundHeader`)
- 全局物理分隔边框：`1px solid #333333` (`BorderColor`)
- Splitter 分隔手柄背景：`#1E1E1E`，悬停高亮背景：`#2A2A2A`
- 容器外边距：`margin: 0px`

## 2. Modified Files List
- `src/ui/ThemeManager.cpp`
- `src/ui/MainWindow.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ThemeManager.cpp`
<<<<<<< SEARCH
        /* 🚀【全局唯一样式真理源】：物理 5 像素分栏凹槽与悬停高亮坚固固化 */
        QSplitter {
            background-color: #141414;
            border: none;
        }
        QSplitter::handle:horizontal {
            background-color: #141414;
            width: 5px;
        }
        QSplitter::handle:horizontal:hover {
            background-color: #378ADD;
        }

        /* 2. 五大实体栏区卡片底板与 1px 精细边界（通过 margin 建立 5px 实体物理切缝） */
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E;
            border: 1px solid #2A2A2A;
            border-radius: 0px;
            margin: 0px 2px;
            padding: 0px;
        }
=======
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
>>>>>>> REPLACE

### `src/ui/MainWindow.cpp`
<<<<<<< SEARCH
    // 🚀【物理 5 像素深黑槽 + 8px 暗黑细滚动条】：彻底消除继承阻断，杜绝系统白底滚动条
    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(5); 
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background-color: #141414; border: none; }"
        "QSplitter::handle:horizontal { background-color: #141414; width: 5px; }"
        "QSplitter::handle:horizontal:hover { background-color: %1; }"
        "#SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {"
        "  background-color: #1E1E1E;"
        "  border: 1px solid #2A2A2A;"
        "  margin: 0px 2px;"
        "}"
=======
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
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. 检查代码改动，确保符合 API 冻结原则。
2. 运行应用验证整体 UI 配色，确认全界面背景、边框、Header 与 QSplitter 完全还原成 `Dual-mode version` 的视觉效果。
