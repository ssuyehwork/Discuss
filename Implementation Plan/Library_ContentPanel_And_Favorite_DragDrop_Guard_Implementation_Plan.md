# Implementation Plan: 库与分类视图内容面板自拖拽拦截及收藏栏入库智能判定实施方案

本实施方案严格遵循《AGENTS.md》协同指南规范与《Memories.md》第 34 章铁律，在 `QuarkMeta_Source` 中实施：
1. **内容面板内部自拖拽拦截**：在列表/网格/瀑布流等视图中，当处于库（`library`）或智能文件夹（`smart_folder`）视图时，拖动已入库的项目并释放在当前面板内（非子文件夹目标）时，精准识别拖拽源来自当前视图，拦截多余的重定向入库逻辑，杜绝误报“已入库到未分类”Toast 与全量刷新；
2. **收藏栏智能防盲目入库**：拖拽项目到收藏夹面板（`FavoritePanel`）时，系统不仅将项目加入收藏夹，更智能检测项目是否已在库中（`isInLibrary`）；对于尚未入库的项目，按需自动纳入库管理（分类至“未分类”），对已入库项目则直接收藏并避免重复触发入库写库，彻底杜绝不加判断的盲目入库行为。

---

## 一、方案设计与架构分析 (Architecture & Design)

### 1.1 问题根因与调用链分析
- **问题 1 根因（自拖拽误入库）**：
  在 `ContentPanel`（特别是在列表模式 `DropTreeView`）中，当视图为 `library`（库）或 `smart_folder`（智能文件夹）时，用户轻微拖动物项并松开，`DropTreeView` 的 `DragDropEventFilter` 捕获到 drop 事件并派发 `pathsDropped`。
  `ContentFileOpsHandler::onPathsDropped` 接收到事件后，**不加判断地直接认定为外部拖入**：
  ```cpp
  if (catType == "smart_folder" || catType == "library") {
      bool ok = LibraryService::instance().addPathsToCategory(-2, paths);
      ...
      ToolTipOverlay::instance()->showToast("项目已入库到“未分类”", 2500, QColor("#3498db"));
  }
  ```
  即使用户只是在当前面板内部微调或误拖放，也会无端触发“项目已入库到未分类”提示，破坏用户操作体验。
  **修复设计**：作用域严格限定在分类/库视图（`catType == "library" || catType == "smart_folder"`）。若拖拽源视图（`lastDragSourceView()`）属于当前面板内部且落点不是文件夹，则判定为自拖拽无意义位移，直接静默退出（`return;`），绝不影响普通磁盘目录下的拖拽复制与移动。

- **问题 2 根因（收藏栏缺乏入库判定机制）**：
  在 `FavoritePanel::onPathsDroppedToFavorite` 中，接收拖放时仅校验了 `FavoriteService::isFavorite(path)`，完全未对接 `LibraryService::isInLibrary(path)`。用户将外部文件或文件夹拖入收藏栏时，无法感知其库状态，若需要关联入库或防止重复写库则毫无判断能力。
  **修复设计**：在添加收藏项目循环中，引入 `LibraryService::instance().isInLibrary(path)` 检查。对未在库中的新项目，智能调用 `LibraryService::instance().addPathsToCategory(-2, unindexedPaths)` 纳入“未分类”；若已在库中则跳过入库动作仅添加到收藏夹，实现收藏与库状态的严密协同与防盲目入库。

---

## 二、受影响文件清单 (Affected Files)

1. `QuarkMeta_Source/src/ui/ViewDragDropHelper.h`
2. `QuarkMeta_Source/src/ui/ViewDragDropHelper.cpp`
3. `QuarkMeta_Source/src/ui/controllers/ContentFileOpsHandler.cpp`
4. `QuarkMeta_Source/src/ui/FavoritePanel.cpp`

---

## 三、精准代码修改方案 (Precise Code Changes)

### 3.1 `QuarkMeta_Source/src/ui/ViewDragDropHelper.h`

<<<<<<< SEARCH
    static void executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions);
    static QPoint lastDragStartPos() { return s_lastDragStartPos; }

    static bool isDropTarget(const QWidget* widget, const QModelIndex& index);
=======
    static void executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions);
    static QPoint lastDragStartPos() { return s_lastDragStartPos; }
    static QAbstractItemView* lastDragSourceView() { return s_lastDragSourceView; }

    static bool isDropTarget(const QWidget* widget, const QModelIndex& index);
>>>>>>> REPLACE

<<<<<<< SEARCH
    static QAbstractItemView* s_hoverView;
    static QPersistentModelIndex s_hoverIndex;
    static QPoint s_lastDragStartPos;
};
=======
    static QAbstractItemView* s_hoverView;
    static QPersistentModelIndex s_hoverIndex;
    static QPoint s_lastDragStartPos;
    static QAbstractItemView* s_lastDragSourceView;
};
>>>>>>> REPLACE

