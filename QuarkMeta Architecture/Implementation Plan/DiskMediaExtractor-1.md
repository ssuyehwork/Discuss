# DiskMediaExtractor Performance Optimization Implementation Plan

## Overview
Optimize `DiskMediaExtractor::getCapsuleExtractResult` to eliminate synchronous `.QuarkMeta.json` disk loading and JSON parsing when thumbnail disk cache hits.

## Changes

```path
src/util/DiskMediaExtractor.cpp
```

<<<<<<< SEARCH
    // 1. 极速缓存命中路径：若磁盘已存在缩略图缓存，免解码瞬间返回
    if (!res.thumbnail512.isNull()) {
        res.isValid = true;

        std::lock_guard<std::mutex> lock(s_jsonSaveMutex);
        QuarkMetaJson jsonCache(parentDir.toStdWString());
        jsonCache.load();
        const auto& cachedItems = jsonCache.items();
        std::wstring wFileName = fileName.toStdWString();
        auto it = cachedItems.find(wFileName);
        if (it != cachedItems.end() && it->second.width > 0 && it->second.height > 0) {
            res.originalSize = QSize(it->second.width, it->second.height);
        }
        return res;
    }
=======
    // 1. 极速缓存命中路径：若磁盘已存在缩略图缓存，免解码瞬间返回
    if (!res.thumbnail512.isNull()) {
        res.isValid = true;
        return res;
    }
>>>>>>> REPLACE
