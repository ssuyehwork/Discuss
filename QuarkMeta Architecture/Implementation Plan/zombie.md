# 根除全量僵尸代码与废弃历史负债无脑实施方案 (Zombie Code Purge Implementation Plan)

## 1. Overview (概述与解决的问题)

本实施方案旨在**彻底物理物理根除** QuarkMeta 纯磁盘直连模式代码库中残留的一切僵尸代码、废弃历史负债与违反三条交互铁律的逻辑。

根据 `File Names and Roles.md` 的深度排查结果，包含以下 6 大核心模块的僵尸代码清理：
1. **`src/core/BasicCommands.h`**：彻底物理清理 `BatchRenameCommand::undo/redo` 中违反三条交互铁律强刷全屏的 `notifyFullUIRebuild()` 僵尸调用。
2. **`src/core/ModelContract.h`**：彻底物理清理内存托管库时代遗留的 `ManagedRole` (UserRole + 105) 契约角色定义。
3. **`src/ui/TreeItemDelegate.h` & `ThumbnailDelegate.h/cpp`**：物理清理对 `ManagedRole` / `m_managedRole` / `isManaged` 的受控判定分支与 setter 成员函数。
4. **`src/meta/MetadataManager.h/cpp`**：物理清理分类树时代的 `notifyCategoryCountChanged()` 僵尸函数与 `RefreshLevel::CategoryOnly` 枚举残留。
5. **`src/meta/StatisticsService.h/cpp`**：物理清理分类树时代遗留的 `uncategorizedCount`（未分类资产计数）以及历史托管库旧版 `.QuarkMeta/trash` 路径判定与 `libraryTrashCount` 僵尸计数逻辑。
6. **`src/meta/TrashRepository.h/cpp`**：物理清理旧版内存托管库 `trash_items` 数据库表查询僵尸方法 `hasTrashItems()`（系统回收站统一使用基于 File_ID 隔离盒的 `disk_trash` 表）。

---

## 2. Modified Files List (影响文件清单)

- `src/core/BasicCommands.h`
- `src/core/ModelContract.h`
- `src/ui/TreeItemDelegate.h`
- `src/ui/ThumbnailDelegate.h`
- `src/ui/ThumbnailDelegate.cpp`
- `src/meta/MetadataManager.h`
- `src/meta/MetadataManager.cpp`
- `src/meta/StatisticsService.h`
- `src/meta/StatisticsService.cpp`
- `src/meta/TrashRepository.h`
- `src/meta/TrashRepository.cpp`

---

## 3. Detailed Line-by-Line Changes (精准替换块)

### 3.1 `src/core/BasicCommands.h`
<<<<<<< SEARCH
            if (!rawPairs.empty()) {
                MetadataManager::instance().renameBatchAsync(rawPairs);
            } else if (mode == DiskOperationMode::Copy) {
                // Copy 模式下虽然不修改 metadata，但删除文件后需要通知 UI 全局重建/刷新
                QMetaObject::invokeMethod(qApp, []() {
                    MetadataManager::instance().notifyFullUIRebuild();
                }, Qt::QueuedConnection);
            }
=======
            if (!rawPairs.empty()) {
                MetadataManager::instance().renameBatchAsync(rawPairs);
            }
>>>>>>> REPLACE

<<<<<<< SEARCH
            if (!rawPairs.empty()) {
                MetadataManager::instance().renameBatchAsync(rawPairs);
            } else if (mode == DiskOperationMode::Copy) {
                // Copy 模式重新生成物理文件后也需要通知 UI 刷新
                QMetaObject::invokeMethod(qApp, []() {
                    MetadataManager::instance().notifyFullUIRebuild();
                }, Qt::QueuedConnection);
            }
=======
            if (!rawPairs.empty()) {
                MetadataManager::instance().renameBatchAsync(rawPairs);
            }
>>>>>>> REPLACE

---

