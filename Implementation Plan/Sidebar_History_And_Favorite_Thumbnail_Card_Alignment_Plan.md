# 侧边栏“历史”与“收藏夹”对标列表视图微型卡片缩略图渲染实施方案 (Sidebar History & Favorite Thumbnail Micro-Card Alignment Plan)

**文档语言**：中文（接口定义与 Git Merge Diff 标记除外）
**基准 Commit**：`42a3950`
**关联规范**：`Memories_QuarkMeta.md` 第 46、47 章节、`Standard Specifications for Implementation Plan.md`

---

## 物理阅读证据清单 (Gate 1: Pre-Draft Physical Evidence)

1. **涉及修改与阅读的物理文件列表**：
   - `QuarkMeta_Source/CMakeLists.txt`（物理核验 CMake 构建列表，确认在 `src/ui/HistoryItemDelegate.h` / `.cpp` 新建时同步注册）
   - `QuarkMeta_Source/src/ui/FavoritePanel.h`（已物理读取第 1~60 行，核验 `FavoriteItemDelegate` 声明）
   - `QuarkMeta_Source/src/ui/FavoritePanel.cpp`（已物理读取第 50~135 行，核验旧缩略图 `clipPath` 与绘制算法，第 94 行精准匹配 `clipPath.addRoundedRect(iconRect, 3, 3);`）
   - `QuarkMeta_Source/src/ui/HistoryItemDelegate.h`（新建独立 Delegate 头文件，严格遵循架构分层与委托解耦规范）
   - `QuarkMeta_Source/src/ui/HistoryItemDelegate.cpp`（新建独立 Delegate 实现文件，对标列表视图规范）
   - `QuarkMeta_Source/src/ui/HistoryPanel.h`（已物理读取第 1~50 行，核验成员变量与接口）
   - `QuarkMeta_Source/src/ui/HistoryPanel.cpp`（已物理读取第 35~110 行与 `eventFilter`，核验双击、空格/回车事件缺失、`ColorPaletteEngine::isGraphicsFile(path)` 传参错误与 `QTreeWidget` 缺少 Delegate 渲染缺陷）
   - `QuarkMeta_Source/src/ui/TreeItemDelegate.h`（已物理读取第 60~135 行，确认列表视图微型卡片 `RowLayoutEngine::calculate`、`scaled` 与 `targetRect` 真实圆角裁剪基准规范）
   - `QuarkMeta_Source/src/util/ColorPaletteEngine.h` 与 `ColorPaletteEngine.cpp`（已物理读取第 16~22 行，确认 `isGraphicsFile` 原生涵盖 `psd/psb/ai/eps/svg` 等所有核心格式，只需补充 `ext == "pdf"`）
   - `QuarkMeta_Source/src/ui/ShellIconManager.h`（已物理读取，确认 `static QIcon getFileIcon(const QString& filePath);`）
   - `QuarkMeta_Source/src/ui/QuickLookWindow.h`（已物理读取，确认 `static QuickLookWindow& instance();` 与 `void preview(const QString& filePath);`）
   - `QuarkMeta_Source/src/core/NavigationService.h`（已物理读取，确认 `void navigateTo(const QString& path, bool recordHistory = true, bool recursive = false);` 与 `void locateItem(const QString& path);`）

2. **外部成员函数原样签名核验**：
   - `ColorPaletteEngine::isGraphicsFile(const QString& ext)`：`QuarkMeta_Source/src/util/ColorPaletteEngine.h:19`
   - `ShellIconManager::getFileIcon(const QString& filePath)`：`QuarkMeta_Source/src/ui/ShellIconManager.h`
   - `QuickLookWindow::instance().preview(const QString& filePath)`：`QuarkMeta_Source/src/ui/QuickLookWindow.h:26`
   - `NavigationService::instance().locateItem(const QString& path)`：`QuarkMeta_Source/src/core/NavigationService.h:35`
   - `NavigationService::instance().navigateTo(const QString& path, bool recordHistory, bool recursive)`：`QuarkMeta_Source/src/core/NavigationService.h:28`
   - `ThumbnailPipelineService::instance().decodePool()`：`QuarkMeta_Source/src/util/ThumbnailPipelineService.h:54`
   - `DiskMediaExtractor::getCapsuleThumbnail(const QString& filePath, int targetSize)`：`QuarkMeta_Source/src/util/DiskMediaExtractor.h`
   - `HistoryService::instance().getAllHistory()`：`QuarkMeta_Source/src/meta/HistoryService.h`

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 缺陷现象与审核整改分析
1. **缺陷一：侧边栏“历史”选项卡无缩略图**
   - **根因 A**：`HistoryPanel::createHistoryItem` 中判定图形文件时，代码错误地写成了 `ColorPaletteEngine::isGraphicsFile(path)`。`isGraphicsFile` 的入参契约是纯文件扩展名（如 `"eps"`、`"png"`），直接传入完整物理路径导致该函数恒定返回 `false`，提取缩略图的代码块根本无法被执行！
   - **根因 B**：`HistoryPanel` 直接使用原生 `QTreeWidget` 且未安装自定义 ItemDelegate，即使提取到了 Pixmap 回填为 `QIcon`，也会被原生树控件的默认 `iconSize (16, 16)` 压扁，且缺乏圆角卡片抗锯齿裁剪管线，导致历史列表全都是静态的小图标。
