#include "DiskTrashService.h"
#include "../meta/DatabaseManager.h"
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QUuid>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

bool DiskTrashService::moveToDiskTrash(const QStringList& paths) {
    if (paths.isEmpty()) return true;

    bool allOk = true;
    for (const QString& p : paths) {
        QFileInfo info(p);
        QString drive = info.absolutePath().left(3); // e.g. "C:/"
        QString trashDir = drive + ".QuarkMeta/disk_trash";

#ifdef Q_OS_WIN
        QDir().mkpath(trashDir);
        // 确保 .QuarkMeta 目录隐藏
        SetFileAttributesW((drive + ".QuarkMeta").toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
#else
        QDir().mkpath(trashDir);
#endif

        // 🚨 关键修复：在物理移动前提前抓取原文件的各项属性，避免移动后 p 不存在导致 info.isDir() 错判为 0！
        bool isFolder = info.isDir();
        qint64 fileSize = info.size();
        qint64 createdAt = info.birthTime().isValid() ? info.birthTime().toMSecsSinceEpoch() : info.lastModified().toMSecsSinceEpoch();

        QString fileId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString itemContainerDir = trashDir + "/" + fileId;
        QDir().mkpath(itemContainerDir);

        QString dest = itemContainerDir + "/" + info.fileName();

        // 1. 物理同盘位移 (原名直接移动至 FILE_ID 隔离盒)
        if (QFile::rename(p, dest)) {
            sqlite3* db = DatabaseManager::instance().getGlobalDb();
            if (!db) {
                allOk = false;
                continue;
            }

            SqlTransaction trans(db);
            sqlite3_stmt* stmt = nullptr;
            const char* sql = "INSERT INTO disk_trash (file_id, trash_path, original_path, drive_letter, file_name, is_folder, file_size, created_at, deleted_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
            
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                QString driveLetter = drive.left(1).toUpper();
                sqlite3_bind_text16(stmt, 1, fileId.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text16(stmt, 2, dest.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text16(stmt, 3, p.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text16(stmt, 4, driveLetter.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text16(stmt, 5, info.fileName().toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt, 6, isFolder ? 1 : 0);
                sqlite3_bind_int64(stmt, 7, fileSize);
                sqlite3_bind_int64(stmt, 8, createdAt);
                sqlite3_bind_int64(stmt, 9, QDateTime::currentMSecsSinceEpoch());

                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    trans.commit();
                    DatabaseManager::instance().setDirty(true);
                } else {
                    qWarning() << "[DiskTrashService] Failed to insert disk_trash row:" << sqlite3_errmsg(db);
                    allOk = false;
                }
                sqlite3_finalize(stmt);
            } else {
                qWarning() << "[DiskTrashService] Failed to prepare insert stmt:" << sqlite3_errmsg(db);
                allOk = false;
            }
        } else {
            qWarning() << "[DiskTrashService] Physical move to trash failed for path:" << p;
            allOk = false;
        }
    }
    return allOk;
}

bool DiskTrashService::restoreFromDiskTrash(int id, const QString& trashPath) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    QString originalPath;
    qint64 trashCreatedAt = 0;
    sqlite3_stmt* stmt = nullptr;
    const char* sqlSel = "SELECT original_path, created_at FROM disk_trash WHERE id = ?";
    if (sqlite3_prepare_v2(db, sqlSel, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const wchar_t* wOrig = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 0));
            if (wOrig) {
                originalPath = QString::fromWCharArray(wOrig);
            }
            trashCreatedAt = sqlite3_column_int64(stmt, 1);
        }
        sqlite3_finalize(stmt);
    }

    if (originalPath.isEmpty()) {
        qWarning() << "[DiskTrashService] Failed to find original path for trash id:" << id;
        return false;
    }

    // 自动创建目标父目录
    QDir().mkpath(QFileInfo(originalPath).absolutePath());

    // 检查目标位置是否存在同名文件/文件夹冲突，基于创建时间权威与连字符 -N 递增避让
    QString targetPath = originalPath;
    if (QFile::exists(originalPath)) {
        QFileInfo existingInfo(originalPath);
        qint64 diskCreatedAt = existingInfo.birthTime().isValid() ? existingInfo.birthTime().toMSecsSinceEpoch() : existingInfo.lastModified().toMSecsSinceEpoch();

        if (trashCreatedAt < diskCreatedAt) {
            // 被还原的项目创建时间更早：占用原名 originalPath，将磁盘现有项目自动重命名为 A-1.ext
            QString baseDir = existingInfo.absolutePath();
            QString baseName = existingInfo.completeBaseName();
            QString suffix = existingInfo.suffix();
            QString newDiskPath;
            int counter = 1;
            do {
                QString candidateName = suffix.isEmpty() ? QString("%1-%2").arg(baseName).arg(counter) : QString("%1-%2.%3").arg(baseName).arg(counter).arg(suffix);
                newDiskPath = baseDir + "/" + candidateName;
                counter++;
            } while (QFile::exists(newDiskPath));

            QFile::rename(originalPath, newDiskPath);
            targetPath = originalPath;
        } else {
            // 磁盘项目更早或等于：磁盘项目保留原名，被还原的项目重命名为 A-1.ext 还原移出
            QFileInfo trashInfo(originalPath);
            QString baseDir = trashInfo.absolutePath();
            QString baseName = trashInfo.completeBaseName();
            QString suffix = trashInfo.suffix();
            int counter = 1;
            do {
                QString candidateName = suffix.isEmpty() ? QString("%1-%2").arg(baseName).arg(counter) : QString("%1-%2.%3").arg(baseName).arg(counter).arg(suffix);
                targetPath = baseDir + "/" + candidateName;
                counter++;
            } while (QFile::exists(targetPath));
        }
    }

    if (QFile::rename(trashPath, targetPath)) {
        // 清理空 FILE_ID 隔离盒目录
        QDir(QFileInfo(trashPath).absolutePath()).removeRecursively();

        SqlTransaction trans(db);
        sqlite3_stmt* delStmt = nullptr;
        const char* sqlDel = "DELETE FROM disk_trash WHERE id = ?";
        bool success = false;
        if (sqlite3_prepare_v2(db, sqlDel, -1, &delStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(delStmt, 1, id);
            if (sqlite3_step(delStmt) == SQLITE_DONE) {
                trans.commit();
                DatabaseManager::instance().setDirty(true);
                success = true;
            }
            sqlite3_finalize(delStmt);
        }
        if (success) return true;
    } else {
        qWarning() << "[DiskTrashService] Failed to physically move back trash item to target path:" << targetPath;
    }

    return false;
}

bool DiskTrashService::restoreToDirectory(const QString& trashPath, const QString& targetDir) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    QFileInfo info(trashPath);
    QString dest = QDir(targetDir).filePath(info.fileName());
    QDir().mkpath(targetDir);

    bool moved = QFile::rename(trashPath, dest);
    if (!moved) {
        if (info.isDir()) {
            moved = QFile::copy(trashPath, dest);
            if (moved) QDir(trashPath).removeRecursively();
        } else {
            moved = QFile::copy(trashPath, dest) && QFile::remove(trashPath);
        }
    }

    if (moved) {
        // 清理空 FILE_ID 隔离盒目录
        QDir(QFileInfo(trashPath).absolutePath()).removeRecursively();

        SqlTransaction trans(db);
        sqlite3_stmt* delStmt = nullptr;
        const char* sqlDel = "DELETE FROM disk_trash WHERE trash_path = ?";
        if (sqlite3_prepare_v2(db, sqlDel, -1, &delStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text16(delStmt, 1, trashPath.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(delStmt) == SQLITE_DONE) {
                trans.commit();
                DatabaseManager::instance().setDirty(true);
            }
            sqlite3_finalize(delStmt);
        }
        return true;
    }
    return false;
}

bool DiskTrashService::permanentlyDeleteDiskTrash(int id, const QString& trashPath) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    // 物理彻底删除 (如果是文件夹则递归删除)
    QFileInfo info(trashPath);
    bool physicalOk = false;
    if (info.exists()) {
        if (info.isDir()) {
            physicalOk = QDir(trashPath).removeRecursively();
        } else {
            physicalOk = QFile::remove(trashPath);
        }
    } else {
        physicalOk = true; // 文件在物理上已经不存在，直接认为成功，允许清洗数据库
    }

    if (physicalOk) {
        // 清理空 FILE_ID 隔离盒目录
        QDir containerDir = QFileInfo(trashPath).absoluteDir();
        if (containerDir.exists()) {
            containerDir.removeRecursively();
        }

        SqlTransaction trans(db);
        sqlite3_stmt* delStmt = nullptr;
        const char* sqlDel = "DELETE FROM disk_trash WHERE id = ?";
        bool success = false;
        if (sqlite3_prepare_v2(db, sqlDel, -1, &delStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(delStmt, 1, id);
            if (sqlite3_step(delStmt) == SQLITE_DONE) {
                trans.commit();
                DatabaseManager::instance().setDirty(true);
                success = true;
            }
            sqlite3_finalize(delStmt);
        }
        if (success) return true;
    }

    return false;
}

bool DiskTrashService::restoreAllDiskTrash() {
    bool allOk = true;
    std::vector<sqlite3*> dbs = { DatabaseManager::instance().getGlobalDb() };
    
    for (sqlite3* db : dbs) {
        struct TrashItem {
            int id;
            QString trashPath;
        };
        std::vector<TrashItem> items;

        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT id, trash_path FROM disk_trash";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                const wchar_t* wPath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
                if (wPath) {
                    items.push_back({id, QString::fromWCharArray(wPath)});
                }
            }
            sqlite3_finalize(stmt);
        }

        for (const auto& item : items) {
            if (!restoreFromDiskTrash(item.id, item.trashPath)) {
                allOk = false;
            }
        }
    }
    return allOk;
}

bool DiskTrashService::emptyDiskTrash() {
    bool allOk = true;
    std::vector<sqlite3*> dbs = { DatabaseManager::instance().getGlobalDb() };

    for (sqlite3* db : dbs) {
        struct TrashItem {
            int id;
            QString trashPath;
        };
        std::vector<TrashItem> items;

        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT id, trash_path FROM disk_trash";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                const wchar_t* wPath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
                if (wPath) {
                    items.push_back({id, QString::fromWCharArray(wPath)});
                }
            }
            sqlite3_finalize(stmt);
        }

        for (const auto& item : items) {
            if (!permanentlyDeleteDiskTrash(item.id, item.trashPath)) {
                allOk = false;
            }
        }
    }
    return allOk;
}

} // namespace QuarkMeta
