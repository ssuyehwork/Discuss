#ifndef ARCMETA_DISK_SEARCH_ENGINE_H
#define ARCMETA_DISK_SEARCH_ENGINE_H

#include <QThread>
#include <QStringList>
#include <QMutex>

namespace ArcMeta {

class DiskSearchEngine : public QThread {
    Q_OBJECT
public:
    DiskSearchEngine(QObject* parent = nullptr);
    ~DiskSearchEngine();

    void startSearch(const QString& rootPath, const QString& keyword);
    void cancelSearch();

signals:
    void fileFound(const QString& path);
    void searchFinished();

protected:
    void run() override;

private:
    QString m_rootPath;
    QString m_keyword;
    QMutex m_mutex;
    bool m_cancelled = false;
};

} // namespace ArcMeta

#endif // ARCMETA_DISK_SEARCH_ENGINE_H
