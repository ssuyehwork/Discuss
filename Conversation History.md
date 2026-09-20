我发现列视图存在很大混乱、混淆问题，例如，快捷键、单击等问题，难道列视图单击选中就必须展开新列吗？

确认了，代码里就是这么写的，不是你的错觉——单击文件夹（不管是文件夹区还是文件区里点到的目录项）就会立刻展开新列，没有"仅选中、不展开"这个中间状态：

cpp
// ColumnViewPane：单击文件夹区/文件区里的目录项
emit folderSelected(itemPath, paneIdx);

// ColumnViewWidget 收到 folderSelected 后：
dismissSubColumns(paneIdx);       // 关掉右侧旧列
appendColumn(folderPath);          // 立刻展开新列

双击（doubleClicked）走的是另一条完全独立的路径（onDoubleClicked，大概率是"用默认程序打开"或者别的逻辑），跟单击展开新列这件事没有任何互斥或配合关系——两者是并行存在、互不知道对方的两套独立处理。

这确实是 macOS Finder 分栏视图的经典交互（单击文件夹＝在下一栏展示其内容），不是凭空捏造的错误，但你现在问出这个问题，说明这个交互跟你的预期不符,或者你觉得它跟其他快捷键/单击行为混在一起造成了困扰。

在我给出"该怎么改"之前，需要你先明确一件事——你期望的单击行为到底是什么？

单击文件夹＝只选中高亮，不展开新列，要展开必须双击（或者点文件夹右侧一个专门的展开箭头/图标）？
单击文件夹＝依然展开新列，但你觉得问题出在"展开这个动作本身跟别的什么逻辑（比如选中状态、快捷键响应）打架了"，展开本身不用改，要改的是它引发的连锁反应？

先确认是这两种里的哪一种（或者都不是，你有别的具体期望），我才能顺着代码继续往下查，不然我没法判断这次要改的是"交互设计本身"还是"交互设计没问题、但触发之后的某个环节写错了"。

就像其他三个视图那样，单击只是选中并显示高亮而已，只有双击文件夹时才展开新列
列视图无法通过滑选中项目

两个问题都找到了明确的代码依据，分别说：

问题1：单击展开新列——方案确认

按你的要求（单击只选中高亮，双击才展开新列），需要把"展开新列"这个触发时机从 clicked 挪到 doubleClicked：

文件：ColumnViewWidget.cpp

SEARCH:
    connect(m_folderListView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        int paneIdx = property("paneIndex").toInt();
        emit folderSelected(itemPath, paneIdx);
    });

    // 文件点击
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderSelected(itemPath, paneIdx);
        } else {
            emit fileSelected(itemPath, paneIdx);
        }
    });

    connect(m_folderListView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (m_contentPanel && index.isValid()) {
            m_contentPanel->onDoubleClicked(index);
        }
    });
    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (m_contentPanel && index.isValid()) {
            m_contentPanel->onDoubleClicked(index);
        }
    });

REPLACE:
    // 单击：文件区里点到文件才发 fileSelected（更新预览/统计），点到文件夹只走Qt原生选中高亮，不再触发展开
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
        int paneIdx = property("paneIndex").toInt();
        if (!isDir) {
            emit fileSelected(itemPath, paneIdx);
        }
    });

    // 双击：文件夹才触发展开新列，文件走原有的打开逻辑
    connect(m_folderListView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        int paneIdx = property("paneIndex").toInt();
        emit folderSelected(itemPath, paneIdx);
    });
    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderSelected(itemPath, paneIdx);
        } else if (m_contentPanel && index.isValid()) {
            m_contentPanel->onDoubleClicked(index);
        }
    });

（folderSelected/fileSelected 信号名字不用改，只是触发时机从"单击"挪到"双击"，ColumnViewWidget 那边接收信号后展开新列的逻辑完全不用动）

问题2：无法拖拽框选——找到根源，但需要你确认范围

文件夹区（m_folderListView）第179行：

cpp
m_folderListView->setSelectionMode(QAbstractItemView::SingleSelection);

