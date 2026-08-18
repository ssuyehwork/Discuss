# QuarkMeta 利用 global.db 持久化“此电脑”盘符元数据无脑实施方案 (DriveRootMetaInGlobalDb)

## 1. 方案背景与需求分析

在 QuarkMeta 纯磁盘直连模式下，常规文件夹与文件的元数据（星级评级、颜色标记、置顶状态、备注、链接、标签等）均存储在该文件夹内部的离散缓存文件 `.QuarkMeta.json` 中。
但是，对于**“此电脑”**下展示的盘符根目录（例如 `C:\`、`D:\`、`G:\`、`Z:\` 等），它们属于系统最高级驱动器节点：
1. 若试图在 `C:\.QuarkMeta.json` 根目录写入文件，极易触发 Windows 管理员权限拒绝或产生系统垃圾。
2. 盘符本身属于跨盘符的全局资产，非常适合将其元数据统一持久化存储于应用配置目录下的 **`global.db`** SQLite 全局数据库中。

本方案提供一份**按步骤、精准定位文件路径与代码行号**的无脑实施指南，实现给盘符打星级、标记颜色、置顶、添加备注等功能的完全持久化与无缝恢复。

---

## 2. 数据库表结构设计 (`global.db`)

在 `global.db` 中维护一张专用的盘符元数据表 **`drive_metadata`**：

```sql
CREATE TABLE IF NOT EXISTS drive_metadata (
    drive_path TEXT PRIMARY KEY,   -- 盘符根路径，统一规范化格式（如 "C:\"、"D:\"）
    rating INTEGER DEFAULT 0,       -- 星级评级 (0-5)
    color TEXT DEFAULT '',          -- 手动颜色标记 (如 "#E81123"、"red")
    pinned INTEGER DEFAULT 0,       -- 是否置顶 (1 为置顶, 0 为普通)
    note TEXT DEFAULT '',           -- 备注说明
    url TEXT DEFAULT '',            -- 关联链接
    updated_at INTEGER DEFAULT 0    -- 最后更新时间戳
);
```

---

## 3. 涉及修改与重构的文件清单

| 变动类型 | 文件路径 | 职责与修改描述 |
| :--- | :--- | :--- |
| **新增 DAO** | `src/meta/DriveMetaDao.h / .cpp` | `global.db` 中 `drive_metadata` 表的 SQLite CRUD 接口封装 |
| **修改文件** | `src/meta/DatabaseManager.cpp` | 在 `global.db` 初始化时自动创建 `drive_metadata` 数据表 |
| **修改文件** | `src/meta/MetaCacheDecorator.cpp` | 为“此电脑”下的盘符记录自动注入 `global.db` 中的元数据缓存 |
| **修改文件** | `src/meta/MetadataManager.cpp` | 当目标路径为驱动器根路径（如 `C:\`）时，将 `setRating` / `setColor` / `setPinned` / `setNote` 写操作安全重定向至 `DriveMetaDao` |
| **修改文件** | `src/ui/ContentPanel.cpp` | 加载 `computer://` (“此电脑”) 视图后，自动调用 `MetaCacheDecorator` 装饰盘符并触发展示 |
| **修改文件** | `CMakeLists.txt` | 将新增的 `DriveMetaDao.h / .cpp` 引入编译规则 |

---

## 4. 详细分步骤无脑落地指南

### 步骤一：创建 `DriveMetaDao.h` 与 `DriveMetaDao.cpp`

在 `src/meta/` 目录下新增文件：

#### `src/meta/DriveMetaDao.h`
```cpp
#pragma once

#include <QString>
#include <string>
#include <unordered_map>
#include "../core/ItemRecord.h"

namespace QuarkMeta {

struct DriveMetaRecord {
    std::wstring drivePath; // 格式："C:\"
    int rating = 0;
    std::wstring color;
    bool pinned = false;
    std::wstring note;
    std::wstring url;
};

class DriveMetaDao {
public:
    /**
     * @brief 初始化 global.db 中的 drive_metadata 数据表
     */
    static bool initTable();

    /**
     * @brief 读取所有盘符的元数据记录并返回 Mapping
     */
    static std::unordered_map<std::wstring, DriveMetaRecord> getAllDriveMeta();

    /**
     * @brief 读取单个盘符的元数据记录
     */
    static DriveMetaRecord getDriveMeta(const std::wstring& drivePath);

    /**
     * @brief 写入或更新盘符元数据
     */
    static bool saveDriveMeta(const DriveMetaRecord& record);
};

} // namespace QuarkMeta
```

