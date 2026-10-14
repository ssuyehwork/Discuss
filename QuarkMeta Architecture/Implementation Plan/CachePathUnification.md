# Implementation Plan - Single-Level AppData Directory & Unified Cache Structure

## 1. Overview
This implementation plan resolves the path redundancy and fragmented cache directory structure across QuarkMeta:
1. Removes duplicate `a.setOrganizationName("QuarkMeta")` in `src/main.cpp` so that `QStandardPaths::CacheLocation` resolves to a clean single-level path (`C:/Users/<Username>/AppData/Local/QuarkMeta/cache/`) without `QuarkMeta/QuarkMeta` repetition.
2. Unifies all cache directories under the exact same centralized `QStandardPaths::CacheLocation` (`cache/`) root directory:
   - File item thumbnails: `cache/disk_thumbs/` (in `DiskMediaExtractor.cpp`)
   - QuickLook preview cache: `cache/quicklook_previews/` (in `QuickLookWindow.cpp`)
3. Generalizes `QuickLookWindow` preview caching so that all graphic image types (`.png`, `.jpg`, `.jpeg`, `.bmp`, `.gif`, `.svg`, `.eps`, `.ai`, `.psd`, `.psb`, etc.) utilize the unified QuickLook high-resolution preview cache (`cache/quicklook_previews/`).

---

## 2. Modified Files List
- `src/main.cpp`
- `src/util/DiskMediaExtractor.cpp`
- `src/ui/QuickLookWindow.h`
- `src/ui/QuickLookWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/main.cpp`
Remove redundant `setOrganizationName` call to eliminate double `QuarkMeta/QuarkMeta` directory nesting:

```cpp
<<<<<<< SEARCH
    a.setApplicationName("QuarkMeta");
    a.setOrganizationName("QuarkMeta");
=======
    a.setApplicationName("QuarkMeta");
>>>>>>> REPLACE
```

### 3.2 `src/util/DiskMediaExtractor.cpp`
Update thumbnail cache path to use `QStandardPaths::CacheLocation` so that all thumbnails reside inside the exact same `cache/` root folder:

```cpp
<<<<<<< SEARCH
    QString cacheDir = QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/" + volStr + "/" + bucket;
=======
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/disk_thumbs/" + volStr + "/" + bucket;
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    return QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/fallback/" + bucket + "/" + fileKey;
=======
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/disk_thumbs/fallback/" + bucket + "/" + fileKey;
>>>>>>> REPLACE
```

### 3.3 `src/ui/QuickLookWindow.h`
Add `loadOrExtractQuickLookImage` static helper declaration in `QuickLookWindow.h`:

```cpp
<<<<<<< SEARCH
    QString detectEncoding(const QByteArray& data);
    bool isBinary(const QByteArray& data);
    static QImage loadOrExtractQuickLookEps(const QString& filePath, int targetSize, std::shared_ptr<CancellationToken> token);
=======
    QString detectEncoding(const QByteArray& data);
    bool isBinary(const QByteArray& data);
    static QImage loadOrExtractQuickLookImage(const QString& filePath, int targetSize, std::shared_ptr<CancellationToken> token);
    static QImage loadOrExtractQuickLookEps(const QString& filePath, int targetSize, std::shared_ptr<CancellationToken> token);
>>>>>>> REPLACE
```

### 3.4 `src/ui/QuickLookWindow.cpp`
Update QuickLook cache path to `cache/quicklook_previews` and unify image preview caching:

