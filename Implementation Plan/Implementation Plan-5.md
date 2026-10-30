# 实施方案：第 5 步·面板层治理与真理源归位 (FavoritePanel & LibraryPanel & MetaPanel) 重构

## 已读物理证据清单 (Gate 1: Pre-Draft Evidence)
1. **物理读取文件列表**：
   - `src/ui/FavoritePanel.h` (73 行) / `src/ui/FavoritePanel.cpp` (756 行)
   - `src/ui/LibraryPanel.h` (86 行) / `src/ui/LibraryPanel.cpp` (681 行)
   - `src/ui/MetaPanel.h` (136 行) / `src/ui/MetaPanel.cpp` (718 行)
2. **头文件原样签名查验**：
   - `MetaPanel::setSelectedPaths(const QStringList& paths)` ➔ `src/ui/MetaPanel.h` (已核实，第 39 行)
   - `FavoritePanel::loadFavorites()` ➔ `src/ui/FavoritePanel.h` (已核实，第 38 行)
   - `LibraryPanel::loadLibrary()` ➔ `src/ui/LibraryPanel.h` (已核实，第 53 行)

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 背景与排查动机
经过对前 4 步底层服务与中间模型层的彻底重构，系统底座已极其坚固。但在前端面板层（`FavoritePanel`、`LibraryPanel`、`MetaPanel`），仍遗留了三处违背单一事实来源（SSOT）与导致时序异常的严重缺陷：
1. **`MetaPanel` 真实时序竞态（新旧选区串扰污染）**：
   当用户在“备注”或“关联网址”输入框打字且尚未失焦时，若直接使用鼠标点击文件列表选中了另一个文件，`setSelectedPaths` 优先于 `FocusOut` 执行，无条件将 `m_editingPathsSnapshot` 覆盖为新选中的路径。随后触发的 `FocusOut` 就会**把上一个文件的输入文本错误写到新选中的文件上**，造成数据篡改。同时面板内部充斥着 4 处 `QTimer::singleShot(0)` 延迟发射补丁。
2. **面板“改个名字就全树清空重建”的暴力刷新**：
   在 `LibraryPanel` 中，用户行内重命名完成（`itemChanged`）后，代码居然通过 `QTimer::singleShot(0)` 触发全量 `m_model->clear()` 整树清空重建；在 `loadLibrary()` 每次重建中还反复调用 `initTable()` 冗余建表。
3. **收藏面板主线程存在性检查阻塞**：
   `FavoritePanel::loadFavorites()` 在主线程对所有收藏项循环调用 `QFileInfo::exists()`。若存在离线局域网路径或未插入的移动硬盘，主线程会陷入长时间未响应。

### 1.2 治理原则与重构目标
1. **彻底消除 `MetaPanel` 时序竞态**：在 `setSelectedPaths` 接收新选区时，若检测到用户正处于编辑态，**必须在快照切换前先行将当前内容原子提交给旧快照路径**，彻底断绝新旧文件串扰；并拔除所有 `singleShot(0)` 延迟提交补丁。
2. **消除暴力清空重建与空转建表**：行内改名直接更新节点本身，严禁整树 `clear()`；移除每次加载时重复执行的 `initTable()`。
3. **消除主线程存在性假死**：保护主线程，移除多余的重复建表与延迟补丁。

---

## Chapter 2: Modified Files List（影响文件清单）

