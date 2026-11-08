#pragma once

#include <QPoint>
#include <QRect>
#include <QSize>
#include <QList>
#include <Qt>

namespace QuarkMeta {

class ContentPanel;

class PaneHitTester {
public:
    struct HitResult {
        bool isValid = false;
        Qt::Orientation orientation = Qt::Horizontal;
        bool insertBefore = false;
        QRect highlightRect;
        ContentPanel* targetPanel = nullptr;
    };

    static HitResult evaluateDropAtPosition(const QPoint& globalPos, 
                                            ContentPanel* sourcePane, 
                                            const QList<ContentPanel*>& allPanes, 
                                            ContentPanel* hostPanel);

    static HitResult evaluateSplitDrop(const QPoint& pos, const QSize& refSize);
};

} // namespace QuarkMeta
