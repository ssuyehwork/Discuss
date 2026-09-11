# ColumnViewWidget & PanelMediator SSOT Sync & Path Normalization Plan (ColumnViewWidget-14.md)

## Overview
This implementation plan addresses the 4 critical architecture issue points identified during analysis of Column View (`ColumnViewWidget`), `PanelMediator`, and `MetaPanel` integration:
1. **Path Normalization Failure (Slash Mismatch)**: Qt scanner outputs forward slashes (`/`), while SQLite `MetadataManager` uses native backslashes (`\`). Querying `MetadataManager` directly with forward slashes yields empty `RuntimeMeta`.
2. **Ternary Operator Overwriting SSOT Data**: `PanelMediator` mistakenly trusted incomplete/empty role data in unpopulated `QModelIndex`es (`idx.isValid() ? idx.data(...) : meta`), overriding valid SSOT metadata retrieved from `MetadataManager`.
3. **Single-Column `QListView` Property Extraction Overflow**: Calling `idx.sibling(idx.row(), 5)` or `(..., 6)` on single-column `QListView` in `ColumnViewPane` returns invalid indexes, rendering size and modification date as `-`.
4. **Model Signal Disconnection in Column View**: `PanelMediator` was only connected to `contentPanel->model()` (the main grid/tree model), leaving each `ColumnViewPane`'s newly allocated `DiskItemModel` isolated.

## Modified Files List
- `src/ui/PanelMediator.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/PanelMediator.cpp`

Normalize path slashes to Windows native separators, fallback to `QFileInfo` for physical properties, and prioritize `MetadataManager` SSOT metadata over `idx` role values.

```cpp
<<<<<<< SEARCH
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
=======
                // 1. 强制统一为 Windows 原生标准路径（绝不允许正斜杠去查库）
                QString nativePath = QDir::toNativeSeparators(QDir::cleanPath(path));
                QFileInfo fi(nativePath);

                // 2. 修复单列 QListView 导致的 sibling(5) / sibling(6) 越界取空问题
                QString name = fi.fileName();
                QString type = fi.isDir() ? "文件夹" : (fi.suffix().isEmpty() ? "文件" : fi.suffix().toUpper() + " 文件");
                QString sizeStr = fi.isDir() ? "-" : UiHelper::formatFileSize(fi.size());
                QString mtimeStr = fi.lastModified().toString("yyyy-MM-dd hh:mm");

                // 3. 权威 SSOT 查询（必须使用规范化后的 nativePath）
                RuntimeMeta meta = MetadataManager::instance().getMeta(nativePath.toStdWString());

                // 4. 彻底干掉盲信 idx 的三元运算符：优先采用数据库权威值，只有当 DB 为空时才降级读 idx
                int rating   = (meta.rating > 0) ? meta.rating : (idx.isValid() ? idx.data(RatingRole).toInt() : 0);
                QString color= !meta.manualColor.empty() ? QString::fromStdWString(meta.manualColor) : (idx.isValid() ? idx.data(ColorRole).toString() : "");
                QStringList tags = !meta.tags.isEmpty() ? meta.tags : (idx.isValid() ? idx.data(TagsRole).toStringList() : QStringList());
                QString note = !meta.note.empty() ? QString::fromStdWString(meta.note) : (idx.isValid() ? idx.data(NoteRole).toString() : "");
                QString url  = !meta.url.empty() ? QString::fromStdWString(meta.url) : (idx.isValid() ? idx.data(UrlRole).toString() : "");

                // 5. 绑定渲染至面板
                metaPanel->updateInfo(
                    name, type, sizeStr, "-", mtimeStr, "-",
                    nativePath, meta.encrypted, meta.width, meta.height
                );
                metaPanel->setRating(rating, false);
                metaPanel->setColor(color, false);
                metaPanel->setTags(tags);
                metaPanel->setNote(note);
                metaPanel->setURL(url);
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Build using standard CMake / MSVC build pipeline in sandbox.
2. Select files in Column View and verify that `MetaPanel` correctly displays physical properties (Size, Modification Time), Rating, Color tag, Tags, Note, and URL without empty placeholders.
