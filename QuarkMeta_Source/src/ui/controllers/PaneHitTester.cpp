#include "PaneHitTester.h"
#include "../ContentPanel.h"

namespace QuarkMeta {

PaneHitTester::HitResult PaneHitTester::evaluateDropAtPosition(
    const QPoint& globalPos, 
    ContentPanel* sourcePane, 
    const QList<ContentPanel*>& allPanes, 
    ContentPanel* hostPanel) 
{
    HitResult res;
    if (!hostPanel) return res;

    for (ContentPanel* target : allPanes) {
        if (!target || !target->isVisible()) continue;
        if (sourcePane && target == sourcePane) continue; // 不能落在自己身上

        QPoint localPos = target->mapFromGlobal(globalPos);
        QRect targetRect = target->rect();
        if (!targetRect.contains(localPos)) continue;

        int w = targetRect.width();
        int h = targetRect.height();
        if (w <= 0 || h <= 0) continue;

        double relX = static_cast<double>(localPos.x()) / w;
        double relY = static_cast<double>(localPos.y()) / h;

        bool isValid = false;
        Qt::Orientation ori = Qt::Horizontal;
        bool insertBefore = false;
        QRect highlightLocal;

        if (relX < 0.25) {
            isValid = true;
            ori = Qt::Horizontal;
            insertBefore = true;
            highlightLocal = QRect(0, 0, w / 2, h);
        } else if (relX > 0.75) {
            isValid = true;
            ori = Qt::Horizontal;
            insertBefore = false;
            highlightLocal = QRect(w / 2, 0, w / 2, h);
        } else if (relY < 0.25) {
            isValid = true;
            ori = Qt::Vertical;
            insertBefore = true;
            highlightLocal = QRect(0, 0, w, h / 2);
        } else if (relY > 0.75) {
            isValid = true;
            ori = Qt::Vertical;
            insertBefore = false;
            highlightLocal = QRect(0, h / 2, w, h / 2);
        }

        if (isValid) {
            // 最小尺寸校验：验证插入后目标区域空间是否足够
            int reqWidth = (ori == Qt::Horizontal) ? ContentPanel::kMinPaneWidth * 2 : ContentPanel::kMinPaneWidth;
            int reqHeight = (ori == Qt::Vertical) ? ContentPanel::kMinPaneHeight * 2 : ContentPanel::kMinPaneHeight;
            if (w < reqWidth || h < reqHeight) {
                isValid = false;
            }
        }

        if (isValid) {
            res.isValid = true;
            res.orientation = ori;
            res.insertBefore = insertBefore;
            res.targetPanel = target;

            QPoint globalTopLeft = target->mapToGlobal(highlightLocal.topLeft());
            QPoint hostTopLeft = hostPanel->mapFromGlobal(globalTopLeft);
            res.highlightRect = QRect(hostTopLeft, highlightLocal.size());
            return res;
        }
    }

    return res;
}

PaneHitTester::HitResult PaneHitTester::evaluateSplitDrop(const QPoint& pos, const QSize& refSize) {
    HitResult res;
    int w = refSize.width();
    int h = refSize.height();
    if (w <= 0 || h <= 0) return res;

    if (pos.x() < w * 0.25) {
        res.isValid = true;
        res.orientation = Qt::Horizontal;
        res.insertBefore = true;
        res.highlightRect = QRect(0, 0, w / 2, h);
    } else if (pos.x() > w * 0.75) {
        res.isValid = true;
        res.orientation = Qt::Horizontal;
        res.insertBefore = false;
        res.highlightRect = QRect(w / 2, 0, w / 2, h);
    } else if (pos.y() < h * 0.25) {
        res.isValid = true;
        res.orientation = Qt::Vertical;
        res.insertBefore = true;
        res.highlightRect = QRect(0, 0, w, h / 2);
    } else if (pos.y() > h * 0.75) {
        res.isValid = true;
        res.orientation = Qt::Vertical;
        res.insertBefore = false;
        res.highlightRect = QRect(0, h / 2, w, h / 2);
    }
    return res;
}

} // namespace QuarkMeta
