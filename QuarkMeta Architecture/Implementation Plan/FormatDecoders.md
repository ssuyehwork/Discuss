# Implementation Plan - FormatDecoders.md

## 1. Overview（概述与解决的问题）

### 1.1 核心问题定位
此前系统对于 EPS 图像提取仅有一套逻辑：
- 采用“版本 30 策略（内嵌优先）”虽然速度极快，但在双击 / 空格唤出 `QuickLookWindow` 全屏预览时，低清内嵌图严重模糊发虚；
- 采用“版本 31 策略（GS 矢量优先）”虽然清晰度极高，但导致日常网格/列表滚动时每个 EPS 都调用外部进程排队，严重拖慢文件夹打开与浏览速度；
- 同时，两种不同画质的图混在一个调用链路与缓存空间中，存在相互覆盖污染的严重隐患。

### 1.2 解决方案与架构隔离
1. **策略与分辨率双轨分流**：
   - **缩略图链路（日常滚动）**：执行【版本 30 策略】，优先内存解析 DOS 头 TIFF 内嵌图，仅在无内嵌图时以 **`-r72` DPI** 调用 Ghostscript 兜底；
   - **QuickLook 链路（大图预览）**：执行【版本 31 策略】，优先以 **`-r144` DPI** 高清矢量光栅化渲染，保证全屏与视网膜屏幕下的极致细腻度；
2. **物理存储绝对隔离**：
   - 72 DPI 低清缩略图存放在系统/侧车标准的 `thumbnails/` 缓存目录；
   - 144 DPI 高清预览图存放在独立的 `quicklook_previews/` 缓存目录；
   - 两套缓存路径物理隔离，彻底杜绝模糊图与高清图互相污染覆盖；
3. **接口兼容契约锁**：
   - `renderGhostscriptSafely` 增补 `int dpi = 72` 默认参数，既有调用方签名 100% 保持兼容。

---

## 2. Modified Files List（影响文件清单）
1. `src/ui/FormatDecoders.h`（声明 `extractEpsThumbnail`、`extractEpsQuickLook` 与 `dpi` 扩展参数）
2. `src/ui/FormatDecoders.cpp`（实现 72 DPI 与 144 DPI 双轨策略及 GS 参数动态化）
3. `src/ui/QuickLookWindow.h`（私有增补 `loadOrExtractQuickLookEps` 辅助声明）
4. `src/ui/QuickLookWindow.cpp`（对接 144 DPI 高清提取与独立的 `quicklook_previews` 物理缓存目录）

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### File: `src/ui/FormatDecoders.h`

```
<<<<<<< SEARCH
    // EPS 预览图与 Ghostscript 矢量渲染
    static QImage extractEpsPreview(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);

    // External Process: Ghostscript 降采样渲染 (customTimeoutMs > 0 时使用自定义长效超时)
    static QImage renderGhostscriptSafely(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);
=======
    // 通用兼容接口（默认路由至缩略图策略）
    static QImage extractEpsPreview(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);

    // 策略 1：日常缩略图（版本 30 策略：内嵌优先，-r72 GS 兜底，极限速度）
    static QImage extractEpsThumbnail(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);

    // 策略 2：QuickLook 快速大图（版本 31 策略：-r144 GS 矢量优先，内嵌降级兜底，极致画质）
    static QImage extractEpsQuickLook(const QString& filePath, int targetSize = 2048, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);

    // External Process: Ghostscript 降采样渲染 (customTimeoutMs > 0 时使用自定义长效超时，默认 72 DPI)
    static QImage renderGhostscriptSafely(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr, int dpi = 72);
>>>>>>> REPLACE
```

---

### File: `src/ui/FormatDecoders.cpp`

