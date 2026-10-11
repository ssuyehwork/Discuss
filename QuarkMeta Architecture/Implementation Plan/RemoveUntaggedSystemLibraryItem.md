# RemoveUntaggedSystemLibraryItem Implementation Plan

## 1. Overview
本方案物理彻底移除收藏栏“库”选项卡中的系统分类项“未标签”（`sysId == -3`）及其相关的数据库 DAO 查询逻辑。

修改后，“库”选项卡中仅保留“全部数据”与“未分类”两个系统基础分类以及用户自定义分类，界面更加简洁纯粹。

---

## 2. Modified Files List
- `src/ui/LibraryPanel.cpp`
- `src/meta/LibraryDao.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/LibraryPanel.cpp`

```
<<<<<<< SEARCH
    addSystemItem("全部数据", "all_data", "#3498db", -1);
    addSystemItem("未分类", "uncategorized", "#95a5a6", -2);
    addSystemItem("未标签", "untagged", "#7f8c8d", -3);

    // 2. 加载用户自定义分类 (带动态计数与完整树构建)
=======
    addSystemItem("全部数据", "all_data", "#3498db", -1);
    addSystemItem("未分类", "uncategorized", "#95a5a6", -2);

    // 2. 加载用户自定义分类 (带动态计数与完整树构建)
>>>>>>> REPLACE
```

### File 2: `src/meta/LibraryDao.cpp`

```
<<<<<<< SEARCH
    } else if (id == -2) {
        // 未分类：获取关联于 category_id <= 0 的路径，或存在于库中但未归属于任何正数 ID 分类的路径
        const char* sql = "SELECT DISTINCT path FROM library_category_paths WHERE category_id <= 0 "
                          "UNION "
                          "SELECT DISTINCT file_path FROM library_item_index WHERE file_path NOT IN (SELECT path FROM library_category_paths WHERE category_id > 0) "
                          "EXCEPT "
                          "SELECT DISTINCT path FROM library_category_paths WHERE category_id > 0;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (pStr) paths.append(QString::fromUtf8(pStr));
            }
            sqlite3_finalize(stmt);
        }
    } else if (id == -3) {
        // 未标签：获取库中所有无标签（tags 为空或 NULL）的项的路径
        const char* sql = "SELECT DISTINCT file_path FROM library_item_index WHERE tags IS NULL OR tags = '' "
                          "UNION "
                          "SELECT DISTINCT path FROM library_category_paths WHERE path NOT IN (SELECT file_path FROM library_item_index WHERE tags IS NOT NULL AND tags != '');";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (pStr) paths.append(QString::fromUtf8(pStr));
            }
            sqlite3_finalize(stmt);
        }
    } else {
=======
    } else if (id == -2) {
        // 未分类：获取关联于 category_id <= 0 的路径，或存在于库中但未归属于任何正数 ID 分类的路径
        const char* sql = "SELECT DISTINCT path FROM library_category_paths WHERE category_id <= 0 "
                          "UNION "
                          "SELECT DISTINCT file_path FROM library_item_index WHERE file_path NOT IN (SELECT path FROM library_category_paths WHERE category_id > 0) "
                          "EXCEPT "
                          "SELECT DISTINCT path FROM library_category_paths WHERE category_id > 0;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (pStr) paths.append(QString::fromUtf8(pStr));
            }
            sqlite3_finalize(stmt);
        }
    } else {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. **构建与编译**：
   ```bash
   cmake --build build
   ```
2. **界面与数据验证**：
   - 启动应用，切换到侧边栏“库”选项卡；
   - 确认树状视图顶部仅保留“全部数据”与“未分类”两个系统项目，原“未标签”项目已彻底消失。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- [x] **死代码彻底清理**：移除 `-3`（未标签）在 UI 和 DAO 中的遗留实现，彻底杜绝孤立死代码。

---

## 6. Header API Signature Verification
- `LibraryDao::getCategoryPaths(int)` ➔ `QStringList`（接口函数签名保持不变）。

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "LibraryPanel.h"` 与 `#include "LibraryDao.h"` 闭合自洽。
