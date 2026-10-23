# MetaPanel Implementation Plan (Iterative Version 5)

> **Note**: As per AGENTS.md Protocol 3.1 (Immutable Plan Files), this is an iterative update (`MetaPanel-1.md`) addressing the latest origin/main sync and incorporating the 220x220px preview canvas and 35x45px icon scaling rules:
> - Preview box bounding area for non-graphics item = **220x220px**.
> - File icon scaled to fit **35x45px** (`QSize(35, 45)` with `Qt::KeepAspectRatio` & `Qt::SmoothTransformation`) and centered inside the **220x220px** preview canvas.

## 1. Overview
This implementation plan addresses two specific user requirements for `MetaPanel` and its wiring in `PanelMediator`:
1. **Dimension Display Restoration**: Restore the display of image/media dimensions (e.g. `1920 x 1080 像素`) in `MetaPanel`'s "基础属性" (Basic Attributes) section by passing real cached `meta.width` and `meta.height` values from `MetadataManager` in `PanelMediator.cpp`, instead of passing hardcoded `0, 0`.
2. **Preview Area & File Icon Sizing**:
   - For real image/media thumbnails: Keep proportional scaling up to max width (120~230px) and max height (220px).
   - For non-graphics items / default file icons: The top preview container/label maintains a **220x220px** canvas, inside which the default file icon is scaled to **35x45px** (max width 35px, max height 45px) and centered.

---

## 2. Modified Files List
1. `src/ui/MetaPanel.h`
2. `src/ui/MetaPanel.cpp`
3. `src/ui/PanelMediator.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/MetaPanel.h`
Add `isDefaultIcon` parameter to `setImagePreview` so `MetaPanel` knows when a non-graphics item icon is being rendered.

```
<<<<<<< SEARCH
    void setImagePreview(const QPixmap& pixmap);
=======
    void setImagePreview(const QPixmap& pixmap, bool isDefaultIcon = false);
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/MetaPanel.cpp`
Update `MetaPanel::setImagePreview` implementation:
- If `isDefaultIcon` is true: Create a 220x220px canvas, draw the default icon scaled to `QSize(35, 45)` centered within the 220x220px canvas.
- If `isDefaultIcon` is false: Maintain normal image thumbnail scaling behavior.

```
<<<<<<< SEARCH
void MetaPanel::setImagePreview(const QPixmap& pixmap) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        int maxW = m_container ? (m_container->width() - 16) : 214;
        maxW = qBound(120, maxW, 230);
        int maxH = 220;

        QPixmap scaled = (pixmap.width() > maxW || pixmap.height() > maxH)
            ? pixmap.scaled(QSize(maxW, maxH), Qt::KeepAspectRatio, Qt::SmoothTransformation)
            : pixmap;

        QImage roundedImg(scaled.size(), QImage::Format_ARGB32_Premultiplied);
        roundedImg.fill(Qt::transparent);
        {
            QPainter painter(&roundedImg);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            QPainterPath path;
            path.addRoundedRect(QRectF(0, 0, scaled.width(), scaled.height()), 4.0, 4.0);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, scaled);
        }

        m_lblImagePreview->setPixmap(QPixmap::fromImage(roundedImg));
        m_lblImagePreview->setFixedSize(scaled.size());

        m_lblImagePreview->show();
        if (m_topPreviewBox) m_topPreviewBox->show();
    }
    m_adjustTimer->start();
}
=======
void MetaPanel::setImagePreview(const QPixmap& pixmap, bool isDefaultIcon) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        if (isDefaultIcon) {
            // 非图形图像（默认文件/文件夹图标）：预览视口 Canvas 设定为 220x220px，中间绘制 35x45px 图标
            QSize canvasSize(220, 220);
            QPixmap canvas(canvasSize);
            canvas.fill(Qt::transparent);

            // 强制将默认 OS 图标等比例缩放到 35x45 目标范围
            QPixmap iconScaled = pixmap.scaled(QSize(35, 45), Qt::KeepAspectRatio, Qt::SmoothTransformation);

            {
                QPainter painter(&canvas);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);

                int x = (220 - iconScaled.width()) / 2;
                int y = (220 - iconScaled.height()) / 2;
                painter.drawPixmap(x, y, iconScaled);
            }

            m_lblImagePreview->setPixmap(canvas);
            m_lblImagePreview->setFixedSize(canvasSize);
        } else {
            int maxW = m_container ? (m_container->width() - 16) : 214;
            maxW = qBound(120, maxW, 230);
            int maxH = 220;

            QPixmap scaled = (pixmap.width() > maxW || pixmap.height() > maxH)
                ? pixmap.scaled(QSize(maxW, maxH), Qt::KeepAspectRatio, Qt::SmoothTransformation)
                : pixmap;

            QImage roundedImg(scaled.size(), QImage::Format_ARGB32_Premultiplied);
            roundedImg.fill(Qt::transparent);
            {
                QPainter painter(&roundedImg);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);

                QPainterPath path;
                path.addRoundedRect(QRectF(0, 0, scaled.width(), scaled.height()), 4.0, 4.0);
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, scaled);
            }

            m_lblImagePreview->setPixmap(QPixmap::fromImage(roundedImg));
            m_lblImagePreview->setFixedSize(scaled.size());
        }

        m_lblImagePreview->show();
        if (m_topPreviewBox) m_topPreviewBox->show();
    }
    m_adjustTimer->start();
}
>>>>>>> REPLACE
```

