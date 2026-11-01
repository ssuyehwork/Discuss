# 代码导出结果 - 20260721_122032

**项目路径**: `G:\C++\FERREX\FERREX-META\src`

**文件总数**: 4

## 文件类型统计

- **cpp**: 4 个文件

---

## 文件: `ui/ThumbnailDelegate.cpp`

```cpp
#include "ThumbnailDelegate.h"
#include <QPainter>
#include <QPainterPath>
#include <QHelpEvent>
#include "ToolTipOverlay.h"
#include <QIcon>
#include <QPixmap>
#include <QStyleOptionViewItem>
#include <QFileInfo>
#include <QMouseEvent>
#include <QLineEdit>
#include <QTimer>
#include <QAbstractItemView>
#include <QTextLayout>
#include <QTextOption>
#include "UiHelper.h"

namespace FERREX {

ThumbnailDelegate::ThumbnailDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void ThumbnailDelegate::setHasThumbnailRole(int role) { m_hasThumbnailRole = role; }
void ThumbnailDelegate::setPathRole(int role) { m_pathRole = role; }
void ThumbnailDelegate::setManagedRole(int role) { m_managedRole = role; }
void ThumbnailDelegate::setIsEmptyRole(int role) { m_isEmptyRole = role; }

ThumbnailDelegate::Metrics ThumbnailDelegate::calculateMetrics(const QStyleOptionViewItem& option) const {
    Metrics m;
    const int textHeight = 36;
    const int gap = 6; // 卡片与文件名的紧凑间隙

    // 底部预留高度调整：文件名高度 + 间距 + 底部内边距补偿(3px)
    m.cardRect = option.rect.adjusted(3, 3, -3, -(textHeight + gap + 3));
    
    // 文件名框紧贴卡片底部下方 gap 像素的位置
    m.textRect = QRect(option.rect.left() + 3,
                       m.cardRect.bottom() + gap,
                       option.rect.width() - 6,
                       textHeight);
    
    m.banRect = QRect();

    return m;
}

void ThumbnailDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Metrics m = calculateMetrics(option);
    bool isSelected = (option.state & QStyle::State_Selected);
    bool isGrid = option.widget ? option.widget->property("gridMode").toBool() : false;

    QVariant decoData = index.data(Qt::DecorationRole);
    
    QPixmap thumb;
    bool hasValidThumb = false;

    // 【物理还原缩略图 QPixmap 优先准入与无损渲染机制】：彻底废除关于缩略图渲染的 thumbStatus == 1 前置校验！
    // 只要 decoData 能转换为 QPixmap，不管当前状态字，均认为这属于可用的、高清晰度的缩略图物理资产，优先提取渲染！
    if (decoData.canConvert<QPixmap>()) {
        thumb = decoData.value<QPixmap>();
        if (!thumb.isNull()) {
            hasValidThumb = true;
        }
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    // ① 绘制内容与裁剪 (Cover 模式)
    painter->save();
    QPainterPath clipPath;
    clipPath.addRoundedRect(m.cardRect, 6, 6);
    painter->setClipPath(clipPath);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor("#2d2d2d"));
    painter->drawRect(m.cardRect);

    // 关键路径重构：缩略图优先（直接以 100% 完全不透明度绘制）
    if (hasValidThumb) {
        // 缩略图平滑拉伸并充满卡片（100% Cover/Contain）
        QPixmap scaled = thumb.scaled(m.cardRect.size(), 
                                      isGrid ? Qt::KeepAspectRatio : Qt::KeepAspectRatioByExpanding, 
                                      Qt::SmoothTransformation);
        int x = m.cardRect.center().x() - scaled.width() / 2;
        int y = m.cardRect.center().y() - scaled.height() / 2;
        
        painter->drawPixmap(x, y, scaled);
    } else {
        // 系统默认图标只作为【没有缩略图或生成失败时】的最后兜底回退手段
        // 在没有有效缩略图物理资产时，直接以 100% 的完全不透明度绘制默认的文件类型关联图标，绝不闪现空白卡片
        QIcon icon = qvariant_cast<QIcon>(decoData);
        if (!icon.isNull()) {
            painter->setOpacity(1.0);
            
            // [性能重构方案物理对齐]：彻底消除 High-DPI 尺寸膨胀与拉伸缺陷，统一在逻辑像素矩形下使用 icon.paint 进行 1:1 等比例、完美居中绘制
            int iconSize = qMin(m.cardRect.width(), m.cardRect.height()) * 0.55;
            QRect iconRect(m.cardRect.center().x() - iconSize / 2,
                           m.cardRect.center().y() - iconSize / 2,
                           iconSize, iconSize);
            
            icon.paint(painter, iconRect);
            
            painter->setOpacity(1.0);
        }
    }
    painter->restore();

    // ③ 绘制卡片边框
    painter->save();
    if (isSelected) {
        painter->setPen(QPen(QColor("#3498db"), 3));
    } else {
        painter->setPen(QPen(QColor("#4a4a4a"), 1));
    }
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(m.cardRect, 6, 6);
    painter->restore();

    // 状态位图标绘制
    if (m_managedRole != -1) {
        bool isManaged = index.data(m_managedRole).toBool();
        if (isManaged) {
            QRect statusRect(m.cardRect.right() - 22, m.cardRect.top() + 8, 16, 16);
            UiHelper::getIcon("check_circle", QColor("#2ecc71"), 16).paint(painter, statusRect);
        }
    }

    // 扩展名角标
    if (m_pathRole != -1) {
        QString path = index.data(m_pathRole).toString();
        QFileInfo info(path);
        QString ext = info.isDir() ? "DIR" : info.suffix().toUpper();
        if (!ext.isEmpty()) {
            QColor badgeColor = UiHelper::getExtensionColor(ext);

            if (!hasValidThumb) {
                badgeColor.setAlpha(160);
            }

            QRect extRect(m.cardRect.left() + 8, m.cardRect.top() + 8, 36, 18);
            painter->setPen(Qt::NoPen);
            painter->setBrush(badgeColor);
            painter->drawRoundedRect(extRect, 2, 2);
            painter->setPen(hasValidThumb ? QColor("#FFFFFF") : QColor(255, 255, 255, 180));
            QFont extFont = painter->font(); extFont.setPointSize(8); extFont.setBold(true);
            painter->setFont(extFont);
            painter->drawText(extRect, Qt::AlignCenter, ext);
        }
    }

    // [已停用] 星级渲染逻辑：星级已不再使用，此处直接跳过以节省 CPU 消耗

    // ③ 文件名（卡片下方）
    painter->save();
    QString name = index.data(Qt::DisplayRole).toString();
    painter->setPen(isSelected ? QColor("#3498db") : QColor("#EEEEEE"));

    if (m_managedRole != -1 && !isSelected && !index.data(m_managedRole).toBool()) {
        painter->setPen(QColor(238, 238, 238, 120));
    }

    QFont textFont = painter->font();
    textFont.setPointSize(8);
    painter->setFont(textFont);

    QString displayName = name;
    displayName.replace("_", "_\u200B");
    displayName.replace(".", ".\u200B");

    // 采用方案 A：使用 QTextLayout 进行精准的双行物理修剪与第二行末尾省略
    QTextLayout textLayout(displayName, painter->font());
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textOption.setAlignment(Qt::AlignCenter);
    textLayout.setTextOption(textOption);

    textLayout.beginLayout();
    int lineCount = 0;
    int textWidth = m.textRect.width() - 8; // 两侧留出 4px 安全边距 (对应 adjusted(4, 0, -4, 0))
    int currentY = m.textRect.top();
    int fontHeight = option.fontMetrics.height();

    // 存储切分出的各行
    struct RenderLine {
        QString text;
        int y;
    };
    QList<RenderLine> linesToRender;

    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid()) {
            break;
        }
        line.setLineWidth(textWidth);
        lineCount++;

        if (lineCount == 1) {
            // 第一行完整保留
            int start = line.textStart();
            int len = line.textLength();
            linesToRender.append({displayName.mid(start, len), currentY});
            currentY += fontHeight;
        } else if (lineCount == 2) {
            // 关键路径：检查是否存在第三行
            QTextLine nextLine = textLayout.createLine();
            if (nextLine.isValid()) {
                // 确实存在第三行或更多，第二行必须承接全部剩余的长尾内容，并做 ElideMiddle 裁剪
                int start = line.textStart();
                QString remainingText = displayName.mid(start);
                // 物理省略
                QString elidedRemaining = option.fontMetrics.elidedText(remainingText, Qt::ElideMiddle, textWidth);
                linesToRender.append({elidedRemaining, currentY});
            } else {
                // 没有第三行，第二行正常显示
                int start = line.textStart();
                int len = line.textLength();
                linesToRender.append({displayName.mid(start, len), currentY});
            }
            break; // 绝对阻断第三行，退出排版循环
        }
    }
    textLayout.endLayout();

    // 物理渲染
    for (const auto& rLine : linesToRender) {
        QRect lineRect(m.textRect.left() + 4, rLine.y, textWidth, fontHeight);
        painter->drawText(lineRect, Qt::AlignCenter, rLine.text);
    }

    painter->restore();

    // ④ 空文件夹特殊标记
    if (!isSelected && m_isEmptyRole != -1) {
        if (index.data(m_isEmptyRole).toBool()) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(QPen(QColor("#41F2F2"), 1, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(m.cardRect, 6, 6);
            painter->restore();
        }
    }

    painter->restore();
}

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    return QStyledItemDelegate::sizeHint(option, index);
}

QWidget* ThumbnailDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
    if (editor) {
        editor->setStyleSheet(
            "background-color: #2D2D2D; color: white; selection-background-color: #3498db; "
            "border: 1px solid #3498db; border-radius: 4px; padding: 0 4px;"
        );
    }
    return editor;
}

void ThumbnailDelegate::updateEditorGeometry(QWidget* editor,
                                              const QStyleOptionViewItem& option,
                                              const QModelIndex& /*index*/) const {
    Metrics m = calculateMetrics(option);
    // 重命名编辑框位置微调，完美契合没有星级后的新 textRect 布局 [1]
    editor->setGeometry(m.textRect.adjusted(1, 5, -1, -5));
}

void ThumbnailDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    QString value = index.model()->data(index, Qt::EditRole).toString();
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor); 
    if (lineEdit) {
        lineEdit->setText(value); 
        int lastDot = value.lastIndexOf('.'); 
        if (lastDot > 0) { 
            lineEdit->setSelection(0, lastDot); 
        } else { 
            lineEdit->selectAll(); 
        }
    }
}

bool ThumbnailDelegate::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = reinterpret_cast<QKeyEvent*>(event); 
        QLineEdit* editor = qobject_cast<QLineEdit*>(obj); 
        if (editor) { 
            switch (keyEvent->key()) { 
                case Qt::Key_Left: 
                case Qt::Key_Right: 
                case Qt::Key_Up: 
                case Qt::Key_Down: 
                case Qt::Key_Home: 
                case Qt::Key_End: 
                    keyEvent->accept(); 
                    return false; 
                default: 
                    break; 
            } 
        } 
    } 
    return QStyledItemDelegate::eventFilter(obj, event); 
} 

bool ThumbnailDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
    // 2026-07-xx 重构：星级已不再使用，不拦截 any 鼠标按下事件修改星级，直接走基类逻辑 [1]
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

} // namespace FERREX
```

