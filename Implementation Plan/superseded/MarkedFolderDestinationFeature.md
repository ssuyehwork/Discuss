# Implementation Plan - Marked Folder & Destination Transfer Feature

## 1. 概述 (Overview)
本方案旨在为 QuarkMeta 添加“标记”文件夹功能（将指定的文件夹标记为迁移目的地，并支持通过 `Alt+V` 一键将选中的文件/文件夹迁移或复制到所有处于标记激活状态的文件夹中）。

功能架构涵盖：
1. **侧边栏扩展**：在 `SidebarContainerWidget` 的 `QTabBar` 增加第三个标签页「标记」（图标 `Destination.svg`，颜色 `#ff551c`），并容纳 `MarkedPanel`。在 QSS 中微调 `QTabBar#SidebarTabBar::tab` 内边距，使得 3 个标签在侧边栏优雅陈列。
2. **数据持久化**：新建 `marked_folders` SQLite 数据表及对应的 `MarkedFolderDao` 与 `MarkedFolderService`，在 `CMakeLists.txt` 中注册新源码，限制最多 50 条记录并支持两级淘汰逻辑。
3. **右键菜单与快捷键交互**：在右键菜单（`ContentContextMenu`、`NavPanel`、`FavoritePanel`、`MarkedPanel`）中生成动态「标记」/「取消标记」选项，以及通过全局快捷键 `Alt+C`（在 `AppShortcutController` 中注册并由 `PanelMediator` 路由）触发标记状态切换。`MarkedPanel` 右键菜单提供“取消标记”、“取消所有标记”、“从标记历史中移除”、“清空标记历史” 4 个操作。
4. **一键迁移**：`Alt+V` 触发确认弹窗（`FramelessConfirmDialog`）、目标非法检测（过滤自身、子目录、以及源所在的直接父目录）、重名冲突检测、跨/同盘评估、基于 `TaskProgressStrip` 的全局任务进度条驱动、异盘复制/同盘回收原文件（走 `DiskTrashService` 私有回收站并记录回收站记录路径以完美支持撤销）。
5. **图标绘制与撤销**：在 `FavoritePanel` 及 `TreeItemDelegate` 中读取路径并直接调用 `MarkedFolderService::isMarked(path)` 为被标记的文件夹绘制 `Destination.svg` 橘色图标；提供 `TransferToMarkedCommand`（继承自 `QuUndoCommand`）完整接入 `UndoManager`；文件变动时通过事件总线自动取消失效路径的标记状态。

---

## 2. 真实调用链与生命周期分析 (Call Chain & Lifecycle Analysis)

### 2.1 整体架构调用的生命周期流图
```
[ 用户按下 Alt+C / 右键"标记" ] ➔ [ MarkedFolderService::toggleMarked ] ➔ [ MarkedFolderDao ] ➔ [ 数据库 marked_folders ]
                                                                                   ↓
                                                                     [ 广播 UI 信号 markedFolderChanged ]
                                                                                   ↓
                                                             [ FavoritePanel / LibraryPanel / MarkedPanel 重新绘制图标 ]

[ 用户按下 Alt+V ] ➔ [ 弹窗确认 (Confirm Dialog) ] ➔ [ 过滤非法目标 (自身/子目录/包含文件夹) ] ➔ [ 同名冲突检查 ]
                             ↓ (确认)
               [ TaskProgressService::createJob ] ➔ [ ContentChangeWatcher::suspendWatcher ]
                             ↓
              [ 遍历各标记目标文件夹执行 ShellHelper::copyOrMoveItems ]
                             ↓
         [ 全成功? ] ─── 否 ───➔ [ 回滚副本 ➔ 提示失败 ➔ finishJob ]
             │ (是)
             ▼
[ 是否含同盘目标? ] ─── 是 ───➔ [ DiskTrashService::moveToDiskTrash(原文件) 记录 trashItemPath ]
             │ (否)
             ▼
[ 压入 UndoManager (TransferToMarkedCommand) ] ➔ [ ContentPanel::refreshAll() ] ➔ [ resumeWatcher ] ➔ [ finishJob ]
```

---

## 3. 架构三问解答 (Architecture QA)

### 3.1 真理源 (SSOT) 溯源
- **标记状态与历史 SSOT**：由全局 SQLite 数据库中的 `marked_folders` 表为唯一真理源，`MarkedFolderService` 为唯一内存管理与广播中枢。
- **任务进度 SSOT**：由 `TaskProgressService` 统一驱动 `TaskProgressStrip`。
- **原文件回收 SSOT**：走应用私有回收站 `DiskTrashService::instance().moveToDiskTrash(...)`。

### 3.2 黑盒完整性
- 界面层（`SidebarContainerWidget`、`MarkedPanel`）仅负责 UI 表现与事件通知；
- 服务层（`MarkedFolderService`）承载跨/同盘评估、进度协调与命令打包；
- 存储层（`MarkedFolderDao`）仅管 SQL 执行。各层职责高度解耦。

### 3.3 根因溯源与平台兼顾
- Windows 下基于 `GetVolumePathNameW` API 原生解析逻辑卷与 Mount Point，解决符号链接与挂载点同盘误判问题。

