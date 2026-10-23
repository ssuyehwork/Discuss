# VisibleThumbnailScanRefactoring.md - 缩略图可见区扫描触发机制与坐标换算重构实施方案

## Overview
彻底解决启动后网络/网格视图最后一排卡片显示占位符、需手动刷新才加载缩略图的问题。
重构聚焦于三点：
1. **事件驱动替代延时猜测**：
   - `JustifiedView::doLayout` 布局完成后发射 `layoutFinished` 信号。
   - `SectionedScrollCanvas` 将 `layoutFinished`、`resizeEvent`、`verticalScrollBar::valueChanged` 以及代理模型的 `modelReset`/`layoutChanged` 四个来源统一接入唯一防抖入口 `m_scrollThumbTimer`。
   - 彻底废除靠固定延时（如 `m_visibleTimer` 60ms 猜测）触发视口扫描的做法。
2. **基于内容坐标的零 `mapFromGlobal` 求交**：
   - 在 `DualSectionPanel::refreshVisibleThumbnails` 中，利用外层 `QScrollArea` 的滚动值 `scrollY` 与视口高度 `vHeight`，减去子视图在面板内的相对 Y 偏移 `view->y()`，计算出子视图内容坐标系下的可见区间 `[top, bottom]`。
   - `JustifiedView` 提供只读接口 `rowsInRange(int top, int bottom)`，内部通过 `m_geometries` 二分查找极其高效地返回包含在区间内的行号集合。
3. **几何未就绪保护与清晰日志**：
   - 当 `JustifiedView` 的 `m_layoutDirty` 为 `true` 或 `m_geometries.size() < model()->rowCount()` 时，判定几何未就绪，直接返回不提交任何行，等待 `layoutFinished` 自动触发下一次扫描。
   - 若某行或视图被跳过，记录明确的日志（含行号、几何状态与跳过原因）。

---

## Modified Files List
- `src/ui/JustifiedView.h`
- `src/ui/JustifiedView.cpp`
- `src/ui/SectionedScrollCanvas.h`
- `src/ui/SectionedScrollCanvas.cpp`
- `src/ui/DualSectionPanel.cpp`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/JustifiedView.h`

**新增 `layoutFinished` 信号与 `rowsInRange` 方法**

```
<<<<<<< SEARCH
signals:
    void totalHeightChanged(int height);

public:
    QRect visualRect(const QModelIndex& index) const override;
=======
signals:
    void totalHeightChanged(int height);
    void layoutFinished();

public:
    QList<int> rowsInRange(int top, int bottom) const;
    bool isLayoutReady() const;
    QRect visualRect(const QModelIndex& index) const override;
>>>>>>> REPLACE
```

---

### 2. `src/ui/JustifiedView.cpp`

**实现 `isLayoutReady()`，`rowsInRange()` 与 `layoutFinished` 信号发射**

```
<<<<<<< SEARCH
    int oldHeight = m_totalHeight;
    m_totalHeight = currentY;
    updateGeometries();
    viewport()->update();

    if (oldHeight != m_totalHeight) {
        emit totalHeightChanged(m_totalHeight);
    }
}
=======
    int oldHeight = m_totalHeight;
    m_totalHeight = currentY;
    updateGeometries();
    viewport()->update();

    if (oldHeight != m_totalHeight) {
        emit totalHeightChanged(m_totalHeight);
    }
    emit layoutFinished();
}

bool JustifiedView::isLayoutReady() const {
    if (m_layoutDirty) return false;
    if (!model()) return true;
    return static_cast<int>(m_geometries.size()) >= model()->rowCount();
}

