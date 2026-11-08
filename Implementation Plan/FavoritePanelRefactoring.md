# FavoritePanelRefactoring.md - 实施方案

## 已读物理证据清单

1. **物理读取文件列表**：
   - `src/ui/FavoritePanel.h`
   - `src/ui/FavoritePanel.cpp`
   - `src/meta/FavoriteService.h`
   - `src/meta/FavoriteService.cpp`
   - `src/meta/FavoriteDao.h`
   - `src/meta/FavoriteDao.cpp`
   - `CMakeLists.txt`

2. **头文件原样签名核验**：
   - `FavoriteDao::getAllFavorites()`：定义于 `FavoriteDao.h` 第 40 行：`static QList<FavoriteRecord> getAllFavorites();`（物理结构体为 `struct FavoriteRecord`）
   - `FavoriteDao::updateFavoriteNode(int id, const QString& name, const QString& iconKey, const QString& colorHex)`：定义于 `FavoriteDao.h`：`static bool updateFavoriteNode(int id, const QString& name, const QString& iconKey, const QString& colorHex);`
   - `FavoriteDao::updateAutoRecursive(int id, bool autoRecursive)`：定义于 `FavoriteDao.h`：`static bool updateAutoRecursive(int id, bool autoRecursive);`
   - `FavoriteDao::updateNodesParentAndOrder(const QList<NodeOrderEntry>& entries)`：定义于 `FavoriteDao.h`：`static bool updateNodesParentAndOrder(const QList<NodeOrderEntry>& entries);`
   - `FavoriteService::instance()`：定义于 `FavoriteService.h` 第 14 行：`static FavoriteService& instance();`

3. **既有 API 检索与防重复造轮子**：
   - `FavoriteService` 现已拥有 `addFavorite`、`removeFavorite`、`removeFavoriteById`、`addVirtualCategory`，但缺少全量读取、节点更新、递归状态更新以及层级排序保存接口，导致 UI 层穿透直接访问 `FavoriteDao`；
   - 本方案将在 `FavoriteService` 中补齐对应下沉封装接口，彻底消除 UI 对 DAO 的越级穿透。

---

## Chapter 1: Overview（概述与根因分析）

### 1. 现象描述与问题定位
`FavoritePanel.cpp` 当前存在严重的多重职责混合（Fat Class / Code Smell）：
1. **视图绘制与容器混杂**：自定义绘制委托 `FavoriteItemDelegate`（包括卡片圆角缩放、高亮文本切片、行内编辑几何修正）与面板容器代码直接耦合在同一文件内；
2. **树形拓扑与拖拽重排逻辑冗长**：`setInternalDropValidator` 与 `onInternalMoveRequested` 占据近 220 行纯算法代码，涉及祖先防环检测、跨层级节点移出插入、状态恢复与展开持久化；
3. **架构分层穿透（DAO 越界直接调用）**：UI 层在 `loadFavorites`、`saveFavorites`、重命名、自动递归切换等多处直接调用底层数据访问对象 `FavoriteDao`，破坏了 `UI -> Service -> DAO` 的分层架构约束。

