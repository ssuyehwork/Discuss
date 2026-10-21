#include "LibrarySyncService.h"
#include "LibraryDao.h"
#include "LibraryItemDao.h"
#include "QuarkMetaJsonStore.h"
#include "ItemMetaMapper.h"
#include "MetadataManager.h"
#include <QtConcurrent/QtConcurrent>
#include <QDir>
#include <QTimer>
#include <QDebug>

namespace QuarkMeta {

LibrarySyncService& LibrarySyncService::instance() {
    static LibrarySyncService s_instance;
    return s_instance;
}

LibrarySyncService::LibrarySyncService(QObject* parent) : QObject(parent) {
    m_workPool.setMaxThreadCount(1);
    loadLibraryPathSet();

    QTimer::singleShot(1500, this, [this]() {
        triggerStartupReconciliation();
    });
}

LibrarySyncService::~LibrarySyncService() {
    m_workPool.waitForDone();
}

QString LibrarySyncService::normalizePath(const QString& path) {
    return QDir::toNativeSeparators(path).toLower();
}

void LibrarySyncService::loadLibraryPathSet() {
    QStringList paths = LibraryDao::getCategoryPaths(-1);
    std::unique_lock<std::shared_mutex> lock(m_pathMutex);
    m_libraryPathsSet.clear();
    for (const QString& p : paths) {
        m_libraryPathsSet.insert(normalizePath(p).toStdWString());
    }
}

bool LibrarySyncService::isLibraryPath(const QString& path) const {
    std::shared_lock<std::shared_mutex> lock(m_pathMutex);
    return m_libraryPathsSet.count(normalizePath(path).toStdWString()) > 0;
}

void LibrarySyncService::syncPaths(const QStringList& paths) {
    if (paths.isEmpty()) return;

    {
        std::unique_lock<std::shared_mutex> lock(m_pathMutex);
        for (const QString& p : paths) {
            m_libraryPathsSet.insert(normalizePath(p).toStdWString());
        }
    }

    (void)QtConcurrent::run(&m_workPool, [this, paths]() {
        const int batchSize = 100;
        for (int i = 0; i < paths.size(); i += batchSize) {
            QStringList batch = paths.mid(i, batchSize);
            processSyncBatch(batch);
        }
    });
}

void LibrarySyncService::removePaths(const QStringList& paths) {
    if (paths.isEmpty()) return;

    (void)QtConcurrent::run(&m_workPool, [this, paths]() {
        QStringList pathsToRemove;
        {
            std::unique_lock<std::shared_mutex> lock(m_pathMutex);
            for (const QString& p : paths) {
                QString norm = normalizePath(p);
                // 确认该路径已不被任何分类引用
                QStringList cats = LibraryDao::getCategoryPaths(-1);
                if (!cats.contains(p, Qt::CaseInsensitive)) {
                    m_libraryPathsSet.erase(norm.toStdWString());
                    pathsToRemove.append(p);
                }
            }
        }
        if (!pathsToRemove.isEmpty()) {
            LibraryItemDao::removeItemsByPaths(pathsToRemove);
        }
    });
}

void LibrarySyncService::processSyncBatch(const QStringList& batch) {
    for (const QString& path : batch) {
        ItemRecord rec = ItemRecord::create(path);
        ItemMeta meta;
        if (QuarkMetaJsonStore::instance().readItemMeta(path.toStdWString(), meta)) {
            ItemMetaMapper::apply(meta, rec);
        }
        LibraryItemDao::upsertItem(rec);
    }
}

void LibrarySyncService::triggerStartupReconciliation() {
    (void)QtConcurrent::run(&m_workPool, [this]() {
        // 对账 1: 分类中有而库表中缺少的，增量补齐
        QStringList categoryPaths = LibraryDao::getCategoryPaths(-1);
        std::vector<ItemRecord> existingItems = LibraryItemDao::getAllLibraryItems();
        std::unordered_set<std::wstring> existingSet;
        for (const auto& item : existingItems) {
            existingSet.insert(normalizePath(item.path).toStdWString());
        }

        QStringList missingPaths;
        for (const QString& cp : categoryPaths) {
            if (existingSet.count(normalizePath(cp).toStdWString()) == 0) {
                missingPaths.append(cp);
            }
        }
        if (!missingPaths.isEmpty()) {
            syncPaths(missingPaths);
        }

        // 对账 2: 库表中有但已无任何分类引用的孤儿条目，自动清理
        std::unordered_set<std::wstring> catSet;
        for (const QString& cp : categoryPaths) {
            catSet.insert(normalizePath(cp).toStdWString());
        }

        QStringList orphanPaths;
        for (const auto& item : existingItems) {
            if (catSet.count(normalizePath(item.path).toStdWString()) == 0) {
                orphanPaths.append(item.path);
            }
        }
        if (!orphanPaths.isEmpty()) {
            LibraryItemDao::removeItemsByPaths(orphanPaths);
        }
    });
}

} // namespace QuarkMeta
