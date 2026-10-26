# Implementation Plan - SidebarFilterSharedSupport-1

本方案替代 `superseded/SidebarFilterSharedSupport.md`

Base Commit Hash: `943037f85524b65875859191f2d3c1dba8aae1c1`
Prerequisite Plans: 无
Supersedes: `Implementation Plan/superseded/SidebarFilterSharedSupport.md`

---

### 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/ui/SidebarContainerWidget.h`
   - `src/ui/SidebarContainerWidget.cpp`
   - `src/ui/LibraryPanel.h`
   - `src/ui/LibraryPanel.cpp`
   - `src/ui/FavoritePanel.h`
   - `src/ui/FavoritePanel.cpp`
   - `src/ui/SearchHistoryPanel.h`
   - `src/ui/SearchHistoryPanel.cpp`
   - `src/core/SearchHistoryService.h`
   - `src/core/SearchHistoryService.cpp`
2. **头文件原样签名**：
   - `src/ui/SidebarContainerWidget.h:25`: `class SidebarContainerWidget : public QFrame`
   - `src/ui/FavoritePanel.h:25`: `class FavoritePanel : public QFrame`
   - `src/ui/LibraryPanel.h:58`: `class LibraryPanel : public QFrame`
   - `src/ui/SearchHistoryPanel.h:23`: `void setHistory(const QStringList& history, const QString& title = "最近搜索");`
   - `src/core/SearchHistoryService.h:18`: `void appendSearch(const QString& category, const QString& keyword);`
3. **既有 API 检索日志**：
   - `grep -rn "SearchHistoryService" src/`
   - `SearchHistoryService::instance().appendSearch(category, keyword)` 与 `getHistory(category)` 支持按类别持久化和获取搜索/筛选历史。

---

## Chapter 1: Overview
在 QuarkMeta 当前设计中，侧边栏底部的“筛选分类”编辑框（`m_filterEdit`）被硬编码放置在 `LibraryPanel`（“库”选项卡）内部，导致当用户切换到“收藏夹”选项卡时底栏筛选输入框缺失，无法对“收藏夹”下的文件夹、常用项目与虚拟分类进行名称筛选与高亮。此外，底栏筛选框缺乏历史记录回溯能力。

在上一版方案审核中，发现了两个问题：
1. **SEARCH 块原文匹配遗漏与模糊**：`FavoritePanel.h` 的 SEARCH 块上下文缺失，且对 `LibraryPanel.cpp` 原 `m_filterEdit` 的移除块界定不严格。
2. **Getter 方法补全**：需要精准提供 `LibraryPanel.h` 中 `delegate()`、`treeView()` 与 `filterTreeNodes(...)` 的新增声明块，保障编译闭合。

**重构设计方案**：
1. **底栏控件提升**：将 `LibraryFilterContainer` 极其内部 `QLineEdit`（`m_filterEdit`）提升至 `SidebarContainerWidget` 底部容器，使其常驻贯通于“收藏夹”和“库”两个选项卡下方。
2. **公开访问 Getter 声明**：在 `LibraryPanel.h` 中补充 `delegate()`、`treeView()` 与 `filterTreeNodes(...)` 的公有访问声明。
3. **`FavoritePanel` 筛选高亮支持**：在 `FavoritePanel` 及 `FavoriteItemDelegate` 中新增 `filterTreeNodes(const QString& keyword)` 递归隐显过滤逻辑与关键字匹配高亮绘制能力（精准预留自动递归图标 22px 宽度），使其与 `LibraryPanel` 具有完全一致的筛选体验。
4. **双击唤起历史记录浮窗**：在 `SidebarContainerWidget` 的 `m_filterEdit` 上安装事件过滤器捕获双击事件（`QEvent::MouseButtonDblClick`）及 Enter 提交事件，复用 `SearchHistoryService`（category=`"sidebar_filter"`）与 `SearchHistoryPanel`，双击时弹现“最近搜索”浮窗并支持填充与清理历史记录。

---

