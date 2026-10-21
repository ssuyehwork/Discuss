# SmartFolderMatcher 智能文件夹过滤失效与数据库死锁修复实施方案

## 1. 概述 (Overview)

在 QuarkMeta 的“智能文件夹”功能中，配置规则（例如针对扩展名 `.cpp`、`.eps` 或关键字/星级过滤）后，视图和分类树中的统计数量与加载结果均返回 0 项。
经过深入分析，定位到以下两个核心根因：
1. **`LibraryItemDao::getAllLibraryItems()` 死锁瓶颈**：在 `getAllLibraryItems()` 中，持有 `m_globalDbMutex` 互斥锁的同时直接调用了 `getItemsByPaths(paths)`，而 `getItemsByPaths` 内部会二次获取同一个非递归互斥锁 `m_globalDbMutex`。这在后台线程（如 `updateSmartFolderCountsAsync` 及 `loadSmartFolder`）执行时引发了自重入死锁或挂起，导致数据库查询中断并返回空集合（0 项）。
2. **`SmartFolderMatcher` 扩展名单字符串比对局限**：智能文件夹配置弹窗（`SmartFolderDialog`）的提示词引导用户输入用逗号/空格分隔的多扩展名格式（如 `png, psd` 或 `cpp, eps`），但 `SmartFolderMatcher::matches` 原先将整个输入字符串当作单一后缀进行 `record.suffix.compare(...)` 比对，导致包含逗号/空格的规则输入无法匹配任何文件。同时，若 `library_items` 中 `extension` 字段暂为空，未能自动回退解析文件名后缀。

本方案针对上述问题进行精准修复：
- 限制 `getAllLibraryItems()` 中互斥锁的作用域，仅在执行路径查询 SQL 时加锁，随后释放锁再调用 `getItemsByPaths(paths)`；同时增加 `WHERE sync_state != 2` 过滤以排除缺失/已删除的遗留项。
- 在 `getItemsByPaths` 中增加 `rec.suffix` 判空与 `QFileInfo` 后缀回退补齐逻辑。
- 在 `SmartFolderMatcher` 中引入 `QRegularExpression`，对扩展名规则值按逗号、分号及空格进行拆分并剥离前导点号（如 `.cpp, .eps`），逐个匹配文件的 `record.suffix`，彻底解决多扩展名及带点号规则匹配失效的问题。

---

## 2. 修改文件清单 (Modified Files List)

| 序号 | 相对文件路径 | 修改类型 | 职责说明 |
| :--- | :--- | :--- | :--- |
| 1 | `src/meta/LibraryItemDao.cpp` | 逻辑修正 | 修复 `getAllLibraryItems` 互斥锁重入死锁，增加 `sync_state != 2` 过滤及后缀缺失回退 |
| 2 | `src/meta/SmartFolderMatcher.cpp` | 逻辑增强 | 引入 `QRegularExpression` 拆分扩展名规则，支持多扩展名与点号前缀匹配 |

---

## 3. 逐行修改详情 (Detailed Line-by-Line Changes)

### 3.1 `src/meta/LibraryItemDao.cpp`

```
<<<<<<< SEARCH
std::vector<ItemRecord> LibraryItemDao::getAllLibraryItems() {
    std::vector<ItemRecord> result;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    const char* sql = "SELECT file_path FROM library_items;";
    sqlite3_stmt* stmt = nullptr;
    QStringList paths;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            paths.append(QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
        }
        sqlite3_finalize(stmt);
    }
    return getItemsByPaths(paths);
}
=======
std::vector<ItemRecord> LibraryItemDao::getAllLibraryItems() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return {};

    QStringList paths;
    {
        std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
        const char* sql = "SELECT file_path FROM library_items WHERE sync_state != 2;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                paths.append(QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
            }
            sqlite3_finalize(stmt);
        }
    }
    return getItemsByPaths(paths);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                rec.filename = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
                rec.suffix = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
                rec.size = sqlite3_column_int64(stmt, 5);
=======
                rec.filename = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
                rec.suffix = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
                if (rec.suffix.isEmpty()) {
                    rec.suffix = QFileInfo(rec.filename.isEmpty() ? rec.path : rec.filename).suffix();
                }
                rec.size = sqlite3_column_int64(stmt, 5);
>>>>>>> REPLACE
```

---

### 3.2 `src/meta/SmartFolderMatcher.cpp`

