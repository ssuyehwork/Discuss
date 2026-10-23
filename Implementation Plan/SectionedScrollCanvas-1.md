# Implementation Plan - SectionedScrollCanvas-1.md

## 1. Overview
本实施方案旨在解决 QuarkMeta 资产管理桌面应用在列表视图（List Mode / QTreeView）下存在的两个交互与渲染 Bug：
1. **文件夹展开状态下内容不可见（文件夹高度异常）**：标题栏显示 `文件夹 (N)` 且箭头处于展开状态，但 `m_folderView`（文件夹 QTreeView）高度计算或可见性存在异常，导致文件夹项无法渲染展示。
2. **Ctrl + 滚轮缩放不同步**：Ctrl + 滚轮调整尺寸时，仅更新了文件视图 (`m_fileView`) 的图标尺寸，而文件夹视图 (`m_folderView`) 完全未响应缩放，导致文件夹与文件的图标尺寸一大一小不一致。

### 根因分析
1. **文件夹视图高度计算异常**：在 `SectionedScrollCanvas::updateSectionCounts()` 中，当列表视图初始化或在折叠状态切换时，`sizeHintForRow(0)` 可能返回 <=0 或未完全布局的值，导致计算出的固定高度不足以完全容纳文件夹项；另外，缩放改变行高后未同步刷新文件夹视图高度。
2. **Ctrl + 滚轮缩放未作用于文件夹视图**：`SectionedScrollCanvas::updateZoom(int zoomLevel)` 的 `else`（列表模式）分支中，仅将 `m_fileView` 强转为 `DropTreeView` 并设置 `setIconSize` 及 `setZoomLevel`，完全遗漏了 `m_folderView`（也是 `DropTreeView`）的缩放处理。同时，图标缩放改变行高后，需要重新触发 `updateSectionCounts()` 以按新行高更新两者的固定高度。

### 解决方案
1. **缩放同步联动**：在 `SectionedScrollCanvas::updateZoom` 的列表模式分支中，对 `m_folderView` 和 `m_fileView` 均进行 `setIconSize` 设置与 `doItemsLayout()` 刷新，保持图标尺寸 100% 同步。
2. **高度计算与动态刷新优化**：在 `updateZoom` 触发图标尺寸调整后，调用 `updateSectionCounts()` 重新计算 `m_folderView` 和 `m_fileView` 的高度；在 `updateSectionCounts()` 中，确保行高与表头高度具备合理的默认保底值，并调用 `updateGeometry()` 确保 Qt 布局更新。

---

## 2. Modified Files List
- `src/ui/SectionedScrollCanvas.cpp`

---

## 3. Detailed Line-by-Line Changes

### File: `src/ui/SectionedScrollCanvas.cpp`