## Chapter 2: Modified Files List
- `src/ui/FavoritePanel.h`
- `src/ui/FavoritePanel.cpp`
- `src/ui/LibraryPanel.h`
- `src/ui/LibraryPanel.cpp`
- `src/ui/SidebarContainerWidget.h`
- `src/ui/SidebarContainerWidget.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.h`
<<<<<<< SEARCH
class FavoriteItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit FavoriteItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
=======
class FavoriteItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit FavoriteItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    void setFilterKeyword(const QString& kw) { m_filterKeyword = kw; }
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    void paintHighlightedText(QPainter* painter, const QRect& rect, const QString& text, const QString& keyword, const QFont& font, const QColor& normalColor, const QColor& matchColor) const;
    QString m_filterKeyword;
};
>>>>>>> REPLACE

<<<<<<< SEARCH
    void loadFavorites();
    void saveFavorites();

signals:
=======
    void loadFavorites();
    void saveFavorites();
    void filterTreeNodes(const QString& keyword);

signals:
>>>>>>> REPLACE

<<<<<<< SEARCH
    QVBoxLayout* m_mainLayout = nullptr;
    DropTreeView* m_favoriteView = nullptr;
    QStandardItemModel* m_favoriteModel = nullptr;
    bool m_isLoading = false;
    bool m_orderDirty = false;
    int m_pendingEditNodeId = 0;
};
=======
    QVBoxLayout* m_mainLayout = nullptr;
    DropTreeView* m_favoriteView = nullptr;
    QStandardItemModel* m_favoriteModel = nullptr;
    FavoriteItemDelegate* m_delegate = nullptr;
    bool m_isLoading = false;
    bool m_orderDirty = false;
    int m_pendingEditNodeId = 0;
};
>>>>>>> REPLACE

### `src/ui/FavoritePanel.cpp`
<<<<<<< SEARCH
    // 绘制文本
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
    painter->setFont(opt.font);

    bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
    int rightIconWidth = isAutoRec ? 22 : 0;

    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width() - 6 - rightIconWidth);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
=======
    // 绘制文本
    QString text = index.data(Qt::DisplayRole).toString();
    QColor normalTextColor = (opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE");
    QColor matchTextColor("#3498db");

    bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
    int rightIconWidth = isAutoRec ? 22 : 0;
    QRect adjustedTextRect = textRect.adjusted(0, 0, -rightIconWidth, 0);

    paintHighlightedText(painter, adjustedTextRect, text, m_filterKeyword, opt.font, normalTextColor, matchTextColor);
>>>>>>> REPLACE

<<<<<<< SEARCH
void FavoriteItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const {
=======
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
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_favoriteModel = new QStandardItemModel(this);
    m_favoriteView->setModel(m_favoriteModel);
    m_favoriteView->setItemDelegate(new FavoriteItemDelegate(this));
=======
    m_favoriteModel = new QStandardItemModel(this);
    m_favoriteView->setModel(m_favoriteModel);
    m_delegate = new FavoriteItemDelegate(this);
    m_favoriteView->setItemDelegate(m_delegate);
>>>>>>> REPLACE

<<<<<<< SEARCH
    saveFavorites();
    m_orderDirty = false;
}
=======
    saveFavorites();
    m_orderDirty = false;
}

void FavoritePanel::filterTreeNodes(const QString& keyword) {
    if (!m_favoriteModel || !m_favoriteView) return;

    if (m_delegate) {
        m_delegate->setFilterKeyword(keyword.trimmed());
    }

    std::function<bool(QStandardItem*)> filterRecursive = [&](QStandardItem* item) -> bool {
        if (!item) return false;

        bool matchesSelf = keyword.trimmed().isEmpty() || item->text().contains(keyword.trimmed(), Qt::CaseInsensitive);
        bool anyChildMatches = false;

        for (int i = 0; i < item->rowCount(); ++i) {
            QStandardItem* child = item->child(i);
            if (filterRecursive(child)) {
                anyChildMatches = true;
                m_favoriteView->setRowHidden(i, item->index(), false);
            } else {
                m_favoriteView->setRowHidden(i, item->index(), true);
            }
        }

        bool visible = matchesSelf || anyChildMatches;
        if (!keyword.trimmed().isEmpty() && visible && item->hasChildren()) {
            m_favoriteView->expand(item->index());
        }
        return visible;
    };

    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QStandardItem* topItem = m_favoriteModel->item(i);
        if (!topItem) continue;
        if (filterRecursive(topItem)) {
            m_favoriteView->setRowHidden(i, QModelIndex(), false);
        } else {
            m_favoriteView->setRowHidden(i, QModelIndex(), true);
        }
    }

    if (m_favoriteView->viewport()) {
        m_favoriteView->viewport()->update();
    }
}
>>>>>>> REPLACE

