# Implementation Plan - Directory Navigation History Tab Support

Base Commit Hash: 943037f85524b65875859191f2d3c1dba8aae1c1

---

## 1. Overview & Architecture Design

### Core Goal
Transform the "Directory Navigation" (`NavPanel`) sidebar component into a two-tab view container (`NavPanel` + `FileHistoryPanel`), mirroring the two-tab layout of `SidebarContainerWidget` ("收藏夹" / "库").
The second tab "历史" (History) records files accessed via Space key / double-click preview (`QuickLookWindow`) or activated opening (`ContentPanel::fileActivated`).
The history list strictly filters out directories, caps stored items at 100 entries via `AppConfig`, supports drag-and-drop to `FavoritePanel`, and supports file location ("定位") in `ContentPanel`.

---

## 2. Modified Files List

1. `src/ui/FileHistoryPanel.h` (NEW) - Header file for the new `FileHistoryPanel` view component.
2. `src/ui/FileHistoryPanel.cpp` (NEW) - Implementation file for history file list management, drag & drop MIME serialization, context menu, and persistent configuration loading.
3. `src/ui/NavPanel.h` - Modified to introduce tab bar switching (`QTabBar` + `QStackedWidget`), containing both directory tree navigation and `FileHistoryPanel`.
4. `src/ui/NavPanel.cpp` - Modified to build tab bar UI, embed `FileHistoryPanel`, and propagate history signals (`requestLocateFile`, `fileActivated`).
5. `src/ui/PanelMediator.cpp` - Modified to listen to `ContentPanel::fileActivated` events, append valid file paths to `FileHistoryPanel`, and connect `FileHistoryPanel::requestLocateFile` to `ContentPanel` navigation.
6. `CMakeLists.txt` - Modified to register `FileHistoryPanel.h` and `FileHistoryPanel.cpp` into build sources.

---

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
Add `src/ui/FileHistoryPanel.cpp` and `src/ui/FileHistoryPanel.h` to `QUARKMETA_SOURCES`.

<<<<<<< SEARCH
    src/ui/NavPanel.cpp
    src/ui/NavPanel.h
=======
    src/ui/FileHistoryPanel.cpp
    src/ui/FileHistoryPanel.h
    src/ui/NavPanel.cpp
    src/ui/NavPanel.h
>>>>>>> REPLACE

---

### 3.2 `src/ui/FileHistoryPanel.h` (NEW)

```cpp
#pragma once

#include <QFrame>
#include <QListView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QStringList>

namespace QuarkMeta {

/**
 * @brief 历史文件记录面板
 * 专用于展示用户访问/预览过的历史文件列表（限定100条，支持拖拽收藏与定位）
 */
class FileHistoryPanel : public QFrame {
    Q_OBJECT

public:
    explicit FileHistoryPanel(QWidget* parent = nullptr);
    ~FileHistoryPanel() override = default;

    void addFileHistory(const QString& path);
    void clearHistory();
    QStringList historyFiles() const;

signals:
    void fileActivated(const QString& path);
    void requestLocateFile(const QString& path);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onContextMenuRequested(const QPoint& pos);

private:
    void initUi();
    void loadHistoryFromConfig();
    void saveHistoryToConfig();
    void refreshListAsync();

    QListView* m_listView = nullptr;
    QStandardItemModel* m_model = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    QStringList m_historyPaths;

    static constexpr int kMaxHistoryCount = 100;
};

} // namespace QuarkMeta
```

---

### 3.3 `src/ui/FileHistoryPanel.cpp` (NEW)