#### `src/meta/DriveMetaDao.cpp`
```cpp
#include "DriveMetaDao.h"
#include "DatabaseManager.h"
#include "sqlite3.h"
#include <QDateTime>

namespace QuarkMeta {

bool DriveMetaDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    const char* sql =
        "CREATE TABLE IF NOT EXISTS drive_metadata ("
        "  drive_path TEXT PRIMARY KEY,"
        "  rating INTEGER DEFAULT 0,"
        "  color TEXT DEFAULT '',"
        "  pinned INTEGER DEFAULT 0,"
        "  note TEXT DEFAULT '',"
        "  url TEXT DEFAULT '',"
        "  updated_at INTEGER DEFAULT 0"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::unordered_map<std::wstring, DriveMetaRecord> DriveMetaDao::getAllDriveMeta() {
    std::unordered_map<std::wstring, DriveMetaRecord> result;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    const char* sql = "SELECT drive_path, rating, color, pinned, note, url FROM drive_metadata;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            DriveMetaRecord rec;
            const wchar_t* pPath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 0));
            if (pPath) rec.drivePath = pPath;
            rec.rating = sqlite3_column_int(stmt, 1);
            const wchar_t* pColor = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 2));
            if (pColor) rec.color = pColor;
            rec.pinned = (sqlite3_column_int(stmt, 3) != 0);
            const wchar_t* pNote = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 4));
            if (pNote) rec.note = pNote;
            const wchar_t* pUrl = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 5));
            if (pUrl) rec.url = pUrl;

            result[rec.drivePath] = rec;
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

DriveMetaRecord DriveMetaDao::getDriveMeta(const std::wstring& drivePath) {
    auto all = getAllDriveMeta();
    auto it = all.find(drivePath);
    if (it != all.end()) return it->second;
    DriveMetaRecord defaultRec;
    defaultRec.drivePath = drivePath;
    return defaultRec;
}

bool DriveMetaDao::saveDriveMeta(const DriveMetaRecord& record) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    const char* sql =
        "INSERT INTO drive_metadata (drive_path, rating, color, pinned, note, url, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(drive_path) DO UPDATE SET "
        "  rating=excluded.rating, "
        "  color=excluded.color, "
        "  pinned=excluded.pinned, "
        "  note=excluded.note, "
        "  url=excluded.url, "
        "  updated_at=excluded.updated_at;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    sqlite3_bind_text16(stmt, 1, record.drivePath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, record.rating);
    sqlite3_bind_text16(stmt, 3, record.color.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, record.pinned ? 1 : 0);
    sqlite3_bind_text16(stmt, 5, record.note.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 6, record.url.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 7, now);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}

} // namespace QuarkMeta
```

---

### 步骤二：在 `MetaCacheDecorator.cpp` 中添加盘符元数据装饰

在 `src/meta/MetaCacheDecorator.cpp` 中引入 `DriveMetaDao.h`，并在装饰循环中对盘符路径（如 `C:\`）进行识别装饰：

#### 代码修改：
```cpp
#include "MetaCacheDecorator.h"
#include "QuarkMetaJson.h"
#include "DriveMetaDao.h"
#include <QFileInfo>
#include <QDir>
#include <unordered_map>
#include <memory>

