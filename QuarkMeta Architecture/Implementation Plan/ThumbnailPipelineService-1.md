# ThumbnailPipelineService Memory Cache Limit Expansion Plan

## Overview
Increase `kMaxMemoryCacheCount` in `ThumbnailPipelineService.h` from 800 to 3000 to prevent frequent memory cache eviction in dense image directories.

## Changes

```path
src/util/ThumbnailPipelineService.h
```

<<<<<<< SEARCH
    static constexpr int kMaxMemoryCacheCount = 800; // 内存最多缓存 800 张缩略图 (约 50~80MB)
=======
    static constexpr int kMaxMemoryCacheCount = 3000; // 内存最多缓存 3000 张缩略图 (约 200MB)，大幅提升高密度相册滚动效率
>>>>>>> REPLACE