这是直接原因——Qt 的拖拽框选（rubber-band多选）在 SingleSelection 模式下天生不生效，只有 ExtendedSelection/MultiSelection/ContiguousSelection 才支持框选多项。文件夹区被设成单选，从设计上就排除了框选的可能。

文件区（m_listView）第208行已经是 ExtendedSelection，理论上原生应该支持框选，代码层面我没看出明显阻断的地方（DropListView 也没有重写 mousePressEvent/mouseMoveEvent 去干扰原生框选逻辑）。

所以需要你确认：你说的"无法滑选"，是文件夹区、文件区、还是两个区域都不行？

如果只是文件夹区不行——直接把第179行改成 ExtendedSelection 就能解决，是个小改动。
如果文件区也不行——那说明文件区还存在另一个我目前从这份代码里看不出来的阻断点，需要进一步排查（可能跟 DropListView 的 startDrag()/setDragEnabled(true) 有关，需要实际测试现象比如"框选的框根本不出现"还是"框出现了但选不中东西"来区分）。
阶段一交付物：DualSectionPanel（共享核心组件）

先只写这一个新类本身，不动现有两个文件，方便你（或Jules）先单独编译验证这一个文件没问题，再进行下一步"接入"。

DualSectionPanel.h
cpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QLabel>
#include "models/FilterProxyModel.h"
#include "models/ItemModelBase.h"

namespace QuarkMeta {

class FolderSectionHeaderBar;
class FileSectionHeaderBar;

/**
 * @brief 文件夹区+文件区 双分区核心逻辑面板
 *
 * List / Grid / Column 三种视图模式共用的唯一实现：负责 header×2 + view×2 的排布、
 * 折叠联动、高度撑开（含"撑满宿主可视区剩余空间"）、选中聚合、可视区域缩略图按需刷新、
 * 筛选后全隐藏提示。
 *
 * 本类不创建 folderView/fileView（由外部按各自需要的具体类型创建后传入），
 * 也不拥有滚动能力（自己是普通QWidget）——滚动由宿主（一个QScrollArea）负责，
 * 本类只需要宿主在必要时告知"当前可视视口高度是多少"。
 */
class DualSectionPanel : public QWidget {
    Q_OBJECT
public:
    explicit DualSectionPanel(QAbstractItemView* folderView, QAbstractItemView* fileView,
                               FilterProxyModel* folderProxy, FilterProxyModel* fileProxy,
                               QWidget* parent = nullptr);
    ~DualSectionPanel() override = default;

    FolderSectionHeaderBar* folderHeader() const { return m_folderHeader; }
    FileSectionHeaderBar* fileHeader() const { return m_fileHeader; }
    QAbstractItemView* folderView() const { return m_folderView; }
    QAbstractItemView* fileView() const { return m_fileView; }

    // hostViewportHeight：宿主QScrollArea当前viewport()->height()，用于撑满剩余空间的计算
    void updateSectionCounts(int hostViewportHeight);
    void toggleFolderSectionCollapse();

    QAbstractItemView* activeItemView() const;
    QModelIndexList getSelectedIndexes() const;

    // hostViewport：宿主QScrollArea的viewport()，用于把物理可视矩形投影到子视图坐标系
    void refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport);

signals:
    void selectionChanged();

private:
    int computeFileViewMinHeight(int hostViewportHeight) const;
    void updateEmptyFilterHint();

    QVBoxLayout* m_layout = nullptr;
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    FileSectionHeaderBar* m_fileHeader = nullptr;
    QAbstractItemView* m_folderView = nullptr;
    QAbstractItemView* m_fileView = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
    QLabel* m_emptyFilterHintLabel = nullptr;
    int m_lastHostViewportHeight = 0;
};

} // namespace QuarkMeta
DualSectionPanel.cpp
cpp
#include "DualSectionPanel.h"
#include "FolderSectionWidget.h"
#include "Logger.h"
#include "../core/CoreController.h"
#include <QElapsedTimer>