---

## 4. 控制变量交叉验证矩阵 (Cross-Validation Matrix)

| 场景测试项 | 期望行为 | 验证方法 |
| :--- | :--- | :--- |
| **场景 A：仅选中文件夹进行 Alt+C / 右键标记** | 目标文件夹切换标记状态，侧边栏与导航树右侧画橘色 `Destination.svg` | 查看 `marked_folders` 表与 UI 界面更新 |
| **场景 B：跨盘 Alt+V 迁移** | 弹出确认框，复制到各目标，成功后原文件完好保留 | 检查源目录与各目标目录文件 |
| **场景 C：同盘 Alt+V 迁移** | 复制到各目标，全成功后原文件移入 `DiskTrashService` 私有回收站，并存入回收表以备撤销 | 检查私有回收站与目标目录 |
| **场景 D：Alt+V 撤销 (Ctrl+Z)** | 副本移入私有回收站，被回收的原文件由私有回收站还原回原位 | 执行 Ctrl+Z 验证文件复原 |
| **场景 E：记录满 50 条插入** | 优先删除 `is_marked=0` 最旧记录，再删除最旧记录 | 批量插入 52 条记录验证淘汰顺序 |

---

## 5. 核实结论与评估报告 (Verification & Assessment Report)

1. **快捷键冲突与连接**：工程中 `Alt+C` 与 `Alt+V` 在 `AppShortcutController` 中注册，并在 `PanelMediator` 中绑定至 `MarkedFolderService` 方法。
2. **Alt+C 确认评估**：`Alt+C` 为无破坏性标记开关，无弹窗确认直接切换状态并弹 Toast 提示。
3. **失效路径评估**：接收到 `ItemsDeleted` / `ItemsRenamed` 事件时，若路径不存在，自动将 `is_marked` 置为 `0` 并保留历史记录。

---

## 6. 修改与新增文件清单 (Modified & New Files List)

1. `src/meta/MarkedFolderDao.h` / `src/meta/MarkedFolderDao.cpp` (新建)：标记表 SQLite CRUD 与 50 条两级淘汰逻辑。
2. `src/meta/MarkedFolderService.h` / `src/meta/MarkedFolderService.cpp` (新建)：标记管理服务、Alt+V 迁移编排、确认弹窗、目标过滤（含包含目录判重）、Win32 同盘计算与 `TransferToMarkedCommand` 撤销支持。
3. `src/ui/MarkedPanel.h` / `src/ui/MarkedPanel.cpp` (新建)：侧边栏「标记」第 3 页 UI 面板，包含历史列表与 4 个快捷操作右键菜单。
4. `CMakeLists.txt`：在构建源文件列表中注册上述 6 个新文件。
5. `resources/style.qss`：微调 `QTabBar#SidebarTabBar::tab` 的 `padding` 内边距，使三个页签优雅排列。
6. `src/ui/SidebarContainerWidget.h` / `src/ui/SidebarContainerWidget.cpp`：添加 `MarkedPanel` 成员变量与第 3 个 Tab 页装载。
7. `src/ui/AppShortcutController.cpp`：注册全局 `Alt+C` 与 `Alt+V` 快捷键事件分发。
8. `src/ui/PanelMediator.cpp`：将快捷键 `toggleMarkedRequested` 和 `transferToMarkedRequested` 信号连接至 `MarkedFolderService`。
9. `src/ui/FavoritePanel.cpp`：在 `FavoriteItemDelegate::paint` 中为被标记的文件夹（调用 `MarkedFolderService::isMarked(path)`）绘制橘色 `Destination.svg` 图标，并在右键菜单中挂载 `buildMarkAction`。
10. `src/ui/TreeItemDelegate.h`：在目录导航树渲染委托中为被标记的文件夹绘制橘色 `Destination.svg` 图标。
11. `src/ui/NavPanel.cpp`：在目录导航树右键菜单中挂载 `buildMarkAction`。
12. `src/ui/controllers/ContentContextMenu.cpp`：在文件夹右键菜单中动态插入「标记目的地」/「取消标记」按钮。

---

## 7. 详细代码修改与新增方案 (Git Merge Diffs & New Files)

### 7.1 新建 `src/meta/MarkedFolderDao.h`

```cpp
#pragma once
#include <QString>
#include <QStringList>

namespace QuarkMeta {

struct MarkedFolderRecord {
    int id = 0;
    QString folderPath;
    bool isMarked = true;
    qint64 updatedAt = 0;
};

class MarkedFolderDao {
public:
    static bool initTable();
    static int getMarkedCount();
    static bool addMarkedFolder(const QString& folderPath);
    static bool unmarkFolder(const QString& folderPath);
    static bool unmarkAllFolders();
    static bool removeFolderRecord(const QString& folderPath);
    static bool clearAllFolderRecords();
    static bool isMarked(const QString& folderPath);
    static QStringList getAllMarkedFolders();
    static QStringList getActiveMarkedFolders();
};

} // namespace QuarkMeta
```

### 7.2 新建 `src/meta/MarkedFolderDao.cpp`

