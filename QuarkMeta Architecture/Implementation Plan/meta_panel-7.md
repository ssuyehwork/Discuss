# Implementation Plan - MetaPanel Preview & Qt 6 QPixmap Syntax Correction (meta_panel-7.md)

This implementation plan corrects all `QPixmap` syntax errors present in earlier plans and fully details how `MainWindow.cpp` connects selected items' icons and thumbnails to `MetaPanel`'s `setImagePreview(const QPixmap& pixmap)` API.

## Overview
In Qt 6, `QLabel::pixmap()` returns a `QPixmap` value object (not a pointer `QPixmap*`). Therefore:
1. Pointer arrow operators `->` cannot be used on `QPixmap` value objects.
2. `QPixmap` value objects cannot be used directly in boolean conditions. `isNull()` must be called explicitly.

This plan resolves those compilation errors and provides 100% syntactically valid Git Merge Diffs for `MetaPanel.h`, `MetaPanel.cpp`, and `MainWindow.cpp`.

---

## Modified Files List
1. `src/ui/MetaPanel.h`
2. `src/ui/MetaPanel.cpp`
3. `src/ui/MainWindow.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/MetaPanel.h`
Add public method declaration `setImagePreview(const QPixmap& pixmap)` to set or clear the preview label:

```git
<<<<<<< SEARCH
    void setSelectedPaths(const QStringList& paths);
    void setPalettes(const QVector<QPair<QColor, float>>& palette);
=======
    void setSelectedPaths(const QStringList& paths);
    void setImagePreview(const QPixmap& pixmap);
    void setPalettes(const QVector<QPair<QColor, float>>& palette);
>>>>>>> REPLACE
```

---

### 2. `src/ui/MetaPanel.cpp`

#### 2.1 Implement `setImagePreview` & Clear Preview on Empty Selection
Add `setImagePreview(const QPixmap& pixmap)` implementation with `QPixmap` value object syntax:

```git
<<<<<<< SEARCH
void MetaPanel::setSelectedPaths(const QStringList& paths) {
    m_selectedPaths = paths;
    updateControlsState(!m_selectedPaths.isEmpty());
}
=======
void MetaPanel::setSelectedPaths(const QStringList& paths) {
    m_selectedPaths = paths;
    updateControlsState(!m_selectedPaths.isEmpty());
    if (paths.isEmpty()) {
        setImagePreview(QPixmap());
    }
}

void MetaPanel::setImagePreview(const QPixmap& pixmap) {
    if (!m_lblImagePreview) return;

    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
    } else {
        int maxW = m_topPreviewBox ? qMax(180, m_topPreviewBox->width() - 12) : 240;
        QPixmap scaled = pixmap.scaled(maxW, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_lblImagePreview->setPixmap(scaled);
        m_lblImagePreview->show();
    }
    adjustFlowHeights();
}
>>>>>>> REPLACE
```

#### 2.2 Fix `adjustFlowHeights` `QPixmap` Syntax Error
Ensure `adjustFlowHeights()` accesses `QPixmap` using dot `.isNull()` and `.height()`:

```git
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
            int previewH = hasPreview ? qMin(180, qMax(60, m_lblImagePreview->pixmap().height())) : 0;
            m_topPreviewBox->setFixedHeight(qMax(32, contentH + previewH + (hasPreview ? 12 : 0)));
        } else {
            m_topPreviewBox->hide();
            m_topPreviewBox->setFixedHeight(0);
        }
        m_paletteFlowLayout->activate();
    }
>>>>>>> REPLACE
```

---

### 3. `src/ui/MainWindow.cpp`

Hook up `selectionChanged` callback in `MainWindow.cpp` to retrieve item icon/thumbnail and pass it to `setImagePreview`:

```git
<<<<<<< SEARCH
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
            // Extract item icon / preview thumbnail from Model index
            QModelIndex idxPreview = indexes.first();
            QPixmap previewPixmap;
            QVariant decData = idxPreview.data(Qt::DecorationRole);
            if (decData.canConvert<QIcon>()) {
                QIcon icon = decData.value<QIcon>();
                if (!icon.isNull()) {
                    previewPixmap = icon.pixmap(256, 256);
                }
            } else if (decData.canConvert<QPixmap>()) {
                previewPixmap = decData.value<QPixmap>();
            }

            if (previewPixmap.isNull()) {
                QImage img = DiskMediaExtractor::getCapsuleThumbnailReadOnly(paths.first());
                if (!img.isNull()) {
                    previewPixmap = QPixmap::fromImage(img);
                } else {
                    previewPixmap = ShellIconManager::getFileIcon(paths.first(), 128).pixmap(128, 128);
                }
            }
            m_metaPanel->setImagePreview(previewPixmap);
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Verify `meta_panel-7.md` includes 100% syntactically correct `QPixmap` dot operator calls (`.isNull()` and `.height()`).
2. Verify all path names and file references adhere to strict project specifications.
