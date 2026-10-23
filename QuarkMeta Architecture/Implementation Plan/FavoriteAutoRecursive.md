# FavoriteAutoRecursive.md - 收藏夹文件夹自动递归功能实施方案

- **Base Commit Hash**: `d471d875fc0d40635fbbef5afbc0c4feb40b9a66`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/meta/FavoriteDao.h`
   - `src/meta/FavoriteDao.cpp`
   - `src/ui/FavoritePanel.h`
   - `src/ui/FavoritePanel.cpp`
   - `src/ui/PanelMediator.cpp`
2. **头文件原样签名核查**：
   - `struct FavoriteRecord` (`src/meta/FavoriteDao.h:23`)
   - `void directorySelected(const QString& path);` (`src/ui/FavoritePanel.h:43`)
   - `void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;` (`src/ui/FavoritePanel.h:17`)
3. **既有 API 检索日志**：
   - `grep -rn "directorySelected" src/ui/PanelMediator.cpp`：发现 `connect(favoritePanel, &FavoritePanel::directorySelected, ...)`
   - `grep -rn "is_auto_recursive" src/meta/FavoriteDao.cpp`：结果为空，确认未另起炉灶。

---

## Chapter 1: Overview（概述与根因分析）

### 1. 需求分析
用户希望对“收藏夹”中的常用文件夹支持“自动递归”配置：
1. **右键菜单控制**：在收藏文件夹右键菜单中“在‘资源管理器’中显示”选项下方新增“开启自动递归”/“关闭自动递归”动作。
2. **数据库持久化**：将自动递归标志保存到 `favorites` 数据库表中。
3. **UI 视图渲染**：开启自动递归的收藏项在右侧渲染小巧的 `layers` 图标作为视觉标识。
4. **自动导航递归**：选中该收藏文件夹时，系统自动带 `recursive = true` 触发主内容区加载，省去用户每次手动点击 `layers` 按钮或按 `Ctrl+R` 的麻烦。

### 2. 方案设计
- **数据库层**：在 `favorites` 表增加 `is_auto_recursive INTEGER DEFAULT 0` 字段，增量升级 SQL 自动补齐列，`FavoriteRecord` 增加 `bool isAutoRecursive = false`，`FavoriteDao` 增加 `updateAutoRecursive(int id, bool autoRecursive)` 接口。
- **视图委派层 (`FavoriteItemDelegate`)**：若 `index.data(Qt::UserRole + 9).toBool()` 为 `true`，在 `textRect` 右侧绘制 `layers` 图标（绿/灰色小图标）。
- **右键菜单层 (`FavoritePanel::onFavoriteContextMenu`)**：在“在‘资源管理器’中显示”下方加入切换动作，点击后调用 `FavoriteDao::updateAutoRecursive` 更新数据库并刷新模型节点数据。
- **导航连接层 (`PanelMediator` & `FavoritePanel`)**：新增 `directorySelectedWithRecursive(const QString& path, bool recursive)` 信号，`PanelMediator` 接收后调用 `targetContentPanel->loadDirectory(path, recursive)` 自动开启递归。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/meta/FavoriteDao.h` （修改）
2. `src/meta/FavoriteDao.cpp` （修改）
3. `src/ui/FavoritePanel.h` （修改）
4. `src/ui/FavoritePanel.cpp` （修改）
5. `src/ui/PanelMediator.cpp` （修改）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/meta/FavoriteDao.h`
```
<<<<<<< SEARCH
    int sortOrder = 0;
    QStringList presetTags;
};
=======
    int sortOrder = 0;
    QStringList presetTags;
    bool isAutoRecursive = false;
};
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    static bool updateNodesParentAndOrder(const QList<NodeOrderEntry>& entries);
    static bool updatePresetTags(int id, const QStringList& tags);
=======
    static bool updateNodesParentAndOrder(const QList<NodeOrderEntry>& entries);
    static bool updatePresetTags(int id, const QStringList& tags);
    static bool updateAutoRecursive(int id, bool autoRecursive);
>>>>>>> REPLACE
```

### 2. `src/meta/FavoriteDao.cpp`
```
<<<<<<< SEARCH
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN parent_id INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN node_type INTEGER DEFAULT 1;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN preset_tags TEXT DEFAULT '';", nullptr, nullptr, nullptr);
=======
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN parent_id INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN node_type INTEGER DEFAULT 1;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN preset_tags TEXT DEFAULT '';", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN is_auto_recursive INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    const char* sql = "SELECT id, parent_id, node_type, path, name, icon_key, color_hex, sort_order, preset_tags FROM favorites ORDER BY sort_order ASC, id ASC;";
=======
    const char* sql = "SELECT id, parent_id, node_type, path, name, icon_key, color_hex, sort_order, preset_tags, is_auto_recursive FROM favorites ORDER BY sort_order ASC, id ASC;";
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        rec.sortOrder = sqlite3_column_int(stmt, 7);
        const char* tagsStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

        if (pathStr) rec.path = QString::fromUtf8(pathStr);
