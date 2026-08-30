# ContentSortController-2 Implementation Plan

## 1. Overview
This implementation plan resolves the non-atomic sort updates, double-trigger overhead in `ContentContextMenu.cpp`, and the initial sorting sequence delay in `ContentPanel.cpp`.

### Fixes:
1. **Atomic Sort Updates (`setSortCriteria`)**:
   - Provide an atomic `setSortCriteria(SortType type, Qt::SortOrder order)` method in `ContentPanel` and `ContentSortController` to merge type and order changes into a single model invalidation/sort pass.
2. **Eliminate Double Triggers in `ContentContextMenu.cpp`**:
   - Clean up manual `AppConfig` writes and redundant `proxyModel->invalidate()`/`sort()` calls in `ContentContextMenu.cpp`, delegating all sort updates cleanly to `ContentSortController`.
3. **Optimize `ContentPanel.cpp` Constructor Initialization Sequence**:
   - Move `m_sortController` instantiation before model initialization in `ContentPanel.cpp` constructor to apply persisted user sorting settings on initial setup rather than hardcoding `Qt::AscendingOrder` and overwriting later.

---

## 2. Modified Files List
1. `src/ui/controllers/ContentSortController.h`
2. `src/ui/controllers/ContentSortController.cpp`
3. `src/ui/ContentPanel.h`
4. `src/ui/ContentPanel.cpp`
5. `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/controllers/ContentContextMenu.cpp`
Clean up redundant manual `AppConfig` writes and model sorting calls in right-click sort actions.

<<<<<<< SEARCH
    QActionGroup* typeGroup = new QActionGroup(this);
    auto addTypeAct = [&](const QString& label, ContentPanel::SortType type) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_panel->currentSortType() == type);
        typeGroup->addAction(act);
        connect(act, &QAction::triggered, [this, type]() {
            m_panel->setSortType(type);
            AppConfig::instance().setValue("ContentPanel/RightClickSortType", static_cast<int>(type));
            m_panel->getProxyModel()->invalidate();
            m_panel->getProxyModel()->sort(0, m_panel->currentSortOrder());
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

    QActionGroup* orderGroup = new QActionGroup(this);
    auto addOrderAct = [&](const QString& label, Qt::SortOrder order) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_panel->currentSortOrder() == order);
        orderGroup->addAction(act);
        connect(act, &QAction::triggered, [this, order]() {
            m_panel->setSortOrder(order);
            AppConfig::instance().setValue("ContentPanel/RightClickSortOrder", static_cast<int>(order));
            m_panel->getProxyModel()->invalidate();
            m_panel->getProxyModel()->sort(0, order);
        });
    };
=======
    QActionGroup* typeGroup = new QActionGroup(this);
    auto addTypeAct = [&](const QString& label, ContentPanel::SortType type) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_panel->currentSortType() == type);
        typeGroup->addAction(act);
        connect(act, &QAction::triggered, [this, type]() {
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

    QActionGroup* orderGroup = new QActionGroup(this);
    auto addOrderAct = [&](const QString& label, Qt::SortOrder order) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_panel->currentSortOrder() == order);
        orderGroup->addAction(act);
        connect(act, &QAction::triggered, [this, order]() {
            m_panel->setSortOrder(order);
        });
    };
>>>>>>> REPLACE

---

### 3.2 `src/ui/ContentPanel.h`
Expose atomic `setSortCriteria` API in `ContentPanel`.

<<<<<<< SEARCH
    void setSortType(SortType type) { if (m_sortController) m_sortController->setSortType(type); }
    void setSortOrder(Qt::SortOrder order) { if (m_sortController) m_sortController->setSortOrder(order); }
=======
    void setSortType(SortType type) { if (m_sortController) m_sortController->setSortType(type); }
    void setSortOrder(Qt::SortOrder order) { if (m_sortController) m_sortController->setSortOrder(order); }
    void setSortCriteria(SortType type, Qt::SortOrder order) { if (m_sortController) m_sortController->setSortCriteria(type, order); }
>>>>>>> REPLACE

---

### 3.3 `src/ui/ContentPanel.cpp`
Optimize `ContentPanel` constructor initialization sequence for `m_sortController`.