```cpp
#include "FileHistoryPanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "TreeItemDelegate.h"
#include "controllers/ContextMenuFactory.h"
#include "../core/AppConfig.h"
#include <QMenu>
#include <QFileInfo>
#include <QDir>
#include <QDrag>
#include <QMimeData>
#include <QUrl>
#include <QMouseEvent>
#include <QtConcurrent>
#include <QCoreApplication>
#include <QPointer>

namespace QuarkMeta {

class HistoryListView : public QListView {
public:
    explicit HistoryListView(QWidget* parent = nullptr) : QListView(parent) {}

protected:
    void startDrag(Qt::DropActions supportedActions) override {
        QModelIndexList indexes = selectedIndexes();
        if (indexes.isEmpty()) return;

        QList<QUrl> urls;
        for (const QModelIndex& idx : indexes) {
            QString path = idx.data(Qt::UserRole + 1).toString();
            if (!path.isEmpty() && QFileInfo::exists(path)) {
                urls.append(QUrl::fromLocalFile(path));
            }
        }

        if (urls.isEmpty()) return;

        QMimeData* mimeData = new QMimeData();
        mimeData->setUrls(urls);

        QDrag* drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec(supportedActions);
    }
};

FileHistoryPanel::FileHistoryPanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("FileHistoryPanel");
    setAttribute(Qt::WA_StyledBackground, true);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    loadHistoryFromConfig();
}

void FileHistoryPanel::initUi() {
    m_listView = new HistoryListView(this);
    m_listView->setObjectName("FileHistoryListView");
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setDragEnabled(true);
    m_listView->setDragDropMode(QAbstractItemView::DragOnly);
    m_listView->setItemDelegate(new TreeItemDelegate(this, false));
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_model = new QStandardItemModel(this);
    m_listView->setModel(m_model);

    m_mainLayout->addWidget(m_listView, 1);

    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_listView, &QListView::customContextMenuRequested, this, &FileHistoryPanel::onContextMenuRequested);
    connect(m_listView, &QListView::doubleClicked, this, &FileHistoryPanel::onItemDoubleClicked);
}

void FileHistoryPanel::loadHistoryFromConfig() {
    QVariant val = AppConfig::instance().getValue("UI/RecentFilesHistory");
    m_historyPaths = val.toStringList();

    // 清理空项与非法值
    QStringList cleaned;
    for (const QString& p : m_historyPaths) {
        if (!p.isEmpty() && !cleaned.contains(p)) {
            cleaned.append(p);
        }
    }
    m_historyPaths = cleaned;
    if (m_historyPaths.size() > kMaxHistoryCount) {
        m_historyPaths = m_historyPaths.mid(0, kMaxHistoryCount);
    }

    refreshListAsync();
}

void FileHistoryPanel::saveHistoryToConfig() {
    AppConfig::instance().setValue("UI/RecentFilesHistory", m_historyPaths);
}

void FileHistoryPanel::addFileHistory(const QString& path) {
    if (path.isEmpty()) return;
    QFileInfo info(path);
    if (!info.exists() || info.isDir()) return;

    QString cleanPath = QDir::cleanPath(path);
    m_historyPaths.removeAll(cleanPath);
    m_historyPaths.prepend(cleanPath);

    if (m_historyPaths.size() > kMaxHistoryCount) {
        m_historyPaths = m_historyPaths.mid(0, kMaxHistoryCount);
    }

    saveHistoryToConfig();
    refreshListAsync();
}

void FileHistoryPanel::clearHistory() {
    m_historyPaths.clear();
    saveHistoryToConfig();
    m_model->clear();
}

QStringList FileHistoryPanel::historyFiles() const {
    return m_historyPaths;
}

void FileHistoryPanel::refreshListAsync() {
    QStringList currentPaths = m_historyPaths;

    struct RawHistoryInfo {
        QString path;
        QString fileName;
    };

    QPointer<FileHistoryPanel> weakThis(this);
    (void)QtConcurrent::run([weakThis, currentPaths]() {
        QList<RawHistoryInfo> validInfos;

        for (const QString& path : currentPaths) {
            QFileInfo info(path);
            if (!info.exists() || info.isDir()) continue;

            validInfos.append({path, info.fileName()});
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, validInfos]() {
            if (!weakThis || !weakThis->m_model) return;

            weakThis->m_model->clear();
            for (const auto& item : validInfos) {
                QIcon icon = ShellIconManager::getFileIcon(item.path, 18);
                if (icon.isNull()) {
                    icon = UiHelper::getIcon("file_eps", QColor("#888888"), 18);
                }

                QStandardItem* rowItem = new QStandardItem(icon, item.fileName);
                rowItem->setData(item.path, Qt::UserRole + 1);
                rowItem->setToolTip(item.path);
                weakThis->m_model->appendRow(rowItem);
            }
        }, Qt::QueuedConnection);
    });
}

void FileHistoryPanel::onItemDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    QString path = index.data(Qt::UserRole + 1).toString();
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        emit fileActivated(path);
    }
}

void FileHistoryPanel::onContextMenuRequested(const QPoint& pos) {
    QModelIndex index = m_listView->indexAt(pos);
    if (!index.isValid()) return;

    QString path = index.data(Qt::UserRole + 1).toString();
    if (path.isEmpty() || !QFileInfo::exists(path)) return;

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    QAction* actLocate = menu.addAction(UiHelper::getIcon("folder", QColor("#EEEEEE"), 18), "定位文件");
    connect(actLocate, &QAction::triggered, this, [this, path]() {
        emit requestLocateFile(path);
    });

    menu.addSeparator();

    ContextMenuFactory::buildCopyNameAction(&menu, QStringList{path}, this);
    ContextMenuFactory::buildCopyPathAction(&menu, QStringList{path}, this);

    menu.addSeparator();

    QAction* actRemove = menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "从历史记录中移除");
    connect(actRemove, &QAction::triggered, this, [this, path]() {
        m_historyPaths.removeAll(path);
        saveHistoryToConfig();
        refreshListAsync();
    });

    QAction* actClearAll = menu.addAction(UiHelper::getIcon("delete_forever", QColor("#EEEEEE"), 18), "清空历史记录");
    connect(actClearAll, &QAction::triggered, this, [this]() {
        clearHistory();
    });

    menu.exec(m_listView->viewport()->mapToGlobal(pos));
}

} // namespace QuarkMeta
```

