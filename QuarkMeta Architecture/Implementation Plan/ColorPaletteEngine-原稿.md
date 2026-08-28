# QuarkMeta 多媒体色彩提取与调色板引擎归一化实施方案 (ColorPaletteEngine)

## 1. 目标与范围
- 新建 `ColorPaletteEngine`（位于 `src/util/`）：将图像格式权威判别、主导色彩提取、5 色调色板聚合、标准色标量化映射以及 CIEDE2000 国际标准色差算法（$\Delta E$）**100% 归一化下沉至底层工具层**。
- 彻底物理删除分层倒挂且重复的类：`src/ui/MediaColorExtractor.h/cpp` 与 `src/ui/ColorAlgorithmEngine.h/cpp`。
- 净化 `UiHelper.h`：彻底切断对 `src/ui/` 色彩类的依赖，所有色彩与图像算法接口纯净平滑转发至 `ColorPaletteEngine`（保持现有外部调用 100% 向后兼容）。

---

## 2. 核心模块独立实现

### 2.1 `src/util/ColorPaletteEngine.h`
```cpp
#pragma once

#include <QColor>
#include <QString>
#include <QVector>
#include <QPair>
#include <QSet>
#include <QMap>

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

    // 2. 物理文件色彩提取
    static QColor extractDominantColor(const QString& filePath);
    static QVector<QPair<QColor, float>> extractPalette(const QString& filePath, int maxColors = 5);

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
```

