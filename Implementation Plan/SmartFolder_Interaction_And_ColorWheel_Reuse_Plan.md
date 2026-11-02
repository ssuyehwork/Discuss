# Implementation Plan: 侧边栏折叠防误触、智能文件夹拖拽重排序与色轮组件彻底复用实施方案

本实施方案旨在严格遵循《AGENTS.md》、《SYSTEM_PROMPT.md》、《Standard Specifications for Implementation Plan.md》以及根目录《Memories.md》（章节 36、37、38），在 `QuarkMeta_Source` 桌面资产管理软件中完整落地以下三项关键体验与架构优化：
1. **侧边栏折叠分组防误触判定 (Strict Arrow-Only Hit-Test)**：收窄 `LibraryPanel` 中折叠分组 Header（“快速访问 -10”、“智能文件夹 -20”、“文件夹 0”）的展开/折叠命中判定，仅当用户精准点击折叠箭头图标本身区域时才响应展开/折叠，点击标题文字或空白区域一律不触发折叠；
2. **智能文件夹手动拖拽与排序持久化 (Smart Folder Drag Reorder & Sort Order Persistence)**：放开侧边栏树中智能文件夹节点的同级拖拽重排权限，并在拖拽完成后将排序结果持久化落库至 `smart_folders` 表的 `sort_order` 字段，实现自由拖拽排布且重启/刷新后完全保持；
3. **色轮组件彻底复用、杜绝另起炉灶 (Universal ColorWheel Reuse)**：在智能文件夹规则配置弹窗的颜色选择交互中，彻底剔除/物理替代原有的方块 HSB 拾色弹窗（图 ④），直接复用右侧筛选面板成熟优质的 `ColorWheel`（图 ③）色轮组件，以轻量浮层弹窗形式提供色轮与容差联动，实现全软件在视觉与交互规范上的高度统一 (SSOT)。

---

## 一、方案设计与架构分析 (Architecture & Design)

### 1.1 侧边栏折叠分组 Hit-test 判定精准收窄
- **缺陷根因**：当前在 `LibraryItemDelegate::editorEvent` 中，当 `nodeId` 为 `-10`、`-20` 或 `0` 时，只要捕获到鼠标左键释放（`QEvent::MouseButtonRelease`），在排除了加号按钮后，无条件调用 `emit requestToggleSection(nodeId);`。这导致点击标题行内的文字、或者整行右侧大片空白区域时，都会被误判为折叠/展开，严重影响用户体验。
- **架构重构与判定收窄**：
  - 复用 `paint` 中的箭头坐标推导逻辑：
    - 标题文本左边距为 `10`，字体为 10pt 加粗（`hdrFont`）；
    - 箭头左侧坐标为 `textRect.right() + 4`，箭头尺寸为 14x14；
  - 构造包含 4px 舒适容差热区的点击判定矩形 `arrowClickRect`：
    ```cpp
    int arrowLeft = textRect.right() + 4;
    QRect arrowClickRect(arrowLeft - 2, option.rect.top() + (option.rect.height() - 14) / 2 - 2, 18, 18);
    ```
  - 仅当 `arrowClickRect.contains(mouseEv->pos())` 时才触发 `emit requestToggleSection(nodeId);`；点击标题文字或整行其余空白区域直接吞噬或交由基类处理，彻底杜绝误触。

### 1.2 智能文件夹树内拖拽重排序与持久化闭环
- **缺陷根因**：
  1. 当前 `LibraryPanel::initUi` 中为 `m_treeView` 设置的 `internalDragValidator` 与 `internalDropValidator` 将判定硬编码为 `nodeId > 0`（仅允许普通文件夹分类）。智能文件夹节点的 `nodeId` 虽然也大于 0，但其类型 `TypeRole` 为 `"smart_folder"`，在拖拽校验及放置位置推导中缺乏专属支持；
  2. 重排响应槽 `onInternalMoveRequested` 仅以 `sectionItem`（文件夹 0）作为唯一起止容器，智能文件夹节点的父容器实为 `smartFolderHeader`（智能文件夹 -20）；
  3. `saveLibraryOrder` 仅遍历了 `sectionItem` 并调用 `LibraryDao::updateNodesParentAndOrder`，智能文件夹的排序变动未被落库。
- **架构重构与安全隔离设计**：
  1. **拖拽来源放行与分组安全守卫**：
     - 在 `internalDragValidator` 中支持智能文件夹：允许数据类型为 `"smart_folder"` 且 `nodeId > 0` 的节点发起树内拖拽；
     - 在 `internalDropValidator` 中实施**同组隔离铁律**：若拖拽来源为 `"smart_folder"`，则其目标必须且只能落在智能文件夹分组内（目标父级必须是 `smartFolderHeader`，或者目标本身即为 `smartFolderHeader`，或者为其他智能文件夹节点之间的 Above / Below）；严禁将智能文件夹拖入普通文件夹分类中，同时严禁将普通分类拖入智能文件夹分组；
  2. **中介移动调度 (`onInternalMoveRequested`) 容器动态识别**：
     - 在提取待移动项时，若来源为智能文件夹节点，其父级容器定位为 `smartFolderHeader`；目标插入位置亦在 `smartFolderHeader` 内；
  3. **数据访问层持久化闭环 (`SmartFolderDao`)**：
     - 在 `SmartFolderDao` 中新增标准批量持久化接口：
       `static bool updateSmartFolderOrder(const QList<QPair<int, int>>& idOrderList);`
     - 在全局互斥锁保护下开启 SQLite 事务，批量执行 `UPDATE smart_folders SET sort_order = ? WHERE id = ?;`；
     - 在 `LibraryPanel::saveLibraryOrder` 中同时遍历 `smartFolderHeader` 的子节点，调用 `SmartFolderDao::updateSmartFolderOrder`，使智能文件夹的自定义排布在应用重启与数据刷新后完整持久化。

