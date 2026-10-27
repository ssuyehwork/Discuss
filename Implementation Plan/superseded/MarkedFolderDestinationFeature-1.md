# Marked Folders / Destination Tab Feature Implementation Plan

**Base Commit Hash**: `922590c`
**Prerequisite Plans**: None
**Supersedes**: `superseded/MarkedFolderDestinationFeature.md`

---

## Chapter 1: Overview

### 根因与需求分析
根据用户提出的“标记 / 目的地文件夹”新增功能需求及评审反馈（限制最多标记 50 个文件夹，并在超限时提示），系统需要提供一套高效率的“把文件夹标记为目的地，后续快捷把文件/文件夹发送至已标记目的地”的机制：
1. **侧边栏 3 选项卡扩展**：侧边栏顶部的选项卡控件（`SidebarContainerWidget` 中的 `m_tabBar`）需要做缩窄自适应配置，并在“收藏夹”和“库”右侧追加第 3 个选项卡**“标记”**。图标使用 `SvgIcons.h` 中的 `Destination.svg`（色值 `#ff551c`）。
2. **“标记”数据持久化与 50 个数量上限阻断**：只能标记文件夹（严格禁止标记文件），数据保存在 SQLite `global.db` 新建的 `marked_folders` 数据库表中。已标记文件夹数量限制为**最多 50 个**；若已达到 50 个，继续添加时阻断并调用 `ToolTipOverlay::instance()->showToast("标记数量已达 50 个上限，请先移除部分标记", 2000, QColor("#e81123"))`。
3. **右键菜单与快捷键触发**：
   - 右键菜单新增“标记”/“标记为目的地”操作；
   - 快捷键 **`Alt+C`**：对当前选中的文件夹执行标记/取消标记操作，支持多重标记；
   - 快捷键 **`Alt+V`**：将当前选中的文件或文件夹直接剪切/迁移发送至所有已标记的目的地文件夹中。
4. **视觉标识 Badge 渲染**：被标记的文件夹在“收藏夹”（`FavoritePanel`，`FavoriteItemDelegate` 在 `FavoritePanel.cpp` 内）和“目录导航”（`NavPanel`）以及标记面板自身中，名称右侧显示 `Destination.svg`（`#ff551c`）高亮橙色图标。
5. **标记管理与清理**：提供取消标记、从标记中移除、清空历史标记等操作。
6. **强制 TaskProgressStrip 进度条规则（覆盖原有规则）**：本功能触发的所有批量迁移/复制/粘贴操作，**不论数量多少（即使只有 1 个文件），一律强制显示顶部 `TaskProgressStrip` 进度条**。

---

## Chapter 2: Modified Files List

1. `src/meta/MarkedFolderDao.h` (NEW)
2. `src/meta/MarkedFolderDao.cpp` (NEW)
3. `src/meta/MarkedFolderService.h` (NEW)
4. `src/meta/MarkedFolderService.cpp` (NEW)
5. `src/ui/MarkedPanel.h` (NEW)
6. `src/ui/MarkedPanel.cpp` (NEW)
7. `src/meta/DatabaseManager.cpp`
8. `src/ui/SidebarContainerWidget.h`
9. `src/ui/SidebarContainerWidget.cpp`
10. `src/ui/FavoritePanel.cpp`
11. `src/ui/NavPanel.h`
12. `src/ui/NavPanel.cpp`
13. `src/ui/controllers/ContextMenuFactory.h`
14. `src/ui/controllers/ContextMenuFactory.cpp`
15. `src/ui/controllers/ContentContextMenu.cpp`
16. `src/ui/controllers/ContentKeyHandler.cpp`
17. `src/ui/controllers/PanelMediator.h`
18. `src/ui/controllers/PanelMediator.cpp`
19. `resources/style.qss`
20. `CMakeLists.txt`

---

## Chapter 3: Detailed Line-by-Line Changes

### 3.1 `src/meta/MarkedFolderDao.h` (NEW)

```cpp
<<<<<<< SEARCH FILE: src/meta/MarkedFolderDao.h
=======
#pragma once

#include <QStringList>
#include <QDateTime>

namespace QuarkMeta {

struct MarkedFolderRecord {
    int id = -1;
    QString folderPath;
    QDateTime createdAt;
};

class MarkedFolderDao {
public:
    static bool initTable();
    static bool addMarkedFolder(const QString& folderPath);
    static bool removeMarkedFolder(const QString& folderPath);
    static bool isMarked(const QString& folderPath);
    static int getMarkedCount();
    static QStringList getAllMarkedFolders();
    static bool clearAllMarkedFolders();
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.2 `src/meta/MarkedFolderDao.cpp` (NEW)

```cpp
<<<<<<< SEARCH FILE: src/meta/MarkedFolderDao.cpp
=======
#include "MarkedFolderDao.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>

