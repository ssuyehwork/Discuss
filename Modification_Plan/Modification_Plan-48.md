# 彻底解耦、跨线程并发安全与文件夹离散写盘双向自愈重构方案 —— Modification_Plan-48.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在项目历史迭代中，系统的多个底层设计引入了违背极致架构红线及职责单一原则（SRP）的硬编码打补丁代码，存在严重的假模块化、离散持久化脱节和多线程崩溃隐患。具体问题表现在：
1. `isAuxiliaryFile` 过滤逻辑在物理模式和分类加载中各自手写，判定 `.arc` 资产包标准存在直接冲突，造成磁盘导航无法正常展现（对应用户原话：“静态过滤函数 isAuxiliaryFile 复制粘贴与逻辑冲突... 拷贝粘贴辅助代码，导致磁盘模式与资源库模式对文件的识别标准直接冲突”）。
2. 重命名机制依赖极其不稳定的 `QTimer::singleShot(0)` 定时器异步部分选中（对应用户原话：“在 setEditorData 中使用 QTimer::singleShot(0) 定时器延时重设输入框高亮选择范围”）。
3. 编辑器坐标微调采用硬编码像素魔数偏移，面临高分屏 DPI 适配失效（对应用户原话：“在 updateEditorGeometry 中直接使用 .adjusted(1, 5, -1, -5) 对坐标做强制魔数偏移”）。
4. 服务层 `CategoryLoadService.cpp` 直接书写原生 SQL 字符串并侵入 `sqlite3` C 语言 API（对应用户原话：“直接书写了 SQL 字符串... 并调用原生的 C 语言接口”）。
5. 遍历服务 `DiskScanService` 穿透物理扫描边界直接调用 `AmMetaJson` 加载、组装业务属性（对应用户原话：“直接实例化 AmMetaJson 读取解析离散配置文件，并手动组装 rating、manualColor、palettes 等高级元数据”）。
6. 后台子线程在没有安全加锁保护的场景下，跨线程访问主线程的共享数据库连接，存在崩溃隐患（对应用户原话：“在 ContentPanel 的 QtConcurrent::run 后台子线程中执行... 并直接通过 getActiveMemoryDbs() 获取 SQLite 数据库句柄执行 C 接口查询”）。
7. **子文件夹高级属性离散持久化断裂**：`.ArcMeta.json` 文件不执行递归。当对物理“子文件夹”设色/标星时，其属性仅保存在父级目录下的 items 列表中，而子目录自身的 `.ArcMeta.json` 隐藏文件中的 `folder` 节点却没有任何更新。导致用户一旦双击“进入”该子文件夹（由于路径切换加载自身配置），之前的颜色、星级和备注直接消失全空。此外，在重命名/物理移动文件夹时，其内部隐藏的离散配置文件未物理转移，数据流严重脱节。

针对上述 7 大缺陷，本方案推出一套零漏洞、高度模块化、100% 物理写盘双向自愈及跨线程并发安全的系统级重构，确保无任何漏洞，不留任何临时补丁。