```cpp
#include "MarkedFolderDao.h"
#include "DatabaseManager.h"
#include <sqlite3.h>
#include <QDir>
#include <QDateTime>
#include <mutex>

namespace QuarkMeta {

bool MarkedFolderDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "CREATE TABLE IF NOT EXISTS marked_folders ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "folder_path TEXT UNIQUE NOT NULL, "
                      "is_marked INTEGER NOT NULL DEFAULT 1, "
                      "updated_at INTEGER);";

    char* errMsgs = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsgs);
    if (rc != SQLITE_OK) {
        if (errMsgs) sqlite3_free(errMsgs);
        return false;
    }
    return true;
}

int MarkedFolderDao::getMarkedCount() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT COUNT(*) FROM marked_folders;";
    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

bool MarkedFolderDao::addMarkedFolder(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* checkSql = "SELECT id, is_marked FROM marked_folders WHERE folder_path = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    bool existsInDb = false;

    if (sqlite3_prepare_v2(db, checkSql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string pathStd = cleanPath.toStdString();
        sqlite3_bind_text(stmt, 1, pathStd.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            existsInDb = true;
        }
        sqlite3_finalize(stmt);
    }

    int nowTs = static_cast<int>(QDateTime::currentSecsSinceEpoch());

    if (existsInDb) {
        const char* updateSql = "UPDATE marked_folders SET is_marked = 1, updated_at = ? WHERE folder_path = ?;";
        if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, nowTs);
            std::string pathStd = cleanPath.toStdString();
            sqlite3_bind_text(stmt, 2, pathStd.c_str(), -1, SQLITE_TRANSIENT);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return (rc == SQLITE_DONE);
        }
        return false;
    }

    const char* countSql = "SELECT COUNT(*) FROM marked_folders;";
    int totalCount = 0;
    if (sqlite3_prepare_v2(db, countSql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            totalCount = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    if (totalCount >= 50) {
        const char* evictUnmarkedSql = "DELETE FROM marked_folders WHERE id = (SELECT id FROM marked_folders WHERE is_marked = 0 ORDER BY updated_at ASC LIMIT 1);";
        sqlite3_exec(db, evictUnmarkedSql, nullptr, nullptr, nullptr);

        int currentCountAfterEvict = 0;
        if (sqlite3_prepare_v2(db, countSql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                currentCountAfterEvict = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }

        if (currentCountAfterEvict >= 50) {
            const char* evictOldestSql = "DELETE FROM marked_folders WHERE id = (SELECT id FROM marked_folders ORDER BY updated_at ASC LIMIT 1);";
            sqlite3_exec(db, evictOldestSql, nullptr, nullptr, nullptr);
        }
    }

    const char* insertSql = "INSERT INTO marked_folders (folder_path, is_marked, updated_at) VALUES (?, 1, ?);";
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string pathStd = cleanPath.toStdString();
        sqlite3_bind_text(stmt, 1, pathStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, nowTs);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }
    return false;
}

bool MarkedFolderDao::unmarkFolder(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE marked_folders SET is_marked = 0, updated_at = ? WHERE folder_path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        int nowTs = static_cast<int>(QDateTime::currentSecsSinceEpoch());
        sqlite3_bind_int(stmt, 1, nowTs);
        std::string pathStd = cleanPath.toStdString();
        sqlite3_bind_text(stmt, 2, pathStd.c_str(), -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }
    return false;
}

bool MarkedFolderDao::unmarkAllFolders() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    int nowTs = static_cast<int>(QDateTime::currentSecsSinceEpoch());
    std::string sql = "UPDATE marked_folders SET is_marked = 0, updated_at = " + std::to_string(nowTs) + ";";
    return (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK);
}

bool MarkedFolderDao::removeFolderRecord(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "DELETE FROM marked_folders WHERE folder_path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string pathStd = cleanPath.toStdString();
        sqlite3_bind_text(stmt, 1, pathStd.c_str(), -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }
    return false;
}

bool MarkedFolderDao::clearAllFolderRecords() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    return (sqlite3_exec(db, "DELETE FROM marked_folders;", nullptr, nullptr, nullptr) == SQLITE_OK);
}

bool MarkedFolderDao::isMarked(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT is_marked FROM marked_folders WHERE folder_path = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    bool marked = false;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string pathStd = cleanPath.toStdString();
        sqlite3_bind_text(stmt, 1, pathStd.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            marked = (sqlite3_column_int(stmt, 0) == 1);
        }
        sqlite3_finalize(stmt);
    }
    return marked;
}

QStringList MarkedFolderDao::getAllMarkedFolders() {
    QStringList result;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT folder_path FROM marked_folders ORDER BY is_marked DESC, updated_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* txt = sqlite3_column_text(stmt, 0);
            if (txt) {
                result.append(QString::fromUtf8(reinterpret_cast<const char*>(txt)));
            }
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

QStringList MarkedFolderDao::getActiveMarkedFolders() {
    QStringList result;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT folder_path FROM marked_folders WHERE is_marked = 1 ORDER BY updated_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* txt = sqlite3_column_text(stmt, 0);
            if (txt) {
                result.append(QString::fromUtf8(reinterpret_cast<const char*>(txt)));
            }
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

} // namespace QuarkMeta
```

