# SmartFolderFilterUnification-1.md - 编译错误修正与智能文件夹统一实施方案

---

## 概述与编译错误归因

在初版方案执行过程中，出现了以下编译报错：
1. `SmartFolderDao.cpp`: `C1083 无法打开包括文件: "QSqlQuery"`：
   - **归因**：QuarkMeta 数据库访问层统一使用原生 `sqlite3_stmt*` 与 `DatabaseManager::instance().getGlobalDb()` 访问全局 `global.db`，初版误引用了 Qt Sql 模块的 `QSqlQuery`。已全面重构为原生 `sqlite3` API。
2. `LibraryPanel.cpp`: `C2039/C2065 "libraryDataChanged" 不是 QuarkMeta::LibrarySyncService 的成员`：
   - **归因**：`LibrarySyncService` 是内部同步服务，库全局变更信号 SSOT 为 `LibraryService::instance().libraryChanged`。现已修正连接至 `LibraryService::libraryChanged` 信号，并将静态局部连接标志提升为成员变量 `m_connectedSync`，防止多实例信号缺失。

---

## 修改文件总结

- `src/meta/SmartFolderDao.cpp`: 彻底使用原生 `sqlite3.h` 实现表初始化、查增删改。
- `src/ui/LibraryPanel.h`: 新增 `bool m_connectedSync = false;` 成员变量。
- `src/ui/LibraryPanel.cpp`: 修正信号连接至 `LibraryService::instance().libraryChanged`。
