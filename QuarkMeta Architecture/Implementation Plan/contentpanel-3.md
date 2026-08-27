# Implementation Plan - ContentHeaderView Alignment & Stretch Header Section (`contentpanel-3.md`)

## 1. Overview
This implementation plan addresses two layout defects in `ContentPanel` list view:
1. **Right Black Space Area on Window Maximize**: All sections were set to `QHeaderView::Interactive`, leaving no flexible stretch section to fill remaining viewport width. Setting section 0 ("名称") to `QHeaderView::Stretch` ensures it fills extra window width and moves metadata sections neatly to the right.
2. **Title Alignment with Filename**: Custom `ContentHeaderView` inherits `QHeaderView` and overrides `paintSection()` to dynamically compute text start position based on thumbnail size `side = m_zoomLevel - 6` (`textStartX = rect.left() + 6 + side + 10`), matching `TreeItemDelegate` drawing offset.

---

## 2. Modified Files List
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### `src/ui/DropTreeView.h`

```git
<<<<<<< SEARCH
#include <QTreeView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>

namespace QuarkMeta {

class DropTreeView : public QTreeView {
=======
#include <QTreeView>
#include <QHeaderView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>

namespace QuarkMeta {

class ContentHeaderView : public QHeaderView {
    Q_OBJECT
public:
    explicit ContentHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr)
        : QHeaderView(orientation, parent) {}

    void setZoomLevel(int zoom) {
        m_zoomLevel = zoom;
        viewport()->update();
    }

protected:
    void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override {
        if (!rect.isValid()) return;

        painter->save();
        painter->fillRect(rect, QColor("#252525"));
        painter->setPen(QColor("#333333"));
        painter->drawLine(rect.topRight(), rect.bottomRight());

        QString title = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString();
        painter->setPen(QColor("#B0B0B0"));
        painter->setFont(font());

        if (logicalIndex == 0) {
            int side = m_zoomLevel - 6;
            if (side <= 0) side = 16;
            int textStartX = rect.left() + 6 + side + 10;

            QRect textRect = rect;
            textRect.setLeft(textStartX);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);
        } else {
            painter->drawText(rect, Qt::AlignCenter, title);
        }
        painter->restore();
    }

private:
    int m_zoomLevel = 96;
};

class DropTreeView : public QTreeView {
>>>>>>> REPLACE
```

### `src/ui/DropTreeView.cpp`

```git
<<<<<<< SEARCH
DropTreeView::DropTreeView(QWidget* parent) : QTreeView(parent) {
    setAcceptDrops(true);
    setDropIndicatorShown(true);
}
=======
DropTreeView::DropTreeView(QWidget* parent) : QTreeView(parent) {
    setHeader(new ContentHeaderView(Qt::Horizontal, this));
    setAcceptDrops(true);
    setDropIndicatorShown(true);
}
>>>>>>> REPLACE
```

### `src/ui/ContentPanel.cpp`

```git
<<<<<<< SEARCH
    // 统一表头样式：干净、左对齐、无任何破坏性 padding
    m_treeView->header()->setStyleSheet( 
        "QHeaderView::section { background-color: #252525; color: #B0B0B0; border: none; border-right: 1px solid #333333; height: 32px; font-size: 11px; padding: 0 4px; }" 
    ); 
    
    auto* header = m_treeView->header();
    header->setStretchLastSection(false);
    header->setCascadingSectionResizes(true);
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    for (int i = 0; i <= 6; ++i) {
        header->setSectionHidden(i, false);
    }
    header->setSectionHidden(7, true);

    header->setMinimumSectionSize(35);
    
    header->resizeSection(0, 260);  // 【第 0 列名称】：保底 260px，确保缩略图和文件名完美并存！
    header->resizeSection(1, 40);   // 【第 1 列状态】：40px
    header->resizeSection(2, 90);   // 【第 2 列星级】：90px
    header->resizeSection(3, 90);   // 【第 3 列尺寸】：90px
    header->resizeSection(4, 70);   // 【第 4 列类型】：70px
    header->resizeSection(5, 75);   // 【第 5 列大小】：75px
    header->resizeSection(6, 110);  // 【第 6 列修改日期】：110px

    for (int i = 0; i <= 6; ++i) {
        header->setSectionResizeMode(i, QHeaderView::Interactive);
    }
=======
    auto* header = m_treeView->header();
    header->setFixedHeight(32);
    header->setStretchLastSection(false);

    for (int i = 0; i <= 6; ++i) {
        header->setSectionHidden(i, false);
    }
    header->setSectionHidden(7, true);

    header->setSectionResizeMode(0, QHeaderView::Stretch);

    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    header->setSectionResizeMode(3, QHeaderView::Fixed);
    header->setSectionResizeMode(4, QHeaderView::Fixed);
    header->setSectionResizeMode(5, QHeaderView::Fixed);
    header->setSectionResizeMode(6, QHeaderView::Fixed);

    header->resizeSection(1, 40);   // 状态
    header->resizeSection(2, 90);   // 评分
    header->resizeSection(3, 100);  // 尺寸
    header->resizeSection(4, 60);   // 类型
    header->resizeSection(5, 80);   // 大小
    header->resizeSection(6, 130);  // 修改日期
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
    } else if (m_viewStack->currentWidget() == m_treeView) {
        // 列表模式：动态计算安全图标尺寸（最低不小于 16px）
        int iconSize = qMax(16, m_zoomLevel - 8);
        m_treeView->setIconSize(QSize(iconSize, iconSize));

        // 动态设置列表项的物理行高为 m_zoomLevel (范围：30px ~ 230px)
=======
    } else if (m_viewStack->currentWidget() == m_treeView) {
        // 列表模式：动态计算安全图标尺寸（最低不小于 16px）
        int iconSize = qMax(16, m_zoomLevel - 8);
        m_treeView->setIconSize(QSize(iconSize, iconSize));

        if (auto* customHeader = qobject_cast<ContentHeaderView*>(m_treeView->header())) {
            customHeader->setZoomLevel(m_zoomLevel);
        }

        // 动态设置列表项的物理行高为 m_zoomLevel (范围：30px ~ 230px)
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Verification Steps
1. Verify `DropTreeView.h`, `DropTreeView.cpp`, and `ContentPanel.cpp` modified correctly using git merge diff/read_file.
2. Confirm section 0 is set to `Stretch` mode and sections 1-6 are `Fixed`.
3. Confirm `ContentHeaderView` calculates text alignment offset using `m_zoomLevel`.
