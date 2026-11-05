#include "ColorPaletteEngine.h"
#include <QImageReader>
#include <QFileInfo>
#include <QSet>
#include <QMap>
#include <cmath>
#include <algorithm>

namespace RapidNotes {

bool ColorPaletteEngine::isGraphicsFile(const QString& ext) {
    return isStandardImage(ext) || isVectorGraphics(ext) || isRawPhoto(ext);
}

bool ColorPaletteEngine::isStandardImage(const QString& ext) {
    static const QSet<QString> standardExts = {
        "jpg", "jpeg", "png", "bmp", "gif", "webp", "ico", "cur",
        "tif", "tiff", "tga", "dds", "wbmp", "pbm", "pgm", "ppm", "xbm", "xpm"
    };
    return standardExts.contains(ext.toLower());
}

bool ColorPaletteEngine::isVectorGraphics(const QString& ext) {
    static const QSet<QString> vectorExts = { "svg", "svgz", "ai", "eps" };
    return vectorExts.contains(ext.toLower());
}

bool ColorPaletteEngine::isRawPhoto(const QString& ext) {
    static const QSet<QString> rawExts = {
        "cr2", "cr3", "nef", "arw", "dng", "orf", "rw2", "pef", "raf"
    };
    return rawExts.contains(ext.toLower());
}

QColor ColorPaletteEngine::getExtensionColor(const QString& ext) {
    return getExtensionBadgeColors(ext).first;
}

QPair<QColor, QColor> ColorPaletteEngine::getExtensionBadgeColors(const QString& ext) {
    QString e = ext.toLower().trimmed();

    static QMap<QString, QPair<QColor, QColor>> s_colorCache;

    auto resolveReserved = [&](const QPair<QColor, QColor>& reservedColors) -> QPair<QColor, QColor> {
        s_colorCache.insert(e, reservedColors);
        return reservedColors;
    };

    if (e == "dir" || e == "folder") return resolveReserved({ QColor("#1C2D37"), QColor("#38BDF8") });
    if (e == "psd" || e == "psb")    return resolveReserved({ QColor("#001D26"), QColor("#02B1DD") });
    if (e == "eps")                  return resolveReserved({ QColor("#35483D"), QColor("#F88025") });
    if (e == "ai")                   return resolveReserved({ QColor("#F88025"), QColor("#35483D") });

    if (s_colorCache.contains(e)) {
        return s_colorCache.value(e);
    }

    uint hashVal = static_cast<uint>(qHash(e));
    int hue = static_cast<int>(hashVal % 360);
    int saturation = 130 + static_cast<int>((hashVal >> 8) % 80);
    int lightness = 80 + static_cast<int>((hashVal >> 16) % 60);

    QColor bgColor = QColor::fromHsl(hue, saturation, lightness);
    double luminance = (0.299 * bgColor.red() + 0.587 * bgColor.green() + 0.114 * bgColor.blue()) / 255.0;
    QColor textColor = (luminance < 0.55) ? QColor("#FFFFFF") : QColor("#1A1A1A");

    QPair<QColor, QColor> colorPair = { bgColor, textColor };
    s_colorCache.insert(e, colorPair);

    return colorPair;
}

QColor ColorPaletteEngine::extractDominantColor(const QString& filePath) {
    auto palette = extractPalette(filePath, 1);
    if (!palette.isEmpty()) {
        return palette.first().first;
    }
    return QColor();
}

QVector<QPair<QColor, float>> ColorPaletteEngine::extractPalette(const QString& filePath, int maxColors) {
    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    if (!reader.canRead()) return {};

    QSize origSize = reader.size();
    if (!origSize.isValid() || origSize.isEmpty()) {
        QImage fullImg = reader.read();
        if (fullImg.isNull()) return {};
        return extractPaletteFromImage(fullImg.scaled(64, 64, Qt::KeepAspectRatio, Qt::FastTransformation), maxColors);
    }

    reader.setScaledSize(origSize.scaled(64, 64, Qt::KeepAspectRatio));
    QImage thumb = reader.read();
    if (thumb.isNull()) return {};

    return extractPaletteFromImage(thumb, maxColors);
}

QColor ColorPaletteEngine::extractDominantColorFromImage(const QImage& preScaledImage) {
    auto palette = extractPaletteFromImage(preScaledImage, 1);
    return palette.isEmpty() ? QColor() : palette.first().first;
}

QVector<QPair<QColor, float>> ColorPaletteEngine::extractPaletteFromImage(const QImage& preScaledImage, int maxColors) {
    if (preScaledImage.isNull()) return {};

    QImage img = preScaledImage.convertToFormat(QImage::Format_ARGB32);
    int totalPixels = 0;
    QMap<int, int> bucketCounts;

    for (int y = 0; y < img.height(); ++y) {
        const QRgb* scanLine = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            QRgb pixel = scanLine[x];
            int a = qAlpha(pixel);
            if (a < 128) continue;

            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);

            int qr = (r >> 5) << 5;
            int qg = (g >> 5) << 5;
            int qb = (b >> 5) << 5;

            int key = (qr << 16) | (qg << 8) | qb;
            bucketCounts[key]++;
            totalPixels++;
        }
    }

    if (totalPixels == 0) return {};

    QVector<QPair<int, int>> sortedBuckets;
    for (auto it = bucketCounts.begin(); it != bucketCounts.end(); ++it) {
        sortedBuckets.append({ it.value(), it.key() });
    }
    std::sort(sortedBuckets.begin(), sortedBuckets.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    QVector<QPair<QColor, float>> result;
    int count = std::min(maxColors, static_cast<int>(sortedBuckets.size()));
    for (int i = 0; i < count; ++i) {
        int key = sortedBuckets[i].second;
        int r = (key >> 16) & 0xFF;
        int g = (key >> 8) & 0xFF;
        int b = key & 0xFF;
        float ratio = static_cast<float>(sortedBuckets[i].first) / totalPixels;
        result.append({ QColor(r + 16, g + 16, b + 16), ratio });
    }

    return result;
}