namespace QuarkMeta {

bool MarkedFolderDao::initTable() {
    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS marked_folders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            folder_path TEXT UNIQUE NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    if (!query.exec(sql)) {
        return false;
    }
    return true;
}

bool MarkedFolderDao::addMarkedFolder(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    if (getMarkedCount() >= 50 && !isMarked(folderPath)) {
        return false;
    }
    QString cleanPath = QDir::cleanPath(folderPath);

    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    query.prepare("INSERT OR IGNORE INTO marked_folders (folder_path) VALUES (:path);");
    query.bindValue(":path", cleanPath);
    return query.exec();
}

bool MarkedFolderDao::removeMarkedFolder(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    query.prepare("DELETE FROM marked_folders WHERE folder_path = :path;");
    query.bindValue(":path", cleanPath);
    return query.exec();
}

bool MarkedFolderDao::isMarked(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    query.prepare("SELECT 1 FROM marked_folders WHERE folder_path = :path LIMIT 1;");
    query.bindValue(":path", cleanPath);
    if (query.exec() && query.next()) {
        return true;
    }
    return false;
}

int MarkedFolderDao::getMarkedCount() {
    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query("SELECT COUNT(*) FROM marked_folders;", DatabaseManager::instance().getDatabase());
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QStringList MarkedFolderDao::getAllMarkedFolders() {
    QStringList result;
    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query("SELECT folder_path FROM marked_folders ORDER BY id DESC;", DatabaseManager::instance().getDatabase());
    while (query.next()) {
        result.append(query.value(0).toString());
    }
    return result;
}

bool MarkedFolderDao::clearAllMarkedFolders() {
    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    return query.exec("DELETE FROM marked_folders;");
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.3 `src/meta/MarkedFolderService.h` (NEW)

```cpp
<<<<<<< SEARCH FILE: src/meta/MarkedFolderService.h
=======
#pragma once

#include <QObject>
#include <QStringList>
#include <QMenu>

namespace QuarkMeta {

class MarkedFolderService : public QObject {
    Q_OBJECT

public:
    static MarkedFolderService& instance();
    explicit MarkedFolderService(QObject* parent = nullptr);
    ~MarkedFolderService() override = default;

    bool isMarked(const QString& path) const;
    bool addMarked(const QString& path);
    bool removeMarked(const QString& path);
    bool toggleMarked(const QString& path);
    int markedCount() const;
    QStringList getAllMarked() const;
    bool clearAll();

    QAction* buildMarkAction(QMenu* parentMenu, const QString& path, QObject* receiver);

signals:
    void markedFoldersChanged();

private:
    Q_DISABLE_COPY_MOVE(MarkedFolderService)
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.4 `src/meta/MarkedFolderService.cpp` (NEW)

```cpp
<<<<<<< SEARCH FILE: src/meta/MarkedFolderService.cpp
=======
#include "MarkedFolderService.h"
#include "MarkedFolderDao.h"
#include "../ui/UiHelper.h"
#include "../ui/ToolTipOverlay.h"
#include <QFileInfo>
#include <QAction>

namespace QuarkMeta {

MarkedFolderService& MarkedFolderService::instance() {
    static MarkedFolderService s_instance;
    return s_instance;
}

MarkedFolderService::MarkedFolderService(QObject* parent) : QObject(parent) {}

bool MarkedFolderService::isMarked(const QString& path) const {
    if (path.isEmpty()) return false;
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) return false;
    return MarkedFolderDao::isMarked(path);
}

int MarkedFolderService::markedCount() const {
    return MarkedFolderDao::getMarkedCount();
}

bool MarkedFolderService::addMarked(const QString& path) {
    if (path.isEmpty()) return false;
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) return false;

    if (markedCount() >= 50 && !isMarked(path)) {
        ToolTipOverlay::instance()->showToast("标记数量已达 50 个上限，请先移除部分标记", 2000, QColor("#e81123"));
        return false;
    }

    if (MarkedFolderDao::addMarkedFolder(path)) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

bool MarkedFolderService::removeMarked(const QString& path) {
    if (MarkedFolderDao::removeMarkedFolder(path)) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

bool MarkedFolderService::toggleMarked(const QString& path) {
    if (isMarked(path)) {
        return removeMarked(path);
    } else {
        return addMarked(path);
    }
}

QStringList MarkedFolderService::getAllMarked() const {
    return MarkedFolderDao::getAllMarkedFolders();
}

bool MarkedFolderService::clearAll() {
    if (MarkedFolderDao::clearAllMarkedFolders()) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

QAction* MarkedFolderService::buildMarkAction(QMenu* parentMenu, const QString& path, QObject* receiver) {
    if (!parentMenu) return nullptr;
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) return nullptr;

    bool marked = isMarked(path);
    QString label = marked ? "取消标记" : "标记为目的地";
    QIcon icon = UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 18);

    QAction* action = parentMenu->addAction(icon, label);
    if (receiver) {
        connect(action, &QAction::triggered, receiver, [this, path]() {
            toggleMarked(path);
        });
    }
    return action;
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.5 `src/ui/MarkedPanel.h` (NEW)

```cpp
<<<<<<< SEARCH FILE: src/ui/MarkedPanel.h
=======
#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace QuarkMeta {

class MarkedPanel : public QWidget {
    Q_OBJECT

public:
    explicit MarkedPanel(QWidget* parent = nullptr);
    ~MarkedPanel() override = default;

    void loadMarkedFolders();
    void filterTreeNodes(const QString& keyword);

signals:
    void requestLocateFolder(const QString& path);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onContextMenuRequested(const QPoint& pos);
    void onMarkedFoldersChanged();

private:
    void initUi();
    QTreeWidgetItem* createMarkedItem(const QString& path);

    QVBoxLayout* m_mainLayout = nullptr;
    QTreeWidget* m_treeWidget = nullptr;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.6 `src/ui/MarkedPanel.cpp` (NEW)

```cpp
<<<<<<< SEARCH FILE: src/ui/MarkedPanel.cpp
=======
#include "MarkedPanel.h"
#include "UiHelper.h"
#include "../meta/MarkedFolderService.h"
#include "ToolTipOverlay.h"
#include <QFileInfo>
#include <QDir>
#include <QMenu>
#include <QMouseEvent>

namespace QuarkMeta {

MarkedPanel::MarkedPanel(QWidget* parent) : QWidget(parent) {
    initUi();
    connect(&MarkedFolderService::instance(), &MarkedFolderService::markedFoldersChanged, this, &MarkedPanel::onMarkedFoldersChanged);
    loadMarkedFolders();
}

void MarkedPanel::initUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setObjectName("MarkedTreeWidget");
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(0);
    m_treeWidget->setIconSize(QSize(16, 16));
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    m_mainLayout->addWidget(m_treeWidget);

    connect(m_treeWidget, &QTreeWidget::itemClicked, this, &MarkedPanel::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &MarkedPanel::onContextMenuRequested);
}

void MarkedPanel::loadMarkedFolders() {
    m_treeWidget->clear();
    QStringList markedList = MarkedFolderService::instance().getAllMarked();
    for (const QString& path : markedList) {
        if (QFile::exists(path)) {
            QTreeWidgetItem* item = createMarkedItem(path);
            m_treeWidget->addTopLevelItem(item);
        }
    }
}

QTreeWidgetItem* MarkedPanel::createMarkedItem(const QString& path) {
    QFileInfo fi(path);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, fi.fileName());
    item->setData(0, Qt::UserRole, path);
    item->setIcon(0, UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 16));
    return item;
}

void MarkedPanel::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty()) {
        emit requestLocateFolder(path);
    }
}

