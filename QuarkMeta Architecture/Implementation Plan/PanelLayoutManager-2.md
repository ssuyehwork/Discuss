# QuarkMeta 分割线归一化与 QTreeView 视口拉满实施方案 (PanelLayoutManager-2.md)

## 1. Overview
本方案旨在彻底消除 2 个视觉交互缺陷：
1. **多重分割线打架与粗线黑缝**：去除面板 QSS 自身左右边框（`border: none;`），并将 `m_mainSplitter` handle 宽度由 5px 设为 `1px`（`background-color: <BorderColor>`，悬停泛蓝），使把手本身充当全系统唯一合法的 1px 细分割线，彻底消除杂线与黑缝。
2. **QTreeView 100px 列宽导致文字过早打断截断**：在 `FavoritePanel.cpp` 和 `NavPanel.cpp` 中开启内部 `DropTreeView` 视口首列自适应拉满（`m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch)`），使文字一直平滑延伸至右边缘。

---

## 2. Modified Files List
1. **修改** `src/ui/MainWindow.cpp`
2. **修改** `src/ui/PanelLayoutManager.h`
3. **修改** `src/ui/FavoritePanel.cpp`
4. **修改** `src/ui/NavPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/MainWindow.cpp` 面板边框去除与 1px 纯净把手注入

```cpp
<<<<<<< SEARCH
            #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
                background-color: %1; border: 1px solid %2; border-radius: 0px;
            }
=======
            #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
                background-color: %1; border: none; border-radius: 0px;
            }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(5); 
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; }"
        "QSplitter::handle { background-color: %1; border-left: 1px solid %2; width: 5px; }"
        "QSplitter::handle:hover { background-color: %3; }"
    ).arg(qssColor(BackgroundDeep)).arg(qssColor(BorderColor)).arg(qssColor(BackgroundHover)));
=======
    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(1); 
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter { background: transparent; border: none; }"
        "QSplitter::handle { background-color: %1; width: 1px; }"
        "QSplitter::handle:hover { background-color: %2; }"
    ).arg(qssColor(BorderColor)).arg(qssColor(PrimaryBlue)));
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/FavoritePanel.cpp` 与 `src/ui/NavPanel.cpp` 全宽 Stretch 开启

```cpp
// FavoritePanel.cpp
if (m_favoriteView->header()) {
    m_favoriteView->header()->setStretchLastSection(true);
    m_favoriteView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

// NavPanel.cpp
if (m_treeView->header()) {
    m_treeView->header()->setStretchLastSection(true);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
}
```

---

## 4. Build & Verification Steps

1. **1px 纯净分割线验证**：
   - 验证各个面板间仅有一条优雅的 1px 细线（悬停时亮蓝色），无粗线或黑缝。
2. **文本全宽展开验证**：
   - 检查收藏夹和目录导航中的条目，确认文字顺畅延展至右侧边缘。
