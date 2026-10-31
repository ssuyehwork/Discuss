# Implementation Plan: SmartFolderMatchFix.md

- **Base Commit Hash**: 57252f1
- **Prerequisite Plans**: None
- **Supersedes**: None

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与缺陷描述
在 QuarkMeta 的智能文件夹（Smart Folder）功能中，用户创建包含扩展名匹配条件（如名称为 "CPP"、规则为扩展名等于 `cpp, h` 或 `cpp`）的智能文件夹后，智能文件夹检索出的匹配条目数量始终为 `0`（与实际库中存在大量 `.cpp` 和 `.h` 文件的事实不符）。

### 1.2 绝对根本原因分析 (Root Cause Analysis)
通过对 `src/meta/SmartFolderDao.cpp` 源码以及 SQLite 数据库查询流程的排查，锁定根本原因如下：

1. **多后缀/逗号分隔扩展名未拆分解析（直接致错因）**：
   在 `SmartFolderDao::countMatchingItems` 与 `SmartFolderDao::getMatchingPaths` 中，处理 `SmartRuleType::Extension` 类型的规则时，直接将用户输入的规则值 `rule.value.trimmed()`（如 `"cpp, h"` 或 `"cpp, h, hpp"`）当作单一字符串处理，生成 SQL 条件：`WHERE (extension = 'cpp, h')`。由于数据库 `library_items` 表中存储的 `extension` 字段为单文件后缀（如 `'cpp'` 或 `'h'`），这种全匹配比对永远不命中，导致匹配数量恒为 `0`。
2. **后缀前导点号与分隔符清洗不彻底**：
   如果规则输入中包含前导点号（如 `.cpp`）或空格/分号/逗号混合，原逻辑仅去除开头单点，无法妥善处理包含多个后缀的场景。
3. **`SmartFolderDao.cpp` 存在严重代码冗余**：
   `countMatchingItems` 和 `getMatchingPaths` 函数独立编写了近 70 行完全相同的 SQL WHERE 条件生成逻辑，违背了单一步骤真理源 (SSOT) 原则。

### 1.3 拟定重构方案与设计目标
1. 在 `SmartFolderDao.h` 中为 `SmartFolderDao` 类新增私有静态辅助方法 `buildWhereClause`；
2. 在 `buildWhereClause` 中使用正则表达式 `[,;\\s|]+` 对扩展名规则值进行多后缀切分，并清洗前导点号与空白；
3. 根据切分后的后缀数量，动态生成 `LOWER(extension) = LOWER('cpp')` 或 `LOWER(extension) IN (LOWER('cpp'), LOWER('h'))`（或对应的 `NOT IN`）SQL 条件，确保大小写不敏感且精确支持多后缀；
4. 使 `countMatchingItems` 与 `getMatchingPaths` 统一调用 `buildWhereClause`，彻底消除 100+ 行重复代码。

---

## Chapter 2: Modified Files List（影响文件清单）

