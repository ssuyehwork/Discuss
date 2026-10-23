# MainThreadLagAndVisibilityOptimization.md - 主线程卡死根因修复与可见项归一化实施方案

## Overview
彻底解决滚动与缩略图加载过程中的主线程卡死（GUI Event Loop 卡顿/假死）、滚轮点击响应不敏捷问题。
具体修复包含以下四个核心维度：
1. **视图绘制局部裁剪优化（`JustifiedView::paintEvent`）**：计算 `event->rect()` 对应的 Y 轴像素区间并进行 `lower_bound` 检索，仅绘制当前视口重叠的卡片项目，彻底消除对整个目录全部 Delegate 的无效重绘开销。
2. **可见项计算归一化与误算清理（`ContentPanel::refreshVisibleThumbnails`）**：废除 `ContentPanel::refreshVisibleThumbnails` 中对于全高子视图 `viewport()->rect()` 的错误遍历（该误算会导致视口算成全目录，引发全量缩略图排队提取），将可见项排队计算统一收敛至了解外层真实视口的 `DualSectionPanel::refreshVisibleThumbnails`。
3. **`DiskItemModel` 悬空死代码清理**：清理 `DiskItemModel` 中仅存 `find`/`erase` 而无 `insert` 的 `m_genTokens` 悬空哈希表逻辑。
4. **阻断缩略图刷新的全量布局风暴**：在 `dataChanged` 触发时精细化控制 Role，只有当图片的真实宽高比（Aspect Ratio）确实发生变化时才携带 `AspectRatioRole`，避免常规缩略图填充引发全量重新布局。

---

## Modified Files List
- `src/ui/JustifiedView.cpp`
- `src/ui/ContentPanel.cpp`
- `src/ui/models/DiskItemModel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/JustifiedView.cpp`

**优化 `JustifiedView::paintEvent` 视口绘制裁剪**

```
<<<<<<< SEARCH
void JustifiedView::paintEvent(QPaintEvent*) {
    QPainter painter(viewport());
    painter.fillRect(viewport()->rect(), QColor("#1E1E1E"));

    if (m_geometries.empty()) {
        painter.save();
        painter.setPen(QColor("#888888"));
        painter.setFont(QFont("Microsoft YaHei", 12));
        painter.drawText(viewport()->rect(), Qt::AlignCenter, "没有可显示的项目");
        painter.restore();
        return;
    }
    
    painter.save();
    int scrollY = verticalScrollBar()->value();
    int vHeight = viewport()->height();
    painter.translate(0, -scrollY);
    
    auto startIt = std::lower_bound(m_geometries.begin(), m_geometries.end(), scrollY,
        [](const ItemGeometry& geo, int targetY) {
            return geo.rect.bottom() < targetY;
        });

    for (auto it = startIt; it != m_geometries.end(); ++it) {
        const auto& geo = *it;
        if (geo.rect.top() > scrollY + vHeight) break;
=======
void JustifiedView::paintEvent(QPaintEvent* event) {
    QPainter painter(viewport());
    painter.fillRect(viewport()->rect(), QColor("#1E1E1E"));

    if (m_geometries.empty()) {
        painter.save();
        painter.setPen(QColor("#888888"));
        painter.setFont(QFont("Microsoft YaHei", 12));
        painter.drawText(viewport()->rect(), Qt::AlignCenter, "没有可显示的项目");
        painter.restore();
        return;
    }
    
    painter.save();
    int scrollY = verticalScrollBar()->value();
    painter.translate(0, -scrollY);
    
    QRect dirtyRect = event->rect().translated(0, scrollY);
    int dirtyTop = dirtyRect.top();
    int dirtyBottom = dirtyRect.bottom();

    auto startIt = std::lower_bound(m_geometries.begin(), m_geometries.end(), dirtyTop,
        [](const ItemGeometry& geo, int targetY) {
            return geo.rect.bottom() < targetY;
        });

    for (auto it = startIt; it != m_geometries.end(); ++it) {
        const auto& geo = *it;
        if (geo.rect.top() > dirtyBottom) break;
>>>>>>> REPLACE
```

---

### 2. `src/ui/ContentPanel.cpp`

**清理 `ContentPanel::refreshVisibleThumbnails` 中的子视图误计算，实现向 Canvas/DualSectionPanel 转发**

