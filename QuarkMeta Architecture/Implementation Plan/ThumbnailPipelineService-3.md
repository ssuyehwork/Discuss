# Implementation Plan - ThumbnailPipelineService-3

## Overview
This implementation plan addresses thumbnail extraction background congestion and enables immediate process/task cancellation (aborting Ghostscript processes within 10-50ms upon directory change or navigation) in `ThumbnailPipelineService`:
1. **Cancellation Token Integration**: Maintains a thread-safe `std::shared_ptr<CancellationToken>` for current async batches and passes it through `DiskMediaExtractor::getCapsuleExtractResult` down to `FormatDecoders::renderGhostscriptSafely`.
2. **Immediate Abort on `incrementGeneration()` / `cancelAll()`**: Triggers `m_currentToken->cancel()` when generation increments, instantly killing active Ghostscript worker processes and aborting pending decodes.
3. **Disk Cache Persistence**: Saves successfully extracted thumbnails to `.QuarkMeta/thumbnails/` via `DiskMediaExtractor::saveCapsuleThumbnail` to prevent redundant re-renders on subsequent folder visits.

## Modified Files List
- `src/util/ThumbnailPipelineService.h`
- `src/util/ThumbnailPipelineService.cpp`

## Detailed Line-by-Line Changes

### `src/util/ThumbnailPipelineService.h`

```
<<<<<<< SEARCH
    /**
     * @brief 递增代际号并瞬间熔断所有正在排队的旧任务
     */
    void incrementGeneration();
    void cancelAll();
=======
    /**
     * @brief 递增代际号并瞬间熔断所有正在排队的旧任务
     */
    void incrementGeneration();
    void cancelAll();

    std::shared_ptr<CancellationToken> currentToken() const;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    std::atomic<uint64_t> m_currentGeneration{0};
    mutable QMutex m_cacheMutex;
=======
    std::atomic<uint64_t> m_currentGeneration{0};
    mutable QMutex m_cacheMutex;
    std::shared_ptr<CancellationToken> m_currentToken;
>>>>>>> REPLACE
```

### `src/util/ThumbnailPipelineService.cpp`

```
<<<<<<< SEARCH
void ThumbnailPipelineService::incrementGeneration() {
    m_currentGeneration.fetch_add(1, std::memory_order_relaxed);
}
=======
void ThumbnailPipelineService::incrementGeneration() {
    m_currentGeneration.fetch_add(1, std::memory_order_relaxed);
    QMutexLocker locker(&m_cacheMutex);
    if (m_currentToken) {
        m_currentToken->cancel();
        m_currentToken.reset();
    }
}

std::shared_ptr<CancellationToken> ThumbnailPipelineService::currentToken() const {
    QMutexLocker locker(&m_cacheMutex);
    return m_currentToken;
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ThumbnailPipelineService::loadBatchAsync(const QStringList& filePaths,
                                              int targetSize,
                                              std::function<void(const QString& path, const QPixmap& pixmap)> onSingleLoaded) {
    if (filePaths.isEmpty()) return;

    uint64_t taskGen = m_currentGeneration.load(std::memory_order_relaxed);
=======
void ThumbnailPipelineService::loadBatchAsync(const QStringList& filePaths,
                                              int targetSize,
                                              std::function<void(const QString& path, const QPixmap& pixmap)> onSingleLoaded) {
    if (filePaths.isEmpty()) return;

    std::shared_ptr<CancellationToken> token;
    {
        QMutexLocker locker(&m_cacheMutex);
        if (!m_currentToken || m_currentToken->isCanceled()) {
            m_currentToken = std::make_shared<CancellationToken>();
        }
        token = m_currentToken;
    }

    uint64_t taskGen = m_currentGeneration.load(std::memory_order_relaxed);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            QImage finalImg = DiskMediaExtractor::getCapsuleThumbnailReadOnly(path);
            if (finalImg.isNull()) {
                qDebug() << "[THUMB_TRACE] ReadOnly cache miss in pipeline, calling decodeImageToThumbnail for:" << path;
                finalImg = decodeImageToThumbnail(path, targetSize);
            } else {
                qDebug() << "[THUMB_TRACE] ReadOnly cache hit in pipeline for:" << path;
            }
=======
            if (token && token->isCanceled()) return;

            QImage finalImg = DiskMediaExtractor::getCapsuleThumbnailReadOnly(path);
            if (finalImg.isNull()) {
                qDebug() << "[THUMB_TRACE] ReadOnly cache miss in pipeline, decoding for:" << path;
                auto res = DiskMediaExtractor::getCapsuleExtractResult(path, targetSize, token);
                finalImg = res.thumbnail;
                if (!finalImg.isNull()) {
                    DiskMediaExtractor::saveCapsuleThumbnail(path, finalImg);
                }
            } else {
                qDebug() << "[THUMB_TRACE] ReadOnly cache hit in pipeline for:" << path;
            }
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Recompile `src/util/ThumbnailPipelineService.cpp`.
2. Test directory switching during heavy EPS/AI thumbnail extraction and confirm Ghostscript processes are killed within 50ms.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `DiskMediaExtractor::getCapsuleExtractResult` and `CancellationToken` without creating parallel cancellation mechanisms.

## Header API Signature Verification
- `DiskMediaExtractor::getCapsuleExtractResult(const QString&, int, std::shared_ptr<CancellationToken>)` -> exact signature in `DiskMediaExtractor.h`.
- `DiskMediaExtractor::saveCapsuleThumbnail(const QString&, const QImage&)` -> exact signature in `DiskMediaExtractor.h`.

## Header Inclusion Chain Check
- Ensure `#include "CoreEngine.h"` (defining `CancellationToken`) is included in `ThumbnailPipelineService.h`.
