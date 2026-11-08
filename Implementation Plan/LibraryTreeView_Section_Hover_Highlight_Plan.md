# Implementation Plan: 库面板分组标题行折叠组件与加号按钮悬停高亮即时刷新修复方案

本实施方案严格遵循工业级 C++/Qt 顶层架构规范、《AGENTS.md》实施界限（只给出方案，绝对禁止直接修改源代码）及《Standard Specifications for Implementation Plan.md》，针对库面板（`LibraryPanel`）中三大分组标头行（“快速访问(N)”、“智能文件夹(N)”、“文件夹(N)”）折叠组件块与右侧“+”按钮鼠标悬停无法即时高亮、必须点击才刷新的缺陷，提供**悬停状态单一真理源（SSOT）治理与局部定向刷新**实施方案。

---

## 前置物理证据清单 (Gate 1: Pre-Draft Evidence)

### 1. 物理读取与分析文件清单
- `QuarkMeta_Source/src/ui/LibraryPanel.h` (声明 `LibraryTreeView` 与 `LibraryPanel`)
- `QuarkMeta_Source/src/ui/LibraryPanel.cpp` (实现 `LibraryTreeView` 与 `LibraryPanel`)
- `QuarkMeta_Source/src/ui/LibraryItemDelegate.h` (声明 `LibraryItemDelegate`)
- `QuarkMeta_Source/src/ui/LibraryItemDelegate.cpp` (实现 `LibraryItemDelegate::paint` 与 `editorEvent`)
- `QuarkMeta_Source/src/ui/LibraryTreeLayout.h` (布局尺寸单一真理源)
- `QuarkMeta_Source/src/ui/DropTreeView.h` / `DropTreeView.cpp` (确认未重写 `mouseMoveEvent`、`viewportEvent`、`scrollContentsBy`)

### 2. 外部使用位置确认
- `QuarkMeta_Source/src/ui/QuarkApplication.cpp:108`:
  ```cpp
  if (candidateTree->objectName() == "LibraryTreeView") {
      ...
  }
  ```
- 确认 `LibraryTreeView` 仅在 `LibraryPanel` 中构建，未被库面板以外的任何业务面板使用。

### 3. 重复计算现状核实
- 分组标题文字：`sectionTitleByNodeId(nodeId, countVal)`
- 分组标题字体：`QFont` 设置点大小 10 且加粗
- 区域矩形判定：在 `paint` 与 `editorEvent` 中分别独立获取 `hdrRect` 与 `addRect`

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 实际现象与问题根因剖析
- **实际现象**：鼠标移入分组标题行的折叠组件块（文字+折叠箭头）或右侧“+”按钮时，不显示悬停背景高亮，必须发生鼠标点击后才显示；
- **问题根因**：`LibraryItemDelegate::paint` 在绘制时使用 `QCursor::pos()` 现场推算鼠标位置。Qt 的树视图只在“悬停的行”发生变化时才重绘该行；鼠标在同一行内从空白处移动到折叠组件或“+”按钮上时，行索引没有变，不会触发行重绘，`paint` 不被调用，因而高亮无法及时画出。根本原因在于**悬停状态没有唯一真理源，由绘制代码现场推算**。

### 1.2 目标架构与设计原则
1. **单一真理源 (SSOT)**：在 `LibraryTreeView` 中统一持有当前悬停的行（`QPersistentModelIndex m_hoveredIndex`）和区域（`SectionHitArea m_hoveredArea: None, Row, Header, AddButton`），作为全软件该树悬停状态的唯一真理源；
2. **纯瘦提取**：在 `LibraryItemDelegate` 中提取静态函数 `sectionTitleByNodeId`、`sectionHeaderFont`、`hitTestSectionRow`，供 `editorEvent`、`paint` 以及树事件统一调用，杜绝任何重复计算；
3. **定向局部重绘**：在 `mouseMoveEvent`、`scrollContentsBy` 及折叠展开变化时重新计算悬停；**仅当行或区域发生真实变化时**，才对旧行与新行分别调用一次 `viewport()->update(visualRect)`，严禁无条件重绘，严禁整视图刷新，严禁使用定时器；
4. **视口离开清理**：通过 `viewportEvent` 捕获 `QEvent::Leave`，在鼠标离开视口时即刻清理悬停并刷新，杜绝残留。