---

### Change 3: `src/ui/PanelMediator.cpp`
1. In `updateMetaPanelFromPanel`, fetch `meta.width` and `meta.height` from `MetadataManager::instance().getMeta(path.toStdWString())` and pass them to `metaPanel->updateInfo`.
2. Check `HasThumbnailRole` from `idx` to determine if `decData` is a default icon vs a real thumbnail, and pass `isDefaultIcon` (`!hasThumb`) to `metaPanel->setImagePreview`.

```
<<<<<<< SEARCH
            bool encrypted = idx.isValid() ? idx.data(EncryptedRole).toBool() : false;

            metaPanel->updateInfo(
                name, type, sizeStr, "-", mtimeStr, "-",
                path, encrypted, 0, 0
            );

            auto meta = MetadataManager::instance().getMeta(path.toStdWString());
=======
            bool encrypted = idx.isValid() ? idx.data(EncryptedRole).toBool() : false;

            auto meta = MetadataManager::instance().getMeta(path.toStdWString());

            metaPanel->updateInfo(
                name, type, sizeStr, "-", mtimeStr, "-",
                path, encrypted, meta.width, meta.height
            );
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                QVariant decData = idx.data(Qt::DecorationRole);
                QPixmap previewPixmap;
                if (decData.canConvert<QIcon>()) {
                    previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                } else if (decData.canConvert<QPixmap>()) {
                    previewPixmap = decData.value<QPixmap>();
                }
                metaPanel->setImagePreview(previewPixmap);
=======
                QVariant decData = idx.data(Qt::DecorationRole);
                bool hasThumb = idx.data(HasThumbnailRole).toBool();
                QPixmap previewPixmap;
                if (decData.canConvert<QIcon>()) {
                    previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                } else if (decData.canConvert<QPixmap>()) {
                    previewPixmap = decData.value<QPixmap>();
                }
                metaPanel->setImagePreview(previewPixmap, !hasThumb);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Clean and build the target:
   `cmake --build build --config Release`
2. Verify:
   - Select an image file (e.g. `.png` / `.jpg`). The "基础属性" section in `MetaPanel` should display the actual dimensions `W x H 像素`.
   - Select a non-graphics item or folder without thumbnail. The top preview area canvas is 220x220px, and the centered file icon size is scaled up/down to strictly fill 35x45px without becoming tiny.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Metadata SSOT**: Directly reuses `MetadataManager::instance().getMeta(path)` to retrieve `meta.width` and `meta.height`.
- **Thumbnail SSOT**: Uses `HasThumbnailRole` (from `ModelContract.h`) to accurately distinguish real thumbnails from default icons.

---

## 6. Header API Signature Verification
- `MetaPanel::updateInfo(const QString&, const QString&, const QString&, const QString&, const QString&, const QString&, const QString&, bool, int width, int height)`: Checked in `MetaPanel.h`.
- `MetadataManager::getMeta(const std::wstring&)`: Returns `RuntimeMeta` struct containing `.width` and `.height`. Checked in `MetadataManager.h`.
- `QModelIndex::data(int role)`: Retrives `HasThumbnailRole` (`Qt::UserRole + 202`). Checked in `ModelContract.h`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/ui/PanelMediator.cpp` includes `../core/ModelContract.h`, ensuring `HasThumbnailRole` is fully declared.
- `src/ui/MetaPanel.h` includes `<QPixmap>`, `MetaPanel.cpp` includes `<QPainter>`, `<QPainterPath>`.