### 2. 重构设计目标
1. **抽离独立绘制委托**：将 `FavoriteItemDelegate` 剥离至 `src/ui/delegates/FavoriteItemDelegate.h` 与 `.cpp`；
2. **抽取独立拖拽控制器**：创建 `src/ui/controllers/FavoriteTreeDropHandler.h` 与 `.cpp`，负责拖拽合法性校验与树节点拓扑移动；
3. **收拢持久层访问至 Service**：在 `FavoriteService` 补齐 `getAllFavorites()`、`updateFavoriteNode(...)`、`updateAutoRecursive(...)`、`saveFavoritesOrder(...)` 接口，使 `FavoritePanel` 100% 仅与 `FavoriteService` 交互，彻底杜绝直接接触 `FavoriteDao`；返回类型严格对齐物理存在的 `QList<FavoriteRecord>`；
4. **CMake 注册同步**：在 `CMakeLists.txt` 中精准注册新建文件。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/meta/FavoriteService.h`（修改：新增收藏夹节点读取、属性更新、排序保存等下沉接口）
2. `src/meta/FavoriteService.cpp`（修改：实现下沉接口，转发至 `FavoriteDao`）
3. `src/ui/delegates/FavoriteItemDelegate.h`（新建：独立绘制委托头文件）
4. `src/ui/delegates/FavoriteItemDelegate.cpp`（新建：独立绘制委托实现文件）
5. `src/ui/controllers/FavoriteTreeDropHandler.h`（新建：树形拖拽拓扑校验与移动控制器头文件）
6. `src/ui/controllers/FavoriteTreeDropHandler.cpp`（新建：树形拖拽拓扑校验与移动控制器实现文件）
7. `src/ui/FavoritePanel.h`（修改：移除 Delegate 声明，引入外部组件）
8. `src/ui/FavoritePanel.cpp`（修改：移除 Delegate 与 Drop 冗长算法实现，全面切换调用 `FavoriteService`）
9. `CMakeLists.txt`（修改：注册新建的 Delegate 与 DropHandler 文件）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/meta/FavoriteService.h`
新增用于收拢 DAO 调用的公共服务接口声明（返回类型严格对齐物理结构体 `FavoriteRecord`）。

<<<<<<< SEARCH
    bool removeFavorite(const QString& path);
    bool removeFavoriteById(int id);
    bool toggleFavorite(const QString& path);

    /**
     * @brief 统一批量添加至收藏夹并触发全局统一交互反馈 (SSOT)
     */
=======
    bool removeFavorite(const QString& path);
    bool removeFavoriteById(int id);
    bool toggleFavorite(const QString& path);

    /**
     * @brief 获取所有收藏项记录（下沉封装 FavoriteDao）
     */
    QList<FavoriteRecord> getAllFavorites() const;

    /**
     * @brief 更新收藏项节点属性（名称、图标、色彩）
     */
    bool updateFavoriteNode(int id, const QString& name, const QString& iconKey, const QString& colorHex);

    /**
     * @brief 更新文件夹自动递归状态
     */
    bool updateAutoRecursive(int id, bool autoRecursive);

    /**
     * @brief 批量更新收藏项父子层级与自定义排序
     */
    bool saveFavoritesOrder(const QList<NodeOrderEntry>& entries);

    /**
     * @brief 统一批量添加至收藏夹并触发全局统一交互反馈 (SSOT)
     */
>>>>>>> REPLACE

---

### 2. `src/meta/FavoriteService.cpp`
实现新增的 Service 层接口，封装与 DAO 的直接交互。

<<<<<<< SEARCH
bool FavoriteService::removeFavorite(const QString& path) {
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    if (cleanPath.isEmpty() || !FavoriteDao::containsPath(cleanPath)) return false;

    bool ok = FavoriteDao::removeFavorite(cleanPath);
    if (ok) {
        emit favoriteChanged(cleanPath, false);
    }
    return ok;
}
=======
bool FavoriteService::removeFavorite(const QString& path) {
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    if (cleanPath.isEmpty() || !FavoriteDao::containsPath(cleanPath)) return false;

    bool ok = FavoriteDao::removeFavorite(cleanPath);
    if (ok) {
        emit favoriteChanged(cleanPath, false);
    }
    return ok;
}

QList<FavoriteRecord> FavoriteService::getAllFavorites() const {
    return FavoriteDao::getAllFavorites();
}

bool FavoriteService::updateFavoriteNode(int id, const QString& name, const QString& iconKey, const QString& colorHex) {
    return FavoriteDao::updateFavoriteNode(id, name, iconKey, colorHex);
}

bool FavoriteService::updateAutoRecursive(int id, bool autoRecursive) {
    return FavoriteDao::updateAutoRecursive(id, autoRecursive);
}

bool FavoriteService::saveFavoritesOrder(const QList<NodeOrderEntry>& entries) {
    return FavoriteDao::updateNodesParentAndOrder(entries);
}
>>>>>>> REPLACE

