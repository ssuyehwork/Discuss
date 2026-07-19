#include "DiskSearchEngine.h"
#include <QDirIterator>
#include <QDebug>

namespace ArcMeta {

DiskSearchEngine::DiskSearchEngine(QObject* parent) : QThread(parent) {}

DiskSearchEngine::~DiskSearchEngine() {
    cancelSearch();
    wait();
}

void DiskSearchEngine::startSearch(const QString& rootPath, const QString& keyword) {
    cancelSearch();
    wait();

    {
        QMutexLocker lock(&m_mutex);
        m_rootPath = rootPath;
        m_keyword = keyword;
        m_cancelled = false;
    }

    start();
}

void DiskSearchEngine::cancelSearch() {
    QMutexLocker lock(&m_mutex);
    m_cancelled = true;
}

void DiskSearchEngine::run() {
    QString root;
    QString kw;
    {
        QMutexLocker lock(&m_mutex);
        root = m_rootPath;
        kw = m_keyword;
    }

    if (root.isEmpty()) {
        emit searchFinished();
        return;
    }

    // 独立搜索引擎线程执行高性能 QDirIterator 物理磁盘全量扫描
    QDirIterator it(root, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        {
            QMutexLocker lock(&m_mutex);
            if (m_cancelled) break;
        }

        QString filePath = it.next();
        if (filePath.contains(kw, Qt::CaseInsensitive)) {
            emit fileFound(filePath);
        }
    }

    emit searchFinished();
}

} // namespace ArcMeta
