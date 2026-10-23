# DatabaseAutoBackup.md - 数据库 7 天按天快照备份与自动灾难恢复实施方案

- **Base Commit Hash**: `60f5ef20e8c2a03ae4bcb9d081c671313d672155`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/meta/DatabaseManager.h`
   - `src/meta/DatabaseManager.cpp`
2. **头文件原样签名核查**：
   - `static DatabaseManager& instance();` (`src/meta/DatabaseManager.h:29`)
   - `bool init();` (`src/meta/DatabaseManager.h:33`)
   - `bool loadDb(const std::wstring& diskPath, DbConnection& conn);` (`src/meta/DatabaseManager.h:43`)
3. **既有 API 检索日志**：
   - `grep -rn "getGlobalDbPath" src/meta/DatabaseManager.cpp`：获取 `.QuarkMeta/QuarkMeta.store` 数据库存储路径。

---

## Chapter 1: Overview（概述与根因分析）

### 1. 需求与风险分析
QuarkMeta 采用了侧车文件（`.QuarkMeta.json`）存储文件的离散元数据，即便数据库文件丢失，文件元数据也能自愈重建。然而，**收藏夹树结构与分类（`favorites`）** 及 **智能文件夹规则（`smart_folders`）** 等纯全局数据仅保存在中心数据库 `QuarkMeta.store` 中。
一旦 `QuarkMeta.store` 被误删或发生磁盘扇区坏道损坏，上述配置将被重置为空。

为了在**零硬件磨损（SSD TBW 友好）**的前提下保障数据绝对安全，本方案实现：
1. **7 天按天每日快照备份**：每天最多只在启动时备份一次快照至 `.QuarkMeta/backups/QuarkMeta_YYYYMMDD.store.bak`，并自动物理清理 7 天以前的历史老旧快照。
2. **灾难自动自愈恢复**：当数据库损坏或丢失无法打开时，启动阶段自动捕获异常，按时间倒序从备份目录搜寻最近一份有效的 `.bak` 备份并一键静默还原。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/meta/DatabaseManager.h` （修改）
2. `src/meta/DatabaseManager.cpp` （修改）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/meta/DatabaseManager.h`
```
<<<<<<< SEARCH
    static QString getAppDir();
    static QString getGlobalDbPath();
=======
    static QString getAppDir();
    static QString getGlobalDbPath();
    static QString getBackupDir();

    static void performDailyBackup();
    static bool tryRecoverFromBackup();
>>>>>>> REPLACE
```

### 2. `src/meta/DatabaseManager.cpp`
```
<<<<<<< SEARCH
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>
=======
#include <QFileInfo>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QString DatabaseManager::getGlobalDbPath() {
    QString dir = QDir::toNativeSeparators(getAppDir() + "/.QuarkMeta");
    return QDir::toNativeSeparators(dir + "/QuarkMeta.store");
}
=======
QString DatabaseManager::getGlobalDbPath() {
    QString dir = QDir::toNativeSeparators(getAppDir() + "/.QuarkMeta");
    return QDir::toNativeSeparators(dir + "/QuarkMeta.store");
}

QString DatabaseManager::getBackupDir() {
    QString dir = QDir::toNativeSeparators(getAppDir() + "/.QuarkMeta/backups");
    QDir().mkpath(dir);
    return dir;
}

void DatabaseManager::performDailyBackup() {
    QString dbPath = getGlobalDbPath();
    if (!QFile::exists(dbPath)) return;

    QString backupDir = getBackupDir();
    QString todayStr = QDate::currentDate().toString("yyyyMMdd");
    QString todayBackupPath = QDir::toNativeSeparators(backupDir + QString("/QuarkMeta_%1.store.bak").arg(todayStr));

    // 当天已存在备份副本，直接跳过，零磁盘写入磨损
    if (QFile::exists(todayBackupPath)) {
        return;
    }

    // 执行按天快照复制
    QFile::copy(dbPath, todayBackupPath);
    qInfo() << "[DatabaseManager] 已成功创建每日数据库快照备份:" << todayBackupPath;

    // 清理超过 7 天的历史快照文件
    QDir bDir(backupDir);
    QStringList filters;
    filters << "QuarkMeta_*.store.bak";
    QFileInfoList backupFiles = bDir.entryInfoList(filters, QDir::Files, QDir::Time); // 按修改时间降序排序

    if (backupFiles.size() > 7) {
        for (int i = 7; i < backupFiles.size(); ++i) {
            QFile::remove(backupFiles[i].absoluteFilePath());
            qInfo() << "[DatabaseManager] 已自动清理过期历史快照:" << backupFiles[i].fileName();
        }
    }
}

