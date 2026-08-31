# QuarkMeta 架构重构方案：ThemeManager 与 MainWindow 边距完全复原

## 一、 Overview
彻底撤销给 `bodyWrapper` 误加的外边距与各面板容器上多余的卡片边框/圆角，彻底恢复 0 边距平整无缝隙的面板风格。保持主分割器把手 `5px` 标准物理宽度（`background-color: #141414`）。

## 二、 Modified Files List
- `src/ui/MainWindow.cpp`
- `src/ui/ThemeManager.cpp`

## 三、 Detailed Line-by-Line Changes

### 1. `src/ui/MainWindow.cpp`
```git
<<<<<<< SEARCH
    m_bodyLayout->setContentsMargins(5, 5, 5, 5); 
    m_bodyLayout->setSpacing(5);

    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(5); 
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; }"
        "QSplitter::handle { background: transparent; width: 5px; }"
        "QSplitter::handle:hover { background-color: %1; }"
    ).arg(qssColor(PrimaryBlue)));
=======
    m_bodyLayout->setContentsMargins(0, 0, 0, 0); 
    m_bodyLayout->setSpacing(0);

    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(5); 
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; spacing: 0px; }"
        "QSplitter::handle { background-color: #141414; width: 5px; margin: 0px; padding: 0px; }"
        "QSplitter::handle:hover { background-color: %1; }"
    ).arg(qssColor(PrimaryBlue)));
>>>>>>> REPLACE
```

### 2. `src/ui/ThemeManager.cpp`
```git
<<<<<<< SEARCH
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E;
            border: 1px solid #333333;
            border-radius: 4px;
        }
        #ContainerHeader {
            background-color: #252526;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            border-bottom: 1px solid #333333;
        }
=======
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E;
            border: none;
            border-radius: 0px;
            margin: 0px;
            padding: 0px;
        }
        #ContainerHeader {
            background-color: #252526;
            border-radius: 0px;
            border-bottom: 1px solid #333333;
        }
>>>>>>> REPLACE
```

## 四、 Build & Verification Steps
1. 确认 `src/ui/MainWindow.cpp` body 边距为 0。
2. 确认 `src/ui/ThemeManager.cpp` 面板容器 `border: none; border-radius: 0px;`。
3. 确认 `m_mainSplitter` handle 宽度 5px，背景色 `#141414`。
