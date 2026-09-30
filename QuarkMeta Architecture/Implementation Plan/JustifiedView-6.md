# Implementation Plan - JustifiedView & ContentPanel Crash & Decoupling Fix (JustifiedView-6.md)

## 1. Overview
本实施方案旨在彻底解决在网格（Grid）与自适应（Justified）视图模式下，用户在元数据面板（MetaPanel）为选中项目设定颜色/评级时发生的**程序崩溃闪退（Segmentation Fault）**，以及**筛选器、元数据面板与视口卡片相互脱钩**的问题。

### 根因与修复对策：
1. **消除闪退盲区**：在 `JustifiedView` 监听到 `rowsAboutToBeRemoved`、`rowsRemoved`、`modelReset`、`layoutChanged` 时，将原本 50ms 延迟重排改为**同步立即触发 `doLayout()` 刷新 `m_geometries`**；并在 `paintEvent` 绘制循环中加入 `if (!idx.isValid()) continue;` 防御保护，杜绝因索引越界导致的 `ThumbnailDelegate` 绘图崩溃。
2. **补齐色标重绘与文件夹视图通知**：在 `DiskItemModel::updateRecordMetadata` 发送 `dataChanged` 时明确携带 `ColorRole` / `RatingRole` / `Qt::DecorationRole` 变化标志；在 `ContentPanel::updateItemMetadata` 中补齐对 `m_folderGridView` 的重绘更新。
3. **收敛选择模型**：通过精准的 `dataChanged` 广播与 `doLayout()` 收敛，驱动 `FilterProxyModel` 与选择模型（`selectionModel`）同步，消除元数据面板（MetaPanel）残留已隐藏路径的脱钩假象。

---

## 2. Modified Files List
- `src/ui/JustifiedView.cpp`
- `src/ui/ContentPanel.cpp`
- `src/ui/models/DiskItemModel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/JustifiedView.cpp`
```
<<<<<<< SEARCH
void JustifiedView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles) {
    if (roles.contains(m_aspectRatioRole)) {
        scheduleLayout();
    } else {
        viewport()->update();
    }
    QAbstractItemView::dataChanged(topLeft, bottomRight, roles);
}

void JustifiedView::rowsInserted(const QModelIndex& parent, int start, int end) {
    scheduleLayout();
    QAbstractItemView::rowsInserted(parent, start, end);
}

void JustifiedView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
}
=======
void JustifiedView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles) {
    if (roles.isEmpty() || roles.contains(m_aspectRatioRole) || roles.contains(Qt::DecorationRole) || roles.contains(ColorRole)) {
        scheduleLayout();
    }
    viewport()->update();
    QAbstractItemView::dataChanged(topLeft, bottomRight, roles);
}

void JustifiedView::rowsInserted(const QModelIndex& parent, int start, int end) {
    scheduleLayout();
    QAbstractItemView::rowsInserted(parent, start, end);
}

void JustifiedView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
    doLayout();
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        QModelIndex idx = model()->index(geo.index, 0);
        QStyleOptionViewItem option;
        initViewItemOption(&option); 
        option.rect = geo.rect;
=======
        QModelIndex idx = model()->index(geo.index, 0);
        if (!idx.isValid()) continue;

        QStyleOptionViewItem option;
        initViewItemOption(&option); 
        option.rect = geo.rect;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void JustifiedView::setModel(QAbstractItemModel* model) {
    if (this->model()) {
        disconnect(this->model(), &QAbstractItemModel::rowsRemoved, this, nullptr);
    }
    QAbstractItemView::setModel(model);
    if (model) {
        connect(model, &QAbstractItemModel::rowsRemoved, this, [this]() {
            scheduleLayout();
        });
    }
}
=======
void JustifiedView::setModel(QAbstractItemModel* model) {
    if (this->model()) {
        disconnect(this->model(), &QAbstractItemModel::rowsRemoved, this, nullptr);
        disconnect(this->model(), &QAbstractItemModel::modelReset, this, nullptr);
        disconnect(this->model(), &QAbstractItemModel::layoutChanged, this, nullptr);
    }
    QAbstractItemView::setModel(model);
    if (model) {
        connect(model, &QAbstractItemModel::rowsRemoved, this, [this]() {
            doLayout();
        });
        connect(model, &QAbstractItemModel::modelReset, this, [this]() {
            doLayout();
        });
        connect(model, &QAbstractItemModel::layoutChanged, this, [this]() {
            doLayout();
        });
    }
}
>>>>>>> REPLACE
```

