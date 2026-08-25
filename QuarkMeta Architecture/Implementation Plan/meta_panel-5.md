# Implementation Plan - MetaPanel Real Image Preview Feature Integration (Qt Syntax Fixed)

## 1. Overview
This implementation plan corrects the C++ syntax in `meta_panel-4.md` where `m_lblImagePreview->pixmap()` was accessed using pointer arrow `->` instead of value object dot `.` operator. `QLabel::pixmap()` in Qt returns a `QPixmap` value object, so calling `.isNull()` and `.height()` requires the dot `.` operator.

This plan integrates the full, 100% compilation-clean **Direction A** real image preview functionality into `MetaPanel`:
1. Exposing `setImagePreview(const QPixmap& pixmap)` in `MetaPanel.h` and `MetaPanel.cpp`.
2. Updating `MetaPanel::adjustFlowHeights()` with correct `QPixmap` dot operator syntax (`!m_lblImagePreview->pixmap().isNull()` and `m_lblImagePreview->pixmap().height()`).
3. Updating `MainWindow.cpp` selection handler to pass thumbnail `QIcon` pixmaps to `MetaPanel::setImagePreview()`.

## 2. Modified Files List
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`
- `src/ui/MainWindow.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/MetaPanel.h`

```diff
<<<<<<< SEARCH
    void updateInfo(const QString& name, const QString& type, const QString& size,
                    const QString& ctime, const QString& mtime, const QString& atime,
                    const QString& path, bool encrypted, int width = 0, int height = 0);

    void setSelectedPaths(const QStringList& paths);
=======
    void updateInfo(const QString& name, const QString& type, const QString& size,
                    const QString& ctime, const QString& mtime, const QString& atime,
                    const QString& path, bool encrypted, int width = 0, int height = 0);

    void setImagePreview(const QPixmap& pixmap);
    void setSelectedPaths(const QStringList& paths);
>>>>>>> REPLACE
```

### `src/ui/MetaPanel.cpp`

```diff
<<<<<<< SEARCH
void MetaPanel::setSelectedPaths(const QStringList& paths) {
    m_selectedPaths = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
    updateControlsState(hasSelection);

    if (!hasSelection) {
        m_isInternalUpdating = true;
=======
void MetaPanel::setImagePreview(const QPixmap& pixmap) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
    } else {
        int maxW = m_topPreviewBox ? qMax(180, m_topPreviewBox->width() - 16) : 200;
        QPixmap scaled = pixmap.scaled(maxW, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_lblImagePreview->setPixmap(scaled);
        m_lblImagePreview->show();
    }
    adjustFlowHeights();
    if (m_container) m_container->adjustSize();
}

void MetaPanel::setSelectedPaths(const QStringList& paths) {
    m_selectedPaths = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
    updateControlsState(hasSelection);

    if (!hasSelection) {
        m_isInternalUpdating = true;
        setImagePreview(QPixmap());
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void MetaPanel::adjustFlowHeights() {
    if (m_topPreviewBox && m_paletteFlowLayout) {
        int contentH = m_paletteFlowLayout->heightForWidth(m_topPreviewBox->width());
        bool hasPreview = (m_lblImagePreview && m_lblImagePreview->isVisible());
        bool hasPalette = (m_paletteFlowLayout->count() > 0);
        if (hasPreview || hasPalette) {
            m_topPreviewBox->show();
            m_topPreviewBox->setFixedHeight(qMax(32, contentH + (hasPreview ? 70 : 0)));
        } else {
            m_topPreviewBox->hide();
            m_topPreviewBox->setFixedHeight(0);
        }
        m_paletteFlowLayout->activate();
    }
=======
void MetaPanel::adjustFlowHeights() {
    if (m_topPreviewBox && m_paletteFlowLayout) {
        int contentH = m_paletteFlowLayout->heightForWidth(m_topPreviewBox->width());
        bool hasPreview = (m_lblImagePreview && m_lblImagePreview->isVisible() && !m_lblImagePreview->pixmap().isNull());
        bool hasPalette = (m_paletteFlowLayout->count() > 0);
        if (hasPreview || hasPalette) {
            m_topPreviewBox->show();
            int previewH = hasPreview ? m_lblImagePreview->pixmap().height() : 0;
            m_topPreviewBox->setFixedHeight(qMax(32, contentH + previewH + 16));
        } else {
            m_topPreviewBox->hide();
            m_topPreviewBox->setFixedHeight(0);
        }
        m_paletteFlowLayout->activate();
    }
>>>>>>> REPLACE
```

### `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
        m_metaPanel->setSelectedPaths(paths);
        if (paths.isEmpty()) {
            m_metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
            m_metaPanel->setRating(0);
            m_metaPanel->setColor(L"");
            m_metaPanel->setPinned(false);
            m_metaPanel->setTags(QStringList());
            m_metaPanel->setNote(L"");
            m_metaPanel->setURL(L"");
            m_metaPanel->setPalettes({});
        } else {
=======
        m_metaPanel->setSelectedPaths(paths);
        if (paths.isEmpty()) {
            m_metaPanel->setImagePreview(QPixmap());
            m_metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
            m_metaPanel->setRating(0);
            m_metaPanel->setColor(L"");
            m_metaPanel->setPinned(false);
            m_metaPanel->setTags(QStringList());
            m_metaPanel->setNote(L"");
            m_metaPanel->setURL(L"");
            m_metaPanel->setPalettes({});
        } else {
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
            // 5. 标签、备注、链接与色板展示
            m_metaPanel->setTags(cleanTags);
            m_metaPanel->setNote(noteStr);
            m_metaPanel->setURL(urlStr);
            m_metaPanel->setPalettes(palettes);
        }
=======
            // 5. 标签、备注、链接、图片预览与色板展示
            m_metaPanel->setTags(cleanTags);
            m_metaPanel->setNote(noteStr);
            m_metaPanel->setURL(urlStr);
            m_metaPanel->setPalettes(palettes);

            QVariant decoData = idx.data(Qt::DecorationRole);
            bool hasThumb = idx.data(HasThumbnailRole).toBool();
            if (hasThumb && decoData.canConvert<QIcon>()) {
                QIcon icon = decoData.value<QIcon>();
                if (!icon.isNull()) {
                    m_metaPanel->setImagePreview(icon.pixmap(256, 256));
                } else {
                    m_metaPanel->setImagePreview(QPixmap());
                }
            } else {
                m_metaPanel->setImagePreview(QPixmap());
            }
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Configure and build CMake target:
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. Launch application, navigate to a directory containing images/vector files (e.g. `.eps`, `.png`, `.jpg`).
3. Click on a file with a generated thumbnail and verify that the top of `MetaPanel` (`m_topPreviewBox`) displays the image preview cleanly with 0 compilation warnings or errors.