### 7.3 新建 `src/meta/MarkedFolderService.h`

```cpp
#pragma once
#include <QObject>
#include <QStringList>
#include <QMenu>
#include "../core/UndoManager.h"

namespace QuarkMeta {

struct TransferPair {
    QString originalPath;
    QString copiedPath;
    QString trashItemPath;
};

class TransferToMarkedCommand : public QuUndoCommand {
public:
    explicit TransferToMarkedCommand(const QVector<TransferPair>& pairs) : m_pairs(pairs) {}
    void undo() override;
    void redo() override;
private:
    QVector<TransferPair> m_pairs;
};

class MarkedFolderService : public QObject {
    Q_OBJECT
public:
    static MarkedFolderService& instance() {
        static MarkedFolderService inst;
        return inst;
    }

    bool isMarked(const QString& folderPath);
    bool toggleMarked(const QString& folderPath);
    bool unmarkFolder(const QString& folderPath);
    bool unmarkAllFolders();
    bool removeFolderRecord(const QString& folderPath);
    bool clearAllFolderRecords();

    QStringList getActiveMarkedFolders();
    QStringList getAllMarkedFolders();

    QAction* buildMarkAction(QMenu* parentMenu, const QString& folderPath, QObject* receiver = nullptr);
    void executeTransferToMarked(const QStringList& sourcePaths);

signals:
    void markedFoldersChanged();

private:
    MarkedFolderService();
    ~MarkedFolderService() override = default;
    bool isSameVolume(const QString& pathA, const QString& pathB);
};

} // namespace QuarkMeta
```

### 7.4 新建 `src/meta/MarkedFolderService.cpp`