namespace QuarkMeta {

DualSectionPanel::DualSectionPanel(QAbstractItemView* folderView, QAbstractItemView* fileView,
                                    FilterProxyModel* folderProxy, FilterProxyModel* fileProxy,
                                    QWidget* parent)
    : QWidget(parent), m_folderView(folderView), m_fileView(fileView),
      m_folderProxyModel(folderProxy), m_fileProxyModel(fileProxy) {

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_folderHeader = new FolderSectionHeaderBar(this);
    m_folderHeader->hide();
    m_layout->addWidget(m_folderHeader, 0);

    if (m_folderView) {
        m_folderView->setParent(this);
        m_folderView->hide();
        m_layout->addWidget(m_folderView, 0);
    }

    m_fileHeader = new FileSectionHeaderBar(this);
    m_fileHeader->hide();
    m_layout->addWidget(m_fileHeader, 0);

    if (m_fileView) {
        m_fileView->setParent(this);
        m_layout->addWidget(m_fileView, 0);
    }

    // 🚀 筛选后全隐藏提示（原 ColumnViewPane 独有，现统一给三种视图）
    m_emptyFilterHintLabel = new QLabel(this);
    m_emptyFilterHintLabel->setAlignment(Qt::AlignCenter);
    m_emptyFilterHintLabel->setWordWrap(true);
    m_emptyFilterHintLabel->setStyleSheet("color: #888888; font-size: 12px; padding: 16px;");
    m_emptyFilterHintLabel->hide();
    m_layout->addWidget(m_emptyFilterHintLabel, 0);

    connect(m_folderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderView && m_folderHeader->count() > 0) {
            m_folderView->setVisible(!collapsed);
            updateSectionCounts(m_lastHostViewportHeight);
        }
    });

    if (m_folderView && m_folderView->selectionModel()) {
        connect(m_folderView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &DualSectionPanel::selectionChanged);
    }
    if (m_fileView && m_fileView->selectionModel()) {
        connect(m_fileView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &DualSectionPanel::selectionChanged);
    }
}

int DualSectionPanel::computeFileViewMinHeight(int hostViewportHeight) const {
    int used = 0;
    if (m_folderHeader && m_folderHeader->isVisible()) used += m_folderHeader->height();
    if (m_folderView && m_folderView->isVisible()) used += m_folderView->height();
    if (m_fileHeader && m_fileHeader->isVisible()) used += m_fileHeader->height();
    return qMax(0, hostViewportHeight - used);
}

void DualSectionPanel::updateEmptyFilterHint() {
    if (!m_emptyFilterHintLabel || !m_folderProxyModel || !m_fileProxyModel) return;
    bool folderEmpty = m_folderProxyModel->rowCount() == 0;
    bool fileEmpty = m_fileProxyModel->rowCount() == 0;
    // 具体"隐藏了多少项"的判断逻辑跟原 ColumnViewPane 实现保持一致，
    // 这里先给出统一入口，实际隐藏计数逻辑在接入阶段核对原实现后补全。
    if (folderEmpty && fileEmpty) {
        m_emptyFilterHintLabel->setText("所有内容已被筛选隐藏");
        m_emptyFilterHintLabel->show();
    } else {
        m_emptyFilterHintLabel->hide();
    }
}

void DualSectionPanel::updateSectionCounts(int hostViewportHeight) {
    m_lastHostViewportHeight = hostViewportHeight;
    if (!m_folderProxyModel || !m_fileProxyModel) return;

    int folderCount = m_folderProxyModel->rowCount();
    int fileCount = m_fileProxyModel->rowCount();

    if (m_folderHeader) {
        m_folderHeader->setCount(folderCount);
        m_folderHeader->setVisible(folderCount > 0);
    }
    if (m_folderView) {
        if (folderCount == 0) {
            m_folderView->hide();
        } else if (!m_folderHeader->isCollapsed()) {
            m_folderView->show();
        }
    }

    if (m_fileHeader) {
        m_fileHeader->setCount(fileCount);
        m_fileHeader->setVisible(fileCount > 0 && folderCount > 0);
    }
    if (m_fileView) {
        if (fileCount == 0) {
            m_fileView->hide();
        } else {
            m_fileView->show();
        }
    }

    updateEmptyFilterHint();
    // 注意：具体的"文件区固定高度撑开"数值计算（Grid用JustifiedView::totalHeight()，
    // List/Column用行数×行高），因为跟具体view类型（JustifiedView vs QTreeView vs QListView）
    // 强相关，保留在 SectionedScrollCanvas / ColumnViewPane 各自的
    // totalHeightChanged信号连接、以及本函数调用之后各自追加一行 setFixedHeight(qMax(计算值, computeFileViewMinHeight(...)))，
    // 不在本类里做，避免这个共享类反而要认识三种不同的view子类。
}