---

## Chapter 2: Modified Files List（影响文件清单）

本次方案仅涉及以下 4 个代码文件的局部改动，完全不触碰 `DropTreeView` 基类或任何其他范围外文件：
1. `QuarkMeta_Source/src/ui/LibraryItemDelegate.h`
2. `QuarkMeta_Source/src/ui/LibraryItemDelegate.cpp`
3. `QuarkMeta_Source/src/ui/LibraryPanel.h`
4. `QuarkMeta_Source/src/ui/LibraryPanel.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/LibraryItemDelegate.h`

<<<<<<< SEARCH
namespace QuarkMeta {

/**
 * @brief 库树视图项目渲染委托
 * 物理独立拆分，严格遵循单一职责原则：只负责视觉绘制与交互点击检测，不含业务逻辑。
 */
class LibraryItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit LibraryItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    ~LibraryItemDelegate() override = default;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    void setFilterKeyword(const QString& keyword) { m_filterKeyword = keyword; }

    // 静态布局转发（兼容旧调用或布局查询）
    static QRect addBtnRect(const QRect& optionRect) {
        return LibraryTreeLayout::addBtnRect(optionRect);
    }
    static QRect sectionHeaderRect(const QRect& optionRect, const QString& text, const QFont& font) {
        return LibraryTreeLayout::sectionHeaderRect(optionRect, text, font);
    }

signals:
    void requestAddCategory(LibraryNodeKind kind);
    void requestToggleSection(LibraryNodeKind kind);
=======
namespace QuarkMeta {

class LibraryTreeView;

/**
 * @brief 分组标头行内的交互命中区域枚举
 */
enum class SectionHitArea {
    None,       // 不在行内或无命中
    Row,        // 行内空白处
    Header,     // 折叠组件块（文字 + 折叠指示箭头全区域）
    AddButton   // 右侧“+”加号按钮区域
};

/**
 * @brief 库树视图项目渲染委托
 * 物理独立拆分，严格遵循单一职责原则：只负责视觉绘制与交互点击检测，不含业务逻辑。
 */
class LibraryItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit LibraryItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    ~LibraryItemDelegate() override = default;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    void setFilterKeyword(const QString& keyword) { m_filterKeyword = keyword; }

    // 静态布局转发（兼容旧调用或布局查询）
    static QRect addBtnRect(const QRect& optionRect) {
        return LibraryTreeLayout::addBtnRect(optionRect);
    }
    static QRect sectionHeaderRect(const QRect& optionRect, const QString& text, const QFont& font) {
        return LibraryTreeLayout::sectionHeaderRect(optionRect, text, font);
    }