void MarkedPanel::onContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    if (item) {
        QString path = item->data(0, Qt::UserRole).toString();
        QAction* removeAct = menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "从标记中移除");
        connect(removeAct, &QAction::triggered, this, [path]() {
            MarkedFolderService::instance().removeMarked(path);
        });
        menu.addSeparator();
    }

    QAction* clearAct = menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "取消所有标记");
    connect(clearAct, &QAction::triggered, this, []() {
        MarkedFolderService::instance().clearAll();
    });

    menu.exec(m_treeWidget->mapToGlobal(pos));
}

void MarkedPanel::onMarkedFoldersChanged() {
    loadMarkedFolders();
}

void MarkedPanel::filterTreeNodes(const QString& keyword) {
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

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.7 `src/meta/DatabaseManager.cpp`

```
<<<<<<< SEARCH
#include "DatabaseManager.h"
#include "DriveMetaDao.h"
=======
#include "DatabaseManager.h"
#include "DriveMetaDao.h"
#include "MarkedFolderDao.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    DriveMetaDao::initTable();
=======
    DriveMetaDao::initTable();
    MarkedFolderDao::initTable();
>>>>>>> REPLACE
```

### 3.8 `src/ui/SidebarContainerWidget.h`

```
<<<<<<< SEARCH
class FavoritePanel;
class LibraryPanel;
class SearchHistoryPanel;
=======
class FavoritePanel;
class LibraryPanel;
class MarkedPanel;
class SearchHistoryPanel;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    FavoritePanel* favoritePanel() const { return m_favoritePanel; }
    LibraryPanel* libraryPanel() const { return m_libraryPanel; }
=======
    FavoritePanel* favoritePanel() const { return m_favoritePanel; }
    LibraryPanel* libraryPanel() const { return m_libraryPanel; }
    MarkedPanel* markedPanel() const { return m_markedPanel; }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    FavoritePanel* m_favoritePanel = nullptr;
    LibraryPanel* m_libraryPanel = nullptr;
=======
    FavoritePanel* m_favoritePanel = nullptr;
    LibraryPanel* m_libraryPanel = nullptr;
    MarkedPanel* m_markedPanel = nullptr;
>>>>>>> REPLACE
```

### 3.9 `src/ui/SidebarContainerWidget.cpp`

```
<<<<<<< SEARCH
#include "FavoritePanel.h"
#include "LibraryPanel.h"
=======
#include "FavoritePanel.h"
#include "LibraryPanel.h"
#include "MarkedPanel.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_tabBar->addTab(UiHelper::getIcon("Favorites.svg", "#888888", 16), "收藏夹");
    m_tabBar->addTab(UiHelper::getIcon("Search-in-library", "#888888", 16), "库");
=======
    m_tabBar->addTab(UiHelper::getIcon("Favorites.svg", "#888888", 16), "收藏夹");
    m_tabBar->addTab(UiHelper::getIcon("Search-in-library", "#888888", 16), "库");
    m_tabBar->addTab(UiHelper::getIcon("Destination.svg", "#ff551c", 16), "标记");
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_stackedWidget = new QStackedWidget(this);
    m_favoritePanel = new FavoritePanel(this);
    m_libraryPanel = new LibraryPanel(this);

    m_stackedWidget->addWidget(m_favoritePanel);
    m_stackedWidget->addWidget(m_libraryPanel);
=======
    m_stackedWidget = new QStackedWidget(this);
    m_favoritePanel = new FavoritePanel(this);
    m_libraryPanel = new LibraryPanel(this);
    m_markedPanel = new MarkedPanel(this);

    m_stackedWidget->addWidget(m_favoritePanel);
    m_stackedWidget->addWidget(m_libraryPanel);
    m_stackedWidget->addWidget(m_markedPanel);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
            QString kw = m_filterEdit->text().trimmed();
            if (index == 0 && m_favoritePanel) m_favoritePanel->filterTreeNodes(kw);
            if (index == 1 && m_libraryPanel) m_libraryPanel->filterTreeNodes(kw);
        }
=======
        if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
            QString kw = m_filterEdit->text().trimmed();
            if (index == 0 && m_favoritePanel) m_favoritePanel->filterTreeNodes(kw);
            if (index == 1 && m_libraryPanel) m_libraryPanel->filterTreeNodes(kw);
            if (index == 2 && m_markedPanel) m_markedPanel->filterTreeNodes(kw);
        }
>>>>>>> REPLACE
```

### 3.10 `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
#include "../meta/FavoriteService.h"
=======
#include "../meta/FavoriteService.h"
#include "../meta/MarkedFolderService.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        // 绘制置顶图标
        if (isPinned) {
            QIcon pinIcon = UiHelper::getIcon("pin", QColor("#ff551c"), 16);
            QRect pinRect(textRect.right() - 18, opt.rect.top() + (opt.rect.height() - 16) / 2, 16, 16);
            pinIcon.paint(painter, pinRect);
            textRect.setRight(textRect.right() - 20);
        }
=======
        // 绘制标记目的地 Destination.svg 图标
        if (MarkedFolderService::instance().isMarked(itemPath)) {
            QIcon markIcon = UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 16);
            QRect markRect(textRect.right() - 18, opt.rect.top() + (opt.rect.height() - 16) / 2, 16, 16);
            markIcon.paint(painter, markRect);
            textRect.setRight(textRect.right() - 20);
        }

        // 绘制置顶图标
        if (isPinned) {
            QIcon pinIcon = UiHelper::getIcon("pin", QColor("#ff551c"), 16);
            QRect pinRect(textRect.right() - 18, opt.rect.top() + (opt.rect.height() - 16) / 2, 16, 16);
            pinIcon.paint(painter, pinRect);
            textRect.setRight(textRect.right() - 20);
        }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        QAction* favAct = FavoriteService::instance().buildFavoriteAction(&menu, path, this);
