# DiskItemModel-5.md - Removing Synchronous Metadata Fallbacks & Cleaning Up Dead Code

## 1. Overview
This implementation plan optimizes `DiskItemModel::data` by removing synchronous fallback calls to `MetadataManager::getMeta` for `RatingRole`, `ColorRole`, `TagsRole`, and `NoteRole`. All metadata values are strictly read from `m_allRecords[index.row()]`, which is kept up-to-date by `updateRecordMetadata` and `MetaCacheDecorator`. Additionally, unused dead token storage `m_genTokens` is removed to simplify generation incrementing.

## 2. Modified Files List
- `src/ui/models/DiskItemModel.h`
- `src/ui/models/DiskItemModel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/models/DiskItemModel.h`

```
<<<<<<< SEARCH
    QMutex m_genTokenMutex;
    QHash<uint64_t, std::shared_ptr<CancellationToken>> m_genTokens;
=======
>>>>>>> REPLACE
```

### `src/ui/models/DiskItemModel.cpp`

```
<<<<<<< SEARCH
void DiskItemModel::incrementGeneration() {
    uint64_t oldGen = m_currentGen.load(std::memory_order_relaxed);
    {
        QMutexLocker locker(&m_genTokenMutex);
        auto it = m_genTokens.find(oldGen);
        if (it != m_genTokens.end()) {
            if (it.value()) it.value()->cancel();
            m_genTokens.erase(it);
        }
    }
    m_currentGen.fetch_add(1, std::memory_order_relaxed);
    ThumbnailPipelineService::instance().cancelAll();
}
=======
void DiskItemModel::incrementGeneration() {
    m_currentGen.fetch_add(1, std::memory_order_relaxed);
    ThumbnailPipelineService::instance().cancelAll();
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
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
    }
=======
    } else if (role == RatingRole) {
        return record.rating;
    } else if (role == ColorRole) {
        return record.manualColor;
    } else if (role == IsLockedRole || role == PinnedRole) {
        return record.pinned;
    } else if (role == EncryptedRole) {
        return record.encrypted;
    } else if (role == TagsRole) {
        return record.tags;
    } else if (role == NoteRole) {
        return record.note;
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
Manual code review confirms that `m_allRecords` acts as the single source of truth for item metadata without synchronous fallbacks during `data()` queries.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
Integrates `ItemRecord` as the SSOT for rendering data, eliminating redundant disk/cache lookups inside `data()`.

## 6. Header API Signature Verification
- `void DiskItemModel::incrementGeneration()`
- `QVariant DiskItemModel::data(const QModelIndex& index, int role) const override`

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "DiskItemModel.h"`
