# Implementation Plan - PanelMediator (Selection Debounce & Two-Stage Metadata Loading)

## Overview
This implementation plan aligns `PanelMediator` with Chapter 9 of `QuarkMeta-Architecture-Planning.md`. It introduces a 30ms debounce timer for `ContentPanel::selectionChanged` routing to prevent high-frequency UI flickering during rapid selection navigation, and implements two-stage metadata loading (synchronous 0ms basic physical attributes + asynchronous deep metadata/preview updating) for `MetaPanel`.

## Modified Files List
- `src/ui/PanelMediator.h`
- `src/ui/PanelMediator.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/PanelMediator.h`
Add `#include <QTimer>` and declare private `m_selectionDebounceTimer` along with pending selection path state.

```
<<<<<<< SEARCH
    QPointer<AppShortcutController> m_shortcutController;

    QString m_currentQuickLookPath;
};
=======
    QPointer<AppShortcutController> m_shortcutController;

    QString m_currentQuickLookPath;
    QTimer* m_selectionDebounceTimer = nullptr;
    QStringList m_pendingSelectionPaths;
};
>>>>>>> REPLACE
```

### 2. `src/ui/PanelMediator.cpp`
Update `PanelMediator::setupConnections()` to route `selectionChanged` through `m_selectionDebounceTimer` (30ms single-shot) to perform debounced two-stage updates on `MetaPanel`.

