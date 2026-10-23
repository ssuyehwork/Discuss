# ThumbnailPipelineService-2.md Implementation Plan

## 1. Overview
This implementation plan modifies the thumbnail storage location back to the system temporary directory format used in `Version-Old-8`:
`C:\Users\<Username>\AppData\Local\Temp\QuarkMeta_Thumbnails\<hash32>_<size>.png`

### Changes Made:
1. **Redirect Disk Cache Location**: Modify `DiskMediaExtractor::getDiskThumbCachePath` to output paths inside `QDir::temp().filePath("QuarkMeta_Thumbnails")`.
2. **File Naming Format Alignment**: Format filename as `<32-char-SHA256>_230.png` (or specified target size), matching `Version-Old-8` conventions.

### Architecture 3-Question Answers (架构三问):
1. **SSOT Source**: The thumbnail disk cache location is centralized inside `DiskMediaExtractor::getDiskThumbCachePath`. All callers (`ThumbnailPipelineService`, batch rename commands, etc.) query this single method.
2. **Black-Box Integrity**: Public `.h` header API signatures are untouched and remain frozen.
3. **Root Cause vs Symptom**: Directly updates the SSOT path generator to satisfy user environment requirements without introducing duplicate path logic across callers.

---

## 2. Modified Files List
- `src/util/DiskMediaExtractor.cpp`

---

## 3. Detailed Line-by-Line Changes

### File: `src/util/DiskMediaExtractor.cpp`
Redirects thumbnail storage directory from `.QuarkMeta/disk_thumbs/` to `QDir::temp().filePath("QuarkMeta_Thumbnails")` with `Version-Old-8` style naming (`<hash32>_230.png`).

```
<<<<<<< SEARCH
QString DiskMediaExtractor::getDiskThumbCachePath(const QString& filePath) {
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString bucket = hashStr.left(2);
    QString cacheDir = QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/" + bucket;
    return cacheDir + "/" + hashStr + ".png";
}
=======
QString DiskMediaExtractor::getDiskThumbCachePath(const QString& filePath) {
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QString hashStr = QString::fromUtf8(QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex());
    QString cacheDir = QDir::temp().filePath("QuarkMeta_Thumbnails");
    return QDir(cacheDir).filePath(QString("%1_230.png").arg(hashStr.left(32)));
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Build Command:
```bash
cmake -B build -S .
cmake --build build --config Release
```

### Verification Methods:
1. Run the application and open any directory containing images.
2. Open Windows File Explorer and navigate to `%LOCALAPPDATA%\Temp\QuarkMeta_Thumbnails\`.
3. Verify that generated thumbnails are stored as `0cb2def4c46632bb52d4bbdeb84244e0_230.png`.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Centralized inside `DiskMediaExtractor::getDiskThumbCachePath`. All thumbnail operations continue to route through this SSOT method.

---

## 6. Header API Signature Verification
- `DiskMediaExtractor::getDiskThumbCachePath(const QString& filePath)`: Exact signature in `src/util/DiskMediaExtractor.h`.
