# Implementation Plan: 监听文件夹自动创建树级分类与子文件多分类绑定实施方案

本实施方案严格遵循系统架构规范、《SYSTEM_PROMPT.md》、《Standard Specifications for Implementation Plan.md》以及 `Memories_QuarkMeta.md` 的核心设计理念。

---

## 已读物理证据清单（第 1 关：Gate 1）

| 文件 | 确认已读 | 关键物理签名 |
|---|---|---|
| `QuarkMeta_Source/src/meta/LibraryDao.h` | ✅ | `static QList<LibraryCategoryRecord> getAllCategories();`<br>`static int addCategory(const QString& name, int parentId = 0, const QString& iconKey = "Data-Classification.svg", const QString& colorHex = "#888888");`<br>`static bool addPathsToCategory(int id, const QStringList& paths);` (注意此函数内部先执行了清空所有旧分类，需增补只追加绑定的新接口)<br>`static QStringList getCategoryPaths(int id, bool verifyExistence = true);` |
| `QuarkMeta_Source/src/meta/LibraryDao.cpp` | ✅ | `library_category_paths` 拥有 `(category_id, path)` 联合主键，支持多分类绑定 |
| `QuarkMeta_Source/src/meta/LibraryService.h` | ✅ | `int createCategory(const QString& name, int parentId);`<br>`void emit libraryChanged();` 通知 UI 侧边栏刷新 |
| `QuarkMeta_Source/src/ui/dialogs/FolderWatchDialog.cpp` | ✅ | `onComplete()` 处理监控文件夹配置完成，启动后台线程异步入库并操作 `TaskProgressService` |
| `QuarkMeta_Source/src/meta/LibrarySyncService.cpp` | ✅ | 监控事件响应：`directoryChanged` 回调中执行增量变动同步 |

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 核心病灶与系统性缺陷深入剖析
1. **监听目录入库数据成为“无主孤儿”，前端 UI 侧边栏完全不可见**：
   - 当前在 `FolderWatchDialog::onComplete()` 中配置完成监听目录后，仅仅将扫描到的全量路径送入 `LibrarySyncService::syncPaths` 写入 `library_items` 表；
   - 在分类表（`library_categories`）中完全没有创建对应的分类节点，也没有在 `library_category_paths` 关联映射表中建立绑定。因此，左侧分类树没有展示这些目录，用户在界面上完全找不到数据，导致监听功能形同虚设。
2. **缺乏多分类绑定（多对多归属）通道**：
   - 现有的 `LibraryDao::addPathsToCategory` 会先清空路径的历史分类关系（`DELETE FROM library_category_paths WHERE path = ?`），违背了“若已属于其他分类，不破坏原有分类绑定，且同时追加绑定新分类”的需求；
   - 必须在 `LibraryDao` 中引入支持多对多幂等追加绑定的 `bindPathsToCategory(int id, const QStringList& paths)`（采用 `INSERT OR IGNORE`，不清除旧关联）。
3. **缺乏自动根据目录层级映射分类树的归一化服务**：
   - 当监控 `G:\SVG\自建` 时，应以根目录名称“自建”创建根分类；
   - 其中的子文件夹（如 `G:\SVG\自建\子目录1`）应自动作为“自建”的子分类创建，递归构建分类层级树；
   - 每一个文件夹本身及其下的直接子文件，精确绑定到该级分类中。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `QuarkMeta_Source/src/meta/LibraryDao.h`（新增多分类追加绑定接口声明与分类按名查找接口声明）
2. `QuarkMeta_Source/src/meta/LibraryDao.cpp`（实现多分类追加绑定接口与查找/创建接口）
3. `QuarkMeta_Source/src/meta/LibraryService.h`（向业务层外露分类树构建与多分类绑定方法）
4. `QuarkMeta_Source/src/meta/LibraryService.cpp`（实现分类树递归同步与路径绑定，并发射 `libraryChanged` 驱动 UI 刷新）
5. `QuarkMeta_Source/src/ui/dialogs/FolderWatchDialog.cpp`（重构完成入库流程：入库同时建立分类树层级并多对多绑定分类）
6. `QuarkMeta_Source/src/meta/LibrarySyncService.cpp`（在目录动态新增文件/子文件夹时，动态维护分类树与绑定）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `QuarkMeta_Source/src/meta/LibraryDao.h`
<<<<<<< SEARCH
    static bool addPathsToCategory(int id, const QStringList& paths);
    static bool removePathsFromCategory(int id, const QStringList& paths);