```cpp
<<<<<<< SEARCH
        QImage img;
        if (ext == "svg") {
            QSvgRenderer renderer(path);
            if (renderer.isValid()) {
                img = QImage(2048, 2048, QImage::Format_ARGB32);
                img.fill(Qt::transparent);
                QPainter painter(&img);
                renderer.render(&painter);
            }
        } else if (ext == "eps") {
            // 物理隔离与 144 DPI 高清矢量：专走 QuickLook 独立管道与独立缓存
            img = loadOrExtractQuickLookEps(path, 2048, token);
        } else if (ext == "ai" || ext == "psd" || ext == "psb") {
            // 预览档：不读也不写缩略图缓存，不碰失败标记
            img = ImageDecoderFacade::decodeSinglePass(path, 2048, ImageDecoderFacade::ExtractMode::Preview, token).thumbnail512;
        } else if (QT_NATIVE_FORMATS.contains(ext)) {
            img.load(path);
        } else {
            img = ImageDecoderFacade::decodeSinglePass(path, 2048, ImageDecoderFacade::ExtractMode::Preview, token).thumbnail512;
        }
=======
        QImage img = loadOrExtractQuickLookImage(path, 2048, token);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
QImage QuickLookWindow::loadOrExtractQuickLookEps(const QString& filePath, int targetSize, std::shared_ptr<CancellationToken> token) {
    // 🚀【物理目录隔离铁律】：QuickLook 144 DPI 大图绝对不与缩略图共用文件夹 (使用 SHA256 与两级分桶)
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString dirL1 = hashStr.left(2);
    QString dirL2 = hashStr.mid(2, 2);

    QString baseCacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/quicklook_previews_v2";
    QString cacheDir = QDir(baseCacheDir).filePath(QString("%1/%2").arg(dirL1, dirL2));
    QDir().mkpath(cacheDir);

    QFileInfo srcInfo(filePath);
    QString cachePath = QDir(cacheDir).filePath(QString("%1.png").arg(hashStr));
    QFileInfo cacheInfo(cachePath);

    // 1. 若独立缓存命中且未过期，直接毫秒载入 144 DPI 大图
    if (cacheInfo.exists() && cacheInfo.size() > 0 && cacheInfo.lastModified() >= srcInfo.lastModified()) {
        QImage cachedImg(cachePath);
        if (!cachedImg.isNull()) {
            return cachedImg;
        }
    }

    // 2. 独立缓存未命中：Ghostscript 144 DPI 矢量优先，内嵌预览兜底（长超时 + 可取消）
    bool fromGhostscript = false;
    QImage highQImg = FormatDecoders::extractEpsQuickLook(filePath, targetSize, ImageDecoderFacade::kLongTimeoutMs, token, &fromGhostscript);
    if (highQImg.isNull()) {
        return QImage();
    }

    // 3. 只缓存 Ghostscript 高清渲染结果；内嵌预览兜底图（低质量）只显示，不进高清缓存
    if (fromGhostscript) {
        highQImg.save(cachePath, "PNG");
    }
    return highQImg;
}
=======
QImage QuickLookWindow::loadOrExtractQuickLookEps(const QString& filePath, int targetSize, std::shared_ptr<CancellationToken> token) {
    return loadOrExtractQuickLookImage(filePath, targetSize, token);
}

QImage QuickLookWindow::loadOrExtractQuickLookImage(const QString& filePath, int targetSize, std::shared_ptr<CancellationToken> token) {
    // 🚀【全图形图像 QuickLook 统一缓存架构】：使用 SHA256 与两级分桶，收拢于 cache/quicklook_previews/
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString dirL1 = hashStr.left(2);
    QString dirL2 = hashStr.mid(2, 2);

    QString baseCacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/quicklook_previews";
    QString cacheDir = QDir(baseCacheDir).filePath(QString("%1/%2").arg(dirL1, dirL2));
    QDir().mkpath(cacheDir);

    QFileInfo srcInfo(filePath);
    QString cachePath = QDir(cacheDir).filePath(QString("%1.png").arg(hashStr));
    QFileInfo cacheInfo(cachePath);

    // 1. 若独立缓存命中且未过期，直接 0ms 秒级载入高清预览图
    if (cacheInfo.exists() && cacheInfo.size() > 0 && cacheInfo.lastModified() >= srcInfo.lastModified()) {
        QImage cachedImg(cachePath);
        if (!cachedImg.isNull()) {
            return cachedImg;
        }
    }

    // 2. 缓存未命中：按图形格式提取高清 2048px 预览图
    QString ext = srcInfo.suffix().toLower();
    QImage highQImg;
    bool shouldSaveCache = true;

    if (ext == "eps") {
        bool fromGhostscript = false;
        highQImg = FormatDecoders::extractEpsQuickLook(filePath, targetSize, ImageDecoderFacade::kLongTimeoutMs, token, &fromGhostscript);
        if (!fromGhostscript) shouldSaveCache = false; // 仅缓存高质量 Ghostscript 渲染
    } else if (ext == "svg") {
        QSvgRenderer renderer(filePath);
        if (renderer.isValid()) {
            highQImg = QImage(targetSize, targetSize, QImage::Format_ARGB32);
            highQImg.fill(Qt::transparent);
            QPainter painter(&highQImg);
            renderer.render(&painter);
        }
    } else {
        static const QSet<QString> nativeFormats = {"png", "jpg", "jpeg", "bmp", "gif"};
        if (nativeFormats.contains(ext)) {
            highQImg.load(filePath);
        } else {
            highQImg = ImageDecoderFacade::decodeSinglePass(filePath, targetSize, ImageDecoderFacade::ExtractMode::Preview, token).thumbnail512;
        }
    }

    if (highQImg.isNull()) {
        return QImage();
    }

    // 3. 将高质量预览图缓存至 cache/quicklook_previews/
    if (shouldSaveCache) {
        highQImg.save(cachePath, "PNG");
    }
    return highQImg;
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile QuarkMeta target using CMake:
   `cmake --build build`
2. Launch QuarkMeta app on Windows.
3. Check AppData Cache directory:
   - Confirm that the AppData path is now a clean single level: `AppData/Local/QuarkMeta/cache/`.
   - Confirm that file item thumbnails are saved in `AppData/Local/QuarkMeta/cache/disk_thumbs/`.
   - Confirm that QuickLook preview cache is generated in `AppData/Local/QuarkMeta/cache/quicklook_previews/`.
4. Test QuickLook preview by pressing `Spacebar` on various image files (`.png`, `.jpg`, `.svg`, `.eps`, `.psd`, `.ai`).

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Cache SSOT Unification**: Both `disk_thumbs` and `quicklook_previews` are now 100% unified under the exact same `QStandardPaths::CacheLocation` (`AppData/Local/QuarkMeta/cache/`) root directory.
- **Single AppName SSOT**: `QApplication::setApplicationName("QuarkMeta")` acts as the single SSOT for the application name without redundant `OrganizationName` nesting.

---

## 6. Header API Signature Verification
| Class / Function Signature | Declared Header File | Signature Match Verification |
| :--- | :--- | :--- |
| `static QImage loadOrExtractQuickLookImage(const QString& filePath, int targetSize, std::shared_ptr<CancellationToken> token)` | `src/ui/QuickLookWindow.h` | 100% Exact Match |
| `static QImage loadOrExtractQuickLookEps(const QString& filePath, int targetSize, std::shared_ptr<CancellationToken> token)` | `src/ui/QuickLookWindow.h` | 100% Exact Match |
| `QString getDiskThumbCachePath(const QString& filePath)` | `src/util/DiskMediaExtractor.h` | 100% Exact Match |

---

## 7. Header Inclusion Chain & Type Completeness Check
| Inclusion / Type Name | File | Purpose & Completeness Check |
| :--- | :--- | :--- |
| `#include <QStandardPaths>` | `src/ui/QuickLookWindow.cpp`, `src/util/DiskMediaExtractor.cpp` | Provides `QStandardPaths::writableLocation(QStandardPaths::CacheLocation)`. |
| `#include "CoreEngine.h"` | `src/ui/QuickLookWindow.h` | Provides `CancellationToken` smart pointer type definition. |
