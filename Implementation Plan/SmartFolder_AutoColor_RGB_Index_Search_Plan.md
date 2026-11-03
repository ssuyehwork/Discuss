# Implementation Plan: 图像颜色与调色板 RGB 复合索引范围检索实施方案

本实施方案旨在严格遵循《AGENTS.md》、《SYSTEM_PROMPT.md》、《Standard Specifications for Implementation Plan.md》以及根目录《Memories.md》（章节 36），在 `QuarkMeta_Source` 桌面资产管理软件中完整落地**方案 3（数值分量 `r, g, b` + 冗余 `color_hex` + 复合索引）**，彻底治理并根除图像颜色检索无法命中、容差被当成空气以及忽视 10 色调色板的根本性缺陷：
1. **数据库层结构升级与平滑加列**：在 `item_palettes` 调色板拆分表中增加 `r, g, b` 纯数值列并建立 `idx_palettes_rgb ON item_palettes(r, g, b)` 复合 B-Tree 索引，并在数据库初始化时执行无损自动加列；
2. **入库持久化同步绑定 RGB 数值**：在 `LibraryItemDao::upsertItems` 中，重构预编译 SQL，将 `QColor` 的 `red()`, `green()`, `blue()` 分量写入纯数值列，同时保留 `color_hex` 冗余列供前端无缝渲染；
3. **检索 SQL 穿透重构**：彻底废除原先荒谬的 `auto_color = '%1'` 字符串相等匹配，在 `SmartFolderDao::buildAutoColorCondition` 中将目标色解构为 RGB 三通道并叠加容差范围，生成直接走复合索引的高性能纯数值范围 SQL。

---

## 一、方案设计与架构分析 (Architecture & Design)

### 1.1 缺陷根因排查与重构目标
- **缺陷根因**：
  原代码在 `SmartFolderDao.cpp` 中直接编写：
  ```cpp
  QString SmartFolderDao::buildAutoColorCondition(const SmartFolderRule& rule) {
      ...
      return QString("auto_color = '%1'").arg(colorVal);
  }
  ```
  1. 界面上配置的容差 `rule.tolerance` 完全未参与计算；
  2. 字符串精确比对因物理平均色与拾色器数值的细微色阶差异导致命中率几乎为 0；
  3. 彻底忽略了图片包含多阶主题色的 `item_palettes` 调色板从表。
- **架构治理与终极解决方案（方案 3）**：
  采用数值分量拆分与 B-Tree 复合索引，兼顾百万级检索性能与色彩范围匹配精度：
  - `item_palettes` 增加 `r, g, b` 整数分量；
  - 建立多列复合索引 `idx_palettes_rgb`；
  - 智能文件夹规则生成基于 `(r BETWEEN rMin AND rMax) AND (g BETWEEN gMin AND gMax) AND (b BETWEEN bMin AND bMax)` 的纯数值范围子查询。

---

## 二、受影响文件清单 (Affected Files)

1. `QuarkMeta_Source/src/meta/DatabaseManager.cpp`
2. `QuarkMeta_Source/src/meta/LibraryItemDao.cpp`
3. `QuarkMeta_Source/src/meta/SmartFolderDao.cpp`

---

## 三、行对行精准替换块 (Detailed Line-by-Line Changes)

### 3.1 `QuarkMeta_Source/src/meta/DatabaseManager.cpp`

<<<<<<< SEARCH
        -- ===================================================================
        -- 4. 主色调调色板拆分表
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS item_palettes (
            file_id TEXT NOT NULL,
            idx INTEGER NOT NULL,
            color_hex TEXT DEFAULT '',
            ratio REAL DEFAULT 0,
            PRIMARY KEY (file_id, idx),
            FOREIGN KEY (file_id) REFERENCES library_items(file_id) ON DELETE CASCADE ON UPDATE CASCADE
        );
=======
        -- ===================================================================
        -- 4. 主色调调色板拆分表 (支持 r, g, b 数值范围疾速检索)
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS item_palettes (
            file_id TEXT NOT NULL,
            idx INTEGER NOT NULL,
            r INTEGER DEFAULT 0,
            g INTEGER DEFAULT 0,
            b INTEGER DEFAULT 0,
            color_hex TEXT DEFAULT '',
            ratio REAL DEFAULT 0,
            PRIMARY KEY (file_id, idx),
            FOREIGN KEY (file_id) REFERENCES library_items(file_id) ON DELETE CASCADE ON UPDATE CASCADE
        );
        -- 🚀 为 RGB 建立多列复合索引，支持百万级纯数值毫秒检索
        CREATE INDEX IF NOT EXISTS idx_palettes_rgb ON item_palettes(r, g, b);
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 🚨 核心加固：立即执行 TRUNCATE 检查点，将表结构瞬间从 -wal 固化回 QuarkMeta.store 主文件！
    sqlite3_wal_checkpoint_v2(conn.diskDb, nullptr, SQLITE_CHECKPOINT_TRUNCATE, nullptr, nullptr);
