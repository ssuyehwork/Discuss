# Implementation Plan - MetaPanel Preview Image & Icon Display Fix (meta_panel-6.md)

This plan details the implementation steps for showing file/folder icons or thumbnail image previews in `MetaPanel`'s top preview box (`m_lblImagePreview`) upon item selection in `MainWindow`.

## Overview
Currently, `MetaPanel` creates `m_lblImagePreview` inside `m_topPreviewBox` but hides it by default. When an item (file or folder) is selected in `ContentPanel`, `MainWindow::initUi` receives the `selectionChanged` signal and updates various metadata properties on `MetaPanel`, but does not pass any `QPixmap` or `QIcon` to `MetaPanel`. Furthermore, `MetaPanel` lacks a public interface to set and toggle the visibility of the preview image/icon.

This implementation adds `setImagePreview` to `MetaPanel` and connects thumbnail/icon retrieval in `MainWindow.cpp` to pass the pixmap to `m_metaPanel`.

---

## Modified Files List
1. `src/ui/MetaPanel.h`
2. `src/ui/MetaPanel.cpp`
3. `src/ui/MainWindow.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/MetaPanel.h`
Add public method `setImagePreview(const QPixmap& pixmap)` to set the preview label pixmap and show/hide `m_lblImagePreview`.

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
Implement `setImagePreview(const QPixmap& pixmap)` to set pixmap on `m_lblImagePreview` and trigger `adjustFlowHeights()`. Also reset preview in `setSelectedPaths` when selection is empty.

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
        // High DPI scaled pixmap display
        QPixmap scaled = pixmap.scaled(QSize(240, 160), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_lblImagePreview->setPixmap(scaled);
        m_lblImagePreview->show();
    }
    adjustFlowHeights();
}
>>>>>>> REPLACE
```

---

### 3. `src/ui/MainWindow.cpp`
In `selectionChanged` callback, when item selection is updated, extract the item's icon/thumbnail from `Qt::DecorationRole` or `DiskMediaExtractor` / `ShellIconManager`, and pass it to `m_metaPanel->setImagePreview(...)`.

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
            // Extract icon / preview thumbnail from model index
            QModelIndex idxPreview = indexes.first();
            QPixmap previewPixmap;
            QVariant decData = idxPreview.data(Qt::DecorationRole);
            if (decData.canConvert<QIcon>()) {
                QIcon icon = decData.value<QIcon>();
                if (!icon.isNull()) {
                    previewPixmap = icon.pixmap(128, 128);
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
1. Verify the implementation plan file complies with all guidelines in `AGENTS.md`.
2. Do not modify C++ source code directly during planning phase per `AGENTS.md`.
