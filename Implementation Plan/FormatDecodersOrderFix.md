# Implementation Plan - AI / PDF / EPS 缩略图提取顺序调整方案 (FormatDecodersOrderFix.md)

- **Base Commit Hash**: 064a8b932918e95757175b4cdbee5915839e14fa
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## Chapter 1: Overview

### 需求目标
调整 QuarkMeta 中 AI / PDF / EPS 格式缩略图与 QuickLook 预览图的提取策略顺序，消除重复的 Ghostscript 调用与低 DPI 渲染退化，提升缩略图生成质量与响应性能：
1. **`extractAiPreview`（AI/PDF 缩略图）**：将 Ghostscript 调整至第 1 位（100 DPI），移除原第 3 位的 72 DPI GS 调用。将后备提取链抽出为私有成员函数 `extractAiEmbeddedFallback`。
2. **`extractAiQuickLook`（AI/PDF 空格键预览）**：保持 144 DPI GS 优先，失败后直接调用 `extractAiEmbeddedFallback` 后备链，避免二次调用 GS。
3. **`extractEpsThumbnail`（EPS 缩略图）**：调整为 Ghostscript 100 DPI 优先，失败后再退回 `extractEpsEmbeddedPreview` 内嵌预览，移除原 72 DPI GS 兜底。
4. **`FormatDecoders.h`**：更新 `extractEpsThumbnail` 上方的策略注释，并在 private 区声明 `extractAiEmbeddedFallback`。

---

## Chapter 2: Modified Files List

- `src/ui/FormatDecoders.h` (修改：更新注释并在 private 区增加 `extractAiEmbeddedFallback` 私有声明)
- `src/ui/FormatDecoders.cpp` (修改：重构 `extractAiPreview`、`extractAiQuickLook` 和 `extractEpsThumbnail` 并实现 `extractAiEmbeddedFallback`)

---

## Chapter 3: Detailed Line-by-Line Changes

### 1. `src/ui/FormatDecoders.h`

```
<<<<<<< SEARCH
    // 策略 1：自动提取缩略图（内嵌 TIFF → %%BeginPreview → GS 72 DPI 兜底）
    static QImage extractEpsThumbnail(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);
=======
    // 策略 1：自动提取缩略图（GS 100 DPI 优先 → 内嵌 TIFF → %%BeginPreview）
    static QImage extractEpsThumbnail(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
private:
    static QString findGhostscriptExecutable();
    static QImage renderPdfAiFirstPage(const QString& filePath, int targetSize = 512);

    // EPS 内嵌预览提取：DOS 二进制头 TIFF → %%BeginPreview 文本预览（不含 Ghostscript）
    static QImage extractEpsEmbeddedPreview(const QString& filePath);
=======
private:
    static QString findGhostscriptExecutable();
    static QImage renderPdfAiFirstPage(const QString& filePath, int targetSize = 512);

    // AI/PDF 内嵌后备提取链（%AI7-10 缩略图 → XMP 图 → Windows 系统缩略图 → 内嵌 PNG → 内嵌 JPG → Shell 兜底）
    static QImage extractAiEmbeddedFallback(const QString& filePath, int targetSize = 512);

    // EPS 内嵌预览提取：DOS 二进制头 TIFF → %%BeginPreview 文本预览（不含 Ghostscript）
    static QImage extractEpsEmbeddedPreview(const QString& filePath);
>>>>>>> REPLACE
```

---

### 2. `src/ui/FormatDecoders.cpp`

