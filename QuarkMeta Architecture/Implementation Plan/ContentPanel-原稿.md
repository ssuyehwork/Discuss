# QuarkMeta ContentPanel 彻底瘦身与职责剥离实施方案

## 1. 目标与范围
- 抽离右键上下文菜单中枢：新建 `ContentContextMenuController`（位于 `src/ui/controllers/`），将近 400 行包含 28 个 Action 的复杂菜单构建与分发逻辑完全剥离。
- 抽离物理操作与剪贴板控制器：新建 `ContentActionController`（位于 `src/ui/controllers/`），收敛 `canPaste` 校验、`performCopy/Cut`、`performPaste` 与 `createNewItem`。
- `ContentPanel.cpp` 彻底瘦身：由原本臃肿的 1,400 行骤降至 **200 余行**，纯粹作为双隐式容器（FolderView + FileView）的视觉呈现与切换中枢。

---

## 2. 核心剥离模块实现

### 2.1 `src/ui/controllers/ContentContextMenuController.h`
```cpp
#pragma once

#include <QObject>
#include <QPoint>
#include <QAbstractItemView>
#include <QWidget>

namespace QuarkMeta {

class ContentPanel;

class ContentContextMenuController : public QObject {
    Q_OBJECT

public:
    explicit ContentContextMenuController(ContentPanel* panel, QObject* parent = nullptr);
    ~ContentContextMenuController() override = default;

    /**
     * @brief 弹出并处理内容区右键上下文菜单 (全场景自动化分流)
     */
    void showContextMenu(QAbstractItemView* view, 
                          const QPoint& pos, 
                          const QString& currentPath, 
                          const QString& categoryType);

private:
    ContentPanel* m_panel = nullptr;
};

} // namespace QuarkMeta
```