```
<<<<<<< SEARCH
#include "SmartFolderMatcher.h"
#include "../ui/StyleLibrary.h"

namespace QuarkMeta {
=======
#include "SmartFolderMatcher.h"
#include "../ui/StyleLibrary.h"
#include <QRegularExpression>

namespace QuarkMeta {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            case SmartRuleType::Extension: {
                QString ext = rule.value.trimmed();
                if (ext.startsWith('.')) ext.remove(0, 1);

                bool equalExt = (record.suffix.compare(ext, Qt::CaseInsensitive) == 0);
                if (rule.op == SmartRuleOperator::NotContains) {
                    ruleMatched = !equalExt;
                } else {
                    ruleMatched = equalExt;
                }
                break;
            }
=======
            case SmartRuleType::Extension: {
                QString rawVal = rule.value.trimmed();
                QStringList exts = rawVal.split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts);
                bool matchedAny = false;
                for (QString ext : exts) {
                    if (ext.startsWith('.')) ext.remove(0, 1);
                    if (!ext.isEmpty() && record.suffix.compare(ext, Qt::CaseInsensitive) == 0) {
                        matchedAny = true;
                        break;
                    }
                }
                if (rule.op == SmartRuleOperator::NotContains) {
                    ruleMatched = !matchedAny;
                } else {
                    ruleMatched = matchedAny;
                }
                break;
            }
>>>>>>> REPLACE
```

---

## 4. 构建与验证步骤 (Build & Verification Steps)

1. **增量编译核查**：
   在 Linux / Windows 编译环境下运行 CMake 构建命令：
   ```bash
   cmake -B build
   cmake --build build -j8
   ```
   验证 `LibraryItemDao.cpp` 及 `SmartFolderMatcher.cpp` 无任何 C++ 编译警告和错误。

2. **功能验证**：
   - 打开智能文件夹对话框，新建规则选择“扩展名/类型”，输入 `.cpp, .eps` 或 `png, psd`。
   - 检查侧边栏“智能文件夹”项右侧的实时计数，确认不再显示 0 而是正确返回数据库内匹配的实体数量。
   - 点击该智能文件夹，确认中间网格/列表视图正常加载对应的文件，没有出现卡死或界面响应阻塞。

---

## 5. SSOT API 复用自查 (SSOT API Reuse Self-Check)

- **`LibraryItemDao::getItemsByPaths`**：直接复用已有数据库记录解析接口，未增加冗余 SQL 查询。
- **`SmartFolderMatcher::matches`**：保持纯函数入口，仅提升规则解析泛化与稳健性。
- **`DatabaseManager::getGlobalMutex`**：严格遵循全局数据库互斥锁规范，仅调整锁的生命周期范围以消除死锁。

---

## 6. 头文件 API 签名校验 (Header API Signature Verification)

本次修改未改动任何 `.h` 头的公开 API 签名，所有公共接口完全保持不动（符合 Contract Lock 契约锁）：
- `LibraryItemDao.h` 中的 `static std::vector<ItemRecord> getAllLibraryItems();` 保持不变。
- `SmartFolderMatcher.h` 中的 `static bool matches(const ItemRecord& record, const SmartFolderRecord& sfRec);` 保持不变。

---

## 7. 头文件包含链与类型完整性检查 (Header Inclusion Chain & Type Completeness Check)

- `src/meta/SmartFolderMatcher.cpp` 中显式包含了 `<QRegularExpression>` 标头，保证 `QRegularExpression` 类型完全定义，无隐式传递或类型不完整问题。
- `src/meta/LibraryItemDao.cpp` 已包含 `<QFileInfo>`、`<QDir>`，支持回退提取 `suffix`。

---

## 8. 合规自查表 (Compliance Self-Check Table)

| 检查项 | 状态 | 说明 |
| :--- | :--- | :--- |
| 1. 语言规范 | 通过 | 全篇使用中文说明 |
| 2. 隔离目录要求 | 通过 | 实施方案存放在 `QuarkMeta Architecture/Implementation Plan/SmartFolderMatcher.md` |
| 3. 代码修改方式 | 通过 | 严格采用 Git Merge Diff 格式描述修改 |
| 4. 无源代码直接修改 | 通过 | 未对 `src/` 目录下实际源文件进行物理改动 |
| 5. 零值变更规范 | 通过 | 未更改任何 UI 样式数值或系统参数 |
| 6. 死锁排查 | 通过 | 明确消除 `getAllLibraryItems` 互斥锁嵌套重入 |
| 7. 头文件契约锁 | 通过 | 无 API 签名修改 |

---

## 9. 零值变更检查 (Zero-Value-Alteration Check)

本次修复仅针对后台数据查询死锁与过滤匹配逻辑，未对任何 QSS 样式表、边距、Padding、颜色 Hex 值或字体大小进行改动，符合 100% 零值变更承诺。