QList<int> JustifiedView::rowsInRange(int top, int bottom) const {
    QList<int> rows;
    if (!isLayoutReady() || m_geometries.empty()) return rows;

    auto startIt = std::lower_bound(m_geometries.begin(), m_geometries.end(), top,
        [](const ItemGeometry& geo, int targetY) {
            return geo.rect.bottom() < targetY;
        });

    for (auto it = startIt; it != m_geometries.end(); ++it) {
        const auto& geo = *it;
        if (geo.rect.top() > bottom) break;
        if (!geo.isHeader) {
            rows.append(geo.index);
        }
    }
    return rows;
}
>>>>>>> REPLACE
```

---

### 3. `src/ui/SectionedScrollCanvas.h` & `src/ui/SectionedScrollCanvas.cpp`

**统一事件驱动触发通道接入**

```
<<<<<<< SEARCH
    connect(m_scrollThumbTimer, &QTimer::timeout, this, [this]() {
        if (m_folderProxyModel && m_folderProxyModel->sourceModel()) {
            if (auto* diskModel = qobject_cast<ItemModelBase*>(m_folderProxyModel->sourceModel())) {
                m_panel->refreshVisibleThumbnails(diskModel, viewport());
            }
        }
    });

    if (verticalScrollBar()) {
        connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            if (m_scrollThumbTimer) {
                m_scrollThumbTimer->start();
            }
        });
    }
=======
    connect(m_scrollThumbTimer, &QTimer::timeout, this, [this]() {
        if (m_folderProxyModel && m_folderProxyModel->sourceModel()) {
            if (auto* diskModel = qobject_cast<ItemModelBase*>(m_folderProxyModel->sourceModel())) {
                m_panel->refreshVisibleThumbnails(diskModel, viewport());
            }
        }
    });

    if (verticalScrollBar()) {
        connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            triggerVisibleScan();
        });
    }
>>>>>>> REPLACE
```

并在 `setupConnections()` 中将 `layoutFinished` 及 proxy模型信号绑定至 `triggerVisibleScan()`：

```
<<<<<<< SEARCH
    if (m_type == CanvasType::Grid) {
        if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
            connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int) {
                updateSectionCounts();
            });
        }
        if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
            connect(jv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_fileProxyModel && m_fileProxyModel->rowCount() > 0) {
                    m_panel->fileView()->setFixedHeight(qMax(height, m_panel->fileViewMinHeight()));
                }
            });
        }
    }

    auto onModelChanged = [this]() { updateSectionCounts(); };
    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);
=======
    if (m_type == CanvasType::Grid) {
        if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
            connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int) {
                updateSectionCounts();
            });
            connect(fjv, &JustifiedView::layoutFinished, this, &SectionedScrollCanvas::triggerVisibleScan);
        }
        if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
            connect(jv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_fileProxyModel && m_fileProxyModel->rowCount() > 0) {
                    m_panel->fileView()->setFixedHeight(qMax(height, m_panel->fileViewMinHeight()));
                }
            });
            connect(jv, &JustifiedView::layoutFinished, this, &SectionedScrollCanvas::triggerVisibleScan);
        }
    }

    auto onModelChanged = [this]() {
        updateSectionCounts();
        triggerVisibleScan();
    };
    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);
>>>>>>> REPLACE
```

并在 `resizeEvent` 中添加 `triggerVisibleScan()`：

```
<<<<<<< SEARCH
void SectionedScrollCanvas::resizeEvent(QResizeEvent* event) {
    QScrollArea::resizeEvent(event);
    updateSectionCounts();
}
=======
void SectionedScrollCanvas::triggerVisibleScan() {
    if (m_scrollThumbTimer) {
        m_scrollThumbTimer->start();
    }
}