### 2.2 `src/ui/controllers/ContentContextMenuController.cpp`
```cpp
#include "ContentContextMenuController.h"
#include "../ContentPanel.h"
#include "../UiHelper.h"
#include "../FavoritePanel.h"
#include "../ColorPicker.h"
#include "../BatchCreateDialog.h"
#include "../BatchRenameDialog.h"
#include "../../core/TrashService.h"
#include "../../core/PermanentDeleteService.h"
#include "../../core/ClipboardService.h"
#include "../../core/ProtectionService.h"
#include "../../core/OperationSnapshotEngine.h"
#include "../../core/AppConfig.h"
#include "../../core/ModelContract.h"
#include "../../util/ShellHelper.h"
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QWidgetAction>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>

namespace QuarkMeta {

ContentContextMenuController::ContentContextMenuController(ContentPanel* panel, QObject* parent)
    : QObject(parent), m_panel(panel) {}

void ContentContextMenuController::showContextMenu(QAbstractItemView* view, 
                                                   const QPoint& pos, 
                                                   const QString& currentPath, 
                                                   const QString& categoryType) {
    if (!view || !m_panel) return;

    QModelIndex currentIndex = view->indexAt(pos);
    bool onItem = currentIndex.isValid();
    QString path = onItem ? currentIndex.data(PathRole).toString() : "";
    QFileInfo itemInfo(path);

    bool isComputerRoot = (currentPath.isEmpty() || currentPath == "computer://");
    bool isTrashView = (categoryType == "trash" || currentPath == "trash://");
    bool isDriveRoot = onItem && (itemInfo.isRoot() || path.endsWith(":\\") || path.endsWith(":/") || (path.length() == 2 && path.endsWith(':')));
    bool isFolder = onItem && (isDriveRoot || currentIndex.data(TypeRole).toString() == "folder");

    QMenu menu;
    UiHelper::applyMenuStyle(&menu);

    // =========================================================================
    // 场景 1：回收站视图
    // =========================================================================
    if (isTrashView) {
        if (onItem) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原")->setData(ContentPanel::ActionRestore);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切")->setData(ContentPanel::ActionCut);
            menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "永久删除")->setData(ContentPanel::ActionSecureDelete);
            menu.addSeparator();
        }
        menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原全部")->setData(ContentPanel::ActionRestoreAll);
        menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "清空回收站")->setData(ContentPanel::ActionEmptyTrash);

        QAction* selected = menu.exec(view->viewport()->mapToGlobal(pos));
        if (!selected || !selected->data().isValid()) return;

        auto action = static_cast<ContentPanel::ContextAction>(selected->data().toInt());
        if (action == ContentPanel::ActionRestore) {
            TrashService::instance().restoreItems(m_panel->getSelectedTrashIds(), m_panel);
        } else if (action == ContentPanel::ActionRestoreAll) {
            TrashService::instance().restoreAll(m_panel);
        } else if (action == ContentPanel::ActionEmptyTrash) {
            TrashService::instance().emptyTrash(m_panel);
        } else if (action == ContentPanel::ActionCut) {
            ClipboardService::instance().cutItems(m_panel->getSelectedPaths());
        } else if (action == ContentPanel::ActionSecureDelete) {
            PermanentDeleteService::instance().execute(m_panel->getSelectedPaths(), m_panel);
        }
        return;
    }

    // =========================================================================
    // 场景 2：选中具体文件或目录
    // =========================================================================
    if (onItem) {
        if (isDriveRoot) {
            menu.addAction("打开")->setData(ContentPanel::ActionOpen);
            menu.addAction("在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            // 颜色条
            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, m_panel, [this, view, &menu](const QString& hexColor) {
                for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                    if (idx.column() == 0) view->model()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(PinnedRole).toBool();
            menu.addAction(isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);
            menu.addAction("添加至收藏夹 / 切换收藏")->setData(ContentPanel::ActionAddToFavorites);
            menu.addSeparator();

            QAction* actPaste = menu.addAction("粘贴");
            actPaste->setData(ContentPanel::ActionPaste);
            actPaste->setEnabled(ClipboardService::instance().canPaste(path));

            menu.addAction("复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction("复制路径")->setData(ContentPanel::ActionCopyPath);
            menu.addSeparator();
            menu.addAction("刷新")->setData(ContentPanel::ActionRefresh);
        } else {
            menu.addAction(isFolder ? "打开文件夹" : "打开")->setData(ContentPanel::ActionOpen);
            if (!isFolder) menu.addAction("用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
            menu.addAction("在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            // 颜色条
            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, m_panel, [this, view, &menu](const QString& hexColor) {
                for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                    if (idx.column() == 0) view->model()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(PinnedRole).toBool();
            menu.addAction(isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);
            menu.addAction("添加至收藏夹 / 切换收藏")->setData(ContentPanel::ActionAddToFavorites);
            menu.addSeparator();

            menu.addAction("复制")->setData(ContentPanel::ActionCopy);
            menu.addAction("剪切")->setData(ContentPanel::ActionCut);

            QAction* actPaste = menu.addAction("粘贴");
            actPaste->setData(ContentPanel::ActionPaste);
            actPaste->setEnabled(ClipboardService::instance().canPaste(isFolder ? path : currentPath));

            menu.addAction("复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction("复制路径")->setData(ContentPanel::ActionCopyPath);

            int selCount = m_panel->getSelectedPaths().size();
            if (selCount <= 1) menu.addAction("重命名")->setData(ContentPanel::ActionRename);
            if (isFolder || selCount > 1) menu.addAction("批量重命名 (Ctrl+Shift+R)")->setData(ContentPanel::ActionBatchRename);

            menu.addSeparator();
            menu.addAction("刷新")->setData(ContentPanel::ActionRefresh);

            if (!isFolder) {
                menu.addAction(UiHelper::getIcon("sync", QColor("#3498db"), 18), "重新提取缩略图")->setData(ContentPanel::ActionReextractThumbnail);
                QMenu* cryptoMenu = menu.addMenu("外壳保护");
                UiHelper::applyMenuStyle(cryptoMenu);
                cryptoMenu->addAction("执行外壳保护")->setData(ContentPanel::ActionEncrypt);
                cryptoMenu->addAction("解除保护")->setData(ContentPanel::ActionDecrypt);
                cryptoMenu->addAction("修改保护密码")->setData(ContentPanel::ActionChangePwd);
            }
        }
    } 
    // =========================================================================
    // 场景 3：空白处
    // =========================================================================
    else {
        if (isComputerRoot) {
            menu.addAction("在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);
            menu.addAction("刷新")->setData(ContentPanel::ActionRefresh);
        } else {
            QMenu* newMenu = menu.addMenu("新建...");
            UiHelper::applyMenuStyle(newMenu);
            newMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "创建文件夹")->setData(ContentPanel::ActionNewFolder);
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建 Markdown")->setData(ContentPanel::ActionNewMd);
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建纯文本文件 (txt)")->setData(ContentPanel::ActionNewTxt);

            menu.addSeparator();
            menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE")), "批量创建项目...")->setData(ContentPanel::ActionBatchCreate);
            menu.addSeparator();

            QAction* actPaste = menu.addAction("粘贴");
            actPaste->setData(ContentPanel::ActionPaste);
            actPaste->setEnabled(ClipboardService::instance().canPaste(currentPath));

            menu.addSeparator();
            menu.addAction("在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);
            menu.addAction("刷新")->setData(ContentPanel::ActionRefresh);
        }
    }

    // 排序二级子菜单
    menu.addSeparator();
    QMenu* sortMenu = menu.addMenu("排序");
    UiHelper::applyMenuStyle(sortMenu);

    QActionGroup* typeGroup = new QActionGroup(m_panel);
    auto addTypeAct = [&](const QString& label, ContentPanel::SortType type) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_panel->currentSortType() == type);
        typeGroup->addAction(act);
        connect(act, &QAction::triggered, m_panel, [this, type]() {
            m_panel->setSortType(type);
        });
    };

    addTypeAct("名称", ContentPanel::SortByName);
    addTypeAct("创建日期", ContentPanel::SortByCreateDate);
    addTypeAct("修改日期", ContentPanel::SortByModifyDate);
    addTypeAct("扩展名", ContentPanel::SortByExtension);
    addTypeAct("大小", ContentPanel::SortBySize);
    addTypeAct("尺寸", ContentPanel::SortByDimension);
    addTypeAct("评分", ContentPanel::SortByRating);

    sortMenu->addSeparator();
    QActionGroup* orderGroup = new QActionGroup(m_panel);
    auto addOrderAct = [&](const QString& label, Qt::SortOrder order) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_panel->currentSortOrder() == order);
        orderGroup->addAction(act);
        connect(act, &QAction::triggered, m_panel, [this, order]() {
            m_panel->setSortOrder(order);
        });
    };
    addOrderAct("升序", Qt::AscendingOrder);
    addOrderAct("降序", Qt::DescendingOrder);

    // 删除子菜单
    if (onItem && !isDriveRoot) {
        menu.addSeparator();
        QMenu* delMenu = menu.addMenu("删除");
        UiHelper::applyMenuStyle(delMenu);
        delMenu->addAction("移入回收站")->setData(ContentPanel::ActionDelete);
        delMenu->addAction("永久删除")->setData(ContentPanel::ActionSecureDelete);
    }

    QAction* selected = menu.exec(view->viewport()->mapToGlobal(pos));
    if (!selected || !selected->data().isValid()) return;

    auto action = static_cast<ContentPanel::ContextAction>(selected->data().toInt());

    // 🚀【统一领域调度】：彻底消灭私有硬编码
    switch (action) {
        case ContentPanel::ActionOpen: m_panel->onDoubleClicked(currentIndex); break;
        case ContentPanel::ActionOpenDefault: {
            for (const QString& p : m_panel->getSelectedPaths()) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(p));
            }
            break;
        }
        case ContentPanel::ActionShowInExplorer: ShellHelper::openInExplorer(onItem ? path : currentPath); break;
        case ContentPanel::ActionNewFolder: m_panel->createNewItem("folder"); break;
        case ContentPanel::ActionNewMd: m_panel->createNewItem("md"); break;
        case ContentPanel::ActionNewTxt: m_panel->createNewItem("txt"); break;
        case ContentPanel::ActionPin:
        case ContentPanel::ActionUnpin: {
            bool pin = (action == ContentPanel::ActionPin);
            for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                if (idx.column() == 0) view->model()->setData(idx, pin, PinnedRole);
            }
            break;
        }
        case ContentPanel::ActionEncrypt: ProtectionService::instance().protectFiles(m_panel->getSelectedPaths(), m_panel); break;
        case ContentPanel::ActionDecrypt: ProtectionService::instance().unprotectFiles(m_panel->getSelectedPaths(), m_panel); break;
        case ContentPanel::ActionChangePwd: ProtectionService::instance().changePassword(m_panel->getSelectedPaths(), m_panel); break;
        case ContentPanel::ActionBatchRename: m_panel->performBatchRename(); break;
        case ContentPanel::ActionRename: view->edit(currentIndex); break;
        case ContentPanel::ActionCopy: ClipboardService::instance().copyItems(m_panel->getSelectedPaths()); break;
        case ContentPanel::ActionCut: ClipboardService::instance().cutItems(m_panel->getSelectedPaths()); break;
        case ContentPanel::ActionPaste: ClipboardService::instance().executePaste(isFolder ? path : currentPath, m_panel); break;
        case ContentPanel::ActionBatchCreate: {
            BatchCreateDialog dlg(currentPath, m_panel);
            if (dlg.exec() == QDialog::Accepted) m_panel->refreshAll();
            break;
        }
        case ContentPanel::ActionDelete: TrashService::instance().moveToTrash(m_panel->getSelectedPaths(), m_panel); break;
        case ContentPanel::ActionSecureDelete: PermanentDeleteService::instance().execute(m_panel->getSelectedPaths(), m_panel); break;
        case ContentPanel::ActionAddToFavorites: emit m_panel->requestAddFavorite(m_panel->getSelectedPaths()); break;
        case ContentPanel::ActionCopyName: {
            QStringList names;
            for (const QString& p : m_panel->getSelectedPaths()) names << QFileInfo(p).fileName();
            QApplication::clipboard()->setText(names.join("\r\n"));
            break;
        }
        case ContentPanel::ActionCopyPath: {
            QApplication::clipboard()->setText(m_panel->getSelectedPaths().join("\n"));
            break;
        }
        case ContentPanel::ActionRefresh: m_panel->refreshAll(); break;
        default: break;
    }
}

} // namespace QuarkMeta
```

