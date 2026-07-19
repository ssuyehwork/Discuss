#ifndef ARCMETA_UI_HELPER_H
#define ARCMETA_UI_HELPER_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <QIcon>
#include <QString>
#include <QColor>
#include <QPixmap>
#include <QMap>
#include <QMutex>
#include <QWidget>
#include "StylePainter.h"
#include "PaletteAnalyzer.h"
#include "ShellIconManager.h"

namespace ArcMeta {

class IconLoadNotifier : public QObject {
    Q_OBJECT
signals:
    void iconLoaded();
public:
    static IconLoadNotifier& instance() {
        static IconLoadNotifier inst;
        return inst;
    }
private:
    IconLoadNotifier(QObject* parent = nullptr) : QObject(parent) {
        // 桥接 ShellIconManager 信号
        connect(&ShellIconManager::instance(), &ShellIconManager::iconLoaded, this, &IconLoadNotifier::iconLoaded);
    }
};

class UiHelper {
public:
    static QMap<QString, QPixmap>& iconPixmapCache() {
        static QMap<QString, QPixmap> cache;
        return cache;
    }

    static QMutex& iconMutex() {
        static QMutex mutex;
        return mutex;
    }

    static void initializeHotIcons() {}

    static QColor parseColorName(const QString& colorName) {
        if (colorName.isEmpty()) return QColor();
        QColor c(colorName);
        if (c.isValid()) return c;

        if (colorName == "red" || colorName == "红") return QColor("#E24B4A");
        if (colorName == "orange" || colorName == "橙") return QColor("#EF9F27");
        if (colorName == "yellow" || colorName == "黄") return QColor("#FECF0E");
        if (colorName == "green" || colorName == "绿") return QColor("#639922");
        if (colorName == "cyan" || colorName == "青") return QColor("#1D9E75");
        if (colorName == "blue" || colorName == "蓝") return QColor("#378ADD");
        if (colorName == "purple" || colorName == "紫") return QColor("#7F77DD");
        if (colorName == "gray" || colorName == "灰") return QColor("#5F5E5A");
        if (colorName == "black" || colorName == "黑") return QColor("#000000");
        if (colorName == "white" || colorName == "白") return QColor("#FFFFFF");
        return QColor();
    }

    static QPixmap renderIcon(const QString& key, const QSize& size, const QColor& color) {
        return StylePainter::renderIcon(key, size, color);
    }

    static QString getSvgDataUrl(const QString& key, const QColor& color = QColor("#3498db")) {
        return StylePainter::getSvgDataUrl(key, color);
    }

    static QString getSvgTempFilePath(const QString& key, const QColor& color) {
        return StylePainter::getSvgTempFilePath(key, color);
    }

    static bool isGraphicsFile(const QString& ext) {
        static const QStringList graphicsExts = {
            "png", "jpg", "jpeg", "bmp", "gif", "webp", "ico", "tiff", "tif",
            "psd", "psb", "ai", "eps", "pdf", "svg", "cdr",
            "sketch", "xd", "fig", "dwg", "dxf", "heic", "raw",
            "mp4", "mkv", "avi", "mov", "wmv", "flv", "webm"
        };
        return graphicsExts.contains(ext.toLower());
    }

    static bool isStandardImage(const QString& ext) {
        static const QStringList standardExts = {
            "png", "jpg", "jpeg", "bmp", "gif", "webp", "ico"
        };
        return standardExts.contains(ext.toLower());
    }

    static QIcon getIcon(const QString& key, const QColor& color, int size = 18) {
        QIcon icon;
        QPixmap pix = getPixmap(key, QSize(size, size), color);
        if (!pix.isNull()) icon.addPixmap(pix);
        return icon;
    }

    static QIcon getFileIcon(const QString& filePath, int size = 18, const QColor& overrideColor = QColor()) {
        Q_UNUSED(overrideColor);
        return ShellIconManager::instance().getFileIcon(filePath, size);
    }

    static QPixmap getPixmap(const QString& key, const QSize& size, const QColor& color) {
        QString cKey = QString("%1_%2_%3_%4").arg(key).arg(size.width()).arg(size.height()).arg(color.rgba());
        {
            QMutexLocker locker(&iconMutex());
            if (iconPixmapCache().contains(cKey)) return iconPixmapCache()[cKey];
        }
        QPixmap rendered = renderIcon(key, size, color);
        if (rendered.isNull()) return rendered;
        QMutexLocker locker(&iconMutex());
        iconPixmapCache().insert(cKey, rendered);
        return rendered;
    }

    static void applyMenuStyle(QWidget* menu) {
        StylePainter::applyMenuStyle(menu);
    }

    static QColor getExtensionColor(const QString& ext) {
        return StylePainter::getExtensionColor(ext);
    }

    static inline QColor quantizeColor(const QColor& color) {
        return color;
    }

    static double calculateDeltaE(const QColor& c1, const QColor& c2) {
        return PaletteAnalyzer::calculateDeltaE(c1, c2);
    }

    static QVector<QPair<QColor, float>> extractPalette(const QString& targetFile) {
        return PaletteAnalyzer::extractPalette(targetFile);
    }

    static inline QColor extractDominantColor(const QString& targetFile) {
        auto palette = extractPalette(targetFile);
        return palette.isEmpty() ? QColor() : palette.first().first;
    }

    static QImage getImageForAnalysis(const QString& path, int size = 256);
    static QImage getShellThumbnail(const QString& path, int size);
};

} // namespace ArcMeta

#endif // ARCMETA_UI_HELPER_H