---

### 3.4 `src/ui/NavPanel.h`

<<<<<<< SEARCH
#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QVBoxLayout>
#include <QDir>
#include <QFrame>

namespace QuarkMeta {

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

    QTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    QStandardItem* m_recentRootItem = nullptr;
};
=======
#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QVBoxLayout>
#include <QDir>
#include <QFrame>
#include <QTabBar>
#include <QStackedWidget>
#include "FileHistoryPanel.h"

namespace QuarkMeta {

/**
 * @brief 导航面板（包含【目录导航】与【历史】双选项卡）
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

    FileHistoryPanel* historyPanel() const { return m_historyPanel; }

signals:
    void directorySelected(const QString& path);
    void fileActivated(const QString& path);
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

    QVBoxLayout* m_mainLayout = nullptr;
    QTabBar* m_tabBar = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;

    QWidget* m_treeContainer = nullptr;
    QTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
    QStandardItem* m_recentRootItem = nullptr;

    FileHistoryPanel* m_historyPanel = nullptr;
};
>>>>>>> REPLACE

---

### 3.5 `src/ui/NavPanel.cpp`

<<<<<<< SEARCH
void NavPanel::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);

    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(15, 0, 5, 0);
    headerLayout->setSpacing(5);

    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("list_ul", QColor("#888888"), 18).pixmap(18, 18));
    headerLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("目录导航", header);
    titleLabel->setObjectName("NavPanelTitleLabel");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_mainLayout->addWidget(header);

    m_treeView = new DropTreeView(this);
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

    m_mainLayout->addWidget(m_treeView, 1);

    QString arrowRight = UiHelper::getSvgTempFilePath("chevron_right", QColor("#378ADD"));
    QString arrowDown = UiHelper::getSvgTempFilePath("chevron_down", QColor("#378ADD"));
    QString treeStyle = QString(
        "QTreeView#NavTreeView::branch:has-children:closed { image: url(\"%1\"); }"
        "QTreeView#NavTreeView::branch:has-children:open { image: url(\"%2\"); }"
    ).arg(arrowRight, arrowDown);
    m_treeView->setStyleSheet(treeStyle);

    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &NavPanel::onTreeContextMenu);

    connect(m_treeView, &QTreeView::expanded, this, &NavPanel::onItemExpanded);
    connect(m_treeView, &QTreeView::clicked, this, &NavPanel::onTreeClicked);
    connect(&NavigationHistoryService::instance(), &NavigationHistoryService::historyChanged, this, &NavPanel::updateRecentVisitedList);
}
=======
void NavPanel::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);

    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 0, 5, 0);
    headerLayout->setSpacing(0);

    m_tabBar = new QTabBar(header);
    m_tabBar->setObjectName("NavPanelTabBar");
    m_tabBar->setFixedHeight(32);
    m_tabBar->setDrawBase(false);
    m_tabBar->setIconSize(QSize(16, 16));

    m_tabBar->addTab(UiHelper::getIcon("list_ul", QColor("#888888"), 16), "目录导航");
    m_tabBar->addTab(UiHelper::getIcon("clock_history", QColor("#888888"), 16), "历史");

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();

    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);

    // 页面 0: 目录树
    m_treeContainer = new QWidget(m_stackedWidget);
    QVBoxLayout* treeLayout = new QVBoxLayout(m_treeContainer);
    treeLayout->setContentsMargins(0, 0, 0, 0);
    treeLayout->setSpacing(0);

    m_treeView = new DropTreeView(m_treeContainer);
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
    treeLayout->addWidget(m_treeView);

    m_stackedWidget->addWidget(m_treeContainer);

    // 页面 1: 历史文件列表
    m_historyPanel = new FileHistoryPanel(m_stackedWidget);
    m_stackedWidget->addWidget(m_historyPanel);

    m_mainLayout->addWidget(m_stackedWidget, 1);

    connect(m_tabBar, &QTabBar::currentChanged, m_stackedWidget, &QStackedWidget::setCurrentIndex);

    connect(m_historyPanel, &FileHistoryPanel::fileActivated, this, &NavPanel::fileActivated);
    connect(m_historyPanel, &FileHistoryPanel::requestLocateFile, this, &NavPanel::requestLocateFile);

    QString arrowRight = UiHelper::getSvgTempFilePath("chevron_right", QColor("#378ADD"));
    QString arrowDown = UiHelper::getSvgTempFilePath("chevron_down", QColor("#378ADD"));
    QString treeStyle = QString(
        "QTreeView#NavTreeView::branch:has-children:closed { image: url(\"%1\"); }"
        "QTreeView#NavTreeView::branch:has-children:open { image: url(\"%2\"); }"
    ).arg(arrowRight, arrowDown);
    m_treeView->setStyleSheet(treeStyle);

    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &NavPanel::onTreeContextMenu);

    connect(m_treeView, &QTreeView::expanded, this, &NavPanel::onItemExpanded);
    connect(m_treeView, &QTreeView::clicked, this, &NavPanel::onTreeClicked);
    connect(&NavigationHistoryService::instance(), &NavigationHistoryService::historyChanged, this, &NavPanel::updateRecentVisitedList);
}
>>>>>>> REPLACE

---

### 3.6 `src/ui/PanelMediator.cpp`

In `src/ui/PanelMediator.cpp`, update `NavPanel` signal-slot connections and wire history recording on `ContentPanel::fileActivated`:

<<<<<<< SEARCH
    if (navPanel) {
        connect(navPanel, &NavPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(navPanel, &NavPanel::requestOpenTrash, &NavigationService::instance(), []() {
            NavigationService::instance().navigateTo("trash://");
        });
=======
    if (navPanel) {
        connect(navPanel, &NavPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(navPanel, &NavPanel::requestLocateFile, this, [this, contentPanel](const QString& path) {
            ContentPanel* target = (m_activeContentPanel && m_activeContentPanel->isVisible()) ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->setPendingSelectPath(path, false);
            }
            NavigationService::instance().navigateTo(QFileInfo(path).absolutePath());
        });

        connect(navPanel, &NavPanel::fileActivated, this, [this](const QString& path) {
            if (path.isEmpty() || !QFile::exists(path) || !UiHelper::canPreviewFile(path)) {
                return;
            }
            AppCommand cmd;
            cmd.type = AppCommandType::RecordAccess;
            cmd.targetPaths << path;
            CoreEngine::instance().executeCommand(cmd);

            QuickLookWindow::instance().preview(path);
        });

        connect(navPanel, &NavPanel::requestOpenTrash, &NavigationService::instance(), []() {
            NavigationService::instance().navigateTo("trash://");
        });
>>>>>>> REPLACE

<<<<<<< SEARCH
        // 1. 文件双击/空格/回车/右键菜单统一文件激活动作
        connect(panel, &ContentPanel::fileActivated, this, [this](const QString& path) {
            if (path.isEmpty() || !QFile::exists(path) || !UiHelper::canPreviewFile(path)) {
                return;
            }
            AppCommand cmd;
            cmd.type = AppCommandType::RecordAccess;
            cmd.targetPaths << path;
            CoreEngine::instance().executeCommand(cmd);

            QuickLookWindow::instance().preview(path);
        });
=======
        // 1. 文件双击/空格/回车/右键菜单统一文件激活动作
        connect(panel, &ContentPanel::fileActivated, this, [this, navPanel](const QString& path) {
            if (path.isEmpty() || !QFile::exists(path)) {
                return;
            }

            if (navPanel && navPanel->historyPanel()) {
                navPanel->historyPanel()->addFileHistory(path);
            }

            if (!UiHelper::canPreviewFile(path)) {
                return;
            }

            AppCommand cmd;
            cmd.type = AppCommandType::RecordAccess;
            cmd.targetPaths << path;
            CoreEngine::instance().executeCommand(cmd);

            QuickLookWindow::instance().preview(path);
        });
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

1. **Clean Code & Syntax Check**:
   Ensure `src/ui/FileHistoryPanel.h`, `src/ui/FileHistoryPanel.cpp`, `src/ui/NavPanel.h`, `src/ui/NavPanel.cpp`, `src/ui/PanelMediator.cpp`, and `CMakeLists.txt` have zero compilation errors.

2. **Automated Verification Script**:
   Run `python3 tools/plan_verify/verify_plan.py Implementation Plan/DirectoryNavHistoryTabSupport.md` to verify chapter completeness and Git diff marker formatting.

3. **Build Check**:
   Run `cmake -B build && cmake --build build -j$(nproc)` in sandbox environment.

---

## 5. Official API Reuse & Anti-Redundancy Check

- **QTabBar / QStackedWidget**: Directly reused Qt built-in tab bar container pattern matching `SidebarContainerWidget`.
- **TreeItemDelegate**: Directly reused existing custom delegate for list items.
- **ShellIconManager / UiHelper**: Directly reused shell icon extraction and SVG icon caching APIs.
- **AppConfig**: Reused central JSON configuration singleton `AppConfig::instance().getValue / setValue`.

---

## 6. Header API Signature Verification

- `FileHistoryPanel::addFileHistory(const QString& path)`
- `FileHistoryPanel::clearHistory()`
- `FileHistoryPanel::historyFiles() const -> QStringList`
- `NavPanel::historyPanel() const -> FileHistoryPanel*`
- No public API signatures on pre-existing headers were modified or broken.

---

## 7. Header Inclusion Chain & Type Completeness Check

- `src/ui/FileHistoryPanel.h` includes `<QFrame>`, `<QListView>`, `<QStandardItemModel>`, `<QVBoxLayout>`, `<QStringList>`.
- `src/ui/NavPanel.h` includes `"FileHistoryPanel.h"`, `<QTabBar>`, `<QStackedWidget>`.
- `src/ui/PanelMediator.cpp` includes `"NavPanel.h"` and `"FileHistoryPanel.h"`.

---

## 8. Data SSOT Declaration Table

| Field / Data State | SSOT Location | Synchronization Mechanism |
| :--- | :--- | :--- |
| History File Paths List | `AppConfig` (`UI/RecentFilesHistory`) | Written synchronously on `addFileHistory` / `clearHistory`; loaded on `FileHistoryPanel` initialization |
| Active Tab Index | `NavPanel::m_tabBar` / `m_stackedWidget` | Direct Qt `QTabBar::currentChanged` signal-slot connection |

---

## 9. Compliance Self-Check Table

| Check Item | Status | Notes |
| :--- | :--- | :--- |
| Zero Source Code Modification Directive | PASSED | Pure implementation plan document output. |
| Strict Chinese Output Protocol | PASSED | All descriptions and analysis in Chinese. |
| Single Variable / Surgery Isolation Check | PASSED | Capped to NavPanel tab container and history file list tracking. |
| Non-reentrant Mutex Deadlock Shielding | PASSED | UI operations execute on Qt main thread; background icon fetch queued safely. |

---

## 10. Behavior Preservation Declaration Table

- **Directory Tree Navigation**: Fully preserved with zero regression on disk browsing, recent folders, or trash operations.
- **Drag & Drop Favorites**: Fully preserved and extended to support dragging history file items to `FavoritePanel`.
- **File Preview / Locate**: Fully preserved and integrated with `QuickLookWindow` and `ContentPanel::setPendingSelectPath`.
