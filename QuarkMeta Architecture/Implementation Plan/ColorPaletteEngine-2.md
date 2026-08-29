# ColorPaletteEngine-2 Implementation Plan

## Overview
This implementation plan outlines the exact changes required to persist non-hardcoded file extension badge colors into `global.db` (SQLite database). It establishes the `extension_colors` database table and connects `ColorPaletteEngine` with an `ExtensionColorDao` data access component to enable physical disk persistence, flexible color updates, and 0-latency memory LRU caching.

## Modified Files List
1. `src/meta/ExtensionColorDao.h` (New File)
2. `src/meta/ExtensionColorDao.cpp` (New File)
3. `src/util/ColorPaletteEngine.h`
4. `src/util/ColorPaletteEngine.cpp`
5. `src/ui/CardPainterHelper.cpp`
6. `CMakeLists.txt`

---

## Detailed Line-by-Line Changes

### 1. `src/meta/ExtensionColorDao.h` (New File)
Define `ExtensionColorDao` for accessing and updating the `extension_colors` table in `global.db`.

```cpp
#pragma once
#include <QString>
#include <QColor>
#include <QPair>
#include <QMap>

namespace QuarkMeta {

class ExtensionColorDao {
public:
    static bool initTable();
    static bool getColorForExtension(const QString& ext, QColor& outBg, QColor& outText);
    static bool saveExtensionColor(const QString& ext, const QColor& bg, const QColor& text, bool isCustom = false);
    static QMap<QString, QPair<QColor, QColor>> loadAllColors();
};

} // namespace QuarkMeta
```

---

### 2. `src/meta/ExtensionColorDao.cpp` (New File)
Implement `ExtensionColorDao` using raw `sqlite3*` interface with `DatabaseManager::getGlobalDatabaseHandle()`.

```cpp
#include "ExtensionColorDao.h"
#include "DatabaseManager.h"
#include <sqlite3.h>
#include <QDateTime>

namespace QuarkMeta {

bool ExtensionColorDao::initTable() {
    sqlite3* db = DatabaseManager::getGlobalDatabaseHandle();
    if (!db) return false;

    const char* sql = "CREATE TABLE IF NOT EXISTS extension_colors ("
                      "extension TEXT PRIMARY KEY, "
                      "bg_color TEXT NOT NULL, "
                      "text_color TEXT NOT NULL, "
                      "is_custom INTEGER DEFAULT 0, "
                      "updated_at INTEGER);";

    char* errMsgs = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsgs);
    if (rc != SQLITE_OK) {
        if (errMsgs) sqlite3_free(errMsgs);
        return false;
    }
    return true;
}

bool ExtensionColorDao::getColorForExtension(const QString& ext, QColor& outBg, QColor& outText) {
    sqlite3* db = DatabaseManager::getGlobalDatabaseHandle();
    if (!db) return false;

    const char* sql = "SELECT bg_color, text_color FROM extension_colors WHERE extension = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string extStd = ext.toLower().trimmed().toStdString();
    sqlite3_bind_text(stmt, 1, extStd.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* bgStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* textStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (bgStr && textStr) {
            outBg = QColor(QString::fromUtf8(bgStr));
            outText = QColor(QString::fromUtf8(textStr));
            found = outBg.isValid() && outText.isValid();
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

bool ExtensionColorDao::saveExtensionColor(const QString& ext, const QColor& bg, const QColor& text, bool isCustom) {
    sqlite3* db = DatabaseManager::getGlobalDatabaseHandle();
    if (!db) return false;

    const char* sql = "INSERT INTO extension_colors (extension, bg_color, text_color, is_custom, updated_at) "
                      "VALUES (?, ?, ?, ?, ?) "
                      "ON CONFLICT(extension) DO UPDATE SET "
                      "bg_color=excluded.bg_color, text_color=excluded.text_color, "
                      "is_custom=excluded.is_custom, updated_at=excluded.updated_at;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string extStd = ext.toLower().trimmed().toStdString();
    std::string bgStd = bg.name().toUpper().toStdString();
    std::string textStd = text.name().toUpper().toStdString();
    qint64 nowSecs = QDateTime::currentSecsSinceEpoch();

    sqlite3_bind_text(stmt, 1, extStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, bgStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, textStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, isCustom ? 1 : 0);
    sqlite3_bind_int64(stmt, 5, nowSecs);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    // 固化 PASSIVE 检查点到 global.db 主文件
    DatabaseManager::flushWalCheckpoint();
    return success;
}

QMap<QString, QPair<QColor, QColor>> ExtensionColorDao::loadAllColors() {
    QMap<QString, QPair<QColor, QColor>> resultMap;
    sqlite3* db = DatabaseManager::getGlobalDatabaseHandle();
    if (!db) return resultMap;

    const char* sql = "SELECT extension, bg_color, text_color FROM extension_colors;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return resultMap;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* extStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* bgStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* textStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (extStr && bgStr && textStr) {
            resultMap[QString::fromUtf8(extStr)] = { QColor(QString::fromUtf8(bgStr)), QColor(QString::fromUtf8(textStr)) };
        }
    }
    sqlite3_finalize(stmt);
    return resultMap;
}

} // namespace QuarkMeta
```