### File 2: `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
void ContentPanel::updateItemMetadata(const QString& path) {
    if (m_model) m_model->updateRecordMetadata(path);
    if (m_gridView && m_gridView->viewport()) m_gridView->viewport()->update();
    if (m_treeView && m_treeView->viewport()) m_treeView->viewport()->update();
    if (m_columnView) m_columnView->updateMetadataForPath(path);
    recalculateAndEmitStats();
}
=======
void ContentPanel::updateItemMetadata(const QString& path) {
    if (m_model) m_model->updateRecordMetadata(path);
    if (m_folderGridView && m_folderGridView->viewport()) m_folderGridView->viewport()->update();
    if (m_gridView && m_gridView->viewport()) m_gridView->viewport()->update();
    if (m_treeView && m_treeView->viewport()) m_treeView->viewport()->update();
    if (m_columnView) m_columnView->updateMetadataForPath(path);
    recalculateAndEmitStats();
}
>>>>>>> REPLACE
```

### File 3: `src/ui/models/DiskItemModel.cpp`
```
<<<<<<< SEARCH
            emit dataChanged(index(i, 0), index(i, columnCount() - 1));
=======
            emit dataChanged(index(i, 0), index(i, columnCount() - 1), {Qt::DisplayRole, Qt::DecorationRole, RatingRole, ColorRole, HasThumbnailRole});
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. 编译验证：
   在 sandbox 中运行 `cmake -B build && cmake --build build --config Release` 或使用标准 Qt 构建环境确保编译通过无语法错误。
2. 逻辑验证：
   - 在网格（Grid）与自适应（Justified）模式下选择单项/多项，在筛选器勾选特定颜色（如“红色”）时，在 MetaPanel 上将项目颜色修改为“蓝色”。
   - 验证程序无任何闪退崩溃，视口卡片流畅移除并同步更新几何分布，底部状态栏统计与 MetaPanel 自动收敛更新。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 本次修改严格复用了既有的 `ContentPanel::updateItemMetadata(path)`、`JustifiedView::doLayout()` 以及 `DiskItemModel::updateRecordMetadata(path)` 通道。
- 未新增任何局部重复实现，完全遵从 Search First 与 SSOT 复用原则。

---

## 6. Header API Signature Verification
- `JustifiedView::doLayout()` 物理签名：`void doLayout();` (声明于 `JustifiedView.h`)
- `JustifiedView::scheduleLayout()` 物理签名：`void scheduleLayout();` (声明于 `JustifiedView.h`)
- `ContentPanel::updateItemMetadata(const QString& path)` 物理签名：`void updateItemMetadata(const QString& path);` (声明于 `ContentPanel.h`)
- `DiskItemModel::updateRecordMetadata(const QString& path)` 物理签名：`void updateRecordMetadata(const QString& path);` (声明于 `DiskItemModel.h`)

---

## 7. Header Inclusion Chain & Type Completeness Check
- `JustifiedView.cpp` 中已正常包含 `JustifiedView.h`、`ModelContract.h`，`ColorRole` 和 `m_aspectRatioRole` 均具备完整枚举与整型定义。
- `ContentPanel.cpp` 已包含 `JustifiedView.h`，`m_folderGridView` 为 `JustifiedView*` 类型指针，调用 `m_folderGridView->viewport()` 完整无误。