2. **缺陷二：侧边栏“收藏夹”选项卡缩略图卡片圆角失真（“左上角圆角、其余三角视觉为直角”）**
   - **根因**：在 `FavoriteItemDelegate::paint`（位于 `FavoritePanel.cpp`）中：
     ```cpp
     clipPath.addRoundedRect(iconRect, 3, 3);
     painter->setClipPath(clipPath);
     QPixmap scaled = pix.scaled(iconRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
     int x = iconRect.center().x() - scaled.width() / 2;
     int y = iconRect.center().y() - scaled.height() / 2;
     painter->drawPixmap(x, y, scaled);
     ```
     当缩略图不是绝对正方形时，`KeepAspectRatioByExpanding` 会导致图像内容严重外溢，并在绘制时冲刷破坏反走样边界；更关键的是，裁剪路径并没有精准施加在居中计算出的真实目标矩形 `targetRect` 上，导致四个角的几何反差失衡，视觉上呈现出“左上角是圆角，其余三角却像直角”。
3. **缺陷三：侧边栏“历史”选项卡缺乏与主内容区对齐的即时激活体验（双击与空格/回车预览）**
   - **根因**：`HistoryPanel` 仅处理了单击高亮，既未连接 `itemDoubleClicked`，也未在 `eventFilter` 中拦截 `Key_Space` / `Key_Return` / `Key_Enter`，导致用户无法像在内容面板一样按下空格或双击查看大图。
4. **审核意见闭环整改项**：
   - **消除跨线程野指针（UAF）风险**：彻底移除在后台线程 lambda 中捕获 `QTreeWidgetItem*` 裸指针的做法；对标 `FavoritePanel` 的 SSOT 做法，后台提图只传 `path` 与 `pix`，通过主线程安全回调函数 `updateItemThumbnail(path, pix)`，在主线程安全查找匹配路径的 `QTreeWidgetItem` 并更新，彻底杜绝 UAF 与 Double-Free 风险；
   - **包含完整性闭合**：在 `HistoryPanel.cpp` 中显式包含 `<QPointer>` 与 `<QCoreApplication>`，杜绝符号未定义报错；
   - **架构解耦拒绝打补丁**：将 `HistoryItemDelegate` 彻底独立物理拆分，创建 `src/ui/HistoryItemDelegate.h` 与 `src/ui/HistoryItemDelegate.cpp`，并在 `CMakeLists.txt` 中全量注册，与 `LibraryItemDelegate`、`ThumbnailDelegate` 架构风格完全对齐；
   - **消除冗余判图逻辑**：基于 `ColorPaletteEngine::isGraphicsFile` 已经原生包含 `psd/psb/ai/eps/svg` 的事实，判定条件精炼规范为 `ColorPaletteEngine::isGraphicsFile(ext) || ext == "pdf"`。

### 1.2 重构设计目标（全面对标列表视图 TreeItemDelegate 与模式 A 联动）
1. **统一对齐列表视图（TreeItemDelegate）标准**：
   - 提取缩略图时统一采用 `Qt::KeepAspectRatio` 平滑等比缩放；
   - 居中计算真实的绘制目标矩形 `targetRect`；
   - 将圆角裁剪路径（`QPainterPath::addRoundedRect`，标准 4px 平滑圆角）严格施加在 `targetRect` 之上；
   - 底板保持纯透明，全流程开启 `Antialiasing` 与 `SmoothPixmapTransform`。
2. **独立物理类 HistoryItemDelegate**：
   - 行高统一设定为标准微卡片行高（28px）；
   - 支持高保真平滑缩略图与搜索关键字高亮。
