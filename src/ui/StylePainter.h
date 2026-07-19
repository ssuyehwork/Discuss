#ifndef ARCMETA_STYLE_PAINTER_H
#define ARCMETA_STYLE_PAINTER_H

#include <QColor>
#include <QSize>
#include <QPixmap>
#include <QWidget>

namespace ArcMeta {

class StylePainter {
public:
    static QPixmap renderIcon(const QString& key, const QSize& size, const QColor& color);
    static QString getSvgDataUrl(const QString& key, const QColor& color = QColor("#3498db"));
    static QString getSvgTempFilePath(const QString& key, const QColor& color);
    static void applyMenuStyle(QWidget* menu);
    static QColor getExtensionColor(const QString& ext);
};

} // namespace ArcMeta

#endif // ARCMETA_STYLE_PAINTER_H
