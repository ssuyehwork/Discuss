# 实施方案：UI 层原生 SQL 越权剥离与 Repository 收拢重构规范 (UiSqlDecoupling)

## 所属大纲章节
**1.1 全局数据与内存管理**（1.1.9 UI 层原生 SQL 剥离与 MVC 架构收拢规范）

---

## 涉及代码文件
* `src/meta/TrashRepository.h` （新增：专职回收站仓储服务头文件）
* `src/meta/TrashRepository.cpp` （新增：专职回收站仓储服务实现文件）
* `src/ui/CategoryPanel.cpp` （修改：彻底剥离硬写 `sqlite3_prepare_v2` 查询回收站的越权代码，改为调用 `TrashRepository`）
* `src/ui/ContentPanel.cpp` （修改：彻底剥离硬写 `sqlite3_prepare_v2` 查询资产关系的越权代码，改为调用 `CategoryBindingManager` 或 `MetadataManager`）

---

## 功能描述
此前在 `CategoryPanel.cpp`（Line 952-960 & 1023-1031）和 `ContentPanel.cpp`（Line 2226-2239）中，UI 视图层直接循环遍历 `DatabaseManager::getActiveMemoryDbs()`，手写 `sqlite3_prepare_v2`、`sqlite3_step` 等底层 SQLite 驱动 API，严重违背了 MVC 模式与单一职责原则（SRP）。
本方案执行彻底重构与 MVC 归位：
1. **创建 `TrashRepository` 仓储类**：将 UI 中散落的回收站状态查询（`SELECT 1 FROM trash LIMIT 1`）收拢至专职的仓储服务类中；
2. **剥离 UI 层 SQL 依赖**：彻底擦除 `CategoryPanel.cpp` 与 `ContentPanel.cpp` 中所有的 `sqlite3_*` 原生数据库 API 调用；
3. **收拢资产关系查询**：`ContentPanel.cpp` 中的资产元数据及关系查询统一通过 `MetadataManager` 或 `CategoryBindingManager` 的高层 API 进行，确立单事实源（SSOT）。

---

## 技术决策
1. **彻底解耦 UI 与 DB**：UI 面板层（`src/ui/`）只允许处理信号槽、用户交互与 Qt 视图模型渲染，绝对禁止出现 `#include <sqlite3.h>` 以及任何原生 SQL 语句拼接。
2. **仓储服务收拢（Repository Pattern）**：所有数据库读写均通过 `meta/` 目录下的 Repository/Manager 封装提供强类型 API，底层连接与锁管理对 UI 彻底透明。
3. **C++ 单例规范**：`TrashRepository` 采用符合规范的 C++11 单例模式，构造函数声明为 `explicit TrashRepository(QObject* parent = nullptr);`。

---

## 强制性五项断层排查清单

1. **头文件核对**：
   * `src/meta/TrashRepository.h` 必须包含 `<QObject>`, `<sqlite3.h>`。
   * `CategoryPanel.cpp` 包含 `"../meta/TrashRepository.h"`，清理不用或直接引入的数据库底层头文件。
2. **成员核对**：
   * `TrashRepository` 提供 `bool hasTrashItems() const` 强类型查询接口。
3. **残留核对**：
   * 搜索 `src/ui/` 目录下所有文件中的 `sqlite3_prepare_v2` 与 `sqlite3_step`，确保 UI 层零原生 SQL 调用。
4. **断层核对（上下文连续性）**：
   * 检查 `CategoryPanel::updateTrashCategoryState()`，将原本 20 行的循环查询替换为单行 `bool hasTrash = TrashRepository::instance().hasTrashItems();`。
5. **C++ 语法与特殊成员函数合规排查**：
   * `TrashRepository` 在 `.h` 中声明 `explicit TrashRepository(QObject* parent = nullptr);`，在 `.cpp` 中编写实现，严禁带有形参的 `= default`。

---

## 核心代码实现与改动对照

### 新增文件：`src/meta/TrashRepository.h`
```cpp
#pragma once

#include <QObject>

namespace ArcMeta {

class TrashRepository : public QObject {
    Q_OBJECT
public:
    static TrashRepository& instance();

    // 检查全库/分库中是否存在回收站资产
    bool hasTrashItems() const;

private:
    explicit TrashRepository(QObject* parent = nullptr);
};

} // namespace ArcMeta
```

### 新增文件：`src/meta/TrashRepository.cpp`
```cpp
#include "TrashRepository.h"
#include "DatabaseManager.h"

namespace ArcMeta {

TrashRepository& TrashRepository::instance() {
    static TrashRepository inst;
    return inst;
}

TrashRepository::TrashRepository(QObject* parent)
    : QObject(parent) {
}

bool TrashRepository::hasTrashItems() const {
    auto dbs = DatabaseManager::instance().getActiveMemoryDbs();
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

} // namespace ArcMeta
```

### 修改文件：`src/ui/CategoryPanel.cpp`
```cpp
<<<<<<< SEARCH
    auto dbs = DatabaseManager::instance().getActiveMemoryDbs();
    bool hasDiskTrash = false;
    const char* sql = "SELECT 1 FROM trash_items LIMIT 1";
    for (sqlite3* db : dbs) {
        if (!db) continue;
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                if (sqlite3_column_int(stmt, 0) > 0) hasDiskTrash = true;
            }
            sqlite3_finalize(stmt);
            if (hasDiskTrash) break;
        }
    }
=======
    bool hasDiskTrash = TrashRepository::instance().hasTrashItems();
>>>>>>> REPLACE
```

---

## 已知问题 / 待办
* 无。

---

## 涉及文件清单
1. `src/meta/TrashRepository.h`（新增：专职回收站仓储服务头文件）
2. `src/meta/TrashRepository.cpp`（新增：专职回收站仓储服务实现文件）
3. `src/ui/CategoryPanel.cpp`（修改：彻底剥离硬写 SQL 越权查询逻辑，改为调用 TrashRepository）
4. `src/ui/ContentPanel.cpp`（修改：剥离硬写 SQL 关系查询，收拢至 CategoryBindingManager）