---

### 3. `src/ui/delegates/FavoriteItemDelegate.h`
新建独立的绘制委托类头文件。

<<<<<<< SEARCH FILE: src/ui/delegates/FavoriteItemDelegate.h
=======
#pragma once

#include <QStyledItemDelegate>
#include <QString>
#include <QPainter>
#include <QFont>
#include <QColor>
#include <QRect>

namespace QuarkMeta {

class FavoriteItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit FavoriteItemDelegate(QObject* parent = nullptr);
    ~FavoriteItemDelegate() override = default;

    void setFilterKeyword(const QString& kw) { m_filterKeyword = kw; }
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    void paintHighlightedText(QPainter* painter, const QRect& rect, const QString& text, const QString& keyword, const QFont& font, const QColor& normalColor, const QColor& matchColor) const;
    QString m_filterKeyword;
};

} // namespace QuarkMeta
>>>>>>> REPLACE

---

### 4. `src/ui/delegates/FavoriteItemDelegate.cpp`
新建独立的绘制委托类实现文件。

<<<<<<< SEARCH FILE: src/ui/delegates/FavoriteItemDelegate.cpp
=======
#include "FavoriteItemDelegate.h"
#include "../UiHelper.h"
#include <QPainterPath>
#include <QFontMetrics>
#include <QIcon>
#include <QPixmap>

namespace QuarkMeta {

FavoriteItemDelegate::FavoriteItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
}

QSize FavoriteItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    sz.setHeight(28);
    return sz;
}

void FavoriteItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    
    // 背景高亮
    if (opt.state & QStyle::State_Selected) {
        QColor selBg("#378ADD");
        selBg.setAlphaF(0.15f);
        painter->fillRect(opt.rect, selBg);
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }

    // 几何布局（根据树层级深度计算缩进，保证子节点图标与文本排版层次清晰）
    int depth = 0;
    QModelIndex parentIdx = index.parent();
    while (parentIdx.isValid()) {
        depth++;
        parentIdx = parentIdx.parent();
    }

    int leftMargin = 10 + depth * 14;
    int iconSize = 18;
    int spacing = 8;

    QRect iconRect(opt.rect.left() + leftMargin, opt.rect.top() + (opt.rect.height() - iconSize) / 2, iconSize, iconSize);
    QRect textRect(iconRect.right() + spacing, opt.rect.top(), opt.rect.width() - leftMargin - iconSize - spacing, opt.rect.height());

    // 微卡片圆角绘制
    QVariant decoData = index.data(Qt::DecorationRole);
    bool isFolder = index.data(Qt::UserRole + 4).toBool();
    bool hasCustomThumb = index.data(Qt::UserRole + 5).toBool();

    if (!isFolder && hasCustomThumb && decoData.canConvert<QIcon>()) {
        QIcon icon = decoData.value<QIcon>();
        QPixmap pix = icon.pixmap(QSize(64, 64));

        if (!pix.isNull()) {
            QPainterPath clipPath;
            clipPath.addRoundedRect(iconRect, 3, 3);
            painter->save();
            painter->setClipPath(clipPath);

            QPixmap scaled = pix.scaled(iconRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            int x = iconRect.center().x() - scaled.width() / 2;
            int y = iconRect.center().y() - scaled.height() / 2;
            painter->drawPixmap(x, y, scaled);
            painter->restore();
        } else {
            icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }
    } else {
        QIcon icon = decoData.value<QIcon>();
        if (!icon.isNull()) {
            icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }
    }

    // 绘制文本
    QString text = index.data(Qt::DisplayRole).toString();
    QColor normalTextColor = (opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE");
    QColor matchTextColor("#3498db");

    bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
    int rightIconWidth = isAutoRec ? 22 : 0;
    QRect adjustedTextRect = textRect.adjusted(0, 0, -rightIconWidth, 0);

    paintHighlightedText(painter, adjustedTextRect, text, m_filterKeyword, opt.font, normalTextColor, matchTextColor);

    // 绘制开启自动递归右侧 layers 图标与标记 Destination 图标
    bool isMarked = index.data(Qt::UserRole + 10).toBool();
    int rightOffset = 22;
    if (isAutoRec) {
        QRect layersRect(opt.rect.right() - rightOffset, opt.rect.top() + (opt.rect.height() - 16) / 2, 16, 16);
        QIcon layersIcon = UiHelper::getIcon("layers", QColor("#2ecc71"), 16);
        layersIcon.paint(painter, layersRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        rightOffset += 20;
    }

    if (isMarked) {
        QRect markIconRect(opt.rect.right() - rightOffset, opt.rect.top() + (opt.rect.height() - 16) / 2, 16, 16);
        QIcon markIcon = UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 16);
        markIcon.paint(painter, markIconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
    }

    painter->restore();
}

void FavoriteItemDelegate::paintHighlightedText(QPainter* painter, const QRect& rect, const QString& text, const QString& keyword, const QFont& font, const QColor& normalColor, const QColor& matchColor) const {
    if (text.isEmpty()) return;

    QFontMetrics fm(font);
    painter->setFont(font);

    if (keyword.isEmpty()) {
        painter->setPen(normalColor);
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, fm.elidedText(text, Qt::ElideRight, rect.width()));
        return;
    }

    int curX = rect.left();
    int endX = rect.right();
    int lastIdx = 0;
    int kwLen = keyword.length();

    while (lastIdx < text.length() && curX < endX) {
        int foundIdx = text.indexOf(keyword, lastIdx, Qt::CaseInsensitive);
        if (foundIdx == -1) {
            QString remStr = text.mid(lastIdx);
            QString elidedRem = fm.elidedText(remStr, Qt::ElideRight, endX - curX);
            painter->setPen(normalColor);
            painter->drawText(QRect(curX, rect.top(), endX - curX, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, elidedRem);
            break;
        }

        if (foundIdx > lastIdx) {
            QString normalSegment = text.mid(lastIdx, foundIdx - lastIdx);
            int segWidth = fm.horizontalAdvance(normalSegment);
            if (curX + segWidth > endX) {
                QString elidedSeg = fm.elidedText(normalSegment, Qt::ElideRight, endX - curX);
                painter->setPen(normalColor);
                painter->drawText(QRect(curX, rect.top(), endX - curX, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, elidedSeg);
                break;
            }
            painter->setPen(normalColor);
            painter->drawText(QRect(curX, rect.top(), segWidth, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, normalSegment);
            curX += segWidth;
        }

        QString matchSegment = text.mid(foundIdx, kwLen);
        int matchWidth = fm.horizontalAdvance(matchSegment);
        if (curX + matchWidth > endX) {
            QString elidedMatch = fm.elidedText(matchSegment, Qt::ElideRight, endX - curX);
            painter->setPen(matchColor);
            painter->drawText(QRect(curX, rect.top(), endX - curX, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, elidedMatch);
            break;
        }
        painter->setPen(matchColor);
        painter->drawText(QRect(curX, rect.top(), matchWidth, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, matchSegment);
        curX += matchWidth;

        lastIdx = foundIdx + kwLen;
    }
}

void FavoriteItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!editor) return;

    int depth = 0;
    QModelIndex parentIdx = index.parent();
    while (parentIdx.isValid()) {
        depth++;
        parentIdx = parentIdx.parent();
    }

    int leftMargin = 10 + depth * 14;
    int iconSize = 18;
    int spacing = 8;

    int textLeft = option.rect.left() + leftMargin + iconSize + spacing;
    
    bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
    int rightMargin = isAutoRec ? 26 : 8;

    QRect editorRect = option.rect;
    editorRect.setLeft(textLeft - 2);
    editorRect.setRight(option.rect.right() - rightMargin);
    editorRect.setTop(option.rect.top() + 2);
    editorRect.setHeight(option.rect.height() - 4);

    editor->setGeometry(editorRect);
}

} // namespace QuarkMeta