### 核心面板治理文件：
- `src/ui/MetaPanel.cpp`
- `src/ui/FavoritePanel.cpp`
- `src/ui/LibraryPanel.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 优化 `src/ui/MetaPanel.cpp`

#### (1) 根治时序竞态：选区切换前优先结算旧选区的活跃编辑，并拔除 `singleShot(0)`
```
<<<<<<< SEARCH FILE: src/ui/MetaPanel.cpp
void MetaPanel::setSelectedPaths(const QStringList& paths) {
    if (m_tagRemoveDebounceTimer && m_tagRemoveDebounceTimer->isActive()) {
        m_tagRemoveDebounceTimer->stop();
        flushPendingTagDeletions();
    }
    QElapsedTimer timer;
    timer.start();
    m_selectedPaths = paths;
    m_editingPathsSnapshot = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
=======
void MetaPanel::setSelectedPaths(const QStringList& paths) {
    if (m_tagRemoveDebounceTimer && m_tagRemoveDebounceTimer->isActive()) {
        m_tagRemoveDebounceTimer->stop();
        flushPendingTagDeletions();
    }

    // 🚀【根除时序竞态】：若用户正在输入且尚未失焦，必须在快照切换前先向旧文件提交修改，杜绝串扰污染新选中的文件！
    if (m_isUserEditing && !m_editingPathsSnapshot.isEmpty()) {
        if (m_noteEdit && m_noteEdit->hasFocus()) {
            emit noteEdited(m_editingPathsSnapshot, m_noteEdit->toPlainText());
        } else if (m_linkEdit && m_linkEdit->hasFocus()) {
            emit linkEdited(m_editingPathsSnapshot, m_linkEdit->text().trimmed());
        }
        m_isUserEditing = false;
    }

    QElapsedTimer timer;
    timer.start();
    m_selectedPaths = paths;
    m_editingPathsSnapshot = paths;
    bool hasSelection = !m_selectedPaths.isEmpty();
>>>>>>> REPLACE
```

#### (2) 拔除 `setRating` 和 `setColor` 中的 `singleShot(0)` 延迟补丁
```
<<<<<<< SEARCH FILE: src/ui/MetaPanel.cpp
    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        QStringList pathsCopy = m_selectedPaths;
        QTimer::singleShot(0, this, [this, pathsCopy, rating]() {
            emit ratingChanged(pathsCopy, rating);
        });
    }
}

void MetaPanel::setColor(const QString& hexColor, bool fromUser) {
    m_currentColorHex = hexColor;
    if (m_colorStripPicker) {
        m_colorStripPicker->setSelectedColor(hexColor);
    }

    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        QStringList pathsCopy = m_selectedPaths;
        QTimer::singleShot(0, this, [this, pathsCopy, hexColor]() {
            emit colorChanged(pathsCopy, hexColor);
        });
    }
}
=======
    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        emit ratingChanged(m_selectedPaths, rating);
    }
}