---

### 3. `src/util/ColorPaletteEngine.cpp`
Integrate `ExtensionColorDao` with memory LRU caching.

```
<<<<<<< SEARCH
QPair<QColor, QColor> ColorPaletteEngine::getExtensionBadgeColors(const QString& ext) {
    QString e = ext.toLower().trimmed();
    
    // 1. 特性定制扩展名配色
    if (e == "psd" || e == "psb") {
        return { QColor("#001D26"), QColor("#02B1DD") };
    }
    if (e == "eps") {
        return { QColor("#35483D"), QColor("#F88025") };
    }
    if (e == "ai") {
        return { QColor("#F88025"), QColor("#35483D") };
    }
    if (e == "svg") {
        return { QColor("#FFB13B"), QColor("#1A1A1A") };
    }
    if (e == "png") {
        return { QColor("#2ECC71"), QColor("#FFFFFF") };
    }
    if (e == "jpg" || e == "jpeg") {
        return { QColor("#E67E22"), QColor("#FFFFFF") };
    }
    if (e == "pdf") {
        return { QColor("#E74C3C"), QColor("#FFFFFF") };
    }

    // 2. 其他未指定扩展名：基于 qHash 确定性 HSL 生成唯一背景色
    uint hashVal = qHash(e);
    int hue = static_cast<int>(hashVal % 360);
    int saturation = 130 + static_cast<int>((hashVal >> 8) % 80); // 130~210
    int lightness = 80 + static_cast<int>((hashVal >> 16) % 60);  // 80~140

    QColor bgColor = QColor::fromHsl(hue, saturation, lightness);

    // 3. 计算亮度自动平衡文字颜色 (YIQ Luminance)
    double luminance = (0.299 * bgColor.red() + 0.587 * bgColor.green() + 0.114 * bgColor.blue()) / 255.0;
    QColor textColor = (luminance < 0.55) ? QColor("#FFFFFF") : QColor("#1A1A1A");

    return { bgColor, textColor };
}
=======
QPair<QColor, QColor> ColorPaletteEngine::getExtensionBadgeColors(const QString& ext) {
    QString e = ext.toLower().trimmed();
    
    // 1. 独占硬编码保护项
    if (e == "psd" || e == "psb") return { QColor("#001D26"), QColor("#02B1DD") };
    if (e == "eps")               return { QColor("#35483D"), QColor("#F88025") };
    if (e == "ai")                return { QColor("#F88025"), QColor("#35483D") };

    // 2. 内存缓存第一级查找
    static QMap<QString, QPair<QColor, QColor>> s_colorCache;
    static bool s_tableInited = false;

    if (!s_tableInited) {
        ExtensionColorDao::initTable();
        s_colorCache = ExtensionColorDao::loadAllColors();
        s_tableInited = true;
    }

    if (s_colorCache.contains(e)) {
        return s_colorCache.value(e);
    }

    // 3. 动态生成新配色并物理落盘写入 global.db
    uint hashVal = qHash(e);
    int hue = static_cast<int>(hashVal % 360);
    int saturation = 130 + static_cast<int>((hashVal >> 8) % 80);
    int lightness = 80 + static_cast<int>((hashVal >> 16) % 60);

    QColor bgColor = QColor::fromHsl(hue, saturation, lightness);
    double luminance = (0.299 * bgColor.red() + 0.587 * bgColor.green() + 0.114 * bgColor.blue()) / 255.0;
    QColor textColor = (luminance < 0.55) ? QColor("#FFFFFF") : QColor("#1A1A1A");

    QPair<QColor, QColor> colorPair = { bgColor, textColor };
    
    // 刷盘固化并填充内存 Cache
    ExtensionColorDao::saveExtensionColor(e, bgColor, textColor, false);
    s_colorCache.insert(e, colorPair);

    return colorPair;
}
>>>>>>> REPLACE
```

---

### 4. `CMakeLists.txt`
Register `ExtensionColorDao.h` and `ExtensionColorDao.cpp` into build sources.

```
<<<<<<< SEARCH
    src/meta/DatabaseManager.h
    src/meta/DatabaseManager.cpp
=======
    src/meta/DatabaseManager.h
    src/meta/DatabaseManager.cpp
    src/meta/ExtensionColorDao.h
    src/meta/ExtensionColorDao.cpp
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Verify implementation plan file existence in `QuarkMeta Architecture/Implementation Plan/ColorPaletteEngine-2.md`.
2. Compile project with CMake:
   ```bash
   cmake -B build -S .
   cmake --build build
   ```
3. Test SQLite table creation and database query for `extension_colors` in `global.db`.