void DualSectionPanel::toggleFolderSectionCollapse() {
    if (m_folderHeader && m_folderHeader->isVisible() && m_folderHeader->count() > 0) {
        m_folderHeader->setCollapsed(!m_folderHeader->isCollapsed());
    }
}

QAbstractItemView* DualSectionPanel::activeItemView() const {
    if (m_folderView && (m_folderView->hasFocus() ||
        (m_folderView->selectionModel() && m_folderView->selectionModel()->hasSelection()))) {
        return m_folderView;
    }
    return m_fileView;
}

QModelIndexList DualSectionPanel::getSelectedIndexes() const {
    QModelIndexList res;
    for (auto* view : {m_folderView, m_fileView}) {
        if (view && view->selectionModel() && view->selectionModel()->hasSelection()) {
            for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                if (idx.column() == 0) res.append(idx);
            }
        }
    }
    return res;
}

void DualSectionPanel::refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport) {
    if (!model || !hostViewport || CoreController::isShuttingDown()) return;

    QRect vpRect = hostViewport->rect();
    QSet<int> visibleRows;

    auto scanView = [&](QAbstractItemView* view, FilterProxyModel* proxy) {
        if (!view || !view->isVisible() || !proxy || proxy->rowCount() == 0) return;

        QPoint topPoint = view->mapFromGlobal(hostViewport->mapToGlobal(vpRect.topLeft()));
        QPoint btmPoint = view->mapFromGlobal(hostViewport->mapToGlobal(vpRect.bottomRight()));

        if (topPoint.y() >= view->height() || btmPoint.y() <= 0) return;

        int clampedTopY = qBound(0, topPoint.y(), view->height());
        int clampedBtmY = qBound(0, btmPoint.y(), view->height());

        QModelIndex topIdx = view->indexAt(QPoint(10, clampedTopY));
        if (!topIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !topIdx.isValid(); offset += 10)
                topIdx = view->indexAt(QPoint(10, clampedTopY + offset));
        }
        QModelIndex btmIdx = view->indexAt(QPoint(10, clampedBtmY));
        if (!btmIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !btmIdx.isValid(); offset += 10)
                btmIdx = view->indexAt(QPoint(10, clampedBtmY - offset));
        }

        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 4) : qMin(proxy->rowCount() - 1, top + 20);

        for (int r = top; r <= bottom; ++r) {
            QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
    };

    scanView(m_folderView, m_folderProxyModel);
    scanView(m_fileView, m_fileProxyModel);

    if (!visibleRows.isEmpty()) {
        model->loadThumbnailsForRows(visibleRows.values());
    }
}

} // namespace QuarkMeta
说明与下一步
updateEmptyFilterHint() 里的具体判断条件我写了一版合理的占位（两边都空才提示），但没有照抄 ColumnViewPane 原来那份判断逻辑（因为我需要先看它具体怎么判断"隐藏了多少项"，上次只看到setText那一行，没看到完整条件）——接入阶段核对原实现后会修正这里，不是最终版本，先让你知道这一点。
文件区"该撑多高"的具体计算公式（Grid用JustifiedView::totalHeight()，List/Column按行数乘行高）故意没有收进这个共享类，因为这部分跟具体是哪种view强绑定，硬塞进来反而会让这个共享类认识三种不同的子类，违背"单一职责"。这部分逻辑还是分别留在两个宿主类里，只是宿主类调用完 updateSectionCounts() 之后，各自补一行 setFixedHeight(qMax(自己算的值, ...))——但注意，这个"剩余空间"数值现在要从 computeFileViewMinHeight 移出来来的，宿主类需要一个新的公开方法才能拿到这个数字。