=======
    static bool addPathsToCategory(int id, const QStringList& paths);
    static bool bindPathsToCategory(int id, const QStringList& paths);
    static int getOrCreateCategory(const QString& name, int parentId = 0);
    static bool removePathsFromCategory(int id, const QStringList& paths);
>>>>>>> REPLACE

### 2. `QuarkMeta_Source/src/meta/LibraryDao.cpp`
<<<<<<< SEARCH
bool LibraryDao::addPathsToCategory(int id, const QStringList& paths) {
=======
int LibraryDao::getOrCreateCategory(const QString& name, int parentId) {
    if (name.trimmed().isEmpty()) return 0;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    const char* querySql = "SELECT id FROM library_categories WHERE name = ? AND parent_id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, querySql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string nStd = name.trimmed().toStdString();
        sqlite3_bind_text(stmt, 1, nStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, parentId);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int existingId = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return existingId;
        }
        sqlite3_finalize(stmt);
    }

    const char* insertSql = "INSERT INTO library_categories (parent_id, name, icon_key, color_hex, sort_order, created_at) "
                           "VALUES (?, ?, 'Data-Classification.svg', '#888888', (SELECT COALESCE(MAX(sort_order), 0) + 1 FROM library_categories), ?);";
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string nStd = name.trimmed().toStdString();
        sqlite3_bind_int(stmt, 1, parentId);
        sqlite3_bind_text(stmt, 2, nStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, QDateTime::currentSecsSinceEpoch());
        int newId = 0;
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            newId = static_cast<int>(sqlite3_last_insert_rowid(db));
        }
        sqlite3_finalize(stmt);
        sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
        return newId;
    }
    return 0;
}

bool LibraryDao::bindPathsToCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0 || paths.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    SqlTransaction trans(db);

    const char* sql = "INSERT OR IGNORE INTO library_category_paths (category_id, path) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        for (const QString& p : paths) {
            std::string pStd = QDir::toNativeSeparators(QDir::cleanPath(p)).toStdString();
            sqlite3_bind_int(stmt, 1, id);
            sqlite3_bind_text(stmt, 2, pStd.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    bool ok = trans.commit();
    if (ok) {
        sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    }
    return ok;
}

bool LibraryDao::addPathsToCategory(int id, const QStringList& paths) {
>>>>>>> REPLACE

### 3. `QuarkMeta_Source/src/meta/LibraryService.h`
<<<<<<< SEARCH
    bool addPathsToCategory(int id, const QStringList& paths);
    bool removePathsFromCategory(int id, const QStringList& paths);
=======
    bool addPathsToCategory(int id, const QStringList& paths);
    bool bindPathsToCategory(int id, const QStringList& paths);
    int syncWatchedFolderHierarchy(const QString& rootFolderPath);
    bool removePathsFromCategory(int id, const QStringList& paths);
>>>>>>> REPLACE

### 4. `QuarkMeta_Source/src/meta/LibraryService.cpp`
<<<<<<< SEARCH
#include "../ui/ToolTipOverlay.h"
#include <QCursor>
=======
#include "../ui/ToolTipOverlay.h"
#include <QCursor>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
>>>>>>> REPLACE

<<<<<<< SEARCH
bool LibraryService::addPathsToCategory(int id, const QStringList& paths) {
    bool ok = LibraryDao::addPathsToCategory(id, paths);
    if (ok) {
        emit libraryChanged();
    }
    return ok;
}
=======
bool LibraryService::bindPathsToCategory(int id, const QStringList& paths) {
    bool ok = LibraryDao::bindPathsToCategory(id, paths);
    if (ok) {
        emit libraryChanged();
    }
    return ok;
}

int LibraryService::syncWatchedFolderHierarchy(const QString& rootFolderPath) {
    QFileInfo rootInfo(rootFolderPath);
    if (!rootInfo.exists() || !rootInfo.isDir()) return 0;

    QString rootName = rootInfo.fileName();
    if (rootName.isEmpty()) {
        rootName = rootInfo.absoluteFilePath();
    }

    int rootCatId = LibraryDao::getOrCreateCategory(rootName, 0);
    if (rootCatId <= 0) return 0;

    // 绑定根文件夹与根目录下的直接子文件
    QString cleanRoot = QDir::toNativeSeparators(QDir::cleanPath(rootFolderPath));
    QStringList rootPaths;
    rootPaths << cleanRoot;
    QDir rDir(cleanRoot);
    QFileInfoList rootEntries = rDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const auto& fi : rootEntries) {
        rootPaths << QDir::toNativeSeparators(fi.absoluteFilePath());
    }
    LibraryDao::bindPathsToCategory(rootCatId, rootPaths);

    // 缓存已创建分类路径到 ID 的映射
    QHash<QString, int> folderCatMap;
    folderCatMap[cleanRoot.toLower()] = rootCatId;

    // 递归遍历所有子目录
    QDirIterator it(cleanRoot, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString subDirPath = QDir::toNativeSeparators(it.filePath());
        QFileInfo subInfo(subDirPath);
        QString parentDirPath = QDir::toNativeSeparators(subInfo.dir().absolutePath());

        int parentCatId = folderCatMap.value(parentDirPath.toLower(), rootCatId);
        int subCatId = LibraryDao::getOrCreateCategory(subInfo.fileName(), parentCatId);
        if (subCatId > 0) {
            folderCatMap[subDirPath.toLower()] = subCatId;
            QStringList subPaths;
            subPaths << subDirPath;
            QDir sDir(subDirPath);
            QFileInfoList fileEntries = sDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
            for (const auto& fi : fileEntries) {
                subPaths << QDir::toNativeSeparators(fi.absoluteFilePath());
            }
            LibraryDao::bindPathsToCategory(subCatId, subPaths);
        }
    }

    emit libraryChanged();
    return rootCatId;
}

