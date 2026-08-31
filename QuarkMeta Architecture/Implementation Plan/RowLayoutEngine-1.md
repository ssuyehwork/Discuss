# QuarkMeta 架构重构方案：RowLayoutEngine 列表几何统一引擎

## 一、 Overview
建立单一真理源几何算子 `RowLayoutEngine`，统一管理列表行高（30px~230px 全区间）、微卡片尺寸、表头对齐、文本起始位置及行内编辑框物理布局，消灭 `TreeItemDelegate` 与 `DropTreeView` 中散落的重复计算。

## 二、 Modified Files List
- `src/ui/RowLayoutEngine.h` (新建)
- `src/ui/RowLayoutEngine.cpp` (新建)
- `CMakeLists.txt`
- `src/ui/TreeItemDelegate.h`
- `src/ui/DropTreeView.h`
- `src/ui/ContentPanel.cpp`

## 三、 Detailed Line-by-Line Changes

### 1. `src/ui/RowLayoutEngine.h` (新建)
```cpp
#pragma once
#include <QRect>

namespace QuarkMeta {

struct RowLayout {
    int rowHeight;          // 统一行高 (30px ~ 230px)
    QRect cardRect;         // 左侧正方形微卡片物理区域 (side x side)
    QRect textRect;         // 右侧文件名文本物理区域
    QRect editorRect;       // 行内重命名编辑框物理区域
    int headerTextStartX;   // 表头第 0 列名称文字绝对对齐的物理起点 X
};

class RowLayoutEngine {
public:
    static constexpr int kLeftMargin = 6;      // 左外边距 6px
    static constexpr int kVerticalPadding = 3; // 上下边距 3px
    static constexpr int kSpacing = 8;         // 卡片与文字间距 8px
    static constexpr int kMinRowHeight = 30;   // 最小行高 30px
    static constexpr int kMaxRowHeight = 230;  // 最大行高 230px

    static RowLayout calculate(const QRect& totalRect, int zoomLevel = 30);
    static int calculateRowHeight(int zoomLevel);
    static int calculateHeaderTextStartX(int zoomLevel);
};

} // namespace QuarkMeta
```

### 2. `src/ui/RowLayoutEngine.cpp` (新建)
```cpp
#include "RowLayoutEngine.h"
#include <algorithm>

namespace QuarkMeta {

int RowLayoutEngine::calculateRowHeight(int zoomLevel) {
    return std::clamp(zoomLevel, kMinRowHeight, kMaxRowHeight);
}

int RowLayoutEngine::calculateHeaderTextStartX(int zoomLevel) {
    int h = calculateRowHeight(zoomLevel);
    int side = std::max(16, h - (kVerticalPadding * 2));
    return kLeftMargin + side + kSpacing;
}

RowLayout RowLayoutEngine::calculate(const QRect& totalRect, int zoomLevel) {
    RowLayout l;
    l.rowHeight = totalRect.height() > 0 ? totalRect.height() : calculateRowHeight(zoomLevel);

    int side = std::max(16, l.rowHeight - (kVerticalPadding * 2));

    l.cardRect = QRect(totalRect.left() + kLeftMargin, 
                       totalRect.top() + kVerticalPadding, 
                       side, side);

    int textStartX = l.cardRect.right() + kSpacing;
    l.headerTextStartX = textStartX - totalRect.left();

    l.textRect = totalRect;
    l.textRect.setLeft(textStartX);

    l.editorRect = l.textRect;
    const int maxEditorH = 28;
    if (l.editorRect.height() > maxEditorH) {
        int diff = l.editorRect.height() - maxEditorH;
        int topAdj = diff / 2;
        int botAdj = diff - topAdj;
        l.editorRect.adjust(0, topAdj, 0, -botAdj);
    } else {
        l.editorRect.adjust(0, 2, 0, -2);
    }

    return l;
}

} // namespace QuarkMeta
```

### 3. `CMakeLists.txt`
```git
<<<<<<< SEARCH
    src/ui/CardLayoutEngine.h
    src/ui/CardLayoutEngine.cpp
=======
    src/ui/CardLayoutEngine.h
    src/ui/CardLayoutEngine.cpp
    src/ui/RowLayoutEngine.h
    src/ui/RowLayoutEngine.cpp
>>>>>>> REPLACE
```

### 4. `src/ui/TreeItemDelegate.h`
```git
<<<<<<< SEARCH
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QSize sz = QStyledItemDelegate::sizeHint(option, index);
        const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(option.widget);
        int iconH = view ? view->iconSize().height() : 24;
        int h = qBound(30, iconH + 8, 230);
        sz.setHeight(h);
        return sz;
    }
=======
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QSize sz = QStyledItemDelegate::sizeHint(option, index);
        const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(option.widget);
        int zoom = view ? view->iconSize().height() + 8 : 30;
        sz.setHeight(RowLayoutEngine::calculateRowHeight(zoom));
        return sz;
    }
>>>>>>> REPLACE
```

### 5. `src/ui/DropTreeView.h`
```git
<<<<<<< SEARCH
        if (logicalIndex == 0) {
            int side = m_zoomLevel - 6;
            if (side <= 0) side = 16;
            int textStartX = rect.left() + 6 + side + 10;

            QRect textRect = rect;
            textRect.setLeft(textStartX);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);
        } else {
=======
        if (logicalIndex == 0) {
            int textStartX = rect.left() + RowLayoutEngine::calculateHeaderTextStartX(m_zoomLevel);
            QRect textRect = rect;
            textRect.setLeft(textStartX);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);
        } else {
>>>>>>> REPLACE
```

### 6. `src/ui/ContentPanel.cpp`
```git
<<<<<<< SEARCH
void ContentPanel::updateGridSize() {
    if (m_viewStack->currentWidget() == m_gridView) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) jv->setTargetRowHeight(m_zoomLevel);
    } else if (m_viewStack->currentWidget() == m_treeView) {
        m_treeView->setIconSize(QSize(qMax(16, m_zoomLevel - 8), qMax(16, m_zoomLevel - 8)));
    }
    AppConfig::instance().setValue("UI/GridZoomLevel", m_zoomLevel);
}
=======
void ContentPanel::updateGridSize() {
    if (m_viewStack->currentWidget() == m_gridView) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
            jv->setTargetRowHeight(m_zoomLevel);
        }
    } else if (m_viewStack->currentWidget() == m_treeView) {
        if (auto* dropTree = qobject_cast<DropTreeView*>(m_treeView)) {
            if (auto* hdr = qobject_cast<ContentHeaderView*>(dropTree->header())) {
                hdr->setZoomLevel(m_zoomLevel);
            }
        }
        m_treeView->setIconSize(QSize(qMax(16, m_zoomLevel - 8), qMax(16, m_zoomLevel - 8)));
        m_treeView->doItemsLayout();
    }
    AppConfig::instance().setValue("UI/GridZoomLevel", m_zoomLevel);
}
>>>>>>> REPLACE
```

## 四、 Build & Verification Steps
1. 校验 `src/ui/RowLayoutEngine.h` 与 `src/ui/RowLayoutEngine.cpp` 的单一定位引擎运算逻辑。
2. 校验 `TreeItemDelegate.h`、`DropTreeView.h`、`ContentPanel.cpp` 与 `CMakeLists.txt` 修改。
3. 确认 30~230px 范围列表行高与表头绝对垂直共线对齐。