3. **实现模式 A 即刻全功能预览与无缝目录定位（用户明确决策）**：
   - 用户在历史选项卡选中项目后，按下**空格键（Space）**、**回车键（Return/Enter）**或**双击鼠标**：
     - 若为可预览文件：**立即零时延弹出 QuickLook 预览窗口**，同时异步调用 `NavigationService::instance().locateItem(path)` 使工作区内容面板同步定位到该文件；
     - 若为文件夹：直接调用 `NavigationService::instance().navigateTo(path)` 打开。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `QuarkMeta_Source/CMakeLists.txt`（注册新建的 `src/ui/HistoryItemDelegate.h` 与 `src/ui/HistoryItemDelegate.cpp`）
2. `QuarkMeta_Source/src/ui/HistoryItemDelegate.h`（**新建文件**：独立历史记录委托类头文件）
3. `QuarkMeta_Source/src/ui/HistoryItemDelegate.cpp`（**新建文件**：独立历史记录委托类实现文件）
4. `QuarkMeta_Source/src/ui/FavoritePanel.cpp`（修复 `FavoriteItemDelegate::paint` 缩略图卡片居中与四角对称平滑裁剪算法，对标列表视图）
5. `QuarkMeta_Source/src/ui/HistoryPanel.h`（引入 `HistoryItemDelegate` 前置声明，新增 `activateCurrentItem()` 与 `updateItemThumbnail(path, pix)` 声明）
6. `QuarkMeta_Source/src/ui/HistoryPanel.cpp`（装配 `HistoryItemDelegate`，修复扩展名判断，实现主线程安全回填与模式 A 预览联动）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `CMakeLists.txt` 中注册新建的 `HistoryItemDelegate`

```
<<<<<<< SEARCH
    src/ui/HistoryPanel.h
    src/ui/HistoryPanel.cpp
=======
    src/ui/HistoryItemDelegate.h
    src/ui/HistoryItemDelegate.cpp
    src/ui/HistoryPanel.h
    src/ui/HistoryPanel.cpp
>>>>>>> REPLACE
```

---

### 3.2 新建 `QuarkMeta_Source/src/ui/HistoryItemDelegate.h`

```
<<<<<<< SEARCH FILE: src/ui/HistoryItemDelegate.h
=======
#pragma once

#include <QStyledItemDelegate>

namespace QuarkMeta {

/**
 * @brief 历史记录微卡片渲染委托 (严格对标列表视图 TreeItemDelegate / FavoriteItemDelegate 规范)
 */
class HistoryItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit HistoryItemDelegate(QObject* parent = nullptr);
    ~HistoryItemDelegate() override = default;

    void setFilterKeyword(const QString& kw) { m_filterKeyword = kw; }
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    void paintHighlightedText(QPainter* painter, const QRect& rect, const QString& text, const QString& keyword, const QFont& font, const QColor& normalColor, const QColor& matchColor) const;
    QString m_filterKeyword;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

### 3.3 新建 `QuarkMeta_Source/src/ui/HistoryItemDelegate.cpp`

```
<<<<<<< SEARCH FILE: src/ui/HistoryItemDelegate.cpp
=======
#include "HistoryItemDelegate.h"
#include <QPainter>
#include <QPainterPath>
#include <QIcon>
#include <QPixmap>

namespace QuarkMeta {

HistoryItemDelegate::HistoryItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
}

QSize HistoryItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    sz.setHeight(28); // 统一 28px 标准微卡片行高
    return sz;
}

void HistoryItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    // 1. 背景高亮
    if (opt.state & QStyle::State_Selected) {
        QColor selBg("#378ADD");
        selBg.setAlphaF(0.15f);
        painter->fillRect(opt.rect, selBg);
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }

    // 2. 几何布局（严格对标列表视图与收藏夹规范）
    int leftMargin = 10;
    int iconSize = 18;
    int spacing = 8;

    QRect iconRect(opt.rect.left() + leftMargin, opt.rect.top() + (opt.rect.height() - iconSize) / 2, iconSize, iconSize);
    QRect textRect(iconRect.right() + spacing, opt.rect.top(), opt.rect.width() - leftMargin - iconSize - spacing, opt.rect.height());

    // 3. 微卡片圆角绘制（对标列表视图 TreeItemDelegate 标准：Qt::KeepAspectRatio + targetRect 真实四角对称圆角裁剪）
    QVariant decoData = index.data(Qt::DecorationRole);
    bool hasCustomThumb = index.data(Qt::UserRole + 5).toBool();

    if (hasCustomThumb && decoData.canConvert<QIcon>()) {
        QIcon icon = decoData.value<QIcon>();
        QPixmap pix = icon.pixmap(QSize(64, 64));

        if (!pix.isNull()) {
            painter->save();
            QPixmap scaled = pix.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int x = iconRect.center().x() - scaled.width() / 2;
            int y = iconRect.center().y() - scaled.height() / 2;
            QRect targetRect(x, y, scaled.width(), scaled.height());

            QPainterPath clipPath;
            clipPath.addRoundedRect(targetRect, 4, 4);
            painter->setClipPath(clipPath);

            painter->drawPixmap(targetRect, scaled);
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

    // 4. 文本绘制
    QString text = index.data(Qt::DisplayRole).toString();
    QColor normalTextColor = (opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE");
    QColor matchTextColor("#3498db");

    paintHighlightedText(painter, textRect, text, m_filterKeyword, opt.font, normalTextColor, matchTextColor);

    painter->restore();
}

void HistoryItemDelegate::paintHighlightedText(QPainter* painter, const QRect& rect, const QString& text, const QString& keyword, const QFont& font, const QColor& normalColor, const QColor& matchColor) const {
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

    while (curX < endX) {
        int foundIdx = text.indexOf(keyword, lastIdx, Qt::CaseInsensitive);
        if (foundIdx == -1) {
            QString remaining = text.mid(lastIdx);
            QString elidedRemaining = fm.elidedText(remaining, Qt::ElideRight, endX - curX);
            painter->setPen(normalColor);
            painter->drawText(QRect(curX, rect.top(), endX - curX, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, elidedRemaining);
            break;
        }

        if (foundIdx > lastIdx) {
            QString beforeMatch = text.mid(lastIdx, foundIdx - lastIdx);
            int beforeWidth = fm.horizontalAdvance(beforeMatch);
            if (curX + beforeWidth > endX) {
                QString elidedBefore = fm.elidedText(beforeMatch, Qt::ElideRight, endX - curX);
                painter->setPen(normalColor);
                painter->drawText(QRect(curX, rect.top(), endX - curX, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, elidedBefore);
                break;
            }
            painter->setPen(normalColor);
            painter->drawText(QRect(curX, rect.top(), beforeWidth, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, beforeMatch);
            curX += beforeWidth;
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

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

### 3.4 修复 `FavoritePanel.cpp` 中 `FavoriteItemDelegate` 的缩略图圆角裁剪

```
<<<<<<< SEARCH
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
=======
    // 微卡片圆角绘制（严格对标列表视图 TreeItemDelegate 标准：Qt::KeepAspectRatio + targetRect 真实四角对称圆角裁剪）
    QVariant decoData = index.data(Qt::DecorationRole);
    bool isFolder = index.data(Qt::UserRole + 4).toBool();
    bool hasCustomThumb = index.data(Qt::UserRole + 5).toBool();

    if (!isFolder && hasCustomThumb && decoData.canConvert<QIcon>()) {
        QIcon icon = decoData.value<QIcon>();
        QPixmap pix = icon.pixmap(QSize(64, 64));

        if (!pix.isNull()) {
            painter->save();
            // 1. 等比缩放计算目标几何矩形 targetRect
            QPixmap scaled = pix.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int x = iconRect.center().x() - scaled.width() / 2;
            int y = iconRect.center().y() - scaled.height() / 2;
            QRect targetRect(x, y, scaled.width(), scaled.height());

            // 2. 将圆角路径精准施加在 targetRect 上，彻底杜绝外溢冲刷与三角直角现象
            QPainterPath clipPath;
            clipPath.addRoundedRect(targetRect, 4, 4);
            painter->setClipPath(clipPath);

            painter->drawPixmap(targetRect, scaled);
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
>>>>>>> REPLACE
```

---

### 3.5 更新 `HistoryPanel.h` 增加 `HistoryItemDelegate` 前置声明与安全更新接口

```
<<<<<<< SEARCH
#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QMenu>
#include <QIcon>
#include "FavoritePanel.h"

namespace QuarkMeta {

class HistoryPanel : public QWidget {
    Q_OBJECT

public:
    explicit HistoryPanel(QWidget* parent = nullptr);
    ~HistoryPanel() override = default;

    void loadHistory();
    void filterTreeNodes(const QString& keyword);

signals:
    void requestLocateFile(const QString& path);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onContextMenuRequested(const QPoint& pos);
    void onHistoryItemAdded(const QString& path);
    void onHistoryItemRemoved(const QString& path);
    void onHistoryCleared();

private:
    void initUi();
    QTreeWidgetItem* createHistoryItem(const QString& path);
    void updateItemThumbnail(QTreeWidgetItem* item, const QString& path);

    QVBoxLayout* m_mainLayout = nullptr;
    QTreeWidget* m_treeWidget = nullptr;
    bool m_isLoaded = false;
    QString m_lastHoveredPath;
};

} // namespace QuarkMeta
=======
#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QMenu>
#include <QIcon>
#include "FavoritePanel.h"

namespace QuarkMeta {

class HistoryItemDelegate;

class HistoryPanel : public QWidget {
    Q_OBJECT

public:
    explicit HistoryPanel(QWidget* parent = nullptr);
    ~HistoryPanel() override = default;

    void loadHistory();
    void filterTreeNodes(const QString& keyword);
    void activateCurrentItem();

signals:
    void requestLocateFile(const QString& path);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onContextMenuRequested(const QPoint& pos);
    void onHistoryItemAdded(const QString& path);
    void onHistoryItemRemoved(const QString& path);
    void onHistoryCleared();

private:
    void initUi();
    QTreeWidgetItem* createHistoryItem(const QString& path);
    void requestThumbnail(const QString& path);
    void updateItemThumbnail(const QString& path, const QPixmap& pix);

    QVBoxLayout* m_mainLayout = nullptr;
    QTreeWidget* m_treeWidget = nullptr;
    HistoryItemDelegate* m_delegate = nullptr;
    bool m_isLoaded = false;
    QString m_lastHoveredPath;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

### 3.6 重构 `HistoryPanel.cpp` 引入安全更新、模式 A 即刻全功能预览与头文件闭合

```
<<<<<<< SEARCH
#include "HistoryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "controllers/ContextMenuFactory.h"
#include "../core/NavigationService.h"
#include "../core/ClipboardService.h"
#include "../meta/FavoriteService.h"
#include "../meta/HistoryService.h"
#include "../meta/LibraryService.h"
#include "../util/ThumbnailPipelineService.h"
#include "../util/ColorPaletteEngine.h"
#include "../util/ShellHelper.h"
#include "dialogs/FramelessConfirmDialog.h"
#include <QDesktopServices>
#include <QFileInfo>
#include <QDir>
#include <QFileIconProvider>
#include <QDrag>
#include <QMimeData>
#include <QUrl>
#include <QMouseEvent>
#include <QHeaderView>

namespace QuarkMeta {

HistoryPanel::HistoryPanel(QWidget* parent) : QWidget(parent) {
    initUi();

    connect(&HistoryService::instance(), &HistoryService::historyItemAdded, this, &HistoryPanel::onHistoryItemAdded);
    connect(&HistoryService::instance(), &HistoryService::historyItemRemoved, this, &HistoryPanel::onHistoryItemRemoved);
    connect(&HistoryService::instance(), &HistoryService::historyCleared, this, &HistoryPanel::onHistoryCleared);
}

void HistoryPanel::initUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setObjectName("HistoryTreeWidget");
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(0);
    m_treeWidget->setIconSize(QSize(16, 16));
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeWidget->setDragEnabled(true);
    m_treeWidget->setMouseTracking(true);

    m_mainLayout->addWidget(m_treeWidget);

    connect(m_treeWidget, &QTreeWidget::itemClicked, this, &HistoryPanel::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &HistoryPanel::onContextMenuRequested);

    if (m_treeWidget->viewport()) {
        m_treeWidget->viewport()->installEventFilter(this);
    }
}

void HistoryPanel::loadHistory() {
    if (m_isLoaded) return;
    m_isLoaded = true;

    m_treeWidget->clear();
    auto list = HistoryService::instance().getAllHistory();
    for (const auto& rec : list) {
        if (QFile::exists(rec.filePath)) {
            QTreeWidgetItem* item = createHistoryItem(rec.filePath);
            m_treeWidget->addTopLevelItem(item);
        }
    }
}

QTreeWidgetItem* HistoryPanel::createHistoryItem(const QString& path) {
    QFileInfo fi(path);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, fi.fileName());
    item->setData(0, Qt::UserRole, path);

    // 普通文件优先展示系统/通用图标
    QFileIconProvider provider;
    QIcon sysIcon = provider.icon(fi);
    if (!sysIcon.isNull()) {
        item->setIcon(0, sysIcon);
    } else {
        item->setIcon(0, UiHelper::getIcon("file", "#888888", 16));
    }

    // 图形图片类文件异步提取缩略图回填
    if (ColorPaletteEngine::isGraphicsFile(path)) {
        updateItemThumbnail(item, path);
    }

    return item;
}

void HistoryPanel::updateItemThumbnail(QTreeWidgetItem* item, const QString& path) {
    ThumbnailPipelineService::instance().loadBatchAsync({path}, 32, [this, item](const QString&, const QPixmap& pix) {
        if (!pix.isNull() && item && item->treeWidget()) {
            item->setIcon(0, QIcon(pix));
        }
    });
}

void HistoryPanel::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(item);
    Q_UNUSED(column);
    // 单击仅由 QTreeWidget 处理选中高亮，不发射定位信号以彻底杜绝额外目录载入与缩略图解析开销
}
=======
#include "HistoryPanel.h"
#include "HistoryItemDelegate.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "QuickLookWindow.h"
#include "ToolTipOverlay.h"
#include "controllers/ContextMenuFactory.h"
#include "../core/NavigationService.h"
#include "../core/ClipboardService.h"
#include "../meta/FavoriteService.h"
#include "../meta/HistoryService.h"
#include "../meta/LibraryService.h"
#include "../util/ThumbnailPipelineService.h"
#include "../util/DiskMediaExtractor.h"
#include "../util/ColorPaletteEngine.h"
#include "../util/ShellHelper.h"
#include "dialogs/FramelessConfirmDialog.h"
#include <QDesktopServices>
#include <QFileInfo>
#include <QDir>
#include <QDrag>
#include <QMimeData>
#include <QUrl>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QHeaderView>
#include <QPointer>
#include <QCoreApplication>
#include <QtConcurrent>

namespace QuarkMeta {

// 角色契约：
// Qt::UserRole: 物理文件路径
// Qt::UserRole + 1: 物理文件路径 (与 FavoritePanel 对齐)
// Qt::UserRole + 5: 是否已被缩略图回填 (bool)

HistoryPanel::HistoryPanel(QWidget* parent) : QWidget(parent) {
    initUi();

    connect(&HistoryService::instance(), &HistoryService::historyItemAdded, this, &HistoryPanel::onHistoryItemAdded);
    connect(&HistoryService::instance(), &HistoryService::historyItemRemoved, this, &HistoryPanel::onHistoryItemRemoved);
    connect(&HistoryService::instance(), &HistoryService::historyCleared, this, &HistoryPanel::onHistoryCleared);
}

void HistoryPanel::initUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setObjectName("HistoryTreeWidget");
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(0);
    m_treeWidget->setIconSize(QSize(18, 18));
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeWidget->setDragEnabled(true);
    m_treeWidget->setMouseTracking(true);

    m_delegate = new HistoryItemDelegate(this);
    m_treeWidget->setItemDelegate(m_delegate);

    m_mainLayout->addWidget(m_treeWidget);

    connect(m_treeWidget, &QTreeWidget::itemClicked, this, &HistoryPanel::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &HistoryPanel::onItemDoubleClicked);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &HistoryPanel::onContextMenuRequested);

    m_treeWidget->installEventFilter(this);
    if (m_treeWidget->viewport()) {
        m_treeWidget->viewport()->installEventFilter(this);
    }
}

void HistoryPanel::loadHistory() {
    if (m_isLoaded) return;
    m_isLoaded = true;

    m_treeWidget->clear();
    auto list = HistoryService::instance().getAllHistory();
    for (const auto& rec : list) {
        if (QFile::exists(rec.filePath)) {
            QTreeWidgetItem* item = createHistoryItem(rec.filePath);
            m_treeWidget->addTopLevelItem(item);
        }
    }
}

QTreeWidgetItem* HistoryPanel::createHistoryItem(const QString& path) {
    QFileInfo fi(path);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, fi.fileName());
    item->setData(0, Qt::UserRole, path);
    item->setData(0, Qt::UserRole + 1, path);
    item->setData(0, Qt::UserRole + 5, false);

    // 普通文件优先展示 Shell 高保真系统图标
    QIcon sysIcon = ShellIconManager::getFileIcon(path);
    if (!sysIcon.isNull()) {
        item->setIcon(0, sysIcon);
    } else {
        item->setIcon(0, UiHelper::getIcon("file", "#888888", 18));
    }

    // 图形图片类文件异步提取缩略图回填（传入纯扩展名，包含原生集合与 PDF）
    QString ext = fi.suffix().toLower();
    if (ColorPaletteEngine::isGraphicsFile(ext) || ext == "pdf") {
        requestThumbnail(path);
    }

    return item;
}

void HistoryPanel::requestThumbnail(const QString& path) {
    QPointer<HistoryPanel> weakThis(this);
    (void)QtConcurrent::run(ThumbnailPipelineService::instance().decodePool(), [weakThis, path]() {
        if (!weakThis) return;
        QImage img = DiskMediaExtractor::getCapsuleThumbnail(path, DiskMediaExtractor::kThumbSize);
        if (!img.isNull()) {
            if (img.width() > 128 || img.height() > 128) {
                img = img.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            QPixmap pix = QPixmap::fromImage(img);
            QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, path, pix]() {
                if (weakThis) {
                    weakThis->updateItemThumbnail(path, pix);
                }
            }, Qt::QueuedConnection);
        }
    });
}

void HistoryPanel::updateItemThumbnail(const QString& path, const QPixmap& pix) {
    if (!m_treeWidget || pix.isNull()) return;

    QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(path));
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* itm = m_treeWidget->topLevelItem(i);
        if (!itm) continue;
        QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(itm->data(0, Qt::UserRole).toString()));
        if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0) {
            itm->setIcon(0, QIcon(pix));
            itm->setData(0, Qt::UserRole + 5, true);
            if (m_treeWidget->viewport()) {
                m_treeWidget->viewport()->update();
            }
            break;
        }
    }
}

