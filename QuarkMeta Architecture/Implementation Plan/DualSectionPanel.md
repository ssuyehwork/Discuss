现在，所有的源代码真相与调用链路已经全部闭环。

看到 `DualSectionPanel.cpp` 第 152~177 行的那段代码，可以说将“凭直觉脑补与拙劣打补丁”展现得淋漓尽致：
```cpp
// 💥 拙劣补丁典型范本：因为 indexAt 算不准，竟然写了个 10 次的循环步进 +10, +20... 去猜索引！
for (int offset = 10; offset <= 100 && !topIdx.isValid(); offset += 10)
    topIdx = view->indexAt(QPoint(clampedTopX + offset, clampedTopY + offset));
```
不仅如此，`view->indexAt(pos)` 在 Qt 标准规范中要求传入的是**子视图 `view->viewport()` 内部视口坐标**，代码却直接传了 `view` 外框坐标；更把 X 坐标硬绑在 `view->width() - 1`（边缘留白/外边距处），导致大部分时间根本击不中任何 Item，只能 fallback 到写死的 `top + 50`，缩略图自然不是全量阻塞就是视口脱节！

遵循《AGENTS.md》铁律，我为您编制了结构完整、100% 物理签名核验的正式实施方案。方案将收敛存放于 `QuarkMeta Architecture/Implementation Plan/ContentPanel.md`。

---

# 实施方案：ContentPanel.md

## 1. Overview（概述与解决的问题）
本方案旨在彻底解决以下三大长期困扰的系统级交互与性能缺陷：
1. **彻底恢复缩略图秒显与视口按需流式加载**：
   - 根治 `DualSectionPanel` 中视口坐标未投影到 `view->viewport()` 导致 `indexAt` 持续失效的几何缺陷；
   - 彻底废除 `for (int offset = 10 ...)` 盲猜补丁，通过真实视口对角线矩形交集精确提取 `[topIdx - 4, btmIdx + 4]` 可见行；
   - 补齐 `ContentPanel::refreshVisibleThumbnails()` 中对 `ColumnView` 缩略图巡检的长期遗漏。
2. **根治 `Ctrl+Shift+N` 全视图模式下无法进入行内重命名的问题**：
   - 铲除 `ContentFileOpsHandler::createNewItem` 中私自调用 `diskModel->addItemRecord` 及正反斜杠不匹配的死循环代码；
   - 回归 SSOT 通道：物理创建成功后下达 `setPendingSelectName(finalName, true)` 并调用唯一的 `refreshAll()`；
   - 修正 `ContentPanel::setPendingSelectName` 内部直接用 `+ "/"` 拼接导致与底层反斜杠不匹配的路径真理源 Bug；
   - 让 `ColumnView` 同样闭环响应 `m_isPendingEdit` 状态机。
3. **消除右击画布留白导致多选被强行清空的恶性 Bug**：
   - 修复 `SectionedScrollCanvas::mousePressEvent` 无差别执行 `clearSelection()` 的缺陷，严格限定仅在 `Qt::LeftButton` 单击留白时取消选区，绝对保护用户右键批量操作的选区集合。

---