<<<<<<< SEARCH
    // 2026-04-12 深度修复：强制锁定过滤列为第 0 列（名称列），确保搜索逻辑不偏离 
    m_proxyModel->setFilterKeyColumn(0); 
    // 2026-05-29 物理修复：开启动态排序，确保“置顶优先”逻辑能在数据加载后自动生效
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->sort(0, Qt::AscendingOrder);
 
    // 2026-06-05 按照要求：从配置中加载上次保存的缩放比例 
    m_zoomLevel = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt(); 
    m_isRecursive = false; 
    // 文件夹默认显示 (true)
    m_showFolders = AppConfig::instance().getValue("ContentPanel/ShowFolders", true).toBool();
    m_showFiles = AppConfig::instance().getValue("ContentPanel/ShowFiles", true).toBool();
    m_showHidden = AppConfig::instance().getValue("ContentPanel/ShowHidden", false).toBool();
    
    // 同步到当前 FilterState
    m_currentFilter.showFolders = m_showFolders;
    m_currentFilter.showFiles = m_showFiles;

    connect(&QuarkMeta::TrashService::instance(), &QuarkMeta::TrashService::trashOperationCompleted, this, &ContentPanel::refreshAll);
    connect(&QuarkMeta::PermanentDeleteService::instance(), &QuarkMeta::PermanentDeleteService::permanentDeleteCompleted, this, &ContentPanel::refreshAll);
    connect(&QuarkMeta::ClipboardService::instance(), &QuarkMeta::ClipboardService::pasteCompleted, this, [this](const QString& dir) {
        if (m_currentPath == dir) refreshAll();
    });
    m_currentFilter.showHidden = m_showHidden;
 
    m_sortController = new ContentSortController(this);
    connect(m_sortController, &ContentSortController::sortCriteriaChanged, this, [this](SortType type, Qt::SortOrder order) {
        if (auto* proxy = qobject_cast<FilterProxyModel*>(m_proxyModel)) {
            proxy->setSortType(static_cast<int>(type));
            proxy->setSortOrder(order);
            proxy->sort(0, order);
        }
    });
    if (auto* proxy = qobject_cast<FilterProxyModel*>(m_proxyModel)) {
        proxy->setSortType(static_cast<int>(m_sortController->sortType()));
        proxy->setSortOrder(m_sortController->sortOrder());
    }
    m_sortController->applySortToModel(m_proxyModel);
=======
    m_sortController = new ContentSortController(this);
    connect(m_sortController, &ContentSortController::sortCriteriaChanged, this, [this](SortType type, Qt::SortOrder order) {
        if (auto* proxy = qobject_cast<FilterProxyModel*>(m_proxyModel)) {
            proxy->setSortType(static_cast<int>(type));
            proxy->setSortOrder(order);
            proxy->sort(0, order);
        }
    });

    // 2026-04-12 深度修复：强制锁定过滤列为第 0 列（名称列），确保搜索逻辑不偏离 
    m_proxyModel->setFilterKeyColumn(0); 
    // 2026-05-29 物理修复：开启动态排序，确保“置顶优先”逻辑能在数据加载后自动生效
    m_proxyModel->setDynamicSortFilter(true);

    if (auto* proxy = qobject_cast<FilterProxyModel*>(m_proxyModel)) {
        proxy->setSortType(static_cast<int>(m_sortController->sortType()));
        proxy->setSortOrder(m_sortController->sortOrder());
    }
    m_sortController->applySortToModel(m_proxyModel);
 
    // 2026-06-05 按照要求：从配置中加载上次保存的缩放比例 
    m_zoomLevel = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt(); 
    m_isRecursive = false; 
    // 文件夹默认显示 (true)
    m_showFolders = AppConfig::instance().getValue("ContentPanel/ShowFolders", true).toBool();
    m_showFiles = AppConfig::instance().getValue("ContentPanel/ShowFiles", true).toBool();
    m_showHidden = AppConfig::instance().getValue("ContentPanel/ShowHidden", false).toBool();
    
    // 同步到当前 FilterState
    m_currentFilter.showFolders = m_showFolders;
    m_currentFilter.showFiles = m_showFiles;

    connect(&QuarkMeta::TrashService::instance(), &QuarkMeta::TrashService::trashOperationCompleted, this, &ContentPanel::refreshAll);
    connect(&QuarkMeta::PermanentDeleteService::instance(), &QuarkMeta::PermanentDeleteService::permanentDeleteCompleted, this, &ContentPanel::refreshAll);
    connect(&QuarkMeta::ClipboardService::instance(), &QuarkMeta::ClipboardService::pasteCompleted, this, [this](const QString& dir) {
        if (m_currentPath == dir) refreshAll();
    });
    m_currentFilter.showHidden = m_showHidden;
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Build the target:
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. Verify that clicking right-click sort items triggers only a single sort pass and saves configuration atomically.