### 2.2 `src/util/ColorPaletteEngine.cpp`
```cpp
#include "ColorPaletteEngine.h"
#include <QImage>
#include <QImageReader>
#include <QFileInfo>
#include <cmath>
#include <algorithm>

namespace QuarkMeta {

bool ColorPaletteEngine::isGraphicsFile(const QString& ext) {
    static const QSet<QString> graphicsExts = {
        "jpg", "jpeg", "png", "bmp", "webp", "gif", "ico", "cur", "ani",
        "psd", "psb", "ai", "eps", "svg", "tif", "tiff", "hdr", "tga",
        "dng", "cr2", "cr3", "nef", "arw", "rw2", "orf", "heic", "avif"
    };
    return graphicsExts.contains(ext.toLower().trimmed());
}

bool ColorPaletteEngine::isStandardImage(const QString& ext) {
    static const QSet<QString> stdExts = {
        "jpg", "jpeg", "png", "bmp", "webp", "gif", "ico", "tif", "tiff"
    };
    return stdExts.contains(ext.toLower().trimmed());
}

bool ColorPaletteEngine::isVectorGraphics(const QString& ext) {
    QString e = ext.toLower().trimmed();
    return (e == "svg" || e == "ai" || e == "eps");
}

bool ColorPaletteEngine::isRawPhoto(const QString& ext) {
    static const QSet<QString> rawExts = {
        "dng", "cr2", "cr3", "nef", "arw", "rw2", "orf"
    };
    return rawExts.contains(ext.toLower().trimmed());
}

QColor ColorPaletteEngine::parseColorName(const QString& colorName) {
    if (colorName.isEmpty()) return QColor();
    
    QColor c(colorName);
    if (c.isValid()) return c;

    QString name = colorName.trimmed().toLower();
    if (name == "red"    || name == "红" || name == "红色") return QColor("#E24B4A");
    if (name == "orange" || name == "橙" || name == "橙色") return QColor("#EF9F27");
    if (name == "yellow" || name == "黄" || name == "黄色") return QColor("#FECF0E");
    if (name == "green"  || name == "绿" || name == "绿色") return QColor("#639922");
    if (name == "cyan"   || name == "青" || name == "青色") return QColor("#1D9E75");
    if (name == "blue"   || name == "蓝" || name == "蓝色") return QColor("#378ADD");
    if (name == "purple" || name == "紫" || name == "紫色") return QColor("#7F77DD");
    if (name == "gray"   || name == "灰" || name == "灰色") return QColor("#5F5E5A");
    if (name == "black"  || name == "黑" || name == "黑色") return QColor("#000000");
    if (name == "white"  || name == "白" || name == "白色") return QColor("#FFFFFF");
    
    return QColor();
}

QString ColorPaletteEngine::normalizeColorHex(const QString& colorStr) {
    if (colorStr.trimmed().isEmpty()) return "";
    QColor c = parseColorName(colorStr);
    if (!c.isValid()) return "";
    return c.name().toUpper();
}

QColor ColorPaletteEngine::getExtensionColor(const QString& ext) {
    QString e = ext.toLower().trimmed();
    if (e == "psd" || e == "psb") return QColor("#31A8FF");
    if (e == "ai" || e == "eps")  return QColor("#FF9A00");
    if (e == "svg")               return QColor("#FFB13B");
    if (e == "png")               return QColor("#2ECC71");
    if (e == "jpg" || e == "jpeg") return QColor("#E67E22");
    if (e == "gif")               return QColor("#9B59B6");
    if (e == "webp")              return QColor("#1ABC9C");
    if (e == "pdf")               return QColor("#E74C3C");
    if (e == "txt" || e == "md")  return QColor("#95A5A6");
    return QColor("#7F8C8D");
}

QColor ColorPaletteEngine::extractDominantColor(const QString& filePath) {
    auto palette = extractPalette(filePath, 1);
    if (!palette.isEmpty()) {
        return palette.first().first;
    }
    return QColor();
}

QVector<QPair<QColor, float>> ColorPaletteEngine::extractPalette(const QString& filePath, int maxColors) {
    QVector<QPair<QColor, float>> result;
    if (!isGraphicsFile(QFileInfo(filePath).suffix())) return result;

    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    // 快速下采样至 128x128 执行毫秒级量化
    reader.setScaledSize(QSize(128, 128));
    QImage img = reader.read();
    if (img.isNull()) return result;

    img = img.convertToFormat(QImage::Format_RGB888);
    const int totalPixels = img.width() * img.height();
    if (totalPixels <= 0) return result;

    // 简单高效的八叉树/直方图桶量化 (5-bit 每通道)
    QMap<quint16, int> histogram;
    const uchar* bits = img.constBits();
    const int stride = img.bytesPerLine();

    for (int y = 0; y < img.height(); ++y) {
        const uchar* line = bits + y * stride;
        for (int x = 0; x < img.width(); ++x) {
            int r = line[x * 3 + 0] >> 3;
            int g = line[x * 3 + 1] >> 3;
            int b = line[x * 3 + 2] >> 3;
            quint16 key = (r << 10) | (g << 5) | b;
            histogram[key]++;
        }
    }

    QVector<QPair<int, quint16>> sortedBuckets;
    for (auto it = histogram.begin(); it != histogram.end(); ++it) {
        sortedBuckets.append({it.value(), it.key()});
    }
    std::sort(sortedBuckets.begin(), sortedBuckets.end(), std::greater<QPair<int, quint16>>());

    int count = std::min(maxColors, static_cast<int>(sortedBuckets.size()));
    for (int i = 0; i < count; ++i) {
        quint16 k = sortedBuckets[i].second;
        int r = ((k >> 10) & 0x1F) << 3;
        int g = ((k >> 5) & 0x1F) << 3;
        int b = (k & 0x1F) << 3;
        float ratio = static_cast<float>(sortedBuckets[i].first) / totalPixels;
        result.append({QColor(r, g, b), ratio});
    }

    return result;
}

QColor ColorPaletteEngine::quantizeToStandardColor(const QColor& color) {
    if (!color.isValid()) return QColor();

    static const QVector<QColor> standardPalette = {
        QColor("#E24B4A"), // 红色
        QColor("#EF9F27"), // 橙色
        QColor("#FECF0E"), // 黄色
        QColor("#639922"), // 绿色
        QColor("#1D9E75"), // 青色
        QColor("#378ADD"), // 蓝色
        QColor("#7F77DD"), // 紫色
        QColor("#5F5E5A")  // 灰色
    };

    double minDelta = 1e9;
    QColor closest = standardPalette.first();

    for (const auto& stdCol : standardPalette) {
        double d = calculateDeltaE(color, stdCol);
        if (d < minDelta) {
            minDelta = d;
            closest = stdCol;
        }
    }
    return closest;
}

ColorPaletteEngine::LabColor ColorPaletteEngine::rgbToLab(const QColor& rgb) {
    double r = rgb.redF();
    double g = rgb.greenF();
    double b = rgb.blueF();

    // sRGB 转线性 XYZ
    auto pivot = [](double n) {
        return (n > 0.04045) ? std::pow((n + 0.055) / 1.055, 2.4) : (n / 12.92);
    };
    r = pivot(r) * 100.0;
    g = pivot(g) * 100.0;
    b = pivot(b) * 100.0;

    double x = r * 0.4124 + g * 0.3576 + b * 0.1805;
    double y = r * 0.2126 + g * 0.7152 + b * 0.0722;
    double z = r * 0.0193 + g * 0.1192 + b * 0.9505;

    // D65 标准白点归一
    x /= 95.047;
    y /= 100.000;
    z /= 108.883;

    auto labPivot = [](double n) {
        return (n > 0.008856) ? std::cbrt(n) : (7.787 * n + 16.0 / 116.0);
    };
    double fx = labPivot(x);
    double fy = labPivot(y);
    double fz = labPivot(z);

    LabColor lab;
    lab.L = (116.0 * fy) - 16.0;
    lab.a = 500.0 * (fx - fy);
    lab.b = 200.0 * (fy - fz);
    return lab;
}

double ColorPaletteEngine::degToRad(double deg) { return deg * (M_PI / 180.0); }
double ColorPaletteEngine::radToDeg(double rad) { return rad * (180.0 / M_PI); }

double ColorPaletteEngine::calculateDeltaE(const QColor& c1, const QColor& c2) {
    // 工业级 CIEDE2000 标准色差实现
    LabColor l1 = rgbToLab(c1);
    LabColor l2 = rgbToLab(c2);

    double c_star1 = std::sqrt(l1.a * l1.a + l1.b * l1.b);
    double c_star2 = std::sqrt(l2.a * l2.a + l2.b * l2.b);
    double c_bar = (c_star1 + c_star2) / 2.0;

    double g = 0.5 * (1.0 - std::sqrt(std::pow(c_bar, 7) / (std::pow(c_bar, 7) + std::pow(25.0, 7))));
    double a1_prime = (1.0 + g) * l1.a;
    double a2_prime = (1.0 + g) * l2.a;

    double c1_prime = std::sqrt(a1_prime * a1_prime + l1.b * l1.b);
    double c2_prime = std::sqrt(a2_prime * a2_prime + l2.b * l2.b);

    auto computeHPrime = [](double a_p, double b) {
        if (a_p == 0.0 && b == 0.0) return 0.0;
        double deg = radToDeg(std::atan2(b, a_p));
        return (deg >= 0.0) ? deg : (deg + 360.0);
    };

    double h1_prime = computeHPrime(a1_prime, l1.b);
    double h2_prime = computeHPrime(a2_prime, l2.b);

    double delta_L_prime = l2.L - l1.L;
    double delta_C_prime = c2_prime - c1_prime;

    double delta_h_prime = 0.0;
    if (c1_prime * c2_prime != 0.0) {
        double diff = h2_prime - h1_prime;
        if (std::abs(diff) <= 180.0) delta_h_prime = diff;
        else if (diff > 180.0) delta_h_prime = diff - 360.0;
        else delta_h_prime = diff + 360.0;
    }
    double delta_H_prime = 2.0 * std::sqrt(c1_prime * c2_prime) * std::sin(degToRad(delta_h_prime / 2.0));

    double L_bar_prime = (l1.L + l2.L) / 2.0;
    double C_bar_prime = (c1_prime + c2_prime) / 2.0;

    double H_bar_prime = 0.0;
    if (c1_prime * c2_prime != 0.0) {
        double sum = h1_prime + h2_prime;
        double diff = std::abs(h1_prime - h2_prime);
        if (diff <= 180.0) H_bar_prime = sum / 2.0;
        else if (sum < 360.0) H_bar_prime = (sum + 360.0) / 2.0;
        else H_bar_prime = (sum - 360.0) / 2.0;
    }

    double T = 1.0 - 0.17 * std::cos(degToRad(H_bar_prime - 30.0))
                   + 0.24 * std::cos(degToRad(2.0 * H_bar_prime))
                   + 0.32 * std::cos(degToRad(3.0 * H_bar_prime + 6.0))
                   - 0.20 * std::cos(degToRad(4.0 * H_bar_prime - 63.0));

    double delta_theta = 30.0 * std::exp(-std::pow((H_bar_prime - 275.0) / 25.0, 2));
    double R_C = 2.0 * std::sqrt(std::pow(C_bar_prime, 7) / (std::pow(C_bar_prime, 7) + std::pow(25.0, 7)));
    double S_L = 1.0 + ((0.015 * std::pow(L_bar_prime - 50.0, 2)) / std::sqrt(20.0 + std::pow(L_bar_prime - 50.0, 2)));
    double S_C = 1.0 + 0.045 * C_bar_prime;
    double S_H = 1.0 + 0.015 * C_bar_prime * T;
    double R_T = -std::sin(degToRad(2.0 * delta_theta)) * R_C;

    double dE = std::sqrt(std::pow(delta_L_prime / S_L, 2) +
                          std::pow(delta_C_prime / S_C, 2) +
                          std::pow(delta_H_prime / S_H, 2) +
                          R_T * (delta_C_prime / S_C) * (delta_H_prime / S_H));
    return dE;
}

} // namespace QuarkMeta
```