```
<<<<<<< SEARCH
QImage FormatDecoders::extractAiPreview(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QImage();

    QByteArray rawData = file.read(2 * 1024 * 1024);
    file.close();

    if (rawData.isEmpty() || CoreController::isShuttingDown()) return QImage();

    int thumbHeaderIdx = rawData.indexOf("%AI7_Thumbnail:");
    if (thumbHeaderIdx == -1) thumbHeaderIdx = rawData.indexOf("%AI8_Thumbnail:");
    if (thumbHeaderIdx == -1) thumbHeaderIdx = rawData.indexOf("%AI9_Thumbnail:");
    if (thumbHeaderIdx == -1) thumbHeaderIdx = rawData.indexOf("%AI10_Thumbnail:");

    if (thumbHeaderIdx != -1) {
        int lineEnd = rawData.indexOf('\n', thumbHeaderIdx);
        if (lineEnd != -1) {
            QByteArray headerLine = rawData.mid(thumbHeaderIdx, lineEnd - thumbHeaderIdx);
            QString headerStr = QString::fromLatin1(headerLine);
            QStringList parts = headerStr.section(':', 1).trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                int width = parts[0].toInt();
                int height = parts[1].toInt();

                int blockEnd = rawData.indexOf("%AI7_ThumbnailEnd", lineEnd);
                if (blockEnd == -1) blockEnd = rawData.indexOf("%AI9_ThumbnailEnd", lineEnd);
                if (blockEnd == -1) blockEnd = rawData.indexOf("%%EndData", lineEnd);

                if (blockEnd != -1 && width > 0 && height > 0) {
                    QByteArray hexData;
                    hexData.reserve(blockEnd - lineEnd);

                    int cur = lineEnd;
                    while (cur < blockEnd) {
                        int nextLine = rawData.indexOf('\n', cur);
                        if (nextLine == -1) break;
                        QByteArray line = rawData.mid(cur, nextLine - cur).trimmed();
                        if (line.startsWith("%")) {
                            hexData.append(line.mid(1).trimmed());
                        }
                        cur = nextLine + 1;
                    }

                    QByteArray binaryData = QByteArray::fromHex(hexData);
                    const int paletteSize = 256 * 3;

                    if (binaryData.size() >= paletteSize + width * height) {
                        QList<QRgb> colorTable;
                        colorTable.reserve(256);
                        const uchar* palPtr = reinterpret_cast<const uchar*>(binaryData.constData());

                        for (int i = 0; i < 256; ++i) {
                            colorTable.append(qRgb(palPtr[i * 3 + 2], palPtr[i * 3 + 1], palPtr[i * 3]));
                        }

                        QImage img(width, height, QImage::Format_Indexed8);
                        img.setColorTable(colorTable);
                        const uchar* pixelPtr = palPtr + paletteSize;
                        for (int y = 0; y < height; ++y) {
                            memcpy(img.scanLine(y), pixelPtr + y * width, width);
                        }
                        if (!img.isNull()) {
                            return img.convertToFormat(QImage::Format_ARGB32);
                        }
                    }
                }
            }
        }
    }

    int xmpStart = rawData.indexOf("<xmpGImg:image>");
    if (xmpStart != -1) {
        xmpStart += 15;
        int xmpEnd = rawData.indexOf("</xmpGImg:image>", xmpStart);
        if (xmpEnd != -1) {
            QByteArray base64Data = rawData.mid(xmpStart, xmpEnd - xmpStart).trimmed();
            base64Data.replace("\n", "").replace("\r", "").replace(" ", "");
            QByteArray jpgBytes = QByteArray::fromBase64(base64Data);
            QImage img;
            if (img.loadFromData(jpgBytes)) {
                return img;
            }
        }
    }

    QImage gsImg = renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token);
    if (!gsImg.isNull()) {
        return gsImg;
    }

    QImage pdfRenderImg = renderPdfAiFirstPage(filePath, targetSize);
    if (!pdfRenderImg.isNull()) {
        return pdfRenderImg;
    }

    int pngStart = rawData.indexOf("\x89PNG\r\n\x1a\n");
    if (pngStart != -1) {
        int pngEnd = rawData.indexOf("IEND", pngStart);
        if (pngEnd != -1) {
            QByteArray pngData = rawData.mid(pngStart, (pngEnd + 8) - pngStart);
            QImage img;
            if (img.loadFromData(pngData, "PNG") && img.width() >= 32) {
                return img;
            }
        }
    }

    int jpgStart = rawData.indexOf("\xFF\xD8\xFF");
    if (jpgStart != -1) {
        int jpgEnd = rawData.indexOf("\xFF\xD9", jpgStart);
        if (jpgEnd != -1) {
            QByteArray jpgData = rawData.mid(jpgStart, (jpgEnd + 2) - jpgStart);
            QImage img;
            if (img.loadFromData(jpgData, "JPEG") && img.width() >= 32) {
                return img;
            }
        }
    }

    return WindowsShellThumbnailProvider::getShellThumbnail(filePath, targetSize);
}

QImage FormatDecoders::extractAiQuickLook(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    QImage gsImg = renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token, 144);
    if (!gsImg.isNull()) {
        return gsImg;
    }
    return extractAiPreview(filePath, targetSize, customTimeoutMs, token);
}
=======
QImage FormatDecoders::extractAiEmbeddedFallback(const QString& filePath, int targetSize) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QImage();

    QByteArray rawData = file.read(2 * 1024 * 1024);
    file.close();

    if (rawData.isEmpty() || CoreController::isShuttingDown()) return QImage();

    // 2. 内嵌缩略图块（%AI7/8/9/10_Thumbnail）
    int thumbHeaderIdx = rawData.indexOf("%AI7_Thumbnail:");
    if (thumbHeaderIdx == -1) thumbHeaderIdx = rawData.indexOf("%AI8_Thumbnail:");
    if (thumbHeaderIdx == -1) thumbHeaderIdx = rawData.indexOf("%AI9_Thumbnail:");
    if (thumbHeaderIdx == -1) thumbHeaderIdx = rawData.indexOf("%AI10_Thumbnail:");

    if (thumbHeaderIdx != -1) {
        int lineEnd = rawData.indexOf('\n', thumbHeaderIdx);
        if (lineEnd != -1) {
            QByteArray headerLine = rawData.mid(thumbHeaderIdx, lineEnd - thumbHeaderIdx);
            QString headerStr = QString::fromLatin1(headerLine);
            QStringList parts = headerStr.section(':', 1).trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                int width = parts[0].toInt();
                int height = parts[1].toInt();

                int blockEnd = rawData.indexOf("%AI7_ThumbnailEnd", lineEnd);
                if (blockEnd == -1) blockEnd = rawData.indexOf("%AI9_ThumbnailEnd", lineEnd);
                if (blockEnd == -1) blockEnd = rawData.indexOf("%%EndData", lineEnd);

                if (blockEnd != -1 && width > 0 && height > 0) {
                    QByteArray hexData;
                    hexData.reserve(blockEnd - lineEnd);

                    int cur = lineEnd;
                    while (cur < blockEnd) {
                        int nextLine = rawData.indexOf('\n', cur);
                        if (nextLine == -1) break;
                        QByteArray line = rawData.mid(cur, nextLine - cur).trimmed();
                        if (line.startsWith("%")) {
                            hexData.append(line.mid(1).trimmed());
                        }
                        cur = nextLine + 1;
                    }

                    QByteArray binaryData = QByteArray::fromHex(hexData);
                    const int paletteSize = 256 * 3;

                    if (binaryData.size() >= paletteSize + width * height) {
                        QList<QRgb> colorTable;
                        colorTable.reserve(256);
                        const uchar* palPtr = reinterpret_cast<const uchar*>(binaryData.constData());

                        for (int i = 0; i < 256; ++i) {
                            colorTable.append(qRgb(palPtr[i * 3 + 2], palPtr[i * 3 + 1], palPtr[i * 3]));
                        }

                        QImage img(width, height, QImage::Format_Indexed8);
                        img.setColorTable(colorTable);
                        const uchar* pixelPtr = palPtr + paletteSize;
                        for (int y = 0; y < height; ++y) {
                            memcpy(img.scanLine(y), pixelPtr + y * width, width);
                        }
                        if (!img.isNull()) {
                            return img.convertToFormat(QImage::Format_ARGB32);
                        }
                    }
                }
            }
        }
    }

    // 3. 内嵌 XMP 图（xmpGImg:image）
    int xmpStart = rawData.indexOf("<xmpGImg:image>");
    if (xmpStart != -1) {
        xmpStart += 15;
        int xmpEnd = rawData.indexOf("</xmpGImg:image>", xmpStart);
        if (xmpEnd != -1) {
            QByteArray base64Data = rawData.mid(xmpStart, xmpEnd - xmpStart).trimmed();
            base64Data.replace("\n", "").replace("\r", "").replace(" ", "");
            QByteArray jpgBytes = QByteArray::fromBase64(base64Data);
            QImage img;
            if (img.loadFromData(jpgBytes)) {
                return img;
            }
        }
    }

    // 4. Windows 系统缩略图（renderPdfAiFirstPage）
    QImage pdfRenderImg = renderPdfAiFirstPage(filePath, targetSize);
    if (!pdfRenderImg.isNull()) {
        return pdfRenderImg;
    }

    // 5. 内嵌 PNG
    int pngStart = rawData.indexOf("\x89PNG\r\n\x1a\n");
    if (pngStart != -1) {
        int pngEnd = rawData.indexOf("IEND", pngStart);
        if (pngEnd != -1) {
            QByteArray pngData = rawData.mid(pngStart, (pngEnd + 8) - pngStart);
            QImage img;
            if (img.loadFromData(pngData, "PNG") && img.width() >= 32) {
                return img;
            }
        }
    }

    // 6. 内嵌 JPG
    int jpgStart = rawData.indexOf("\xFF\xD8\xFF");
    if (jpgStart != -1) {
        int jpgEnd = rawData.indexOf("\xFF\xD9", jpgStart);
        if (jpgEnd != -1) {
            QByteArray jpgData = rawData.mid(jpgStart, (jpgEnd + 2) - jpgStart);
            QImage img;
            if (img.loadFromData(jpgData, "JPEG") && img.width() >= 32) {
                return img;
            }
        }
    }

    // 7. Windows 系统缩略图（WindowsShellThumbnailProvider，原有兜底）
    return WindowsShellThumbnailProvider::getShellThumbnail(filePath, targetSize);
}

QImage FormatDecoders::extractAiPreview(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    // 1. Ghostscript (100 DPI) 优先
    QImage gsImg = renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token, 100);
    if (!gsImg.isNull()) {
        return gsImg;
    }
    // 2~7. 内嵌后备链
    return extractAiEmbeddedFallback(filePath, targetSize);
}

QImage FormatDecoders::extractAiQuickLook(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    // 1. Ghostscript (144 DPI) 优先
    QImage gsImg = renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token, 144);
    if (!gsImg.isNull()) {
        return gsImg;
    }
    // 2~7. 内嵌后备链（避免重复触发 Ghostscript）
    return extractAiEmbeddedFallback(filePath, targetSize);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QImage FormatDecoders::extractEpsThumbnail(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    QImage img = extractEpsEmbeddedPreview(filePath);
    if (!img.isNull()) {
        return img;
    }
    return renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token, 72);
}
=======
QImage FormatDecoders::extractEpsThumbnail(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    // 1. Ghostscript (100 DPI) 优先
    QImage gsImg = renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token, 100);
    if (!gsImg.isNull()) {
        return gsImg;
    }
    // 2. 内嵌预览（TIFF / %%BeginPreview）
    return extractEpsEmbeddedPreview(filePath);
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps

1. **编译核验**：运行 `cmake --build build` 确保没有语法错误。
2. **逻辑核验**：
   - 确认 `extractAiPreview` 内部仅包含一次 100 DPI 的 GS 调用。
   - 确认 `extractAiQuickLook` 内部在 144 DPI GS 失败后直接调用 `extractAiEmbeddedFallback`，不产生二次 GS 调用。
   - 确认 `extractEpsThumbnail` 优先尝试 100 DPI GS，失败后退回内嵌预览。

---

## Chapter 5: Official API Reuse & Anti-Redundancy

- 提取并共用私有函数 `extractAiEmbeddedFallback`，消除 `extractAiPreview` 与 `extractAiQuickLook` 之间的后备提取逻辑冗余，完全杜绝二次触发 GS。

---

## Chapter 6: Data SSOT Declaration Table

| 数据源 | 提取算法 | 涉及函数 |
| :--- | :--- | :--- |
| **AI/PDF 缩略图** | GS 100 DPI → 后备链 | `extractAiPreview` |
| **AI/PDF 大图预览** | GS 144 DPI → 后备链 | `extractAiQuickLook` |
| **EPS 缩略图** | GS 100 DPI → 内嵌预览 | `extractEpsThumbnail` |

---

## Chapter 7: Interface & Visibility Consistency Check

- **`FormatDecoders::extractAiEmbeddedFallback`**:
  - 声明：`src/ui/FormatDecoders.h` (private)
  - 实现：`src/ui/FormatDecoders.cpp`
  - 签名：`static QImage extractAiEmbeddedFallback(const QString& filePath, int targetSize = 512)`
  - 公有公开 API 签名 100% 保持冻结不动。

---

## Chapter 8: Header Chain & Inclusion Closure Check

- 修改仅触及 `FormatDecoders.h` 与 `FormatDecoders.cpp`，未删除任何依赖的头文件，类型完整性闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 检查项 | 是否满足 | 说明 |
| :--- | :--- | :--- |
| 1. 公开 API 签名未破坏 | 满足 | 仅新增 private 辅助函数，公开签名零变动 |
| 2. 严禁改动无关代码 | 满足 | 未改动 PSD、SVG、ImageDecoderFacade 及并发逻辑 |
| 3. 代码未直接动码 | 满足 | 实施方案规范放置于 `Implementation Plan/` 目录 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table

- 保持 Ghostscript 的 `renderGhostscriptSafely` 默认参数（72 DPI）不变，仅在调用点明确传递 100 或 144 DPI。