## 文件: `ui/GridResultView.cpp`

```cpp
#include "GridResultView.h"
#include "JustifiedView.h"
#include "ThumbnailDelegate.h"
#include "ScanDialog.h"
#include "ScanTableModel.h"
#include <QThreadPool>

namespace FERREX {

GridResultView::GridResultView(QWidget* parent) : IScanResultView(parent) {
    m_justifiedView = new JustifiedView(parent);
    m_justifiedView->setLayoutMode(JustifiedView::GridMode);
    
    auto* delegate = new ThumbnailDelegate(m_justifiedView);
    delegate->setHasThumbnailRole(Qt::UserRole + 1);
    delegate->setPathRole(Qt::UserRole + 3);
    m_justifiedView->setItemDelegate(delegate);
    
    m_justifiedView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_justifiedView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_justifiedView->setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_justifiedView->setDragEnabled(true);
    m_justifiedView->setStyleSheet(
        "background-color: #1E1E1E; border: 1px solid #333; color: #D4D4D4; outline: none;"
    );
}

GridResultView::~GridResultView() {
    if (m_thumbPool) {
        m_thumbPool->clear();
    }
}

QWidget* GridResultView::getWidget() {
    return m_justifiedView;
}

QAbstractItemView* GridResultView::getBaseView() {
    return m_justifiedView;
}

void GridResultView::setModel(QAbstractItemModel* model) {
    m_justifiedView->setModel(model);
    auto* tableModel = qobject_cast<ScanTableModel*>(model);
    if (tableModel) {
        m_thumbPool = tableModel->getThumbPool();
    }
}

void GridResultView::setIconSize(int size) {
    m_justifiedView->setTargetRowHeight(size);
}

void GridResultView::refreshLayout() {
    m_justifiedView->setLayoutMode(JustifiedView::GridMode);
    m_justifiedView->doItemsLayout();
}

} // namespace FERREX
```