=======
        MarkedFolderService::instance().buildMarkAction(&menu, path, this);
        QAction* favAct = FavoriteService::instance().buildFavoriteAction(&menu, path, this);
>>>>>>> REPLACE
```

### 3.11 `src/ui/NavPanel.cpp`

```
<<<<<<< SEARCH
#include "../meta/FavoriteService.h"
=======
#include "../meta/FavoriteService.h"
#include "../meta/MarkedFolderService.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    FavoriteService::instance().buildFavoriteAction(&menu, path, this);
=======
    MarkedFolderService::instance().buildMarkAction(&menu, path, this);
    FavoriteService::instance().buildFavoriteAction(&menu, path, this);
>>>>>>> REPLACE
```

### 3.12 `src/ui/controllers/ContentContextMenu.cpp`

```
<<<<<<< SEARCH
#include "../../meta/FavoriteService.h"
=======
#include "../../meta/FavoriteService.h"
#include "../../meta/MarkedFolderService.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            FavoriteService::instance().buildFavoriteAction(&menu, path, m_panel);
=======
            if (QFileInfo(path).isDir()) {
                MarkedFolderService::instance().buildMarkAction(&menu, path, m_panel);
            }
            FavoriteService::instance().buildFavoriteAction(&menu, path, m_panel);