void HistoryPanel::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(item);
    Q_UNUSED(column);
    // 单击仅由 QTreeWidget 处理选中高亮，不发射定位信号以彻底杜绝额外目录载入与缩略图解析开销
}

void HistoryPanel::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    activateCurrentItem();
}

void HistoryPanel::activateCurrentItem() {
    QTreeWidgetItem* curItem = m_treeWidget ? m_treeWidget->currentItem() : nullptr;
    if (!curItem) return;

    QString path = curItem->data(0, Qt::UserRole).toString();
    if (path.isEmpty() || !QFile::exists(path)) return;

    QFileInfo fi(path);
    if (fi.isDir()) {
        NavigationService::instance().navigateTo(path, true, false);
    } else {
        // 模式 A 契约：即刻零延迟弹出 QuickLook 预览，同时异步联动内容面板精准定位
        if (UiHelper::canPreviewFile(path)) {
            QuickLookWindow::instance().preview(path);
        }
        NavigationService::instance().locateItem(path);
    }
}
>>>>>>> REPLACE
```

---

### 3.7 拦截空格键与回车键并更新过滤高亮

```
<<<<<<< SEARCH
void HistoryPanel::filterTreeNodes(const QString& keyword) {
    QString kw = keyword.trimmed().toLower();
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
        if (kw.isEmpty()) {
            item->setHidden(false);
        } else {
            bool match = item->text(0).toLower().contains(kw) || item->data(0, Qt::UserRole).toString().toLower().contains(kw);
            item->setHidden(!match);
        }
    }
}

