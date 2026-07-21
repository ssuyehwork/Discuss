#include "GridResultView.h"
#include "DropJustifiedView.h"
#include "ThumbnailDelegate.h"
#include "../core/ModelContract.h"
#include <QPalette>
#include <QListView>

namespace ArcMeta {

GridResultView::GridResultView(QWidget* parent)
    : IScanResultView(parent) {
    m_view = new DropJustifiedView(parent);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);

    if (auto* lv = qobject_cast<QListView*>(m_view)) lv->setSelectionRectVisible(true);

    QPalette p = m_view->palette();
    p.setColor(QPalette::Highlight, QColor(55, 138, 221, 80));
    p.setColor(QPalette::HighlightedText, Qt::white);
    m_view->setPalette(p);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);

    m_view->setDragEnabled(true);
    m_view->setAcceptDrops(true);
    m_view->setDragDropMode(QAbstractItemView::DragDrop);

    m_view->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    m_view->setAspectRatioRole(AspectRatioRole);
    auto* delegate = new ThumbnailDelegate(this);
    delegate->setHasThumbnailRole(HasThumbnailRole);
    delegate->setRatingRole(RatingRole);
    delegate->setPathRole(PathRole);
    delegate->setPinnedRole(PinnedRole);
    delegate->setManagedRole(ManagedRole);
    delegate->setTypeRole(TypeRole);
    delegate->setIsEmptyRole(IsEmptyRole);
    delegate->setColorRole(ColorRole);
    delegate->setRegistrationProgressRole(RegistrationProgressRole);
    m_view->setItemDelegate(delegate);

    m_view->setStyleSheet(
        "QAbstractItemView { background-color: transparent; border: none; outline: none; }"
        "QAbstractItemView::item { background: transparent; }"
        "QAbstractItemView::item:selected { background-color: transparent; }"
        "QAbstractItemView::item:hover { background-color: transparent; }"
    );
}

GridResultView::~GridResultView() {
}

QWidget* GridResultView::getWidget() {
    return m_view;
}

QAbstractItemView* GridResultView::getBaseView() {
    return m_view;
}

void GridResultView::setModel(QAbstractItemModel* model) {
    m_view->setModel(model);
}

void GridResultView::setIconSize(int size) {
    m_view->setTargetRowHeight(size);
}

void GridResultView::refreshLayout() {
    m_view->setLayoutMode(JustifiedView::GridMode);
    m_view->doItemsLayout();
}

} // namespace ArcMeta