这一版你（或Jules）可以先单独编译这一个新文件（暂时不接入任何调用方，只保证它自己能编译通过、没有语法错误），确认没问题后，下一步我再给"如何把 SectionedScrollCanvas 改造成内部持有一个 DualSectionPanel"的具体接入方案。要现在就继续，还是先编译验证这一步？

继续下一步

阶段二交付物：把 SectionedScrollCanvas 改造成"内部持有一个 DualSectionPanel"
先给 DualSectionPanel 补一个小口子
文件：DualSectionPanel.h

SEARCH:
    void updateSectionCounts(int hostViewportHeight);
    void toggleFolderSectionCollapse();

REPLACE:
    void updateSectionCounts(int hostViewportHeight);
    void toggleFolderSectionCollapse();
    int fileViewMinHeight() const { return computeFileViewMinHeight(m_lastHostViewportHeight); }

（宿主需要拿这个值去跟自己算的"内容真实高度"做 qMax，之前故意没让共享类自己管这部分，所以要开一个公开读取口）

SectionedScrollCanvas.h（完整替换）
cpp
#pragma once

#include <QScrollArea>
#include <QAbstractItemView>
#include <QSet>
#include "models/FilterProxyModel.h"
#include "models/ItemModelBase.h"

namespace QuarkMeta {

class FolderSectionHeaderBar;
class FileSectionHeaderBar;
class DualSectionPanel;

/**
 * @brief 双分区协同撑开滚动画布：List/Grid 专属外壳，内部持有唯一的 DualSectionPanel 核心逻辑
 */
class SectionedScrollCanvas : public QScrollArea {
    Q_OBJECT

public:
    enum class CanvasType {
        Grid,
        List
    };

    explicit SectionedScrollCanvas(CanvasType type, FilterProxyModel* folderProxy, FilterProxyModel* fileProxy, QObject* eventFilter = nullptr, QWidget* parent = nullptr);
    ~SectionedScrollCanvas() override = default;

    CanvasType canvasType() const { return m_type; }
    QAbstractItemView* folderView() const;
    QAbstractItemView* fileView() const;
    FolderSectionHeaderBar* folderHeader() const;
    FileSectionHeaderBar* fileHeader() const;
    FilterProxyModel* folderProxyModel() const { return m_folderProxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_fileProxyModel; }

    void updateSectionCounts();
    void updateZoom(int zoomLevel);
    void toggleFolderSectionCollapse();

    QAbstractItemView* activeItemView() const;
    QModelIndexList getSelectedIndexes() const;
    void refreshVisibleThumbnails(ItemModelBase* model);

signals:
    void selectionChanged();
    void doubleClicked(const QModelIndex& index);
    void customContextMenuRequested(const QPoint& pos);
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex, QAbstractItemModel* sourceProxy);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QAbstractItemView* createFolderView(QObject* eventFilter);
    QAbstractItemView* createFileView(QObject* eventFilter);
    void setupConnections();

    CanvasType m_type;
    DualSectionPanel* m_panel = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
};

} // namespace QuarkMeta
SectionedScrollCanvas.cpp（完整替换）
cpp
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SectionedScrollCanvas.h"
#include "DualSectionPanel.h"
#include "FolderSectionWidget.h"
#include "ContentHeaderWidget.h"
#include "DropJustifiedView.h"
#include "DropTreeView.h"
#include "ThumbnailDelegate.h"
#include "TreeItemDelegate.h"
#include "JustifiedView.h"
#include "models/ItemModelBase.h"
#include "../core/NavigationService.h"
#include <QHeaderView>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

