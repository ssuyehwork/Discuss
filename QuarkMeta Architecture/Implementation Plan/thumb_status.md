# Thumbnail Extraction Failure Status Persistence & Filter Integration Implementation Plan (thumb_status.md)

## Overview
This implementation plan establishes a persistent failure/skip tracking mechanism (`thumb_status: 1`) for image/graphics thumbnail extraction in QuarkMeta's pure disk mode. 

When a thumbnail extraction task fails (e.g., due to corrupted image headers, unsupported codecs, or rendering process errors) and is not interrupted by explicit cancellation, the failure status is persisted directly into the directory's `.QuarkMeta.json` metadata file as `"thumb_status": 1`. Subsequent folder views use this status to bypass intensive decoding routines, while the FilterPanel leverages it to allow users to batch-filter all failed/skipped thumbnail items.

## Modified Files List
1. `src/meta/MetadataDefs.h`
2. `src/meta/QuarkMetaJson.cpp`
3. `src/core/ItemRecord.h`
4. `src/util/DiskMediaExtractor.cpp`
5. `src/ui/models/DiskItemModel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/meta/MetadataDefs.h`

<<<<<<< SEARCH
    int width;              // 2026-07-xx 1:1对等：图像宽度
    int height;             // 2026-07-xx 1:1对等：图像高度

    ItemMeta()
        : type(L"file")
        , rating(0)
        , pinned(false)
        , encrypted(false)
        , ingestionStatus(-1)
        , size(0)
        , creationTime(0)
        , modificationTime(0)
        , accessTime(0)
        , addedAt(0)
        , width(0)
        , height(0)
    {}

    bool hasUserOperations() const {
        return rating > 0 || !color.empty() || !tags.empty() || pinned ||
               !note.empty() || !url.empty() || encrypted || !folderId.empty() || !palettes.empty() ||
               !autoColor.empty() || addedAt > 0 || width > 0 || height > 0;
    }
=======
    int width;              // 2026-07-xx 1:1对等：图像宽度
    int height;             // 2026-07-xx 1:1对等：图像高度
    int thumbStatus;        // 0: 正常/未处理, 1: 提取失败/跳过

    ItemMeta()
        : type(L"file")
        , rating(0)
        , pinned(false)
        , encrypted(false)
        , ingestionStatus(-1)
        , size(0)
        , creationTime(0)
        , modificationTime(0)
        , accessTime(0)
        , addedAt(0)
        , width(0)
        , height(0)
        , thumbStatus(0)
    {}

    bool hasUserOperations() const {
        return rating > 0 || !color.empty() || !tags.empty() || pinned ||
               !note.empty() || !url.empty() || encrypted || !folderId.empty() || !palettes.empty() ||
               !autoColor.empty() || addedAt > 0 || width > 0 || height > 0 || thumbStatus > 0;
    }
>>>>>>> REPLACE

---

### 2. `src/meta/QuarkMetaJson.cpp`

<<<<<<< SEARCH
    if (meta.width > 0) obj["width"] = meta.width;
    if (meta.height > 0) obj["height"] = meta.height;
    return obj;
=======
    if (meta.width > 0) obj["width"] = meta.width;
    if (meta.height > 0) obj["height"] = meta.height;
    if (meta.thumbStatus > 0) obj["thumb_status"] = meta.thumbStatus;
    return obj;
>>>>>>> REPLACE

<<<<<<< SEARCH
    meta.addedAt = static_cast<long long>(obj.value("added_at").toDouble(0));
    meta.width = obj.value("width").toInt(0);
    meta.height = obj.value("height").toInt(0);
=======
    meta.addedAt = static_cast<long long>(obj.value("added_at").toDouble(0));
    meta.width = obj.value("width").toInt(0);
    meta.height = obj.value("height").toInt(0);
    meta.thumbStatus = obj.value("thumb_status").toInt(0);
>>>>>>> REPLACE

---

### 3. `src/core/ItemRecord.h`

<<<<<<< SEARCH
    int width = 0;
    int height = 0;
=======
    int width = 0;
    int height = 0;
    int thumbStatus = 0; // 0: 正常/未处理, 1: 缩略图提取失败/跳过
>>>>>>> REPLACE

---

### 4. `src/util/DiskMediaExtractor.cpp`

<<<<<<< SEARCH
    if (res.image.isNull()) {
        return res;
    }
=======
    if (res.image.isNull()) {
        if (!token || !token->isCancelled()) {
            QuarkMeta::QuarkMetaJson::updateItemMeta(filePath.toStdWString(), [](QuarkMeta::ItemMeta& meta) {
                meta.thumbStatus = 1;
            });
        }
        return res;
    }
>>>>>>> REPLACE

---

### 5. `src/ui/models/DiskItemModel.cpp`

<<<<<<< SEARCH
            if (token->isCancelled()) return;

            DiskMediaExtractor::ExtractResult res = DiskMediaExtractor::getCapsuleExtractResult(path, 512, token);
=======
            if (token->isCancelled()) return;

            if (rec.thumbStatus == 1) {
                // Skip re-extraction for items previously marked as failed
                return;
            }

            DiskMediaExtractor::ExtractResult res = DiskMediaExtractor::getCapsuleExtractResult(path, 512, token);
>>>>>>> REPLACE

---

## Build & Verification Steps

1. **Compilation Check**:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --config Debug
   ```

2. **Functional Verification**:
   - Place a corrupted image file (e.g. invalid PNG/JPG header) in a directory.
   - Navigate into the directory using QuarkMeta.
   - Verify that `.QuarkMeta.json` is updated with `"thumb_status": 1` for the corrupted file.
   - Re-visit the folder and confirm no re-extraction overhead or process stalls occur.
   - In FilterPanel, confirm filtering options accurately isolate items where `thumbStatus == 1`.
