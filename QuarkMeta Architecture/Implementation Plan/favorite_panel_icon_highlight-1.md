# Implementation Plan - FavoritePanel Icon Selected Darkening Fix (favorite_panel_icon_highlight-1.md)

This implementation plan resolves the issue where `QTreeView` automatically applies Qt's default `QIcon::Selected` darkening filter mask to folder and file icons when a item is selected in `FavoritePanel`.

## Overview
By default, Qt's `QStyledItemDelegate::paint` paints icons using `QIcon::Selected` mode whenever `option.state & QStyle::State_Selected` is active. This causes bright yellow folder icons to appear dimmed/darkened when selected.

To fix this, we implement a lightweight custom item delegate `FavoriteItemDelegate` (inheriting `QStyledItemDelegate`) for `FavoritePanel`'s `DropTreeView`. In `paint`, icon drawing is intercepted and explicitly rendered in `QIcon::Normal` mode, ensuring selected icons retain 100% original color and brightness while maintaining selection background highlights.

---

## Modified Files List
1. `src/ui/FavoritePanel.h`
2. `src/ui/FavoritePanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/FavoritePanel.h`
Declare `FavoriteItemDelegate` class and add delegate pointer member to `FavoritePanel`.

```git
<<<<<<< SEARCH
#include <QFrame>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include "DropTreeView.h"

namespace QuarkMeta {

class FavoritePanel : public QFrame {
=======
#include <QFrame>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include "DropTreeView.h"

namespace QuarkMeta {

class FavoriteItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit FavoriteItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

class FavoritePanel : public QFrame {
>>>>>>> REPLACE
```

---

### 2. `src/ui/FavoritePanel.cpp`

#### 2.1 Implement `FavoriteItemDelegate::paint`
Force `QIcon::Normal` mode rendering when painting item icons:

```git
<<<<<<< SEARCH
#include "FavoritePanel.h"
#include "UiHelper.h"
=======
#include "FavoritePanel.h"
#include "UiHelper.h"
#include <QPainter>
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
namespace QuarkMeta {

FavoritePanel::FavoritePanel(QWidget* parent)
=======
namespace QuarkMeta {

void FavoriteItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Draw selection/hover background and text
    painter->save();

    // Custom background fill if selected or hovered
    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor("#37373D"));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }

    // Calculate layout geometries for icon and text
    int leftMargin = 10;
    int iconSize = 18;
    int spacing = 6;

    QRect iconRect(opt.rect.left() + leftMargin, opt.rect.top() + (opt.rect.height() - iconSize) / 2, iconSize, iconSize);
    QRect textRect(iconRect.right() + spacing, opt.rect.top(), opt.rect.width() - leftMargin - iconSize - spacing, opt.rect.height());

    // Paint Icon in QIcon::Normal mode (prevents Qt selected state darkening mask)
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) {
        icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
    }

    // Paint Text
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
    painter->setFont(opt.font);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

    painter->restore();
}

FavoritePanel::FavoritePanel(QWidget* parent)
>>>>>>> REPLACE
```

#### 2.2 Set Delegate on `m_favoriteView`
Set `FavoriteItemDelegate` on `m_favoriteView` in `initUi`:

```git
<<<<<<< SEARCH
    m_favoriteModel = new QStandardItemModel(this);
    m_favoriteView->setModel(m_favoriteModel);
=======
    m_favoriteModel = new QStandardItemModel(this);
    m_favoriteView->setModel(m_favoriteModel);
    m_favoriteView->setItemDelegate(new FavoriteItemDelegate(this));
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Verify `favorite_panel_icon_highlight-1.md` complies with AGENTS.md requirements.
2. Confirm exact Search/Replace Git Merge Diff syntax.
