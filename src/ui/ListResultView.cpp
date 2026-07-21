#include "ListResultView.h"
#include "DropTreeView.h"
#include "TreeItemDelegate.h"
#include "../core/AppConfig.h"
#include <QHeaderView>

namespace ArcMeta {

ListResultView::ListResultView(QWidget* parent)
    : IScanResultView(parent) {
    m_treeView = new DropTreeView(parent);
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setSortingEnabled(true);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QPalette tp = m_treeView->palette();
    tp.setColor(QPalette::Highlight, QColor(55, 138, 221, 80));
    tp.setColor(QPalette::HighlightedText, Qt::white);
    m_treeView->setPalette(tp);

    m_treeView->setDragEnabled(true);
    m_treeView->setAcceptDrops(true);
    m_treeView->setDragDropMode(QAbstractItemView::DragDrop);

    m_treeView->setExpandsOnDoubleClick(false);
    m_treeView->setRootIsDecorated(false);

    m_treeView->setItemDelegate(new TreeItemDelegate(this));

    m_treeView->setStyleSheet(
        "QTreeView { background-color: transparent; border: none; outline: none; font-size: 12px; }"
        "QTreeView::item { height: 28px; color: #EEEEEE; padding-left: 0px; }"
        "QTreeView::item:selected { background-color: rgba(52, 152, 219, 0.2); border-left: 2px solid #3498db; }"
        "QTreeView::item:hover { background-color: #2A2A2A; }"
        "QTreeView QLineEdit { background-color: #2D2D2D; color: #FFFFFF; border: 1px solid #378ADD; border-radius: 6px; padding: 2px; selection-background-color: #378ADD; selection-color: #FFFFFF; }"
    );

    m_treeView->header()->setDefaultAlignment(Qt::AlignCenter);
    m_treeView->header()->setStyleSheet(
        "QHeaderView::section { background-color: #252525; color: #B0B0B0; border: none; border-right: 1px solid #333333; height: 32px; font-size: 11px; }"
    );

    auto* header = m_treeView->header();
    header->setStretchLastSection(false);
    header->setCascadingSectionResizes(false);
    header->setMinimumSectionSize(30);
}

ListResultView::~ListResultView() {
}

QWidget* ListResultView::getWidget() {
    return m_treeView;
}

QAbstractItemView* ListResultView::getBaseView() {
    return m_treeView;
}

void ListResultView::setModel(QAbstractItemModel* model) {
    m_treeView->setModel(model);

    auto* header = m_treeView->header();
    header->setStretchLastSection(false);
    header->setCascadingSectionResizes(false);
    header->setMinimumSectionSize(30);

    QByteArray headerState = AppConfig::instance().getValue("UI/ListHeaderState").toByteArray();
    if (!headerState.isEmpty()) {
        header->restoreState(headerState);
    }

    for(int i = 0; i <= 7; ++i) header->setSectionHidden(i, false);

    header->resizeSection(0, 400); // 名称
    header->resizeSection(1, 40);  // 状态
    header->resizeSection(2, 60);  // 星级
    header->resizeSection(3, 60);  // 颜色标记
    header->resizeSection(4, 100); // 标签
    header->resizeSection(5, 80);  // 类型
    header->resizeSection(6, 80);  // 大小
    header->resizeSection(7, 150); // 修改日期

    for(int i = 1; i <= 7; ++i) {
        header->setSectionResizeMode(i, QHeaderView::Interactive);
    }
    header->setSectionResizeMode(0, QHeaderView::Stretch);

    disconnect(header, &QHeaderView::sectionResized, nullptr, nullptr);
    connect(header, &QHeaderView::sectionResized, this, [this, header](int index, int oldSize, int newSize) {
        Q_UNUSED(oldSize);
        static bool guard = false;
        if (guard || index == 0) return;

        guard = true;

        if (index == 7 && newSize < 150) {
            header->resizeSection(7, 150);
            guard = false;
            return;
        }

        int currentTotal = header->length();
        int maxAvailable = m_treeView->viewport()->width();

        if (currentTotal > maxAvailable && maxAvailable > 100) {
             int allowed = newSize - (currentTotal - maxAvailable);
             int minAllowed = header->minimumSectionSize();
             if (index == 7) minAllowed = 150;

             header->resizeSection(index, qMax(minAllowed, allowed));
        }

        AppConfig::instance().setValue("UI/ListHeaderState", header->saveState());

        guard = false;
    });
}

void ListResultView::setIconSize(int size) {
    m_treeView->setIconSize(QSize(size - 8, size - 8));

    static int lastTreeHeight = -1;
    if (lastTreeHeight != size) {
        m_treeView->setStyleSheet(
            QString("QTreeView { background-color: transparent; border: none; outline: none; font-size: 12px; }"
                    "QTreeView::item { height: %1px; color: #EEEEEE; padding-left: 0px; }"
                    "QTreeView::item:selected { background-color: rgba(52, 152, 219, 0.2); border-left: 2px solid #3498db; }"
                    "QTreeView::item:hover { background-color: #2A2A2A; }"
                    "QTreeView QLineEdit { background-color: #2D2D2D; color: #FFFFFF; border: 1px solid #378ADD; border-radius: 6px; padding: 2px; selection-background-color: #378ADD; selection-color: #FFFFFF; }")
            .arg(size)
        );
        lastTreeHeight = size;
    }
}

void ListResultView::refreshLayout() {
}

} // namespace ArcMeta