---

## 3. `src/ui/UiHelper.h` 净化与平滑转发

彻底切断对 `src/ui/` 目录下旧色彩类的包含，全部平滑转发给底层的 `ColorPaletteEngine`（确保全系统既有代码 100% 正常编译）：

```cpp
#pragma once

#include <QIcon>
#include <QString>
#include <QColor>
#include <QPixmap>
#include <QSize>
#include <QWidget>
#include <QVector>
#include <QPair>

#include "SvgIconRenderer.h"
#include "../util/ColorPaletteEngine.h" // 👈 纯净指向底层算法引擎

namespace QuarkMeta {

class UiHelper {
public:
    static inline QColor parseColorName(const QString& colorName) {
        return ColorPaletteEngine::parseColorName(colorName);
    }

    static inline QString normalizeColorHex(const QString& colorStr) {
        return ColorPaletteEngine::normalizeColorHex(colorStr);
    }

    static inline bool isGraphicsFile(const QString& ext) {
        return ColorPaletteEngine::isGraphicsFile(ext);
    }

    static inline bool isStandardImage(const QString& ext) {
        return ColorPaletteEngine::isStandardImage(ext);
    }

    static inline QColor getExtensionColor(const QString& ext) {
        return ColorPaletteEngine::getExtensionColor(ext);
    }

    static inline QColor quantizeColor(const QColor& color) {
        return ColorPaletteEngine::quantizeToStandardColor(color);
    }

    static inline double calculateDeltaE(const QColor& c1, const QColor& c2) {
        return ColorPaletteEngine::calculateDeltaE(c1, c2);
    }

    static inline QVector<QPair<QColor, float>> extractPalette(const QString& targetFile) {
        return ColorPaletteEngine::extractPalette(targetFile);
    }

    static inline QColor extractDominantColor(const QString& targetFile) {
        return ColorPaletteEngine::extractDominantColor(targetFile);
    }

    // 纯 UI 图标与样式渲染保持原生转发
    static inline QPixmap renderIcon(const QString& key, const QSize& size, const QColor& color) {
        return SvgIconRenderer::renderIcon(key, size, color);
    }

    static inline QString getSvgDataUrl(const QString& key, const QColor& color = QColor("#3498db")) {
        return SvgIconRenderer::getSvgDataUrl(key, color);
    }

    static inline QString getSvgTempFilePath(const QString& key, const QColor& color) {
        return SvgIconRenderer::getSvgTempFilePath(key, color);
    }

    static inline QIcon getIcon(const QString& key, const QColor& color, int size = 18) {
        return SvgIconRenderer::getIcon(key, color, size);
    }

    static inline QPixmap getPixmap(const QString& key, const QSize& size, const QColor& color) {
        return SvgIconRenderer::getPixmap(key, size, color);
    }

    static inline void applyMenuStyle(QWidget* menu) {
        SvgIconRenderer::applyMenuStyle(menu);
    }
};

} // namespace QuarkMeta
```

---

## 4. 物理清理与构建配置更新

### 4.1 物理删除废弃文件
- 删除 `src/ui/MediaColorExtractor.h`
- 删除 `src/ui/MediaColorExtractor.cpp`
- 删除 `src/ui/ColorAlgorithmEngine.h`
- 删除 `src/ui/ColorAlgorithmEngine.cpp`

### 4.2 `CMakeLists.txt` 构建配置注册
```cmake
set(UTIL_SOURCES
    # ... 现有 util 源文件 ...
    src/util/ColorPaletteEngine.h
    src/util/ColorPaletteEngine.cpp
)

set(UI_SOURCES
    # ... 现有 UI 源文件 ...
    # 🚨 彻底移除 MediaColorExtractor 与 ColorAlgorithmEngine
)
```