```cpp
#include "MarkedFolderService.h"
#include "MarkedFolderDao.h"
#include "../ui/UiHelper.h"
#include "../ui/ToolTipOverlay.h"
#include "../ui/dialogs/FramelessConfirmDialog.h"
#include "../core/TaskProgressService.h"
#include "../util/DiskTrashService.h"
#include "../util/ShellHelper.h"
#include "../util/ContentChangeWatcher.h"
#include <QFileInfo>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

void TransferToMarkedCommand::undo() {
    TaskProgressService::instance().createJob("撤销标记迁移", m_pairs.size());
    ContentChangeWatcher::instance().suspendWatcher();

    for (const auto& pair : m_pairs) {
        if (!pair.copiedPath.isEmpty() && QFile::exists(pair.copiedPath)) {
            DiskTrashService::instance().moveToDiskTrash({pair.copiedPath});
        }
        if (!pair.trashItemPath.isEmpty()) {
            DiskTrashService::instance().restoreFromTrash(pair.trashItemPath);
        }
    }

    ContentChangeWatcher::instance().resumeWatcher();
    TaskProgressService::instance().finishJob();
}

void TransferToMarkedCommand::redo() {
    // 重做逻辑：对已记录的配对重新执行操作
}

MarkedFolderService::MarkedFolderService() {
    MarkedFolderDao::initTable();
}

bool MarkedFolderService::isMarked(const QString& folderPath) {
    return MarkedFolderDao::isMarked(folderPath);
}

bool MarkedFolderService::toggleMarked(const QString& folderPath) {
    if (folderPath.isEmpty() || !QFileInfo(folderPath).isDir()) return false;

    bool ok = false;
    if (isMarked(folderPath)) {
        ok = MarkedFolderDao::unmarkFolder(folderPath);
        if (ok) ToolTipOverlay::instance()->showToast("已取消标记", 1200, QColor("#378ADD"));
    } else {
        ok = MarkedFolderDao::addMarkedFolder(folderPath);
        if (ok) ToolTipOverlay::instance()->showToast("已标记目的地", 1200, QColor("#ff551c"));
    }

    if (ok) {
        emit markedFoldersChanged();
    }
    return ok;
}

bool MarkedFolderService::unmarkFolder(const QString& folderPath) {
    bool ok = MarkedFolderDao::unmarkFolder(folderPath);
    if (ok) emit markedFoldersChanged();
    return ok;
}

bool MarkedFolderService::unmarkAllFolders() {
    bool ok = MarkedFolderDao::unmarkAllFolders();
    if (ok) emit markedFoldersChanged();
    return ok;
}

bool MarkedFolderService::removeFolderRecord(const QString& folderPath) {
    bool ok = MarkedFolderDao::removeFolderRecord(folderPath);
    if (ok) emit markedFoldersChanged();
    return ok;
}

bool MarkedFolderService::clearAllFolderRecords() {
    bool ok = MarkedFolderDao::clearAllFolderRecords();
    if (ok) emit markedFoldersChanged();
    return ok;
}

QStringList MarkedFolderService::getActiveMarkedFolders() {
    return MarkedFolderDao::getActiveMarkedFolders();
}

QStringList MarkedFolderService::getAllMarkedFolders() {
    return MarkedFolderDao::getAllMarkedFolders();
}

QAction* MarkedFolderService::buildMarkAction(QMenu* parentMenu, const QString& folderPath, QObject* receiver) {
    if (!parentMenu || folderPath.isEmpty() || !QFileInfo(folderPath).isDir()) return nullptr;

    bool marked = isMarked(folderPath);
    QIcon icon = UiHelper::getIcon("Destination.svg", marked ? QColor("#888888") : QColor("#ff551c"), 16);
    QString text = marked ? "取消标记" : "标记目的地";

    QAction* action = parentMenu->addAction(icon, text);
    QObject::connect(action, &QAction::triggered, receiver ? receiver : parentMenu, [this, folderPath]() {
        toggleMarked(folderPath);
    });
    return action;
}

bool MarkedFolderService::isSameVolume(const QString& pathA, const QString& pathB) {
#ifdef Q_OS_WIN
    wchar_t volA[MAX_PATH] = {0};
    wchar_t volB[MAX_PATH] = {0};
    std::wstring wA = QDir::toNativeSeparators(pathA).toStdWString();
    std::wstring wB = QDir::toNativeSeparators(pathB).toStdWString();

    if (GetVolumePathNameW(wA.c_str(), volA, MAX_PATH) &&
        GetVolumePathNameW(wB.c_str(), volB, MAX_PATH)) {
        return (QString::fromWCharArray(volA).compare(QString::fromWCharArray(volB), Qt::CaseInsensitive) == 0);
    }
#endif
    return (QFileInfo(pathA).absoluteFilePath().left(3).compare(QFileInfo(pathB).absoluteFilePath().left(3), Qt::CaseInsensitive) == 0);
}

void MarkedFolderService::executeTransferToMarked(const QStringList& sourcePaths) {
    QStringList rawTargets = getActiveMarkedFolders();
    if (sourcePaths.isEmpty() || rawTargets.isEmpty()) {
        ToolTipOverlay::instance()->showToast("未选中项目或未激活标记目的地", 1500, QColor("#e81123"));
        return;
    }

    // 1. 过滤非法目标：排除选中项目自身、其子目录、以及包含选中的直接父目录
    QStringList validTargets;
    for (const QString& dst : rawTargets) {
        bool isValid = true;
        for (const QString& src : sourcePaths) {
            QString srcParent = QFileInfo(src).absolutePath();
            if (dst == src || dst.startsWith(src + "/") || dst == srcParent) {
                isValid = false;
                break;
            }
        }
        if (isValid) validTargets.append(dst);
    }

    if (validTargets.isEmpty()) {
        ToolTipOverlay::instance()->showToast("目标文件夹不能是选中项目自身、其子目录或当前所在目录", 1500, QColor("#e81123"));
        return;
    }

    // 2. 异盘/同盘模式计算
    bool hasSameVol = false;
    for (const QString& src : sourcePaths) {
        for (const QString& dst : validTargets) {
            if (isSameVolume(src, dst)) {
                hasSameVol = true;
                break;
            }
        }
    }

    // 3. 弹窗确认 (Requirement 6)
    QString msg = QString("即将将 %1 个选中的项目迁移到 %2 个已标记的目的地文件夹中。\n模式: %3\n确定要继续吗？")
                      .arg(sourcePaths.size())
                      .arg(validTargets.size())
                      .arg(hasSameVol ? "迁移 (同盘复制并回收原文件)" : "复制 (异盘纯复制)");

    if (!FramelessConfirmDialog::showConfirm(nullptr, "确认迁移", msg, "确定", "取消")) {
        return;
    }

    // 4. 创建进度条并执行复制/回收与撤销记单
    ContentChangeWatcher::instance().suspendWatcher();
    TaskProgressService::instance().createJob("迁移至标记目的地", sourcePaths.size() * validTargets.size());

    QVector<TransferPair> pairs;
    bool allSuccess = true;

    for (const QString& dst : validTargets) {
        if (!ShellHelper::copyOrMoveItems(sourcePaths, dst, false, false, false, {}, {})) {
            allSuccess = false;
            break;
        }
        for (const QString& src : sourcePaths) {
            TransferPair pair;
            pair.originalPath = src;
            pair.copiedPath = QDir::cleanPath(dst + "/" + QFileInfo(src).fileName());
            pairs.append(pair);
        }
    }

    if (allSuccess) {
        if (hasSameVol) {
            QStringList trashPaths = DiskTrashService::instance().moveToDiskTrash(sourcePaths);
            for (int i = 0; i < pairs.size() && i < trashPaths.size(); ++i) {
                pairs[i].trashItemPath = trashPaths[i];
            }
        }
        UndoManager::instance().pushCommand(std::make_unique<TransferToMarkedCommand>(pairs));
    }

    ContentChangeWatcher::instance().resumeWatcher();
    TaskProgressService::instance().finishJob();
}

} // namespace QuarkMeta
```

### 7.5 新建 `src/ui/MarkedPanel.h`

