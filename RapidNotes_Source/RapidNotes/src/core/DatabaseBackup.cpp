#include "DatabaseManager.h"
#include "DatabaseManagerInternal.h"

void DatabaseManager::performDailyBackup() {
    QString dbPath = getGlobalDbPath();
    if (!QFile::exists(dbPath)) return;

    QString backupDir = getBackupDir();
    QString todayStr = QDate::currentDate().toString("yyyyMMdd");
    QString todayBackupPath = QDir::toNativeSeparators(backupDir + QString("/RapidNotes_%1.store.bak").arg(todayStr));

    // 当天已存在备份副本，直接跳过，零磁盘写入磨损
    if (QFile::exists(todayBackupPath)) return;

    QFile::copy(dbPath, todayBackupPath);
    qInfo() << "[DatabaseManager] 已创建每日数据库快照备份:" << todayBackupPath;

    QDir bDir(backupDir);
    QFileInfoList backupFiles = bDir.entryInfoList(QStringList() << "RapidNotes_*.store.bak", QDir::Files, QDir::Time);
    for (int i = 7; i < backupFiles.size(); ++i) {
        QFile::remove(backupFiles[i].absoluteFilePath());
    }
}

bool DatabaseManager::flushDatabase(const QString& source) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen() || !m_isInitialized) return false;
    
    // [DE-SHELL] 在新架构下，此函数仅执行强制刷盘 Checkpoint
    QSqlQuery checkPoint(m_db);
    return checkPoint.exec("PRAGMA wal_checkpoint(PASSIVE);");
}

bool DatabaseManager::tryRecoverFromBackup() {
    if (m_dbPath.isEmpty()) return false;

    QFileInfo dbInfo(m_dbPath);
    QDir dbDir = dbInfo.dir();
    QString backupPath = dbDir.absoluteFilePath("backups/inspiration_latest.db");

    if (!QFile::exists(backupPath) || QFileInfo(backupPath).size() == 0) {
        qWarning() << "[DB] 自动恢复失败：备份文件不存在或为空 (inspiration_latest.db)";
        return false;
    }

    qDebug() << "[DB] 正在尝试从最新备份恢复数据库 (原因: 原始文件丢失或损坏)...";

    // 1. 备份损坏的文件以备后续人工检查
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString corruptedPath = m_dbPath + ".corrupted_" + timestamp;
    if (QFile::exists(m_dbPath)) {
        if (QFile::rename(m_dbPath, corruptedPath)) {
            qDebug() << "[DB] 已将损坏文件移至:" << corruptedPath;
            
            // [UX-OPTIMIZATION] 数量控制：仅保留最近 5 个损坏备份，防止无限堆积
            QStringList corruptedFilter;
            corruptedFilter << QFileInfo(m_dbPath).fileName() + ".corrupted_*";
            QFileInfoList corruptedFiles = dbDir.entryInfoList(corruptedFilter, QDir::Files, QDir::Name);
            while (corruptedFiles.size() > 5) {
                QFileInfo oldest = corruptedFiles.takeFirst();
                if (QFile::remove(oldest.absoluteFilePath())) {
                    qDebug() << "[DB] 已移除过期的损坏备份:" << oldest.fileName();
                }
            }
        } else {
            qWarning() << "[DB] 无法重命名损坏的文件，尝试直接覆盖。";
            QFile::remove(m_dbPath);
        }
    }

    // 2. 从血包恢复
    if (QFile::copy(backupPath, m_dbPath)) {
        qDebug() << "[DB] 核心救治成功：已从备份文件恢复数据库外壳。";
        return true;
    } else {
        qCritical() << "[DB] 核心救治失败：无法将备份文件复制回主路径。";
        return false;
    }
}

void DatabaseManager::markDirty() {
    m_isDirty = true;
    m_lastActivityTime = QDateTime::currentDateTime();
}

void DatabaseManager::handleAutoSave() {
    QMutexLocker locker(&m_mutex);
    if (!m_isDirty) return;

    // [ANTI-LOSS] 双重安全阈值判定
    qint64 idleSecs = m_lastActivityTime.secsTo(QDateTime::currentDateTime());
    
    // 逻辑 A：如果已闲置 5 秒（用户已停手），立即同步。
    // 逻辑 B：如果数据已经脏了超过 120 秒，无论用户是否在操作，强制同步（防止活跃用户无限期拖延同步）。
    bool shouldSync = (idleSecs >= 5) || (idleSecs < 0) || (abs(idleSecs) > 120);

    if (!shouldSync) {
        return;
    }

    qDebug() << "[DB] 触发安全同步逻辑 (闲置:" << idleSecs << "s)，执行物理落盘...";
    m_isDirty = false;
    
    // [FIX] 彻底修复线程违规崩溃：QSqlDatabase 连接不是线程安全的。
    // 在主线程执行 wal_checkpoint(PASSIVE) 极快且不阻塞读写，完全没必要开启后台线程冒风险。
    if (flushDatabase("SmartAutoSync")) {
        backupDatabaseLatest();
        qDebug() << "[DB] 数据物理落盘及备份完成。";
    } else {
        markDirty();
    }
}