bool HistoryPanel::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_treeWidget->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
=======
void HistoryPanel::filterTreeNodes(const QString& keyword) {
    if (m_delegate) {
        m_delegate->setFilterKeyword(keyword.trimmed());
    }

    QString kw = keyword.trimmed().toLower();
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
        if (kw.isEmpty()) {
            item->setHidden(false);
        } else {
            bool match = item->text(0).toLower().contains(kw) || item->data(0, Qt::UserRole).toString().toLower().contains(kw);
            item->setHidden(!match);
        }
    }

    if (m_treeWidget->viewport()) {
        m_treeWidget->viewport()->update();
    }
}

bool HistoryPanel::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_treeWidget) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            // 模式 A 契约：未带 Ctrl 的空格键或回车键直接触发激活
            if ((keyEvent->key() == Qt::Key_Space && !(keyEvent->modifiers() & Qt::ControlModifier)) ||
                keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                activateCurrentItem();
                return true;
            }
        }
    }

    if (watched == m_treeWidget->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译指令
```bash
cd QuarkMeta_Source
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 4.2 运行时验收标准
1. **历史选项卡缩略图正常显现**：打开左侧边栏“历史”选项卡，对于曾经打开过的 `eps`、`ai`、`psd`、`png`、`jpg` 等图形设计图档，不再是单一的格式图标，全部正确显示出彩色缩略图！
2. **微卡片圆角平滑对称**：无论是“收藏夹”还是“历史”选项卡，缩略图左上、右上、左下、右下四个角均为标准的 4px 平滑圆角，彻底消除左上角圆角、其余三角为直角的视觉缺陷。
3. **模式 A 即刻全功能预览与定位同步**：
   - 选中历史条目按下空格键（Space）或回车键（Return）：瞬间弹出 QuickLook 高保真大图预览，同时中间主内容面板无缝定位到该图档；
   - 鼠标双击历史条目：同样瞬间弹出 QuickLook 高保真大图预览并无缝定位；
   - 文件夹条目：回车/双击直接导航进入该文件夹。
4. **性能与内存生命周期安全**：
   - 历史记录滚动极其流畅，后台提图完成后主线程安全寻址回填，即使在后台提图过程中清空历史也绝无崩溃闪退（Zero UAF）。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. 复用 `ShellIconManager::getFileIcon(path)` 获取高保真操作系统文件图标；
2. 复用 `ColorPaletteEngine::isGraphicsFile(ext)` 正确传入 `suffix` 判定图片；
3. 复用 `ThumbnailPipelineService::instance().decodePool()` 与 `DiskMediaExtractor::getCapsuleThumbnail`，与收藏夹复用同一条高性能磁盘缓存降采样解码流水线；
4. 复用 `QuickLookWindow::instance().preview(path)` 与 `NavigationService::instance().locateItem(path)`，严禁另起炉灶。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 唯一数据源 | 派生展示副本 | 刷新机制与方向 |
| :--- | :--- | :--- | :--- |
| 历史记录条目 | `HistoryService` 内存与 SQLite 记录 | `HistoryPanel::m_treeWidget` Item | 监听 `historyItemAdded` / `Removed` / `Cleared` 信号驱动刷新 |
| 历史缩略图缓存 | `DiskMediaExtractor` 磁盘缓存与内存池 | `QTreeWidgetItem` DecorationRole & `UserRole + 5` | 后台线程池解码完成后 `QMetaObject::invokeMethod` 调度回主线程安全按路径更新 |
| 预览与定位调度 | `QuickLookWindow` 与 `NavigationService` | `HistoryPanel::activateCurrentItem()` | 单击双击/空格驱动分发调用 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部调用接口保持**：
   - `HistoryPanel::loadHistory()` (public)
   - `HistoryPanel::filterTreeNodes(const QString&)` (public)
   - 信号 `requestLocateFile(const QString&)` (signals)
   - 签名与访问级别 100% 保持，不破坏任何上层调用契约。
2. **新增类与成员一致性**：
   - `HistoryItemDelegate` 声明在独立的 `HistoryItemDelegate.h` 中，在 `HistoryItemDelegate.cpp` 中实现，全工程统一解耦；
   - `HistoryPanel::activateCurrentItem()` 声明为 public，定义在 `HistoryPanel.cpp`，内部安全核验 `currentItem()` 指针；
   - `HistoryPanel::updateItemThumbnail(const QString& path, const QPixmap& pix)` 声明为 private，在主线程安全匹配路径更新，杜绝跨线程传递裸指针。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `HistoryItemDelegate.h` 显式包含 `<QStyledItemDelegate>`；
2. `HistoryItemDelegate.cpp` 显式包含 `HistoryItemDelegate.h`、`<QPainter>`、`<QPainterPath>`、`<QIcon>`、`<QPixmap>`；
3. `HistoryPanel.h` 前置声明 `class HistoryItemDelegate;`；
4. `HistoryPanel.cpp` 显式包含 `HistoryItemDelegate.h`、`<QPointer>`、`<QCoreApplication>`、`<QKeyEvent>`、`<QtConcurrent>`、`ShellIconManager.h`、`QuickLookWindow.h`、`DiskMediaExtractor.h`，所有使用到的类型均显式包含定义，绝对闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 结果 | 证据与位置 |
| :--- | :--- | :--- | :--- |
| 1 | 是否物理隔离且未直接修改业务源代码 | 满足 | 方案独立位于 `Implementation Plan/` 目录下 |
| 2 | SEARCH 块是否在目标文件中真实存在且唯一匹配 | 满足 | 经过实际源码比对，精确匹配唯一位置 |
| 3 | 是否包含全部 10 大必备章节 | 满足 | 第 1 至第 10 章节完整具备 |
| 4 | 是否杜绝在后台线程直接操作 QWidget | 满足 | 采用 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 调度回主线程 |
| 5 | 是否消除了四角计算失真缺陷 | 满足 | 第 3.4 节与 `HistoryItemDelegate` 统一采用 `KeepAspectRatio` + `targetRect` 圆角裁剪 |
| 6 | 是否支持模式 A 预览与定位联动 | 满足 | 第 3.5 节、第 3.6 节与第 4.2 节全面闭环实现 |
| 7 | 是否杜绝跨线程 Lambda 捕获裸指针（防 UAF） | 满足 | 第 3.6 节改用按 `path` 字符串传递并在主线程安全检索更新 |
| 8 | 是否在 CMakeLists.txt 中注册新建文件 | 满足 | 第 3.1 节在 `CMakeLists.txt` 中完整注册新建的 `.h` 与 `.cpp` |
| 9 | 头文件包含链是否显式包含 QPointer 与 QCoreApplication | 满足 | 第 3.6 节头文件中已显式增加相应包含 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

1. **交互行为 100% 保持**：历史面板的右键上下文菜单（用默认程序打开、在资源管理器显示、在 QuarkMeta 显示、添加收藏等 12 项菜单）、拖拽行为、悬停 Toast 路径提示 100% 保持不变。
2. **数值标准统一**：行高统一对标 28px，图标尺寸对标 18px，圆角曲率半径统一为 4px。