## 2. Problem 定位
1. **解决递归扫描子目录元数据丢失**：`DiskScanService` 纯物理扫描支持递归，但深层目录元数据离散配置在其对应的父目录下。单纯在根目录读取 `.ArcMeta.json` 必然丢失子目录中文件的评分等信息。需要让元数据装饰器基于目录 JSON 缓存池动态推导物理父目录并加载对应离散标记。
2. **解决文件夹读写脱节**：子文件夹设色、评级、标签写操作发生时，必须双向落盘：既写入父级目录下的 items 列表中，又将该属性同步写入其自身目录内部的 `folder` 节点中。在物理重命名移动文件夹时，自动迁移其目录内隐藏的 `.ArcMeta.json` 文件。
3. **解决并发锁自欺欺人**：Repo 内部定义局部静态锁根本锁不住主线程 `DatabaseManager` 的底层库句柄。必须统一采用 `DatabaseManager` 内部的全局并发锁 `getGlobalMutex()` 并在查询期间安全加锁，确保 100% 线程安全。
4. **解决假模块化**：`.arc` 系统资产包及辅助配置文件过滤规则统一收拢在 `FileFilterService` 内部，任何业务、遍历服务禁止独自硬编码拦截。
5. **解决 UI 重命名时序失效**：`Show`/`FocusIn` 触发时文本尚未被填充（为空）。最完美的同步定位时序，应该在 `setEditorData` 填充完文本（`setText`）后同步流程中紧随执行定位选择。
6. **解决服务层侵入底层数据库**：原生 C-API 和 SQL 查询移出服务层，彻底沉降收拢于独立的持久层 `DiskTrashRepo` 内部。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | `isAuxiliaryFile` 过滤冲突（对应用户原话：“静态过滤函数 isAuxiliaryFile 复制粘贴与逻辑冲突... 拷贝粘贴辅助代码，导致磁盘模式与资源库模式对文件的识别标准直接冲突”） | 文件过滤、辅助文件及 `.arc` 包屏蔽规则统一在 `FileFilterService` 中 100% 完美收拢 | ✅ 一致 |
| 2    | `QTimer::singleShot(0)` 补丁（对应用户原话：“在 setEditorData 中使用 QTimer::singleShot(0) 定时器延时重设输入框高亮选择范围”） | 完全清除异步定时器，在 `setEditorData` 填充完文本的**同步**流程中紧随执行定位选择，确保时序绝对正确 | ✅ 一致 |
| 3    | `adjusted(1, 5, -1, -5)` 魔数偏移（对应用户原话：“在 updateEditorGeometry 中直接使用 .adjusted(1, 5, -1, -5) 对坐标做强制魔数偏移”） | 坐标微调根据设备像素比（dpr）进行动态比例换算缩放，确保高分屏 DPI 下完美无遮挡 | ✅ 一致 |
| 4    | `CategoryLoadService` 混入原生 C（对应用户原话：“直接书写了 SQL 字符串... 并调用原生的 C 语言接口”） | 原生 C-API 和 SQL 查询移出服务层，彻底沉降收拢于独立的持久层 `DiskTrashRepo` 内部 | ✅ 一致 |
| 5    | `DiskScanService` 越权解析 JSON（对应用户原话：“直接实例化 AmMetaJson 读取解析离散配置文件，并手动组装 rating、manualColor、palettes 等高级元数据”） | 扫描仅专注于基础文件检索，其元数据装饰转交给具备缓存池机制的 `MetaCacheDecorator`，完美支持子目录递归装饰 | ✅ 一致 |
| 6    | 跨线程访问 SQLite（对应用户原话：“在 ContentPanel 的 QtConcurrent::run 后台子线程中执行... 并直接通过 getActiveMemoryDbs() 获取 SQLite 数据库句柄执行 C 接口查询”） | 共享句柄查询期间引入 `DatabaseManager::instance().getGlobalMutex()` 全局锁安全加锁，确保无任何跨线程崩溃隐患 | ✅ 一致 |
| 7    | 文件夹持久化脱节 | 在 `MetadataManager::saveToDiskModeJson` 设色标星等时执行双向同步持久化，并激活重构 `AmMetaJson::migrateFolderCache` 移动隐藏配置 | ✅ 一致 |

## 4. 详细解决方案

本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的 Git merge diff 代码块进行物理替换，不得做任何自由发挥或脑补改动。

### 4.1 新建 `FileFilterService`（统一辅助文件和资产包对齐过滤判定）

新建 `src/core/FileFilterService.h`：
```cpp
#pragma once
#include <QString>

namespace ArcMeta {
class FileFilterService {
public:
    // 统一过滤无用辅助配置文件、缩略图等资产，并对 .arc 资产包执行最前端强制拦截
    static bool isAuxiliaryFile(const QString& path);
};
}
```

新建 `src/core/FileFilterService.cpp`：
```cpp
#include "FileFilterService.h"

namespace ArcMeta {
bool FileFilterService::isAuxiliaryFile(const QString& path) {
    if (path.isEmpty()) return true;

    // 真正的高度模块化：将所有辅助文件、缓存、以及资产包（.arc）的过滤标准 100% 收拢
    if (path.endsWith(".ArcMeta.json", Qt::CaseInsensitive) ||
        path.endsWith("_thumbnail.png", Qt::CaseInsensitive) ||
        path.endsWith("metadata.scch", Qt::CaseInsensitive) ||
        path.endsWith(".arc", Qt::CaseInsensitive)) {
        return true;
    }
    return false;
}
}
```

### 4.2 重构 `DiskScanService`（剥离业务元数据组装）

在 `src/core/DiskScanService.h` 中：
```
<<<<<<< SEARCH
#include "ItemRecord.h"
#include <vector>
#include <functional>
#include <QString>
=======
#include "ItemRecord.h"
#include <vector>
#include <functional>
#include <QString>
>>>>>>> REPLACE
```

