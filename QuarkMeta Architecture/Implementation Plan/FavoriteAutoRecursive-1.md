# FavoriteAutoRecursive-1.md - 收藏夹文件夹自动递归覆盖冲突根治实施方案

- **Base Commit Hash**: `fea3877403076754476d110247feb42e36035c01`
- **Prerequisite Plans**: 无
- **Supersedes**: `FavoriteAutoRecursive.md`

---

## 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/meta/FavoriteDao.h`
   - `src/meta/FavoriteDao.cpp`
   - `src/ui/FavoritePanel.h`
   - `src/ui/FavoritePanel.cpp`
   - `src/ui/PanelMediator.cpp`
   - `src/core/NavigationService.h`
   - `src/core/NavigationService.cpp`
2. **头文件原样签名核查**：
   - `void navigateTo(const QString& rawUrl, bool recordHistory = true);` (`src/core/NavigationService.h:16`)
   - `void currentUrlChanged(const QString& url, const QString& displayPath);` (`src/core/NavigationService.h:44`)
3. **既有 API 检索日志**：
   - `grep -rn "currentUrlChanged" src/ui/PanelMediator.cpp`：发现导航广播重新触发了 `targetPanel->loadDirectory(url)`，将 `recursive` 覆盖复位为 `false`。

---

## Chapter 1: Overview（概述与根因分析）

### 1. 根因分析（物理死因）
在初版 `FavoriteAutoRecursive.md` 实施后，用户点击已开启自动递归的收藏项时，发生了如下严重的**逻辑竞争与覆盖冲突**：

1. 用户点击收藏项 `src` ➔ `FavoritePanel` 发射 `directorySelectedWithRecursive(path, true)` 信号；
2. `PanelMediator` 响应该信号，调用 `targetPanel->loadDirectory(path, true)`（主内容区短暂开启了递归）；
3. 紧接着，`FavoritePanel` 发射 `directorySelected(path)` 信号 ➔ 触发 `NavigationService::instance().navigateTo(path)`；
4. `NavigationService` 发射 `currentUrlChanged(url, displayPath)` 全局导航变更广播；
5. `PanelMediator` 监听到 `currentUrlChanged` 广播，执行其统一导航逻辑：
   ```cpp
   targetPanel->loadDirectory(url); // ❌ 默认 recursive = false！
   ```
6. **惨剧发生**：统一导航逻辑执行 `loadDirectory(url, false)`，瞬间将第 2 步开启的 `recursive = true` 覆盖重置为 `false`！
   这直接导致：
   - **标记 1**：收藏项列表渲染出了绿色 `layers` 图标（正常）；
   - **标记 2**：但右侧工具栏 `layers` 按钮却是灰色的，主内容区仅加载了当前目录，**未能自动递归显示子文件夹项目**！

### 2. 重构设计目标
1. 扩展 `NavigationService::navigateTo` 和 `currentUrlChanged` 信号，增加可选参数 `bool recursive = false`。
2. 当由收藏夹或其他指定入口带 `recursive = true` 跳转时，`NavigationService` 记录并广播该 `recursive` 标志。
3. `PanelMediator` 响应 `currentUrlChanged(url, displayPath, recursive)` 时，透传 `recursive` 标志调用 `targetPanel->loadDirectory(url, recursive)`。
4. 彻底消除状态覆盖冲突，保证标记 1（收藏项图标）与标记 2（工具栏 layers 高亮及子文件夹展平显示）100% 同步达到预期。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/core/NavigationService.h` （修改）
2. `src/core/NavigationService.cpp` （修改）
3. `src/meta/FavoriteDao.h` （修改）
4. `src/meta/FavoriteDao.cpp` （修改）
5. `src/ui/FavoritePanel.h` （修改）
6. `src/ui/FavoritePanel.cpp` （修改）
7. `src/ui/PanelMediator.cpp` （修改）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/core/NavigationService.h`
```
<<<<<<< SEARCH
    void navigateTo(const QString& rawUrl, bool recordHistory = true);
=======
    void navigateTo(const QString& rawUrl, bool recordHistory = true, bool recursive = false);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void currentUrlChanged(const QString& url, const QString& displayPath);
=======
    void currentUrlChanged(const QString& url, const QString& displayPath, bool recursive = false);
>>>>>>> REPLACE
```

