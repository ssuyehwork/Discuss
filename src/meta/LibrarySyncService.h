#pragma once

#include <QObject>
#include <QStringList>
#include <QThreadPool>
#include <shared_mutex>
#include <unordered_set>

namespace QuarkMeta {

class LibrarySyncService : public QObject {
    Q_OBJECT

public:
    static LibrarySyncService& instance();

    void syncPaths(const QStringList& paths);
    void removePaths(const QStringList& paths);
    bool isLibraryPath(const QString& path) const;

    void triggerStartupReconciliation();

private:
    explicit LibrarySyncService(QObject* parent = nullptr);
    ~LibrarySyncService() override;

    void loadLibraryPathSet();
    void processSyncBatch(const QStringList& batch);
    static QString normalizePath(const QString& path);

    QThreadPool m_workPool;
    std::unordered_set<std::wstring> m_libraryPathsSet;
    mutable std::shared_mutex m_pathMutex;
};

} // namespace QuarkMeta