    // 提取的单一真理源静态方法
    static QString sectionTitleByNodeId(int nodeId, int count);
    static QFont sectionHeaderFont(const QFont& baseFont);
    static SectionHitArea hitTestSectionRow(const QRect& optionRect, int nodeId, int count, const QFont& baseFont, const QPoint& pos);

signals:
    void requestAddCategory(LibraryNodeKind kind);
    void requestToggleSection(LibraryNodeKind kind);
>>>>>>> REPLACE

---

### 3.2 `QuarkMeta_Source/src/ui/LibraryItemDelegate.cpp`

<<<<<<< SEARCH
#include "LibraryItemDelegate.h"
#include "UiHelper.h"
#include "ViewDragDropHelper.h"
#include <QTreeView>
#include <QCursor>

namespace QuarkMeta {

QSize LibraryItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    sz.setHeight(LibraryTreeLayout::kRowHeight);
    return sz;
}

bool LibraryItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
    if (!index.isValid()) {
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    LibraryNodeKind kind = static_cast<LibraryNodeKind>(index.data(NodeKindRole).toInt());

    // 1. 三大分组标头交互
    if (isSectionKind(kind)) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
            if (mouseEv && mouseEv->button() == Qt::LeftButton) {
                // 先判"+"加号按钮：仅对智能文件夹与文件夹生效
                if (kind == LibraryNodeKind::SectionSmartFolder || kind == LibraryNodeKind::SectionCustom) {
                    QRect btnRect = LibraryTreeLayout::addBtnRect(option.rect);
                    if (btnRect.contains(mouseEv->pos())) {
                        emit requestAddCategory(kind);
                        return true;
                    }
                }

                // 再判③区域（折叠组件块：文字+折叠指示箭头全区域）
                int countVal = index.data(CountRole).toInt();
                QString sectionText = sectionTitle(kind, countVal);

                QFont hdrFont = option.font;
                hdrFont.setPointSize(10);
                hdrFont.setBold(true);

                QRect hdrRect = LibraryTreeLayout::sectionHeaderRect(option.rect, sectionText, hdrFont);
                if (hdrRect.contains(mouseEv->pos())) {
                    emit requestToggleSection(kind);
                    return true;
                }
                return true; // 其余点击消费掉，防止误触或漂移
            }
        }
    } else if (kind == LibraryNodeKind::Category && model && model->hasChildren(index)) {
=======
#include "LibraryItemDelegate.h"
#include "UiHelper.h"
#include "ViewDragDropHelper.h"
#include "LibraryPanel.h"
#include <QTreeView>

namespace QuarkMeta {

QString LibraryItemDelegate::sectionTitleByNodeId(int nodeId, int count) {
    if (nodeId == -10) return QString("快速访问(%1)").arg(count);
    if (nodeId == -20) return QString("智能文件夹(%1)").arg(count);
    return QString("文件夹(%1)").arg(count);
}

QFont LibraryItemDelegate::sectionHeaderFont(const QFont& baseFont) {
    QFont font = baseFont;
    font.setPointSize(10);
    font.setBold(true);
    return font;
}

SectionHitArea LibraryItemDelegate::hitTestSectionRow(const QRect& optionRect, int nodeId, int count, const QFont& baseFont, const QPoint& pos) {
    if (!optionRect.contains(pos)) {
        return SectionHitArea::None;
    }

    // 优先判定 AddButton（仅节点 -20 与 0 拥有加号按钮）
    if (nodeId == -20 || nodeId == 0) {
        QRect btnRect = LibraryTreeLayout::addBtnRect(optionRect);
        if (btnRect.contains(pos)) {
            return SectionHitArea::AddButton;
        }
    }

    // 次判定 Header（折叠组件区域：文字+折叠指示箭头全区域）
    QString title = sectionTitleByNodeId(nodeId, count);
    QFont hdrFont = sectionHeaderFont(baseFont);
    QRect hdrRect = LibraryTreeLayout::sectionHeaderRect(optionRect, title, hdrFont);
    if (hdrRect.contains(pos)) {
        return SectionHitArea::Header;
    }

    // 其余落在行内但非上述组件的区域为 Row
    return SectionHitArea::Row;
}

QSize LibraryItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    sz.setHeight(LibraryTreeLayout::kRowHeight);
    return sz;
}

bool LibraryItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
    if (!index.isValid()) {
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    LibraryNodeKind kind = static_cast<LibraryNodeKind>(index.data(NodeKindRole).toInt());

    // 1. 三大分组标头交互
    if (isSectionKind(kind)) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
            if (mouseEv && mouseEv->button() == Qt::LeftButton) {
                int nodeId = index.data(CategoryIdRole).toInt();
                if (kind == LibraryNodeKind::SectionQuickAccess) nodeId = -10;
                else if (kind == LibraryNodeKind::SectionSmartFolder) nodeId = -20;
                else if (kind == LibraryNodeKind::SectionCustom) nodeId = 0;

                int countVal = index.data(CountRole).toInt();
                SectionHitArea area = hitTestSectionRow(option.rect, nodeId, countVal, option.font, mouseEv->pos());

                if (area == SectionHitArea::AddButton) {
                    emit requestAddCategory(kind);
                    return true;
                } else if (area == SectionHitArea::Header) {
                    emit requestToggleSection(kind);
                    return true;
                }
                return true; // 其余点击消费掉，防止误触或漂移
            }
        }
    } else if (kind == LibraryNodeKind::Category && model && model->hasChildren(index)) {
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 1. 渲染三大分组标头 (快速访问, 智能文件夹, 文件夹)，绝对禁止绘制选中/拖放背景
    if (isSectionKind(kind)) {
        // 全行底色严格透明，保证②区域（中间空白留白）彻底透明，绝不与两端连接成一体
        painter->fillRect(opt.rect, Qt::transparent);

        bool isRowHovered = (opt.state & QStyle::State_MouseOver);

        const QTreeView* treeView = qobject_cast<const QTreeView*>(option.widget);
        bool isExpanded = treeView ? treeView->isExpanded(index) : true;
        int countVal = index.data(CountRole).toInt();

        QString sectionText = sectionTitle(kind, countVal);

        QFont hdrFont = opt.font;
        hdrFont.setPointSize(10);
        hdrFont.setBold(true);

        // 获取鼠标在树视图中的相对坐标，用于分别判定③与①区域的独立悬停状态
        QPoint mousePos = treeView ? treeView->mapFromGlobal(QCursor::pos()) : QPoint(-1, -1);

        // ③ 区域：折叠组件块（文字 + 折叠指示箭头全区域）
        QRect hdrRect = LibraryTreeLayout::sectionHeaderRect(opt.rect, sectionText, hdrFont);
        bool isHdrHovered = isRowHovered && hdrRect.contains(mousePos);

        // 独立绘制 ③ 区域的高亮背景与内容
        if (isHdrHovered) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor("#2A2D2E"));
            painter->drawRoundedRect(hdrRect, 3, 3);
        }