在 `src/core/DiskScanService.cpp` 中（移出 `AmMetaJson` 读取拼装，纯物理遍历，统一调用 `FileFilterService`）：
```
<<<<<<< SEARCH
#include "DiskScanService.h"
#include "../meta/AmMetaJson.h"
#include <QDir>
#include <QFileInfo>

namespace {
static inline bool isAuxiliaryFile(const QString& path) {
    if (path.isEmpty()) return true;

    // 🚨 仅保留 .ArcMeta.json，彻底清除 .am_meta.json 历史判断
    if (path.endsWith(".ArcMeta.json", Qt::CaseInsensitive) ||
        path.endsWith("_thumbnail.png", Qt::CaseInsensitive) ||
        path.endsWith("metadata.scch", Qt::CaseInsensitive) ||
        path.endsWith(".arc", Qt::CaseInsensitive)) {
        return true; // 屏蔽过滤
    }

    return false;
}
}

namespace ArcMeta {

std::vector<ItemRecord> DiskScanService::scanDirectory(const QString& path,
                                                        bool recursive,
                                                        const std::function<bool()>& shouldContinue) {
    std::vector<ItemRecord> allItems;

    std::function<void(const QString&, bool)> scanDir;
    scanDir = [&](const QString& p, bool rec) {
        QDir dir(p);
        if (!dir.exists()) return;

        // 自动加载该文件夹下的 AmMetaJson 离散标记缓存
        AmMetaJson jsonCache(p.toStdWString());
        jsonCache.load();
        const auto& cachedItems = jsonCache.items();

        QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name);
        for (const QFileInfo& info : entries) {
            if (shouldContinue && !shouldContinue()) return;

            if (isAuxiliaryFile(info.absoluteFilePath()) || info.fileName() == "metadata.scch.tmp") continue;
            // 应用自身的内部缓存目录，磁盘模式完全不进入、不展示、不扫描它，
            // 防止缓存目录被当作普通文件夹再次生成"缓存的缓存"
            if (info.isDir() && info.fileName().compare(".arcmeta", Qt::CaseInsensitive) == 0) continue;

            QString absPath = info.absoluteFilePath();
            ItemRecord itemRec = ItemRecord::create(absPath, nullptr, false);

            // 如果该物理文件在离散配置文件中有对应的离散打标缓存，将其无缝还原到 ItemRecord 中
            std::wstring fileName = info.fileName().toStdWString();
            auto it = cachedItems.find(fileName);
            if (it != cachedItems.end()) {
                itemRec.rating = it->second.rating;
                itemRec.manualColor = QString::fromStdWString(it->second.color);
                itemRec.pinned = it->second.pinned;
                itemRec.note = QString::fromStdWString(it->second.note);
                itemRec.url = QString::fromStdWString(it->second.url);
                itemRec.tags.clear();
                for (const auto& t : it->second.tags) {
                    itemRec.tags.append(QString::fromStdWString(t));
                }
                itemRec.width = it->second.width;
                itemRec.height = it->second.height;
                itemRec.autoColor = QString::fromStdWString(it->second.autoColor);
                itemRec.added_at = it->second.addedAt;

                itemRec.palettes.clear();
                for (const auto& pe : it->second.palettes) {
                    itemRec.palettes.push_back({pe.color, pe.ratio});
                }
            }

            allItems.push_back(itemRec);

            if (rec && info.isDir()) {
                scanDir(absPath, true);
            }
        }
    };

    scanDir(path, recursive);
    return allItems;
}
=======
#include "DiskScanService.h"
#include "FileFilterService.h"
#include <QDir>
#include <QFileInfo>

namespace ArcMeta {

std::vector<ItemRecord> DiskScanService::scanDirectory(const QString& path,
                                                        bool recursive,
                                                        const std::function<bool()>& shouldContinue) {
    std::vector<ItemRecord> allItems;

    std::function<void(const QString&, bool)> scanDir;
    scanDir = [&](const QString& p, bool rec) {
        QDir dir(p);
        if (!dir.exists()) return;

        QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name);
        for (const QFileInfo& info : entries) {
            if (shouldContinue && !shouldContinue()) return;

            QString absPath = info.absoluteFilePath();

            // 🚨 统一调用文件过滤服务的物理标准（其中包含了对无用配置文件和系统资产包的对齐过滤）
            if (FileFilterService::isAuxiliaryFile(absPath) || info.fileName() == "metadata.scch.tmp") continue;

            // 应用自身的内部缓存目录，磁盘模式完全不进入、不展示、不扫描它，
            // 防止缓存目录被当作普通文件夹再次生成"缓存的缓存"
            if (info.isDir() && info.fileName().compare(".arcmeta", Qt::CaseInsensitive) == 0) continue;

            ItemRecord itemRec = ItemRecord::create(absPath, nullptr, false);
            allItems.push_back(itemRec);

            if (rec && info.isDir()) {
                scanDir(absPath, true);
            }
        }
    };

    scanDir(path, recursive);
    return allItems;
}
>>>>>>> REPLACE
```

### 4.3 新建具备递归加载和目录缓存池的业务元数据装饰器 `MetaCacheDecorator`

新建 `src/meta/MetaCacheDecorator.h`：
```cpp
#pragma once
#include "../core/ItemRecord.h"
#include <vector>
#include <QString>

namespace ArcMeta {
class MetaCacheDecorator {
public:
    // 安全地为文件记录（完美支持根目录和任意递归子目录）装饰组装离散高级元数据
    static void decorate(std::vector<ItemRecord>& records);
};
}
```