```
<<<<<<< SEARCH
QImage FormatDecoders::extractEpsPreview(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    // 1. 优先尝试 Ghostscript 矢量渲染（画质最好）
    QImage gsImg = renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token);
    if (!gsImg.isNull()) {
        return gsImg;
    }

    // 2. Ghostscript 不可用/渲染失败时，退回内嵌预览：先试 DOS 二进制头 (C5D0D3C6) 里嵌的 TIFF 预览
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QImage();
    }

    QByteArray header = file.read(30);
    if (header.size() < 30) {
        return QImage();
    }

    if (quint8(header[0]) == 0xC5 && quint8(header[1]) == 0xD0 &&
        quint8(header[2]) == 0xD3 && quint8(header[3]) == 0xC6) {

        quint32 tiffOffset = (quint8(header[20])) | (quint8(header[21]) << 8) |
                             (quint8(header[22]) << 16) | (quint8(header[23]) << 24);
        quint32 tiffLength = (quint8(header[24])) | (quint8(header[25]) << 8) |
                             (quint8(header[26]) << 16) | (quint8(header[27]) << 24);
        if (tiffOffset > 0 && tiffLength > 0) {
            file.seek(tiffOffset);
            QByteArray tiffData = file.read(tiffLength);
            QImage img = decodeTiffMemorySafely(tiffData);
            if (!img.isNull()) {
                return img;
            }
        }
    }

    // 3. 最后兜底：%%BeginPreview / %%EndPreview 内嵌 EPSI 灰网预览
    file.seek(0);
    QTextStream in(&file);
    bool inPreview = false;
    QString hexData;
    int width = 0, height = 0;

    QRegularExpression rxSpaces("\\s+");

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("%%BeginPreview:")) {
            QStringList parts = line.split(rxSpaces, Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                width = parts[1].toInt();
                height = parts[2].toInt();
                inPreview = true;
            }
            continue;
        }
        if (line.startsWith("%%EndPreview")) {
            break;
        }
        if (inPreview) {
            if (line.startsWith("%")) {
                hexData.append(line.mid(1).trimmed());
            }
        }
    }

    if (!hexData.isEmpty() && width > 0 && height > 0) {
        QByteArray binaryData = QByteArray::fromHex(hexData.toLatin1());
        QImage img;
        if (img.loadFromData(binaryData)) {
            return img;
        }
    }

    return QImage();
}
=======
QImage FormatDecoders::extractEpsPreview(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    // 兼容层：默认走缩略图极速通道
    return extractEpsThumbnail(filePath, targetSize, customTimeoutMs, token);
}

QImage FormatDecoders::extractEpsThumbnail(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    // =========================================================================
    // 策略 1（版本 30 策略）：【内嵌图优先，GS 兜底】（追求极限速度，-r72 分辨率）
    // =========================================================================
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QImage();
    }

    QByteArray header = file.read(30);
    if (header.size() < 30) {
        return QImage();
    }

    // 第一优先级：读取 DOS 二进制头 (0xC5D0D3C6) 内嵌 TIFF 预览（纯内存解码，极速毫秒出图）
    if (quint8(header[0]) == 0xC5 && quint8(header[1]) == 0xD0 &&
        quint8(header[2]) == 0xD3 && quint8(header[3]) == 0xC6) {

        quint32 tiffOffset = (quint8(header[20])) | (quint8(header[21]) << 8) |
                             (quint8(header[22]) << 16) | (quint8(header[23]) << 24);
        quint32 tiffLength = (quint8(header[24])) | (quint8(header[25]) << 8) |
                             (quint8(header[26]) << 16) | (quint8(header[27]) << 24);
        if (tiffOffset > 0 && tiffLength > 0) {
            file.seek(tiffOffset);
            QByteArray tiffData = file.read(tiffLength);
            QImage img = decodeTiffMemorySafely(tiffData);
            if (!img.isNull()) {
                return img;
            }
        }
    }

    // 第二优先级：读取 ASCII 文本格式的 %%BeginPreview 预览块
    file.seek(0);
    QTextStream in(&file);
    bool inPreview = false;
    QString hexData;
    int width = 0, height = 0;
    QRegularExpression rxSpaces("\\s+");

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("%%BeginPreview:")) {
            QStringList parts = line.split(rxSpaces, Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                width = parts[1].toInt();
                height = parts[2].toInt();
                inPreview = true;
            }
            continue;
        }
        if (line.startsWith("%%EndPreview")) {
            break;
        }
        if (inPreview) {
            if (line.startsWith("%")) {
                hexData.append(line.mid(1).trimmed());
            }
        }
    }

    if (!hexData.isEmpty() && width > 0 && height > 0) {
        QByteArray binaryData = QByteArray::fromHex(hexData.toLatin1());
        QImage img;
        if (img.loadFromData(binaryData)) {
            return img;
        }
    }

    // 第三优先级（最后兜底）：Ghostscript 外部引擎，严格采用 72 DPI
    return renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token, 72);
}

QImage FormatDecoders::extractEpsQuickLook(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    // =========================================================================
    // 策略 2（版本 31 策略）：【GS 矢量优先，内嵌降级】（追求极致画质，-r144 分辨率）
    // =========================================================================
    // 第一优先级：Ghostscript 矢量高清光栅化，严格采用 144 DPI
    QImage gsImg = renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token, 144);
    if (!gsImg.isNull()) {
        return gsImg;
    }

    // 第二优先级（降级）：若 GS 缺失或失败，退回尝试 DOS 二进制头内嵌 TIFF
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QImage();
    }

    QByteArray header = file.read(30);
    if (header.size() < 30) {
        return QImage();
    }

    if (quint8(header[0]) == 0xC5 && quint8(header[1]) == 0xD0 &&
        quint8(header[2]) == 0xD3 && quint8(header[3]) == 0xC6) {

        quint32 tiffOffset = (quint8(header[20])) | (quint8(header[21]) << 8) |
                             (quint8(header[22]) << 16) | (quint8(header[23]) << 24);
        quint32 tiffLength = (quint8(header[24])) | (quint8(header[25]) << 8) |
                             (quint8(header[26]) << 16) | (quint8(header[27]) << 24);
        if (tiffOffset > 0 && tiffLength > 0) {
            file.seek(tiffOffset);
            QByteArray tiffData = file.read(tiffLength);
            QImage img = decodeTiffMemorySafely(tiffData);
            if (!img.isNull()) {
                return img;
            }
        }
    }

    // 第三优先级（最后兜底）：尝试 %%BeginPreview 文本预览
    file.seek(0);
    QTextStream in(&file);
    bool inPreview = false;
    QString hexData;
    int width = 0, height = 0;
    QRegularExpression rxSpaces("\\s+");

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("%%BeginPreview:")) {
            QStringList parts = line.split(rxSpaces, Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                width = parts[1].toInt();
                height = parts[2].toInt();
                inPreview = true;
            }
            continue;
        }
        if (line.startsWith("%%EndPreview")) {
            break;
        }
        if (inPreview) {
            if (line.startsWith("%")) {
                hexData.append(line.mid(1).trimmed());
            }
        }
    }

    if (!hexData.isEmpty() && width > 0 && height > 0) {
        QByteArray binaryData = QByteArray::fromHex(hexData.toLatin1());
        QImage img;
        if (img.loadFromData(binaryData)) {
            return img;
        }
    }

    return QImage();
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QImage FormatDecoders::renderGhostscriptSafely(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
=======
QImage FormatDecoders::renderGhostscriptSafely(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token, int dpi) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QStringList args;
    args << "-dNOPAUSE"
         << "-dBATCH"
         << "-dSAFER"
         << "-sDEVICE=pngalpha"
         << QString("-r%1").arg(72)
         << "-dFirstPage=1"
         << "-dLastPage=1"
         << QString("-sOutputFile=%1").arg(tempPng)
         << QDir::toNativeSeparators(filePath);
=======
    QStringList args;
    args << "-dNOPAUSE"
         << "-dBATCH"
         << "-dSAFER"
         << "-sDEVICE=pngalpha"
         << QString("-r%1").arg(dpi)
         << "-dFirstPage=1"
         << "-dLastPage=1"
         << QString("-sOutputFile=%1").arg(tempPng)
         << QDir::toNativeSeparators(filePath);
>>>>>>> REPLACE
```

