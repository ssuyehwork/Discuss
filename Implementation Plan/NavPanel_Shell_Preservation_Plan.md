# NavPanel 空壳化与五栏区保留改造实施方案 (NavPanel Shell & 5-Panel Architecture Preservation)

## Chapter 1: Problem Statement & Root Cause Analysis（问题与根因分析）

### 1. 现象与代码坏味道
在 QuarkMeta 界面复刻至 RapidNotes 过程中，用户明确指出：
- RapidNotes 采用纯文本笔记架构，数据唯一真理源为 SQLite 数据库；
- 主界面 `MainWindow` 必须维持 **5 个栏区**（`m_navPanel`、`m_sidebarContainer`、`m_contentPanel`、`m_metaPanel`、`m_filterPanel`）；
- 但原有的 `NavPanel` 包含了大量的 QuarkMeta 本地物理文件系统与磁盘直连操作（扫描桌面、此电脑、物理盘符 C/D/E、物理历史记录 `HistoryPanel` 以及物理回收站操作），严重违背了纯文本笔记架构；
- 同时用户明确要求：**最左侧的 NavPanel 栏区必须保留，仅设为空壳（TODO）即可**，采用视觉方案 B（保留选项卡框架，内容设为 TODO 占位），并保留状态栏与 `PanelLayoutManager` 对 NavPanel 的显示/隐藏折叠控制与记忆。

### 2. 根因剖析
历史复刻过程中，NavPanel 仍旧完整保留了从 QuarkMeta 复制而来的物理磁盘扫描器、`HistoryPanel`、`QDir::drives()` 及 `ShellIconManager`。如果直接粗暴删除 NavPanel，将破坏 MainWindow 五栏区布局契约与状态栏控制逻辑；若保留其扫描逻辑，则违背纯笔记架构。

### 3. 治理目标
1. **五栏区布局与联动契约 100% 保持**：`MainWindow` 与 `PanelLayoutManager` 维持对 `NavPanel` 的完整生命周期管理、QSplitter 分割比例与显示/隐藏记忆；
2. **NavPanel 视觉方案 B（保留 Tab，内容空壳化）**：保留顶部“目录导航”与“历史”Tab 栏（`SidebarTabBar`）及 `QStackedWidget`，在 Page 0 和 Page 1 分别挂载优雅统一的置灰 TODO 提示；
3. **彻底拔除物理磁盘扫描代码**：从 `NavPanel.h` 和 `NavPanel.cpp` 中彻底移除 `QDir`、`QFileInfo`、`DiskIoService`、`TrashService` 等物理依赖，所有对外接口（`deferredInit`、`setFocusHighlight`、`setRootPath`、`selectPath`）安全 no-op，确保编译零报错。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `RapidNotes_Source/RapidNotes/src/ui/NavPanel.h` — 纯化头文件，保留公开接口与信号，移除物理私有槽函数与数据结构。
2. `RapidNotes_Source/RapidNotes/src/ui/NavPanel.cpp` — 移除物理扫描与菜单逻辑，挂载 Tab 下的优雅 TODO 占位页面，接口转为安全 no-op。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `RapidNotes_Source/RapidNotes/src/ui/NavPanel.h` — 纯化接口与移除物理结构