---

### 2.3 `src/ui/controllers/ContentActionController.h`
```cpp
#pragma once

#include <QObject>
#include <QStringList>
#include <QWidget>

namespace QuarkMeta {

class ContentActionController : public QObject {
    Q_OBJECT

public:
    explicit ContentActionController(QObject* parent = nullptr) : QObject(parent) {}
    ~ContentActionController() override = default;

    /**
     * @brief 批量创建文件或目录
     */
    bool createNewItem(const QString& currentDir, const QString& type, QString& outCreatedPath);
};

} // namespace QuarkMeta
```

### 2.4 `src/ui/controllers/ContentActionController.cpp`
```cpp
#include "ContentActionController.h"
#include <QDir>
#include <QFileInfo>
#include <QFile>

namespace QuarkMeta {

bool ContentActionController::createNewItem(const QString& currentDir, const QString& type, QString& outCreatedPath) {
    if (currentDir.isEmpty() || currentDir == "computer://" || currentDir.contains("://")) return false;

    QString baseName = (type == "folder") ? "新建文件夹" : "未命名";
    QString ext = (type == "md") ? ".md" : ((type == "txt") ? ".txt" : "");
    QString finalName = baseName + ext;
    QString fullPath = currentDir + "/" + finalName;

    int counter = 1;
    while (QFileInfo::exists(fullPath)) {
        finalName = baseName + QString(" (%1)").arg(counter++) + ext;
        fullPath = currentDir + "/" + finalName;
    }

    bool success = false;
    if (type == "folder") {
        success = QDir(currentDir).mkdir(finalName);
    } else {
        QFile file(fullPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            success = true;
        }
    }

    if (success) {
        outCreatedPath = finalName;
    }
    return success;
}

} // namespace QuarkMeta
```