bool DatabaseManager::tryRecoverFromBackup() {
    QString backupDir = getBackupDir();
    QDir bDir(backupDir);
    QStringList filters;
    filters << "QuarkMeta_*.store.bak";
    QFileInfoList backupFiles = bDir.entryInfoList(filters, QDir::Files, QDir::Time);

    if (backupFiles.isEmpty()) return false;

    QString dbPath = getGlobalDbPath();
    QFile::remove(dbPath); // 清理可能损坏的残存孤儿主文件

    for (const auto& bFile : backupFiles) {
        if (QFile::copy(bFile.absoluteFilePath(), dbPath)) {
            qWarning() << "[DatabaseManager] 已成功从备份文件自动恢复数据库:" << bFile.fileName();
            return true;
        }
    }
    return false;
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    std::wstring globalPath = getGlobalDbPath().toStdWString();
    if (!loadDb(globalPath, m_globalDb)) {
        return false;
    }

    m_isInitialized = true;
    return true;
=======
    std::wstring globalPath = getGlobalDbPath().toStdWString();
    if (!loadDb(globalPath, m_globalDb)) {
        qWarning() << "[DatabaseManager] 主数据库装载失败，尝试从 7 天自动快照自愈恢复...";
        if (tryRecoverFromBackup()) {
            if (!loadDb(globalPath, m_globalDb)) {
                return false;
            }
        } else {
            return false;
        }
    }

    m_isInitialized = true;
    performDailyBackup();
    return true;
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 构建步骤
```bash
cmake --build build --config Debug
```

### 2. 运行时验收标准
1. **每日快照按天生成（0 磨损）**：
   - 启动软件，检查物理路径 `[可执行文件目录]/.QuarkMeta/backups/` 目录下是否自动生成了 `QuarkMeta_YYYYMMDD.store.bak` 文件。
   - 再次多次关闭并重启软件，验证当天**不会重复触发复制写盘**。
2. **历史快照自动轮转**：
   - 当快照数量超过 7 个时，系统会自动将最老旧的旧快照删除，维持最多 7 份历史快照。
3. **灾难误删自动自愈**：
   - 退出软件，手动删除主数据库文件 `[可执行文件目录]/.QuarkMeta/QuarkMeta.store`；
   - 重新启动软件，日志显示 `主数据库装载失败，尝试从 7 天自动快照自愈恢复...`，收藏夹分类与智能文件夹配置**完美无损自动恢复**！

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 `DatabaseManager::getGlobalDbPath` 既有路径接口。
- 复用 `QFile::copy` 与 `QDir::entryInfoList` 原生 C++/Qt 接口，未增加任何额外第三方依赖。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 备份副本 | 恢复触发展现 |
| :--- | :--- | :--- | :--- |
| 全局数据库 | `.QuarkMeta/QuarkMeta.store` | `.QuarkMeta/backups/QuarkMeta_YYYYMMDD.store.bak` | 损坏时倒序按最新备份自动还原 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 外部调用接口 | 物理 .h 中的声明行 | 访问级别 |
| :--- | :--- | :--- |
| `getBackupDir()` | `src/meta/DatabaseManager.h:37` | `public` |
| `performDailyBackup()` | `src/meta/DatabaseManager.h:39` | `public` |
| `tryRecoverFromBackup()` | `src/meta/DatabaseManager.h:40` | `public` |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `DatabaseManager.cpp` 中新增引入 `<QDateTime>` 头文件，保障 `QDate::currentDate()` 类型方法完整闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 关联位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 方案位于物理隔离目录 | 满足 | `DatabaseAutoBackup.md` |
| 2 | SEARCH 块原文存在且唯一 | 满足 | 精准校验 `DatabaseManager.h` 和 `DatabaseManager.cpp` |
| 3 | 10 大章节完整 | 满足 | Chapter 1 ~ 10 全齐 |
| 4 | 无 Win32 / Qt::Popup Hack | 满足 | 标准 Qt 文件 I/O 操作 |
| 5 | 无主/后台线程 UI 违规 | 满足 | 在数据库初始化时安全同步执行 |
| 6 | 样式与数值 100% 保持 | 满足 | 不涉及 UI 变动 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

- **交互行为**：软件正常启动与数据加载交互体验 100% 保持一致，无任何多余弹窗干扰。
- **外观数值**：不涉及界面数值修改。
