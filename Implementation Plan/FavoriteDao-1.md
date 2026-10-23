# Implementation Plan: FavoriteDao MSVC Compilation & SQL Syntax Fix

## 1. Overview
This implementation plan fixes MSVC 2022 compilation errors (C3861 `qrand` not found, C2672 `QString::arg` ambiguity) and a SQL CTE syntax error in `FavoriteDao.cpp`.

### Key Fixes:
1. **`qrand()` Depreciation**: Replaced `qrand()` with `QRandomGenerator::global()->generate()`, including `<QRandomGenerator>`.
2. **`QString::arg` Ambiguity (C2672)**: Explicitly cast `QDateTime::currentMSecsSinceEpoch()` to `qlonglong`.
3. **SQLite Recursive CTE Syntax**: Fixed `WHERE id IN cnt` to valid SQLite syntax `WHERE id IN (SELECT x FROM cnt)`.

---

## 2. Modified Files List
1. `src/meta/FavoriteDao.cpp`
2. `src/meta/FavoriteService.cpp`
3. `src/ui/FavoritePanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/meta/FavoriteDao.cpp`
```
<<<<<<< SEARCH
int FavoriteDao::addVirtualCategory(const QString& name, int parentId, const QString& iconKey, const QString& colorHex) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QString virtPath = QString("virtual_cat_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(qrand() % 1000);
=======
int FavoriteDao::addVirtualCategory(const QString& name, int parentId, const QString& iconKey, const QString& colorHex) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    quint32 randVal = QRandomGenerator::global()->generate() % 1000;
    QString virtPath = QString("virtual_cat_%1_%2").arg(static_cast<qlonglong>(QDateTime::currentMSecsSinceEpoch())).arg(randVal);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool FavoriteDao::removeFavoriteById(int id) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "WITH RECURSIVE cnt(x) AS ("
                      "  SELECT ? UNION ALL SELECT id FROM favorites, cnt WHERE favorites.parent_id = cnt.x"
                      ") DELETE FROM favorites WHERE id IN cnt;";
=======
bool FavoriteDao::removeFavoriteById(int id) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "WITH RECURSIVE cnt(x) AS ("
                      "  SELECT ? UNION ALL SELECT id FROM favorites, cnt WHERE favorites.parent_id = cnt.x"
                      ") DELETE FROM favorites WHERE id IN (SELECT x FROM cnt);";
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
Re-run MSVC 2022 x64 build. Confirm C3861 and C2672 are resolved, and `FavoriteDao` compiles with zero warnings/errors.