---

### 3.2 `QuarkMeta_Source/src/ui/ViewDragDropHelper.cpp`

<<<<<<< SEARCH
QAbstractItemView* ViewDragDropHelper::s_hoverView = nullptr;
QPersistentModelIndex ViewDragDropHelper::s_hoverIndex;
QPoint ViewDragDropHelper::s_lastDragStartPos;

bool ViewDragDropHelper::isDropTarget(const QWidget* widget, const QModelIndex& index) {
=======
QAbstractItemView* ViewDragDropHelper::s_hoverView = nullptr;
QPersistentModelIndex ViewDragDropHelper::s_hoverIndex;
QPoint ViewDragDropHelper::s_lastDragStartPos;
QAbstractItemView* ViewDragDropHelper::s_lastDragSourceView = nullptr;

bool ViewDragDropHelper::isDropTarget(const QWidget* widget, const QModelIndex& index) {
>>>>>>> REPLACE

<<<<<<< SEARCH
void ViewDragDropHelper::executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions) {
    s_lastDragStartPos = QCursor::pos();
    if (!view || !view->selectionModel()) return;
=======
void ViewDragDropHelper::executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions) {
    s_lastDragStartPos = QCursor::pos();
    s_lastDragSourceView = view;
    if (!view || !view->selectionModel()) return;
>>>>>>> REPLACE

<<<<<<< SEARCH
    drag->exec(supportedActions | Qt::CopyAction | Qt::MoveAction, defaultAction);
    s_lastDragStartPos = QPoint();
}
=======
    drag->exec(supportedActions | Qt::CopyAction | Qt::MoveAction, defaultAction);
    s_lastDragStartPos = QPoint();
    s_lastDragSourceView = nullptr;
}
>>>>>>> REPLACE

---

### 3.3 `QuarkMeta_Source/src/ui/controllers/ContentFileOpsHandler.cpp`