### 3.2 `src/core/ModelContract.h`
<<<<<<< SEARCH
    EncryptedRole       = Qt::UserRole + 103, // 是否加密
    EncryptHintRole     = Qt::UserRole + 104, // 加密提示
    ManagedRole         = Qt::UserRole + 105, // 是否受控 (已在索引中登记)
    IsEmptyRole         = Qt::UserRole + 106, // 是否为空目录
=======
    EncryptedRole       = Qt::UserRole + 103, // 是否加密
    EncryptHintRole     = Qt::UserRole + 104, // 加密提示
    IsEmptyRole         = Qt::UserRole + 106, // 是否为空目录
>>>>>>> REPLACE

---

### 3.3 `src/ui/TreeItemDelegate.h`
<<<<<<< SEARCH
        if (selected) {
            opt.palette.setColor(QPalette::Text, Qt::white);
        } else if (m_showStatus) {
            // 2026-06-xx 按照视觉要求：未录入项文字半透明暗淡处理
            // 物理修复：校准作用域
            bool isManaged = index.data(ManagedRole).toBool();
            if (!isManaged) {
                opt.palette.setColor(QPalette::Text, QColor(238, 238, 238, 120));
            }
        }
=======
        if (selected) {
            opt.palette.setColor(QPalette::Text, Qt::white);
        }
>>>>>>> REPLACE

