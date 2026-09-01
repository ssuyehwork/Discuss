# Implementation Plan: ThemeManager-3

## 1. Overview
采用容器 `margin` 与 `QSplitter` 手柄样式的解耦设计，解决多栏布局中栏区之间 5 像素物理视觉间距不明显的问题。
通过为五大面板容器（`#SidebarContainer`, `#FavoriteContainer`, `#EditorContainer`, `#MetadataContainer`, `#FilterContainer`）显式配置 `margin: 0px 2px`，结合 1px 手柄或 1px border，建立真实的 5 像素实体缝隙分割，强化栏区独立的“卡片物理切割感”。

## 2. Modified Files List
- `src/ui/ThemeManager.cpp`
- `src/ui/MainWindow.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ThemeManager.cpp`
<<<<<<< SEARCH
        /* 2. 五大实体栏区卡片底板与 1px 精细边界 */
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E;
            border: 1px solid #2A2A2A;
            border-radius: 0px;
            margin: 0px;
            padding: 0px;
        }
=======
        /* 2. 五大实体栏区卡片底板与 1px 精细边界（通过 margin 建立 5px 实体物理切缝） */
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E;
            border: 1px solid #2A2A2A;
            border-radius: 0px;
            margin: 0px 2px;
            padding: 0px;
        }
>>>>>>> REPLACE

### `src/ui/MainWindow.cpp`
<<<<<<< SEARCH
        "#SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {"
        "  background-color: #1E1E1E;"
        "  border: 1px solid #2A2A2A;"
        "}"
=======
        "#SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {"
        "  background-color: #1E1E1E;"
        "  border: 1px solid #2A2A2A;"
        "  margin: 0px 2px;"
        "}"
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. 检查代码改动，确保符合 API 冻结原则。
2. 运行应用验证界面多栏间距，确认栏区之间展现出清晰独立的 5 像素物理间距黑槽。