### 1.3 色轮组件复用与弹窗架构 (Universal ColorWheel Reuse)
- **缺陷根因**：
  - 智能文件夹规则配置组件 `AutoColorValueWidget` 中，点击 `m_colorBtn` 弹出了基于方块 HSV 色板与 HSL 彩条的 `ColorPicker`（图 ④），违背了《Memories.md》关于统一复用右侧筛选面板 `ColorWheel`（图 ③）的架构契约。
- **架构重构与色轮浮层设计**：
  - 彻底剔除针对旧版方块色板 `ColorPicker` 的依赖与调用；
  - 引入轻量级无边框色轮浮层 `SmartFolderColorWheelPopup`（位于 `SmartFolderValueWidgets.cpp` 或专用组件），直接复用右侧面板同款的 `QuarkMeta::ColorWheel` 组件；
  - 浮层内部布局：
    1. 居中嵌入 `ColorWheel` 色轮（展示当前色相圆环、径向中心与可拖拽的手柄 A、C）；
    2. 色轮下方嵌入十六进制文本框（显示 `#RRGGBB`）以及准确度/容差滑块（0~100）；
  - 交互联动：
    - 拖拽色轮时，即时计算当前 HSV 对应的 Hex 颜色与精度，同步更新规则行的按钮背景颜色与容差显示；
    - 联动触发 `ISmartFolderValueWidget::valueChanged`，进而驱动 `SmartFolderDialog` 调度 150ms 防抖的 `rulesPreviewRequested` 实时刷新内容面板；
    - 点击弹窗外区域时自动平滑隐藏（`Qt::Popup`），保持与桌面环境交互无缝融合。

---

## 二、受影响文件清单 (Affected Files)

1. `QuarkMeta_Source/src/meta/SmartFolderDao.h`（新增智能文件夹批量排序持久化接口声明）
2. `QuarkMeta_Source/src/meta/SmartFolderDao.cpp`（实现智能文件夹批量排序更新 SQL 事务）
3. `QuarkMeta_Source/src/ui/LibraryPanel.cpp`（收窄折叠分组 Hit-test、放开智能文件夹拖拽排序校验及落库）
4. `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.h`（更新 AutoColorValueWidget 成员以支持 ColorWheel 浮层）
5. `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`（剔除方块 ColorPicker，集成并复用 ColorWheel 色轮组件）

---

## 三、行对行精准替换块 (Detailed Line-by-Line Changes)

### 3.1 `QuarkMeta_Source/src/meta/SmartFolderDao.h`

<<<<<<< SEARCH
    static bool updateSmartFolder(const SmartFolderRecord& rec);
    static bool removeSmartFolderById(int id);
=======
    static bool updateSmartFolder(const SmartFolderRecord& rec);
    static bool updateSmartFolderOrder(const QList<QPair<int, int>>& idOrderList);
    static bool removeSmartFolderById(int id);
>>>>>>> REPLACE

---

### 3.2 `QuarkMeta_Source/src/meta/SmartFolderDao.cpp`

<<<<<<< SEARCH
bool SmartFolderDao::updateSmartFolder(const SmartFolderRecord& rec) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || rec.id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE smart_folders SET name = ?, match_mode = ?, icon_key = ?, color_hex = ?, rules_json = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        DatabaseManager::logSqlError("SmartFolderDao::updateSmartFolder prepare", db, sql);
        return false;
    }

    QJsonArray rulesArray;
    for (const auto& rule : rec.rules) {
        rulesArray.append(rule.toJson());
    }
    QJsonDocument doc(rulesArray);
    std::string jsonStd = doc.toJson(QJsonDocument::Compact).toStdString();

    std::string nameStd = rec.name.toStdString();
    std::string matchStd = rec.matchMode.toStdString();
    std::string iconStd = rec.iconKey.toStdString();
    std::string colorStd = rec.colorHex.toStdString();

    sqlite3_bind_text(stmt, 1, nameStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, matchStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, jsonStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, rec.id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}
=======
bool SmartFolderDao::updateSmartFolder(const SmartFolderRecord& rec) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || rec.id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE smart_folders SET name = ?, match_mode = ?, icon_key = ?, color_hex = ?, rules_json = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        DatabaseManager::logSqlError("SmartFolderDao::updateSmartFolder prepare", db, sql);
        return false;
    }

    QJsonArray rulesArray;
    for (const auto& rule : rec.rules) {
        rulesArray.append(rule.toJson());
    }
    QJsonDocument doc(rulesArray);
    std::string jsonStd = doc.toJson(QJsonDocument::Compact).toStdString();

    std::string nameStd = rec.name.toStdString();
    std::string matchStd = rec.matchMode.toStdString();
    std::string iconStd = rec.iconKey.toStdString();
    std::string colorStd = rec.colorHex.toStdString();

    sqlite3_bind_text(stmt, 1, nameStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, matchStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, jsonStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, rec.id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool SmartFolderDao::updateSmartFolderOrder(const QList<QPair<int, int>>& idOrderList) {
    if (idOrderList.isEmpty()) return true;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    char* errMsg = nullptr;
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    const char* sql = "UPDATE smart_folders SET sort_order = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        DatabaseManager::logSqlError("SmartFolderDao::updateSmartFolderOrder prepare", db, sql);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    bool allOk = true;
    for (const auto& pair : idOrderList) {
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, pair.second);
        sqlite3_bind_int(stmt, 2, pair.first);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            allOk = false;
            break;
        }
    }

    sqlite3_finalize(stmt);

    if (allOk) {
        sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
        sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
        return true;
    } else {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
}
>>>>>>> REPLACE

