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
#include "../util/ColorPaletteEngine.h"

namespace QuarkMeta {

/**
 * @brief UI 辅助兼容及转发层 (完全解耦重构版)
 */
class UiHelper {
public:
    static inline QColor parseColorName(const QString& colorName) {
        return ColorPaletteEngine::parseColorName(colorName);
    }

    static inline QString normalizeColorHex(const QString& colorStr) {
        return ColorPaletteEngine::normalizeColorHex(colorStr);
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
        return ColorPaletteEngine::isGraphicsFile(ext);
    }

    static inline bool isStandardImage(const QString& ext) {
        return ColorPaletteEngine::isStandardImage(ext);
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
};

} // namespace QuarkMeta
