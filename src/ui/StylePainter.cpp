#include "StylePainter.h"
#include "SvgIcons.h"
#include "../core/AppConfig.h"
#include <QSvgRenderer>
#include <QPainter>
#include <QBuffer>
#include <QDir>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>

namespace ArcMeta {

static QMap<QString, QPixmap>& styleIconPixmapCache() {
    static QMap<QString, QPixmap> cache;
    return cache;
}

static QMutex& styleIconMutex() {
    static QMutex mutex;
    return mutex;
}

QPixmap StylePainter::renderIcon(const QString& key, const QSize& size, const QColor& color) {
    if (!SvgIcons::icons.contains(key)) return QPixmap();
    QString svgData = SvgIcons::icons[key];
    svgData.replace("currentColor", color.name());
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QSvgRenderer renderer(svgData.toUtf8());
    renderer.render(&painter);
    return pixmap;
}

QString StylePainter::getSvgDataUrl(const QString& key, const QColor& color) {
    QPixmap pix = renderIcon(key, QSize(20, 20), color);
    if (pix.isNull()) return QString();
    
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    pix.save(&buffer, "PNG");
    return QString("data:image/png;base64,%1").arg(QString(ba.toBase64()));
}

QString StylePainter::getSvgTempFilePath(const QString& key, const QColor& color) {
    QPixmap pix = renderIcon(key, QSize(20, 20), color);
    if (pix.isNull()) return QString();

    QString tmpPath = QDir::temp().filePath(
        QString("arcmeta_%1_%2_v3.png").arg(key).arg(color.name().mid(1))
    );
    pix.save(tmpPath, "PNG");
    return QDir::fromNativeSeparators(tmpPath);
}

void StylePainter::applyMenuStyle(QWidget* menu) {
    if (!menu) return;
    menu->setAttribute(Qt::WA_TranslucentBackground);
    menu->setWindowFlag(Qt::FramelessWindowHint);

    QString arrowPath = getSvgTempFilePath("menu_triangle", QColor("#CCCCCC"));

    menu->setStyleSheet(QString(
        "QMenu { background-color: #2D2D2D; color: #EEE; border: 1px solid #444; padding: 4px; border-radius: 8px; }"
        "QMenu::item { padding: 6px 25px 6px 10px; border-radius: 4px; font-size: 12px; }"
        "QMenu::item:selected { background-color: #3E3E42; color: white; }"
        "QMenu::separator { height: 1px; background: #444; margin: 4px 8px; }"
        "QMenu::right-arrow { "
        "  image: url(%1); "
        "  subcontrol-origin: padding; "
        "  subcontrol-position: center right; "
        "  right: 8px; "
        "}"
    ).arg(arrowPath));
}

QColor StylePainter::getExtensionColor(const QString& ext) {
    static QMap<QString, QColor> s_cache;
    QString upperExt = ext.toUpper();
    if (upperExt == "DIR") return QColor(45, 65, 85, 200);
    if (upperExt.isEmpty()) return QColor(60, 60, 60, 180);
    if (s_cache.contains(upperExt)) return s_cache[upperExt];

    QString settingKey = QString("ExtensionColors/%1").arg(upperExt);
    QVariant val = AppConfig::instance().getValue(settingKey);
    if (val.isValid()) {
        QColor color = val.value<QColor>();
        s_cache[upperExt] = color;
        return color;
    }

    size_t hash = qHash(upperExt);
    int hue = static_cast<int>(hash % 360);
    QColor color = QColor::fromHsl(hue, 160, 110, 200); 
    s_cache[upperExt] = color;
    AppConfig::instance().setValue(settingKey, color);
    return color;
}

} // namespace ArcMeta