QVector<QPair<QColor, float>> ColorPaletteEngine::extractWeightedPalette(const QImage& preScaledImage) {
    return extractPaletteFromImage(preScaledImage, 5);
}

QColor ColorPaletteEngine::quantizeToStandardColor(const QColor& color) {
    static const QVector<QColor> standardPalette = {
        QColor("#e74c3c"), QColor("#e67e22"), QColor("#f1c40f"), QColor("#2ecc71"),
        QColor("#1abc9c"), QColor("#3498db"), QColor("#9b59b6"), QColor("#e84393"),
        QColor("#ffffff"), QColor("#95a5a6"), QColor("#2d3436")
    };

    QColor bestMatch = standardPalette.first();
    double minDelta = calculateDeltaE(color, bestMatch);

    for (int i = 1; i < standardPalette.size(); ++i) {
        double d = calculateDeltaE(color, standardPalette[i]);
        if (d < minDelta) {
            minDelta = d;
            bestMatch = standardPalette[i];
        }
    }
    return bestMatch;
}

QColor ColorPaletteEngine::parseColorName(const QString& colorName) {
    static const QMap<QString, QString> colorMap = {
        { "red", "#E05656" },
        { "orange", "#F2994A" },
        { "yellow", "#F2C94C" },
        { "green", "#27AE60" },
        { "blue", "#2F80ED" },
        { "purple", "#9B51E0" },
        { "grey", "#828282" },
        { "gray", "#828282" }
    };
    QString lower = colorName.toLower().trimmed();
    if (colorMap.contains(lower)) {
        return QColor(colorMap.value(lower));
    }
    return QColor(colorName);
}

QString ColorPaletteEngine::normalizeColorHex(const QString& colorStr) {
    QColor c = parseColorName(colorStr);
    return c.isValid() ? c.name(QColor::HexRgb).toUpper() : QString();
}

bool ColorPaletteEngine::matchesAutoColor(const QColor& target, const QColor& candidate, int precision) {
    double d = calculateDeltaE(target, candidate);
    return d <= static_cast<double>(precision);
}

float ColorPaletteEngine::matchedAreaRatio(const QColor& target, const std::vector<std::pair<QColor, float>>& palettes, const QString& autoColorHex, int precision) {
    float totalMatched = 0.0f;
    for (const auto& p : palettes) {
        if (matchesAutoColor(target, p.first, precision)) {
            totalMatched += p.second;
        }
    }
    return totalMatched;
}

float ColorPaletteEngine::matchedAreaRatio(const QColor& target, const QVector<QPair<QColor, float>>& palettes, const QString& autoColorHex, int precision) {
    float totalMatched = 0.0f;
    for (const auto& p : palettes) {
        if (matchesAutoColor(target, p.first, precision)) {
            totalMatched += p.second;
        }
    }
    return totalMatched;
}

ColorPaletteEngine::LabColor ColorPaletteEngine::rgbToLab(const QColor& rgb) {
    double r = rgb.redF();
    double g = rgb.greenF();
    double b = rgb.blueF();

    r = (r > 0.04045) ? std::pow((r + 0.055) / 1.055, 2.4) : (r / 12.92);
    g = (g > 0.04045) ? std::pow((g + 0.055) / 1.055, 2.4) : (g / 12.92);
    b = (b > 0.04045) ? std::pow((b + 0.055) / 1.055, 2.4) : (b / 12.92);

    r *= 100.0;
    g *= 100.0;
    b *= 100.0;

    double x = r * 0.4124 + g * 0.3576 + b * 0.1805;
    double y = r * 0.2126 + g * 0.7152 + b * 0.0722;
    double z = r * 0.0193 + g * 0.1192 + b * 0.9505;

    x /= 95.047;
    y /= 100.000;
    z /= 108.883;

    auto f = [](double t) {
        return (t > 0.008856) ? std::cbrt(t) : (7.787 * t + 16.0 / 116.0);
    };

    LabColor lab;
    lab.L = (116.0 * f(y)) - 16.0;
    lab.a = 500.0 * (f(x) - f(y));
    lab.b = 200.0 * (f(y) - f(z));
    return lab;
}

double ColorPaletteEngine::deltaE76(const QColor& c1, const QColor& c2) {
    LabColor l1 = rgbToLab(c1);
    LabColor l2 = rgbToLab(c2);
    return std::sqrt(std::pow(l1.L - l2.L, 2) + std::pow(l1.a - l2.a, 2) + std::pow(l1.b - l2.b, 2));
}

double ColorPaletteEngine::degToRad(double deg) {
    return deg * 3.14159265358979323846 / 180.0;
}

double ColorPaletteEngine::radToDeg(double rad) {
    return rad * 180.0 / 3.14159265358979323846;
}

double ColorPaletteEngine::calculateDeltaE(const QColor& c1, const QColor& c2) {
    return deltaE76(c1, c2);
}

} // namespace RapidNotes