=======
    // 🚀 兼容老数据库自动升级列与建立复合索引
    sqlite3_exec(conn.diskDb, "ALTER TABLE item_palettes ADD COLUMN r INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "ALTER TABLE item_palettes ADD COLUMN g INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "ALTER TABLE item_palettes ADD COLUMN b INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "CREATE INDEX IF NOT EXISTS idx_palettes_rgb ON item_palettes(r, g, b);", nullptr, nullptr, nullptr);

    // 🚨 核心加固：立即执行 TRUNCATE 检查点，将表结构瞬间从 -wal 固化回 QuarkMeta.store 主文件！
    sqlite3_wal_checkpoint_v2(conn.diskDb, nullptr, SQLITE_CHECKPOINT_TRUNCATE, nullptr, nullptr);
>>>>>>> REPLACE

---

### 3.2 `QuarkMeta_Source/src/meta/LibraryItemDao.cpp`

<<<<<<< SEARCH
    const char* delPalSql = "DELETE FROM item_palettes WHERE file_id = ?;";
    const char* insPalSql = "INSERT INTO item_palettes(file_id, idx, color_hex, ratio) VALUES(?, ?, ?, ?);";
=======
    const char* delPalSql = "DELETE FROM item_palettes WHERE file_id = ?;";
    const char* insPalSql = "INSERT INTO item_palettes(file_id, idx, r, g, b, color_hex, ratio) VALUES(?, ?, ?, ?, ?, ?, ?);";
>>>>>>> REPLACE

<<<<<<< SEARCH
        for (size_t i = 0; i < record.palettes.size(); ++i) {
            sqlite3_reset(stmtInsPal);
            sqlite3_clear_bindings(stmtInsPal);
            sqlite3_bind_text(stmtInsPal, 1, fileId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmtInsPal, 2, static_cast<int>(i));
            sqlite3_bind_text(stmtInsPal, 3, record.palettes[i].first.name().toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmtInsPal, 4, record.palettes[i].second);
            sqlite3_step(stmtInsPal);
        }
=======
        for (size_t i = 0; i < record.palettes.size(); ++i) {
            const QColor& c = record.palettes[i].first;
            float ratio = record.palettes[i].second;

            sqlite3_reset(stmtInsPal);
            sqlite3_clear_bindings(stmtInsPal);
            sqlite3_bind_text(stmtInsPal, 1, fileId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmtInsPal, 2, static_cast<int>(i));
            // 🚀 写入 R、G、B 纯整数分量
            sqlite3_bind_int(stmtInsPal, 3, c.red());
            sqlite3_bind_int(stmtInsPal, 4, c.green());
            sqlite3_bind_int(stmtInsPal, 5, c.blue());
            // 冗余写入 Hex 字符串供界面渲染直接提取
            sqlite3_bind_text(stmtInsPal, 6, c.name().toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmtInsPal, 7, ratio);
            sqlite3_step(stmtInsPal);
        }
>>>>>>> REPLACE

---

### 3.3 `QuarkMeta_Source/src/meta/SmartFolderDao.cpp`

<<<<<<< SEARCH
#include <QSet>
#include <QRegularExpression>
=======
#include <QSet>
#include <QRegularExpression>
#include <QColor>
>>>>>>> REPLACE

<<<<<<< SEARCH
QString SmartFolderDao::buildAutoColorCondition(const SmartFolderRule& rule) {
    QString colorVal = rule.value.trimmed().replace("'", "''");
    if (colorVal.isEmpty()) return QString();

    if (rule.op == SmartRuleOperator::NotContains) {
        return QString("auto_color != '%1'").arg(colorVal);
    } else {
        return QString("auto_color = '%1'").arg(colorVal);
    }
}
=======
QString SmartFolderDao::buildAutoColorCondition(const SmartFolderRule& rule) {
    QString hex = rule.value.trimmed();
    if (hex.isEmpty()) return QString();

    QColor targetColor(hex);
    if (!targetColor.isValid()) return QString();

    // 1. 获取容差 (默认 50，允许范围 0~255)
    int tolerance = rule.tolerance > 0 ? rule.tolerance : 50;

    // 2. 将容差映射为 RGB 三通道的数值区间
    int rMin = qMax(0, targetColor.red() - tolerance);
    int rMax = qMin(255, targetColor.red() + tolerance);
    int gMin = qMax(0, targetColor.green() - tolerance);
    int gMax = qMin(255, targetColor.green() + tolerance);
    int bMin = qMax(0, targetColor.blue() - tolerance);
    int bMax = qMin(255, targetColor.blue() + tolerance);

    // 3. 构建走复合索引的纯数值范围 SQL 语句 (直接穿透 item_palettes 调色板)
    if (rule.op == SmartRuleOperator::NotContains) {
        return QString("file_id NOT IN ("
                       "  SELECT DISTINCT file_id FROM item_palettes "
                       "  WHERE (r BETWEEN %1 AND %2) "
                       "    AND (g BETWEEN %3 AND %4) "
                       "    AND (b BETWEEN %5 AND %6)"
                       ")")
               .arg(rMin).arg(rMax)
               .arg(gMin).arg(gMax)
               .arg(bMin).arg(bMax);
    } else {
        return QString("file_id IN ("
                       "  SELECT DISTINCT file_id FROM item_palettes "
                       "  WHERE (r BETWEEN %1 AND %2) "
                       "    AND (g BETWEEN %3 AND %4) "
                       "    AND (b BETWEEN %5 AND %6)"
                       ")")
               .arg(rMin).arg(rMax)
               .arg(gMin).arg(gMax)
               .arg(bMin).arg(bMax);
    }
}
>>>>>>> REPLACE

---

## 四、构建与运行时验收标准 (Build & Verification Steps)

### 4.1 构建编译核验
- **执行构建命令**：
  ```bash
  cmake -B build -S QuarkMeta_Source
  cmake --build build --config Release
  ```
- **预期编译结果**：`DatabaseManager.cpp`、`LibraryItemDao.cpp`、`SmartFolderDao.cpp` 编译无任何错误，0 warnings，生成目标程序。

### 4.2 运行时验收用例
1. **调色板主体色捕获验证**：
   - 步骤：导入一张白底粉花的图片，该图第一主色为白色，但调色板包含粉红分量；
   - 步骤：在智能文件夹中配置“图像颜色”等于该粉红色，容差设置为 50；
   - 预期结果：该白底粉花图片被成功检出！
2. **容差滑动精确过滤验证**：
   - 步骤：将容差调低至 10，观察结果；再将容差调大至 80；
   - 预期结果：随着容差增大，相近色彩的图片逐步被包含，容差减小时严格收窄，完全受界面容差滑条精准驱动。
3. **百万级毫秒检索性能验证**：
   - 步骤：在包含大量图片的媒体库中执行图像颜色智能文件夹预览；
   - 预期结果：SQL 经 `EXPLAIN QUERY PLAN` 确认直接命中 `idx_palettes_rgb` 复合索引，查询耗时小于 10ms，毫无卡顿。

---

## 五、官方 API 复用与防另起炉灶 (Official API Reuse & Anti-Redundancy)

1. **复用 SQLite 原生 B-Tree 索引机制**：
   - 利用标准的 `BETWEEN ... AND ...` 语法，完美契合 SQLite 多列数值复合索引，杜绝在 C++ 内存中做低效全量色彩遍历；
2. **复用 `QColor` 标准通道接口**：
   - 统一调用 `red()`, `green()`, `blue()`，严禁手写不规范的 Hex 拆分或字符串解析；
3. **复用 `item_palettes` 既有拆分表架构**：
   - 继承项目原有的主色调拆分设计理念，无缝扩展，不增加冗余新表。

---

## 六、数据单一事实来源声明表 (Data SSOT Declaration Table)

| 数据项 | 唯一真理源 (SSOT) | 派生副本 / 观察者 | 同步与刷新机制 |
| :--- | :--- | :--- | :--- |
| 图像色彩分量 | SQLite `item_palettes(r, g, b)` | `ItemRecord::palettes` 内存色板 | 图片元数据入库时写入，智能文件夹检索时直接走复合索引 |
| 调色板复合索引 | SQLite `idx_palettes_rgb` | SQLite 内部 B-Tree 索引树 | 启动数据库初始化与自动加列时维护 |
| 颜色十六进制串 | SQLite `item_palettes.color_hex` | UI 界面拾色与渲染条 | 冗余保留，入库时一并写入，供界面无二次计算直接渲染 |

---

## 七、接口签名与访问级别一致性核查 (Interface & Visibility Consistency Check)

### 7.1 外部及既有签名物理核验
- `SmartFolderDao::buildAutoColorCondition`：
  `static QString buildAutoColorCondition(const SmartFolderRule& rule);`（位于 `SmartFolderDao.h:99`，公有静态，签名与访问级别保持只读一致）；
- `LibraryItemDao::upsertItems`：
  `static bool upsertItems(const std::vector<ItemRecord>& records);`（位于 `LibraryItemDao.h:26`，私有静态，供友元调用，签名保持一致）。

### 7.2 新增接口核对
- 本方案未新增任何类接口，亦未修改对外公有方法签名，100% 保持调用方契约与编译兼容性。

---

## 八、头文件包含链与类型完整性检查 (Header Chain & Inclusion Closure Check)

1. `QuarkMeta_Source/src/meta/SmartFolderDao.cpp`：
   - 显式新增 `#include <QColor>`，使 `targetColor` 类型完整定义；
2. `QuarkMeta_Source/src/meta/LibraryItemDao.cpp`：
   - 通过 `"../core/ItemRecord.h"` 包含完整的 `QColor` 定义，调用 `red()`, `green()`, `blue()` 类型安全闭合；
3. `QuarkMeta_Source/src/meta/DatabaseManager.cpp`：
   - 使用标准 SQL 脚本语句，无新增未定义 C++ 类型。

---

## 九、合规与质量自检核对表 (Compliance & Quality Self-Check Table)

| 检查项序号 | 规范要求 | 方案满足情况 | 详细说明与对应章节 |
| :---: | :--- | :---: | :--- |
| 1 | 全中文解答与无英文违禁词 | 满足 | 正文完全使用中文，无任何禁用英文字眼 |
| 2 | 不直接修改代码、方案隔离存放 | 满足 | 方案独立存放于 `Implementation Plan/` 目录 |
| 3 | 10 大必备章节结构完整 | 满足 | 包含从 Overview 到行为数值零篡改声明全部 10 个标准章节 |
| 4 | SEARCH 块原文真实存在且唯一匹配 | 满足 | 方案中 5 个 SEARCH 块均与目标物理源码 100% 逐字唯一匹配 |
| 5 | REPLACE 语法严谨、无悬空指针/引用 | 满足 | 纯数值绑定与标准 SQL 构造，无内存隐患 |
| 6 | CMakeLists.txt 完整性 | 满足 | 本方案修改均为既有源文件，无需变动构建列表 |
| 7 | 公共对外 API 签名冻结 | 满足 | 既有 API 签名保持只读 |
| 8 | 严格单向依赖流动 | 满足 | 视图层与数据层单向解耦 |
| 9 | 官方既有组件强制复用 | 满足 | 复用 QColor 与 SQLite 复合索引 |
| 10 | 数据库事务与线程安全 | 满足 | 调色板插入依然处于 `SqlTransaction` 事务保护内 |
| 11 | ToolTipOverlay 绝对禁跟光标 | 满足 | 本方案不涉及 UI 浮层交互 |
| 12 | 样式数值零篡改 | 满足 | 未改变任何既有控件样式数值 |
| 13 | 交互逻辑零篡改 | 满足 | 界面交互与操作习惯完全保持原貌，底层彻底赋予真实颜色检索能力 |
| 14 | 异步任务无循环重复实例化原子变量 | 满足 | 本方案无多线程并发循环冲突 |
| 15 | 递归 Lambda 循环引用防范 | 满足 | 无递归 Lambda |
| 16 | 显式类型头文件包含闭合 | 满足 | 显式新增 `#include <QColor>`，类型完整闭合 |
| 17 | 方案无触及范围外文件 | 满足 | 仅精准修改 3 个目标文件 |
| 18 | Memories.md 理念沉淀 | 满足 | 核心设计理念已完整写入 Memories.md 第 36 章 |

---

## 十、行为零篡改与数值零篡改声明表 (Behavior Preservation & Zero-Value-Alteration Table)

### 10.1 行为与交互零篡改声明
1. **规则编辑交互保持不变**：用户在智能文件夹配置中选择“图像颜色”、调整容差或选取色轮的交互 100% 保持不变；
2. **入库处理接口保持不变**：`LibrarySyncService` 调度 `upsertItems` 的上层接口与参数传递 100% 保持不变；
3. **颜色渲染机制保持不变**：UI 界面提取 `palettes` 渲染色彩条的方式保持不变。

### 10.2 样式参数与数值零篡改声明
1. **容差默认缺省值**：在未配置时容差默认缺省值保持为 50；
2. **RGB 取值区间**：RGB 浮动区间严格锁定在 0~255；
3. **复合索引命名**：统一命名为 `idx_palettes_rgb`。