namespace QuarkMeta {

SectionedScrollCanvas::SectionedScrollCanvas(CanvasType type, FilterProxyModel* folderProxy, FilterProxyModel* fileProxy, QObject* eventFilter, QWidget* parent)
    : QScrollArea(parent), m_type(type), m_folderProxyModel(folderProxy), m_fileProxyModel(fileProxy) {
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);

    QAbstractItemView* folderView = createFolderView(eventFilter);
    QAbstractItemView* fileView = createFileView(eventFilter);

    m_panel = new DualSectionPanel(folderView, fileView, m_folderProxyModel, m_fileProxyModel, this);
    m_panel->setContextMenuPolicy(Qt::CustomContextMenu);
    m_panel->setFocusPolicy(Qt::StrongFocus);
    m_panel->setAcceptDrops(true);
    setWidget(m_panel);

    setupConnections();
}

QAbstractItemView* SectionedScrollCanvas::createFolderView(QObject* eventFilter) {
    QAbstractItemView* view = nullptr;
    if (m_type == CanvasType::Grid) {
        auto* folderJv = new DropJustifiedView();
        folderJv->setFrameShape(QFrame::NoFrame);
        folderJv->setSelectionMode(QAbstractItemView::SingleSelection);
        folderJv->setContextMenuPolicy(Qt::CustomContextMenu);
        folderJv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        folderJv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        folderJv->setModel(m_folderProxyModel);
        folderJv->setAspectRatioRole(AspectRatioRole);
        auto* fDelegate = new ThumbnailDelegate(this);
        fDelegate->setHasThumbnailRole(HasThumbnailRole);
        fDelegate->setRatingRole(RatingRole);
        fDelegate->setPathRole(PathRole);
        fDelegate->setPinnedRole(PinnedRole);
        fDelegate->setTypeRole(TypeRole);
        fDelegate->setIsEmptyRole(IsEmptyRole);
        fDelegate->setColorRole(ColorRole);
        folderJv->setItemDelegate(fDelegate);
        view = folderJv;
    } else {
        auto* folderTv = new DropTreeView();
        folderTv->setFrameShape(QFrame::NoFrame);
        folderTv->setAlternatingRowColors(true);
        folderTv->setSortingEnabled(true);
        folderTv->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        folderTv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        folderTv->setContextMenuPolicy(Qt::CustomContextMenu);
        folderTv->setSelectionMode(QAbstractItemView::SingleSelection);
        folderTv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        folderTv->setRootIsDecorated(false);
        folderTv->setItemDelegate(new TreeItemDelegate(this, true, true));
        folderTv->setModel(m_folderProxyModel);
        folderTv->header()->setFixedHeight(32);
        folderTv->header()->setMinimumSectionSize(0);
        folderTv->applyColumnPolicies();
        view = folderTv;
    }
    if (eventFilter && view) {
        view->installEventFilter(eventFilter);
        if (view->viewport()) view->viewport()->installEventFilter(eventFilter);
    }
    return view;
}

QAbstractItemView* SectionedScrollCanvas::createFileView(QObject* eventFilter) {
    QAbstractItemView* view = nullptr;
    if (m_type == CanvasType::Grid) {
        auto* fileJv = new DropJustifiedView();
        fileJv->setFrameShape(QFrame::NoFrame);
        fileJv->setSelectionMode(QAbstractItemView::ExtendedSelection);
        fileJv->setContextMenuPolicy(Qt::CustomContextMenu);
        fileJv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        fileJv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        fileJv->setModel(m_fileProxyModel);
        fileJv->setAspectRatioRole(AspectRatioRole);
        auto* delegate = new ThumbnailDelegate(this);
        delegate->setHasThumbnailRole(HasThumbnailRole);
        delegate->setRatingRole(RatingRole);
        delegate->setPathRole(PathRole);
        delegate->setPinnedRole(PinnedRole);
        delegate->setTypeRole(TypeRole);
        delegate->setIsEmptyRole(IsEmptyRole);
        delegate->setColorRole(ColorRole);
        fileJv->setItemDelegate(delegate);
        view = fileJv;
    } else {
        auto* fileTv = new DropTreeView();
        fileTv->setFrameShape(QFrame::NoFrame);
        fileTv->setAlternatingRowColors(true);
        fileTv->setSortingEnabled(true);
        fileTv->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        fileTv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        fileTv->setContextMenuPolicy(Qt::CustomContextMenu);
        fileTv->setSelectionMode(QAbstractItemView::ExtendedSelection);
        fileTv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        fileTv->setRootIsDecorated(false);
        fileTv->setItemDelegate(new TreeItemDelegate(this, true, true));
        fileTv->setModel(m_fileProxyModel);
        fileTv->header()->setFixedHeight(32);
        fileTv->header()->setMinimumSectionSize(0);
        fileTv->applyColumnPolicies();
        view = fileTv;
    }
    if (eventFilter && view) {
        view->installEventFilter(eventFilter);
        if (view->viewport()) view->viewport()->installEventFilter(eventFilter);
    }
    return view;
}