bool LibraryService::addPathsToCategory(int id, const QStringList& paths) {
    bool ok = LibraryDao::addPathsToCategory(id, paths);
    if (ok) {
        emit libraryChanged();
    }
    return ok;
}
>>>>>>> REPLACE

### 5. `QuarkMeta_Source/src/ui/dialogs/FolderWatchDialog.cpp`
<<<<<<< SEARCH
#include "../../meta/LibrarySyncService.h"
#include "../../core/DwsTrace.h"
=======
#include "../../meta/LibrarySyncService.h"
#include "../../meta/LibraryService.h"
#include "../../core/DwsTrace.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
            const int batchSize = 100;
            for (int i = 0; i < total; i += batchSize) {
                QStringList batch = allPathsToSync.mid(i, batchSize);
                LibrarySyncService::instance().syncPaths(batch);
                int processed = qMin(i + batchSize, total);
                QMetaObject::invokeMethod(QCoreApplication::instance(), [jobId, processed, total]() {
                    TaskProgressService::instance().updateProgress(jobId, processed, total, QString("已录入 %1 / %2 项").arg(processed).arg(total));
                }, Qt::QueuedConnection);
            }

            QMetaObject::invokeMethod(QCoreApplication::instance(), [jobId]() {
                TaskProgressService::instance().finishJob(jobId);
            }, Qt::QueuedConnection);
=======
            const int batchSize = 100;
            for (int i = 0; i < total; i += batchSize) {
                QStringList batch = allPathsToSync.mid(i, batchSize);
                LibrarySyncService::instance().syncPaths(batch);
                int processed = qMin(i + batchSize, total);
                QMetaObject::invokeMethod(QCoreApplication::instance(), [jobId, processed, total]() {
                    TaskProgressService::instance().updateProgress(jobId, processed, total, QString("已录入 %1 / %2 项").arg(processed).arg(total));
                }, Qt::QueuedConnection);
            }

            // 自动将监听目录结构同步至分类树并追加绑定
            for (const QString& folder : validFolders) {
                LibraryService::instance().syncWatchedFolderHierarchy(folder);
            }

            QMetaObject::invokeMethod(QCoreApplication::instance(), [jobId]() {
                TaskProgressService::instance().finishJob(jobId);
            }, Qt::QueuedConnection);
>>>>>>> REPLACE

### 6. `QuarkMeta_Source/src/meta/LibrarySyncService.cpp`
<<<<<<< SEARCH
#include "MetadataManager.h"
#include "../core/DirectoryWatchService.h"
=======
#include "MetadataManager.h"
#include "LibraryService.h"
#include "../core/DirectoryWatchService.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (!toSync.isEmpty()) {
            syncPaths(toSync);
        }
