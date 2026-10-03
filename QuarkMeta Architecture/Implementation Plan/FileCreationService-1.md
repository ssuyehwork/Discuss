# Implementation Plan - FileCreationService-1.md

## 1. 概述 (Overview)
本方案旨在为 `FileCreationService` 及 `ContentPanel` 中的新建文件/文件夹与行内重命名触发流程添加详尽的诊断调试日志 (`[CREATE_ITEM_DIAG]`)，以排查并定位创建文件夹后未成功调起行内编辑框的原因。

### 诊断日志覆盖点
1. **`FileCreationService::createNewItem`**：
   - 记录新建类型（文件夹或文件）及生成的物理绝对路径。
   - 记录磁盘物理目录/文件创建结果。
   - 记录同步追加至 `DiskItemModel` 的结果与筛选器刷新动作。
   - 记录调用 `selectAndEditPath` 的触发点。

2. **`ContentPanel::selectAndEditPath`**：
   - 记录当前视图模式（网格视图、列表视图、分栏视图）。
   - 记录当前激活视图 (`activeItemView()`) 与当前激活代理模型 (`getActiveProxyModel()`) 的指针及行数。
   - 记录在代理模型中按路径匹配查找目标行索引的全过程（匹配成功行号或未找到匹配项）。
   - 记录设置视图焦点 (`setFocus()`)、滚动至指定行 (`scrollTo`)、选中该行及调用 `view->edit(proxyIdx)` 的执行细节。

3. **`ContentPanel::activeItemView`**：
   - 记录当前获取焦点或选中的具体视图（文件夹视图还是文件视图）。

## 2. 修改文件列表 (Modified Files List)
- `src/core/FileCreationService.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

## 3. 逐行变更细节 (Detailed Changes)

### `src/core/FileCreationService.cpp`
```diff
<<<<<<< SEARCH
    // 1. 同步将新项目追加至 Model，彻底消除全盘异步扫描的时序脱节与状态遗失
    ItemRecord newRec = ItemRecord::create(fullPath);
    if (panel->model()) {
        panel->model()->appendRecord(newRec);
    }
    panel->applyFilters();

    // 2. 强锁定焦点并即时触发 Delegate 代理重命名编辑框 (100% 稳固)
    panel->selectAndEditPath(fullPath);
    return true;
=======
    qDebug() << "[CREATE_ITEM_DIAG] 新建类型:" << type << "| 目标路径:" << fullPath << "| 物理创建成功";

    // 1. 同步将新项目追加至 Model，彻底消除全盘异步扫描的时序脱节与状态遗失
    ItemRecord newRec = ItemRecord::create(fullPath);
    if (panel->model()) {
        panel->model()->appendRecord(newRec);
        qDebug() << "[CREATE_ITEM_DIAG] 已向 Model 追加新记录，当前记录总数:" << panel->model()->allRecords().size();
    }
    panel->applyFilters();

    // 2. 强锁定焦点并即时触发 Delegate 代理重命名编辑框 (100% 稳固)
    qDebug() << "[CREATE_ITEM_DIAG] 准备调用 selectAndEditPath...";
    panel->selectAndEditPath(fullPath);
    return true;
>>>>>>> REPLACE
```

### `src/ui/ContentPanel.cpp`
```diff
<<<<<<< SEARCH
void ContentPanel::selectAndEditPath(const QString& path) {
    QSortFilterProxyModel* proxy = getActiveProxyModel();
    QAbstractItemView* view = activeItemView();
    if (!proxy || !view || path.isEmpty()) return;

    for (int i = 0; i < proxy->rowCount(); ++i) {
        QModelIndex proxyIdx = proxy->index(i, 0);
        if (proxyIdx.data(PathRole).toString() == path) {
            view->setFocus();
            view->scrollTo(proxyIdx);
            view->setCurrentIndex(proxyIdx);
            if (view->selectionModel()) {
                view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            view->edit(proxyIdx);
            break;
        }
    }
}
=======
void ContentPanel::selectAndEditPath(const QString& path) {
    QSortFilterProxyModel* proxy = getActiveProxyModel();
    QAbstractItemView* view = activeItemView();

    qDebug() << "[CREATE_ITEM_DIAG] selectAndEditPath 触发 | 目标路径:" << path
             << "| 当前视图模式:" << m_currentViewMode
             << "| view 指针:" << view
             << "| proxy 指针:" << proxy
             << "| proxy 行数:" << (proxy ? proxy->rowCount() : -1);

    if (!proxy || !view || path.isEmpty()) {
        qWarning() << "[CREATE_ITEM_DIAG] 异常退出: proxy 或 view 为空或路径为空！";
        return;
    }

    bool found = false;
    for (int i = 0; i < proxy->rowCount(); ++i) {
        QModelIndex proxyIdx = proxy->index(i, 0);
        QString idxPath = proxyIdx.data(PathRole).toString();
        if (QString::compare(QDir::cleanPath(idxPath), QDir::cleanPath(path), Qt::CaseInsensitive) == 0) {
            found = true;
            qDebug() << "[CREATE_ITEM_DIAG] 在 proxy 行" << i << "找到匹配项目，准备激活焦点与触发编辑...";

            view->setFocus();
            view->scrollTo(proxyIdx);
            view->setCurrentIndex(proxyIdx);
            if (view->selectionModel()) {
                view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }

            bool isEditable = (proxyIdx.flags() & Qt::ItemIsEditable);
            qDebug() << "[CREATE_ITEM_DIAG] 目标索引是否包含 Qt::ItemIsEditable 标志:" << isEditable;

            view->edit(proxyIdx);
            qDebug() << "[CREATE_ITEM_DIAG] view->edit(proxyIdx) 已调用完毕";
            break;
        }
    }

    if (!found) {
        qWarning() << "[CREATE_ITEM_DIAG] 警告: 未在当前 view 的 proxy 中找到路径:" << path;
    }
}
>>>>>>> REPLACE
```

## 4. 构建与验证步骤 (Build & Verification Steps)
1. 编写与应用诊断日志代码。
2. 运行程序并触发新建文件夹（如快捷键 `Ctrl+Shift+N` 或右键菜单新建）。
3. 检查控制台/调试输出窗口中 `[CREATE_ITEM_DIAG]` 日志：
   - 确认新建文件夹项目追加后，`activeItemView()` 返回的是文件夹视图还是文件视图。
   - 确认 `proxy` 中是否查找到新建文件夹的索引行号。
   - 确认 `view->edit(proxyIdx)` 是否被成功执行。

## 5. 架构与 API 复用自查
- 复用现有的 `qDebug()` 与 `qWarning()` 日志输出宏，无冗余代码与全局污染。