<<<<<<< SEARCH
            if (col == 1) { // 🚨 物理修复 ①：状态列图标在单元格内部 100% 水平+垂直绝对居中！
                bool isPinned = idx0.data(IsLockedRole).toBool();
                bool isManaged = idx0.data(ManagedRole).toBool();

                int iconSize = 16;
                // 计算单元格物理中心坐标
                QRect centeredRect(option.rect.left() + (option.rect.width() - iconSize) / 2,
                                   option.rect.top() + (option.rect.height() - iconSize) / 2,
                                   iconSize, iconSize);

                if (isPinned) {
                    UiHelper::getIcon("pin_vertical", QColor("#FF551C"), 16).paint(painter, centeredRect, Qt::AlignCenter);
                } else if (isManaged) {
                    UiHelper::getIcon("check_circle", QColor("#2ecc71"), 16).paint(painter, centeredRect, Qt::AlignCenter);
                }
            } else if (col == 2) { // 星级列
=======
            if (col == 1) { // 🚨 物理修复 ①：状态列图标在单元格内部 100% 水平+垂直绝对居中！
                bool isPinned = idx0.data(IsLockedRole).toBool();

                int iconSize = 16;
                // 计算单元格物理中心坐标
                QRect centeredRect(option.rect.left() + (option.rect.width() - iconSize) / 2,
                                   option.rect.top() + (option.rect.height() - iconSize) / 2,
                                   iconSize, iconSize);

                if (isPinned) {
                    UiHelper::getIcon("pin_vertical", QColor("#FF551C"), 16).paint(painter, centeredRect, Qt::AlignCenter);
                }
            } else if (col == 2) { // 星级列
>>>>>>> REPLACE

---

### 3.4 `src/ui/ThumbnailDelegate.h`
<<<<<<< SEARCH
    void setPinnedRole(int role);
    void setManagedRole(int role);
    void setTypeRole(int role);
=======
    void setPinnedRole(int role);
    void setTypeRole(int role);
>>>>>>> REPLACE

<<<<<<< SEARCH
    int m_pinnedRole = -1;
    int m_managedRole = -1;
    int m_typeRole = -1;
=======
    int m_pinnedRole = -1;
    int m_typeRole = -1;
>>>>>>> REPLACE

---

### 3.5 `src/ui/ThumbnailDelegate.cpp`
<<<<<<< SEARCH
void ThumbnailDelegate::setPinnedRole(int role) { m_pinnedRole = role; }
void ThumbnailDelegate::setManagedRole(int role) { m_managedRole = role; }
void ThumbnailDelegate::setTypeRole(int role) { m_typeRole = role; }
=======
void ThumbnailDelegate::setPinnedRole(int role) { m_pinnedRole = role; }
void ThumbnailDelegate::setTypeRole(int role) { m_typeRole = role; }
>>>>>>> REPLACE

<<<<<<< SEARCH
void ThumbnailDelegate::drawFileNameText(QPainter* painter, const QRect& textRect, bool isSelected, const QModelIndex& index, const QStyleOptionViewItem& option) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QString name = index.data(Qt::DisplayRole).toString();
    painter->setPen(isSelected ? QColor("#3498db") : QColor("#EEEEEE"));

    // 针对未录入项目应用半透明效果
    if (m_managedRole != -1 && !isSelected && !index.data(m_managedRole).toBool()) {
        painter->setPen(QColor(238, 238, 238, 120));
    }

    QFont textFont = painter->font();
=======
void ThumbnailDelegate::drawFileNameText(QPainter* painter, const QRect& textRect, bool isSelected, const QModelIndex& index, const QStyleOptionViewItem& option) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QString name = index.data(Qt::DisplayRole).toString();
    painter->setPen(isSelected ? QColor("#3498db") : QColor("#EEEEEE"));

    QFont textFont = painter->font();
>>>>>>> REPLACE

---

### 3.6 `src/meta/MetadataManager.h`
<<<<<<< SEARCH
    enum class RefreshLevel {
        Full,         // 全量界面重构
        TagOnly,      // 仅物理更新标签相关
        CategoryOnly  // 仅物理更新分类，避免数据全量重载
    };
=======
    enum class RefreshLevel {
        Full,         // 全量界面重构
        TagOnly       // 仅物理更新标签相关
    };
>>>>>>> REPLACE

<<<<<<< SEARCH
    void notifyMetadataChangedBatch(const std::vector<std::wstring>& paths, RefreshLevel level = RefreshLevel::Full);

    void notifyCategoryCountChanged();
=======
    void notifyMetadataChangedBatch(const std::vector<std::wstring>& paths, RefreshLevel level = RefreshLevel::Full);
>>>>>>> REPLACE

---

### 3.7 `src/meta/MetadataManager.cpp`
<<<<<<< SEARCH
        case RefreshLevel::TagOnly:
            CentralEventHub::instance().dispatchTagsUpdated();
            break;
        case RefreshLevel::CategoryOnly:
            CentralEventHub::instance().dispatchCategoryCountUpdated();
            break;
=======
        case RefreshLevel::TagOnly:
            CentralEventHub::instance().dispatchTagsUpdated();
            break;
>>>>>>> REPLACE

<<<<<<< SEARCH
void MetadataManager::notifyCategoryCountChanged() {
    CentralEventHub::instance().dispatchCategoryCountUpdated();
}
=======
>>>>>>> REPLACE

---

### 3.8 `src/meta/StatisticsService.h`
<<<<<<< SEARCH
    std::atomic<int> m_totalCount{0};
    std::atomic<int> m_uncategorizedCount{0};
    std::atomic<int> m_untaggedCount{0};
    std::atomic<int> m_trashCount{0};
=======
    std::atomic<int> m_totalCount{0};
    std::atomic<int> m_untaggedCount{0};
    std::atomic<int> m_trashCount{0};
>>>>>>> REPLACE

---

### 3.9 `src/meta/StatisticsService.cpp`
<<<<<<< SEARCH
    int allCount = 0;
    int untaggedCount = 0;
    int uncategorizedCount = 0;
    int libraryTrashCount = 0;

    // 1. 纯内存 0ms 秒级核算（绝对真相源）
    MetadataManager::instance().forEachCachedItem([&](const std::wstring& path, const RuntimeMeta& meta) {
        if (meta.isFolder) return;

        // 🛡️ 物理在线断言 (谓词：drive_letter IN onlineDrives)
        if (path.length() >= 2 && path[1] == L':') {
            QChar dChar = QChar(path[0]).toUpper();
            if (dChar.isLetter()) {
                QString driveStr(dChar);
                if (!onlineDrives.contains(driveStr)) {
                    return; // 🚨 盘符离线直接排除该资产，不参与全库任何计数！
                }
            }
        }

        // 🛡️ 第一防线：强力回收站拦截 (物理路径特征)
        bool isInTrash = (path.find(L"/.QuarkMeta/trash") != std::wstring::npos) ||
                         (path.find(L"\\.QuarkMeta\\trash") != std::wstring::npos);

        if (isInTrash) {
            libraryTrashCount++;
            return; // 🚨 绝对提前退出！绝不参与 全部数据、未分类 的任何计数！
        }

        // 全部有效数据
        allCount++;

        // 未标签
        if (meta.tags.isEmpty()) {
            untaggedCount++;
        }
    });
=======
    int allCount = 0;
    int untaggedCount = 0;

    // 1. 纯内存 0ms 秒级核算（绝对真相源）
    MetadataManager::instance().forEachCachedItem([&](const std::wstring& path, const RuntimeMeta& meta) {
        if (meta.isFolder) return;

        // 🛡️ 物理在线断言 (谓词：drive_letter IN onlineDrives)
        if (path.length() >= 2 && path[1] == L':') {
            QChar dChar = QChar(path[0]).toUpper();
            if (dChar.isLetter()) {
                QString driveStr(dChar);
                if (!onlineDrives.contains(driveStr)) {
                    return; // 🚨 盘符离线直接排除该资产，不参与全库任何计数！
                }
            }
        }

        // 全部有效数据
        allCount++;

        // 未标签
        if (meta.tags.isEmpty()) {
            untaggedCount++;
        }
    });
>>>>>>> REPLACE

<<<<<<< SEARCH
    snapshot.totalCount = allCount;
    snapshot.untaggedCount = untaggedCount;
    snapshot.uncategorizedCount = uncategorizedCount;
    snapshot.trashCount = libraryTrashCount + diskTrashCount;

    // 3. 同步原子内存缓存
    m_totalCount.store(allCount);
    m_uncategorizedCount.store(uncategorizedCount);
    m_untaggedCount.store(untaggedCount);
    m_trashCount.store(snapshot.trashCount);
=======
    snapshot.totalCount = allCount;
    snapshot.untaggedCount = untaggedCount;
    snapshot.trashCount = diskTrashCount;

    // 3. 同步原子内存缓存
    m_totalCount.store(allCount);
    m_untaggedCount.store(untaggedCount);
    m_trashCount.store(snapshot.trashCount);
>>>>>>> REPLACE

---

### 3.10 `src/meta/TrashRepository.h`
<<<<<<< SEARCH
    // 检查全库/分库中是否存在回收站资产
    bool hasTrashItems() const;

    // 根据原始路径查询磁盘回收站记录 (id 和 trash_path)
    bool getDiskTrashRecordByPath(const std::wstring& originalPath, int& outId, QString& outTrashPath) const;
=======
    // 根据原始路径查询磁盘回收站记录 (id 和 trash_path)
    bool getDiskTrashRecordByPath(const std::wstring& originalPath, int& outId, QString& outTrashPath) const;
>>>>>>> REPLACE

---

### 3.11 `src/meta/TrashRepository.cpp`
<<<<<<< SEARCH
bool TrashRepository::hasTrashItems() const {
    std::vector<sqlite3*> dbs = { DatabaseManager::instance().getGlobalDb() };
    const char* sql = "SELECT 1 FROM trash_items LIMIT 1";
    
    for (sqlite3* db : dbs) {
        if (!db) continue;
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            bool hasItems = (sqlite3_step(stmt) == SQLITE_ROW);
            sqlite3_finalize(stmt);
            if (hasItems) return true;
        }
    }
    return false;
}

bool TrashRepository::getDiskTrashRecordByPath(const std::wstring& originalPath, int& outId, QString& outTrashPath) const {
=======
bool TrashRepository::getDiskTrashRecordByPath(const std::wstring& originalPath, int& outId, QString& outTrashPath) const {
>>>>>>> REPLACE

---

## 4. Build & Verification Steps (编译命令与验证方法)

1. **执行编译验证**：
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. **校验说明**：
   - 确认编译 100% 成功，无 `ManagedRole` / `trash_items` / `CategoryOnly` 未定义或链接错误。
   - `File Names and Roles.md` 中所有僵尸代码将被物理消灭干净。