```diff
<<<<<<< SEARCH
#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QVBoxLayout>
#include <QDir>
#include <QFrame>
#include <QTabBar>
#include <QStackedWidget>
#include "HistoryPanel.h"

namespace RapidNotes {

/**
 * @brief 导航面板（面板一）
 * 使用 QTreeView + QFileSystemModel 实现文件夹树导航
 */
class NavPanel : public QFrame {
    Q_OBJECT

public:
    explicit NavPanel(QWidget* parent = nullptr);
    ~NavPanel() override = default;

    bool eventFilter(QObject* watched, QEvent* event) override;

    // 🚀【物理契约】：刚性锁定 230px 下限，杜绝被 QSplitter 挤压偷扣像素
    QSize minimumSizeHint() const override { return QSize(230, 100); }

    void deferredInit();
    void setFocusHighlight(bool visible);
    void setRootPath(const QString& path);
    void selectPath(const QString& path);

signals:
    void directorySelected(const QString& path);
    void requestLocateFile(const QString& path);
    void requestOpenTrash();
    void requestAddFavorite(const QString& path);
    void requestRemoveFavorite(const QString& path);

private slots:
    void onItemExpanded(const QModelIndex& index);
    void onTreeClicked(const QModelIndex& index);
    void onTreeContextMenu(const QPoint& pos);
    void updateTreeHeight();
    void updateRecentVisitedList();

private:
    void initUi();
    void fetchChildDirs(QStandardItem* parent);
    QStandardItem* findItemByData(QStandardItem* parent, const QString& targetData);

    QTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    QStandardItem* m_recentRootItem = nullptr;
    QString m_currentPath;

    QTabBar* m_tabBar = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;
    HistoryPanel* m_historyPanel = nullptr;
};

} // namespace RapidNotes=======
#include <QWidget>
#include <QVBoxLayout>
#include <QFrame>
#include <QTabBar>
#include <QStackedWidget>

namespace RapidNotes {

/**
 * @brief 导航面板（面板一）
 * 遵循纯文本笔记架构标准，保留五栏区 Tab 框架与契约占位，作为空壳 (TODO) 运行
 */
class NavPanel : public QFrame {
    Q_OBJECT

public:
    explicit NavPanel(QWidget* parent = nullptr);
    ~NavPanel() override = default;

    bool eventFilter(QObject* watched, QEvent* event) override;

    // 🚀【物理契约】：刚性锁定 230px 下限，杜绝被 QSplitter 挤压偷扣像素
    QSize minimumSizeHint() const override { return QSize(230, 100); }

    void deferredInit();
    void setFocusHighlight(bool visible);
    void setRootPath(const QString& path);
    void selectPath(const QString& path);

signals:
    void directorySelected(const QString& path);
    void requestLocateFile(const QString& path);
    void requestOpenTrash();
    void requestAddFavorite(const QString& path);
    void requestRemoveFavorite(const QString& path);

private:
    void initUi();
    QWidget* createPlaceholderPage(const QString& iconName, const QString& title, const QString& subtitle);

    QVBoxLayout* m_mainLayout = nullptr;
    QTabBar* m_tabBar = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;
};

} // namespace RapidNotes
>>>>>>> REPLACE
```

### 3.2 `RapidNotes_Source/RapidNotes/src/ui/NavPanel.cpp` — 移除物理扫描与构建 TODO 占位

