# BatchRenameDialog-1 Implementation Plan

## 1. Overview
This implementation plan addresses the rendering issue where non-1:1 aspect ratio image thumbnails loaded in the `BatchRenameDialog` table view were squeezed into a 1:1 square icon slot (20x20px), causing black borders and mismatched margins.

### Solution (Option B: 1:1 Center Crop with Rounded Corners):
1. Apply 1:1 center cropping (Center Crop) to loaded thumbnail pixmaps.
2. Render smooth anti-aliased rounded corners (3px) onto a clean 1:1 square target pixmap before wrapping in a `QIcon`.
3. Eliminate all black/transparent letterboxing and maintain perfect vertical alignment in the rename preview table.

---

## 2. Modified Files List
1. `src/ui/BatchRenameDialog.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/BatchRenameDialog.cpp`
Update `initTableItems` to apply 1:1 center cropping and smooth rounded corners to cached thumbnails.

<<<<<<< SEARCH
        // 恢复老版本高速机制：直接加载现成的缩略图缓存小图（微秒级），没有才退避为文件图标
        QIcon fileIcon;
        QString thumbPath = DiskMediaExtractor::getDiskThumbCachePath(oldPath);
        if (QFile::exists(thumbPath)) {
            QPixmap pix(thumbPath);
            if (!pix.isNull()) {
                fileIcon = QIcon(pix);
            }
        }
        if (fileIcon.isNull()) {
            fileIcon = ShellIconManager::getFileIconFast(oldPath, info.isDir(), info.suffix().toLower());
        }
=======
        // 🚀 1:1 标准正方形中心裁切 (Center Crop) + 3px 微圆角，消除上下/左右黑边与留白
        QIcon fileIcon;
        QString thumbPath = DiskMediaExtractor::getDiskThumbCachePath(oldPath);
        if (QFile::exists(thumbPath)) {
            QPixmap pix(thumbPath);
            if (!pix.isNull() && pix.width() > 0 && pix.height() > 0) {
                int minSide = qMin(pix.width(), pix.height());
                QRect cropRect((pix.width() - minSide) / 2, (pix.height() - minSide) / 2, minSide, minSide);
                QPixmap cropped = pix.copy(cropRect);

                QPixmap target(48, 48);
                target.fill(Qt::transparent);
                QPainter painter(&target);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
                QPainterPath clipPath;
                clipPath.addRoundedRect(QRectF(0, 0, 48, 48), 6.0, 6.0);
                painter.setClipPath(clipPath);
                painter.drawPixmap(QRect(0, 0, 48, 48), cropped);
                painter.end();

                fileIcon = QIcon(target);
            }
        }
        if (fileIcon.isNull()) {
            fileIcon = ShellIconManager::getFileIconFast(oldPath, info.isDir(), info.suffix().toLower());
        }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Build the project:
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. Open `BatchRenameDialog` with several images of varying aspect ratios (16:9, 9:16, 4:3).
3. Verify that all preview thumbnails in the table's left column render as clean 1:1 rounded square cards without letterboxing or black borders.