### 2. `src/core/NavigationService.cpp`
```
<<<<<<< SEARCH
void NavigationService::navigateTo(const QString& rawUrl, bool recordHistory) {
=======
void NavigationService::navigateTo(const QString& rawUrl, bool recordHistory, bool recursive) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    emit currentUrlChanged(m_currentUrl, currentDisplayPath());
    emitNavState();
}
=======
    emit currentUrlChanged(m_currentUrl, currentDisplayPath(), recursive);
    emitNavState();
}
>>>>>>> REPLACE
```

### 3. `src/meta/FavoriteDao.h`
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

### 4. `src/meta/FavoriteDao.cpp`
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

### 5. `src/ui/FavoritePanel.h`
```
<<<<<<< SEARCH
    void directorySelected(const QString& path);
    void requestLocateFile(const QString& path);
=======
    void directorySelected(const QString& path, bool recursive = false);
    void requestLocateFile(const QString& path);
>>>>>>> REPLACE
```

### 6. `src/ui/FavoritePanel.cpp`
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
        emit directorySelected(path, isAutoRec);
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

### 7. `src/ui/PanelMediator.cpp`
```
<<<<<<< SEARCH
    // 1. 路径变更与导航驱动
    connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, this,
            [this, contentPanel, addressBar, navPanel, filterPanel, searchController, titleBar](const QString& url, const QString& displayPath) {
=======
    // 1. 路径变更与导航驱动
    connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, this,
            [this, contentPanel, addressBar, navPanel, filterPanel, searchController, titleBar](const QString& url, const QString& displayPath, bool recursive) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            if (url == "computer://") {
                targetPanel->loadDirectory("computer://");
            } else if (url == "trash://") {
                targetPanel->loadCategory("trash");
            } else {
                targetPanel->loadDirectory(url);
            }
=======
            if (url == "computer://") {
                targetPanel->loadDirectory("computer://");
            } else if (url == "trash://") {
                targetPanel->loadCategory("trash");
            } else {
                targetPanel->loadDirectory(url, recursive);
            }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        connect(favoritePanel, &FavoritePanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });
=======
        connect(favoritePanel, &FavoritePanel::directorySelected, &NavigationService::instance(), [](const QString& path, bool recursive) {
            NavigationService::instance().navigateTo(path, true, recursive);
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
1. **收藏项视觉标识（标记 1）**：开启“自动递归”的收藏文件夹项目右侧精确绘制出绿色的 `layers` 层次图标。
2. **工具栏按钮与内容区同步（标记 2）**：鼠标点击开启了“自动递归”的收藏文件夹：
   - 右上角工具栏上的 `layers` 按钮同步变为高亮绿色；
   - 右侧主内容区递归展开并展示所有子文件夹中的项目。
3. **状态持久化**：重启软件或重新加载收藏夹，该收藏项的自动递归设置与图标依然生效。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 `NavigationService::navigateTo` 和 `currentUrlChanged` 全局导航主干通道，消除信号竞争与重复调用。
- 复用 `ContentPanel::loadDirectory(url, recursive)` 官方 API 加载数据。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 自动递归配置 | `favorites` 表 `is_auto_recursive` | QStandardItem `UserRole + 9` | 导航时透传给 `ContentPanel::m_isRecursive` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 外部调用接口 | 物理 .h 中的声明行 | 访问级别 |
| :--- | :--- | :--- |
| `NavigationService::navigateTo` | `src/core/NavigationService.h:16` | `public` |
| `NavigationService::currentUrlChanged` | `src/core/NavigationService.h:44` | `public signal` |
| `FavoritePanel::directorySelected` | `src/ui/FavoritePanel.h:43` | `public signal` |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 所有涉及文件包含链完备闭合，类型无需新增头文件依赖。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 关联位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 方案位于物理隔离目录 | 满足 | `FavoriteAutoRecursive-1.md` |
| 2 | SEARCH 块原文存在且唯一 | 满足 | 精准校验 7 个修改文件 |
| 3 | 10 大章节完整 | 满足 | Chapter 1 ~ 10 全齐 |
| 4 | 无 Win32 / Qt::Popup Hack | 满足 | 遵循 Qt 原生信号槽机制 |
| 5 | 无主/后台线程 UI 违规 | 满足 | 主线程同步 UI 刷新 |
| 6 | 样式与数值 100% 原样保持 | 满足 | 遵守原有 QPainter 几何逻辑 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

- **交互行为**：普通导航与点击行为完全保持不变，仅扩展可选的 `recursive` 状态传递。
- **外观数值**：右侧 `layers` 图标尺寸与原工具栏图标 100% 保持相同颜色与大小。