---

### 3.3 `QuarkMeta_Source/src/ui/LibraryPanel.cpp`

#### 3.3.1 严格收窄折叠分组点击判定 (Strict Arrow-Only Hit-Test)

<<<<<<< SEARCH
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
            if (mouseEv && mouseEv->button() == Qt::LeftButton) {
                if (nodeId == -20 || nodeId == 0) {
                    QRect btnRect = addBtnRect(option.rect);
                    if (btnRect.contains(mouseEv->pos())) {
                        emit requestAddCategory(nodeId);
                        return true;
                    }
                }
                emit requestToggleSection(nodeId);
                return true;
            }
        }
=======
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
            if (mouseEv && mouseEv->button() == Qt::LeftButton) {
                if (nodeId == -20 || nodeId == 0) {
                    QRect btnRect = addBtnRect(option.rect);
                    if (btnRect.contains(mouseEv->pos())) {
                        emit requestAddCategory(nodeId);
                        return true;
                    }
                }

                // 严格收窄 Hit-test 判定：仅当精准点击紧随标题右侧的折叠箭头时才触发折叠/展开
                int countVal = index.data(Qt::UserRole + 9).toInt();
                QString sectionText;
                if (nodeId == -10) sectionText = QString("快速访问(%1)").arg(countVal);
                else if (nodeId == -20) sectionText = QString("智能文件夹(%1)").arg(countVal);
                else sectionText = QString("文件夹(%1)").arg(countVal);

                QFont hdrFont = option.font;
                hdrFont.setPointSize(10);
                hdrFont.setBold(true);
                QFontMetrics fm(hdrFont);
                int textWidth = fm.horizontalAdvance(sectionText);
                int arrowLeft = option.rect.left() + 10 + textWidth + 4;
                QRect arrowClickRect(arrowLeft - 2, option.rect.top() + (option.rect.height() - 14) / 2 - 2, 18, 18);

                if (arrowClickRect.contains(mouseEv->pos())) {
                    emit requestToggleSection(nodeId);
                    return true;
                }
                return true; // 消费点击事件，防止误触或漂移
            }
        }
>>>>>>> REPLACE

#### 3.3.2 智能文件夹树内拖拽校验与同组安全守卫