## 文件: `ui/JustifiedResultView.cpp`

```cpp
#include "JustifiedResultView.h"
#include "JustifiedView.h"
#include "ThumbnailDelegate.h"
#include "ScanDialog.h"
#include "ScanTableModel.h"
#include <QThreadPool>

namespace FERREX {

JustifiedResultView::JustifiedResultView(QWidget* parent) : IScanResultView(parent) {
    m_justifiedView = new JustifiedView(parent);
    m_justifiedView->setLayoutMode(JustifiedView::JustifiedMode);
    
    auto* delegate = new ThumbnailDelegate(m_justifiedView);
    delegate->setHasThumbnailRole(Qt::UserRole + 1);
    delegate->setPathRole(Qt::UserRole + 3); 
    m_justifiedView->setItemDelegate(delegate);
    
    m_justifiedView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_justifiedView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_justifiedView->setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_justifiedView->setDragEnabled(true);
    m_justifiedView->setStyleSheet(
        "background-color: #1E1E1E; border: 1px solid #333; color: #D4D4D4; outline: none;"
    );
}

JustifiedResultView::~JustifiedResultView() {
    if (m_thumbPool) {
        m_thumbPool->clear();
    }
}

QWidget* JustifiedResultView::getWidget() {
    return m_justifiedView;
}

QAbstractItemView* JustifiedResultView::getBaseView() {
    return m_justifiedView;
}

void JustifiedResultView::setModel(QAbstractItemModel* model) {
    m_justifiedView->setModel(model);
    auto* tableModel = qobject_cast<ScanTableModel*>(model);
    if (tableModel) {
        m_thumbPool = tableModel->getThumbPool();
    }
}

void JustifiedResultView::setIconSize(int size) {
    m_justifiedView->setTargetRowHeight(size);
}

void JustifiedResultView::refreshLayout() {
    m_justifiedView->setLayoutMode(JustifiedView::JustifiedMode);
    m_justifiedView->doItemsLayout();
}

} // namespace FERREX
```

