#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <QIcon>
#include <QString>
#include <QColor>
#include <QPixmap>
#include <QSize>
#include <QWidget>
#include <QImage>
#include <QVector>
#include <QPair>
#include <QDebug>

#include "SvgIconRenderer.h"
#include "MediaColorExtractor.h"

namespace QuarkMeta {

/**
 * @brief UI 辅助兼容及转发层 (完全解耦重构版)
 */
class UiHelper {
public:
    static inline QColor parseColorName(const QString& colorName) {
        if (colorName.isEmpty()) return QColor();
        
        QColor c(colorName);
        if (c.isValid()) return c;

        if (colorName == "red" || colorName == "红" || colorName == "红色") return QColor("#E24B4A");
        if (colorName == "orange" || colorName == "橙" || colorName == "橙色") return QColor("#EF9F27");
        if (colorName == "yellow" || colorName == "黄" || colorName == "黄色") return QColor("#FECF0E");
        if (colorName == "green" || colorName == "绿" || colorName == "绿色") return QColor("#639922");
        if (colorName == "cyan" || colorName == "青" || colorName == "青色") return QColor("#1D9E75");
        if (colorName == "blue" || colorName == "蓝" || colorName == "蓝色") return QColor("#378ADD");
        if (colorName == "purple" || colorName == "紫" || colorName == "紫色") return QColor("#7F77DD");
        if (colorName == "gray" || colorName == "灰" || colorName == "灰色") return QColor("#5F5E5A");
        if (colorName == "black" || colorName == "黑" || colorName == "黑色") return QColor("#000000");
        if (colorName == "white" || colorName == "白" || colorName == "白色") return QColor("#FFFFFF");
        
        return QColor();
    }

    static inline QString normalizeColorHex(const QString& colorStr) {
        if (colorStr.trimmed().isEmpty()) return "";
        QColor c = parseColorName(colorStr);
        if (!c.isValid()) return "";
        return c.name().toUpper(); // 恒定返回 "#FECF0E" 格式
    }

    static inline QPixmap renderIcon(const QString& key, const QSize& size, const QColor& color) {
        return SvgIconRenderer::renderIcon(key, size, color);
    }

    static inline QString getSvgDataUrl(const QString& key, const QColor& color = QColor("#3498db")) {
        return SvgIconRenderer::getSvgDataUrl(key, color);
    }

    static inline QString getSvgTempFilePath(const QString& key, const QColor& color) {
        return SvgIconRenderer::getSvgTempFilePath(key, color);
    }

    static inline bool isGraphicsFile(const QString& ext) {
        return MediaColorExtractor::isGraphicsFile(ext);
    }

    static inline bool isStandardImage(const QString& ext) {
        return MediaColorExtractor::isStandardImage(ext);
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

    static inline QColor getExtensionColor(const QString& ext) {
        return MediaColorExtractor::getExtensionColor(ext);
    }

    static inline QColor quantizeColor(const QColor& color) {
        return MediaColorExtractor::quantizeColor(color);
    }

    static inline double calculateDeltaE(const QColor& c1, const QColor& c2) {
        return MediaColorExtractor::calculateDeltaE(c1, c2);
    }

    static inline QVector<QPair<QColor, float>> extractPalette(const QString& targetFile) {
        return MediaColorExtractor::extractPalette(targetFile);
    }

    static inline QColor extractDominantColor(const QString& targetFile) {
        return MediaColorExtractor::extractDominantColor(targetFile);
    }
};

} // namespace QuarkMeta