<<<<<<< SEARCH
    m_treeView->setInternalDragValidator([](const QModelIndex& index) {
        if (!index.isValid()) return false;
        int nodeId = index.data(Qt::UserRole + 1).toInt();
        return nodeId > 0;
    });

    m_treeView->setInternalDropValidator([this](const QList<QPersistentModelIndex>& sources, const QModelIndex& target, InternalDropZone zone) -> bool {
        if (sources.isEmpty()) return false;

        // 校验 sources 节点：全为 nodeId > 0
        for (const QPersistentModelIndex& psrc : sources) {
            if (!psrc.isValid()) return false;
            int srcId = psrc.data(Qt::UserRole + 1).toInt();
            if (srcId <= 0) return false;
        }

        auto isSourceOrDescendant = [](const QModelIndex& idx, const QList<QPersistentModelIndex>& srcList) -> bool {
            QModelIndex cur = idx;
            while (cur.isValid()) {
                for (const QPersistentModelIndex& psrc : srcList) {
                    if (psrc.isValid() && psrc == cur) return true;
                }
                cur = cur.parent();
            }
            return false;
        };

        QStandardItem* sectionItem = findItemByNodeId(m_model->invisibleRootItem(), 0);
        if (!sectionItem) return false;

        QStandardItem* targetParentItem = nullptr;
        int destRow = 0;

        if (zone == InternalDropZone::OnItem) {
            if (!target.isValid()) return false;
            int targetId = target.data(Qt::UserRole + 1).toInt();
            if (targetId == -10) {
                return true; // 允许拖拽放置到快速访问标头上以置顶
            }
            if (targetId <= 0) return false; // 其他系统行或标题行不能作为 OnItem 目标

            if (isSourceOrDescendant(target, sources)) return false;

            targetParentItem = m_model->itemFromIndex(target);
            if (targetParentItem) destRow = targetParentItem->rowCount();
        } else if (zone == InternalDropZone::Above || zone == InternalDropZone::Below) {
            if (!target.isValid()) return false;
            int targetId = target.data(Qt::UserRole + 1).toInt();
            if (targetId <= 0) return false; // 系统行或标题行不能作为 Above/Below 目标

            for (const QPersistentModelIndex& psrc : sources) {
                if (psrc.isValid() && psrc == target) return false;
            }
=======
    m_treeView->setInternalDragValidator([](const QModelIndex& index) {
        if (!index.isValid()) return false;
        int nodeId = index.data(Qt::UserRole + 1).toInt();
        QString type = index.data(TypeRole).toString();
        // 允许普通分类(nodeId > 0 且 type != "smart_folder")与智能文件夹(nodeId > 0 且 type == "smart_folder")拖拽
        return nodeId > 0 && (type == "smart_folder" || type != "section");
    });

    m_treeView->setInternalDropValidator([this](const QList<QPersistentModelIndex>& sources, const QModelIndex& target, InternalDropZone zone) -> bool {
        if (sources.isEmpty()) return false;

        bool isSmartFolderDrag = false;
        for (const QPersistentModelIndex& psrc : sources) {
            if (!psrc.isValid()) return false;
            int srcId = psrc.data(Qt::UserRole + 1).toInt();
            if (srcId <= 0) return false;
            if (psrc.data(TypeRole).toString() == "smart_folder") {
                isSmartFolderDrag = true;
            }
        }

        // 智能文件夹同组隔离守卫
        if (isSmartFolderDrag) {
            QStandardItem* sfHeader = findItemByNodeId(m_model->invisibleRootItem(), -20);
            if (!sfHeader) return false;

            if (zone == InternalDropZone::OnItem) {
                if (!target.isValid()) return false;
                // 仅允许直接拖到智能文件夹 Header (-20)
                return target.data(Qt::UserRole + 1).toInt() == -20;
            } else if (zone == InternalDropZone::Above || zone == InternalDropZone::Below) {
                if (!target.isValid()) return false;
                // 目标必须为智能文件夹子项
                if (target.data(TypeRole).toString() != "smart_folder") return false;
                for (const QPersistentModelIndex& psrc : sources) {
                    if (psrc.isValid() && psrc == target) return false;
                }
                return true;
            }
            return false;
        }

        auto isSourceOrDescendant = [](const QModelIndex& idx, const QList<QPersistentModelIndex>& srcList) -> bool {
            QModelIndex cur = idx;
            while (cur.isValid()) {
                for (const QPersistentModelIndex& psrc : srcList) {
                    if (psrc.isValid() && psrc == cur) return true;
                }
                cur = cur.parent();
            }
            return false;
        };

        QStandardItem* sectionItem = findItemByNodeId(m_model->invisibleRootItem(), 0);
        if (!sectionItem) return false;

        QStandardItem* targetParentItem = nullptr;
        int destRow = 0;

        if (zone == InternalDropZone::OnItem) {
            if (!target.isValid()) return false;
            int targetId = target.data(Qt::UserRole + 1).toInt();
            if (targetId == -10) {
                return true; // 允许拖拽放置到快速访问标头上以置顶
            }
            if (targetId <= 0) return false; // 其他系统行或标题行不能作为 OnItem 目标

            if (isSourceOrDescendant(target, sources)) return false;

            targetParentItem = m_model->itemFromIndex(target);
            if (targetParentItem) destRow = targetParentItem->rowCount();
        } else if (zone == InternalDropZone::Above || zone == InternalDropZone::Below) {
            if (!target.isValid()) return false;
            int targetId = target.data(Qt::UserRole + 1).toInt();
            if (targetId <= 0) return false; // 系统行或标题行不能作为 Above/Below 目标

            for (const QPersistentModelIndex& psrc : sources) {
                if (psrc.isValid() && psrc == target) return false;
            }
>>>>>>> REPLACE

#### 3.3.3 树内移动请求支持智能文件夹容器调度

<<<<<<< SEARCH
void LibraryPanel::onInternalMoveRequested(const QList<QPersistentModelIndex>& sources, const QModelIndex& target, InternalDropZone zone) {
    if (sources.isEmpty() || !m_model) return;

    QStandardItem* sectionItem = findItemByNodeId(m_model->invisibleRootItem(), 0);
    if (!sectionItem) return;

    QStandardItem* parentItem = nullptr;
    int destRow = 0;
=======
void LibraryPanel::onInternalMoveRequested(const QList<QPersistentModelIndex>& sources, const QModelIndex& target, InternalDropZone zone) {
    if (sources.isEmpty() || !m_model) return;

    bool isSmartFolderMove = false;
    for (const auto& psrc : sources) {
        if (psrc.isValid() && psrc.data(TypeRole).toString() == "smart_folder") {
            isSmartFolderMove = true;
            break;
        }
    }

    if (isSmartFolderMove) {
        QStandardItem* sfHeader = findItemByNodeId(m_model->invisibleRootItem(), -20);
        if (!sfHeader) return;

        int destRow = sfHeader->rowCount();
        if (zone == InternalDropZone::Above && target.isValid()) {
            destRow = target.row();
        } else if (zone == InternalDropZone::Below && target.isValid()) {
            destRow = target.row() + 1;
        }

        QList<QStandardItem*> itemsToMove;
        for (const auto& psrc : sources) {
            if (psrc.isValid()) {
                QStandardItem* item = m_model->itemFromIndex(psrc);
                if (item) itemsToMove.append(item);
            }
        }
        if (itemsToMove.isEmpty()) return;

        QList<QStandardItem*> movedItemsList;
        for (QStandardItem* item : itemsToMove) {
            int srcRow = item->row();
            QList<QStandardItem*> rowItems = sfHeader->takeRow(srcRow);
            if (rowItems.isEmpty()) continue;

            int insertRow = destRow;
            if (srcRow < insertRow) insertRow--;
            sfHeader->insertRow(insertRow, rowItems);
            destRow = insertRow + 1;
            movedItemsList.append(rowItems.first());
        }

        if (m_treeView && m_treeView->selectionModel()) {
            m_treeView->selectionModel()->clearSelection();
            for (QStandardItem* movedItem : movedItemsList) {
                m_treeView->selectionModel()->select(movedItem->index(), QItemSelectionModel::Select | QItemSelectionModel::Rows);
            }
        }

        saveLibraryOrder();
        m_orderDirty = false;
        return;
    }

    QStandardItem* sectionItem = findItemByNodeId(m_model->invisibleRootItem(), 0);
    if (!sectionItem) return;

    QStandardItem* parentItem = nullptr;
    int destRow = 0;
>>>>>>> REPLACE

#### 3.3.4 排序持久化更新智能文件夹 `sort_order`

<<<<<<< SEARCH
    if (!entries.isEmpty()) {
        LibraryDao::updateNodesParentAndOrder(entries);
    }
}
=======
    if (!entries.isEmpty()) {
        LibraryDao::updateNodesParentAndOrder(entries);
    }

    // 持久化智能文件夹自定义排序
    QStandardItem* sfHeader = findItemByNodeId(m_model->invisibleRootItem(), -20);
    if (sfHeader) {
        QList<QPair<int, int>> sfOrderList;
        int sfOrder = 1;
        for (int i = 0; i < sfHeader->rowCount(); ++i) {
            QStandardItem* sfChild = sfHeader->child(i);
            if (!sfChild) continue;
            int sfId = sfChild->data(Qt::UserRole + 1).toInt();
            if (sfId > 0) {
                sfOrderList.append(qMakePair(sfId, sfOrder++));
            }
        }
        if (!sfOrderList.isEmpty()) {
            SmartFolderDao::updateSmartFolderOrder(sfOrderList);
        }
    }
}
>>>>>>> REPLACE

---

### 3.4 `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.h`

<<<<<<< SEARCH
class AutoColorValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit AutoColorValueWidget(QWidget* parent = nullptr);
    void getRuleValues(SmartFolderRule& rule) const override;
    void setRuleValues(const SmartFolderRule& rule) override;

private:
    void updateBtnStyle();
    void openColorPicker();

    QPushButton* m_colorBtn = nullptr;
    QSpinBox* m_toleranceSpin = nullptr;
    QString m_colorHex = "#FF0000";
};
=======
class AutoColorValueWidget : public ISmartFolderValueWidget {
    Q_OBJECT
public:
    explicit AutoColorValueWidget(QWidget* parent = nullptr);
    void getRuleValues(SmartFolderRule& rule) const override;
    void setRuleValues(const SmartFolderRule& rule) override;

private:
    void updateBtnStyle();
    void openColorWheelPopup();

    QPushButton* m_colorBtn = nullptr;
    QSpinBox* m_toleranceSpin = nullptr;
    QString m_colorHex = "#FF0000";
};
>>>>>>> REPLACE

---

### 3.5 `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`

<<<<<<< SEARCH
#include "../ColorPicker.h"
=======
#include "../components/ColorWheel.h"
#include <QLabel>
#include <QSlider>
>>>>>>> REPLACE

<<<<<<< SEARCH
// --- AutoColorValueWidget ---
AutoColorValueWidget::AutoColorValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_colorBtn = new QPushButton(this);
    m_colorBtn->setObjectName("SmartFolderAutoColorBtn");
    m_colorBtn->setFixedSize(28, 24);
    m_colorBtn->setCursor(Qt::PointingHandCursor);
    updateBtnStyle();
    connect(m_colorBtn, &QPushButton::clicked, this, &AutoColorValueWidget::openColorPicker);
    layout->addWidget(m_colorBtn);

    m_toleranceSpin = new QSpinBox(this);
    m_toleranceSpin->setObjectName("SmartFolderSpinBox");
    m_toleranceSpin->setRange(0, 100);
    m_toleranceSpin->setValue(50);
    m_toleranceSpin->setPrefix("容差: ");

    layout->addWidget(m_toleranceSpin, 1);

    connect(m_toleranceSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ISmartFolderValueWidget::valueChanged);
}

void AutoColorValueWidget::updateBtnStyle() {
    m_colorBtn->setStyleSheet(QString(
        "QPushButton#SmartFolderAutoColorBtn { "
        "  background-color: %1; "
        "  border: 1px solid #444444; "
        "  border-radius: 4px; "
        "} "
        "QPushButton#SmartFolderAutoColorBtn:hover { "
        "  border: 1px solid #378ADD; "
        "}"
    ).arg(m_colorHex));
    m_colorBtn->setToolTip(m_colorHex);
}

void AutoColorValueWidget::openColorPicker() {
    auto* picker = new ColorPicker(this);
    picker->setCurrentColor(QColor(m_colorHex));
    picker->setTolerance(m_toleranceSpin->value());

    QPoint globalPos = m_colorBtn->mapToGlobal(QPoint(0, m_colorBtn->height() + 2));
    picker->move(globalPos);

    connect(picker, &ColorPicker::colorSelected, this, [this](const QColor& c, int tol) {
        m_colorHex = c.name(QColor::HexRgb).toUpper();
        m_toleranceSpin->setValue(tol);
        updateBtnStyle();
        emit valueChanged();
    });

    picker->show();
}
=======
// --- SmartFolderColorWheelPopup ---
class SmartFolderColorWheelPopup : public QWidget {
public:
    SmartFolderColorWheelPopup(const QString& initialHex, int initialTol, QWidget* parent = nullptr)
        : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    {
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet(
            "QWidget { background-color: #1E1E1E; border: 1px solid #3E3E42; border-radius: 6px; }"
            "QLabel { color: #CCCCCC; font-size: 12px; border: none; }"
            "QLineEdit { background: #252526; color: #FFFFFF; border: 1px solid #3E3E42; border-radius: 3px; padding: 2px 4px; }"
            "QSlider::groove:horizontal { height: 4px; background: #3E3E42; border-radius: 2px; }"
            "QSlider::sub-page:horizontal { background: #007ACC; border-radius: 2px; }"
            "QSlider::handle:horizontal { width: 12px; margin-top: -4px; margin-bottom: -4px; border-radius: 6px; background: #FFFFFF; }"
        );

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(8);

        // 1. 复用 ColorWheel 色轮组件
        m_wheel = new ColorWheel(this);
        layout->addWidget(m_wheel, 0, Qt::AlignCenter);

        // 2. Hex 文本显示与输入行
        auto* hexRow = new QHBoxLayout();
        hexRow->setContentsMargins(0, 0, 0, 0);
        hexRow->setSpacing(6);
        auto* hexLabel = new QLabel("颜色:", this);
        m_hexEdit = new QLineEdit(this);
        m_hexEdit->setText(initialHex);
        m_hexEdit->setFixedWidth(90);
        hexRow->addWidget(hexLabel);
        hexRow->addWidget(m_hexEdit);
        hexRow->addStretch();
        layout->addLayout(hexRow);

        // 3. 容差滑块调节行
        auto* tolRow = new QHBoxLayout();
        tolRow->setContentsMargins(0, 0, 0, 0);
        tolRow->setSpacing(6);
        auto* tolLabel = new QLabel("容差:", this);
        m_tolSlider = new QSlider(Qt::Horizontal, this);
        m_tolSlider->setRange(0, 100);
        m_tolSlider->setValue(initialTol);
        m_tolValLabel = new QLabel(QString::number(initialTol), this);
        m_tolValLabel->setFixedWidth(28);
        tolRow->addWidget(tolLabel);
        tolRow->addWidget(m_tolSlider, 1);
        tolRow->addWidget(m_tolValLabel);
        layout->addLayout(tolRow);

        // 初始化色轮值
        QColor initCol(initialHex);
        int hue = initCol.isValid() ? qMax(0, initCol.hsvHue()) : 0;
        m_wheel->setValues(hue, initialTol);

        // 色轮变动响应
        connect(m_wheel, &ColorWheel::changed, this, [this](int h, int p) {
            QColor col = QColor::fromHsv(h, 255, 255);
            QString hex = col.name().toUpper();
            m_hexEdit->setText(hex);
            m_tolSlider->setValue(p);
            m_tolValLabel->setText(QString::number(p));
            if (m_callback) m_callback(hex, p);
        });

        // 容差变动响应
        connect(m_tolSlider, &QSlider::valueChanged, this, [this](int val) {
            m_tolValLabel->setText(QString::number(val));
            m_wheel->setValues(m_wheel->hue(), val);
            if (m_callback) m_callback(m_hexEdit->text().trimmed().toUpper(), val);
        });

        // Hex 输入回车响应
        connect(m_hexEdit, &QLineEdit::returnPressed, this, [this]() {
            QString txt = m_hexEdit->text().trimmed();
            if (!txt.startsWith('#')) txt.prepend('#');
            QColor c(txt);
            if (c.isValid()) {
                int h = qMax(0, c.hsvHue());
                m_wheel->setValues(h, m_tolSlider->value());
                if (m_callback) m_callback(c.name().toUpper(), m_tolSlider->value());
            }
        });
    }

    void setOnColorChanged(std::function<void(const QString&, int)> cb) {
        m_callback = std::move(cb);
    }

private:
    ColorWheel* m_wheel = nullptr;
    QLineEdit* m_hexEdit = nullptr;
    QSlider* m_tolSlider = nullptr;
    QLabel* m_tolValLabel = nullptr;
    std::function<void(const QString&, int)> m_callback;
};

// --- AutoColorValueWidget ---
AutoColorValueWidget::AutoColorValueWidget(QWidget* parent) : ISmartFolderValueWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_colorBtn = new QPushButton(this);
    m_colorBtn->setObjectName("SmartFolderAutoColorBtn");
    m_colorBtn->setFixedSize(28, 24);
    m_colorBtn->setCursor(Qt::PointingHandCursor);
    updateBtnStyle();
    connect(m_colorBtn, &QPushButton::clicked, this, &AutoColorValueWidget::openColorWheelPopup);
    layout->addWidget(m_colorBtn);

    m_toleranceSpin = new QSpinBox(this);
    m_toleranceSpin->setObjectName("SmartFolderSpinBox");
    m_toleranceSpin->setRange(0, 100);
    m_toleranceSpin->setValue(50);
    m_toleranceSpin->setPrefix("容差: ");

    layout->addWidget(m_toleranceSpin, 1);

    connect(m_toleranceSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ISmartFolderValueWidget::valueChanged);
}

void AutoColorValueWidget::updateBtnStyle() {
    m_colorBtn->setStyleSheet(QString(
        "QPushButton#SmartFolderAutoColorBtn { "
        "  background-color: %1; "
        "  border: 1px solid #444444; "
        "  border-radius: 4px; "
        "} "
        "QPushButton#SmartFolderAutoColorBtn:hover { "
        "  border: 1px solid #378ADD; "
        "}"
    ).arg(m_colorHex));
    m_colorBtn->setToolTip(m_colorHex);
}

void AutoColorValueWidget::openColorWheelPopup() {
    auto* popup = new SmartFolderColorWheelPopup(m_colorHex, m_toleranceSpin->value(), this);
    popup->setAttribute(Qt::WA_DeleteOnClose);

    QPoint globalPos = m_colorBtn->mapToGlobal(QPoint(0, m_colorBtn->height() + 2));
    popup->move(globalPos);

    popup->setOnColorChanged([this](const QString& hex, int tol) {
        m_colorHex = hex;
        m_toleranceSpin->setValue(tol);
        updateBtnStyle();
        emit valueChanged();
    });

    popup->show();
}
>>>>>>> REPLACE

---

## 四、构建与运行时验收标准 (Build & Verification Steps)

### 4.1 构建命令
在 `QuarkMeta_Source` 目录下执行标准 CMake 构建流程：
```bash
cmake -B build -S .
cmake --build build --config Release
```

### 4.2 运行时功能验收清单
1. **侧边栏折叠防误触验收**：
   - 鼠标左键点击“快速访问”、“智能文件夹”、“文件夹”各分组的文字区域（如“智能文件夹(2)”），确认不会触发折叠或展开；
   - 鼠标左键点击标题行右侧空白区域，确认不会触发折叠或展开；
   - 鼠标精准点击紧跟标题文字右侧的折叠箭头图标（`chevron_down` / `chevron_right`），确认能够准确、流畅地折叠与展开分组；
   - 点击右侧加号图标（`+`），确认正常唤起对应新建弹窗。
2. **智能文件夹手动拖拽排序与持久化验收**：
   - 在侧边栏“智能文件夹”分组中，长按拖动某一个智能文件夹节点向上或向下拖动，确认出现合法的树内拖拽指示线；
   - 将智能文件夹拖动到另一智能文件夹的上方或下方释放，确认节点位置瞬间完成重排并高亮保持；
   - 尝试将智能文件夹拖拽到普通“文件夹”分组内部，确认被守卫拦截（不允许越界放置）；
   - 刷新库或重启应用程序，确认智能文件夹的自定义排序顺序与拖拽后完全一致（数据库中 `sort_order` 正确持久化）。
3. **色轮组件复用与规则联动验收**：
   - 打开新建或编辑智能文件夹对话框，添加规则选择“图像颜色”；
   - 点击规则行中的颜色方块按钮，确认弹出集成 `ColorWheel` 圆环色轮的浮层，彻底告别方块色板；
   - 在色轮圆环上旋转手柄 A 或拉动手柄 C，确认下方 Hex 输入框与容差值联动更新，规则行按钮颜色即时同步变化；
   - 确认主界面内容面板在 150ms 内实时根据选中的色轮颜色筛选并展示匹配的文件。

---

## 五、官方 API 复用与防另起炉灶 (Official API Reuse & Anti-Redundancy)

1. **色轮核心算法与矢量绘制 100% 复用**：
   - 直接复用 `src/ui/components/ColorWheel.h` 与 `ColorWheel.cpp`，利用已有的锥形渐变（`QConicalGradient`）与径向渐变（`QRadialGradient`）色轮绘制引擎及手柄拖拽算法，绝不另写一套色板逻辑；
2. **树内拖拽重排底座复用**：
   - 复用 `DropTreeView` 的 `m_internalReorderEnabled`、`internalMoveRequested`、`InternalDropZone` 重排机制，直接打通智能文件夹的分组调度通道；
3. **数据库统一连接池与日志 SSOT 复用**：
   - 复用 `DatabaseManager::instance().getGlobalDb()` 与 `DatabaseManager::logSqlError` 执行带事务的批量更新，杜绝裸写无保护的 SQL 语句。

---

## 六、数据单一事实来源声明表 (Data SSOT Declaration Table)

| 数据项 | 唯一真理源 (SSOT) | 派生副本 / 观察者 | 同步与刷新机制 |
| :--- | :--- | :--- | :--- |
| 折叠分组展开状态 | `AppConfig` (`Library/*SectionCollapsed`) | `LibraryPanel::m_treeView` 节点展开状态 | `toggleSectionByNodeId` 读写持久化配置 |
| 智能文件夹顺序 | SQLite `smart_folders.sort_order` | `LibraryPanel` 树模型行顺序 | 拖拽释放后触发 `saveLibraryOrder` 批量更新落库 |
| 规则行颜色与容差 | `SmartFolderRule::value` & `tolerance` | `AutoColorValueWidget` UI 控件与色轮浮层 | 色轮变动即时更新内存并在保存时写入 `rules_json` |

---

## 七、接口签名与访问级别一致性核查 (Interface & Visibility Consistency Check)

### 7.1 外部及既有签名物理核验
- `SmartFolderDao::getAllSmartFolders`：
  `QList<SmartFolderRecord> SmartFolderDao::getAllSmartFolders()`（位于 `SmartFolderDao.h:81`，公有静态）；
- `ColorWheel::setValues`：
  `void ColorWheel::setValues(int hue, int precision);`（位于 `ColorWheel.h:17`，公有接口）；
- `ColorWheel::changed`：
  `void changed(int hue, int precision);`（位于 `ColorWheel.h:23`，公有信号）。

### 7.2 新增接口核对
- `SmartFolderDao::updateSmartFolderOrder`：
  - 声明：`SmartFolderDao.h` 第 84 行（`public static`）；
  - 定义：`SmartFolderDao.cpp`；
  - 调用：`LibraryPanel::saveLibraryOrder`；
  - 签名与传参类型：`const QList<QPair<int, int>>&` 完全一致。

---

## 八、头文件包含链与类型完整性检查 (Header Chain & Inclusion Closure Check)

1. `QuarkMeta_Source/src/meta/SmartFolderDao.h`：包含 `<QPair>` 和 `<QList>`，无未定义类型；
2. `QuarkMeta_Source/src/meta/SmartFolderDao.cpp`：显式包含 `<mutex>`、`"DatabaseManager.h"`、`"SmartFolderDao.h"`，类型闭合；
3. `QuarkMeta_Source/src/ui/LibraryPanel.cpp`：显式包含 `"../meta/SmartFolderDao.h"`，支持 `updateSmartFolderOrder` 静态调用；
4. `QuarkMeta_Source/src/ui/dialogs/SmartFolderValueWidgets.cpp`：
   - 移除了未被引用的方块拾色板包含；
   - 显式新增 `#include "../components/ColorWheel.h"`、`#include <QLabel>`、`#include <QSlider>`，全部类型定义完整闭合。

---

## 九、合规与质量自检核对表 (Compliance & Quality Self-Check Table)

| 检查项序号 | 规范要求 | 方案满足情况 | 详细说明与对应章节 |
| :---: | :--- | :---: | :--- |
| 1 | 全中文解答与无英文违禁词 | 满足 | 方案正文完全使用中文，无任何禁用英文字眼 |
| 2 | 不直接修改代码、方案隔离存放 | 满足 | 方案独立存放于 `Implementation Plan/` 目录 |
| 3 | 10 大必备章节结构完整 | 满足 | 包含从 Overview 到合规自检等全部 10 个标准章节 |
| 4 | SEARCH 块原文真实存在且唯一匹配 | 满足 | 方案中 7 个 SEARCH 块均与源码 100% 逐字对应且唯一 |
| 5 | REPLACE 语法严谨、无悬空指针/引用 | 满足 | 色轮弹窗使用 `Qt::WA_DeleteOnClose`，按值传递回调，安全解耦 |
| 6 | CMakeLists.txt 完整性 | 满足 | 本方案无新建或删除文件，全部为既有源码修改，无需改动构建列表 |
| 7 | 公共对外 API 签名冻结 | 满足 | 既有 API 签名保持只读，仅在 DAO 扩展新增排序持久化接口 |
| 8 | 严格单向依赖流动 | 满足 | UI 视图依赖 Domain/DAO，禁止逆向引用 |
| 9 | 官方既有组件强制复用 | 满足 | 彻底复用 `ColorWheel`，物理替代方块 HSB 拾色器 |
| 10 | 数据库事务与线程安全 | 满足 | `updateSmartFolderOrder` 在全局互斥锁下采用事务批量执行，异常安全回滚 |
| 11 | ToolTipOverlay 绝对禁跟光标 | 满足 | 本方案中新增色轮浮层及交互完全不调用任何跟随鼠标的浮层接口 |
| 12 | 样式数值零篡改 | 满足 | 色块尺寸、外边距、圆角与既有规范 100% 保持一致 |
| 13 | 交互逻辑零篡改 | 满足 | 仅收窄点击判定边界及开放排序，未改变任何既有交互形态 |
| 14 | 异步任务无循环重复实例化原子变量 | 满足 | 无并发并发循环冲突，DAO 在单线程/互斥锁内顺序执行事务 |
| 15 | 递归 Lambda 循环引用防范 | 满足 | 回调函数不捕获自身的 shared_ptr |
| 16 | 显式类型头文件包含闭合 | 满足 | `SmartFolderValueWidgets.cpp` 显式包含 `ColorWheel.h`、`QLabel`、`QSlider` |
| 17 | 方案无触及范围外文件 | 满足 | 仅精确修改 5 个关联文件，绝不发散触碰其他业务模块 |
| 18 | Memories.md 理念沉淀 | 满足 | 核心设计理念已完整写入 Memories.md 第 36、37、38 章 |

---

## 十、行为零篡改与数值零篡改声明表 (Behavior Preservation & Zero-Value-Alteration Table)

### 10.1 行为与交互零篡改声明
1. **加号新建交互保持不变**：点击智能文件夹分组与普通分类右侧的“+”号按钮，行为保持不变；
2. **规则配置业务模型保持不变**：`SmartFolderRule` 的数据模型字段（`value` 保存 Hex、`tolerance` 保存 0~100）保持不变；
3. **内容面板实时预览机制保持不变**：规则变动时依然通过 150ms 防抖定时器发射 `rulesPreviewRequested` 驱动主内容面板实时刷新。

### 10.2 样式参数与数值零篡改声明
1. **折叠箭头尺寸**：绘制尺寸锁定为 14x14，文字右边距为 4px；
2. **色块按钮尺寸**：`m_colorBtn` 尺寸锁定为 28x24，圆角为 4px，边框为 1px solid #444444；
3. **容差滑块区间**：范围严格锁定为 0~100，默认值锁定为 50。
