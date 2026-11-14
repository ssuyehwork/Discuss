#pragma once
// DatabaseManager 各实现文件共用的内部头文件（不对外暴露）
#include "DatabaseManager.h"
#include "ItemType.h"
#include "WordBucket.h"
#include "TagCodec.h"
#include <QDebug>
#include <QSqlRecord>
#include <QSqlError>
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QSet>
#include <QRegularExpression>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent>
#include <QThreadPool>
#include <utility>
#include <algorithm>
#include "FileCryptoHelper.h"
#include "HardwareInfoHelper.h"
#include "ClipboardMonitor.h"
#include "../meta/SmartFolderDao.h"
#include "../meta/DriveMetaDao.h"
#include "../util/AppDirectoryInitializer.h"
#include <functional>
#ifdef Q_OS_WIN
#include <windows.h>
#endif


namespace DbInternal {
    // 2026-04-09 按照用户要求：采用纯动态后缀解析架构，彻底废弃并移除所有静态白/黑名单
    QString extractFileExtensions(const QString& itemType, const QString& content) {
        RapidNotes::ItemType::Type type = RapidNotes::ItemType::fromString(itemType);
        if (type == RapidNotes::ItemType::Type::Link) return "Link";
        
        // 仅处理历史5种文件/文件夹相关类型 (file, local_file, local_batch, folder, local_folder)
        if (!RapidNotes::ItemType::isLegacyFiveFileTypes(type)) {
            return "";
        }
        
        QStringList paths = content.split(";", Qt::SkipEmptyParts);
        QSet<QString> extensions;
        for (const QString& path : paths) {
            QFileInfo info(path);
            if (info.isDir()) {
                extensions.insert("文件夹");
            } else {
                QString ext = info.suffix().toLower();
                if (!ext.isEmpty()) extensions.insert(ext);
            }
        }
        
        QStringList result = extensions.values();
        result.sort();
        return result.join(", ");
    }

}


using DbInternal::extractFileExtensions;

namespace DbInternal {
    constexpr int kInChunk = 500;   // 单条 SQL 的 IN(...) 上限，远低于 SQLite 变量上限
    QString inList(int n) {
        QString s; s.reserve(n * 2);
        for (int i = 0; i < n; ++i) { if (i) s += ','; s += '?'; }
        return s;
    }
    template <typename F>
    void forEachIdChunk(const QList<int>& ids, F&& fn) {
        for (int i = 0; i < ids.size(); i += kInChunk) fn(ids.mid(i, kInChunk));
    }
}


// 笔记"底色"(notes.color)是派生值：回收站灰 > 收藏绿 > 所属分类色。用户手动颜色标记另存于 color_label，两者互不覆盖。
inline constexpr const char* kNoteColorExpr =
    "CASE WHEN notes.is_deleted = 1 THEN '#2d2d2d' "
    "WHEN notes.is_favorite = 1 THEN '#2ECC71' "
    "ELSE COALESCE((SELECT c.color FROM categories c WHERE c.id = notes.category_id), '#0A362F') END";


using DbInternal::kInChunk;
using DbInternal::inList;
using DbInternal::forEachIdChunk;
