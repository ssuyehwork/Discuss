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
#include <QSet>
#include <QStringList>
#include <QFileInfo>
#include <QFile>

#include "SvgIconRenderer.h"
#include "ThemeManager.h"
#include "../util/ColorPaletteEngine.h"
#include "../core/AppConfig.h"

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

    static inline QStringList getBuiltInTextExtensions() {
        return {
            "txt", "md", "markdown", "log", "rtf", "tex",
            "bat", "cmd", "ps1", "sh", "bash", "zsh", "fish", "vbs",
            "json", "xml", "yaml", "yml", "ini", "conf", "config", "toml", "cmake", "qrc", "rc", "manifest", "properties", "env",
            "cpp", "cxx", "cc", "c", "h", "hpp", "hxx", "py", "js", "mjs", "ts", "jsx", "tsx", "html", "htm", "css", "scss", "sass", "less", "vue",
            "php", "rb", "rs", "go", "java", "cs", "sql", "swift", "kt", "kts", "lua", "pl", "r", "dart", "asm", "s",
            "diff", "patch", "gradle", "properties", "dockerfile", "makefile", "mk", "sol", "proto"
        };
    }

    static inline QStringList getCustomTextExtensions() {
        QVariant val = AppConfig::instance().getValue("QuickLook/CustomTextExtensions");
        if (val.isValid() && val.canConvert<QStringList>()) {
            return val.toStringList();
        }
        return QStringList();
    }

    static inline void setCustomTextExtensions(const QStringList& exts) {
        QStringList cleaned;
        for (QString ext : exts) {
            ext = ext.trimmed().toLower();
            if (ext.startsWith('.')) ext = ext.mid(1);
            if (!ext.isEmpty() && !cleaned.contains(ext)) {
                cleaned << ext;
            }
        }
        AppConfig::instance().setValue("QuickLook/CustomTextExtensions", cleaned);
        AppConfig::instance().sync();
    }

    static inline bool canPreviewFile(const QString& path) {
        QFileInfo fi(path);
        if (fi.isDir()) return false;

        QString ext = fi.suffix().toLower();
        if (isGraphicsFile(ext) || isTextFile(ext) || ext == "pdf") {
            return true;
        }

        static const QSet<QString> unpreviewableExts = {
            "zip", "rar", "7z", "tar", "gz", "bz2", "xz", "exe", "dll", "msi", "sys", "iso", "dmg", "pkg", "bin", "lnk",
            "mp4", "m4v", "mov", "avi", "mkv", "wmv", "flv", "webm", "3gp", "mp3", "wav", "wma", "flac", "aac", "ogg", "m4a", "ape"
        };
        if (unpreviewableExts.contains(ext)) {
            return false;
        }

        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray header = file.read(512);
            file.close();
            if (!header.isEmpty()) {
                int continuousNulls = 0;
                for (int i = 0; i < header.size(); ++i) {
                    if (header[i] == '\0') {
                        continuousNulls++;
                        if (continuousNulls > 2) return false;
                    } else {
                        continuousNulls = 0;
                    }
                }
                return true;
            }
        }
        return false;
    }

    static inline bool isTextFile(const QString& ext) {
        QString cleanExt = ext.trimmed().toLower();
        if (cleanExt.startsWith('.')) cleanExt = cleanExt.mid(1);
        if (cleanExt.isEmpty()) return false;

        static const QSet<QString> builtIn = []() {
            const auto list = getBuiltInTextExtensions();
            return QSet<QString>(list.begin(), list.end());
        }();
        if (builtIn.contains(cleanExt)) return true;

        QStringList custom = getCustomTextExtensions();
        for (const QString& cExt : custom) {
            if (cExt.trimmed().toLower() == cleanExt) return true;
        }

        return false;
    }

    static inline QIcon getIcon(const QString& key, const QColor& color, int size = 18) {
        return SvgIconRenderer::getIcon(key, color, size);
    }

    static inline QPixmap getPixmap(const QString& key, const QSize& size, const QColor& color) {
        return SvgIconRenderer::getPixmap(key, size, color);
    }

    static inline void applyMenuStyle(QWidget* menu) {
        ThemeManager::instance().applyMenuStyle(menu);
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