        QFontMetrics fm(hdrFont);
        int textWidth = fm.horizontalAdvance(sectionText);
        int textLeft = hdrRect.left() + LibraryTreeLayout::kSectionPaddingLeft;
        QRect textRect(textLeft, opt.rect.top(), textWidth, opt.rect.height());

        painter->setFont(hdrFont);
        painter->setPen(isHdrHovered ? QColor("#FFFFFF") : (isRowHovered ? QColor("#DDDDDD") : QColor("#888888")));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, sectionText);

        // 绘制紧跟文字右侧的展开/折叠箭头 (chevron_down / chevron_right)
        QString arrowKey = isExpanded ? QStringLiteral("chevron_down") : QStringLiteral("chevron_right");
        QIcon arrowIcon = UiHelper::getIcon(arrowKey, isHdrHovered ? QColor("#FFFFFF") : (isRowHovered ? QColor("#CCCCCC") : QColor("#888888")), LibraryTreeLayout::kArrowSize);
        int arrowLeft = textRect.right() + LibraryTreeLayout::kArrowSpacing;
        QRect arrowRect(arrowLeft, opt.rect.top() + (opt.rect.height() - LibraryTreeLayout::kArrowSize) / 2, LibraryTreeLayout::kArrowSize, LibraryTreeLayout::kArrowSize);
        arrowIcon.paint(painter, arrowRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);

        // ① 区域：右侧加号按钮 (+) 图标 ("add")，仅当整行处于悬停时才显示，且高亮完全独立
        if ((kind == LibraryNodeKind::SectionSmartFolder || kind == LibraryNodeKind::SectionCustom) && isRowHovered) {
            QRect addRect = LibraryTreeLayout::addBtnRect(opt.rect);
            bool isAddBtnHovered = addRect.contains(mousePos);

            if (isAddBtnHovered) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor("#3A3D3E"));
                painter->drawRoundedRect(addRect, 3, 3);
            }

            QIcon addIcon = UiHelper::getIcon("add", isAddBtnHovered ? QColor("#FFFFFF") : QColor("#CCCCCC"), LibraryTreeLayout::kAddBtnIconSize);
            addIcon.paint(painter, addRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }

        painter->restore();
        return;
    }