---

### File: `src/ui/QuickLookWindow.h`

```
<<<<<<< SEARCH
    QString detectEncoding(const QByteArray& data);
    bool isBinary(const QByteArray& data);
=======
    QString detectEncoding(const QByteArray& data);
    bool isBinary(const QByteArray& data);
    static QImage loadOrExtractQuickLookEps(const QString& filePath, int targetSize);
>>>>>>> REPLACE
```

---

### File: `src/ui/QuickLookWindow.cpp`

```
<<<<<<< SEARCH
#include "controllers/ContextMenuFactory.h"
#include "dialogs/TextExtensionDialog.h"
=======
#include "controllers/ContextMenuFactory.h"
#include "dialogs/TextExtensionDialog.h"
#include "FormatDecoders.h"
#include <QCryptographicHash>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        } else if (ext == "ai" || ext == "eps" || ext == "psd" || ext == "psb") {
            img = DiskMediaExtractor::getDiskThumbnail(path, 2048);
        } else if (QT_NATIVE_FORMATS.contains(ext)) {
=======
        } else if (ext == "eps") {
            // 物理隔离与 144 DPI 高清矢量：专走 QuickLook 独立管道与独立缓存
            img = loadOrExtractQuickLookEps(path, 2048);
        } else if (ext == "ai" || ext == "psd" || ext == "psb") {
            img = DiskMediaExtractor::getDiskThumbnail(path, 2048);
        } else if (QT_NATIVE_FORMATS.contains(ext)) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool QuickLookWindow::isBinary(const QByteArray& fileData) {
=======
QImage QuickLookWindow::loadOrExtractQuickLookEps(const QString& filePath, int targetSize) {
    // 🚀【物理目录隔离铁律】：QuickLook 144 DPI 大图绝对不与缩略图共用文件夹
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/quicklook_previews";
    QDir().mkpath(cacheDir);

    QFileInfo srcInfo(filePath);
    QString hashKey = QString::fromLatin1(QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex());
    QString cachePath = QString("%1/%2.png").arg(cacheDir, hashKey);
    QFileInfo cacheInfo(cachePath);

    // 1. 若独立缓存命中且未过期，直接毫秒载入 144 DPI 大图
    if (cacheInfo.exists() && cacheInfo.size() > 0 && cacheInfo.lastModified() >= srcInfo.lastModified()) {
        QImage cachedImg(cachePath);
        if (!cachedImg.isNull()) {
            return cachedImg;
        }
    }

    // 2. 独立缓存未命中，调用【版本 31 策略】：-r144 GS 矢量优先提取
    QImage highQImg = FormatDecoders::extractEpsQuickLook(filePath, targetSize);
    if (!highQImg.isNull()) {
        // 3. 安全异步落地到独立的 quicklook_previews 物理目录
        highQImg.save(cachePath, "PNG");
        return highQImg;
    }

    return QImage();
}

bool QuickLookWindow::isBinary(const QByteArray& fileData) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译与验证方法）

### 4.1 构建命令
在已配置好的 MSVC 2022 x64 环境下执行：
```powershell
cmake --build build --config Release --target QuarkMeta
```

### 4.2 验证步骤
1. **缩略图极限速度验证（版本 30，72 DPI）**：
   - 打开包含大量 EPS 文件的文件夹；
   - 观察网格/列表中的缩略图瞬间加载，DOS 头内嵌图毫秒级浮现，CPU 占用极低，无任何排队卡死现象；
   - 检查缩略图缓存，确认生成的是 72 DPI 快速缩略图，且仅存放在标准缩略图缓存区。
2. **QuickLook 极致高清验证（版本 31，144 DPI）**：
   - 选中任意一张 EPS，按下空格键或双击唤起 `QuickLookWindow`；
   - 观察画面通过 Ghostscript 矢量引擎渲染出极度锐利的高清大图，字体无毛刺，线条无马赛克；
   - 检查缓存物理路径，确认生成的高清图片存放在独立的 `Cache/quicklook_previews/` 目录下；
3. **物理隔离互不干扰验证**：
   - 确认缩略图目录中没有被写入 144 DPI 大图；
   - 确认 QuickLook 目录中没有被写入 72 DPI 模糊小图；
   - 再次双击关闭 QuickLook，主面板滚动帧率保持丝滑。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（真理源复用自查）
- **真理源分流**：原图为唯一源头，72 DPI 缩略图与 144 DPI 预览图均为只读派生缓存；
- **防另起炉灶**：Ghostscript 进程调用收敛于 `FormatDecoders::renderGhostscriptSafely`，通过参数化 `dpi` 复用全部进程监控、超时熔断与安全降采样逻辑；
- **防覆盖污染**：两套缓存使用独立的子路径（`thumbnails` vs `quicklook_previews`），杜绝了同名覆盖或跨模块脏读。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的物理方法 / 成员签名 | 来源头文件 |
| :--- | :--- |
| `FormatDecoders::extractEpsThumbnail(const QString&, int, int, std::shared_ptr<CancellationToken>)` | `src/ui/FormatDecoders.h` |
| `FormatDecoders::extractEpsQuickLook(const QString&, int, int, std::shared_ptr<CancellationToken>)` | `src/ui/FormatDecoders.h` |
| `FormatDecoders::renderGhostscriptSafely(const QString&, int, int, std::shared_ptr<CancellationToken>, int)` | `src/ui/FormatDecoders.h` |
| `QStandardPaths::writableLocation(QStandardPaths::StandardLocation)` | `<QStandardPaths>` |
| `QCryptographicHash::hash(const QByteArray&, QCryptographicHash::Algorithm)` | `<QCryptographicHash>` |
| `QFileInfo::lastModified() const` / `QFileInfo::exists() const` | `<QFileInfo>` |