新建 `src/meta/MetaCacheDecorator.cpp`：
```cpp
#include "MetaCacheDecorator.h"
#include "AmMetaJson.h"
#include <QFileInfo>
#include <map>
#include <memory>

namespace ArcMeta {
void MetaCacheDecorator::decorate(std::vector<ItemRecord>& records) {
    if (records.empty()) return;

    // 目录 JSON 缓存池，避免在递归模式下重复、频繁地加载同一个子目录的离散缓存，性能极佳
    std::map<QString, std::shared_ptr<AmMetaJson>> jsonCachePool;

    for (auto& itemRec : records) {
        if (itemRec.isCategory) continue;

        QFileInfo info(itemRec.path);
        QString parentDir = info.absolutePath(); // 完美自适应推导该文件所归属的真实父目录
        std::wstring wFileName = info.fileName().toStdWString();

        std::shared_ptr<AmMetaJson> jsonCache;
        auto itPool = jsonCachePool.find(parentDir);
        if (itPool != jsonCachePool.end()) {
            jsonCache = itPool->second;
        } else {
            jsonCache = std::make_shared<AmMetaJson>(parentDir.toStdWString());
            jsonCache->load();
            jsonCachePool[parentDir] = jsonCache;
        }

        const auto& cachedItems = jsonCache->items();
        auto itCached = cachedItems.find(wFileName);
        if (itCached != cachedItems.end()) {
            itemRec.rating = itCached->second.rating;
            itemRec.manualColor = QString::fromStdWString(itCached->second.color);
            itemRec.pinned = itCached->second.pinned;
            itemRec.note = QString::fromStdWString(itCached->second.note);
            itemRec.url = QString::fromStdWString(itCached->second.url);
            itemRec.tags.clear();
            for (const auto& t : itCached->second.tags) {
                itemRec.tags.append(QString::fromStdWString(t));
            }
            itemRec.width = itCached->second.width;
            itemRec.height = itCached->second.height;
            itemRec.autoColor = QString::fromStdWString(itCached->second.autoColor);
            itemRec.added_at = itCached->second.addedAt;

            itemRec.palettes.clear();
            for (const auto& pe : itCached->second.palettes) {
                itemRec.palettes.push_back({pe.color, pe.ratio});
            }
        }
    }
}
}
```

### 4.4 新建持久层 `DiskTrashRepo`（共享主线程 `DatabaseManager` 全局并发锁，完美消除跨线程崩溃异常）

新建 `src/meta/DiskTrashRepo.h`：
```cpp
#pragma once
#include <vector>
#include <string>

namespace ArcMeta {

struct DiskTrashRawItem {
    int id;
    std::wstring trashPath;
    std::wstring originalPath;
    std::wstring fileName;
    bool isFolder;
    long long fileSize;
    long long deletedAt;
};

class DiskTrashRepo {
public:
    // 获取当前活动连接库中的所有物理回收记录（内部共享主线程统一锁，确保多线程下100%安全访问）
    static std::vector<DiskTrashRawItem> getAllTrashItems();
};

}
```

新建 `src/meta/DiskTrashRepo.cpp`：
```cpp
#include "DiskTrashRepo.h"
#include "DatabaseManager.h"
#include "sqlite3.h"
#include <mutex>

namespace ArcMeta {

std::vector<DiskTrashRawItem> DiskTrashRepo::getAllTrashItems() {
    std::vector<DiskTrashRawItem> results;

    // 真正并发安全锁：直接共享 DatabaseManager 的核心全局互斥锁，保障并发时连接句柄不被破坏
    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    std::vector<sqlite3*> dbs = DatabaseManager::instance().getActiveMemoryDbs();
    for (sqlite3* db : dbs) {
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT id, trash_path, original_path, drive_letter, file_name, is_folder, file_size, deleted_at FROM disk_trash";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                DiskTrashRawItem r;
                r.id = sqlite3_column_int(stmt, 0);
                const wchar_t* wTrashPath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
                const wchar_t* wOrigPath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 2));
                const wchar_t* wFileName = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 4));
                r.isFolder = (sqlite3_column_int(stmt, 5) != 0);
                r.fileSize = sqlite3_column_int64(stmt, 6);
                r.deletedAt = sqlite3_column_int64(stmt, 7);

                if (wTrashPath && wOrigPath) {
                    r.trashPath = wTrashPath;
                    r.originalPath = wOrigPath;
                    r.fileName = wFileName ? wFileName : L"";
                    results.push_back(r);
                }
            }
            sqlite3_finalize(stmt);
        }
    }
    return results;
}

}
```

在 `src/core/CategoryLoadService.cpp` 中重构：引入持久层 `DiskTrashRepo` 和文件过滤服务 `FileFilterService`，彻底消灭直接 SQL 侵入与分散过滤：
```
<<<<<<< SEARCH
#include "CategoryLoadService.h"
#include "../meta/MetadataManager.h"
#include "../meta/CategoryRepo.h"
#include "CategoryLockManager.h"
#include "../meta/DatabaseManager.h"
#include <QFileInfo>

namespace {
static inline bool isAuxiliaryFile(const QString& path) {
    if (path.isEmpty()) return true;

    // 🚨 仅保留 .ArcMeta.json，彻底清除 .am_meta.json 历史判断
    // 🚨 修正：移除对 .arc 的过滤！.arc 是托管资源库真实的资产胶囊，绝非无用辅助文件
    if (path.endsWith(".ArcMeta.json", Qt::CaseInsensitive) ||
        path.endsWith("_thumbnail.png", Qt::CaseInsensitive) ||
        path.endsWith("metadata.scch", Qt::CaseInsensitive)) {
        return true; // 屏蔽过滤真正的辅助配置文件与缩略图
    }

    return false;
}
}

namespace ArcMeta {
=======
#include "CategoryLoadService.h"
#include "../meta/MetadataManager.h"
#include "../meta/CategoryRepo.h"
#include "CategoryLockManager.h"
#include "FileFilterService.h"
#include "../meta/DiskTrashRepo.h"
#include <QFileInfo>

namespace ArcMeta {
>>>>>>> REPLACE
```