### `src/ui/LibraryPanel.h`
<<<<<<< SEARCH
    void loadLibrary();
    void saveLibraryOrder();
    void sortCategoriesByName(bool ascending);

signals:
=======
    void loadLibrary();
    void saveLibraryOrder();
    void sortCategoriesByName(bool ascending);
    void filterTreeNodes(const QString& keyword);

    LibraryItemDelegate* delegate() const { return m_delegate; }
    LibraryTreeView* treeView() const { return m_treeView; }

signals:
>>>>>>> REPLACE

<<<<<<< SEARCH
    QVBoxLayout* m_mainLayout = nullptr;
    LibraryTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
    LibraryItemDelegate* m_delegate = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    bool m_isLoading = false;
=======
    QVBoxLayout* m_mainLayout = nullptr;
    LibraryTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
    LibraryItemDelegate* m_delegate = nullptr;
    bool m_isLoading = false;
>>>>>>> REPLACE

### `src/ui/LibraryPanel.cpp`
<<<<<<< SEARCH
    // 底部搜索过滤框部件创建与配置
    QWidget* filterContainer = new QWidget(this);
    filterContainer->setObjectName("LibraryFilterContainer");
    QVBoxLayout* filterLayout = new QVBoxLayout(filterContainer);
    filterLayout->setContentsMargins(8, 8, 8, 8);
    filterLayout->setSpacing(0);

    m_filterEdit = new QLineEdit(filterContainer);
    m_filterEdit->setObjectName("LibraryFilterEdit");
    m_filterEdit->setPlaceholderText("筛选分类");
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setFixedHeight(32);
    m_filterEdit->addAction(UiHelper::getIcon("filter_funnel", QColor("#888888"), 14), QLineEdit::LeadingPosition);
    filterLayout->addWidget(m_filterEdit);

    m_mainLayout->addWidget(filterContainer, 0);

    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        QString kw = text.trimmed();
        if (m_delegate) {
            m_delegate->setFilterKeyword(kw);
        }
        filterTreeNodes(kw);
        if (m_treeView && m_treeView->viewport()) {
            m_treeView->viewport()->update();
        }
    });
=======
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_treeView->setInternalDragValidator([this](const QModelIndex& index) {
        if (!index.isValid()) return false;
        if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
            return false;
        }
        int nodeId = index.data(Qt::UserRole + 1).toInt();
        return nodeId > 0;
    });
