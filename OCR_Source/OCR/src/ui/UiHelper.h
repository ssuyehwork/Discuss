#ifndef UIHELPER_H
#define UIHELPER_H

#include <QString>
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QSize>
#include "IconHelper.h"

class UiHelper {
public:
    static QIcon getIcon(const QString& name, const QColor& color = QColor("#CCCCCC"), int size = 16) {
        return IconHelper::getIcon(name, color.name(QColor::HexRgb), size);
    }

    static QPixmap getPixmap(const QString& name, const QSize& size = QSize(16, 16), const QColor& color = QColor("#AAAAAA")) {
        int maxDim = qMax(size.width(), size.height());
        QIcon icon = IconHelper::getIcon(name, color.name(QColor::HexRgb), maxDim);
        return icon.pixmap(size);
    }
};

#endif // UIHELPER_H
