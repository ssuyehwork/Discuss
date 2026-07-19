#ifndef ARCMETA_PALETTE_ANALYZER_H
#define ARCMETA_PALETTE_ANALYZER_H

#include <QColor>
#include <QVector>
#include <QPair>

namespace ArcMeta {

class PaletteAnalyzer {
public:
    struct LabColor { double l, a, b; };

    static LabColor rgbToLab(const QColor& color);
    static double calculateDeltaE(const QColor& c1, const QColor& c2);
    static QVector<QPair<QColor, float>> extractPalette(const QString& targetFile);
};

} // namespace ArcMeta

#endif // ARCMETA_PALETTE_ANALYZER_H