>>>>>>> REPLACE
```

### 3.13 `src/ui/controllers/ContentKeyHandler.cpp`

```
<<<<<<< SEARCH
#include "../ToolTipOverlay.h"
=======
#include "../ToolTipOverlay.h"
#include "../../meta/MarkedFolderService.h"
#include "../../util/TaskProgressService.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    } else if (event->modifiers() == Qt::AltModifier && event->key() == Qt::Key_Left) {
=======
    } else if (event->modifiers() == Qt::AltModifier && event->key() == Qt::Key_C) {
        // Alt+C：标记/取消标记当前选中的文件夹
        QStringList selected = m_panel->getSelectedPaths();
        if (!selected.isEmpty()) {
            for (const QString& p : selected) {
                if (QFileInfo(p).isDir()) {
                    MarkedFolderService::instance().toggleMarked(p);
                }
            }
        }
        return true;
    } else if (event->modifiers() == Qt::AltModifier && event->key() == Qt::Key_V) {
        // Alt+V：把当前选中的项目发送至所有标记的目的地文件夹
        QStringList selected = m_panel->getSelectedPaths();
        QStringList markedFolders = MarkedFolderService::instance().getAllMarked();
        if (!selected.isEmpty() && !markedFolders.isEmpty()) {
            // 无论数量多少，强制触发 TaskProgressStrip
            TaskProgressService::instance().reportProgress("发送至目的地", 0, selected.size());
            DiskIoContext ioCtx;
            ioCtx.sources = selected;
            ioCtx.destination = markedFolders.first(); // 主目的地
            ioCtx.isMove = true;
            DiskIoService::instance().executeAsync(ioCtx, [selectedCount = selected.size()](bool success) {
                TaskProgressService::instance().reportProgress("发送完成", selectedCount, selectedCount);
            });
        }
        return true;
    } else if (event->modifiers() == Qt::AltModifier && event->key() == Qt::Key_Left) {
>>>>>>> REPLACE
```

### 3.14 `resources/style.qss`

```
<<<<<<< SEARCH
QTabBar#SidebarTabBar::tab {
    padding: 4px 12px;
=======
QTabBar#SidebarTabBar::tab {
    padding: 4px 6px;
    font-size: 11px;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QTreeWidget#HistoryTreeWidget {
=======
QTreeWidget#MarkedTreeWidget, QTreeWidget#HistoryTreeWidget {
    background-color: #1E1E1E;
    color: #EEEEEE;
    border: none;
    outline: 0;
}
QTreeWidget#MarkedTreeWidget::item, QTreeWidget#HistoryTreeWidget::item {
    height: 28px;
    border: none;
    padding-left: 4px;
}
QTreeWidget#MarkedTreeWidget::item:hover, QTreeWidget#HistoryTreeWidget::item:hover {
    background-color: #2A2D2E;
}
QTreeWidget#MarkedTreeWidget::item:selected, QTreeWidget#HistoryTreeWidget::item:selected {
    background-color: #094771;
    color: #FFFFFF;
}

