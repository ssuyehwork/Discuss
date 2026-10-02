# LibraryPanel 仅显示分类节点与内容面板渲染方案

## Overview
根据最新需求，侧边栏“库” (Library) 选项卡的树形视图 (`LibraryPanel`) 仅用于展示虚拟分类与子分类节点，**严禁**在侧边栏树节点下展开挂载物理文件或文件夹子项。
当用户在“库”树形视图中单击选中某个分类节点时，由 `LibraryPanel` 获取该分类下绑定的所有物理路径，并通过 `categoryPathsSelected(paths)` 信号触发主内容面板 (`ContentPanel::loadPaths(paths)`) 进行集中展示与管理。

## Modified Files List
- `src/ui/LibraryPanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/LibraryPanel.cpp`

```
<<<<<<< SEARCH
        // 挂载绑定的物理路径作为子项 Tree Items
        for (const QString& boundPath : rec.associatedPaths) {
            QFileInfo fi(boundPath);
            QIcon pathIcon = fi.isDir() ? UiHelper::getIcon("folder_filled", QColor("#378ADD"), 16) : ShellIconManager::getFileIcon(boundPath);
            QStandardItem* pathItem = new QStandardItem(pathIcon, fi.fileName().isEmpty() ? boundPath : fi.fileName());
            pathItem->setData(true, Qt::UserRole + 4); // true = Path node
            pathItem->setData(boundPath, Qt::UserRole + 5);
            pathItem->setData(rec.id, Qt::UserRole + 6);
            item->appendRow(pathItem);
        }
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void LibraryPanel::onCategoryClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    bool isPathItem = index.data(Qt::UserRole + 4).toBool();

    if (isPathItem) {
        QString path = index.data(Qt::UserRole + 5).toString();
        if (!path.isEmpty()) {
            QFileInfo fi(path);
            if (!fi.isDir()) {
                emit requestLocateFile(path);
            }
        }
        return;
    }

    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId > 0) {
        QStringList paths = LibraryService::instance().getCategoryPaths(nodeId);
        emit categoryPathsSelected(paths);
    }
}
=======
void LibraryPanel::onCategoryClicked(const QModelIndex& index) {
    if (!index.isValid()) return;

    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId > 0) {
        QStringList paths = LibraryService::instance().getCategoryPaths(nodeId);
        emit categoryPathsSelected(paths);
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    bool isPathItem = index.data(Qt::UserRole + 4).toBool();
    if (isPathItem) {
        int catId = index.data(Qt::UserRole + 6).toInt();
        QString boundPath = index.data(Qt::UserRole + 5).toString();

        QAction* removePathAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "从分类中移除此路径");
        connect(removePathAct, &QAction::triggered, this, [catId, boundPath]() {
            LibraryService::instance().removePathsFromCategory(catId, {boundPath});
        });

        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
        return;
    }
=======
>>>>>>> REPLACE
```

## Build & Verification Steps
1. 在终端运行 `ninja -C build` 或对应 MSVC/CMake 构建命令。
2. 启动 QuarkMeta，切换至侧边栏“库”选项卡。
3. 确认侧边栏树形视图中仅显示库分类与子分类节点，不会在分类下方展开物理文件或文件夹子节点。
4. 单击某个分类节点，验证主内容面板 (`ContentPanel`) 立即加载并渲染该分类下绑定的全部文件与文件夹。

## SSOT API Reuse & Anti-Redundancy Self-Check
- 分类路径选择信号 `categoryPathsSelected(paths)` 继续通过 `PanelMediator` 转发至 `ContentPanel::loadPaths(paths)` 入口，严格复用既有多路径内容加载与渲染 SSOT 通道。

## Header API Signature Verification
- `LibraryService::instance().getCategoryPaths(int nodeId)` 物理签名核查通过（返回 `QStringList`）。
- `LibraryPanel::categoryPathsSelected(const QStringList& paths)` 信号物理签名核查通过。

## Header Inclusion Chain & Type Completeness Check
- 无头文件包含关系变动，类型完整性校验通过。