```
<<<<<<< SEARCH
        connect(contentPanel, &ContentPanel::selectionChanged, metaPanel, [contentPanel, metaPanel](const QStringList& paths) {
            metaPanel->setSelectedPaths(paths);
            if (paths.isEmpty()) {
                metaPanel->setImagePreview(QPixmap());
                metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
                metaPanel->setRating(0, false);
                metaPanel->setColor(QString(""), false);
                metaPanel->setTags(QStringList());
                metaPanel->setNote(QString(""));
                metaPanel->setURL(QString(""));
                metaPanel->setPalettes({});
            } else if (paths.size() == 1) {
                QModelIndexList selectedIndices = contentPanel->getSelectedIndexes();
                QModelIndex idx = selectedIndices.isEmpty() ? QModelIndex() : selectedIndices.first();

                QString path = paths.first();
                QFileInfo fi(path);

                QString name = idx.isValid() ? idx.sibling(idx.row(), 0).data(Qt::DisplayRole).toString() : fi.fileName();
                QString type = idx.isValid() ? ((idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), 4).data(Qt::DisplayRole).toString() + " 文件") : (fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件");
                QString sizeStr = idx.isValid() ? idx.sibling(idx.row(), 5).data(Qt::DisplayRole).toString() : "-";
                QString mtimeStr = idx.isValid() ? idx.sibling(idx.row(), 6).data(Qt::DisplayRole).toString() : "-";

                metaPanel->updateInfo(
                    name, type, sizeStr, "-", mtimeStr, "-",
                    path, idx.data(EncryptedRole).toBool(), 0, 0
                );
                metaPanel->setRating(idx.data(RatingRole).toInt(), false);
                metaPanel->setColor(idx.data(ColorRole).toString(), false);
                metaPanel->setTags(idx.data(TagsRole).toStringList());
                metaPanel->setNote(idx.data(NoteRole).toString());
                metaPanel->setURL(idx.data(UrlRole).toString());

                QVariant decData = idx.data(Qt::DecorationRole);
                QPixmap previewPixmap;
                if (decData.canConvert<QIcon>()) {
                    previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                } else if (decData.canConvert<QPixmap>()) {
                    previewPixmap = decData.value<QPixmap>();
                }
                metaPanel->setImagePreview(previewPixmap);
            }
        });
=======
        if (!m_selectionDebounceTimer) {
            m_selectionDebounceTimer = new QTimer(this);
            m_selectionDebounceTimer->setSingleShot(true);
            m_selectionDebounceTimer->setInterval(30);
        }

        m_selectionDebounceTimer->disconnect();
        connect(m_selectionDebounceTimer, &QTimer::timeout, this, [this, contentPanel, metaPanel]() {
            if (!metaPanel || !contentPanel) return;
            const QStringList& paths = m_pendingSelectionPaths;
            metaPanel->setSelectedPaths(paths);

            if (paths.isEmpty()) {
                metaPanel->setImagePreview(QPixmap());
                metaPanel->updateInfo("-", "-", "-", "-", "-", "-", "-", false, 0, 0);
                metaPanel->setRating(0, false);
                metaPanel->setColor(QString(""), false);
                metaPanel->setTags(QStringList());
                metaPanel->setNote(QString(""));
                metaPanel->setURL(QString(""));
                metaPanel->setPalettes({});
            } else if (paths.size() == 1) {
                QModelIndexList selectedIndices = contentPanel->getSelectedIndexes();
                QModelIndex idx = selectedIndices.isEmpty() ? QModelIndex() : selectedIndices.first();

                QString path = paths.first();
                QFileInfo fi(path);

                // 第一阶段：0ms 物理属性与 SSOT 元数据同步呈现
                QString name = idx.isValid() ? idx.sibling(idx.row(), 0).data(Qt::DisplayRole).toString() : fi.fileName();
                QString type = idx.isValid() ? ((idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), 4).data(Qt::DisplayRole).toString() + " 文件") : (fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件");
                QString sizeStr = idx.isValid() ? idx.sibling(idx.row(), 5).data(Qt::DisplayRole).toString() : "-";
                QString mtimeStr = idx.isValid() ? idx.sibling(idx.row(), 6).data(Qt::DisplayRole).toString() : "-";

                // SSOT 权威校验兜底
                RuntimeMeta meta = MetadataManager::instance().getMeta(path.toStdWString());
                int rating = idx.isValid() ? idx.data(RatingRole).toInt() : meta.rating;
                QString color = idx.isValid() ? idx.data(ColorRole).toString() : QString::fromStdWString(meta.manualColor);
                QStringList tags = idx.isValid() ? idx.data(TagsRole).toStringList() : meta.tags;
                QString note = idx.isValid() ? idx.data(NoteRole).toString() : QString::fromStdWString(meta.note);
                QString url = idx.isValid() ? idx.data(UrlRole).toString() : QString::fromStdWString(meta.url);

                metaPanel->updateInfo(
                    name, type, sizeStr, "-", mtimeStr, "-",
                    path, idx.isValid() ? idx.data(EncryptedRole).toBool() : meta.encrypted,
                    meta.width, meta.height
                );
                metaPanel->setRating(rating, false);
                metaPanel->setColor(color, false);
                metaPanel->setTags(tags);
                metaPanel->setNote(note);
                metaPanel->setURL(url);

                // 第二阶段：异步缩略图/预览管线呈现
                QVariant decData = idx.isValid() ? idx.data(Qt::DecorationRole) : QVariant();
                QPixmap previewPixmap;
                if (decData.canConvert<QIcon>()) {
                    previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                } else if (decData.canConvert<QPixmap>()) {
                    previewPixmap = decData.value<QPixmap>();
                }
                metaPanel->setImagePreview(previewPixmap);
            }
        });

        connect(contentPanel, &ContentPanel::selectionChanged, this, [this](const QStringList& paths) {
            m_pendingSelectionPaths = paths;
            if (m_selectionDebounceTimer) {
                m_selectionDebounceTimer->start();
            }
        });
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `PanelMediator.h` and `PanelMediator.cpp` syntax and include `#include <QTimer>`.
2. Ensure CMake builds cleanly and MOC processes `PanelMediator`.
3. Verify selection changes across GridView, ListView, JustifiedView, and ColumnView route through `PanelMediator` debounced at 30ms before updating `MetaPanel`.