namespace QuarkMeta {
void MetaCacheDecorator::decorate(std::vector<ItemRecord>& records) {
    if (records.empty()) return;

    // 预先批量拉取全局盘符元数据
    auto driveMetas = DriveMetaDao::getAllDriveMeta();

    // 按父目录路径建立离散 JSON 缓存池
    std::unordered_map<std::wstring, std::shared_ptr<QuarkMetaJson>> jsonCacheMap;

    for (auto& itemRec : records) {
        if (itemRec.isCategory) continue;

        // 【盘符特殊处理】：如果是驱动器根目录（如 C:\、D:\）
        std::wstring wPath = itemRec.path.toStdWString();
        QFileInfo info(itemRec.path);
        if (info.isRoot() || itemRec.path.endsWith(":\\") || itemRec.path.endsWith(":/")) {
            auto driveIt = driveMetas.find(wPath);
            if (driveIt != driveMetas.end()) {
                itemRec.rating = driveIt->second.rating;
                itemRec.manualColor = QString::fromStdWString(driveIt->second.color);
                itemRec.pinned = driveIt->second.pinned;
                itemRec.note = QString::fromStdWString(driveIt->second.note);
                itemRec.url = QString::fromStdWString(driveIt->second.url);
            }
            continue;
        }

        std::wstring dirPath = info.absolutePath().toStdWString();
        // ... (原离散 JSON 装饰逻辑保持不变)
    }
}
}
```

---

### 步骤三：修改 `MetadataManager.cpp` 写重定向

在 `src/meta/MetadataManager.cpp` 的 `setRating` / `setColor` / `setNote` / `setURL` 等函数顶部，添加盘符判断：如果目标路径属于盘符根目录（`info.isRoot()`），则调用 `DriveMetaDao::saveDriveMeta` 写入 `global.db`：

#### 示例代码片段：
```cpp
void MetadataManager::setRating(const std::wstring& path, int rating) {
    QFileInfo info(QString::fromStdWString(path));
    if (info.isRoot()) {
        auto rec = DriveMetaDao::getDriveMeta(path);
        rec.rating = rating;
        DriveMetaDao::saveDriveMeta(rec);
        emit metaChanged(QString::fromStdWString(path));
        return;
    }
    // ... 原离散 JSON 写入代码
}
```

---

### 步骤四：修改 `ContentPanel.cpp` 中的 “此电脑” (`computer://`) 加载函数

定位到 `src/ui/ContentPanel.cpp`（约第 2555 行）：

#### 修改前代码：
```cpp
    if (path.isEmpty() || path == "computer://") {
        m_currentPath = "computer://";
        updateLayersButtonState();

        const auto drives = QDir::drives();
        std::vector<ItemRecord> driveRecords;
        for (const QFileInfo& drive : drives) {
            driveRecords.push_back(ItemRecord::create(drive.absolutePath()));
        }
        m_model->setRecords(driveRecords);
        m_proxyModel->sort(0, Qt::AscendingOrder);
        m_isLoading = false;
        recalculateAndEmitStats();
        return;
    }
```

#### 修改后代码：
```cpp
    if (path.isEmpty() || path == "computer://") {
        m_currentPath = "computer://";
        updateLayersButtonState();

        const auto drives = QDir::drives();
        std::vector<ItemRecord> driveRecords;
        for (const QFileInfo& drive : drives) {
            driveRecords.push_back(ItemRecord::create(drive.absolutePath()));
        }
        // 【核心点火】：调用 MetaCacheDecorator 从 global.db 注入盘符的置顶/颜色/星级/备注
        MetaCacheDecorator::decorate(driveRecords);

        m_model->setRecords(driveRecords);
        // 自动触发排序，确保置顶的硬盘（pinned = true）排在最前面
        m_proxyModel->sort(0, Qt::AscendingOrder);
        m_isLoading = false;
        recalculateAndEmitStats();
        return;
    }
```

---

### 步骤五：更新 `CMakeLists.txt`

在 `CMakeLists.txt` 的 `SOURCES` 列表中加入：
```cmake
src/meta/DriveMetaDao.h
src/meta/DriveMetaDao.cpp
```

---

## 5. 预期成果与验证方法

1. **“此电脑”下硬盘操作验真**：
   打开 QuarkMeta 并点击左侧导航栏的“此电脑”，对 `C:\` 或 `D:\` 硬盘图标右键/选择后，在右侧元数据面板打 5 星、标记红色、添加备注或选择置顶。
2. **数据落盘验证**：
   在 `.QuarkMeta/global.db` 中查询 `drive_metadata` 表，确认盘符路径与对应的元数据记入成功。
3. **恢复与持久化验证**：
   重启软件或在软件内重新导航至“此电脑”，盘符图标上的**置顶位置、星级评级、颜色高亮**完美恢复显示。