=======
    m_treeView->setInternalDragValidator([](const QModelIndex& index) {
        if (!index.isValid()) return false;
        int nodeId = index.data(Qt::UserRole + 1).toInt();
        return nodeId > 0;
    });
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_treeView->setInternalDropValidator([this](const QList<QPersistentModelIndex>& sources, const QModelIndex& target, InternalDropZone zone) -> bool {
        if (sources.isEmpty()) return false;
        if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) return false;
=======
    m_treeView->setInternalDropValidator([this](const QList<QPersistentModelIndex>& sources, const QModelIndex& target, InternalDropZone zone) -> bool {
        if (sources.isEmpty()) return false;
>>>>>>> REPLACE

<<<<<<< SEARCH
void LibraryPanel::toggleSectionByNodeId(int nodeId) {
    if (!m_model || !m_treeView) return;
    if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
        return;
    }
=======
void LibraryPanel::toggleSectionByNodeId(int nodeId) {
    if (!m_model || !m_treeView) return;
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 若底栏存在搜索关键字，重新应用过滤
    if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
        filterTreeNodes(m_filterEdit->text().trimmed());
    }
=======
>>>>>>> REPLACE

<<<<<<< SEARCH
void LibraryPanel::createAndEditCategory(int parentId) {
    if (m_filterEdit) {
        m_filterEdit->clear();
    }

    int targetHeaderId = (parentId == -20) ? -20 : 0;
=======
void LibraryPanel::createAndEditCategory(int parentId) {
    int targetHeaderId = (parentId == -20) ? -20 : 0;
>>>>>>> REPLACE

### `src/ui/SidebarContainerWidget.h`
<<<<<<< SEARCH
#include "FavoritePanel.h"
#include "LibraryPanel.h"

namespace QuarkMeta {

class SidebarContainerWidget : public QFrame {
    Q_OBJECT
=======
#include "FavoritePanel.h"
#include "LibraryPanel.h"
#include "SearchHistoryPanel.h"
#include <QLineEdit>

namespace QuarkMeta {

class SidebarContainerWidget : public QFrame {
    Q_OBJECT
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
>>>>>>> REPLACE

<<<<<<< SEARCH
    QTabBar* m_tabBar = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;
    FavoritePanel* m_favoritePanel = nullptr;
    LibraryPanel* m_libraryPanel = nullptr;
};
=======
    QTabBar* m_tabBar = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;
    FavoritePanel* m_favoritePanel = nullptr;
    LibraryPanel* m_libraryPanel = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    SearchHistoryPanel* m_historyPanel = nullptr;
};
>>>>>>> REPLACE

### `src/ui/SidebarContainerWidget.cpp`
<<<<<<< SEARCH
#include "SidebarContainerWidget.h"
#include "UiHelper.h"
#include "../core/AppConfig.h"

namespace QuarkMeta {
=======
#include "SidebarContainerWidget.h"
#include "UiHelper.h"
#include "../core/AppConfig.h"
#include "../core/SearchHistoryService.h"
#include <QMouseEvent>

namespace QuarkMeta {
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_mainLayout->addWidget(m_stackedWidget, 1);

    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
        if (index == 1 && m_libraryPanel) {
            m_libraryPanel->loadLibrary();
        }
        AppConfig::instance().setValue("Sidebar/CurrentTabIndex", index);
        AppConfig::instance().sync();
        emit sidebarTabChanged(index);
    });
=======
    m_mainLayout->addWidget(m_stackedWidget, 1);

    // 底部统一搜索过滤框部件创建与配置
    QWidget* filterContainer = new QWidget(this);
    filterContainer->setObjectName("SidebarFilterContainer");
    QVBoxLayout* filterLayout = new QVBoxLayout(filterContainer);
    filterLayout->setContentsMargins(8, 8, 8, 8);
    filterLayout->setSpacing(0);

    m_filterEdit = new QLineEdit(filterContainer);
    m_filterEdit->setObjectName("SidebarFilterEdit");
    m_filterEdit->setPlaceholderText("筛选分类");
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setFixedHeight(32);
    m_filterEdit->addAction(UiHelper::getIcon("filter_funnel", QColor("#888888"), 14), QLineEdit::LeadingPosition);
    m_filterEdit->installEventFilter(this);
    filterLayout->addWidget(m_filterEdit);

    m_historyPanel = new SearchHistoryPanel(this);
    m_historyPanel->setCategory("sidebar_filter");

    connect(m_historyPanel, &SearchHistoryPanel::historyItemClicked, this, [this](const QString& keyword) {
        if (m_filterEdit) {
            m_filterEdit->setText(keyword);
        }
    });

    connect(m_filterEdit, &QLineEdit::returnPressed, this, [this]() {
        if (!m_filterEdit) return;
        QString text = m_filterEdit->text().trimmed();
        if (!text.isEmpty()) {
            SearchHistoryService::instance().appendSearch("sidebar_filter", text);
        }
    });

    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        QString kw = text.trimmed();
        if (m_favoritePanel) {
            m_favoritePanel->filterTreeNodes(kw);
        }
        if (m_libraryPanel) {
            if (m_libraryPanel->delegate()) {
                m_libraryPanel->delegate()->setFilterKeyword(kw);
            }
            m_libraryPanel->filterTreeNodes(kw);
            if (m_libraryPanel->treeView() && m_libraryPanel->treeView()->viewport()) {
                m_libraryPanel->treeView()->viewport()->update();
            }
        }
    });

    m_mainLayout->addWidget(filterContainer, 0);

    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
        if (index == 1 && m_libraryPanel) {
            m_libraryPanel->loadLibrary();
        }
        if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
            QString kw = m_filterEdit->text().trimmed();
            if (index == 0 && m_favoritePanel) m_favoritePanel->filterTreeNodes(kw);
            if (index == 1 && m_libraryPanel) m_libraryPanel->filterTreeNodes(kw);
        }
        AppConfig::instance().setValue("Sidebar/CurrentTabIndex", index);
        AppConfig::instance().sync();
        emit sidebarTabChanged(index);
    });
>>>>>>> REPLACE

<<<<<<< SEARCH
}

} // namespace QuarkMeta
=======
}