- `src/meta/SmartFolderDao.h` (修改：新增 `buildWhereClause` 私有静态方法声明)
- `src/meta/SmartFolderDao.cpp` (修改：新增 `<QRegularExpression>` 包含，实现 `buildWhereClause`，重构 `countMatchingItems` 与 `getMatchingPaths`)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/meta/SmartFolderDao.h`

<<<<<<< SEARCH
class SmartFolderDao {
public:
    static bool initTable();
    static QList<SmartFolderRecord> getAllSmartFolders();
    static SmartFolderRecord getSmartFolderById(int id);
    static int addSmartFolder(const SmartFolderRecord& rec);
    static bool updateSmartFolder(const SmartFolderRecord& rec);
    static bool removeSmartFolderById(int id);
    static int countMatchingItems(const SmartFolderRecord& rec);
    static QStringList getMatchingPaths(const SmartFolderRecord& rec);
};
=======
class SmartFolderDao {
public:
    static bool initTable();
    static QList<SmartFolderRecord> getAllSmartFolders();
    static SmartFolderRecord getSmartFolderById(int id);
    static int addSmartFolder(const SmartFolderRecord& rec);
    static bool updateSmartFolder(const SmartFolderRecord& rec);
    static bool removeSmartFolderById(int id);
    static int countMatchingItems(const SmartFolderRecord& rec);
    static QStringList getMatchingPaths(const SmartFolderRecord& rec);

private:
    static QString buildWhereClause(const SmartFolderRecord& rec);
};
>>>>>>> REPLACE

---

### 3.2 `src/meta/SmartFolderDao.cpp`

<<<<<<< SEARCH
#include <sqlite3.h>
#include <QJsonDocument>
#include <QDateTime>
#include <QFileInfo>
#include <QDirIterator>
#include <QSet>
=======
#include <sqlite3.h>
#include <QJsonDocument>
#include <QDateTime>
#include <QFileInfo>
#include <QDirIterator>
#include <QSet>
#include <QRegularExpression>
>>>>>>> REPLACE

<<<<<<< SEARCH
int SmartFolderDao::countMatchingItems(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return 0;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    // 1. 动态生成 WHERE 条件
    QStringList whereConditions;
    for (const auto& rule : rec.rules) {
        if (rule.type == SmartRuleType::Rating) {
            QString op = (rule.op == SmartRuleOperator::GreaterOrEqual) ? ">=" :
                         (rule.op == SmartRuleOperator::LessOrEqual) ? "<=" : "=";
            whereConditions << QString("rating %1 %2").arg(op).arg(rule.value.toInt());
        }
        else if (rule.type == SmartRuleType::Extension) {
            QString ext = rule.value.trimmed();
            if (ext.startsWith('.')) ext.remove(0, 1);
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("extension != '%1'").arg(ext);
            } else {
                whereConditions << QString("extension = '%1'").arg(ext);
            }
        }
        else if (rule.type == SmartRuleType::Color) {
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("color_hex != '%1'").arg(rule.value.trimmed());
            } else {
                whereConditions << QString("color_hex = '%1'").arg(rule.value.trimmed());
            }
        }
        else if (rule.type == SmartRuleType::Tag) {
            // 利用 file_tags 多对多关联表实现秒级查询 (以 file_id 解耦匹配)
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("file_id NOT IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            } else {
                whereConditions << QString("file_id IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            }
        }
        else if (rule.type == SmartRuleType::Keyword) {
            QString kw = rule.value.trimmed();
            if (!kw.isEmpty()) {
                if (rule.op == SmartRuleOperator::NotContains) {
                    whereConditions << QString("file_name NOT LIKE '%%%1%%'").arg(kw);
                } else {
                    whereConditions << QString("file_name LIKE '%%%1%%'").arg(kw);
                }
            }
        }
    }

    if (whereConditions.isEmpty()) return 0;

    QString connector = (rec.matchMode.toUpper() == "ALL") ? " AND " : " OR ";
    QString sql = QString("SELECT COUNT(*) FROM library_items WHERE (%1) AND sync_state != 2;")
                  .arg(whereConditions.join(connector));

    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

QStringList SmartFolderDao::getMatchingPaths(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return {};

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return {};

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QStringList whereConditions;
    for (const auto& rule : rec.rules) {
        if (rule.type == SmartRuleType::Rating) {
            QString op = (rule.op == SmartRuleOperator::GreaterOrEqual) ? ">=" :
                         (rule.op == SmartRuleOperator::LessOrEqual) ? "<=" : "=";
            whereConditions << QString("rating %1 %2").arg(op).arg(rule.value.toInt());
        }
        else if (rule.type == SmartRuleType::Extension) {
            QString ext = rule.value.trimmed();
            if (ext.startsWith('.')) ext.remove(0, 1);
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("extension != '%1'").arg(ext);
            } else {
                whereConditions << QString("extension = '%1'").arg(ext);
            }
        }
        else if (rule.type == SmartRuleType::Color) {
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("color_hex != '%1'").arg(rule.value.trimmed());
            } else {
                whereConditions << QString("color_hex = '%1'").arg(rule.value.trimmed());
            }
        }
        else if (rule.type == SmartRuleType::Tag) {
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("file_id NOT IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            } else {
                whereConditions << QString("file_id IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            }
        }
        else if (rule.type == SmartRuleType::Keyword) {
            QString kw = rule.value.trimmed();
            if (!kw.isEmpty()) {
                if (rule.op == SmartRuleOperator::NotContains) {
                    whereConditions << QString("file_name NOT LIKE '%%%1%%'").arg(kw);
                } else {
                    whereConditions << QString("file_name LIKE '%%%1%%'").arg(kw);
                }
            }
        }
    }

    if (whereConditions.isEmpty()) return {};

    QString connector = (rec.matchMode.toUpper() == "ALL") ? " AND " : " OR ";
    QString sql = QString("SELECT file_path FROM library_items WHERE (%1) AND sync_state != 2;")
                  .arg(whereConditions.join(connector));

    sqlite3_stmt* stmt = nullptr;
    QStringList paths;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* p = sqlite3_column_text(stmt, 0);
            if (p) {
                paths.append(QString::fromUtf8(reinterpret_cast<const char*>(p)));
            }
        }
        sqlite3_finalize(stmt);
    }
    return paths;
}
=======
QString SmartFolderDao::buildWhereClause(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return QString();

    QStringList whereConditions;
    for (const auto& rule : rec.rules) {
        if (rule.type == SmartRuleType::Rating) {
            QString op = (rule.op == SmartRuleOperator::GreaterOrEqual) ? ">=" :
                         (rule.op == SmartRuleOperator::LessOrEqual) ? "<=" : "=";
            whereConditions << QString("rating %1 %2").arg(op).arg(rule.value.toInt());
        }
        else if (rule.type == SmartRuleType::Extension) {
            QString rawValue = rule.value.trimmed();
            if (!rawValue.isEmpty()) {
                static const QRegularExpression splitRegex("[,;\\s|]+");
                QStringList rawTokens = rawValue.split(splitRegex, Qt::SkipEmptyParts);
                QStringList cleanExts;
                for (QString token : rawTokens) {
                    token = token.trimmed();
                    if (token.startsWith('.')) {
                        token.remove(0, 1);
                    }
                    token = token.trimmed();
                    if (!token.isEmpty()) {
                        cleanExts << token.replace("'", "''");
                    }
                }

                if (!cleanExts.isEmpty()) {
                    if (cleanExts.size() == 1) {
                        if (rule.op == SmartRuleOperator::NotContains) {
                            whereConditions << QString("LOWER(extension) != LOWER('%1')").arg(cleanExts.first());
                        } else {
                            whereConditions << QString("LOWER(extension) = LOWER('%1')").arg(cleanExts.first());
                        }
                    } else {
                        QStringList quotedList;
                        for (const QString& ext : cleanExts) {
                            quotedList << QString("LOWER('%1')").arg(ext);
                        }
                        if (rule.op == SmartRuleOperator::NotContains) {
                            whereConditions << QString("LOWER(extension) NOT IN (%1)").arg(quotedList.join(", "));
                        } else {
                            whereConditions << QString("LOWER(extension) IN (%1)").arg(quotedList.join(", "));
                        }
                    }
                }
            }
        }
        else if (rule.type == SmartRuleType::Color) {
            QString colorVal = rule.value.trimmed().replace("'", "''");
            if (!colorVal.isEmpty()) {
                if (rule.op == SmartRuleOperator::NotContains) {
                    whereConditions << QString("color_hex != '%1'").arg(colorVal);
                } else {
                    whereConditions << QString("color_hex = '%1'").arg(colorVal);
                }
            }
        }
        else if (rule.type == SmartRuleType::Tag) {
            // 利用 file_tags 多对多关联表实现秒级查询 (以 file_id 解耦匹配)
            QString tagVal = rule.value.trimmed().replace("'", "''");
            if (!tagVal.isEmpty()) {
                if (rule.op == SmartRuleOperator::NotContains) {
                    whereConditions << QString("file_id NOT IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                       .arg(tagVal);
                } else {
                    whereConditions << QString("file_id IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                       .arg(tagVal);
                }
            }
        }
        else if (rule.type == SmartRuleType::Keyword) {
            QString kw = rule.value.trimmed().replace("'", "''");
            if (!kw.isEmpty()) {
                if (rule.op == SmartRuleOperator::NotContains) {
                    whereConditions << QString("file_name NOT LIKE '%%%1%%'").arg(kw);
                } else {
                    whereConditions << QString("file_name LIKE '%%%1%%'").arg(kw);
                }
            }
        }
    }

    if (whereConditions.isEmpty()) return QString();

    QString connector = (rec.matchMode.toUpper() == "ALL") ? " AND " : " OR ";
    return "(" + whereConditions.join(connector) + ")";
}

int SmartFolderDao::countMatchingItems(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return 0;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QString whereClause = buildWhereClause(rec);
    if (whereClause.isEmpty()) return 0;

    QString sql = QString("SELECT COUNT(*) FROM library_items WHERE %1 AND sync_state != 2;")
                  .arg(whereClause);

    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

QStringList SmartFolderDao::getMatchingPaths(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return {};

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return {};

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QString whereClause = buildWhereClause(rec);
    if (whereClause.isEmpty()) return {};

    QString sql = QString("SELECT file_path FROM library_items WHERE %1 AND sync_state != 2;")
                  .arg(whereClause);

    sqlite3_stmt* stmt = nullptr;
    QStringList paths;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* p = sqlite3_column_text(stmt, 0);
            if (p) {
                paths.append(QString::fromUtf8(reinterpret_cast<const char*>(p)));
            }
        }
        sqlite3_finalize(stmt);
    }
    return paths;
}
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译指令
在 CMake 构建框架下执行编译命令：
```bash
cmake --build build --config Debug --target QuarkMeta
```

### 4.2 运行时验收标准
1. **智能文件夹显示非零条目**：在“库”侧边栏中新建或查看名为 “CPP” 的智能文件夹（规则配置为扩展名：`cpp, h` 或 `cpp`），侧边栏数字标识正确显示匹配到的真实文件数量（而非 0）；
2. **多后缀与不区分大小写匹配验证**：输入 `cpp, h`、`.cpp`、`CPP` 或 `cpp; h` 时，智能文件夹均能精准检索并加载对应的源文件与头文件路径；
3. **复合规则匹配正确性**：验证当规则包含星级、标签、颜色及多后缀复合条件时，AND (ALL) 与 OR (ANY) 逻辑匹配结果完全符合 SQL 预期；
4. **功能无回归验证**：原有智能文件夹的增删改查及图标/匹配模式选择功能正常工作。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- **数据库底层单例复用**：完整复用 `DatabaseManager::instance().getGlobalDb()` 及 `DatabaseManager::instance().getGlobalMutex()` 线程安全锁，无另起炉灶建立数据库连接。
- **内部 SQL 生成归一化**：通过引入私有辅助方法 `buildWhereClause`，将 `countMatchingItems` 与 `getMatchingPaths` 两处冗余重复的条件生成逻辑彻底物理收拢为唯一的真理源函数。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 智能文件夹配置记录 | `global.db` 中的 `smart_folders` 表 | `SmartFolderRecord` | 读写通过 `SmartFolderDao` 进行序列化与反序列化 |
| 库文件元数据 | `global.db` 中的 `library_items` 表 | UI 视图项 / `ItemRecord` | `SmartFolderDao::getMatchingPaths` 查询生成符合 SQL 条件的路径列表 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 物理 `.h` 头文件签名核查
物理读取 `src/meta/SmartFolderDao.h`：
- 原有 `public` 静态方法签名完全保持原样，对外接口契约 100% 冻结：
  - `static int countMatchingItems(const SmartFolderRecord& rec);`
  - `static QStringList getMatchingPaths(const SmartFolderRecord& rec);`

### 7.2 新增私有辅助函数签名与可见性
- 新增声明位置：`src/meta/SmartFolderDao.h` 中的 `private:` 区域：
  ```cpp
  static QString buildWhereClause(const SmartFolderRecord& rec);
  ```
- 定义位置：`src/meta/SmartFolderDao.cpp`；
- 调用点：仅在 `SmartFolderDao::countMatchingItems` 和 `SmartFolderDao::getMatchingPaths` 中内部调用，访问控制权限（`private`）完美匹配。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `src/meta/SmartFolderDao.cpp` 新增 `#include <QRegularExpression>`，闭合了切分正则 `splitRegex` 对象的类型定义需求；
- 原有 `<sqlite3.h>`、`<QJsonDocument>`、`<QDateTime>`、`<QFileInfo>`、`<QSet>` 保持不变，包含链完全闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 核对项目 | 是否满足 | 说明/具体位置 |
| :---: | :--- | :---: | :--- |
| 1 | 方案放置于 `Implementation Plan/` 物理隔离目录 | 满足 | `Implementation Plan/SmartFolderMatchFix.md` |
| 2 | 方案只读与版本号增量契约 | 满足 | 初始文件 `SmartFolderMatchFix.md` |
| 3 | 采用 Git Merge Diff 格式 | 满足 | Chapter 3 均使用标准 SEARCH/REPLACE 块 |
| 4 | SEARCH 块物理匹配唯一性 | 满足 | 块内上下文均在 `SmartFolderDao.h/.cpp` 中唯一存在 |
| 5 | 无直接修改源代码承诺 | 满足 | 方案修改仅存储于本物理隔离方案文件中 |
| 6 | 工业级五道工程硬锁之【契约锁】 | 满足 | 公开 `SmartFolderDao` 接口签名冻结无修改 |
| 7 | 工业级五道工程硬锁之【依赖锁】 | 满足 | Data 基础设施层不逆向包含 UI/Controller |
| 8 | 工业级五道工程硬锁之【门禁锁】 | 满足 | 答复 SSOT 溯源与绝对根因 |
| 9 | 工业级五道工程硬锁之【静态红线】 | 满足 | 无 SetForegroundWindow、无 Hack 事件过滤器 |
| 10 | 工业级五道工程硬锁之【黑盒隔离】 | 满足 | 模块私有逻辑封装于 `SmartFolderDao.cpp` 内部 |
| 11 | 开发测试阶段免数据迁移规则 | 满足 | SQL 查询层面自适应优化，无需对已有表改动 |
| 12 | CMakeLists.txt 注册全覆盖 | 不涉及 | 本次无新建或删除 `.h`/`.cpp` 文件 |
| 13 | 线程安全与 SQL 互斥锁 | 满足 | 严格保持 `std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());` |
| 14 | 多线程 UI 跨线程限制 | 满足 | 数据库 DAO 纯后台同步处理，无 UI 控件跨线程引用 |
| 15 | SQL 单引号安全转义 | 满足 | 规则值经过 `replace("'", "''")` 处理，防止 SQL 注入 |
| 16 | 不区分大小写匹配支持 | 满足 | 使用 `LOWER(extension)` 解决字段大小写混用匹配 |
| 17 | 多后缀分割解析支持 | 满足 | 使用 `QRegularExpression("[,;\\s|]+")` 正则切分多后缀 |
| 18 | 代码冗余物理消除 | 满足 | 抽离 `buildWhereClause` 统一条件构建 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 行为与交互零篡改声明
- `SmartFolderDialog` 智能文件夹编辑弹窗的创建、编辑、添加规则交互与保存逻辑 100% 保持不变；
- 侧边栏智能文件夹节点的显示、数量计算触发与导航逻辑 100% 保持不变。

### 10.2 样式数值零篡改声明
- 本次重构不修改任何 UI QSS 样式、物理尺寸、边距或颜色数值，样式与几何参数 100% 零篡改。
