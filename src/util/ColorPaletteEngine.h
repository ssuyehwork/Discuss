#pragma once

#include <QColor>
#include <QImage>
#include <QString>
#include <QVector>
#include <QPair>

namespace QuarkMeta {

/**
 * @brief 底层图像多媒体色彩与调色板计算引擎 (纯计算与图像处理，0 UI 依赖)
 */
class ColorPaletteEngine {
public:
    // 1. 图像与多媒体格式权威判定
    static bool isGraphicsFile(const QString& ext);
    static bool isStandardImage(const QString& ext);
    static bool isVectorGraphics(const QString& ext);
    static bool isRawPhoto(const QString& ext);

    // 2. 物理文件与内存 QImage 色彩提取
    static QColor extractDominantColor(const QString& filePath);
    static QVector<QPair<QColor, float>> extractPalette(const QString& filePath, int maxColors = 5);
    
    static QColor extractDominantColorFromImage(const QImage& preScaledImage);
    static QVector<QPair<QColor, float>> extractPaletteFromImage(const QImage& preScaledImage, int maxColors = 5);

    // 3. 颜色量化与标准名称/Hex 映射
    static QColor quantizeToStandardColor(const QColor& color);
    static QColor getExtensionColor(const QString& ext);
    static QColor parseColorName(const QString& colorName);
    static QString normalizeColorHex(const QString& colorStr);

    // 4. 国际标准色差算法 (CIEDE2000)
    static double calculateDeltaE(const QColor& c1, const QColor& c2);

private:
    struct LabColor {
        double L = 0.0;
        double a = 0.0;
        double b = 0.0;
    };

    static LabColor rgbToLab(const QColor& rgb);
    static double degToRad(double deg);
    static double radToDeg(double rad);
};

} // namespace QuarkMeta