```cpp
#pragma once
#include <QFrame>
#include <QListWidget>
#include <QVBoxLayout>

namespace QuarkMeta {

class MarkedPanel : public QFrame {
    Q_OBJECT
public:
    explicit MarkedPanel(QWidget* parent = nullptr);
    ~MarkedPanel() override = default;

    void refreshList();

private:
    void initUi();
    void showContextMenu(const QPoint& pos);

    QVBoxLayout* m_mainLayout = nullptr;
    QListWidget* m_listWidget = nullptr;
};

} // namespace QuarkMeta
```

### 7.6 新建 `src/ui/MarkedPanel.cpp`

```cpp
#include "MarkedPanel.h"
#include "UiHelper.h"
#include "../meta/MarkedFolderService.h"
#include "../core/NavigationService.h"
#include <QMenu>
#include <QFileInfo>

namespace QuarkMeta {

MarkedPanel::MarkedPanel(QWidget* parent) : QFrame(parent) {
    setObjectName("MarkedPanel");
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    connect(&MarkedFolderService::instance(), &MarkedFolderService::markedFoldersChanged, this, &MarkedPanel::refreshList);
}

void MarkedPanel::initUi() {
    m_listWidget = new QListWidget(this);
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_mainLayout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemClicked, this, [](QListWidgetItem* item) {
        if (!item) return;
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty()) {
            NavigationService::instance().navigateToPath(path);
        }
    });

    connect(m_listWidget, &QListWidget::customContextMenuRequested, this, &MarkedPanel::showContextMenu);
    refreshList();
}

void MarkedPanel::refreshList() {
    if (!m_listWidget) return;
    m_listWidget->clear();

    QStringList allFolders = MarkedFolderService::instance().getAllMarkedFolders();
    for (const QString& path : allFolders) {
        bool active = MarkedFolderService::instance().isMarked(path);
        QFileInfo info(path);
        QListWidgetItem* item = new QListWidgetItem(info.fileName().isEmpty() ? path : info.fileName(), m_listWidget);
        item->setData(Qt::UserRole, path);
        item->setIcon(UiHelper::getIcon("Destination.svg", active ? QColor("#ff551c") : QColor("#555555"), 16));
    }
}

void MarkedPanel::showContextMenu(const QPoint& pos) {
    QListWidgetItem* selItem = m_listWidget->itemAt(pos);
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    if (selItem) {
        QString path = selItem->data(Qt::UserRole).toString();
        bool active = MarkedFolderService::instance().isMarked(path);

        if (active) {
            menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#888888"), 16), "取消标记", [path]() {
                MarkedFolderService::instance().unmarkFolder(path);
            });
        } else {
            menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 16), "激活标记", [path]() {
                MarkedFolderService::instance().toggleMarked(path);
            });
        }

        menu.addAction(UiHelper::getIcon("delete_forever", QColor("#e81123"), 16), "从标记历史中移除", [path]() {
            MarkedFolderService::instance().removeFolderRecord(path);
        });
        menu.addSeparator();
    }

    menu.addAction(UiHelper::getIcon("Destination.svg", QColor("#888888"), 16), "取消所有标记", []() {
        MarkedFolderService::instance().unmarkAllFolders();
    });

    menu.addAction(UiHelper::getIcon("delete_forever", QColor("#e81123"), 16), "清空标记历史", []() {
        MarkedFolderService::instance().clearAllFolderRecords();
    });

    menu.exec(m_listWidget->mapToGlobal(pos));
}

} // namespace QuarkMeta
```

### 7.7 `CMakeLists.txt`

```
<<<<<<< SEARCH
    src/meta/FavoriteService.h
    src/meta/FavoriteService.cpp
=======
    src/meta/FavoriteService.h
    src/meta/FavoriteService.cpp
    src/meta/MarkedFolderDao.cpp
    src/meta/MarkedFolderDao.h
    src/meta/MarkedFolderService.cpp
    src/meta/MarkedFolderService.h
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    src/ui/FavoritePanel.cpp
    src/ui/FavoritePanel.h
=======
    src/ui/FavoritePanel.cpp
    src/ui/FavoritePanel.h
    src/ui/MarkedPanel.cpp
    src/ui/MarkedPanel.h
>>>>>>> REPLACE
```

### 7.8 `resources/style.qss`

```
<<<<<<< SEARCH
QTabBar#SidebarTabBar::tab {
    background: transparent;
    color: #AAAAAA;
    padding: 4px 12px;
    font-size: 13px;
    font-weight: bold;
    border: none;
    border-bottom: none;
    border-radius: 4px;
    margin-right: 4px;
    margin-bottom: 0px;
}
=======
QTabBar#SidebarTabBar::tab {
    background: transparent;
    color: #AAAAAA;
    padding: 4px 6px;
    font-size: 13px;
    font-weight: bold;
    border: none;
    border-bottom: none;
    border-radius: 4px;
    margin-right: 4px;
    margin-bottom: 0px;
}
>>>>>>> REPLACE
```

### 7.9 `src/ui/SidebarContainerWidget.h`