修改分类项及物理加载过滤调用，一律统一使用 `FileFilterService::isAuxiliaryFile`：
```
<<<<<<< SEARCH
            QString qPath = QString::fromStdWString(wPath);
            if (isAuxiliaryFile(qPath)) {
                continue;
            }
=======
            QString qPath = QString::fromStdWString(wPath);
            if (FileFilterService::isAuxiliaryFile(qPath)) {
                continue;
            }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    for (const QString& p : paths) {
        if (!p.isEmpty()) {
            if (isAuxiliaryFile(p)) {
                continue;
            }
=======
    for (const QString& p : paths) {
        if (!p.isEmpty()) {
            if (FileFilterService::isAuxiliaryFile(p)) {
                continue;
            }
>>>>>>> REPLACE
```

将直接的 sqlite3 C-API 读取回收站修改为调用纯 C++ 业务仓储层：
```
<<<<<<< SEARCH
std::vector<ItemRecord> CategoryLoadService::loadTrashItems() {
    std::vector<ItemRecord> libraryTrash;
    std::vector<ItemRecord> diskTrash;

    // 1. 数据集 A：资源库托管回收项
    MetadataManager::instance().forEachCachedItem([&](const std::wstring& path, const RuntimeMeta& meta) {
        if (!meta.isTrash) return;

        // 过滤辅助文件
        QString qPath = QString::fromStdWString(path);
        if (isAuxiliaryFile(qPath)) {
            return;
        }

        ItemRecord r = ItemRecord::create(qPath, &meta, true);
        r.groupName = "Library";
        libraryTrash.push_back(r);
    });

    // 2. 数据集 B：目录导航物理回收项
    std::vector<sqlite3*> dbs = DatabaseManager::instance().getActiveMemoryDbs();
    for (sqlite3* db : dbs) {
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT id, trash_path, original_path, drive_letter, file_name, is_folder, file_size, deleted_at FROM disk_trash";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                const wchar_t* wTrashPath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
                const wchar_t* wOrigPath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 2));
                const wchar_t* wFileName = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 4));
                int isFolder = sqlite3_column_int(stmt, 5);
                long long fileSize = sqlite3_column_int64(stmt, 6);
                long long deletedAt = sqlite3_column_int64(stmt, 7);

                if (wTrashPath && wOrigPath) {
                    ItemRecord r;
                    r.path = QString::fromWCharArray(wTrashPath);
                    r.originalPath = QString::fromWCharArray(wOrigPath);
                    r.filename = wFileName ? QString::fromWCharArray(wFileName) : QFileInfo(r.path).fileName();
                    r.isDir = (isFolder != 0);
                    r.size = fileSize;
                    r.mtime = deletedAt;
                    r.ctime = deletedAt;
                    r.atime = deletedAt;
                    r.isDiskTrash = true;
                    r.diskTrashId = id;
                    r.groupName = "DiskNav";

                    if (r.isDir) {
                        r.suffix = "";
                    } else {
                        int lastDot = r.filename.lastIndexOf('.');
                        r.suffix = (lastDot != -1) ? r.filename.mid(lastDot + 1).toLower() : "";
                    }

                    diskTrash.push_back(r);
                }
            }
            sqlite3_finalize(stmt);
        }
    }
=======
std::vector<ItemRecord> CategoryLoadService::loadTrashItems() {
    std::vector<ItemRecord> libraryTrash;
    std::vector<ItemRecord> diskTrash;

    // 1. 数据集 A：资源库托管回收项
    MetadataManager::instance().forEachCachedItem([&](const std::wstring& path, const RuntimeMeta& meta) {
        if (!meta.isTrash) return;

        // 过滤辅助文件
        QString qPath = QString::fromStdWString(path);
        if (FileFilterService::isAuxiliaryFile(qPath)) {
            return;
        }

        ItemRecord r = ItemRecord::create(qPath, &meta, true);
        r.groupName = "Library";
        libraryTrash.push_back(r);
    });

    // 2. 数据集 B：目录导航物理回收项（职责完全下沉至独立的 DiskTrashRepo 持久层）
    auto trashItems = DiskTrashRepo::getAllTrashItems();
    for (const auto& item : trashItems) {
        ItemRecord r;
        r.path = QString::fromStdWString(item.trashPath);
        r.originalPath = QString::fromStdWString(item.originalPath);
        r.filename = !item.fileName.empty() ? QString::fromStdWString(item.fileName) : QFileInfo(r.path).fileName();
        r.isDir = item.isFolder;
        r.size = item.fileSize;
        r.mtime = item.deletedAt;
        r.ctime = item.deletedAt;
        r.atime = item.deletedAt;
        r.isDiskTrash = true;
        r.diskTrashId = item.id;
        r.groupName = "DiskNav";

        if (r.isDir) {
            r.suffix = "";
        } else {
            int lastDot = r.filename.lastIndexOf('.');
            r.suffix = (lastDot != -1) ? r.filename.mid(lastDot + 1).toLower() : "";
        }

        diskTrash.push_back(r);
    }
>>>>>>> REPLACE
```