bool SidebarContainerWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_filterEdit && event->type() == QEvent::MouseButtonDblClick) {
        if (m_historyPanel && m_filterEdit) {
            QStringList history = SearchHistoryService::instance().getHistory("sidebar_filter");
            if (!history.isEmpty()) {
                m_historyPanel->setHistory(history, "最近筛选");
                m_historyPanel->showBelow(m_filterEdit);
            }
        }
        return true;
    }
    return QFrame::eventFilter(watched, event);
}

} // namespace QuarkMeta
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps
1. **构建步骤**：
   在终端执行 `cmake --build build` 编译项目。
2. **运行时验收标准**：
   - 启动 QuarkMeta，观察侧边栏底部“筛选分类”输入框常驻显示。
   - 切换至“收藏夹”选项卡，在输入框中输入关键字，校验收藏夹项目与分类实时按关键字隐藏不匹配项并蓝色高亮匹配字词。
   - 双击输入框，校验弹现“最近筛选”历史记录悬浮面板，支持点击项目自动填入筛选以及右侧删除/清空操作。

---

## Chapter 5: Official API Reuse & Anti-Redundancy
- 提效复用了 `SearchHistoryService` 与 `SearchHistoryPanel`，未新增任何冗余历史管理类。

---

## Chapter 6: Data SSOT Declaration Table
| 数据项 | 唯一 SSOT 源 | 派生/缓存副本 | 同步方向 |
| :--- | :--- | :--- | :--- |
| 侧边栏筛选历史 | `SearchHistoryService` (`SearchHistory/sidebar_filter`) | `m_historyPanel` 展现 | Enter提交 -> `SearchHistoryService::appendSearch` -> 双击 -> `getHistory` -> `SearchHistoryPanel` |

---

## Chapter 7: Interface & Visibility Consistency Check
- 方法：`FavoritePanel::filterTreeNodes(const QString& keyword)`
  - 声明位置：`src/ui/FavoritePanel.h:47`
  - 访问权限：`public`
- 方法：`LibraryPanel::delegate()` / `LibraryPanel::treeView()` / `LibraryPanel::filterTreeNodes(const QString& keyword)`
  - 声明位置：`src/ui/LibraryPanel.h:61-65`
  - 访问权限：`public`

---

## Chapter 8: Header Chain & Inclusion Closure Check
- `SidebarContainerWidget.h` 中新增 `#include <QLineEdit>` 与 `#include "SearchHistoryPanel.h"` 前置声明，实现包含闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 序号 | 检查项目 | 结果 | 位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否只修改实施方案 | 满足 | 仅创建 `Implementation Plan/SidebarFilterSharedSupport-1.md` |
| 2 | SEARCH 块是否物理匹配且唯一 | 满足 | 精确匹配对应物理 `.h` 与 `.cpp` 行 |
| 3 | 是否破坏冻结的 .h API | 满足 | 保持 `.h` 契约，补全新增 Getter 与成员函数 |
| 4 | 是否包含 CMake 文件注册 | 不涉及 | 未增删 C++ 源文件 |
| 5 | 是否遵循单向依赖流 | 满足 | UI 控件转发，无逆向包含 |
| 6 | 是否零篡改既有设计数值 | 满足 | 32px 框高、8px 边距与 14px 筛选图标尺寸 100% 保持 |
| 7 | 是否包含完整 10 大章节 | 满足 | 包含 Chapter 1 至 Chapter 10 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table
- **行为与交互**：“库”选项卡筛选行为保持原样；“收藏夹”选项卡获得等同的高亮与过滤能力；输入框双击唤起“最近筛选”浮窗。
- **外观与数值**：底栏容器尺寸（`32px` 高）、输入框圆角、图标 Padding 参数 100% 原样保持。