## 文件: `ui/ListResultView.cpp`

```cpp
#include "ListResultView.h"
#include "UiHelper.h"
#include <QTableView>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QLineEdit>

namespace FERREX {

class ListDefaultColumnDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        int hoveredRow = -1;
        if (option.widget) {
            hoveredRow = option.widget->property("hoveredRow").toInt();
        }
        bool isSelected = (option.state & QStyle::State_Selected);
        bool isHovered = (index.row() == hoveredRow);

        if (isSelected) {
            painter->fillRect(option.rect, QColor("#094771"));
        } else if (isHovered) {
            painter->fillRect(option.rect, QColor("#2A2A2A"));
        }

        QStyleOptionViewItem opt = option;
        opt.state &= ~QStyle::State_Selected;
        opt.state &= ~QStyle::State_MouseOver;
        
        QStyledItemDelegate::paint(painter, opt, index);
        painter->restore();
    }
};

class ListThumbnailDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::SmoothPixmapTransform);

        int hoveredRow = -1;
        if (option.widget) {
            hoveredRow = option.widget->property("hoveredRow").toInt();
        }
        bool isSelected = (option.state & QStyle::State_Selected);
        bool isHovered = (index.row() == hoveredRow);

        if (isSelected) {
            painter->fillRect(option.rect, QColor("#094771")); 
        } else if (isHovered) {
            painter->fillRect(option.rect, QColor("#2A2A2A"));
        }

        int padding = 3;
        int side = option.rect.height() - (padding * 2);
        if (side <= 0) side = 16; 

        QRect squareRect(option.rect.left() + 6, option.rect.top() + padding, side, side);

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#2d2d2d"));
        QPainterPath cardPath;
        cardPath.addRoundedRect(squareRect, 4, 4);
        painter->drawPath(cardPath);

        QVariant decoData = index.data(Qt::DecorationRole);

        if (decoData.canConvert<QPixmap>()) {
            QPixmap thumb = decoData.value<QPixmap>();
            if (!thumb.isNull()) {
                QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                int x = squareRect.center().x() - scaled.width() / 2;
                int y = squareRect.center().y() - scaled.height() / 2;
                painter->drawPixmap(x, y, scaled);
            }
        } else {
            QIcon icon = qvariant_cast<QIcon>(decoData);
            if (!icon.isNull()) {
                int iconSize = side * 0.6; 
                QRect iconRect(squareRect.center().x() - iconSize / 2,
                               squareRect.center().y() - iconSize / 2,
                               iconSize, iconSize);
                icon.paint(painter, iconRect);
            }
        }

        QString name = index.data(Qt::DisplayRole).toString();
        QColor textColor = isSelected ? QColor("#FFFFFF") : QColor("#3498db");

        painter->setPen(textColor);
        painter->setFont(option.font);

        QRect textRect = option.rect;
        textRect.setLeft(squareRect.right() + 10);

        QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideMiddle, textRect.width() - 10);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

        // 【物理自绘底部分割线补齐逻辑】（对应用户原话：“唯独名称列到路径列出现这样的截断 / 冗余像素...排查根本原因”）
        // 在 delegate 绘制的最后，使用与 QSS 相同的 #252526 灰色画笔，在最底下一像素处画一条贯通线，使之与第二列无缝相接
        painter->save();
        // 显式关闭抗锯齿，确保绘制出 1 像素物理线，避免变粗和模糊
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->setPen(QColor("#252526"));
        // QRect::bottom() 是底部的 y 坐标，绘制从左边缘到右边缘的 1px 水平线
        painter->drawLine(option.rect.left(), option.rect.bottom(), option.rect.right(), option.rect.bottom());
        painter->restore();

        painter->restore();
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
        if (editor) {
            editor->setStyleSheet(
                "background-color: #2D2D2D; color: white; "
                "selection-background-color: #3498db; "
                "border: 1px solid #3498db; border-radius: 4px; padding: 0 4px;"
            );
        }
        return editor;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        QString value = index.model()->data(index, Qt::EditRole).toString();
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor); 
        if (lineEdit) {
            lineEdit->setText(value); 
            int lastDot = value.lastIndexOf('.'); 
            if (lastDot > 0) { 
                lineEdit->setSelection(0, lastDot); 
            } else { 
                lineEdit->selectAll(); 
            }
        }
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const override {
        int padding = 3;
        int side = option.rect.height() - (padding * 2);
        if (side <= 0) side = 16;

        int textLeft = option.rect.left() + 6 + side + 10;
        const int targetEditorHeight = 28;
        int yOffset = (option.rect.height() - targetEditorHeight) / 2;
        int editorTop = option.rect.top() + yOffset;

        int editorWidth = option.rect.width() - (textLeft - option.rect.left()) - 10;
        QRect editorRect(textLeft, editorTop, editorWidth, targetEditorHeight);

        editor->setGeometry(editorRect);
    }
};

class ListRowHoverFilter : public QObject {
public:
    explicit ListRowHoverFilter(QTableView* tableView) : QObject(tableView), m_tableView(tableView) {
        m_tableView->viewport()->setAttribute(Qt::WA_Hover, true);
        m_tableView->viewport()->setMouseTracking(true);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == m_tableView->viewport()) {
            if (event->type() == QEvent::MouseMove || event->type() == QEvent::HoverMove ||
                event->type() == QEvent::Enter || event->type() == QEvent::HoverEnter) {
                QPoint pos = m_tableView->viewport()->mapFromGlobal(QCursor::pos());
                QModelIndex idx = m_tableView->indexAt(pos);
                int row = idx.isValid() ? idx.row() : -1;
                
                int oldRow = m_tableView->property("hoveredRow").toInt();
                if (row != oldRow) {
                    m_tableView->setProperty("hoveredRow", row);
                    m_tableView->viewport()->update();
                }
            } else if (event->type() == QEvent::Leave || event->type() == QEvent::HoverLeave) {
                int oldRow = m_tableView->property("hoveredRow").toInt();
                if (oldRow != -1) {
                    m_tableView->setProperty("hoveredRow", -1);
                    m_tableView->viewport()->update();
                }
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QTableView* m_tableView;
};

ListResultView::ListResultView(QWidget* parent) : IScanResultView(parent) {
    m_tableView = new QTableView(parent);
    m_tableView->verticalHeader()->setDefaultSectionSize(30); 
    m_tableView->setItemDelegateForColumn(0, new ListThumbnailDelegate(m_tableView)); 
    m_tableView->setItemDelegateForColumn(1, new ListDefaultColumnDelegate(m_tableView)); 
    m_tableView->setItemDelegateForColumn(2, new ListDefaultColumnDelegate(m_tableView)); 
    m_tableView->setItemDelegateForColumn(3, new ListDefaultColumnDelegate(m_tableView)); 
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    
    m_tableView->setProperty("hoveredRow", -1);
    ListRowHoverFilter* hoverFilter = new ListRowHoverFilter(m_tableView);
    m_tableView->viewport()->installEventFilter(hoverFilter);
    
    m_tableView->setStyleSheet(
        "QTableView { "
        "background-color: #1E1E1E; "
        "alternate-background-color: #000000; "
        "border: 1px solid #333; "
        "color: #D4D4D4; "
        "selection-background-color: #094771; "
        "selection-color: #FFFFFF; "
        "outline: none; "
        "gridline-color: transparent; "
        "}"
        "QTableView::item { border-bottom: 1px solid #252526; }"
        "QHeaderView::section { background-color: #252526; color: #888; border: none; border-right: 1px solid #333; padding: 4px; height: 24px; }"
        "QHeaderView::section:horizontal:first { padding-left: 14px; }" 
        "QHeaderView { background-color: #252526; border: none; }"
    );
    
    m_tableView->horizontalHeader()->setStretchLastSection(false); 
    m_tableView->horizontalHeader()->setMinimumSectionSize(60);
    m_tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    
    m_tableView->setColumnWidth(0, 260); 
    m_tableView->setColumnWidth(2, 100); 
    m_tableView->setColumnWidth(3, 140); 
    
    m_tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);

    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection); 
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    m_tableView->setDragEnabled(true);
    m_tableView->setDragDropMode(QAbstractItemView::DragOnly);
    m_tableView->setDefaultDropAction(Qt::CopyAction);

    m_tableView->setShowGrid(false);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
}

ListResultView::~ListResultView() {
}

QWidget* ListResultView::getWidget() {
    return m_tableView;
}

QAbstractItemView* ListResultView::getBaseView() {
    return m_tableView;
}

void ListResultView::setModel(QAbstractItemModel* model) {
    m_tableView->setModel(model);
    m_tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
}

void ListResultView::setIconSize(int size) {
    m_tableView->verticalHeader()->setDefaultSectionSize(size);
}

void ListResultView::refreshLayout() {
}

} // namespace FERREX
```