=======
    // 1. 渲染三大分组标头 (快速访问, 智能文件夹, 文件夹)，绝对禁止绘制选中/拖放背景
    if (isSectionKind(kind)) {
        // 全行底色严格透明，保证②区域（中间空白留白）彻底透明，绝不与两端连接成一体
        painter->fillRect(opt.rect, Qt::transparent);

        const LibraryTreeView* libTree = qobject_cast<const LibraryTreeView*>(option.widget);
        bool isExpanded = libTree ? libTree->isExpanded(index) : true;

        bool isRowHovered = false;
        bool isHdrHovered = false;
        bool isAddBtnHovered = false;

        if (libTree && libTree->hoveredIndex() == index) {
            isRowHovered = true;
            SectionHitArea area = libTree->hoveredArea();
            if (area == SectionHitArea::Header) {
                isHdrHovered = true;
            } else if (area == SectionHitArea::AddButton) {
                isAddBtnHovered = true;
            }
        }

        int countVal = index.data(CountRole).toInt();
        int nodeId = index.data(CategoryIdRole).toInt();
        if (kind == LibraryNodeKind::SectionQuickAccess) nodeId = -10;
        else if (kind == LibraryNodeKind::SectionSmartFolder) nodeId = -20;
        else if (kind == LibraryNodeKind::SectionCustom) nodeId = 0;

        QString sectionText = sectionTitleByNodeId(nodeId, countVal);
        QFont hdrFont = sectionHeaderFont(opt.font);

        // ③ 区域：折叠组件块（文字 + 折叠指示箭头全区域）
        QRect hdrRect = LibraryTreeLayout::sectionHeaderRect(opt.rect, sectionText, hdrFont);

        // 独立绘制 ③ 区域的高亮背景与内容
        if (isHdrHovered) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor("#2A2D2E"));
            painter->drawRoundedRect(hdrRect, 3, 3);
        }

        QFontMetrics fm(hdrFont);
        int textWidth = fm.horizontalAdvance(sectionText);
        int textLeft = hdrRect.left() + LibraryTreeLayout::kSectionPaddingLeft;
        QRect textRect(textLeft, opt.rect.top(), textWidth, opt.rect.height());

        painter->setFont(hdrFont);
        painter->setPen(isHdrHovered ? QColor("#FFFFFF") : (isRowHovered ? QColor("#DDDDDD") : QColor("#888888")));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, sectionText);

        // 绘制紧跟文字右侧的展开/折叠箭头 (chevron_down / chevron_right)
        QString arrowKey = isExpanded ? QStringLiteral("chevron_down") : QStringLiteral("chevron_right");
        QIcon arrowIcon = UiHelper::getIcon(arrowKey, isHdrHovered ? QColor("#FFFFFF") : (isRowHovered ? QColor("#CCCCCC") : QColor("#888888")), LibraryTreeLayout::kArrowSize);
        int arrowLeft = textRect.right() + LibraryTreeLayout::kArrowSpacing;
        QRect arrowRect(arrowLeft, opt.rect.top() + (opt.rect.height() - LibraryTreeLayout::kArrowSize) / 2, LibraryTreeLayout::kArrowSize, LibraryTreeLayout::kArrowSize);
        arrowIcon.paint(painter, arrowRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);

        // ① 区域：右侧加号按钮 (+) 图标 ("add")，仅当整行处于悬停时才显示，且高亮完全独立
        if ((kind == LibraryNodeKind::SectionSmartFolder || kind == LibraryNodeKind::SectionCustom) && isRowHovered) {
            QRect addRect = LibraryTreeLayout::addBtnRect(opt.rect);

            if (isAddBtnHovered) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor("#3A3D3E"));
                painter->drawRoundedRect(addRect, 3, 3);
            }

            QIcon addIcon = UiHelper::getIcon("add", isAddBtnHovered ? QColor("#FFFFFF") : QColor("#CCCCCC"), LibraryTreeLayout::kAddBtnIconSize);
            addIcon.paint(painter, addRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }

        painter->restore();
        return;
    }
>>>>>>> REPLACE