```diff
<<<<<<< SEARCH
#include "NavPanel.h"
#include "controllers/ContentKeyHandler.h"
#include <QKeyEvent>
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "Logger.h"
#include "TreeItemDelegate.h"
#include "DropTreeView.h"
#include "ContentPanel.h"
#include "ToolTipOverlay.h"
#include "../core/AppConfig.h"
#include "../core/NavigationHistoryService.h"
#include "../core/TrashService.h"
#include "../meta/FavoriteDao.h"
#include "../meta/FavoriteService.h"
#include "../meta/MarkedFolderService.h"
#include "../meta/LibraryService.h"
#include "../core/ClipboardService.h"
#include "../core/NavigationService.h"
#include "TabBarWidget.h"
#include "controllers/ContextMenuFactory.h"
#include <QStyleOptionTab>
#include <QPainter>
#include <QHeaderView>
#include <QScrollBar>
#include <QLabel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStandardPaths>
#include <QTimer>
#include <QPushButton>
#include <QPointer>
#include <QMenu>
#include <QClipboard>
#include <QtConcurrent>
#include <QApplication>

namespace RapidNotes {

/**
 * @brief 构造函数，设置面板属性
 */
NavPanel::NavPanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("SidebarContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setContextMenuPolicy(Qt::CustomContextMenu);
    initUi();
}

/**
 * @brief 初始化 UI 组件
 */
void NavPanel::deferredInit() {
    if (m_model && m_model->rowCount() > 0) {
        return;
    }

    // 1. 桌面入口 (使用 SVG 语义图标替代原生图标)
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QIcon desktopIcon = UiHelper::getIcon("home", QColor("#3498db"), 18);
    QStandardItem* desktopItem = new QStandardItem(desktopIcon, "桌面");
    desktopItem->setData(desktopPath, Qt::UserRole + 1);
    desktopItem->appendRow(new QStandardItem("Loading..."));
    m_model->appendRow(desktopItem);

    // 2. 此电脑入口
    QIcon computerIcon = UiHelper::getIcon("monitor", QColor("#3498db"), 18);
    QStandardItem* computerItem = new QStandardItem(computerIcon, "此电脑");
    computerItem->setData("computer://", Qt::UserRole + 1);
    m_model->appendRow(computerItem);

    // 3. 磁盘列表 (先填充基础文字路径，0 毫秒卡顿)
    const auto drives = QDir::drives();
    for (const QFileInfo& drive : drives) {
        QString driveName = drive.absolutePath();
        QStandardItem* driveItem = new QStandardItem(driveName);
        driveItem->setData(driveName, Qt::UserRole + 1);
        driveItem->appendRow(new QStandardItem("Loading..."));
        m_model->appendRow(driveItem);
    }

    // 主线程分批次补齐磁盘图标
    QTimer::singleShot(0, [this, drives]() {
        for (int i = 0; i < drives.size(); ++i) {
            if (i + 2 < m_model->rowCount()) {
                QIcon driveIcon = UiHelper::getIcon("hard_drive", QColor("#95a5a6"), 18);
                m_model->item(i + 2)->setIcon(driveIcon);
            }
        }
    });

    // 4. 最近访问 (固定主节点，在所有磁盘正下方)
    QIcon recentIcon = UiHelper::getIcon("clock_history", QColor("#3498db"), 18);
    m_recentRootItem = new QStandardItem(recentIcon, "最近访问");
    m_recentRootItem->setData("recent_root", Qt::UserRole + 1);
    m_model->appendRow(m_recentRootItem);

    // 异步探测填充历史记录，防止慢速物理/网络驱动器阻塞启动
    updateRecentVisitedList();
    if (m_treeView && m_recentRootItem->index().isValid()) {
        m_treeView->expand(m_recentRootItem->index());
    }

    // 5. 回收站 (固定主节点)
    QIcon trashIcon = UiHelper::getIcon("delete_forever", QColor("#e81123"), 18);
    QStandardItem* trashItem = new QStandardItem(trashIcon, "回收站");
    trashItem->setData("trash_root", Qt::UserRole + 1);
    m_model->appendRow(trashItem);
}

void NavPanel::setFocusHighlight(bool visible) {
    Q_UNUSED(visible);
}

void NavPanel::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);

    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(5, 0, 5, 0);
    headerLayout->setSpacing(0);

    // 侧边栏专用选项卡：直接掌控绘制，彻底杜绝 QSS 下样式代理被拦截失效问题，硬锁 7 像素间距
    class SidebarTabBar : public QTabBar {
    public:
        using QTabBar::QTabBar;
    protected:
        QSize tabSizeHint(int index) const override {
            QSize s = QTabBar::tabSizeHint(index);
            if (!tabIcon(index).isNull() && !tabText(index).isEmpty()) {
                s.setWidth(s.width() - 5);
            }
            return s;
        }

        void paintEvent(QPaintEvent* event) override {
            Q_UNUSED(event);
            QPainter painter(this);
            QStyleOptionTab opt;
            for (int i = 0; i < count(); ++i) {
                initStyleOption(&opt, i);
                // 1. 保留 QSS 样式表对背景色、边框、选态与悬停底色的完全渲染
                style()->drawControl(QStyle::CE_TabBarTabShape, &opt, &painter, this);

                // 2. 自定义精确绘制图标与文字，硬锁间隙为 7px
                QRect tr = opt.rect;
                QSize iconSize = opt.iconSize.isValid() ? opt.iconSize : QSize(16, 16);
                int iconWidth = opt.icon.isNull() ? 0 : iconSize.width();
                int textWidth = opt.fontMetrics.horizontalAdvance(opt.text);
                int spacing = (iconWidth > 0 && !opt.text.isEmpty()) ? 7 : 0;
                int totalContentWidth = iconWidth + spacing + textWidth;
                int startX = tr.left() + (tr.width() - totalContentWidth) / 2;

                if (iconWidth > 0) {
                    int iconY = tr.top() + (tr.height() - iconSize.height()) / 2;
                    QRect iconRect(startX, iconY, iconSize.width(), iconSize.height());
                    opt.icon.paint(&painter, iconRect, Qt::AlignCenter);
                }

                QRect textRect(startX + iconWidth + spacing, tr.top(), textWidth + 2, tr.height());
                painter.save();
                painter.setFont(font());
                QColor textColor = (opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : ((opt.state & QStyle::State_MouseOver) ? QColor("#CCCCCC") : QColor("#AAAAAA"));
                painter.setPen(textColor);
                painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, opt.text);
                painter.restore();
            }
        }
    };

    m_tabBar = new SidebarTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setIconSize(QSize(16, 16));
    m_tabBar->addTab(UiHelper::getIcon("list_ul", "#888888", 16), "目录导航");
    m_tabBar->addTab(UiHelper::getIcon("clock", "#888888", 16), "历史");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);

    // Page 0: 目录导航
    QWidget* navPage = new QWidget(m_stackedWidget);
    QVBoxLayout* navPageLayout = new QVBoxLayout(navPage);
    navPageLayout->setContentsMargins(0, 0, 0, 0);
    navPageLayout->setSpacing(0);

    m_treeView = new DropTreeView(navPage);
    m_treeView->setObjectName("NavTreeView");
    m_treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_treeView->setHeaderHidden(true);
    if (m_treeView->header()) {
        m_treeView->header()->setStretchLastSection(true);
        m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(20);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setExpandsOnDoubleClick(true);
    m_treeView->setDragEnabled(true);
    m_treeView->setDragDropMode(QAbstractItemView::DragOnly);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, false));
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_model = new QStandardItemModel(this);
    m_treeView->setModel(m_model);
    navPageLayout->addWidget(m_treeView);

    // Page 1: 历史记录
    m_historyPanel = new HistoryPanel(m_stackedWidget);

    m_stackedWidget->addWidget(navPage);
    m_stackedWidget->addWidget(m_historyPanel);

    m_mainLayout->addWidget(m_stackedWidget, 1);

    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
        if (index == 1 && m_historyPanel) {
            m_historyPanel->loadHistory();
        }
    });

    connect(m_historyPanel, &HistoryPanel::requestLocateFile, this, &NavPanel::requestLocateFile);

    QString arrowRight = UiHelper::getSvgTempFilePath("chevron_right", QColor("#378ADD"));
    QString arrowDown = UiHelper::getSvgTempFilePath("chevron_down", QColor("#378ADD"));
    QString treeStyle = QString(
        "QTreeView#NavTreeView::branch:has-children:closed { image: url(\"%1\"); }"
        "QTreeView#NavTreeView::branch:has-children:open { image: url(\"%2\"); }"
    ).arg(arrowRight, arrowDown);
    m_treeView->setStyleSheet(treeStyle);

    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->installEventFilter(this);
    if (m_treeView->viewport()) {
        m_treeView->viewport()->installEventFilter(this);
    }
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &NavPanel::onTreeContextMenu);

    connect(m_treeView, &QTreeView::expanded, this, &NavPanel::onItemExpanded);
    connect(m_treeView, &QTreeView::clicked, this, &NavPanel::onTreeClicked);
    connect(&NavigationHistoryService::instance(), &NavigationHistoryService::historyChanged, this, &NavPanel::updateRecentVisitedList);
}

void NavPanel::updateRecentVisitedList() {
    if (!m_recentRootItem) return;

    QStringList history = NavigationHistoryService::instance().getHistory();
    if (history.isEmpty()) {
        m_recentRootItem->removeRows(0, m_recentRootItem->rowCount());
        return;
    }

    struct RecentItemData {
        QString path;
        QString displayName;
        QIcon icon;
    };

    // 🚀【物理并发加速】：历史路径存在性检测与图标提取异步化，杜绝局域网/失效U盘拖死主线程
    QPointer<NavPanel> weakThis(this);
    (void)QtConcurrent::run([weakThis, history]() {
        QList<RecentItemData> validItems;
        QSet<QString> seenPaths;
        int count = 0;

        for (const QString& path : history) {
            if (count >= 14) break;
            if (path.isEmpty() || path == "computer://" || path.startsWith("trash") || path.startsWith("分类: ")) continue;

            QString normalizedKey = QDir::cleanPath(path).toLower();
            if (seenPaths.contains(normalizedKey)) continue;
            seenPaths.insert(normalizedKey);

            QFileInfo info(path);
            if (!info.exists() || !info.isDir()) continue;

            QString displayName = info.fileName();
            if (displayName.isEmpty()) {
                displayName = QDir::toNativeSeparators(path);
            }

            QIcon icon = ShellIconManager::getFileIcon(path, 18);
            if (icon.isNull()) {
                icon = UiHelper::getIcon("folder_filled", QColor("#3498db"), 18);
            }

            validItems.append({path, displayName, icon});
            count++;
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, validItems]() {
            if (!weakThis || !weakThis->m_recentRootItem) return;

            weakThis->m_recentRootItem->removeRows(0, weakThis->m_recentRootItem->rowCount());
            for (const auto& item : validItems) {
                QStandardItem* child = new QStandardItem(item.icon, item.displayName);
                child->setData(item.path, Qt::UserRole + 1);
                child->setData(QDir::toNativeSeparators(item.path), Qt::UserRole + 2);
                weakThis->m_recentRootItem->appendRow(child);
            }

            if (!weakThis->m_currentPath.isEmpty()) {
                weakThis->selectPath(weakThis->m_currentPath);
            }
        }, Qt::QueuedConnection);
    });
}

void NavPanel::setRootPath(const QString& path) {
    Q_UNUSED(path);
}

QStandardItem* NavPanel::findItemByData(QStandardItem* parent, const QString& targetData) {
    if (!parent) return nullptr;
    for (int i = 0; i < parent->rowCount(); ++i) {
        QStandardItem* child = parent->child(i);
        if (!child) continue;
        if (child->data(Qt::UserRole + 1).toString() == targetData) {
            return child;
        }
        QStandardItem* subMatch = findItemByData(child, targetData);
        if (subMatch) {
            return subMatch;
        }
    }
    return nullptr;
}

void NavPanel::selectPath(const QString& path) {
    m_currentPath = path;
    QString targetData = (path == "trash://" || path == "trash") ? "trash_root" : path;
    QStandardItem* foundItem = findItemByData(m_model->invisibleRootItem(), targetData);
    if (foundItem) {
        m_treeView->setCurrentIndex(foundItem->index());
        m_treeView->setFocus();
    }
}

void NavPanel::onTreeContextMenu(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid()) return;

    QString path = index.data(Qt::UserRole + 1).toString();

    if (path == "trash_root") {
        QMenu menu(this);
        UiHelper::applyMenuStyle(&menu);

        QAction* actRestore = menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "还原全部");
        QAction* actEmpty = menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "清空回收站");

        connect(actRestore, &QAction::triggered, this, [this]() {
            TrashService::instance().restoreAll(this);
        });
        connect(actEmpty, &QAction::triggered, this, [this]() {
            TrashService::instance().emptyTrash(this);
        });

        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
        return;
    }

    if (path.isEmpty() || path == "computer://" || path == "recent_root") {
        return;
    }

    QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        return;
    }

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    // 1. 打开文件夹
    QAction* actOpenFolder = menu.addAction(UiHelper::getIcon("folder", QColor("#EEEEEE"), 18), "打开文件夹");
    connect(actOpenFolder, &QAction::triggered, this, [this, path]() {
        emit directorySelected(path);
    });

    // 2. 标签页级联子菜单
    ContextMenuFactory::buildTabCascadeMenu(&menu, path, this);

    // 3. 在“资源管理器”中显示
    ContextMenuFactory::buildShowInExplorerAction(&menu, path, this);

    menu.addSeparator();

    // 4. 置顶 / 收藏夹 / 库
    MarkedFolderService::instance().buildMarkAction(&menu, path, this);
    FavoriteService::instance().buildFavoriteAction(&menu, path, this);
    LibraryService::instance().buildLibraryAction(&menu, QStringList{path}, this);

    menu.addSeparator();

    // 5. 复制名称 / 复制完整路径
    ContextMenuFactory::buildCopyNameAction(&menu, QStringList{path}, this);
    ContextMenuFactory::buildCopyPathAction(&menu, QStringList{path}, this);

    // 6. 粘贴到此文件夹
    QAction* actPaste = menu.addAction(UiHelper::getIcon("paste", QColor("#EEEEEE"), 18), "粘贴到此文件夹");
    actPaste->setEnabled(ClipboardService::instance().canPaste(path));
    connect(actPaste, &QAction::triggered, this, [this, path]() {
        ClipboardService::instance().executePaste(path, this);
    });

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void NavPanel::onTreeClicked(const QModelIndex& index) {
    QString path = index.data(Qt::UserRole + 1).toString();
    if (path == "trash_root") {
        emit requestOpenTrash();
    } else if (!path.isEmpty() && path != "computer://" && path != "recent_root") {
        emit directorySelected(path);
    } else if (path == "computer://") {
        emit directorySelected("computer://");
    }
}

void NavPanel::onItemExpanded(const QModelIndex& index) {
    QStandardItem* item = m_model->itemFromIndex(index);
    if (!item) return;

    if (item->rowCount() == 1 && item->child(0)->text() == "Loading...") {
        fetchChildDirs(item);
    }
}

void NavPanel::updateTreeHeight() {
}

bool NavPanel::eventFilter(QObject* watched, QEvent* event) {
    if ((watched == m_treeView || (m_treeView && watched == m_treeView->viewport())) 
        && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEv = static_cast<QKeyEvent*>(event);
        if (keyEv->modifiers() & Qt::ControlModifier) {
            if (keyEv->key() == Qt::Key_Home) {
                if (ContentKeyHandler::navigateViewFirstItem(m_treeView)) return true;
            } else if (keyEv->key() == Qt::Key_End) {
                if (ContentKeyHandler::navigateViewLastItem(m_treeView)) return true;
            }
        }
    }
    if (event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            return true;
        }
    } else if (event->type() == QEvent::Leave) {
        ToolTipOverlay::hideTip();
    }
    return QFrame::eventFilter(watched, event);
}

void NavPanel::fetchChildDirs(QStandardItem* parent) {
    QString path = parent->data(Qt::UserRole + 1).toString();
    if (path.isEmpty() || path == "computer://") return;

    parent->removeRows(0, parent->rowCount());
    parent->appendRow(new QStandardItem("正在读取..."));

    QPersistentModelIndex pIdx(parent->index());
    (void)QtConcurrent::run([this, pIdx, path]() {
        QDir dir(path);
        QFileInfoList list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

        struct DirInfo { QString name; QString absPath; bool hasSub; };
        QList<DirInfo> results;
        for (const QFileInfo& info : list) {
            QDir subDir(info.absoluteFilePath());
            bool hasSub = !subDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
            results << DirInfo{info.fileName(), info.absoluteFilePath(), hasSub};
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [this, pIdx, results]() {
            if (!pIdx.isValid()) return;
            QStandardItem* safeParent = m_model->itemFromIndex(pIdx);
            if (!safeParent) return;

            safeParent->removeRows(0, safeParent->rowCount());

            for (const auto& info : results) {
                QIcon folderIcon = ShellIconManager::getFileIcon(info.absPath, 18);
                QStandardItem* child = new QStandardItem(folderIcon, info.name);
                child->setData(info.absPath, Qt::UserRole + 1);

                if (info.hasSub) {
                    child->appendRow(new QStandardItem("Loading..."));
                }
                safeParent->appendRow(child);
            }
        }, Qt::QueuedConnection);
    });
}

} // namespace RapidNotes=======
#include "NavPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include <QStyleOptionTab>
#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace RapidNotes {

/**
 * @brief 构造函数，设置面板属性与布局
 */
NavPanel::NavPanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("SidebarContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setContextMenuPolicy(Qt::CustomContextMenu);
    initUi();
}

void NavPanel::deferredInit() {
    // 纯文本笔记架构：NavPanel 作为空壳运行，无需进行物理磁盘与目录树扫描
}

void NavPanel::setFocusHighlight(bool visible) {
    Q_UNUSED(visible);
}

void NavPanel::setRootPath(const QString& path) {
    Q_UNUSED(path);
}

void NavPanel::selectPath(const QString& path) {
    Q_UNUSED(path);
}

QWidget* NavPanel::createPlaceholderPage(const QString& iconName, const QString& title, const QString& subtitle) {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 40, 20, 20);
    layout->setSpacing(10);
    layout->setAlignment(Qt::AlignCenter);

    QLabel* iconLabel = new QLabel(page);
    iconLabel->setPixmap(UiHelper::getIcon(iconName, "#555555", 36).pixmap(36, 36));
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel* titleLabel = new QLabel(title, page);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #777777; font-size: 13px; font-weight: bold;");

    QLabel* descLabel = new QLabel(subtitle, page);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setStyleSheet("color: #555555; font-size: 11px;");

    layout->addStretch();
    layout->addWidget(iconLabel);
    layout->addWidget(titleLabel);
    layout->addWidget(descLabel);
    layout->addStretch();

    return page;
}

void NavPanel::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);

    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(5, 0, 5, 0);
    headerLayout->setSpacing(0);

    // 侧边栏专用选项卡：高保真绘制，硬锁 7 像素间距
    class SidebarTabBar : public QTabBar {
    public:
        using QTabBar::QTabBar;
    protected:
        QSize tabSizeHint(int index) const override {
            QSize s = QTabBar::tabSizeHint(index);
            if (!tabIcon(index).isNull() && !tabText(index).isEmpty()) {
                s.setWidth(s.width() - 5);
            }
            return s;
        }

        void paintEvent(QPaintEvent* event) override {
            Q_UNUSED(event);
            QPainter painter(this);
            QStyleOptionTab opt;
            for (int i = 0; i < count(); ++i) {
                initStyleOption(&opt, i);
                style()->drawControl(QStyle::CE_TabBarTabShape, &opt, &painter, this);

                QRect tr = opt.rect;
                QSize iconSize = opt.iconSize.isValid() ? opt.iconSize : QSize(16, 16);
                int iconWidth = opt.icon.isNull() ? 0 : iconSize.width();
                int textWidth = opt.fontMetrics.horizontalAdvance(opt.text);
                int spacing = (iconWidth > 0 && !opt.text.isEmpty()) ? 7 : 0;
                int totalContentWidth = iconWidth + spacing + textWidth;
                int startX = tr.left() + (tr.width() - totalContentWidth) / 2;

                if (iconWidth > 0) {
                    int iconY = tr.top() + (tr.height() - iconSize.height()) / 2;
                    QRect iconRect(startX, iconY, iconSize.width(), iconSize.height());
                    opt.icon.paint(&painter, iconRect, Qt::AlignCenter);
                }

                QRect textRect(startX + iconWidth + spacing, tr.top(), textWidth + 2, tr.height());
                painter.save();
                painter.setFont(font());
                QColor textColor = (opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : ((opt.state & QStyle::State_MouseOver) ? QColor("#CCCCCC") : QColor("#AAAAAA"));
                painter.setPen(textColor);
                painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, opt.text);
                painter.restore();
            }
        }
    };

    m_tabBar = new SidebarTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setIconSize(QSize(16, 16));
    m_tabBar->addTab(UiHelper::getIcon("list_ul", "#888888", 16), "目录导航");
    m_tabBar->addTab(UiHelper::getIcon("clock", "#888888", 16), "历史");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);

    // Page 0: 目录导航空壳占位
    QWidget* navPage = createPlaceholderPage("list_ul", "目录导航 (TODO)", "纯文本笔记架构占位容器");

    // Page 1: 历史记录空壳占位
    QWidget* historyPage = createPlaceholderPage("clock", "历史记录 (TODO)", "纯文本笔记架构占位容器");

    m_stackedWidget->addWidget(navPage);
    m_stackedWidget->addWidget(historyPage);

    m_mainLayout->addWidget(m_stackedWidget, 1);

    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
    });
}

bool NavPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            return true;
        }
    } else if (event->type() == QEvent::Leave) {
        ToolTipOverlay::hideTip();
    }
    return QFrame::eventFilter(watched, event);
}

} // namespace RapidNotes
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译验证命令
在 `RapidNotes_Source/RapidNotes` 目录下执行构建，确认 `NavPanel.cpp` 编译无任何报错：
```bash
cmake -B build
cmake --build build --target RapidNotes
```

### 4.2 运行时验收标准
1. **五栏区布局完整呈现**：启动应用唤起 `MainWindow`，水平分割器保持五个栏区完整布局，最左侧为 `NavPanel`；
2. **选项卡保真**：最左侧 `NavPanel` 顶部正常显示“目录导航”与“历史”Tab 标签，切换 Tab 时可在 Page 0 与 Page 1 之间平滑切换；
3. **占位与无任何物理阻塞**：内容区域显示规范的置灰 TODO 提示，不再执行任何本地物理磁盘读取、历史记录异步探测或驱动器轮询；
4. **状态栏与折叠记忆**：状态栏与视图菜单中“显示目录导航”动作能正常控制最左侧 `NavPanel` 的显示与隐藏，且重启后记忆状态完整恢复。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）
1. **复用 Qt 原生布局与容器组件**：直接基于 `QStackedWidget`、`QTabBar` 与 `QVBoxLayout` 构建，杜绝重造自定义 Tab 容器；
2. **复用统一 UI 语义助手 `UiHelper::getIcon`**：使用 SVG 语义图标 `list_ul` 与 `clock`，统一使用系统高保真调色板；
3. **消除重复代码**：将 Page 0 与 Page 1 的占位构建统一收敛至私有辅助函数 `createPlaceholderPage`。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本/视图层 | 同步与刷新流向 |
| :--- | :--- | :--- | :--- |
| **五栏区折叠与尺寸状态** | `AppConfig` (`QSettings`) | `PanelLayoutManager` / `MainWindow` | `PanelLayoutManager::initLayout()` 读取，状态变更原子落盘 |
| **NavPanel 选项卡索引** | `QStackedWidget::currentIndex()` | `SidebarTabBar` 选态 | `m_tabBar::currentChanged` -> `m_stackedWidget::setCurrentIndex` |
| **笔记核心元数据** | SQLite `notes` 表 | `LibraryPanel` / `ContentPanel` | 彻底脱离 NavPanel，由数据库专职服务单向流动 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 外部调用接口核查（物理 .h 原样签名匹配）
- `MainWindow.cpp:558` 调用 `m_navPanel->deferredInit()`：原样保留 `void deferredInit();`，内部为安全 no-op；
- `PanelMediator.cpp:350` 调用 `navPanel->selectPath(...)`：原样保留 `void selectPath(const QString& path);`，内部为安全 no-op；
- `PanelLayoutManager.cpp:21` 接收 `NavPanel* navPanel`：保留 `NavPanel` 类型声明与公共 `QFrame` 继承属性；
- `PanelMediator.cpp:371-398` 监听信号 `directorySelected`、`requestLocateFile` 等：全部保留在 `signals` 区域，签名 100% 匹配。

### 7.2 新增内部接口
- `QWidget* createPlaceholderPage(const QString& iconName, const QString& title, const QString& subtitle);`：私有辅助函数，声明与定义完全匹配。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

### 8.1 移除的无用头文件
- 移除 `DropTreeView.h`、`TreeItemDelegate.h`、`QFileSystemModel`、`QStandardItemModel`、`HistoryPanel.h`；
- 移除物理磁盘操作头文件：`QDir`、`QFileInfo`、`TrashService.h`、`ShellIconManager.h`、`FavoriteService.h`、`ClipboardService.h` 等。

### 8.2 显式保留并闭合的头文件
- `NavPanel.h` 显式包含：`<QWidget>`、`<QVBoxLayout>`、`<QFrame>`、`<QTabBar>`、`<QStackedWidget>`；
- `NavPanel.cpp` 显式包含：`"NavPanel.h"`、`"UiHelper.h"`、`"ToolTipOverlay.h"`、`<QStyleOptionTab>`、`<QPainter>`、`<QLabel>`、`<QVBoxLayout>`、`<QHBoxLayout>`。全部类型均有显式定义，杜绝隐式包含。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 证据/位置 |
| :--- | :--- | :--- | :--- |
| 1 | 语言只允许使用中文 | 满足 | 全文使用中文表述，除固定技术英文外杜绝英文正文 |
| 2 | 不得直接修改源代码（物理隔离） | 满足 | 方案独立存放在 `Implementation Plan/NavPanel_Shell_Preservation_Plan.md` |
| 3 | SEARCH 块在仓库中真实存在且唯一匹配 | 满足 | 通过 `verify_plan.py` 自动化 100% 验证唯一匹配 |
| 4 | 对外接口签名冻结 | 满足 | `NavPanel.h` 保留所有原有公开接口与信号，调用方契约 0 破坏 |
| 5 | 用户意图优先（保留五栏区+空壳方案 B） | 满足 | 严格遵照用户指令，保留五栏区与 Tab 结构，仅内容设为空壳 TODO |
| 6 | 移除物理磁盘直接操作 | 满足 | 拔除 `QDir::drives()`、桌面、此电脑扫描与文件定位逻辑 |
| 7 | ToolTipOverlay 不跟随鼠标位置显示 | 满足 | 仅保留静态 `ToolTipOverlay::hideTip()` 处理 |
| 8 | 完整包含 10 大必备章节 | 满足 | 涵盖 Chapter 1 至 Chapter 10 全部规范章节 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 规范项 | 既有实现 / 契约数值 | 新实现 / 迁移数值 | 结论 |
| :--- | :--- | :--- | :--- |
| **NavPanel 最小宽度** | `minimumSizeHint() const override { return QSize(230, 100); }` | `QSize(230, 100)` | 严格一致，防止 QSplitter 挤压 |
| **顶部 Header 高度** | `header->setFixedHeight(32);` | `32px` | 严格一致 |
| **Tab 栏字间距硬锁** | `int spacing = (iconWidth > 0 && !opt.text.isEmpty()) ? 7 : 0;` | `7px` | 严格一致 |
| **Tab 栏文字颜色** | `#FFFFFF` (Selected), `#CCCCCC` (Hover), `#AAAAAA` (Normal) | 完全一致 | 严格一致 |
| **五栏区分割器布局** | `m_mainSplitter` 包含 5 个面板 | 包含 5 个面板 | 严格一致 |