void SectionedScrollCanvas::resizeEvent(QResizeEvent* event) {
    QScrollArea::resizeEvent(event);
    updateSectionCounts();
    triggerVisibleScan();
}
>>>>>>> REPLACE
```

---

### 4. `src/ui/DualSectionPanel.cpp`

**基于内容坐标换算 `[top, bottom]` 与零 `mapFromGlobal` 求交采样**

```
<<<<<<< SEARCH
void DualSectionPanel::refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport) {
    if (!model || !hostViewport || CoreController::isShuttingDown()) return;

    QRect hostVpRect = hostViewport->rect();
    QSet<int> visibleRows;

    auto scanView = [&](QAbstractItemView* view, FilterProxyModel* proxy) {
        if (!view || !view->isVisible() || !view->viewport() || !proxy || proxy->rowCount() == 0) return;

        QWidget* subVp = view->viewport();
        QString viewTag = (view == m_folderView) ? "FolderView" : "FileView";

        // 将外层 QScrollArea 视口矩形投影到子视图真实的 viewport 坐标系
        QPoint topPoint = subVp->mapFromGlobal(hostViewport->mapToGlobal(hostVpRect.topLeft()));
        QPoint btmPoint = subVp->mapFromGlobal(hostViewport->mapToGlobal(hostVpRect.bottomRight()));

        // 完全在可视区域之外时直接跳过
        if (topPoint.y() >= subVp->height() || btmPoint.y() <= 0) {
            return;
        }

        int visibleCount = 0;
        int firstVisible = -1;
        int lastVisible = -1;

        int rowCount = proxy->rowCount();
        for (int r = 0; r < rowCount; ++r) {
            QModelIndex pIdx = proxy->index(r, 0);
            QRect rRect = view->visualRect(pIdx);

            if (!rRect.isValid() || rRect.isEmpty()) continue;

            // 卡片完全在可视视口上方，跳过找下一张
            if (rRect.bottom() < topPoint.y()) continue;

            // 卡片完全在可视视口下方
            if (rRect.top() > btmPoint.y()) {
                // 如果已经找到了可见项，且当前项已经彻底超出下边缘一定缓冲，退出循环
                if (lastVisible != -1 && r > lastVisible + 20) {
                    break;
                }
                continue;
            }

            // 几何相交：当前卡片在屏幕上可见
            if (firstVisible == -1) firstVisible = r;
            lastVisible = r;
            visibleCount++;

            QModelIndex srcIdx = proxy->mapToSource(pIdx);
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }

        qDebug().noquote() << QString("[THUMB_TRACE] [%1] Geometry scan: visible rows [%2, %3], total visible: %4")
            .arg(viewTag).arg(firstVisible).arg(lastVisible).arg(visibleCount);
    };

    scanView(m_folderView, m_folderProxyModel);
    scanView(m_fileView, m_fileProxyModel);

    if (!visibleRows.isEmpty()) {
        qDebug() << "[THUMB_TRACE] DualSectionPanel::refreshVisibleThumbnails - Submitting" << visibleRows.size() << "rows to loadThumbnailsForRows.";
        model->loadThumbnailsForRows(visibleRows.values());
    } else {
        qDebug() << "[THUMB_TRACE] DualSectionPanel::refreshVisibleThumbnails - No visible rows found in viewport sampling.";
    }
}
=======
void DualSectionPanel::refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport) {
    if (!model || !hostViewport || CoreController::isShuttingDown()) return;

    auto* scrollArea = qobject_cast<QScrollArea*>(hostViewport->parent());
    int scrollY = (scrollArea && scrollArea->verticalScrollBar()) ? scrollArea->verticalScrollBar()->value() : 0;
    int vpHeight = hostViewport->height();

    QSet<int> visibleRows;

    auto scanView = [&](QAbstractItemView* view, FilterProxyModel* proxy) {
        if (!view || !view->isVisible() || !proxy || proxy->rowCount() == 0) return;

        QString viewTag = (view == m_folderView) ? "FolderView" : "FileView";
        auto* jv = qobject_cast<JustifiedView*>(view);

        if (jv) {
            if (!jv->isLayoutReady()) {
                qDebug().noquote() << QString("[THUMB_TRACE] [%1] Geometry not ready (dirty or unpopulated), scan deferred.").arg(viewTag);
                return;
            }

            int topInContent = scrollY - view->y();
            int bottomInContent = topInContent + vpHeight;

            QList<int> proxyRows = jv->rowsInRange(topInContent, bottomInContent);
            if (proxyRows.isEmpty()) {
                qDebug().noquote() << QString("[THUMB_TRACE] [%1] Geometry scan: no intersecting rows in range [%2, %3].")
                    .arg(viewTag).arg(topInContent).arg(bottomInContent);
                return;
            }

            for (int r : proxyRows) {
                QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
                if (srcIdx.isValid()) {
                    visibleRows.insert(srcIdx.row());
                } else {
                    qDebug().noquote() << QString("[THUMB_TRACE] [%1] Skip row %2: invalid source index.").arg(viewTag).arg(r);
                }
            }

            qDebug().noquote() << QString("[THUMB_TRACE] [%1] Geometry scan: visible rows [%2, %3], total visible: %4")
                .arg(viewTag).arg(proxyRows.first()).arg(proxyRows.last()).arg(proxyRows.size());
        } else {
            // 传统 TreeView 换算
            int topInContent = scrollY - view->y();
            int bottomInContent = topInContent + vpHeight;

            int firstVisible = -1, lastVisible = -1, visibleCount = 0;
            int rowCount = proxy->rowCount();

            for (int r = 0; r < rowCount; ++r) {
                QModelIndex pIdx = proxy->index(r, 0);
                QRect rRect = view->visualRect(pIdx);

                if (!rRect.isValid() || rRect.isEmpty()) {
                    qDebug().noquote() << QString("[THUMB_TRACE] [%1] Skip row %2: invalid visualRect.").arg(viewTag).arg(r);
                    continue;
                }

                if (rRect.bottom() < topInContent) continue;
                if (rRect.top() > bottomInContent) break;

                if (firstVisible == -1) firstVisible = r;
                lastVisible = r;
                visibleCount++;

                QModelIndex srcIdx = proxy->mapToSource(pIdx);
                if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
            }

            qDebug().noquote() << QString("[THUMB_TRACE] [%1] Geometry scan: visible rows [%2, %3], total visible: %4")
                .arg(viewTag).arg(firstVisible).arg(lastVisible).arg(visibleCount);
        }
    };

    scanView(m_folderView, m_folderProxyModel);
    scanView(m_fileView, m_fileProxyModel);

    if (!visibleRows.isEmpty()) {
        qDebug() << "[THUMB_TRACE] DualSectionPanel::refreshVisibleThumbnails - Submitting" << visibleRows.size() << "rows to loadThumbnailsForRows.";
        model->loadThumbnailsForRows(visibleRows.values());
    } else {
        qDebug() << "[THUMB_TRACE] DualSectionPanel::refreshVisibleThumbnails - No visible rows found in viewport sampling.";
    }
}
>>>>>>> REPLACE
```

---

### 5. `src/ui/ContentPanel.cpp`

**收敛 `m_visibleTimer` 废弃的固定延时触发逻辑**

```
<<<<<<< SEARCH
    m_visibleTimer = new QTimer(this);
    m_visibleTimer->setSingleShot(true);
    m_visibleTimer->setInterval(60);
    connect(m_visibleTimer, &QTimer::timeout, this, &ContentPanel::refreshVisibleThumbnails);
=======
    // 旧版 post-loading 定时器已移除，完全由 SectionedScrollCanvas 的事件驱动机制替代
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. 使用 `ninja` 编译：
   `ninja -C build`
2. 启动并测试：
   - 目录切换、启动初始化、窗口最大化/还原、缩放时，检查日志输出，核查是否在几何准备就绪后触发一次精准扫描。
   - 文件区所有可见行（包含最后一排）全额进入 `loadThumbnailsForRows` 并呈现缩略图。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- 缩略图加载流程一律不动；
- 界面交互及双击/右键行为零改动；
- 扫描触发完全收敛至 `SectionedScrollCanvas` 内部防抖入口。