QAbstractItemView* SectionedScrollCanvas::folderView() const { return m_panel->folderView(); }
QAbstractItemView* SectionedScrollCanvas::fileView() const { return m_panel->fileView(); }
FolderSectionHeaderBar* SectionedScrollCanvas::folderHeader() const { return m_panel->folderHeader(); }
FileSectionHeaderBar* SectionedScrollCanvas::fileHeader() const { return m_panel->fileHeader(); }

void SectionedScrollCanvas::setupConnections() {
    connect(m_panel, &DualSectionPanel::selectionChanged, this, &SectionedScrollCanvas::selectionChanged);

    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();

    if (m_type == CanvasType::Grid) {
        if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
            connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_folderProxyModel && m_folderProxyModel->rowCount() > 0) {
                    m_panel->folderView()->setFixedHeight(height);
                }
            });
        }
        if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
            connect(jv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_fileProxyModel && m_fileProxyModel->rowCount() > 0) {
                    m_panel->fileView()->setFixedHeight(qMax(height, m_panel->fileViewMinHeight()));
                }
            });
        }
    }

    auto onModelChanged = [this]() { updateSectionCounts(); };
    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);

    connect(folderView, &QAbstractItemView::doubleClicked, this, &SectionedScrollCanvas::doubleClicked);
    connect(fileView, &QAbstractItemView::doubleClicked, this, &SectionedScrollCanvas::doubleClicked);

    connect(this, &QScrollArea::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    connect(m_panel, &QWidget::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    connect(folderView, &QAbstractItemView::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    connect(fileView, &QAbstractItemView::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);

    if (m_type == CanvasType::Grid) {
        if (auto* dropFolder = qobject_cast<DropJustifiedView*>(folderView)) {
            connect(dropFolder, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_folderProxyModel);
            });
        }
        if (auto* dropFile = qobject_cast<DropJustifiedView*>(fileView)) {
            connect(dropFile, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_fileProxyModel);
            });
        }
    } else {
        if (auto* dropFolder = qobject_cast<DropTreeView*>(folderView)) {
            connect(dropFolder, &DropTreeView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_folderProxyModel);
            });
        }
        if (auto* dropFile = qobject_cast<DropTreeView*>(fileView)) {
            connect(dropFile, &DropTreeView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_fileProxyModel);
            });
        }
    }
}

void SectionedScrollCanvas::updateSectionCounts() {
    m_panel->updateSectionCounts(viewport()->height());
    if (!m_folderProxyModel || !m_fileProxyModel) return;

    int folderCount = m_folderProxyModel->rowCount();
    int fileCount = m_fileProxyModel->rowCount();
    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();

    if (folderView && folderCount > 0 && folderView->isVisible()) {
        if (m_type == CanvasType::Grid) {
            if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
                folderView->setFixedHeight(fjv->totalHeight());
            }
        } else {
            auto* tv = static_cast<QTreeView*>(folderView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            folderView->setFixedHeight(folderCount * rowH + hdrH + 2);
            folderView->updateGeometry();
        }
    }

    if (fileView && fileCount > 0) {
        if (m_type == CanvasType::Grid) {
            if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
                fileView->setFixedHeight(qMax(jv->totalHeight(), m_panel->fileViewMinHeight()));
            }
        } else {
            auto* tv = static_cast<QTreeView*>(fileView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            fileView->setFixedHeight(qMax(fileCount * rowH + hdrH + 2, m_panel->fileViewMinHeight()));
            fileView->updateGeometry();
        }
    }
}