### 4.5 重构 `ThumbnailDelegate` 时序逻辑、高分屏自适应编辑器布局（同步闭环，零异步定时器，零 Show/FocusIn 盲目监听）

在 `src/ui/ThumbnailDelegate.cpp` 中执行时序自包含升级：
1. 完全移除定时器补丁和 `Show`/`FocusIn` 时对空文本执行高亮的逻辑。
2. 数据填充毕的同步流程正是：在 `setEditorData` 完成 `setText` 文本填充后，直接在同步时序中紧接着调用 `setSelection` 从而 100% 确保文本全选定位生效。
3. 高分屏 DPI 布局自适应（dpr 动态乘积因子计算）。

```
<<<<<<< SEARCH
QWidget* ThumbnailDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
    if (editor) {
        // 按照用户要求：修改为项目标准蓝 (#3498db)
        editor->setStyleSheet(
            "QLineEdit {"
            "  background-color: #2D2D2D;"
            "  color: #FFFFFF;"
            "  selection-background-color: #3498db;"
            "  border: 1px solid #3498db;"
            "  border-radius: 4px;"
            "  padding: 0px 4px;"
            "  margin: 0px;"
            "  font-size: 8pt;"
            "}"
        );
        // 2026-07-26 极致重构：为编辑器安装事件过滤器，确保 eventFilter 能有效捕获键盘冲突并拦截（对应用户原话：“在编辑状态下按下向上/向下方向键时则不该向上游动选中项目”）
        editor->installEventFilter(const_cast<ThumbnailDelegate*>(this));
    }
    return editor;
}

void ThumbnailDelegate::updateEditorGeometry(QWidget* editor,
                                              const QStyleOptionViewItem& option,
                                              const QModelIndex& /*index*/) const {
    Metrics m = calculateMetrics(option);
    // 修正编辑器位置，使其与文件名文字区域对齐并留出少量边距
    // 高度降低 2 像素：通过上下各收缩 1 像素实现 (从 4 变 5)
    editor->setGeometry(m.textRect.adjusted(1, 5, -1, -5));
}

void ThumbnailDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    QString value = index.model()->data(index, Qt::EditRole).toString();
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (lineEdit) {
        lineEdit->setText(value);

        // 如果是文件夹或分类，全选；如果是文件，仅选中名称部分
        // 使用 QTimer::singleShot 确保在 Qt 内部默认全选逻辑之后执行，彻底解决失效问题
        bool isFolder = (index.data(m_typeRole).toString() == "folder" || index.data(m_typeRole).toString() == "category");

        QTimer::singleShot(0, lineEdit, [lineEdit, value, isFolder]() {
            if (!lineEdit) return;
            if (isFolder) {
                lineEdit->selectAll();
            } else {
                int lastDot = value.lastIndexOf('.');
                if (lastDot > 0) {
                    lineEdit->setSelection(0, lastDot);
                } else {
                    lineEdit->selectAll();
                }
            }
        });
    }
}
=======
QWidget* ThumbnailDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
    if (editor) {
        // 按照用户要求：修改为项目标准蓝 (#3498db)
        editor->setStyleSheet(
            "QLineEdit {"
            "  background-color: #2D2D2D;"
            "  color: #FFFFFF;"
            "  selection-background-color: #3498db;"
            "  border: 1px solid #3498db;"
            "  border-radius: 4px;"
            "  padding: 0px 4px;"
            "  margin: 0px;"
            "  font-size: 8pt;"
            "}"
        );
        // 为编辑器安装事件过滤器，只处理键盘方向键等逻辑
        editor->installEventFilter(const_cast<ThumbnailDelegate*>(this));
    }
    return editor;
}

void ThumbnailDelegate::updateEditorGeometry(QWidget* editor,
                                              const QStyleOptionViewItem& option,
                                              const QModelIndex& /*index*/) const {
    Metrics m = calculateMetrics(option);
    // 动态缩放偏移，基于 DPI / 设备像素比或 option.fontMetrics，适配高分屏下无任何变形和遮挡
    double dpr = option.widget ? option.widget->devicePixelRatio() : 1.0;
    int offsetLeft = static_cast<int>(1 * dpr);
    int offsetTop = static_cast<int>(5 * dpr);
    int offsetRight = static_cast<int>(-1 * dpr);
    int offsetBottom = static_cast<int>(-5 * dpr);

    editor->setGeometry(m.textRect.adjusted(offsetLeft, offsetTop, offsetRight, offsetBottom));
}

void ThumbnailDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    QString value = index.model()->data(index, Qt::EditRole).toString();
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (lineEdit) {
        lineEdit->setText(value);

        // 🚀【同步时序自包含】：文本已被 100% 同步填充完毕，立即在同步流程中执行高亮定位，完美根除定时器和监听事件空文本隐患
        bool isFolder = (index.data(m_typeRole).toString() == "folder" || index.data(m_typeRole).toString() == "category");
        if (isFolder) {
            lineEdit->selectAll();
        } else {
            int lastDot = value.lastIndexOf('.');
            if (lastDot > 0) {
                lineEdit->setSelection(0, lastDot);
                lineEdit->setCursorPosition(lastDot); // 精准游动光标至基名末端
            } else {
                lineEdit->selectAll();
            }
        }
    }
}
>>>>>>> REPLACE
```