<<<<<<< SEARCH
    // 1. 目标窗格分类视图拖放重定向逻辑
    QString catType = m_panel->getCurrentCategoryType();
    int catId = m_panel->currentCategoryId();

    if (catType == "library" && (catId > 0 || catId == -2)) {
=======
    // 1. 目标窗格分类视图拖放重定向逻辑
    QString catType = m_panel->getCurrentCategoryType();
    int catId = m_panel->currentCategoryId();

    // 🚀【分类/库视图自拖拽拦截】：若处于库或智能文件夹且拖拽源来自当前面板内部，且未拖入子文件夹，则判定为自拖拽，杜绝重复入库
    if (catType == "library" || catType == "smart_folder") {
        QAbstractItemView* srcView = ViewDragDropHelper::lastDragSourceView();
        if (srcView && m_panel->isAncestorOf(srcView)) {
            bool targetIsFolder = false;
            if (targetIndex.isValid()) {
                QString targetPath = targetIndex.data(PathRole).toString();
                if (!targetPath.isEmpty() && QFileInfo(targetPath).isDir()) {
                    targetIsFolder = true;
                }
            }
            if (!targetIsFolder) {
                return;
            }
        }
    }

    if (catType == "library" && (catId > 0 || catId == -2)) {
>>>>>>> REPLACE

---

### 3.4 `QuarkMeta_Source/src/ui/FavoritePanel.cpp`

<<<<<<< SEARCH
    int addedCount = 0;
    int duplicateCount = 0;
    for (const QString& path : paths) {
        if (FavoriteService::instance().isFavorite(path)) {
            duplicateCount++;
        } else {
            addFavoriteItem(path, parentId);
            addedCount++;
        }
    }

    if (addedCount > 0) {
        saveFavorites();
        m_orderDirty = false;
        if (duplicateCount == 0) {
            ToolTipOverlay::instance()->showToast("已成功添加至收藏夹", 1500, QColor("#2ecc71"));
        } else {
            ToolTipOverlay::instance()->showToast(QString("已添加 %1 个项目 (其余 %2 个已在收藏夹中)").arg(addedCount).arg(duplicateCount), 2000, QColor("#3498db"));
        }
    } else if (duplicateCount > 0) {
        ToolTipOverlay::instance()->showToast("该文件夹已在收藏夹中，请勿重复添加", 2000, QColor("#e81123"));
    }
=======
    int addedCount = 0;
    int duplicateCount = 0;
    QStringList unindexedToLibrary;
    for (const QString& path : paths) {
        if (FavoriteService::instance().isFavorite(path)) {
            duplicateCount++;
        } else {
            // 🚀【收藏栏入库智能判定】：先检查项目是否已经在库中，对于尚未入库的项目收集后归入“未分类”
            if (!LibraryService::instance().isInLibrary(path)) {
                unindexedToLibrary.append(path);
            }
            addFavoriteItem(path, parentId);
            addedCount++;
        }
    }

    if (!unindexedToLibrary.isEmpty()) {
        LibraryService::instance().addPathsToCategory(-2, unindexedToLibrary);
    }

    if (addedCount > 0) {
        saveFavorites();
        m_orderDirty = false;
        if (duplicateCount == 0) {
            ToolTipOverlay::instance()->showToast("已成功添加至收藏夹", 1500, QColor("#2ecc71"));
        } else {
            ToolTipOverlay::instance()->showToast(QString("已添加 %1 个项目 (其余 %2 个已在收藏夹中)").arg(addedCount).arg(duplicateCount), 2000, QColor("#3498db"));
        }
    } else if (duplicateCount > 0) {
        ToolTipOverlay::instance()->showToast("该文件夹已在收藏夹中，请勿重复添加", 2000, QColor("#e81123"));
    }
>>>>>>> REPLACE

---

## 四、构建与运行时验收标准 (Build & Verification Steps)

1. **编译检查**：
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. **列表模式自拖拽验收**：
   - 打开 QuarkMeta，切换至“库”或“智能文件夹”视图；
   - 视图模式切换为列表视图（`DropTreeView`）；
   - 鼠标选中某一行文件微动拖放，释放到列表空白处或当前项上；
   - 预期结果：不再弹出“项目已入库到‘未分类’”Toast，界面无任何抖动与冗余重载。
3. **磁盘常规模式不受影响验收**：
   - 在普通物理磁盘路径下，按住 Ctrl 拖放文件可正常触发原地复制副本。
4. **收藏栏拖拽入库智能判定验收**：
   - 将一个尚未入库的文件夹拖入收藏夹，项目正常进入收藏夹，且智能判定为未入库后同步归入“未分类”；
   - 将一个已经入库的项目拖入收藏夹，系统判定为已入库，仅执行收藏添加，杜绝重复写入或盲目操作。

---

## 五、官方 API 复用与防另起炉灶 (Official API Reuse & Anti-Redundancy)

1. 复用 `ViewDragDropHelper` 既有的全局拖拽追踪机制，新增静态 `lastDragSourceView()`，无需新建全局单例。
2. 复用 `LibraryService::instance().isInLibrary(path)` 作为库归属的唯一权威事实来源，杜绝另写 SQL 查询。
3. 复用 `m_panel->isAncestorOf(sourceView)` 严谨判定拖拽源是否属于当前面板。

---

## 六、数据单一事实来源声明表 (Data SSOT Declaration Table)

| 数据项 | 唯一 SSOT 来源 | 派生消费者 | 说明 |
| :--- | :--- | :--- | :--- |
| 当前全局拖拽源视图 | `ViewDragDropHelper::s_lastDragSourceView` | `ContentFileOpsHandler` | 在 `executeStartDrag` 赋值，拖拽结束清空 |
| 文件是否已入库 | `LibraryService::instance().isInLibrary` | `FavoritePanel` | SQLite `library_items` 权威缓存 |

---

## 七、接口签名与访问级别一致性核查 (Interface Consistency Check)

1. `ViewDragDropHelper::lastDragSourceView()`：公有静态内联方法，返回 `QAbstractItemView*`。
2. `ViewDragDropHelper::s_lastDragSourceView`：私有静态变量，由 `executeStartDrag` 维护。
3. `LibraryService::instance().isInLibrary(const QString& path) const`：`QuarkMeta_Source/src/meta/LibraryService.h` 第 16 行既有共有方法。

---

## 八、头文件包含链与类型完整性检查 (Header Chain Check)

1. `ContentFileOpsHandler.cpp` 已包含 `../ViewDragDropHelper.h` 与 `../../meta/LibraryService.h`。
2. `FavoritePanel.cpp` 已包含 `../meta/LibraryService.h`。
3. 所有用到的类型均有完整头文件声明，无需新增额外 include。

---

## 九、合规与质量核对表 (Compliance Self-Check Table)

| 检查项 | 状态 | 说明 |
| :--- | :--- | :--- |
| 1. SEARCH 块唯一存在性 | 满足 | 经过 verify_plan.py 物理校验 |
| 2. 杜绝虚构 API | 满足 | 全部 API 在既有工程中已存在 |
| 3. 无中文乱码 | 满足 | UTF-8 编码 |
| 4. 架构隔离与物理分立 | 满足 | 独立存放于 `Implementation Plan/` 目录 |

---

## 十、行为零篡改与数值零篡改声明表 (Behavior Preservation Table)

- 既有跨窗格拖拽归类功能保持 100% 不变；
- 既有普通磁盘目录文件拖拽操作完全不受影响；
- 既有收藏夹增删改查逻辑与样式完全保持不变。