=======
        if (!toSync.isEmpty()) {
            syncPaths(toSync);
            // 实时维护监听目录分类与文件绑定
            for (const QString& wf : watchedFolders) {
                QString normWf = DirectoryWatchService::normalizePathKey(wf);
                if (normDirPath == normWf || normDirPath.startsWith(normWf + "/")) {
                    LibraryService::instance().syncWatchedFolderHierarchy(wf);
                    break;
                }
            }
        }
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

1. **静态代码分析与构建验证**：
   - 核对新增加的接口声明与定义：`LibraryDao::getOrCreateCategory`、`LibraryDao::bindPathsToCategory`、`LibraryService::syncWatchedFolderHierarchy`。
2. **运行时验收标准**：
   - 用户打开“目录监听设置”，填入路径 `G:\SVG\自建` 并点击“完成”；
   - 界面弹出进度条，后台平滑同步元数据与子项；
   - 进度完成后，左侧分类树自动刷出“自建”分类，其下层级完整展现各个子文件夹分类；
   - 点击各分类，右侧视图能够正确展示对应的子文件夹与文件；
   - 若某些文件之前已属于“常用”或“未分类”，其在原有分类中的关联保持不变，且在“自建”分类树下同样可见（完美达成多对多绑定）。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）
- 分类变更广播：复用 `LibraryService::instance().libraryChanged` 信号驱动 `LibraryPanel::loadLibrary` 原位重新装载，消除另起炉灶刷新；
- 数据入库闭环：复用 `LibrarySyncService::instance().syncPaths` 保证 `.QuarkMeta.json` 与 `ItemRecord` 的单通道入库；
- 进度展示复用：复用全局单一真理源 `TaskProgressService::instance()`。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本 / 呈现端 | 同步驱动与生命周期 |
|---|---|---|---|
| 分类节点定义 | SQLite `library_categories` | `LibraryPanel` 树模型 | `LibraryDao` 增改后发射 `libraryChanged` |
| 路径与分类关联 | SQLite `library_category_paths` | `LibraryItemDao`、视图内容 | `bindPathsToCategory` 追加绑定，`INSERT OR IGNORE` 保持多分类共存 |
| 监听路径列表 | `AppConfig` (`DirectoryWatch/WatchedFolders`) | `FolderWatchDialog` 行列表 | `onComplete()` 同步更新并驱动 `DirectoryWatchService` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- `LibraryDao::getOrCreateCategory(const QString& name, int parentId)`：声明于 `LibraryDao.h`，公开 `public static`，定义于 `LibraryDao.cpp`；
- `LibraryDao::bindPathsToCategory(int id, const QStringList& paths)`：声明于 `LibraryDao.h`，公开 `public static`，定义于 `LibraryDao.cpp`；
- `LibraryService::syncWatchedFolderHierarchy(const QString& rootFolderPath)`：声明于 `LibraryService.h`，公开 `public`，定义于 `LibraryService.cpp`。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `LibraryDao.cpp`：已包含 `<QDateTime>`、`<QDir>`、`<QFileInfo>`、`DatabaseManager.h`；
- `LibraryService.cpp`：补充包含 `<QDir>`、`<QDirIterator>`、`<QFileInfo>`、`<QHash>`，保证类型完整性；
- `FolderWatchDialog.cpp`：显式包含 `../../meta/LibraryService.h`；
- `LibrarySyncService.cpp`：显式包含 `LibraryService.h`。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 结论 | 证据 / 说明 |
|---|---|---|---|
| 1 | 是否破坏既有公开头文件 ABI | 满足 | 仅新增方法，既有函数签名未变 |
| 2 | 是否产生平台级 Hack 或私有穿透 | 满足 | 完全走 DAO 与 Service 规范接口 |
| 3 | 是否存在打补丁行为 | 满足 | 从根因出发解决分类缺失，提供通用层级同步管道 |
| 4 | 是否遵循 SSOT 铁律 | 满足 | 数据库 `library_categories` 与 `library_category_paths` 作为唯一真理源 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

- 既有手拖拽分类归入行为：保持原有 `addPathsToCategory` 逻辑完全不变；
- 既有样式与控件尺寸：零修改，保持 100% 原样；
- 用户体验改进：使摆设的监听文件夹功能真正发挥效用，并达成多分类无损追加绑定的设计预期。