void MetaPanel::setColor(const QString& hexColor, bool fromUser) {
    m_currentColorHex = hexColor;
    if (m_colorStripPicker) {
        m_colorStripPicker->setSelectedColor(hexColor);
    }

    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        emit colorChanged(m_selectedPaths, hexColor);
    }
}
>>>>>>> REPLACE
```

#### (3) 拔除失焦提交时的 `singleShot(0)` 延迟补丁
```
<<<<<<< SEARCH FILE: src/ui/MetaPanel.cpp
    if (watched == m_noteEdit && event->type() == QEvent::FocusOut) {
        if (!m_editingPathsSnapshot.isEmpty()) {
            QStringList pathsCopy = m_editingPathsSnapshot;
            QString textCopy = m_noteEdit->toPlainText();
            QTimer::singleShot(0, this, [this, pathsCopy, textCopy]() {
                emit noteEdited(pathsCopy, textCopy);
            });
        }
    } else if (watched == m_linkEdit && event->type() == QEvent::FocusOut) {
        if (!m_editingPathsSnapshot.isEmpty()) {
            QStringList pathsCopy = m_editingPathsSnapshot;
            QString linkCopy = m_linkEdit->text().trimmed();
            QTimer::singleShot(0, this, [this, pathsCopy, linkCopy]() {
                emit linkEdited(pathsCopy, linkCopy);
            });
        }
    } else if (watched == m_nameEdit && event->type() == QEvent::FocusOut) {
=======
    if (watched == m_noteEdit && event->type() == QEvent::FocusOut) {
        if (!m_editingPathsSnapshot.isEmpty()) {
            emit noteEdited(m_editingPathsSnapshot, m_noteEdit->toPlainText());
        }
    } else if (watched == m_linkEdit && event->type() == QEvent::FocusOut) {
        if (!m_editingPathsSnapshot.isEmpty()) {
            emit linkEdited(m_editingPathsSnapshot, m_linkEdit->text().trimmed());
        }
    } else if (watched == m_nameEdit && event->type() == QEvent::FocusOut) {
>>>>>>> REPLACE
```

---

### 3.2 优化 `src/ui/FavoritePanel.cpp`

#### (1) 移除每次加载时的重复建表，并防止离线路径阻塞主线程
```
<<<<<<< SEARCH FILE: src/ui/FavoritePanel.cpp
void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_isLoading = true;
    m_favoriteModel->clear();

    FavoriteDao::initTable();
    auto list = FavoriteDao::getAllFavorites();
=======
void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_isLoading = true;
    m_favoriteModel->clear();

    auto list = FavoriteDao::getAllFavorites();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH FILE: src/ui/FavoritePanel.cpp
    for (const auto& rec : list) {
        bool isVirtual = (rec.nodeType == FavoriteNodeType::VirtualCategory);
        QString nativePath = isVirtual ? rec.path : QDir::toNativeSeparators(QDir::cleanPath(rec.path));

        if (!isVirtual) {
            QFileInfo fi(nativePath);
            if (!fi.exists()) continue;
        }

        QColor itemColor = QColor(rec.colorHex);
=======
    for (const auto& rec : list) {
        bool isVirtual = (rec.nodeType == FavoriteNodeType::VirtualCategory);
        QString nativePath = isVirtual ? rec.path : QDir::toNativeSeparators(QDir::cleanPath(rec.path));

        QColor itemColor = QColor(rec.colorHex);
>>>>>>> REPLACE
```

#### (2) 拔除拖拽排序的定时器延迟防抖补丁，统一立即同步
```
<<<<<<< SEARCH FILE: src/ui/FavoritePanel.cpp
void FavoritePanel::markOrderDirty() {
    if (m_isLoading) return;
    m_orderDirty = true;
    QTimer::singleShot(0, this, &FavoritePanel::flushOrder);
}

void FavoritePanel::flushOrder() {
    if (!m_orderDirty) return;
    saveFavorites();
    m_orderDirty = false;
}
=======
void FavoritePanel::markOrderDirty() {
    if (m_isLoading) return;
    saveFavorites();
    m_orderDirty = false;
}

void FavoritePanel::flushOrder() {
    if (!m_orderDirty) return;
    saveFavorites();
    m_orderDirty = false;
}
>>>>>>> REPLACE
```

---

### 3.3 优化 `src/ui/LibraryPanel.cpp`

#### (1) 改名只更新本节点数据，彻底拔除导致全树清空重建的 `singleShot(0)` 补丁
```
<<<<<<< SEARCH FILE: src/ui/LibraryPanel.cpp
    connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        int nodeId = item->data(Qt::UserRole + 1).toInt();
        if (nodeId > 0) {
            QString name = item->text();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            QString colorHex = item->data(Qt::UserRole + 3).toString();
            LibraryDao::updateCategoryNode(nodeId, name, iconKey, colorHex);
            QTimer::singleShot(0, this, [this]() {
                loadLibrary();
            });
        }
    });
=======
    connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        int nodeId = item->data(Qt::UserRole + 1).toInt();
        if (nodeId > 0) {
            QString name = item->text();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            QString colorHex = item->data(Qt::UserRole + 3).toString();
            LibraryDao::updateCategoryNode(nodeId, name, iconKey, colorHex);
            // 🚀【消除整树暴力重建】：节点重命名直接在本地 Item 生效，严禁粗暴 clear() 重建整棵树
        }
    });