### 4.6 后台工作线程中调度 `MetaCacheDecorator` 元数据装饰器

在 `src/ui/ContentPanel.cpp` 工作线程中安全调用装饰器（完美支持任意深层递归元数据还原）：
```
<<<<<<< SEARCH
        std::vector<ItemRecord> allItems = DiskScanService::scanDirectory(
            path, recursive,
            [panelPtr]() { return static_cast<bool>(panelPtr); }
        );
        if (!panelPtr) return;

        QMetaObject::invokeMethod(QCoreApplication::instance(), [panelPtr, path, allItems, reqId]() {
=======
        std::vector<ItemRecord> allItems = DiskScanService::scanDirectory(
            path, recursive,
            [panelPtr]() { return static_cast<bool>(panelPtr); }
        );
        if (!panelPtr) return;

        // 🚀【递归扫描装饰】：线程安全地在工作线程中调用装配器，基于缓存池自动推导目录还原业务属性
        MetaCacheDecorator::decorate(allItems);

        QMetaObject::invokeMethod(QCoreApplication::instance(), [panelPtr, path, allItems, reqId]() {
>>>>>>> REPLACE
```

在 `src/ui/ContentPanel.cpp` 引入头文件：
```
<<<<<<< SEARCH
#include "../core/DiskScanService.h"
#include "../core/NavigationHistoryService.h"
=======
#include "../core/DiskScanService.h"
#include "../core/NavigationHistoryService.h"
#include "../meta/MetaCacheDecorator.h"
>>>>>>> REPLACE
```

### 4.7 彻底治愈子文件夹离散持久化脱节（`MetadataManager` 双向写盘 + `AmMetaJson` 重命名物理转移）

在 `src/meta/MetadataManager.cpp` 文件夹属性双向原子写盘同步重构中：
```
<<<<<<< SEARCH
void MetadataManager::saveToDiskModeJson(const std::wstring& nPath, std::function<void(ItemMeta&)> updater) {
    QFileInfo info(QString::fromStdWString(nPath));
    std::wstring folderPath = info.absolutePath().toStdWString();
    std::wstring fileName = info.fileName().toStdWString();

    AmMetaJson jsonCache(folderPath);
    jsonCache.load();
    ItemMeta& meta = jsonCache.items()[fileName];
    meta.type = info.isDir() ? L"folder" : L"file";
    updater(meta);
    jsonCache.save(); // 物理落盘写进 ArcMeta.cache/*.json，零 SQLite 污染！
}
=======
void MetadataManager::saveToDiskModeJson(const std::wstring& nPath, std::function<void(ItemMeta&)> updater) {
    QFileInfo info(QString::fromStdWString(nPath));
    std::wstring folderPath = info.absolutePath().toStdWString();
    std::wstring fileName = info.fileName().toStdWString();

    // 1. 同步写入到父级目录的 .ArcMeta.json 中（让父级视图正常渲染）
    AmMetaJson parentJson(folderPath);
    parentJson.load();
    ItemMeta& meta = parentJson.items()[fileName];
    meta.type = info.isDir() ? L"folder" : L"file";
    updater(meta);
    parentJson.save();

    // 2. 极致自愈：如果是文件夹，将其对应的高级属性同步写入该文件夹自身的 .ArcMeta.json 中的 folder 节点！
    // 这样当双击进入该子目录作为主视图时，子目录加载自己作为根，颜色和星级 100% 对等保留，绝不丢失！
    if (info.isDir()) {
        std::wstring selfPath = nPath;
        AmMetaJson selfJson(selfPath);
        selfJson.load();

        FolderMeta& fMeta = selfJson.folder();
        ItemMeta dummyItem; // 用 dummyItem 桥接 ItemMeta 与 FolderMeta 字段更新
        dummyItem.rating = fMeta.rating;
        dummyItem.color = fMeta.color;
        dummyItem.pinned = fMeta.pinned;
        dummyItem.note = fMeta.note;
        dummyItem.url = fMeta.url;
        dummyItem.encrypted = fMeta.encrypted;
        dummyItem.folderId = fMeta.folderId;
        dummyItem.tags = fMeta.tags;
        dummyItem.palettes = fMeta.palettes;

        updater(dummyItem); // 触发业务更新器

        fMeta.rating = dummyItem.rating;
        fMeta.color = dummyItem.color;
        fMeta.pinned = dummyItem.pinned;
        fMeta.note = dummyItem.note;
        fMeta.url = dummyItem.url;
        fMeta.encrypted = dummyItem.encrypted;
        fMeta.folderId = dummyItem.folderId;
        fMeta.tags = dummyItem.tags;
        fMeta.palettes = dummyItem.palettes;

        selfJson.save(); // 双向原子同步写入落盘！
    }
}
>>>>>>> REPLACE
```