## 2. Modified Files List（影响文件清单）
1. `src/ui/DualSectionPanel.cpp`
2. `src/ui/SectionedScrollCanvas.cpp`
3. `src/ui/ContentPanel.cpp`
4. `src/ui/controllers/ContentFileOpsHandler.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 修改 `src/ui/DualSectionPanel.cpp`
彻底重写 `refreshVisibleThumbnails`，将外层视口对角线坐标准确投影至子视图真实的 `viewport()` 坐标系中，采用几何交集精确提取可见 Item。

<<<<<<< SEARCH
void DualSectionPanel::refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport) {
    if (!model || !hostViewport || CoreController::isShuttingDown()) return;

    QRect vpRect = hostViewport->rect();
    QSet<int> visibleRows;

    auto scanView = [&](QAbstractItemView* view, FilterProxyModel* proxy) {
        if (!view || !view->isVisible() || !proxy || proxy->rowCount() == 0) return;

        QPoint topPoint = view->mapFromGlobal(hostViewport->mapToGlobal(vpRect.topLeft()));
        QPoint btmPoint = view->mapFromGlobal(hostViewport->mapToGlobal(vpRect.bottomRight()));

        if (topPoint.y() >= view->height() || btmPoint.y() <= 0) return;

        int clampedTopX = qBound(0, topPoint.x(), view->width() - 1);
        int clampedTopY = qBound(0, topPoint.y(), view->height() - 1);

        int clampedBtmX = qBound(0, btmPoint.x(), view->width() - 1);
        int clampedBtmY = qBound(0, btmPoint.y(), view->height() - 1);

        QModelIndex topIdx = view->indexAt(QPoint(clampedTopX, clampedTopY));
        if (!topIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !topIdx.isValid(); offset += 10)
                topIdx = view->indexAt(QPoint(clampedTopX + offset, clampedTopY + offset));
        }
        QModelIndex btmIdx = view->indexAt(QPoint(clampedBtmX, clampedBtmY));
        if (!btmIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !btmIdx.isValid(); offset += 10)
                btmIdx = view->indexAt(QPoint(clampedBtmX - offset, clampedBtmY - offset));
        }

        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 10) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 10) : qMin(proxy->rowCount() - 1, top + 50);

        for (int r = top; r <= bottom; ++r) {
            QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
    };

    scanView(m_folderView, m_folderProxyModel);
    scanView(m_fileView, m_fileProxyModel);

    if (!visibleRows.isEmpty()) {
        qDebug() << "[THUMB_TRACE] refreshVisibleThumbnails calculated visible source rows:" << visibleRows.values();
        model->loadThumbnailsForRows(visibleRows.values());
    } else {
        qDebug() << "[THUMB_TRACE] refreshVisibleThumbnails found ZERO visible rows in viewport.";
    }
}
=======
void DualSectionPanel::refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport) {
    if (!model || !hostViewport || CoreController::isShuttingDown()) return;

    QRect hostVpRect = hostViewport->rect();
    QSet<int> visibleRows;

    auto scanView = [&](QAbstractItemView* view, FilterProxyModel* proxy) {
        if (!view || !view->isVisible() || !view->viewport() || !proxy || proxy->rowCount() == 0) return;

        QWidget* subVp = view->viewport();
        // 将外层 QScrollArea 视口矩形投影到子视图真实的 viewport 坐标系
        QPoint topPoint = subVp->mapFromGlobal(hostViewport->mapToGlobal(hostVpRect.topLeft()));
        QPoint btmPoint = subVp->mapFromGlobal(hostViewport->mapToGlobal(hostVpRect.bottomRight()));

        // 完全在可视区域之外时直接跳过
        if (topPoint.y() >= subVp->height() || btmPoint.y() <= 0) return;

        // 限制在子视图可视区域内，并避开极端边缘 padding
        int sampleTopY = qBound(0, topPoint.y(), subVp->height() - 1);
        int sampleBtmY = qBound(0, btmPoint.y(), subVp->height() - 1);
        int sampleLeftX = qBound(0, qMax(topPoint.x(), 12), subVp->width() - 1);
        int sampleRightX = qBound(0, qMin(btmPoint.x(), subVp->width() - 12), subVp->width() - 1);

        QModelIndex topIdx = view->indexAt(QPoint(sampleLeftX, sampleTopY));
        QModelIndex btmIdx = view->indexAt(QPoint(sampleRightX, sampleBtmY));

        // 若直接采样点位于卡片间隙，向中心采样
        if (!topIdx.isValid()) {
            topIdx = view->indexAt(QPoint(subVp->width() / 2, sampleTopY));
        }
        if (!btmIdx.isValid()) {
            btmIdx = view->indexAt(QPoint(subVp->width() / 2, sampleBtmY));
        }

        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 4) : proxy->rowCount() - 1;

        for (int r = top; r <= bottom; ++r) {
            QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
    };

    scanView(m_folderView, m_folderProxyModel);
    scanView(m_fileView, m_fileProxyModel);

    if (!visibleRows.isEmpty()) {
        model->loadThumbnailsForRows(visibleRows.values());
    }
}
>>>>>>> REPLACE

---

### 3.2 修改 `src/ui/SectionedScrollCanvas.cpp`
保护右键操作：仅当鼠标**左键**点击留白处时才执行清空选区，彻底杜绝右键破坏多选。

<<<<<<< SEARCH
void SectionedScrollCanvas::mousePressEvent(QMouseEvent* event) {
    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();
    if (folderView && folderView->selectionModel()) folderView->selectionModel()->clearSelection();
    if (fileView && fileView->selectionModel()) fileView->selectionModel()->clearSelection();
    QScrollArea::mousePressEvent(event);
}
=======
void SectionedScrollCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        auto* folderView = m_panel->folderView();
        auto* fileView = m_panel->fileView();
        if (folderView && folderView->selectionModel()) folderView->selectionModel()->clearSelection();
        if (fileView && fileView->selectionModel()) fileView->selectionModel()->clearSelection();
    }
    QScrollArea::mousePressEvent(event);
}
>>>>>>> REPLACE

---

### 3.3 修改 `src/ui/ContentPanel.cpp`
1. 补齐 `refreshVisibleThumbnails()` 对 `ColumnView` 缩略图的加载；
2. 修复 `setPendingSelectName` 中的路径拼接格式（消除正反斜杠不一致）；
3. 在 `restoreSelections()` 中为 `ColumnView` 同样闭环传递 `m_isPendingEdit`。

<<<<<<< SEARCH
void ContentPanel::refreshVisibleThumbnails() {
    if (m_currentViewMode == ListView && m_listCanvas) {
        m_listCanvas->refreshVisibleThumbnails(m_model);
    } else if ((m_currentViewMode == GridView || m_currentViewMode == JustifiedViewMode) && m_gridCanvas) {
        m_gridCanvas->refreshVisibleThumbnails(m_model);
    }
}
=======
void ContentPanel::refreshVisibleThumbnails() {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            QList<QAbstractItemView*> views;
            if (m_columnView->activePane()->folderListView()) views << m_columnView->activePane()->folderListView();
            if (m_columnView->activePane()->listView()) views << m_columnView->activePane()->listView();
            QSet<int> visibleRows;
            for (auto* view : views) {
                if (!view || !view->viewport() || !view->model()) continue;
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
            if (!visibleRows.isEmpty() && m_columnView->activePane()->model()) {
                m_columnView->activePane()->model()->loadThumbnailsForRows(visibleRows.values());
            }
        }
        return;
    }

    if (m_currentViewMode == ListView && m_listCanvas) {
        m_listCanvas->refreshVisibleThumbnails(m_model);
    } else if ((m_currentViewMode == GridView || m_currentViewMode == JustifiedViewMode) && m_gridCanvas) {
        m_gridCanvas->refreshVisibleThumbnails(m_model);
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPanel::restoreSelections() {
    if (m_selectionState.selectedPaths.isEmpty() || m_isRestoringSelections) return;

    m_isRestoringSelections = true;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->rightmostPane()) {
            m_columnView->rightmostPane()->setPendingSelectPaths(m_selectionState.selectedPaths);
        }
        m_isRestoringSelections = false;
        return;
    }
=======
void ContentPanel::restoreSelections() {
    if (m_selectionState.selectedPaths.isEmpty() || m_isRestoringSelections) return;

    m_isRestoringSelections = true;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->rightmostPane()) {
            m_columnView->rightmostPane()->setPendingSelectPaths(m_selectionState.selectedPaths);
            if (m_isPendingEdit) {
                m_isPendingEdit = false;
                QString targetPath = m_selectionState.focusedPath;
                QPointer<ColumnViewWidget> weakCol = m_columnView;
                QTimer::singleShot(0, this, [weakCol, targetPath]() {
                    if (weakCol && weakCol->rightmostPane()) {
                        QAbstractItemView* view = weakCol->rightmostPane()->listView();
                        if (view && view->currentIndex().isValid()) {
                            view->setFocus();
                            view->edit(view->currentIndex());
                        }
                    }
                });
            }
        }
        m_isRestoringSelections = false;
        return;
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPanel::setPendingSelectName(const QString& name, bool edit) {
    m_selectionState.selectedPaths.clear();
    if (!name.isEmpty()) {
        QString fullPath = m_currentPath + "/" + name;
        m_selectionState.selectedPaths.insert(fullPath);
        m_selectionState.focusedPath = fullPath;
    }
    m_isPendingEdit = edit;
}
=======
void ContentPanel::setPendingSelectName(const QString& name, bool edit) {
    m_selectionState.selectedPaths.clear();
    if (!name.isEmpty()) {
        QString fullPath = QDir::cleanPath(QDir(m_currentPath).filePath(name));
        m_selectionState.selectedPaths.insert(fullPath);
        m_selectionState.focusedPath = fullPath;
    }
    m_isPendingEdit = edit;
}
>>>>>>> REPLACE

---

### 3.4 修改 `src/ui/controllers/ContentFileOpsHandler.cpp`
彻底铲除手写插入数据与遍历代理模型的脏代码，全线回归统一的 `setPendingSelectName` + `refreshAll()` 状态机。

<<<<<<< SEARCH
        ItemRecord rec = ItemRecord::create(fullPath);

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, rec, fullPath]() {
            if (!weakPanel) return;

            if (weakPanel->currentViewMode() == ContentPanel::ColumnView && weakPanel->columnView()) {
                weakPanel->columnView()->refreshActiveColumn();
                return;
            }

            if (DiskItemModel* diskModel = qobject_cast<DiskItemModel*>(weakPanel->model())) {
                diskModel->addItemRecord(rec);
            }

            weakPanel->applySort();
            weakPanel->applyFilters();
            weakPanel->recalculateAndEmitStats();

            QSortFilterProxyModel* proxy = weakPanel->getActiveProxyModel();
            QAbstractItemView* view = weakPanel->activeItemView();
            if (proxy && view) {
                for (int i = 0; i < proxy->rowCount(); ++i) {
                    QModelIndex proxyIdx = proxy->index(i, 0);
                    if (proxyIdx.data(PathRole).toString() == fullPath) {
                        view->setFocus();
                        view->scrollTo(proxyIdx);
                        view->setCurrentIndex(proxyIdx);
                        view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                        view->edit(proxyIdx);
                        break;
                    }
                }
            }
        });
=======
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, finalName]() {
            if (!weakPanel) return;
            weakPanel->setPendingSelectName(finalName, true);
            weakPanel->refreshAll();
        });
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. **编译构建**：
   使用 CMake 重新构建 `QuarkMeta`，确保 0 警告、0 错误编译通过：
   ```bash
   cmake --build build --config RelWithDebInfo
   ```
2. **缩略图秒显验证**：
   - 打开包含上百张图片的大文件夹，快速滑动网格视图，验证卡片是否跟随视口秒级渲染，不再出现大面积白板卡顿；
   - 切换到列表视图（ListView）与分栏视图（ColumnView），确认缩略图均能稳定秒显。
3. **`Ctrl+Shift+N` 行内重命名验证**：
   - 分别在网格视图、列表视图、分栏视图下，按下 `Ctrl+Shift+N`；
   - 验证新建文件夹瞬间被自动选中，光标直接进入重命名输入框，敲击回车或失焦后成功命名。
4. **多选右键保护验证**：
   - 框选 5 个项目后，在画布留白处单击右键，验证已有 5 个项目的选区完好保留，不再被意外清空。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用自查）
- [x] **复用官方刷新通道**：文件物理落地后，统一调用 `weakPanel->refreshAll()`，彻底销毁了在业务层手动塞 `addItemRecord` 和手动调 `applySort` 的死代码；
- [x] **复用声明式状态机**：复用既有的 `setPendingSelectName(finalName, true)`，生命周期由 `restoreSelections()` 统一托管；
- [x] **彻底清除特例代码**：消除了 `ContentFileOpsHandler` 中针对 ColumnView 的不规范 `return`，所有模式行为严格一致。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）
| 调用成员函数 | 所属物理类 | 所在头文件 | 签名匹配核实 |
| :--- | :--- | :--- | :--- |
| `setPendingSelectName(const QString&, bool)` | `ContentPanel` | `ContentPanel.h` | 物理签名一致 (默认参数 `edit = false`) |
| `refreshAll()` | `ContentPanel` | `ContentPanel.h` | 物理签名一致 (`void refreshAll()`) |
| `activePane()` | `ColumnViewWidget` | `ColumnViewWidget.h` | 物理签名一致 (`ColumnViewPane* activePane() const`) |
| `loadThumbnailsForRows(const QList<int>&)` | `ItemModelBase` | `ItemModelBase.h` | 物理签名一致 (`virtual void loadThumbnailsForRows(...) = 0`) |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件完整性检查表）
- `src/ui/DualSectionPanel.cpp`：已完整包含 `QAbstractItemView`、`QWidget`、`QPoint`、`QRect`、`QSet`；
- `src/ui/ContentPanel.cpp`：已完整包含 `ColumnViewWidget.h`、`ColumnViewPane.h`、`QTimer`；
- `src/ui/controllers/ContentFileOpsHandler.cpp`：已完整包含 `ContentPanel.h`、`QMetaObject`。无任何不完整类型。

---