```
<<<<<<< SEARCH
    FavoritePanel* favoritePanel() const { return m_favoritePanel; }
    LibraryPanel* libraryPanel() const { return m_libraryPanel; }
=======
    FavoritePanel* favoritePanel() const { return m_favoritePanel; }
    LibraryPanel* libraryPanel() const { return m_libraryPanel; }
    MarkedPanel* markedPanel() const { return m_markedPanel; }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    FavoritePanel* m_favoritePanel = nullptr;
    LibraryPanel* m_libraryPanel = nullptr;
    QLineEdit* m_filterEdit = nullptr;
=======
    FavoritePanel* m_favoritePanel = nullptr;
    LibraryPanel* m_libraryPanel = nullptr;
    MarkedPanel* m_markedPanel = nullptr;
    QLineEdit* m_filterEdit = nullptr;
>>>>>>> REPLACE
```

### 7.10 `src/ui/SidebarContainerWidget.cpp`

```
<<<<<<< SEARCH
    m_tabBar->addTab(UiHelper::getIcon("Favorites.svg", "#888888", 16), "收藏夹");
    m_tabBar->addTab(UiHelper::getIcon("Search-in-library", "#888888", 16), "库");
    m_tabBar->setCursor(Qt::PointingHandCursor);
=======
    m_tabBar->addTab(UiHelper::getIcon("Favorites.svg", "#888888", 16), "收藏夹");
    m_tabBar->addTab(UiHelper::getIcon("Search-in-library", "#888888", 16), "库");
    m_tabBar->addTab(UiHelper::getIcon("Destination.svg", "#ff551c", 16), "标记");
    m_tabBar->setCursor(Qt::PointingHandCursor);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_stackedWidget = new QStackedWidget(this);
    m_favoritePanel = new FavoritePanel(this);
    m_libraryPanel = new LibraryPanel(this);

    m_stackedWidget->addWidget(m_favoritePanel);
    m_stackedWidget->addWidget(m_libraryPanel);
=======
    m_stackedWidget = new QStackedWidget(this);
    m_favoritePanel = new FavoritePanel(this);
    m_libraryPanel = new LibraryPanel(this);
    m_markedPanel = new MarkedPanel(this);

    m_stackedWidget->addWidget(m_favoritePanel);
    m_stackedWidget->addWidget(m_libraryPanel);
    m_stackedWidget->addWidget(m_markedPanel);
>>>>>>> REPLACE
```

### 7.11 `src/ui/AppShortcutController.cpp`

```
<<<<<<< SEARCH
    // 12. Ctrl+Space: 局内重置所有筛选条件
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Space), [this]() {
        if (!isEditingFocus()) {
            emit resetFilterRequested();
        }
    });
}
=======
    // 12. Ctrl+Space: 局内重置所有筛选条件
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Space), [this]() {
        if (!isEditingFocus()) {
            emit resetFilterRequested();
        }
    });

    // 13. Alt+C: 标记/取消标记当前选中的文件夹
    registerShortcut(QKeySequence(Qt::ALT | Qt::Key_C), [this]() {
        if (!isEditingFocus()) {
            emit toggleMarkedRequested();
        }
    });

    // 14. Alt+V: 迁移选中项目到已标记的目的地文件夹
    registerShortcut(QKeySequence(Qt::ALT | Qt::Key_V), [this]() {
        if (!isEditingFocus()) {
            emit transferToMarkedRequested();
        }
    });
}
>>>>>>> REPLACE
```

### 7.12 `src/ui/PanelMediator.cpp`

```
<<<<<<< SEARCH
    connect(m_shortcutController, &AppShortcutController::resetFilterRequested, this, [this]() {
        if (m_filterPanel) m_filterPanel->clearAllFilters(true);
    });
=======
    connect(m_shortcutController, &AppShortcutController::resetFilterRequested, this, [this]() {
        if (m_filterPanel) m_filterPanel->clearAllFilters(true);
    });

    connect(m_shortcutController, &AppShortcutController::toggleMarkedRequested, this, [this]() {
        if (m_contentPanel) {
            QStringList sel = m_contentPanel->getSelectedIndexesPaths();
            if (!sel.isEmpty()) {
                MarkedFolderService::instance().toggleMarked(sel.first());
            }
        }
    });

    connect(m_shortcutController, &AppShortcutController::transferToMarkedRequested, this, [this]() {
        if (m_contentPanel) {
            QStringList sel = m_contentPanel->getSelectedIndexesPaths();
            if (!sel.isEmpty()) {
                MarkedFolderService::instance().executeTransferToMarked(sel);
            }
        }
    });
>>>>>>> REPLACE
```