void SectionedScrollCanvas::updateZoom(int zoomLevel) {
    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();
    if (m_type == CanvasType::Grid) {
        if (auto* jv = qobject_cast<JustifiedView*>(fileView)) jv->setTargetRowHeight(zoomLevel);
        if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) fjv->setTargetRowHeight(zoomLevel);
    } else {
        QSize iconSize(qMax(16, zoomLevel - 8), qMax(16, zoomLevel - 8));
        if (auto* folderTree = qobject_cast<DropTreeView*>(folderView)) {
            folderTree->setIconSize(iconSize);
            folderTree->doItemsLayout();
        }
        if (auto* fileTree = qobject_cast<DropTreeView*>(fileView)) {
            if (auto* hdr = qobject_cast<ContentHeaderView*>(fileTree->header())) {
                hdr->setZoomLevel(zoomLevel);
            }
            fileTree->setIconSize(iconSize);
            fileTree->doItemsLayout();
        }
        updateSectionCounts();
    }
}

void SectionedScrollCanvas::toggleFolderSectionCollapse() {
    m_panel->toggleFolderSectionCollapse();
}

QAbstractItemView* SectionedScrollCanvas::activeItemView() const {
    return m_panel->activeItemView();
}

QModelIndexList SectionedScrollCanvas::getSelectedIndexes() const {
    return m_panel->getSelectedIndexes();
}

void SectionedScrollCanvas::refreshVisibleThumbnails(ItemModelBase* model) {
    m_panel->refreshVisibleThumbnails(model, viewport());
}

void SectionedScrollCanvas::resizeEvent(QResizeEvent* event) {
    QScrollArea::resizeEvent(event);
    updateSectionCounts();
}

void SectionedScrollCanvas::mousePressEvent(QMouseEvent* event) {
    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();
    if (folderView && folderView->selectionModel()) folderView->selectionModel()->clearSelection();
    if (fileView && fileView->selectionModel()) fileView->selectionModel()->clearSelection();
    QScrollArea::mousePressEvent(event);
}

void SectionedScrollCanvas::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        NavigationService::instance().goUp();
        event->accept();
        return;
    }
    QScrollArea::mouseDoubleClickEvent(event);
}

void SectionedScrollCanvas::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QScrollArea::dragEnterEvent(event);
    }
}

void SectionedScrollCanvas::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QScrollArea::dragMoveEvent(event);
    }
}

void SectionedScrollCanvas::dropEvent(QDropEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        QStringList paths;
        for (const QUrl& url : event->mimeData()->urls()) {
            QString localPath = url.toLocalFile();
            if (!localPath.isEmpty()) paths << localPath;
        }
        if (!paths.isEmpty()) {
            emit pathsDropped(paths, QModelIndex(), m_fileProxyModel);
            event->acceptProposedAction();
            return;
        }
    }
    QScrollArea::dropEvent(event);
}

} // namespace QuarkMeta
重要说明
对 ContentPanel.cpp 完全零影响——SectionedScrollCanvas 的公开接口（folderView()、fileView()、updateSectionCounts()、getSelectedIndexes() 等等）一个都没变签名，外部调用方不需要动一行代码。
m_folderHeader/m_fileHeader 之前是私有成员，现在通过 m_panel->folderHeader()/fileHeader() 转发，行为完全一致。
m_emptyFilterHintLabel 和统一的"筛选全隐藏"提示，List/Grid 现在自动获得了（因为 DualSectionPanel 自带）——这是合并顺带修复的那个缺失项，不需要额外改动。

这一版编译验证通过之后，下一步是 ColumnViewPane 那边的接入（改法类似，但要处理它额外的路径导航、固定宽度、以及"按需加载缩略图"这个之前缺失能力的补齐）。要继续吗？