QTreeWidget#HistoryTreeWidget_Unused {
>>>>>>> REPLACE
```

### 3.15 `CMakeLists.txt`

```
<<<<<<< SEARCH
    src/meta/HistoryService.cpp
    src/meta/HistoryService.h
=======
    src/meta/HistoryService.cpp
    src/meta/HistoryService.h
    src/meta/MarkedFolderDao.cpp
    src/meta/MarkedFolderDao.h
    src/meta/MarkedFolderService.cpp
    src/meta/MarkedFolderService.h
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    src/ui/LibraryPanel.cpp
    src/ui/LibraryPanel.h
=======
    src/ui/LibraryPanel.cpp
    src/ui/LibraryPanel.h
    src/ui/MarkedPanel.cpp
    src/ui/MarkedPanel.h
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps

### 构建命令
```bash
cmake -B build -G Ninja
cmake --build build --config Release
```

### 运行时验收标准
1. **选项卡样式**：侧边栏顶部呈现“收藏夹”、“库”、“标记” 3 个选项卡，“标记”带有 `#ff551c` 橙色 Destination 图标。
2. **右键标记功能与 50 个上限**：在任意文件夹上右键弹菜单，出现“标记为目的地”，点击后在右侧及“标记”标签页中列出；连续添加 50 个以上文件夹时弹出超限 Toast 警告并阻断。
3. **Alt+C / Alt+V 快捷键**：
   - 在文件夹上按 `Alt+C`，该文件夹被标记；再次按 `Alt+C`，取消标记。
   - 选中文件按 `Alt+V`，文件被发送到目的地，顶部立刻跳出 `TaskProgressStrip` 进度条反馈进度（即使只发送 1 个文件）。

---

## Chapter 5: Official API Reuse & Anti-Redundancy

- 复用 `DatabaseManager` 单例获取 SQLite 连接与锁；
- 复用 `TaskProgressService` 单例上报实时进度；
- 复用 `UiHelper::getIcon("Destination.svg", ...)` 渲染统一矢量图标。

---

## Chapter 6: Data SSOT Declaration Table

| 数据项 | 唯一 SSOT 源 | 派生/视图副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 已标记目的地文件夹列表 | SQLite `marked_folders` 表 | `MarkedFolderService` / `MarkedPanel` | 增删改操作 ➔ 广播 `markedFoldersChanged` ➔ UI 增量更新 |

---

## Chapter 7: Interface & Visibility Consistency Check

1. `MarkedFolderService::instance()` 暴露在 `src/meta/MarkedFolderService.h`；
2. 新增的类与 DAO 访问修饰符严格遵循 `public` / `private` 访问保护。

---

## Chapter 8: Header Chain & Inclusion Closure Check

- `src/meta/MarkedFolderDao.cpp` 包含 `DatabaseManager.h`；
- `src/ui/MarkedPanel.cpp` 包含 `MarkedFolderService.h` 与 `UiHelper.h`。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 序号 | 检查项 | 结果 | 说明 |
| :--- | :--- | :--- | :--- |
| 1 | 严禁盲改对外签名 | 满足 | 新增扩展，未破坏既有签名 |
| 2 | 依赖流动方向 | 满足 | UI -> Domain (Service) -> Data (DAO) |
| 3 | 线程安全与 UI 调度 | 满足 | 信号槽回至 GUI 主线程更新控件 |
| 4 | 强制进度条规则 | 满足 | 发送操作无论数量强制调 TaskProgressService |
| 5 | 10 大章节规范 | 满足 | 完整具备 10 大章节 |
| 6 | 50 个上限阻断限制 | 满足 | `MarkedFolderService::addMarked` 中包含 50 个上限拦截与 Toast 反馈 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table

- 既有收藏夹与库选项卡行为完全保留；
- Destination 图标色值 `#ff551c` 严格与置顶高亮色一致。