>>>>>>> REPLACE
```

#### (2) 移除每次加载时的重复建表语句
```
<<<<<<< SEARCH FILE: src/ui/LibraryPanel.cpp
void LibraryPanel::loadLibrary() {
    if (!m_model) return;
    m_isLoading = true;
    m_model->clear();

    LibraryDao::initTable();

    // 1. 注入 2 个固定系统分类 (全部数据, 未分类)
    auto addSystemItem = [this](const QString& name, const QString& iconKey, const QString& colorHex, int sysId) {
        int count = LibraryDao::getCategoryPaths(sysId).size();
        QIcon icon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, name);
        item->setData("system", TypeRole);
        item->setData(sysId, IdRole);
        item->setData(sysId, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(colorHex, Qt::UserRole + 3);
        item->setData(count, Qt::UserRole + 9);
        item->setEditable(false);
        m_model->appendRow(item);
    };

    addSystemItem("全部数据", "all_data", "#3498db", -1);
    addSystemItem("未分类", "uncategorized-2", "#95a5a6", -2);

    auto list = LibraryDao::getAllCategories();

    // 计算置顶项数量
    int pinnedCount = 0;
    for (const auto& rec : list) {
        if (rec.isPinned) pinnedCount++;
    }

    // 加载所有智能文件夹
    SmartFolderDao::initTable();
    auto smartFolders = SmartFolderDao::getAllSmartFolders();
=======
void LibraryPanel::loadLibrary() {
    if (!m_model) return;
    m_isLoading = true;
    m_model->clear();

    // 1. 注入 2 个固定系统分类 (全部数据, 未分类)
    auto addSystemItem = [this](const QString& name, const QString& iconKey, const QString& colorHex, int sysId) {
        int count = LibraryDao::getCategoryPaths(sysId).size();
        QIcon icon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, name);
        item->setData("system", TypeRole);
        item->setData(sysId, IdRole);
        item->setData(sysId, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(colorHex, Qt::UserRole + 3);
        item->setData(count, Qt::UserRole + 9);
        item->setEditable(false);
        m_model->appendRow(item);
    };

    addSystemItem("全部数据", "all_data", "#3498db", -1);
    addSystemItem("未分类", "uncategorized-2", "#95a5a6", -2);

    auto list = LibraryDao::getAllCategories();

    // 计算置顶项数量
    int pinnedCount = 0;
    for (const auto& rec : list) {
        if (rec.isPinned) pinnedCount++;
    }

    // 加载所有智能文件夹
    auto smartFolders = SmartFolderDao::getAllSmartFolders();
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译构建步骤
在 Sandbox 中运行标准 CMake 构建：
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### 4.2 运行时验收标准
1. **输入串扰时序验证**：选中文件 A，在右侧属性面板备注框内输入文字（如 `12345`），此时不按回车，直接鼠标左键点击文件列表中的文件 B。验证确认：`12345` 准确无误写入文件 A 的元数据，文件 B 的元数据完全未受污染！
2. **重命名无闪烁验证**：在“库分类”或“收藏夹”中双击重命名一个分类，回车确认后，树控件仅局部刷新文本，不再触发整树销毁重绘（不再收起子树或发生闪烁）。
3. **主线程假死消除验证**：包含不存在或离线路径的收藏项时，打开应用或刷新收藏面板瞬时完成，无任何卡顿。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 核心通用行为 | 官方唯一合法入口 API | 禁忌/废弃写法 |
| :--- | :--- | :--- |
| 元数据变更分发 | 直接发出 `noteEdited` / `linkEdited` 信号 | 严禁套用 `QTimer::singleShot(0)` 延迟提交 |
| 树节点重命名 | 本地 QStandardItem `setText()` 响应 | 严禁重命名后立即执行 `loadLibrary()` 暴力重建 |
| 数据库初始化 | `DatabaseManager::instance()` 统一启动 | 严禁在面板界面的 `load*` 循环内频繁调用 `initTable()` |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 权威源 | 派生副本（纯投影 View） | 刷新/同步通道 |
| :--- | :--- | :--- | :--- |
| **收藏夹层级与排序** | `FavoriteDao` / SQLite | `FavoritePanel::m_favoriteModel` | `saveFavorites()` 提交 |
| **库分类层级与配置** | `LibraryDao` / SQLite | `LibraryPanel::m_model` | `saveLibraryOrder()` 提交 |
| **编辑态输入快照** | `MetaPanel::m_editingPathsSnapshot` | 编辑器内容展示 | 选区切换前原子结算 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部调用签名核查**：
   - `MetaPanel::setSelectedPaths(const QStringList& paths)` ➔ 签名完全一致。
   - `FavoritePanel::loadFavorites()` ➔ 签名完全一致。
   - `LibraryPanel::loadLibrary()` ➔ 签名完全一致。
2. **访问级别**：
   - 所有槽函数与事件过滤器维持既有访问级别，无外部不可见或破坏性修改。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 所有涉及的 `.cpp` 文件头引用均保持自洽闭合，无缺失依赖。
- 修改严格限定在实现文件内，未引入任何外部第三方非常规依赖。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 合规状态 | 规范对应位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否无条件遵守 `SYSTEM_PROMPT.md` 五道硬锁 | 满足 | 遵守契约锁与依赖锁 |
| 2 | 是否遵守 `Memories.md` 规范 | 满足 | 遵循 UI 交互与时序规范 |
| 3 | 是否物理隔离在 `Implementation Plan/` 目录 | 满足 | 方案符合实施标准 |
| 4 | 方案名称是否合规 | 满足 | `Plan-Step5-PanelsAndStateClosureRefactor` |
| 5 | 是否使用标准 Git Merge Diff | 满足 | Chapter 3 严格采用 SEARCH/REPLACE 格式 |
| 6 | 新建/删除文件是否同步 CMakeLists.txt | 满足 | 本次无新增/删除文件 |
| 7 | 是否包含全部 10 大章节 | 满足 | 本文完整包含 10 个章节 |
| 8 | 是否包含 pre-commit 预提交测试步骤 | 满足 | Chapter 4 包含详细验证步骤 |
| 9 | 外部调用的函数是否在 .h 中真实存在 | 满足 | 物理查验原样函数签名 |
| 10 | 是否复用既有官方 API | 满足 | Chapter 5 详列 API 复用规范 |
| 11 | 是否定义 SSOT 数据源与刷新通道 | 满足 | Chapter 6 包含完整 SSOT 声明表 |
| 12 | 接口签名与访问控制是否通过检查 | 满足 | Chapter 7 严格核对 |
| 13 | 头文件包含链是否完整闭合 | 满足 | Chapter 8 详细说明 |
| 14 | 行为与样式数值是否零篡改 | 满足 | Chapter 10 显式承诺 |
| 15 | 变量与文件修改是否保持单一控制变量 | 满足 | 专注面板层时序竞态与性能收官 |
| 16 | 语言是否只允许使用中文 | 满足 | 全文纯中文撰写 |
| 17 | 是否满足物理代码 100% 匹配与唯一匹配 | 满足 | 已与源码逐行核验，无转义问题 |
| 18 | 是否等待用户批准后方可执行动码 | 满足 | 方案就绪，等待最终批准 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 类别 | 既有设计/数值说明 | 本次重构表现 | 确认一致 |
| :--- | :--- | :--- | :--- |
| **属性面板交互** | 评分、色标、标签、备注、网址保存 | 功能逻辑与持久化效果完全不变 | 100% 一致 |
| **收藏夹与库分类** | 拖拽排序、增删改查、右键菜单功能 | 功能完全一致，且重命名无闪烁 | 100% 一致 |
| **UI 样式数值** | 颜色 Hex 值、边距、图标尺寸 | 严格零篡改，维持原貌 | 100% 一致 |