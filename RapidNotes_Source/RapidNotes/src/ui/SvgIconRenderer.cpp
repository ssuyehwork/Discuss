#include "SvgIconRenderer.h"
#include "SvgIcons.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QBuffer>
#include <QDir>
#include <QMutexLocker>
#include <mutex>

namespace RapidNotes {

namespace {
    static std::mutex s_qtGuiMutex;
}

QMap<QString, QPixmap>& SvgIconRenderer::iconPixmapCache() {
    static QMap<QString, QPixmap> s_cache;
    return s_cache;
}

QMutex& SvgIconRenderer::iconMutex() {
    static QMutex s_mutex;
    return s_mutex;
}

QPixmap SvgIconRenderer::renderIcon(const QString& key, const QSize& size, const QColor& color) {
    if (!SvgIcons::icons.contains(key)) return QPixmap();
    QString svgData = SvgIcons::icons[key];
    svgData.replace("currentColor", color.name());

    std::lock_guard<std::mutex> guard(s_qtGuiMutex);
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QSvgRenderer renderer(svgData.toUtf8());

    QSize defaultSz = renderer.defaultSize();
    QRectF targetRect(0, 0, size.width(), size.height());
    if (defaultSz.isValid() && !defaultSz.isEmpty()) {
        QSizeF scaled = defaultSz.scaled(size, Qt::KeepAspectRatio);
        qreal x = (size.width() - scaled.width()) / 2.0;
        qreal y = (size.height() - scaled.height()) / 2.0;
        targetRect = QRectF(x, y, scaled.width(), scaled.height());
    }

    renderer.render(&painter, targetRect);
    return pixmap;
}

QString SvgIconRenderer::getSvgDataUrl(const QString& key, const QColor& color) {
    if (!SvgIcons::icons.contains(key)) return QString();
    QString svgData = SvgIcons::icons[key];
    svgData.replace("currentColor", color.name());
    return QString("data:image/svg+xml;utf8,%1").arg(QString::fromUtf8(svgData.toUtf8().toPercentEncoding()));
}

QIcon SvgIconRenderer::getIcon(const QString& key, const QColor& color, int size) {
    QString cacheKey = QString("%1_%2_%3").arg(key).arg(color.name()).arg(size);
    QMutexLocker locker(&iconMutex());
    if (iconPixmapCache().contains(cacheKey)) {
        return QIcon(iconPixmapCache().value(cacheKey));
    }

    QPixmap pixmap = renderIcon(key, QSize(size, size), color);
    if (!pixmap.isNull()) {
        iconPixmapCache().insert(cacheKey, pixmap);
        return QIcon(pixmap);
    }
    return QIcon();
}

QPixmap SvgIconRenderer::getPixmap(const QString& key, const QSize& size, const QColor& color) {
    QString cacheKey = QString("%1_%2_%3x%4").arg(key).arg(color.name()).arg(size.width()).arg(size.height());
    QMutexLocker locker(&iconMutex());
    if (iconPixmapCache().contains(cacheKey)) {
        return iconPixmapCache().value(cacheKey);
    }

    QPixmap pixmap = renderIcon(key, size, color);
    if (!pixmap.isNull()) {
        iconPixmapCache().insert(cacheKey, pixmap);
        return pixmap;
    }
    return QPixmap();
}

QString SvgIconRenderer::getSvgTempFilePath(const QString& key, const QColor& color) {
    if (!SvgIcons::icons.contains(key)) return QString();
    QString svgData = SvgIcons::icons[key];
    svgData.replace("currentColor", color.name());

    QString tempDir = QDir::tempPath() + "/RapidNotes_Icons";
    QDir().mkpath(tempDir);
    QString filePath = QString("%1/%2_%3.svg").arg(tempDir).arg(key).arg(color.name().mid(1));

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(svgData.toUtf8());
        file.close();
        return filePath;
    }
    return QString();
}

} // namespace RapidNotes
