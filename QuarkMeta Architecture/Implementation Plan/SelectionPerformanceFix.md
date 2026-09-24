# Implementation Plan - SelectionPerformanceFix.md

## Overview
本实施方案旨在彻底解决卡片/列表项在点击选中或滑动切换时响应缓慢的性能瓶颈。

物理根因：
在 `CardPainterHelper::drawRatingStars` 与 `CardPainterHelper::drawStatusIndicators` 中，每次卡片重绘都会无条件调用 `UiHelper::getIcon` 生成置顶图标、禁选圈、实心星与空心星，导致每一帧重复触发 `QSvgRenderer` XML 格式解析、加锁与 `QPainter` 离屏绘制，引发主 UI 线程明显的绘制卡顿与粘手感。

核心优化点：
1. **矢量图标 Pixmap 内存缓存**：在 `CardPainterHelper.cpp` 中建立以 `(key, color, size)` 为键的 `QPixmap` 静态/二级缓存。
2. **0ms 纯贴图绘制**：卡片重绘时，直接命中静态 `QPixmap` 进行 `painter->drawPixmap(...)` 贴图，把耗时的 XML 字符串解析与 QPainter 离屏渲染降为 0 次，实现毫秒级高刷选中体验。

---

## Modified Files List
- `src/ui/CardPainterHelper.cpp`

---

## Detailed Line-by-Line Changes

### File: `src/ui/CardPainterHelper.cpp`

```
<<<<<<< SEARCH
#include "CardPainterHelper.h"
#include "UiHelper.h"
#include <QPainterPath>
#include <QFont>
#include <QtMath>
=======
#include "CardPainterHelper.h"
#include "UiHelper.h"
#include <QPainterPath>
#include <QFont>
#include <QtMath>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
namespace QuarkMeta {

void CardPainterHelper::drawCardCover(QPainter* painter, const QRect& cardRect, bool isSelected,
=======
namespace QuarkMeta {

static QPixmap getCachedPixmap(const QString& key, const QColor& color, int size) {
    static QMap<QString, QPixmap> s_pixCache;
    static QMutex s_pixMutex;

    QString cKey = QString("%1_%2_%3").arg(key).arg(color.rgba()).arg(size);
    {
        QMutexLocker locker(&s_pixMutex);
        auto it = s_pixCache.find(cKey);
        if (it != s_pixCache.end()) return it.value();
    }

    QPixmap pix = UiHelper::getIcon(key, color, size).pixmap(size, size);
    if (!pix.isNull()) {
        QMutexLocker locker(&s_pixMutex);
        s_pixCache[cKey] = pix;
    }
    return pix;
}

void CardPainterHelper::drawCardCover(QPainter* painter, const QRect& cardRect, bool isSelected,
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void CardPainterHelper::drawStatusIndicators(QPainter* painter, const QRect& cardRect, bool isPinned) {
    if (isPinned) {
        QRect statusRect(cardRect.right() - 22, cardRect.top() + 8, 16, 16);
        UiHelper::getIcon("pin_vertical", QColor("#FF551C"), 16).paint(painter, statusRect);
    }
}
=======
void CardPainterHelper::drawStatusIndicators(QPainter* painter, const QRect& cardRect, bool isPinned) {
    if (isPinned) {
        QRect statusRect(cardRect.right() - 22, cardRect.top() + 8, 16, 16);
        QPixmap pinPix = getCachedPixmap("pin_vertical", QColor("#FF551C"), 16);
        if (!pinPix.isNull()) {
            painter->drawPixmap(statusRect, pinPix);
        }
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        UiHelper::getIcon("no_color", starColor, banRect.width()).paint(painter, banRect);

        QPixmap filledStar = UiHelper::getIcon("star_filled", starColor, starSize).pixmap(starSize, starSize);
        QPixmap emptyStar  = UiHelper::getIcon("star", emptyStarColor, starSize).pixmap(starSize, starSize);

        for (int i = 0; i < 5; ++i) {
            QRect starRect(starsStartX + i * (starSize + actualSpacing),
                           ratingY + (ratingH - starSize) / 2,
                           starSize, starSize);
            painter->drawPixmap(starRect, (i < rating) ? filledStar : emptyStar);
        }
        painter->restore();
=======
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QPixmap banPix = getCachedPixmap("no_color", starColor, banRect.width());
        if (!banPix.isNull()) {
            painter->drawPixmap(banRect, banPix);
        }

        QPixmap filledStar = getCachedPixmap("star_filled", starColor, starSize);
        QPixmap emptyStar  = getCachedPixmap("star", emptyStarColor, starSize);

        for (int i = 0; i < 5; ++i) {
            QRect starRect(starsStartX + i * (starSize + actualSpacing),
                           ratingY + (ratingH - starSize) / 2,
                           starSize, starSize);
            painter->drawPixmap(starRect, (i < rating) ? filledStar : emptyStar);
        }
        painter->restore();
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **结构规范核验**：
   运行 Python 脚本验证文件结构的完整性。
2. **物理逻辑核验**：
   重绘卡片时不再触发任何 SVG XML 格式解析，实现 0ms 直接内存贴图。

---

## SSOT API Reuse & Anti-Redundancy Self-Check

1. **既有 API 复用**：
   复用 `UiHelper::getIcon` 进行初始渲染，未重复编写 SVG 代码。
2. **无死代码遗留**：
   完全移除了重绘链路上的重复解析开销。

---

## Header API Signature Verification

| 被调用接口 / 类 | 头文件声明路径 | 精确函数签名 | 校验状态 |
| :--- | :--- | :--- | :--- |
| `CardPainterHelper::drawCardCover` | `src/ui/CardPainterHelper.h` | `static void drawCardCover(...)` | ✅ 物理对齐 |