```
<<<<<<< SEARCH
void SectionedScrollCanvas::updateZoom(int zoomLevel) {
    if (m_type == CanvasType::Grid) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_fileView)) jv->setTargetRowHeight(zoomLevel);
        if (auto* fjv = qobject_cast<JustifiedView*>(m_folderView)) fjv->setTargetRowHeight(zoomLevel);
    } else {
        auto* tree = static_cast<DropTreeView*>(m_fileView);
        if (auto* hdr = qobject_cast<ContentHeaderView*>(tree->header())) {
            hdr->setZoomLevel(zoomLevel);
        }
        // 绝对照抄原数值：qMax(16, zoomLevel - 8)
        tree->setIconSize(QSize(qMax(16, zoomLevel - 8), qMax(16, zoomLevel - 8)));
        tree->doItemsLayout();
    }
}
=======
void SectionedScrollCanvas::updateZoom(int zoomLevel) {
    if (m_type == CanvasType::Grid) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_fileView)) jv->setTargetRowHeight(zoomLevel);
        if (auto* fjv = qobject_cast<JustifiedView*>(m_folderView)) fjv->setTargetRowHeight(zoomLevel);
    } else {
        QSize iconSize(qMax(16, zoomLevel - 8), qMax(16, zoomLevel - 8));
        if (auto* folderTree = qobject_cast<DropTreeView*>(m_folderView)) {
            folderTree->setIconSize(iconSize);
            folderTree->doItemsLayout();
        }
        if (auto* fileTree = qobject_cast<DropTreeView*>(m_fileView)) {
            if (auto* hdr = qobject_cast<ContentHeaderView*>(fileTree->header())) {
                hdr->setZoomLevel(zoomLevel);
            }
            fileTree->setIconSize(iconSize);
            fileTree->doItemsLayout();
        }
        updateSectionCounts();
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            } else {
                // 绝对照抄原数值：默认行高 30，边距 2
                auto* tv = static_cast<QTreeView*>(m_folderView);
                int rowH = tv->sizeHintForRow(0);
                if (rowH <= 0) rowH = 30;
                int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
                m_folderView->setFixedHeight(folderCount * rowH + hdrH + 2);
            }
=======
            } else {
                // 绝对照抄原数值：默认行高 30，边距 2，配合图标大小保持安全保底高度
                auto* tv = static_cast<QTreeView*>(m_folderView);
                int rowH = tv->sizeHintForRow(0);
                int iconH = tv->iconSize().height();
                if (rowH <= iconH) rowH = iconH + 10;
                if (rowH <= 0) rowH = 30;
                int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
                m_folderView->setFixedHeight(folderCount * rowH + hdrH + 2);
                m_folderView->updateGeometry();
            }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            } else {
                // 遵循 AllViewsCoExpansion.md：绝对照抄原数值
                auto* tv = static_cast<QTreeView*>(m_fileView);
                int rowH = tv->sizeHintForRow(0);
                if (rowH <= 0) rowH = 30;
                int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
                m_fileView->setFixedHeight(fileCount * rowH + hdrH + 2);
            }
=======
            } else {
                // 遵循 AllViewsCoExpansion.md：绝对照抄原数值，配合图标大小保持安全保底高度
                auto* tv = static_cast<QTreeView*>(m_fileView);
                int rowH = tv->sizeHintForRow(0);
                int iconH = tv->iconSize().height();
                if (rowH <= iconH) rowH = iconH + 10;
                if (rowH <= 0) rowH = 30;
                int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
                m_fileView->setFixedHeight(fileCount * rowH + hdrH + 2);
                m_fileView->updateGeometry();
            }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### 构建步骤
在项目根目录运行 CMake 构建：
```bash
cmake -B build -S .
cmake --build build --config Debug
```

### 验证方法
1. **列表视图文件夹可见性测试**：
   - 切换到列表视图（ListView）模式；
   - 打开同时包含文件夹和文件的目录；
   - 观察文件夹列表是否能正常完整显示在“文件夹”标题栏下方。
2. **Ctrl + 滚轮按键缩放测试**：
   - 在列表模式下按下 `Ctrl` 键并滚动鼠标滚轮；
   - 观察文件夹图标与文件图标是否同步按比例同步放大/缩小，且图标尺寸完全一致。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- [x] **复用性检查**：完全复用 `DropTreeView::setIconSize` 及 `SectionedScrollCanvas::updateSectionCounts` 既有计算逻辑。
- [x] **零另起炉灶**：没有新建任何额外的缩放控制代码或重复控件。
- [x] **零参数篡改**：完全保持 `qMax(16, zoomLevel - 8)` 图标尺寸计算公式及 `hdrH + 2` 的物理边距计算，符合《Zero-Value-Alteration Contract》。

---

## 6. Header API Signature Verification

| 调用的成员/类 | 头文件物理声明源 | 头文件物理精确签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `QAbstractItemView::setIconSize` | `<QAbstractItemView>` (Qt) | `void setIconSize(const QSize &size)` | 物理核实一致 |
| `QAbstractItemView::iconSize` | `<QAbstractItemView>` (Qt) | `QSize iconSize() const` | 物理核实一致 |
| `DropTreeView` | `src/ui/DropTreeView.h` | `class DropTreeView : public QTreeView` | 物理核实一致 |
| `SectionedScrollCanvas::updateSectionCounts` | `src/ui/SectionedScrollCanvas.h` | `void updateSectionCounts()` | 物理核实一致 |
