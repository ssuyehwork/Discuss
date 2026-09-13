# Implementation Plan - ColumnView Metadata Bridge (ColumnViewMetadataBridge.md)

## 1. Overview
This implementation plan addresses the issue where selecting an item in `ColumnView` displays basic physical metadata (file name, type, size, modified time) on `MetaPanel`, but fails to show bound associated extended metadata (tags, rating, manual color, note, etc.).

### Root Causes
1. **`DiskItemModel` Initialization**: When `DiskItemModel` loads filesystem records, `rec.rating`, `rec.manualColor`, `rec.tags`, and `rec.note` are unpopulated default values.
2. **`PanelMediator` Routing Trap**: When `PanelMediator` receives `selectionChanged`, `idx.isValid()` evaluates to `true` for `DiskItemModel` indexes. It directly accesses `idx.data(RatingRole)` / `idx.data(TagsRole)` etc., returning empty/default values, which bypasses the `else` fallback branch that queries `MetadataManager`.

### Solution
1. **`DiskItemModel::data` Dynamic Fetch**: In `DiskItemModel::data`, query `MetadataManager::instance().getMeta(path)` for `RatingRole`, `ColorRole`, `TagsRole`, and `NoteRole` if internal record data is unpopulated.
2. **`PanelMediator` Metadata Fallback Validation**: In `PanelMediator::connect` for `selectionChanged`, check if the selected `ModelIndex` provides empty extended metadata (tags, rating, note, color). If so, fallback to `MetadataManager::instance().getMeta(path)` to fill extended attributes for `MetaPanel`.

---

## 2. Modified Files List
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/PanelMediator.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/models/DiskItemModel.cpp`
Dynamically fallback to `MetadataManager` for extended roles (`RatingRole`, `ColorRole`, `NoteRole`) similar to existing `TagsRole` behavior.

```git
<<<<<<< SEARCH
    } else if (role == RatingRole) {
        return record.rating;
    } else if (role == ColorRole) {
        return record.manualColor;
    } else if (role == IsLockedRole || role == PinnedRole) {
        return record.pinned;
    } else if (role == EncryptedRole) {
        return record.encrypted;
    } else if (role == TagsRole) {
        // 如果 record.tags 为空，尝试从 MetadataManager 读取最新数据
        if (record.tags.isEmpty()) {
            std::wstring wpath = path.toStdWString();
            RuntimeMeta meta = MetadataManager::instance().getMeta(wpath);
            if (!meta.tags.isEmpty()) {
                return meta.tags;
            }
        }
        return record.tags;
    } else if (role == NoteRole) {
        return record.note;
=======
    } else if (role == RatingRole) {
        if (record.rating == 0) {
            std::wstring wpath = path.toStdWString();
            RuntimeMeta meta = MetadataManager::instance().getMeta(wpath);
            if (meta.rating > 0) return meta.rating;
        }
        return record.rating;
    } else if (role == ColorRole) {
        if (record.manualColor.isEmpty()) {
            std::wstring wpath = path.toStdWString();
            RuntimeMeta meta = MetadataManager::instance().getMeta(wpath);
            QString colorStr = QString::fromStdWString(meta.manualColor);
            if (!colorStr.isEmpty()) return colorStr;
        }
        return record.manualColor;
    } else if (role == IsLockedRole || role == PinnedRole) {
        return record.pinned;
    } else if (role == EncryptedRole) {
        return record.encrypted;
    } else if (role == TagsRole) {
        // 如果 record.tags 为空，尝试从 MetadataManager 读取最新数据
        if (record.tags.isEmpty()) {
            std::wstring wpath = path.toStdWString();
            RuntimeMeta meta = MetadataManager::instance().getMeta(wpath);
            if (!meta.tags.isEmpty()) {
                return meta.tags;
            }
        }
        return record.tags;
    } else if (role == NoteRole) {
        if (record.note.isEmpty()) {
            std::wstring wpath = path.toStdWString();
            RuntimeMeta meta = MetadataManager::instance().getMeta(wpath);
            QString noteStr = QString::fromStdWString(meta.note);
            if (!noteStr.isEmpty()) return noteStr;
        }
        return record.note;
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/PanelMediator.cpp`
Ensure `PanelMediator` queries `MetadataManager` if `ModelIndex` returns empty extended metadata.

```git
<<<<<<< SEARCH
                if (idx.isValid()) {
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
                } else {
                    auto meta = MetadataManager::instance().getMeta(path.toStdWString());
                    metaPanel->setRating(meta.rating, false);
                    metaPanel->setColor(QString::fromStdWString(meta.manualColor), false);
                    metaPanel->setTags(meta.tags);
                    metaPanel->setNote(QString::fromStdWString(meta.note));
                    metaPanel->setURL(QString::fromStdWString(meta.url));
                    metaPanel->setPalettes(meta.palettes);
                }
=======
                auto meta = MetadataManager::instance().getMeta(path.toStdWString());
                QVector<QPair<QColor, float>> qPalettes;
                qPalettes.reserve(static_cast<int>(meta.palettes.size()));
                for (const auto& entry : meta.palettes) {
                    qPalettes.append(qMakePair(entry.color, entry.ratio));
                }

                if (idx.isValid()) {
                    int rating = idx.data(RatingRole).toInt();
                    QString color = idx.data(ColorRole).toString();
                    QStringList tags = idx.data(TagsRole).toStringList();
                    QString note = idx.data(NoteRole).toString();
                    QString url = idx.data(UrlRole).toString();

                    metaPanel->setRating(rating > 0 ? rating : meta.rating, false);
                    metaPanel->setColor(!color.isEmpty() ? color : QString::fromStdWString(meta.manualColor), false);
                    metaPanel->setTags(!tags.isEmpty() ? tags : meta.tags);
                    metaPanel->setNote(!note.isEmpty() ? note : QString::fromStdWString(meta.note));
                    metaPanel->setURL(!url.isEmpty() ? url : QString::fromStdWString(meta.url));
                    metaPanel->setPalettes(qPalettes);

                    QVariant decData = idx.data(Qt::DecorationRole);
                    QPixmap previewPixmap;
                    if (decData.canConvert<QIcon>()) {
                        previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                    } else if (decData.canConvert<QPixmap>()) {
                        previewPixmap = decData.value<QPixmap>();
                    }
                    metaPanel->setImagePreview(previewPixmap);
                } else {
                    metaPanel->setRating(meta.rating, false);
                    metaPanel->setColor(QString::fromStdWString(meta.manualColor), false);
                    metaPanel->setTags(meta.tags);
                    metaPanel->setNote(QString::fromStdWString(meta.note));
                    metaPanel->setURL(QString::fromStdWString(meta.url));
                    metaPanel->setPalettes(qPalettes);
                }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Build Command
```bash
cmake -B build -G Ninja
cmake --build build
```

### Verification Method
1. Launch QuarkMeta and switch to **Column View (分栏视图)** mode.
2. Select any file/folder item that has previously attached tags, ratings, color labels, or notes in the database.
3. Observe the right-hand **MetaPanel (元数据面板)**.
4. Verify that:
   - Basic metadata (name, size, type, modified time) is rendered instantly.
   - Associated extended metadata (tags, star rating, color pills, notes, EXIF/palettes) is correctly retrieved and displayed.