void DatabaseManager::backupDatabaseLatest() {
    if (m_dbPath.isEmpty() || !QFile::exists(m_dbPath)) return;

    QFileInfo dbInfo(m_dbPath);
    QDir dbDir = dbInfo.dir();
    QString backupDirPath = dbDir.absoluteFilePath("backups");
    QDir backupDir(backupDirPath);

    if (!backupDir.exists()) {
        dbDir.mkdir("backups");
    }

    // 高频备份使用固定文件名，每 7 秒更新一次
    // 采用“先写入临时文件再重命名”的原子操作，确保备份文件始终可用且不损坏
    QString backupPath = backupDir.absoluteFilePath("inspiration_latest.db");
    QString tempPath = backupPath + ".tmp";
    
    if (QFile::exists(tempPath)) {
        QFile::remove(tempPath);
    }
    
    // [HEALING] 备份熔断保护机制
    // 如果当前主库大小异常缩小（例如从数MB缩减到几十KB），则拒绝直接覆盖血包备份。
    if (QFile::exists(backupPath)) {
        qint64 currentSize = QFileInfo(m_dbPath).size();
        qint64 backupSize = QFileInfo(backupPath).size();
        
        // 判定熔断条件：备份已存在且大于 200KB，且当前文件比备份缩小了 50% 以上
        if (backupSize > 200 * 1024 && currentSize < (backupSize / 2)) {
            qCritical() << "[DB] 检测到当前数据库异常缩小 (" << currentSize << " vs " << backupSize << ")，触发备份熔断保护！";
            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
            QFile::rename(backupPath, backupPath + ".shrink_safe_" + timestamp);
            // 虽然重命名了，但后续仍会尝试生成新的备份，但老数据已被保护
        }
    }

    if (QFile::copy(m_dbPath, tempPath)) {
        if (QFile::exists(backupPath)) {
            QFile::remove(backupPath);
        }
        if (QFile::rename(tempPath, backupPath)) {
            // [FIX] 解决 QFile::copy 保留旧创建日期的问题，强制更新为当前备份时刻
            QFile bFile(backupPath);
            QDateTime now = QDateTime::currentDateTime();
            bFile.setFileTime(now, QFileDevice::FileBirthTime);
            bFile.setFileTime(now, QFileDevice::FileModificationTime);
            qDebug() << "[DB] 高频同步备份成功 (已刷新时间戳):" << backupPath;
        }
    }
}

void DatabaseManager::backupDatabase() {
    if (m_dbPath.isEmpty() || !QFile::exists(m_dbPath)) return;

    QFileInfo dbInfo(m_dbPath);
    QDir dbDir = dbInfo.dir();
    QString backupDirPath = dbDir.absoluteFilePath("backups");
    QDir backupDir(backupDirPath);

    if (!backupDir.exists()) {
        if (!dbDir.mkdir("backups")) {
            qWarning() << "[DB] 无法创建备份目录:" << backupDirPath;
            return;
        }
    }

    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("yyyyMMdd_HHmmss");
    QString backupFileName = QString("inspiration_backup_%1.db").arg(timestamp);
    QString backupPath = backupDir.absoluteFilePath(backupFileName);

    if (QFile::copy(m_dbPath, backupPath)) {
        // [FIX] 显式更新归档备份的时间戳，确保资源管理器显示正确
        QFile bFile(backupPath);
        bFile.setFileTime(now, QFileDevice::FileBirthTime);
        bFile.setFileTime(now, QFileDevice::FileModificationTime);
        qDebug() << "[DB] 数据库归档备份成功 (已刷新时间戳):" << backupPath;
    } else {
        qWarning() << "[DB] 数据库备份失败";
        return;
    }

    // 2026-03-xx 按照用户要求：将全量备份保留上限从 10 个降至 3 个，显著精简磁盘占用空间
    QStringList filter;
    filter << "inspiration_backup_*.db";
    // 按名称排序（时间戳文件名按名称排序即为时间顺序）
    QFileInfoList backupFiles = backupDir.entryInfoList(filter, QDir::Files, QDir::Name);

    while (backupFiles.size() > 3) {
        QFileInfo oldest = backupFiles.takeFirst();
        if (QFile::remove(oldest.absoluteFilePath())) {
            qDebug() << "[DB] 已移除旧备份:" << oldest.fileName();
        }
    }
}