=======
        rec.sortOrder = sqlite3_column_int(stmt, 7);
        const char* tagsStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        rec.isAutoRecursive = (sqlite3_column_int(stmt, 9) != 0);

        if (pathStr) rec.path = QString::fromUtf8(pathStr);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool FavoriteDao::updatePresetTags(int id, const QStringList& tags) {
=======
bool FavoriteDao::updateAutoRecursive(int id, bool autoRecursive) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE favorites SET is_auto_recursive = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, autoRecursive ? 1 : 0);
    sqlite3_bind_int(stmt, 2, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool FavoriteDao::updatePresetTags(int id, const QStringList& tags) {
>>>>>>> REPLACE
```

### 3. `src/ui/FavoritePanel.h`
```
<<<<<<< SEARCH
    void directorySelected(const QString& path);
    void requestLocateFile(const QString& path);
=======
    void directorySelected(const QString& path);
    void directorySelectedWithRecursive(const QString& path, bool recursive);
    void requestLocateFile(const QString& path);
>>>>>>> REPLACE
```

### 4. `src/ui/FavoritePanel.cpp`
```
<<<<<<< SEARCH
    // 绘制文本
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
    painter->setFont(opt.font);

    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width() - 6);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    painter->restore();
=======
    // 绘制文本
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
    painter->setFont(opt.font);

    bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
    int rightIconWidth = isAutoRec ? 22 : 0;

    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width() - 6 - rightIconWidth);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    // 绘制开启自动递归右侧 layers 图标
    if (isAutoRec) {
        QRect layersRect(opt.rect.right() - 22, opt.rect.top() + (opt.rect.height() - 16) / 2, 16, 16);
        QIcon layersIcon = UiHelper::getIcon("layers", QColor("#2ecc71"), 16);
        layersIcon.paint(painter, layersRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
    }

    painter->restore();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (fi.isDir()) {
        emit directorySelected(path);
    } else {
=======
    if (fi.isDir()) {
        bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
        emit directorySelectedWithRecursive(path, isAutoRec);
        emit directorySelected(path);
    } else {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            // 在“资源管理器”中显示
            ContextMenuFactory::buildShowInExplorerAction(&menu, path, this);

            menu.addSeparator();
=======
            // 在“资源管理器”中显示
            ContextMenuFactory::buildShowInExplorerAction(&menu, path, this);

            bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
            QAction* actAutoRec = menu.addAction(UiHelper::getIcon("layers", isAutoRec ? QColor("#2ecc71") : QColor("#EEEEEE"), 18),
                                                 isAutoRec ? "关闭自动递归" : "开启自动递归");
            connect(actAutoRec, &QAction::triggered, this, [this, nodeId, isAutoRec, index]() {
                bool nextState = !isAutoRec;
                FavoriteDao::updateAutoRecursive(nodeId, nextState);
                if (m_favoriteModel) {
                    QStandardItem* item = m_favoriteModel->itemFromIndex(index);
                    if (item) {
                        item->setData(nextState, Qt::UserRole + 9);
                    }
                }
                if (m_favoriteView && m_favoriteView->viewport()) {
                    m_favoriteView->viewport()->update();
                }
            });

            menu.addSeparator();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        item->setData(rec.parentId, Qt::UserRole + 8);

        itemMap.insert(rec.id, item);
=======
        item->setData(rec.parentId, Qt::UserRole + 8);
        item->setData(rec.isAutoRecursive, Qt::UserRole + 9);

        itemMap.insert(rec.id, item);
>>>>>>> REPLACE
```

### 5. `src/ui/PanelMediator.cpp`
```
<<<<<<< SEARCH
        connect(favoritePanel, &FavoritePanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });
=======
        connect(favoritePanel, &FavoritePanel::directorySelectedWithRecursive, this, [this, contentPanel](const QString& path, bool recursive) {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->loadDirectory(path, recursive);
            }
        });

        connect(favoritePanel, &FavoritePanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 构建步骤
```bash
cmake --build build --config Debug
```

### 2. 运行时验收标准
1. **右键菜单显示**：右键点击收藏夹中的某个文件夹，在“在‘资源管理器’中显示”菜单项正下方，能看到“开启自动递归”选项。
2. **图标与状态切换**：点击“开启自动递归”，该收藏项右侧出现绿色 `layers` 层次图标，右键菜单文案自动切为“关闭自动递归”。
3. **点击自动递归加载**：鼠标点击开启了自动递归的收藏文件夹，右侧主内容区自动展开并递归展示子文件夹中包含的所有项目。
4. **持久化保存**：重启软件后，该收藏文件夹的“自动递归”标志与右侧 `layers` 图标仍然保持有效。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 `ContentPanel::loadDirectory(path, recursive)` 官方 API 实现加载，未另起炉灶。
- 复用 `UiHelper::getIcon("layers", ...)` 官方图标样式。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 自动递归配置状态 | `favorites` 表 `is_auto_recursive` 字段 | `FavoriteRecord::isAutoRecursive` / QStandardItem `UserRole + 9` | 内存更新并驱动 `FavoriteDao::updateAutoRecursive` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 外部调用接口 | 物理 .h 中的声明行 | 访问级别 |
| :--- | :--- | :--- |
| `FavoriteDao::updateAutoRecursive` | `src/meta/FavoriteDao.h:47` | `public` |
| `directorySelectedWithRecursive` | `src/ui/FavoritePanel.h:44` | `public signal` |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 所有涉及文件包含链完整，类型使用与声明对齐，无未定义符号隐患。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 关联位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 方案位于物理隔离目录 | 满足 | `FavoriteAutoRecursive.md` |
| 2 | SEARCH 块原文存在且唯一 | 满足 | 精准核对各目标 cpp/h 文件 |
| 3 | 10 大章节完整 | 满足 | Chapter 1 ~ 10 全覆盖 |
| 4 | 无 Win32 / Qt::Popup Hack | 满足 | 纯标准 Qt 信号槽与 QMenu |
| 5 | 无跨线程 UI 违规 | 满足 | 主线程同步视图更新 |
| 6 | 样式与数值 100% 照抄保持 | 满足 | 遵循原有 QPainter 几何与 QSS 渲染规范 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

- **交互行为**：只新增可选菜单项与扩展信号，未修改普通文件的点击与右键基础行为。
- **外观数值**：`layers` 图标像素尺寸与高亮配色与工具栏一致，未随意微调。