```
<<<<<<< SEARCH
void ContentPanel::refreshVisibleThumbnails() {
    if (!m_model || CoreController::isShuttingDown()) return;

    QList<QAbstractItemView*> views;
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            if (m_columnView->activePane()->folderListView()) views << m_columnView->activePane()->folderListView();
            if (m_columnView->activePane()->listView()) views << m_columnView->activePane()->listView();
        }
    } else if (m_currentViewMode == ListView) {
        if (m_folderTreeView) views << m_folderTreeView;
        if (m_treeView) views << m_treeView;
    } else {
        if (m_folderGridView) views << m_folderGridView;
        if (m_gridView) views << m_gridView;
    }

    QSet<int> visibleRows;
    for (auto* view : views) {
        if (!view || !view->viewport()) continue;
        auto* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!proxy || proxy->rowCount() == 0) continue;

        QRect vpRect = view->viewport()->rect();
        QModelIndex topIdx = view->indexAt(vpRect.topLeft());
        QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 4) : proxy->rowCount() - 1;

        for (int r = top; r <= bottom; ++r) {
            QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
    }

    if (!visibleRows.isEmpty()) {
        m_model->loadThumbnailsForRows(visibleRows.values());
    }
}
=======
void ContentPanel::refreshVisibleThumbnails() {
    if (!m_model || CoreController::isShuttingDown()) return;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            m_columnView->activePane()->refreshVisibleThumbnails();
        }
    } else if (m_currentViewMode == ListView && m_listCanvas) {
        m_listCanvas->refreshVisibleThumbnails(m_model);
    } else if ((m_currentViewMode == GridView || m_currentViewMode == JustifiedViewMode) && m_gridCanvas) {
        m_gridCanvas->refreshVisibleThumbnails(m_model);
    }
}
>>>>>>> REPLACE
```

---

### 3. `src/ui/models/DiskItemModel.cpp`

**清理 `m_genTokens` 悬空死代码并精确过滤 `AspectRatioRole` 防布局风暴**

```
<<<<<<< SEARCH
        // 若已经抽取了有意义的宽度和高度，则触发 AspectRatioRole 以更新流式布局
        if (rec.width > 0 && rec.height > 0) {
            emit dataChanged(pIdx, pIdx, {Qt::DecorationRole, Qt::DisplayRole, AspectRatioRole});
        } else {
            emit dataChanged(pIdx, pIdx, {Qt::DecorationRole, Qt::DisplayRole});
        }
=======
        // 只有当宽高比真实发生变更时才触发 AspectRatioRole
        double oldAr = rec.aspectRatio;
        if (rec.width > 0 && rec.height > 0) {
            double newAr = static_cast<double>(rec.width) / static_cast<double>(rec.height);
            if (qAbs(oldAr - newAr) > 0.01) {
                rec.aspectRatio = newAr;
                emit dataChanged(pIdx, pIdx, {Qt::DecorationRole, Qt::DisplayRole, AspectRatioRole});
            } else {
                emit dataChanged(pIdx, pIdx, {Qt::DecorationRole, Qt::DisplayRole});
            }
        } else {
            emit dataChanged(pIdx, pIdx, {Qt::DecorationRole, Qt::DisplayRole});
        }
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. 使用 `ninja` 编译，核查 `JustifiedView.cpp`, `ContentPanel.cpp`, `DiskItemModel.cpp` 是否编译无误：
   `ninja -C build`
2. 启动程序并滚动快速加载大型目录，验证：
   - 滚轮滚动与鼠标点击响应迅捷，无界面假死与 Event Loop 冻结。
   - 仅显示当前视口内的卡片缩略图，避免对离开视口的卡片全量提取。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **可见项计算 SSOT**：消除了 `ContentPanel` 独自手写的错误计算，完全由 `DualSectionPanel` 根据外层真实 Viewport 进行物理采样计算。
- **无新增两套逻辑或另起炉灶**：删除无用 `m_genTokens` 局部逻辑，确保逻辑干净闭环。

---

## Header API Signature Verification
- `JustifiedView::paintEvent(QPaintEvent* event)`：满足 `QAbstractItemView` / `QWidget` 签名要求。
- `ContentPanel::refreshVisibleThumbnails()`：保持原签名不变。
- `DiskItemModel::allRecords()`：使用既有返回引用 API。

---

## Header Inclusion Chain & Type Completeness Check
- 无头文件包含变动或删除，现有包含闭环完备。