---

### 3.3 `QuarkMeta_Source/src/ui/LibraryPanel.h`

<<<<<<< SEARCH
class LibraryTreeView : public DropTreeView {
    Q_OBJECT
public:
    explicit LibraryTreeView(QWidget* parent = nullptr) : DropTreeView(parent) {}
    ~LibraryTreeView() override = default;

protected:
    void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override {
        if (index.isValid()) {
            LibraryNodeKind kind = static_cast<LibraryNodeKind>(index.data(NodeKindRole).toInt());
            if (isSectionKind(kind)) {
                return;
            }
        }
        DropTreeView::drawBranches(painter, rect, index);
    }
};
=======
class LibraryTreeView : public DropTreeView {
    Q_OBJECT
public:
    explicit LibraryTreeView(QWidget* parent = nullptr);
    ~LibraryTreeView() override = default;

    // 悬停状态唯一访问入口
    QPersistentModelIndex hoveredIndex() const { return m_hoveredIndex; }
    SectionHitArea hoveredArea() const { return m_hoveredArea; }

    void reevaluateHover();

protected:
    void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override;
    void mouseMoveEvent(QMouseEvent* event) override;
    bool viewportEvent(QEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;

private:
    void updateHoverState(const QPoint& viewportPos);

    QPersistentModelIndex m_hoveredIndex;
    SectionHitArea m_hoveredArea = SectionHitArea::None;
};
>>>>>>> REPLACE

---

### 3.4 `QuarkMeta_Source/src/ui/LibraryPanel.cpp`

<<<<<<< SEARCH
namespace QuarkMeta {

LibraryPanel::LibraryPanel(QWidget* parent) : QFrame(parent) {
=======
namespace QuarkMeta {

LibraryTreeView::LibraryTreeView(QWidget* parent) : DropTreeView(parent) {
    connect(this, &QTreeView::expanded, this, [this](const QModelIndex&) {
        reevaluateHover();
    });
    connect(this, &QTreeView::collapsed, this, [this](const QModelIndex&) {
        reevaluateHover();
    });
}

void LibraryTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const {
    if (index.isValid()) {
        LibraryNodeKind kind = static_cast<LibraryNodeKind>(index.data(NodeKindRole).toInt());
        if (isSectionKind(kind)) {
            return;
        }
    }
    DropTreeView::drawBranches(painter, rect, index);
}

void LibraryTreeView::updateHoverState(const QPoint& viewportPos) {
    QModelIndex curIdx = indexAt(viewportPos);
    SectionHitArea newArea = SectionHitArea::None;
    QPersistentModelIndex newIndex;

    if (curIdx.isValid()) {
        newIndex = curIdx;
        LibraryNodeKind kind = static_cast<LibraryNodeKind>(curIdx.data(NodeKindRole).toInt());
        if (isSectionKind(kind)) {
            int nodeId = curIdx.data(CategoryIdRole).toInt();
            if (kind == LibraryNodeKind::SectionQuickAccess) nodeId = -10;
            else if (kind == LibraryNodeKind::SectionSmartFolder) nodeId = -20;
            else if (kind == LibraryNodeKind::SectionCustom) nodeId = 0;

            int countVal = curIdx.data(CountRole).toInt();
            QRect rowRect = visualRect(curIdx);
            newArea = LibraryItemDelegate::hitTestSectionRow(rowRect, nodeId, countVal, font(), viewportPos);
        } else {
            newArea = SectionHitArea::Row;
        }
    }

    if (m_hoveredIndex != newIndex || m_hoveredArea != newArea) {
        if (m_hoveredIndex.isValid() && viewport()) {
            viewport()->update(visualRect(m_hoveredIndex));
        }
        m_hoveredIndex = newIndex;
        m_hoveredArea = newArea;
        if (m_hoveredIndex.isValid() && viewport()) {
            viewport()->update(visualRect(m_hoveredIndex));
        }
    }
}

void LibraryTreeView::mouseMoveEvent(QMouseEvent* event) {
    DropTreeView::mouseMoveEvent(event);
    updateHoverState(event->pos());
}

bool LibraryTreeView::viewportEvent(QEvent* event) {
    if (event->type() == QEvent::Leave) {
        if (m_hoveredIndex.isValid() || m_hoveredArea != SectionHitArea::None) {
            if (m_hoveredIndex.isValid() && viewport()) {
                viewport()->update(visualRect(m_hoveredIndex));
            }
            m_hoveredIndex = QPersistentModelIndex();
            m_hoveredArea = SectionHitArea::None;
        }
    }
    return DropTreeView::viewportEvent(event);
}

void LibraryTreeView::scrollContentsBy(int dx, int dy) {
    DropTreeView::scrollContentsBy(dx, dy);
    reevaluateHover();
}

void LibraryTreeView::reevaluateHover() {
    if (!viewport()) return;
    QPoint localPos = viewport()->mapFromGlobal(QCursor::pos());
    if (viewport()->rect().contains(localPos)) {
        updateHoverState(localPos);
    } else {
        if (m_hoveredIndex.isValid() || m_hoveredArea != SectionHitArea::None) {
            if (m_hoveredIndex.isValid() && viewport()) {
                viewport()->update(visualRect(m_hoveredIndex));
            }
            m_hoveredIndex = QPersistentModelIndex();
            m_hoveredArea = SectionHitArea::None;
        }
    }
}

LibraryPanel::LibraryPanel(QWidget* parent) : QFrame(parent) {
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 离线检索验证
1. 检索 `LibraryItemDelegate.cpp` 与 `LibraryItemDelegate.h`，确认已彻底无 `QCursor` 引用；
2. 检索 `paint` 与 `editorEvent`，确认完全复用 `hitTestSectionRow`、`sectionTitleByNodeId` 与 `sectionHeaderFont`；
3. 检查 `LibraryTreeView` 对悬停重绘的触发时机，确认仅在 `(m_hoveredIndex != newIndex || m_hoveredArea != newArea)` 分支触发受影响行的 `viewport()->update`；
4. 检查基类事件调用，确认 `DropTreeView::mouseMoveEvent`、`DropTreeView::viewportEvent`、`DropTreeView::scrollContentsBy` 均被照常调用。

### 4.2 运行时验收标准 (Parity Checklist)
1. **即时悬停高亮**：鼠标移动到折叠组件块（文字+箭头）上方，立即显示 `#2A2D2E` 背景高亮、文字变 `#FFFFFF`、箭头变 `#FFFFFF`，无需任何点击；移出立即还原；
2. **加号按钮即时高亮**：鼠标移到右侧“+”按钮上方，立即显示 `#3A3D3E` 独立圆角高亮；移出立即还原；
3. **离开视口彻底清除**：鼠标移出面板视口边界，所有分组标题行的悬停高亮完全清除；
4. **滚动与展开折叠联动**：滚轮滚动视口内容、展开/折叠节点导致标头行位置相对鼠标移动时，标头悬停状态同步自动重新计算并即时刷新；
5. **计算性能极度轻量**：仅做单次轻量矩形包含判断，不触发任何数据库或全局重新计算。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **复用 `LibraryTreeLayout`**：`hitTestSectionRow` 内部直接调用 `LibraryTreeLayout::addBtnRect` 与 `LibraryTreeLayout::sectionHeaderRect`，杜绝重复手写矩形计算；
2. **复用 `sectionTitleByNodeId` 与 `sectionHeaderFont`**：消除原 `paint` 与 `editorEvent` 中各自重复设置字体（10pt加粗）与格式化标题字符串的代码；
3. **复用 Qt 视口局部重绘机制**：通过 `viewport()->update(visualRect(idx))` 仅重绘受影响行，禁止调用全视图刷新。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 / 业务概念 | 唯一 SSOT 真理源 | 派生副本与表现形式 | 读写与同步方向 |
| :--- | :--- | :--- | :--- |
| 当前悬停行与区域 | `LibraryTreeView` (`m_hoveredIndex`, `m_hoveredArea`) | `LibraryItemDelegate::paint` 绘制依据 | `LibraryTreeView` 唯一计算写入，委托只读读取 |
| 标题文字与字体 | `LibraryItemDelegate` 静态方法 | 绘制与命中判定 | 单一静态函数输出，委托内部统一调用 |
| 标头区域矩形几何 | `LibraryTreeLayout` | `hitTestSectionRow` 命中范围 | 单一真理源提供 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. `LibraryTreeView::hoveredIndex() const`：公开只读访问；
2. `LibraryTreeView::hoveredArea() const`：公开只读访问；
3. `LibraryTreeView::reevaluateHover()`：公开辅助刷新方法；
4. `LibraryItemDelegate::hitTestSectionRow`：公有静态方法，签名与实现完全一致；
5. 成员访问级别均保持严格受控，私有成员仅在所属类中使用。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `LibraryItemDelegate.h` 前置声明 `class LibraryTreeView;`；
2. `LibraryItemDelegate.cpp` 包含 `LibraryPanel.h`（以获得 `LibraryTreeView` 的完整类定义）；
3. `LibraryItemDelegate.cpp` 移除了无用的 `<QCursor>` 包含；
4. `LibraryPanel.h` 完整声明 `LibraryTreeView` 并继承 `DropTreeView`。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 检查项编号 | 规范要求与检查内容 | 达标状态 | 位置或证明说明 |
| :--- | :--- | :--- | :--- |
| 1 | 悬停状态唯一来源（SSOT） | 满足 | `LibraryTreeView` 统一持有 `m_hoveredIndex` 与 `m_hoveredArea` |
| 2 | paint 中彻底移除 QCursor | 满足 | 删除 `QCursor::pos()`，改为读取树视图状态 |
| 3 | 只有状态变化才重绘 | 满足 | 严格在 `(m_hoveredIndex != newIndex || m_hoveredArea != newArea)` 分支触发 |
| 4 | 离开视口彻底清除 | 满足 | `viewportEvent` 捕获 `QEvent::Leave` 并清理 |
| 5 | 滚动与折叠联动重新计算 | 满足 | `scrollContentsBy` 与 `expanded`/`collapsed` 均触发 `reevaluateHover` |
| 6 | 严格保持外观与颜色数值不变 | 满足 | `#2A2D2E`、`#3A3D3E`、`#FFFFFF`、`#DDDDDD`、`#888888`、`#CCCCCC`、圆角 3 完全原样照抄 |
| 7 | 禁止打补丁与无条件全量 update | 满足 | 无任何定时器轮询，仅定向重绘受影响行 |
| 8 | 源代码 100% 保持基准零改动 | 满足 | 改动严格作为实施方案隔离在 `Implementation Plan/` 目录 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 交互设计零篡改核对表
| 交互场景 | 既有行为 | 方案行为 | 是否完全一致 |
| :--- | :--- | :--- | :--- |
| 折叠组件点击 | 点击触发折叠/展开 | 经 `hitTestSectionRow` 命中 `Header`，发射 `requestToggleSection` | 完全一致 |
| 加号按钮点击 | 智能文件夹与分类标题行点击加号触发新建 | 经 `hitTestSectionRow` 命中 `AddButton`，发射 `requestAddCategory` | 完全一致 |
| 树拖拽与重排 | 内部拖拽校验与移动 | 继承并透传调用 `DropTreeView::mouseMoveEvent` 等 | 完全一致 |

### 10.2 样式数值零篡改核对表
| 样式属性 | 原实现数值 | 方案数值 | 是否 100% 照抄 |
| :--- | :--- | :--- | :--- |
| 折叠组件高亮色 | `#2A2D2E` | `#2A2D2E` | 100% 照抄 |
| 加号按钮高亮色 | `#3A3D3E` | `#3A3D3E` | 100% 照抄 |
| 圆角大小 | `3px` | `3px` | 100% 照抄 |
| 文字与箭头颜色 | 高亮 `#FFFFFF`，常态 `#888888`，行悬停 `#DDDDDD`/`#CCCCCC` | 完全一致 | 100% 照抄 |