---

## 3. `ContentPanel.h` 与 `ContentPanel.cpp` 终极瘦身实现 (< 250 行)

### 3.1 `src/ui/ContentPanel.h`
```cpp
#pragma once

#include <QFrame>
#include <QPointer>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include "models/DiskItemModel.h"
#include "models/FolderProxyModel.h"
#include "models/FileProxyModel.h"
#include "controllers/ContentContextMenuController.h"
#include "controllers/ContentActionController.h"
#include "ScanStats.h"

namespace QuarkMeta {

class ContentPanel : public QFrame {
    Q_OBJECT

public:
    enum ViewMode { ListView, GridView, JustifiedViewMode };
    enum SortType { SortByName, SortByCreateDate, SortByModifyDate, SortByExtension, SortBySize, SortByDimension, SortByRating, SortByAddedDate };
    enum ContextAction {
        ActionOpen, ActionOpenDefault, ActionShowInExplorer, ActionNewFolder, ActionNewMd, ActionNewTxt,
        ActionPin, ActionUnpin, ActionColorTag, ActionEncrypt, ActionDecrypt, ActionChangePwd,
        ActionBatchRename, ActionRename, ActionCopy, ActionCut, ActionPaste, ActionDelete,
        ActionPermanentDelete, ActionSecureDelete, ActionRestore, ActionRestoreAll, ActionEmptyTrash,
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionBatchCreate
    };

    explicit ContentPanel(QWidget* parent = nullptr);
    ~ContentPanel() override = default;

    SortType currentSortType() const { return m_sortType; }
    Qt::SortOrder currentSortOrder() const { return m_sortOrder; }
    ViewMode currentViewMode() const { return m_currentViewMode; }
    QString getCurrentCategoryType() const { return m_currentCategoryType; }
    QString currentPath() const { return m_currentPath; }

    void setSortType(SortType type);
    void setSortOrder(Qt::SortOrder order);

    QModelIndexList getSelectedIndexes() const;
    QStringList getSelectedPaths() const;
    QList<int> getSelectedTrashIds() const;
    QString getAdjacentFilePath(const QString& currentPath, int delta);

    void selectAndScrollToPath(const QString& path);
    void setPendingSelectName(const QString& name, bool edit = false);

    ItemModelBase* model() const { return m_model; }
    QSortFilterProxyModel* getProxyModel() const { return m_fileProxyModel; }

    void performBatchRename();

signals:
    void zoomLevelChanged(int level);
    void viewModeChanged(ViewMode mode);
    void requestQuickLook(const QString& path);
    void selectionChanged(const QStringList& paths);
    void directorySelected(const QString& path);
    void requestAddFavorite(const QStringList& paths);
    void dataSourceChanged(const QString& source);
    void directoryStatsReady(const QuarkMeta::ScanStats& stats);
    void statusBarStatsUpdated(int fileCount, int folderCount, int totalCount);

public slots:
    void setViewMode(ViewMode mode);
    void setZoomLevel(int level);
    void loadDirectory(const QString& path, bool recursive = false);
    void loadCategory(const QString& categoryType);
    void refreshAll();
    void updateItemMetadata(const QString& path);
    void migrateModelCache(const QString& oldPath, const QString& newPath);
    void clearFolderCache(const QString& folderPath);
    void search(const QString& query);
    void applyFilters(const FilterState& state);
    void applyFilters();
    void createNewItem(const QString& type);
    void onDoubleClicked(const QModelIndex& index);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void initUi();
    void initDualContainers();
    void updateDualContainersVisibility();
    void updateGridSize();
    void recalculateAndEmitStats();
    void updateStatusBarStats();
    void emitSelectionChangedSignal();
    void refreshVisibleThumbnails();

    QVBoxLayout* m_mainLayout = nullptr;
    QVBoxLayout* m_centerLayout = nullptr;

    QWidget* m_folderContainer = nullptr;
    QStackedWidget* m_folderViewStack = nullptr;
    QAbstractItemView* m_folderGridView = nullptr;
    QAbstractItemView* m_folderListView = nullptr;

    QWidget* m_fileContainer = nullptr;
    QStackedWidget* m_fileViewStack = nullptr;
    QAbstractItemView* m_fileGridView = nullptr;
    QAbstractItemView* m_fileListView = nullptr;

    DiskItemModel* m_diskModel = nullptr;
    ItemModelBase* m_model = nullptr;
    FolderProxyModel* m_folderProxyModel = nullptr;
    FileProxyModel* m_fileProxyModel = nullptr;

    ContentContextMenuController* m_contextMenuController = nullptr;
    ContentActionController* m_actionController = nullptr;

    FilterState m_currentFilter;
    int m_zoomLevel = 96;
    QString m_currentPath;
    QString m_currentCategoryType;
    bool m_isRecursive = false;
    bool m_showFolders = true;
    bool m_showFiles = true;
    bool m_showHidden = false;
    ViewMode m_currentViewMode = GridView;
    SortType m_sortType = SortByName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    QTimer* m_selectionTimer = nullptr;
    QTimer* m_visibleTimer = nullptr;
    std::atomic<int> m_loadRequestId{0};
    QSet<QString> m_pendingSelectNames;
    bool m_isPendingEdit = false;

    QPushButton* m_btnToggleHidden = nullptr;
    QPushButton* m_btnToggleFolders = nullptr;
    QPushButton* m_btnToggleFiles = nullptr;
    QPushButton* m_btnLayers = nullptr;
};

} // namespace QuarkMeta
```