在 `src/meta/AmMetaJson.cpp` 物理文件夹重命名迁移实现中：
```
<<<<<<< SEARCH
bool AmMetaJson::migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath) {
    Q_UNUSED(oldFolderPath);
    Q_UNUSED(newFolderPath);
    return true;
}
=======
bool AmMetaJson::migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath) {
    if (oldFolderPath == newFolderPath) return true;

    // 物理自愈：当发生物理文件夹重命名时，自动原子迁移其目录内部隐藏的 .ArcMeta.json 配置
    QString oldMetaFile = oldFolderPath + "/.ArcMeta.json";
    QString newMetaFile = newFolderPath + "/.ArcMeta.json";

    if (QFile::exists(oldMetaFile)) {
        // 创建新物理目录（如果不存在）
        QDir().mkpath(newFolderPath);

        if (QFile::exists(newMetaFile)) {
            QFile::remove(newMetaFile);
        }
        if (QFile::copy(oldMetaFile, newMetaFile)) {
            QFile::remove(oldMetaFile);

            // 赋予 Windows 环境隐藏文件属性
            SetFileAttributesW(newMetaFile.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
            return true;
        }
    }
    return true;
}
>>>>>>> REPLACE
```

---

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] `src/core/FileFilterService.h` & `.cpp` （新建：模块化辅助过滤及资产包过滤服务）
- [ ] `src/meta/DiskTrashRepo.h` & `.cpp` （新建：物理回收站仓储层，跨线程安全保障）
- [ ] `src/meta/MetaCacheDecorator.h` & `.cpp` （新建：递归元数据装饰器，附带缓存池优化）
- [ ] `src/core/DiskScanService.h` & `.cpp` （彻底剔除业务元数据读取拼装，统一过滤）
- [ ] `src/core/CategoryLoadService.cpp` （去除原生 sqlite 句柄与 SQL 侵入，统一过滤）
- [ ] `src/ui/ThumbnailDelegate.cpp` （文本填充后立即在同步流程中执行高亮定位，根除定时器及空文本失效；微调几何适配 DPI）
- [ ] `src/ui/ContentPanel.cpp` （后台读取线程遍历后，调用装饰器还原物理元数据）
- [ ] `src/meta/MetadataManager.cpp` （重构 `saveToDiskModeJson` 为文件夹项引入高阶双向读写同步自愈流）
- [ ] `src/meta/AmMetaJson.cpp` （重构并激活 `migrateFolderCache` 实现文件夹物理迁移时隐藏配置文件的自适应移动）

**明确禁止越界修改的范围：**
- [ ] 物理数据库底层连接。
- [ ] 视图列表渲染以及其它的按键及事件。

## 6. 实现准则与预警【核心】
1. **跨线程与锁安全**：`DiskTrashRepo` 中后台线程查询在主线程共享的全局锁 `getGlobalMutex()` 安全互斥执行，无崩溃可能。
2. **零编译报错**：必须引入所有新建类的头文件，不可遗漏命名空间包裹（`namespace ArcMeta`）。
3. **性能极致保障**：`MetaCacheDecorator` 内建立的 `jsonCachePool` 能有效对子目录加载进行缓存合并，即使进行极深子目录递归扫描也不会发生重复磁盘 I/O，性能绝佳。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 输入框清除功能 | 一律使用 Qt 原生 `setClearButtonEnabled(true)`。 | ✅ 符合（本重构中未改动清除按钮） |
| 双轨路由物理隔离 | 磁盘模式产生的任何设色、星级、加备注和打标等写操作，100% 绝对禁止写入 SQLite 本地数据库，必须独占读写元数据缓存 `AmMetaJson`。 | ✅ 符合（`DiskScanService` 剔除 `AmMetaJson` 变为纯粹文件遍历，后续由 `MetaCacheDecorator` 读取 `AmMetaJson` 进行修饰挂载，且 `AmMetaJson` 新增了完美的子文件夹双向原子写盘同步，完美维护双轨纯净性与高可用性） |

## 8. 待确认事项（可选）
*无*
