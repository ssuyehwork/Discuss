# Implementation Plan - Cache Bucketing and SHA256 Normalization

## Overview
This implementation plan unifies thumbnail and preview cache storage across `QuickLookWindow` and `DiskMediaExtractor`. Both modules will strictly adhere to:
1. **SHA256 Hash Algorithm**: Eliminating legacy MD5 usage in `QuickLookWindow` and standardizing on SHA256 hashing based on normalized lower-case native paths.
2. **Two-Level Subfolder Bucketing**: Organizing cached files into a 2-level directory tree (`<L1>/<L2>/`) using the first 4 characters of the SHA256 hex digest (e.g. `a1/b2/`), preventing single-directory file overload on disk.

---

## Modified Files List
- `src/ui/QuickLookWindow.cpp`
- `src/util/DiskMediaExtractor.cpp`

---

## Detailed Line-by-Line Changes

### 1. Update `QuickLookWindow.cpp` to use SHA256 and Two-Level Bucketing
```cpp
<<<<<<< SEARCH
    // 🚀【物理目录隔离铁律】：QuickLook 144 DPI 大图绝对不与缩略图共用文件夹
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/quicklook_previews";
    QDir().mkpath(cacheDir);

    QFileInfo srcInfo(filePath);
    QString hashKey = QString::fromLatin1(QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex());
    QString cachePath = QString("%1/%2.png").arg(cacheDir, hashKey);
=======
    // 🚀【物理目录隔离铁律】：QuickLook 144 DPI 大图绝对不与缩略图共用文件夹 (使用 SHA256 与两级分桶)
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString dirL1 = hashStr.left(2);
    QString dirL2 = hashStr.mid(2, 2);

    QString baseCacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/quicklook_previews";
    QString cacheDir = QDir(baseCacheDir).filePath(QString("%1/%2").arg(dirL1, dirL2));
    QDir().mkpath(cacheDir);

    QFileInfo srcInfo(filePath);
    QString cachePath = QDir(cacheDir).filePath(QString("%1.png").arg(hashStr));
>>>>>>> REPLACE
```

### 2. Update `DiskMediaExtractor.cpp` to use Two-Level Bucketing
```cpp
<<<<<<< SEARCH
QString DiskMediaExtractor::getDiskThumbCachePath(const QString& filePath) {
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString cacheDir = QDir::temp().filePath("QuarkMeta_Thumbnails");
    return QDir(cacheDir).filePath(QString("%1_230.png").arg(hashStr.left(32)));
}
=======
QString DiskMediaExtractor::getDiskThumbCachePath(const QString& filePath) {
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString dirL1 = hashStr.left(2);
    QString dirL2 = hashStr.mid(2, 2);

    QString baseCacheDir = QDir::temp().filePath("QuarkMeta_Thumbnails");
    QString cacheDir = QDir(baseCacheDir).filePath(QString("%1/%2").arg(dirL1, dirL2));
    return QDir(cacheDir).filePath(QString("%1_230.png").arg(hashStr.left(32)));
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Verify header inclusion and type completeness for `QCryptographicHash`, `QDir`, `QFileInfo`, `QStandardPaths`.
2. Inspect directory creation logic (`QDir().mkpath(...)`) in both functions to confirm two-level subdirectories are dynamically created before saving cache files.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- `DiskMediaExtractor::getDiskThumbCachePath` remains the SSOT entry point for disk thumbnail cache paths.
- `QuickLookWindow::loadOrExtractQuickLookEps` uses the unified SHA256 + 2-level bucketing pattern while keeping its isolated `quicklook_previews` root folder.

---

## Header API Signature Verification
- `QCryptographicHash::hash(const QByteArray &key, QCryptographicHash::Algorithm method)`
- `QCryptographicHash::Sha256` (enum value in `QCryptographicHash`)
- `QDir::toNativeSeparators(const QString &pathName)`
- `QDir::filePath(const QString &fileName) const`
- `QDir::mkpath(const QString &dirPath) const`

---

## Header Inclusion Chain & Type Completeness Check
- `QuickLookWindow.cpp` includes `<QCryptographicHash>`, `<QStandardPaths>`, `<QDir>`, `<QFileInfo>`.
- `DiskMediaExtractor.cpp` includes `<QCryptographicHash>`, `<QDir>`, `<QFileInfo>`.
