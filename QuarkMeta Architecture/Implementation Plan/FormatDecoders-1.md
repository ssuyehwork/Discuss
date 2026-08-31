# QuarkMeta 架构重构方案：FormatDecoders EPS 预览提取顺序调整

## 一、 Overview
调整 `FormatDecoders::extractEpsPreview` 函数内部三种 EPS 预览提取方法的尝试顺序：优先调用 Ghostscript 矢量引擎渲染最高画质预览图；当 Ghostscript 不可用或渲染失败时，退回尝试 DOS 二进制头 TIFF 内嵌预览；最后兜底使用 EPSI 灰网预览。函数签名、外部契约及内部逻辑细节一律保持不变。

## 二、 Modified Files List
- `src/ui/FormatDecoders.cpp`

## 三、 Detailed Line-by-Line Changes

### 1. `src/ui/FormatDecoders.cpp`
```git
<<<<<<< SEARCH
QImage FormatDecoders::extractEpsPreview(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QImage();
    }

    QByteArray header = file.read(30);
    if (header.size() < 30) {
        return QImage();
    }

    // 1. 优先尝试 DOS 二进制头 (C5D0D3C6)
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

    // 2. 普通 ASCII EPS (文本格式) 的 %%BeginPreview: 预览块解析
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
            if (parts.size() >=Part 3) {
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

    // Ghostscript 终极矢量引擎
    QImage gsImg = renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token);
    if (!gsImg.isNull()) {
        return gsImg;
    }

    return QImage();
}
=======
QImage FormatDecoders::extractEpsPreview(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    // 1. 优先尝试 Ghostscript 矢量渲染（画质最好，代价是更慢、依赖外部程序是否安装）
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

    // 3. 最后兜底：%%BeginPreview / %%EndPreview 内嵌 EPSI 灰网预览（画质最差，但覆盖率最高，总能读到点东西）
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
                width = parts[Part 1].toInt();
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

## 四、 Build & Verification Steps
1. 确认 `src/ui/FormatDecoders.cpp` 修改无误。
2. 验证 `extractEpsPreview` 优先尝试 Ghostscript 渲染。
