# Implementation Plan - QuickLook Window Focus & Scaling Fix (QuickLookWindow-1.md)

## Overview
本实施方案旨在解决 QuickLook 预览窗口在 Windows 平台下按空格键“一闪而过/弹不出来”、高分辨率图像默认放大局部而非全屏展示，以及在 `ContentPanel` 中快捷键选区索引容错缺失的问题。

---

## Modified Files List
1. `src/ui/QuickLookWindow.cpp`
2. `src/ui/QuickLookGraphicsView.cpp`
3. `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/QuickLookWindow.cpp`
优化失焦事件拦截逻辑，增加对活动窗口及内部子控件获焦状态的校验，防止子控件获焦触发伪失焦而自我销毁关闭。

```
<<<<<<< SEARCH
    if (event->type() == QEvent::WindowDeactivate) {
        if (m_ignoreDeactivate) {
            return true;
        }
        closePreview();
    }
    return QWidget::eventFilter(watched, event);
=======
    if (event->type() == QEvent::WindowDeactivate) {
        if (m_ignoreDeactivate) {
            return true;
        }

        QWidget* activeWin = QApplication::activeWindow();
        QWidget* focusW = QApplication::focusWidget();
        if (activeWin == this || (focusW && this->isAncestorOf(focusW))) {
            return true;
        }

        closePreview();
        return true;
    }
    return QWidget::eventFilter(watched, event);
>>>>>>> REPLACE
```

---

### 2. `src/ui/QuickLookGraphicsView.cpp`
修改图像设置逻辑，默认采用 `fitImage()` 使图片全屏居中展示，取代 100% 原始像素展示。

```
<<<<<<< SEARCH
void QuickLookGraphicsView::setPixmap(const QPixmap& pixmap) {
    m_pixmapItem->setPixmap(pixmap);
    m_scene->setSceneRect(m_pixmapItem->boundingRect());

    if (m_minimap) {
        m_minimap->setPixmap(pixmap);
    }

    setZoomOriginal();
    updateMinimap();
}
=======
void QuickLookGraphicsView::setPixmap(const QPixmap& pixmap) {
    m_pixmapItem->setPixmap(pixmap);
    m_scene->setSceneRect(m_pixmapItem->boundingRect());

    if (m_minimap) {
        m_minimap->setPixmap(pixmap);
    }

    fitImage();
    updateMinimap();
}
>>>>>>> REPLACE
```

---

### 3. `src/ui/ContentPanel.cpp`
在处理空格键按键事件时，补充对选中区域索引的容错逻辑。若 `currentIndex()` 无效，尝试从 `selectedIndexes()` 提取第一个有效选中项目。

```
<<<<<<< SEARCH
            if (keyEvent->key() == Qt::Key_Space) {
                QModelIndex idx = view->currentIndex();
                if (idx.isValid()) {
=======
            if (keyEvent->key() == Qt::Key_Space) {
                QModelIndex idx = view->currentIndex();
                if (!idx.isValid() && view->selectionModel()) {
                    auto selected = view->selectionModel()->selectedIndexes();
                    if (!selected.isEmpty()) {
                        idx = selected.first();
                    }
                }

                if (idx.isValid()) {
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **构建工程**：
   在 MSVC 编译环境下运行：
   ```bash
   cmake --build build --config Release
   ```
2. **功能验证**：
   - 选定任意图像/文本文件，按 `Space` 键，确认预览窗口稳定弹出，不会因失焦自动瞬间关闭。
   - 打开高分辨率（如 4K/8K）图片，确认图片默认自适应居中完整展示在预览区域内。
   - 使用鼠标框选文件后按 `Space` 键，确认选区索引容错能够生效并正常弹窗。