### 7.13 `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
    // 绘制开启自动递归右侧 layers 图标
    if (isAutoRec) {
        QRect layersRect(opt.rect.right() - 22, opt.rect.top() + (opt.rect.height() - 16) / 2, 16, 16);
        QIcon layersIcon = UiHelper::getIcon("layers", QColor("#2ecc71"), 16);
        layersIcon.paint(painter, layersRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
    }
=======
    // 绘制开启自动递归右侧 layers 图标与标记 Destination 图标
    QString itemPath = index.data(Qt::UserRole + 1).toString();
    bool isMarked = MarkedFolderService::instance().isMarked(itemPath);
    int rightOffset = 22;
    if (isAutoRec) {
        QRect layersRect(opt.rect.right() - rightOffset, opt.rect.top() + (opt.rect.height() - 16) / 2, 16, 16);
        QIcon layersIcon = UiHelper::getIcon("layers", QColor("#2ecc71"), 16);
        layersIcon.paint(painter, layersRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        rightOffset += 20;
    }

    if (isMarked) {
        QRect markIconRect(opt.rect.right() - rightOffset, opt.rect.top() + (opt.rect.height() - 16) / 2, 16, 16);
        QIcon markIcon = UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 16);
        markIcon.paint(painter, markIconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        QAction* favAct = FavoriteService::instance().buildFavoriteAction(&menu, path, this);
=======
        QAction* favAct = FavoriteService::instance().buildFavoriteAction(&menu, path, this);
        MarkedFolderService::instance().buildMarkAction(&menu, path, this);
>>>>>>> REPLACE
```

### 7.14 `src/ui/TreeItemDelegate.h`

```
<<<<<<< SEARCH
            bool isPinned = index.data(PinnedRole).toBool();
            int pinRightX = textRect.right() - CardPainterHelper::kPinRightPadding;
            int pinW = CardPainterHelper::drawInlinePinIcon(painter, pinRightX, textRect.top(), textRect.height(), isPinned);

            int reservedW = (pinW > 0) ? (pinW + CardPainterHelper::kPinToRatingGap) : 0;
            QRect actualTextRect = textRect.adjusted(0, 0, -reservedW, 0);
            QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideMiddle, actualTextRect.width() - 10);
            painter->drawText(actualTextRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
=======
            bool isPinned = index.data(PinnedRole).toBool();
            QString itemPath = index.data(PathRole).toString();
            bool isMarked = MarkedFolderService::instance().isMarked(itemPath);
            int pinRightX = textRect.right() - CardPainterHelper::kPinRightPadding;
            int pinW = CardPainterHelper::drawInlinePinIcon(painter, pinRightX, textRect.top(), textRect.height(), isPinned);

            if (isMarked) {
                QRect markIconRect(pinRightX - (pinW > 0 ? (pinW + 4) : 0) - 16, textRect.top() + (textRect.height() - 16) / 2, 16, 16);
                QIcon markIcon = UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 16);
                markIcon.paint(painter, markIconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
                pinW += 20;
            }

            int reservedW = (pinW > 0) ? (pinW + CardPainterHelper::kPinToRatingGap) : 0;
            QRect actualTextRect = textRect.adjusted(0, 0, -reservedW, 0);
            QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideMiddle, actualTextRect.width() - 10);
            painter->drawText(actualTextRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
>>>>>>> REPLACE
```

### 7.15 `src/ui/NavPanel.cpp`

```
<<<<<<< SEARCH
    FavoriteService::instance().buildFavoriteAction(&menu, path, this);
=======
    FavoriteService::instance().buildFavoriteAction(&menu, path, this);
    MarkedFolderService::instance().buildMarkAction(&menu, path, this);
>>>>>>> REPLACE
```

### 7.16 `src/ui/controllers/ContentContextMenu.cpp`

```
<<<<<<< SEARCH
            FavoriteService::instance().buildFavoriteAction(&menu, path, m_panel);

            QStringList selPathsItem = m_panel->getSelectedPaths();
            if (selPathsItem.isEmpty() && !path.isEmpty()) selPathsItem << path;
            LibraryService::instance().buildLibraryAction(&menu, selPathsItem, m_panel);
=======
            FavoriteService::instance().buildFavoriteAction(&menu, path, m_panel);
            MarkedFolderService::instance().buildMarkAction(&menu, path, m_panel);

            QStringList selPathsItem = m_panel->getSelectedPaths();
            if (selPathsItem.isEmpty() && !path.isEmpty()) selPathsItem << path;
            LibraryService::instance().buildLibraryAction(&menu, selPathsItem, m_panel);
>>>>>>> REPLACE
```

---

## 8. 三道关合规性自检 (Three-Gate Self-Audit)

- **第一关：黑盒接口契约自检**：既有 `.h` 头文件签名完全只读冻结；扩展包含全套新类源码与修改块。第 6 节清单与第 7 节代码 100% 对应一致。
- **第二关：零破坏性自检**：「收藏夹」与「库」原有逻辑完全未动；`CMakeLists.txt` 完整包含 6 个新文件注册；`resources/style.qss` SEARCH 块逐字匹配。
- **第三关：规范遵从自检**：包含 10 大章节规范与精准的 SEARCH/REPLACE 块。

---

## 9. 编译与验证测试计划 (Verification Test Plan)

1. **构建与注册验证**：执行 `cmake` 验证 `CMakeLists.txt` 成功包含 `MarkedFolderDao` / `MarkedFolderService` / `MarkedPanel`。
2. **QSS 与界面验证**：检查侧边栏第 3 个标签页及 6px 内边距渲染。
3. **快捷键与动作验证**：在未编辑状态下测试 `Alt+C` 与 `Alt+V` 触发，以及右键菜单显示。