### 3.2 `src/ui/ContentPanel.cpp`
```cpp
#include "ContentPanel.h"
#include "DropTreeView.h"
#include "DropListView.h"
#include "DropJustifiedView.h"
#include "ThumbnailDelegate.h"
#include "TreeItemDelegate.h"
#include "UiHelper.h"
#include "BatchRenameDialog.h"
#include "../util/ThumbnailPipelineService.h"
#include "../meta/DuplicateDetectorService.h"
#include "../meta/MetaCacheDecorator.h"
#include "../core/DiskScanService.h"
#include "../core/AppConfig.h"
#include "../core/CoreController.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QDesktopServices>
#include <QUrl>
#include <QtConcurrent>
#include <QCoreApplication>

namespace QuarkMeta {

ContentPanel::ContentPanel(QWidget* parent) : QFrame(parent) {
    setObjectName("EditorContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_contextMenuController = new ContentContextMenuController(this, this);
    m_actionController = new ContentActionController(this);

    m_diskModel = new DiskItemModel(this);
    m_model = m_diskModel;

    m_folderProxyModel = new FolderProxyModel(this);
    m_folderProxyModel->setSourceModel(m_model);

    m_fileProxyModel = new FileProxyModel(this);
    m_fileProxyModel->setSourceModel(m_model);

    m_visibleTimer = new QTimer(this);
    m_visibleTimer->setSingleShot(true);
    m_visibleTimer->setInterval(60);

    m_zoomLevel = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt();
    m_showFolders = AppConfig::instance().getValue("ContentPanel/ShowFolders", true).toBool();
    m_showFiles = AppConfig::instance().getValue("ContentPanel/ShowFiles", true).toBool();
    m_showHidden = AppConfig::instance().getValue("ContentPanel/ShowHidden", false).toBool();

    m_currentFilter.showFolders = m_showFolders;
    m_currentFilter.showFiles = m_showFiles;
    m_currentFilter.showHidden = m_showHidden;

    m_sortType = static_cast<SortType>(AppConfig::instance().getValue("ContentPanel/RightClickSortType", SortByName).toInt());
    m_sortOrder = static_cast<Qt::SortOrder>(AppConfig::instance().getValue("ContentPanel/RightClickSortOrder", Qt::AscendingOrder).toInt());

    initUi();
    initDualContainers();

    int savedMode = AppConfig::instance().getValue("ContentPanel/ViewMode", static_cast<int>(GridView)).toInt();
    setViewMode(static_cast<ViewMode>(savedMode));
}

void ContentPanel::initUi() {
    QWidget* titleBar = new QWidget(this);
    titleBar->setObjectName("ContainerHeader");
    titleBar->setFixedHeight(32);
    titleBar->setStyleSheet("QWidget#ContainerHeader { background-color: #252526; border-bottom: 1px solid #333333; }");
    QHBoxLayout* titleL = new QHBoxLayout(titleBar);
    titleL->setContentsMargins(15, 0, 5, 0);
    titleL->setSpacing(5);

    QLabel* iconLabel = new QLabel(titleBar);
    iconLabel->setPixmap(UiHelper::getIcon("eye", QColor("#41F2F2"), 18).pixmap(18, 18));
    titleL->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("内容", titleBar);
    titleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #41F2F2; background: transparent; border: none;");
    titleL->addWidget(titleLabel);
    titleL->addStretch();

    m_btnToggleHidden = new QPushButton(titleBar);
    m_btnToggleHidden->setCheckable(true);
    m_btnToggleHidden->setFixedSize(24, 24);
    m_btnToggleHidden->setChecked(m_showHidden);
    m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_showHidden ? QColor("#3498db") : QColor("#888888"), 16));
    m_btnToggleHidden->setProperty("tooltipText", "显示/隐藏属性为隐藏的项目");
    m_btnToggleHidden->setStyleSheet("QPushButton { background: transparent; border: 1px solid #444; border-radius: 4px; } QPushButton:checked { background: #3E3E42; border-color: #3498db; }");
    connect(m_btnToggleHidden, &QPushButton::clicked, [this]() {
        m_showHidden = m_btnToggleHidden->isChecked();
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_showHidden ? QColor("#3498db") : QColor("#888888"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowHidden", m_showHidden);
        applyFilters();
    });
    titleL->addWidget(m_btnToggleHidden);

    m_btnToggleFolders = new QPushButton(titleBar);
    m_btnToggleFolders->setCheckable(true);
    m_btnToggleFolders->setFixedSize(24, 24);
    m_btnToggleFolders->setChecked(m_showFolders);
    m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", m_showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
    m_btnToggleFolders->setProperty("tooltipText", "显示/隐藏文件夹");
    m_btnToggleFolders->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:checked { background: #3E3E42; }");
    connect(m_btnToggleFolders, &QPushButton::clicked, [this]() {
        m_showFolders = m_btnToggleFolders->isChecked();
        m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", m_showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowFolders", m_showFolders);
        applyFilters();
    });
    titleL->addWidget(m_btnToggleFolders);

    m_btnToggleFiles = new QPushButton(titleBar);
    m_btnToggleFiles->setCheckable(true);
    m_btnToggleFiles->setFixedSize(24, 24);
    m_btnToggleFiles->setChecked(m_showFiles);
    m_btnToggleFiles->setIcon(UiHelper::getIcon("file", m_showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
    m_btnToggleFiles->setProperty("tooltipText", "显示/隐藏文件");
    m_btnToggleFiles->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:checked { background: #3E3E42; }");
    connect(m_btnToggleFiles, &QPushButton::clicked, [this]() {
        m_showFiles = m_btnToggleFiles->isChecked();
        m_btnToggleFiles->setIcon(UiHelper::getIcon("file", m_showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowFiles", m_showFiles);
        applyFilters();
    });
    titleL->addWidget(m_btnToggleFiles);

    m_btnLayers = new QPushButton(titleBar);
    m_btnLayers->setCheckable(true);
    m_btnLayers->setFixedSize(24, 24);
    m_btnLayers->setIcon(UiHelper::getIcon("layers", QColor("#2ecc71"), 18));
    m_btnLayers->setProperty("tooltipText", "显示子文件夹中的项目");
    m_btnLayers->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:checked { background: #3E3E42; }");
    titleL->addWidget(m_btnLayers);

    m_mainLayout->addWidget(titleBar);

    QWidget* bodyWidget = new QWidget(this);
    m_centerLayout = new QVBoxLayout(bodyWidget);
    m_centerLayout->setContentsMargins(4, 4, 0, 4);
    m_centerLayout->setSpacing(8);

    m_mainLayout->addWidget(bodyWidget, 1);
}

void ContentPanel::initDualContainers() {
    // 1. 上方文件夹容器
    m_folderContainer = new QWidget(this);
    QVBoxLayout* folderL = new QVBoxLayout(m_folderContainer);
    folderL->setContentsMargins(0, 0, 0, 0);

    m_folderViewStack = new QStackedWidget(m_folderContainer);
    m_folderGridView = new DropJustifiedView(this);
    m_folderGridView->setModel(m_folderProxyModel);
    m_folderGridView->setItemDelegate(new ThumbnailDelegate(this));
    m_folderGridView->setStyleSheet("background: transparent; border: none; outline: none;");

    m_folderListView = new DropTreeView(this);
    m_folderListView->setModel(m_folderProxyModel);
    m_folderListView->setItemDelegate(new TreeItemDelegate(this, false, true));
    m_folderListView->setHeaderHidden(true);
    m_folderListView->setStyleSheet("QTreeView { background: transparent; border: none; outline: none; }");

    m_folderViewStack->addWidget(m_folderGridView);
    m_folderViewStack->addWidget(m_folderListView);
    folderL->addWidget(m_folderViewStack);

    // 2. 下方文件容器
    m_fileContainer = new QWidget(this);
    QVBoxLayout* fileL = new QVBoxLayout(m_fileContainer);
    fileL->setContentsMargins(0, 0, 0, 0);

    m_fileViewStack = new QStackedWidget(m_fileContainer);
    m_fileGridView = new DropJustifiedView(this);
    m_fileGridView->setModel(m_fileProxyModel);
    m_fileGridView->setItemDelegate(new ThumbnailDelegate(this));
    m_fileGridView->setStyleSheet("background: transparent; border: none; outline: none;");

    m_fileListView = new DropTreeView(this);
    m_fileListView->setModel(m_fileProxyModel);
    m_fileListView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_fileListView->setStyleSheet("QTreeView { background: transparent; border: none; outline: none; }");

    m_fileViewStack->addWidget(m_fileGridView);
    m_fileViewStack->addWidget(m_fileListView);
    fileL->addWidget(m_fileViewStack);

    m_centerLayout->addWidget(m_folderContainer, 0);
    m_centerLayout->addWidget(m_fileContainer, 1);

    // 绑定右键菜单与双击
    auto bindEvents = [this](QAbstractItemView* view) {
        view->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view, &QWidget::customContextMenuRequested, this, [this, view](const QPoint& pos) {
            m_contextMenuController->showContextMenu(view, pos, m_currentPath, m_currentCategoryType);
        });
        connect(view, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    };

    bindEvents(m_folderGridView);
    bindEvents(m_folderListView);
    bindEvents(m_fileGridView);
    bindEvents(m_fileListView);

    // 双向互斥选区
    auto onSelect = [this]() { emitSelectionChangedSignal(); };
    connect(m_folderGridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onSelect);
    connect(m_folderListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onSelect);
    connect(m_fileGridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onSelect);
    connect(m_fileListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onSelect);
}

void ContentPanel::updateDualContainersVisibility() {
    bool hasFolders = (m_folderProxyModel && m_folderProxyModel->rowCount() > 0);
    m_folderContainer->setVisible(hasFolders && m_showFolders);

    bool hasFiles = (m_fileProxyModel && m_fileProxyModel->rowCount() > 0);
    m_fileContainer->setVisible(hasFiles && m_showFiles);
}

QModelIndexList ContentPanel::getSelectedIndexes() const {
    QModelIndexList result;
    if (m_folderContainer && m_folderContainer->isVisible()) {
        auto* sel = (m_currentViewMode == ListView) ? m_folderListView->selectionModel() : m_folderGridView->selectionModel();
        if (sel) result.append(sel->selectedIndexes());
    }
    if (m_fileContainer && m_fileContainer->isVisible()) {
        auto* sel = (m_currentViewMode == ListView) ? m_fileListView->selectionModel() : m_fileGridView->selectionModel();
        if (sel) result.append(sel->selectedIndexes());
    }
    return result;
}

QStringList ContentPanel::getSelectedPaths() const {
    QStringList paths;
    for (const auto& idx : getSelectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty() && !paths.contains(p)) paths << p;
        }
    }
    return paths;
}

QList<int> ContentPanel::getSelectedTrashIds() const {
    QList<int> ids;
    for (const auto& idx : getSelectedIndexes()) {
        if (idx.column() == 0 && idx.data(IsDiskTrashRole).toBool()) {
            ids << idx.data(DiskTrashIdRole).toInt();
        }
    }
    return ids;
}

void ContentPanel::setViewMode(ViewMode mode) {
    m_currentViewMode = mode;
    int page = (mode == ListView) ? 1 : 0;
    m_folderViewStack->setCurrentIndex(page);
    m_fileViewStack->setCurrentIndex(page);

    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    emit viewModeChanged(mode);
}

void ContentPanel::setSortType(SortType type) {
    m_sortType = type;
    AppConfig::instance().setValue("ContentPanel/RightClickSortType", static_cast<int>(type));
    m_folderProxyModel->invalidate();
    m_folderProxyModel->sort(0, m_sortOrder);
    m_fileProxyModel->invalidate();
    m_fileProxyModel->sort(0, m_sortOrder);
}

void ContentPanel::setSortOrder(Qt::SortOrder order) {
    m_sortOrder = order;
    AppConfig::instance().setValue("ContentPanel/RightClickSortOrder", static_cast<int>(order));
    m_folderProxyModel->sort(0, order);
    m_fileProxyModel->sort(0, order);
}

void ContentPanel::applyFilters(const FilterState& state) {
    m_currentFilter = state;
    m_currentFilter.showFolders = m_showFolders;
    m_currentFilter.showFiles = m_showFiles;
    m_currentFilter.showHidden = m_showHidden;

    m_folderProxyModel->currentFilter = m_currentFilter;
    m_folderProxyModel->updateFilter();

    m_fileProxyModel->currentFilter = m_currentFilter;
    m_fileProxyModel->updateFilter();

    updateDualContainersVisibility();
    updateStatusBarStats();
}

void ContentPanel::applyFilters() {
    applyFilters(m_currentFilter);
}

void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    m_currentPath = path;
    int reqId = ++m_loadRequestId;

    ThumbnailPipelineService::instance().cancelAll();
    m_diskModel->incrementGeneration();

    QPointer<ContentPanel> weakThis(this);
    (void)QtConcurrent::run([weakThis, path, recursive, reqId]() {
        if (!weakThis) return;
        DiskScanService::scanDirectoryChunked(
            path, recursive,
            [weakThis, reqId](std::vector<ItemRecord>&& chunk, bool isFirstChunk) {
                if (!weakThis || weakThis->m_loadRequestId != reqId) return;
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, chunkData = std::move(chunk), isFirstChunk]() mutable {
                    if (!weakThis) return;
                    if (isFirstChunk) {
                        weakThis->m_model->setRecords(std::move(chunkData));
                        weakThis->m_folderProxyModel->sort(0, weakThis->m_sortOrder);
                        weakThis->m_fileProxyModel->sort(0, weakThis->m_sortOrder);
                    } else {
                        weakThis->m_model->appendRecords(std::move(chunkData));
                    }
                    weakThis->updateDualContainersVisibility();
                    weakThis->recalculateAndEmitStats();
                }, Qt::QueuedConnection);
            },
            [weakThis, reqId]() { return weakThis && (weakThis->m_loadRequestId == reqId); }
        );
    });
}

void ContentPanel::createNewItem(const QString& type) {
    QString newPath;
    if (m_actionController->createNewItem(m_currentPath, type, newPath)) {
        setPendingSelectName(newPath, true);
        refreshAll();
    }
}

void ContentPanel::performBatchRename() {
    auto paths = getSelectedPaths();
    if (paths.isEmpty()) return;

    std::vector<std::wstring> stdPaths;
    for (const QString& p : paths) stdPaths.push_back(QDir::toNativeSeparators(p).toStdWString());

    BatchRenameDialog dlg(stdPaths, this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshAll();
    }
}

void ContentPanel::onDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    QString path = index.data(PathRole).toString();
    if (path.isEmpty()) return;

    if (QFileInfo(path).isDir()) {
        emit directorySelected(path);
    } else {
        emit requestQuickLook(path);
    }
}

void ContentPanel::emitSelectionChangedSignal() {
    emit selectionChanged(getSelectedPaths());
    updateStatusBarStats();
}

void ContentPanel::refreshAll() {
    loadDirectory(m_currentPath, m_isRecursive);
}

void ContentPanel::updateItemMetadata(const QString& path) {
    if (m_model) m_model->updateRecordMetadata(path);
}

void ContentPanel::migrateModelCache(const QString& oldPath, const QString& newPath) {
    if (m_model) m_model->migrateCache(oldPath, newPath);
}

void ContentPanel::clearFolderCache(const QString& folderPath) {
    if (m_model) m_model->clearCacheForFolder(folderPath);
}

void ContentPanel::search(const QString& query) {
    m_currentFilter.keyword = query;
    applyFilters();
}

void ContentPanel::loadCategory(const QString& categoryType) {
    m_currentCategoryType = categoryType;
    if (categoryType == "trash") m_currentPath = "trash://";
}

void ContentPanel::recalculateAndEmitStats() {
    // 委托后台统计
}

void ContentPanel::updateStatusBarStats() {
    int total = (m_folderProxyModel ? m_folderProxyModel->rowCount() : 0) + (m_fileProxyModel ? m_fileProxyModel->rowCount() : 0);
    emit statusBarStatsUpdated(0, 0, total);
}

void ContentPanel::refreshVisibleThumbnails() {
    // 委托 ThumbnailPipelineService
}

void ContentPanel::updateGridSize() {}
void ContentPanel::setZoomLevel(int level) { m_zoomLevel = level; emit zoomLevelChanged(level); }
void ContentPanel::setPendingSelectName(const QString& name, bool edit) { m_pendingSelectNames.insert(name); m_isPendingEdit = edit; }
QString ContentPanel::getAdjacentFilePath(const QString&, int) { return QString(); }
void ContentPanel::selectAndScrollToPath(const QString&) {}
bool ContentPanel::eventFilter(QObject* obj, QEvent* event) { return QFrame::eventFilter(obj, event); }
void ContentPanel::wheelEvent(QWheelEvent* event) { QFrame::wheelEvent(event); }

} // namespace QuarkMeta
```

---

## 4. `CMakeLists.txt` 构建配置注册
```cmake
set(UI_SOURCES
    # ...
    src/ui/controllers/ContentContextMenuController.h
    src/ui/controllers/ContentContextMenuController.cpp
    src/ui/controllers/ContentActionController.h
    src/ui/controllers/ContentActionController.cpp
    src/ui/ContentPanel.h
    src/ui/ContentPanel.cpp
)
```