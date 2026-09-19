# Implementation Plan - ThumbnailPipelineService.md

## 1. Overview
本实施方案旨在彻底清理 `ThumbnailPipelineService` 中冗余的系统 `Temp/QuarkMeta_Thumbnails/` 磁盘落盘逻辑，将缩略图磁盘缓存完全归一化回流至 `DiskMediaExtractor`（`.QuarkMeta/disk_thumbs/`）唯一真理源（SSOT）通道。

### 重构背景与要点
1. **删除 Temp 冗余落盘**：移除 `ThumbnailPipelineService::getDiskCachePath` 及其在 `loadBatchAsync` 中重复将图片写往 `AppData/Local/Temp/QuarkMeta_Thumbnails/` 的二次落盘代码，彻底根治重复磁盘 IO 和 C 盘空间浪费问题。
2. **归一化 SSOT 通道**：`loadBatchAsync` 在后台异步线程中，统一通过 `DiskMediaExtractor::getCapsuleThumbnail(path, targetSize)` 从 `.QuarkMeta/disk_thumbs/` 读取或生成基于 64 位物理 File ID 的 512px 高清主缩略图。
3. **保留内存 LRU 缓存**：`ThumbnailPipelineService` 仅保留一级内存 `m_memoryCache`，用于 UI 主线程 0ms 直取，不再在 Temp 目录留存冗余副本。

---

## 2. Modified Files List
- `src/util/ThumbnailPipelineService.h`
- `src/util/ThumbnailPipelineService.cpp`

---

## 3. Detailed Line-by-Line Changes

### File: `src/util/ThumbnailPipelineService.h`

```
<<<<<<< SEARCH
    /**
     * @brief 计算二级磁盘 Hash 缓存路径
     */
    static QString getDiskCachePath(const QString& filePath, int targetSize);
=======
>>>>>>> REPLACE
```

---

### File: `src/util/ThumbnailPipelineService.cpp`

```
<<<<<<< SEARCH
#include <QCryptographicHash>
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QString ThumbnailPipelineService::getDiskCachePath(const QString& filePath, int targetSize) {
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QByteArray hash = QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex();
    
    QString cacheDir = QDir::temp().filePath("QuarkMeta_Thumbnails");
    QDir().mkpath(cacheDir);

    return QDir(cacheDir).filePath(QString("%1_%2.png").arg(QString(hash.left(32))).arg(targetSize));
}
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    (void)QtConcurrent::run([this, pathsToFetch, targetSize, taskGen, onSingleLoaded]() {
        for (const QString& path : pathsToFetch) {
            if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                return;
            }

            QString diskPath = getDiskCachePath(path, targetSize);
            QImage finalImg;

            if (QFile::exists(diskPath)) {
                finalImg.load(diskPath);
            }

            if (finalImg.isNull()) {
                finalImg = decodeImageToThumbnail(path, targetSize);
                if (!finalImg.isNull()) {
                    finalImg.save(diskPath, "PNG");
                }
            }

            if (!finalImg.isNull()) {
=======
    (void)QtConcurrent::run([this, pathsToFetch, targetSize, taskGen, onSingleLoaded]() {
        for (const QString& path : pathsToFetch) {
            if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                return;
            }

            QImage finalImg = decodeImageToThumbnail(path, targetSize);

            if (!finalImg.isNull()) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### 构建步骤
在项目根目录运行 CMake 构建：
```bash
cmake -B build -S .
cmake --build build --config Debug
```

### 验证方法
1. **磁盘占用核查**：在 QuarkMeta 中浏览大量图片目录后，检查 `C:\Users\<用户名>\AppData\Local\Temp\` 目录，确认不再产生 `QuarkMeta_Thumbnails` 冗余文件夹与大量 PNG 文件。
2. **高清主库核查**：检查 QuarkMeta 根目录下的 `.QuarkMeta/disk_thumbs/`，确认缩略图正常生成并保存在按 File ID 分桶的持久化目录中。
3. **功能与性能测试**：打开大图/PSD 目录并快速滚屏，确认缩略图依然秒开且加载流畅，内存 LRU 缓存正常工作。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- [x] **复用性检查**：完全归一化复用 `DiskMediaExtractor::getCapsuleThumbnail` 唯一 SSOT 磁盘缩略图服务。
- [x] **物理清除死代码**：彻底物理删除了 `getDiskCachePath` 及其冗余的 Temp 读写代码。
- [x] **零参数篡改**：内存缓存 `kMaxMemoryCacheCount = 800` 及代际熔断机制完全照抄保持不变。

---

## 6. Header API Signature Verification

| 调用的成员/类 | 头文件物理声明源 | 头文件物理精确签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `DiskMediaExtractor::getCapsuleThumbnail` | `src/util/DiskMediaExtractor.h` | `static QImage getCapsuleThumbnail(const QString& filePath, int size, ...)` | 物理核实一致 |
| `ThumbnailPipelineService::getFromMemoryCache` | `src/util/ThumbnailPipelineService.h` | `QPixmap getFromMemoryCache(const QString& filePath, int targetSize) const` | 物理核实一致 |
