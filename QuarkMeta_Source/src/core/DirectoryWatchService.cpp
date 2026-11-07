#include "DirectoryWatchService.h"
#include "CoreController.h"
#include "../ui/Logger.h"
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QMetaObject>

namespace QuarkMeta {

// 特殊的完成键：用于唤醒工作线程处理队列
static constexpr ULONG_PTR SPECIAL_KEY_WAKEUP = static_cast<ULONG_PTR>(-1);

DirectoryWatchService& DirectoryWatchService::instance() {
    static DirectoryWatchService service;
    return service;
}

QString DirectoryWatchService::normalizePathKey(const QString& rawPath) {
    if (rawPath.isEmpty()) return QString();
    QString cleaned = QDir::cleanPath(rawPath);
    cleaned = cleaned.toLower();

    // 移除尾部斜杠（盘符如 c: 保留 c:/）
    if (cleaned.endsWith('/') && !cleaned.endsWith(":/")) {
        cleaned.chop(1);
    }
    if (cleaned.endsWith(':')) {
        cleaned += '/';
    }

    return cleaned;
}

DirectoryWatchService::DirectoryWatchService(QObject* parent) : QObject(parent) {
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setInterval(50);
    connect(m_debounceTimer, &QTimer::timeout, this, &DirectoryWatchService::checkDebounceTimers);

    m_workerThread = new WorkerThread(this);
    m_workerThread->start();
}

DirectoryWatchService::~DirectoryWatchService() {
    stopAll();
}

void DirectoryWatchService::watch(const QString& dirPath, bool includeSubtree) {
    if (dirPath.isEmpty() || m_isStoppingAll.load()) return;

    QString normKey = normalizePathKey(dirPath);
    if (normKey.isEmpty()) return;

    WatchKey key{normKey, includeSubtree};

    QMutexLocker locker(&m_mutex);
    auto it = m_watches.find(key);
    if (it != m_watches.end()) {
        it.value()->refCount++;
        return;
    }

    auto item = std::make_shared<WatchItem>();
    item->key = key;
    item->originalDirPath = dirPath;
    item->refCount = 1;

    m_watches.insert(key, item);
    m_pendingStarts.append(item);

    if (m_workerThread) {
        m_workerThread->wakeUpWorker();
    }
}

void DirectoryWatchService::unwatch(const QString& dirPath, bool includeSubtree) {
    if (dirPath.isEmpty() || m_isStoppingAll.load()) return;

    QString normKey = normalizePathKey(dirPath);
    if (normKey.isEmpty()) return;

    WatchKey key{normKey, includeSubtree};

    QMutexLocker locker(&m_mutex);
    auto it = m_watches.find(key);
    if (it == m_watches.end()) return;

    it.value()->refCount--;
    if (it.value()->refCount <= 0) {
        auto item = it.value();
        item->isStopping = true;
        m_watches.erase(it);
        m_retiredItems.insert(key, item);
        m_pendingStops.append(item);

        if (m_workerThread) {
            m_workerThread->wakeUpWorker();
        }
    }
}

void DirectoryWatchService::discardPending(const QString& dirPath, bool includeSubtree) {
    if (dirPath.isEmpty() || m_isStoppingAll.load()) return;

    QString normKey = normalizePathKey(dirPath);
    if (normKey.isEmpty()) return;

    WatchKey key{normKey, includeSubtree};

    QMutexLocker locker(&m_mutex);
    m_debounceMap.remove(key);
    m_discardGenerations[key]++;

    if (m_debounceMap.isEmpty() && m_debounceTimer) {
        m_debounceTimer->stop();
    }
}

void DirectoryWatchService::stopAll() {
    if (m_isStoppingAll.exchange(true)) return;

    {
        QMutexLocker locker(&m_mutex);
        if (m_debounceTimer) m_debounceTimer->stop();
        m_debounceMap.clear();

        for (auto it = m_watches.begin(); it != m_watches.end(); ++it) {
            it.value()->isStopping = true;
            m_retiredItems.insert(it.key(), it.value());
            m_pendingStops.append(it.value());
        }
        m_watches.clear();
        m_pendingStarts.clear();
    }

    if (m_workerThread) {
        m_workerThread->stopWorker();
        if (!m_workerThread->wait(1000)) {
            Logger::log("[DirectoryWatchService] Worker thread wait timeout (1s), forcing cleanup.");
        }
        delete m_workerThread;
        m_workerThread = nullptr;
    }
}

void DirectoryWatchService::onRawEventsBatch(const WatchKey& key, const QStringList& dirtyNames, bool overflow) {
    if (m_isStoppingAll.load()) return;

    QMutexLocker locker(&m_mutex);
    auto watchIt = m_watches.find(key);
    QString displayPath = (watchIt != m_watches.end()) ? watchIt.value()->originalDirPath : key.normPath;

    auto debIt = m_debounceMap.find(key);
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (debIt == m_debounceMap.end()) {
        DebounceState state;
        state.firstEventTime = now;
        state.lastEventTime = now;
        state.displayPath = displayPath;
        if (overflow) {
            state.accumulatedOverflow = true;
        } else {
            for (const QString& name : dirtyNames) {
                state.accumulatedDirtyNames.insert(name);
            }
        }
        m_debounceMap.insert(key, state);
    } else {
        debIt.value().lastEventTime = now;
        if (overflow) {
            debIt.value().accumulatedOverflow = true;
        } else {
            for (const QString& name : dirtyNames) {
                debIt.value().accumulatedDirtyNames.insert(name);
            }
        }
    }

    if (!m_debounceTimer->isActive()) {
        m_debounceTimer->start();
    }
}

void DirectoryWatchService::onDirectoryLostInternal(const WatchKey& key, const QString& dirPath) {
    if (m_isStoppingAll.load()) return;

    {
        QMutexLocker locker(&m_mutex);
        auto it = m_watches.find(key);
        if (it != m_watches.end()) {
            auto item = it.value();
            item->isStopping = true;
            m_watches.erase(it);
            m_retiredItems.insert(key, item);
            m_pendingStops.append(item);
        }
        m_debounceMap.remove(key);
    }

    emit watchedDirectoryLost(dirPath);
}

void DirectoryWatchService::removeRetiredItem(const WatchKey& key) {
    QMutexLocker locker(&m_mutex);
    m_retiredItems.remove(key);
}

void DirectoryWatchService::checkDebounceTimers() {
    if (m_isStoppingAll.load()) return;

    QMutexLocker locker(&m_mutex);
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    QList<WatchKey> keysToEmit;
    for (auto it = m_debounceMap.begin(); it != m_debounceMap.end(); ++it) {
        const DebounceState& st = it.value();
        qint64 quietTime = now - st.lastEventTime;
        qint64 totalTime = now - st.firstEventTime;

        if (st.accumulatedOverflow || quietTime >= 300 || totalTime >= 1000) {
            keysToEmit.append(it.key());
        }
    }

    for (const WatchKey& key : keysToEmit) {
        DebounceState st = m_debounceMap.take(key);
        QString displayPath = st.displayPath;
        QStringList names = st.accumulatedDirtyNames.values();
        bool overflow = st.accumulatedOverflow;
        uint64_t gen = m_discardGenerations.value(key, 0);

        QMetaObject::invokeMethod(this, [this, key, gen, displayPath, names, overflow]() {
            if (m_isStoppingAll.load()) return;
            {
                QMutexLocker locker(&m_mutex);
                if (m_discardGenerations.value(key, 0) != gen) {
                    return;
                }
            }
            emit directoryChanged(displayPath, names, overflow);
        }, Qt::QueuedConnection);
    }

    if (m_debounceMap.isEmpty()) {
        m_debounceTimer->stop();
    }
}

#ifdef Q_OS_WIN
bool DirectoryWatchService::issueReadDirectoryChanges(WatchItem* item) {
    if (!item || item->hDir == INVALID_HANDLE_VALUE || item->isStopping) {
        return false;
    }

    ZeroMemory(&item->overlapped, sizeof(OVERLAPPED));

    // 关键修正：必须传入合法的 &item->bytesReturned，禁止传 NULL；回调必须为 NULL
    BOOL ok = ReadDirectoryChangesW(
        item->hDir,
        item->buffer,
        sizeof(item->buffer),
        item->key.includeSubtree ? TRUE : FALSE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_ATTRIBUTES,
        &item->bytesReturned,
        &item->overlapped,
        NULL
    );

    if (!ok) {
        DWORD err = GetLastError();
        return (err == ERROR_IO_PENDING);
    }
    return true;
}

void DirectoryWatchService::handleCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, WatchItem* item) {
    if (!item) return;

    if (dwErrorCode != ERROR_SUCCESS || item->isStopping || CoreController::isShuttingDown()) {
        if (dwErrorCode == ERROR_ACCESS_DENIED || dwErrorCode == ERROR_INVALID_HANDLE ||
            dwErrorCode == ERROR_DEVICE_NOT_CONNECTED || dwErrorCode == ERROR_HANDLE_EOF ||
            dwErrorCode == ERROR_NETNAME_DELETED) {
            QMetaObject::invokeMethod(&DirectoryWatchService::instance(), [key = item->key, path = item->originalDirPath]() {
                DirectoryWatchService::instance().onDirectoryLostInternal(key, path);
            }, Qt::QueuedConnection);
        }
        QMetaObject::invokeMethod(&DirectoryWatchService::instance(), [key = item->key]() {
            DirectoryWatchService::instance().removeRetiredItem(key);
        }, Qt::QueuedConnection);
        return;
    }

    if (dwNumberOfBytesTransfered == 0) {
        QMetaObject::invokeMethod(&DirectoryWatchService::instance(), [key = item->key]() {
            DirectoryWatchService::instance().onRawEventsBatch(key, QStringList(), true);
        }, Qt::QueuedConnection);
    } else {
        PFILE_NOTIFY_INFORMATION pNotify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(item->buffer);
        QStringList dirtyNames;

        while (pNotify) {
            int nameLen = pNotify->FileNameLength / sizeof(WCHAR);
            if (nameLen > 0) {
                QString relPath = QString::fromWCharArray(pNotify->FileName, nameLen);
                relPath = QDir::cleanPath(relPath);
                dirtyNames.append(relPath);
            }

            if (pNotify->NextEntryOffset == 0) break;
            pNotify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(reinterpret_cast<LPBYTE>(pNotify) + pNotify->NextEntryOffset);
        }

        if (!dirtyNames.isEmpty()) {
            QMetaObject::invokeMethod(&DirectoryWatchService::instance(), [key = item->key, dirtyNames]() {
                DirectoryWatchService::instance().onRawEventsBatch(key, dirtyNames, false);
            }, Qt::QueuedConnection);
        }
    }

    // 重新闭环挂接异步监听
    if (!issueReadDirectoryChanges(item)) {
        DWORD err = GetLastError();
        Logger::log(QString("[DirectoryWatchService] Re-issue ReadDirectoryChangesW failed for %1, error: %2")
                    .arg(item->originalDirPath).arg(err));
        QMetaObject::invokeMethod(&DirectoryWatchService::instance(), [key = item->key, path = item->originalDirPath]() {
            DirectoryWatchService::instance().onDirectoryLostInternal(key, path);
        }, Qt::QueuedConnection);
    }
}
#endif

DirectoryWatchService::WorkerThread::WorkerThread(DirectoryWatchService* service)
    : m_service(service) {
#ifdef Q_OS_WIN
    m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
#endif
}

DirectoryWatchService::WorkerThread::~WorkerThread() {
#ifdef Q_OS_WIN
    if (m_hIOCP) {
        CloseHandle(m_hIOCP);
        m_hIOCP = NULL;
    }
#endif
}

void DirectoryWatchService::WorkerThread::stopWorker() {
    m_stopRequested.store(true);
    wakeUpWorker();
}

void DirectoryWatchService::WorkerThread::wakeUpWorker() {
#ifdef Q_OS_WIN
    if (m_hIOCP) {
        PostQueuedCompletionStatus(m_hIOCP, 0, SPECIAL_KEY_WAKEUP, NULL);
    }
#endif
}

void DirectoryWatchService::WorkerThread::run() {
    while (!m_stopRequested.load() && !CoreController::isShuttingDown()) {
        m_service->processWorkerQueue();

#ifdef Q_OS_WIN
        DWORD dwBytes = 0;
        ULONG_PTR ulKey = 0;
        LPOVERLAPPED lpOverlapped = NULL;

        // 阻塞等待事件，超时 250ms 用于维持线程退出的周期检查
        BOOL ok = GetQueuedCompletionStatus(m_hIOCP, &dwBytes, &ulKey, &lpOverlapped, 250);
        DWORD dwErr = ok ? ERROR_SUCCESS : GetLastError();

        if (ulKey == SPECIAL_KEY_WAKEUP) {
            continue;
        }

        // 关键修正：ulKey 就是我们在 CreateIoCompletionPort 绑定的 WatchItem*
        if (ulKey != 0 && ulKey != SPECIAL_KEY_WAKEUP) {
            WatchItem* item = reinterpret_cast<WatchItem*>(ulKey);
            DirectoryWatchService::handleCompletion(dwErr, dwBytes, item);
        }
#else
        QThread::msleep(100);
#endif
    }

    // 线程退出时清理所有句柄
    QList<std::shared_ptr<WatchItem>> itemsToCleanup;
    {
        QMutexLocker locker(&m_service->m_mutex);
        for (auto item : m_service->m_pendingStops) itemsToCleanup.append(item);
        m_service->m_pendingStops.clear();

        for (auto item : m_service->m_pendingStarts) itemsToCleanup.append(item);
        m_service->m_pendingStarts.clear();

        for (auto it = m_service->m_watches.begin(); it != m_service->m_watches.end(); ++it) {
            itemsToCleanup.append(it.value());
        }
        m_service->m_watches.clear();

        for (auto it = m_service->m_retiredItems.begin(); it != m_service->m_retiredItems.end(); ++it) {
            itemsToCleanup.append(it.value());
        }
        m_service->m_retiredItems.clear();
    }

#ifdef Q_OS_WIN
    for (auto item : itemsToCleanup) {
        if (item->hDir != INVALID_HANDLE_VALUE) {
            CancelIoEx(item->hDir, &item->overlapped);
            CloseHandle(item->hDir);
            item->hDir = INVALID_HANDLE_VALUE;
        }
    }
#endif
}

void DirectoryWatchService::processWorkerQueue() {
    QList<std::shared_ptr<WatchItem>> starts;
    QList<std::shared_ptr<WatchItem>> stops;

    {
        QMutexLocker locker(&m_mutex);
        starts = m_pendingStarts;
        m_pendingStarts.clear();

        stops = m_pendingStops;
        m_pendingStops.clear();
    }

#ifdef Q_OS_WIN
    // 1. 处理移除的句柄
    for (auto item : stops) {
        if (item->hDir != INVALID_HANDLE_VALUE) {
            CancelIoEx(item->hDir, &item->overlapped);
            CloseHandle(item->hDir);
            item->hDir = INVALID_HANDLE_VALUE;
        }
    }

    // 2. 处理挂接的句柄
    for (auto item : starts) {
        if (m_isStoppingAll.load() || item->isStopping) continue;

        // 关键修正：规范化 Windows 目录路径（严禁非根目录末尾携带斜杠）
        QString cleanedPath = QDir::cleanPath(item->originalDirPath);
        QString native = QDir::toNativeSeparators(cleanedPath);
        
        // 如果是 D:\ 则保留，如果是 D:\Folder\ 则去除末尾斜杠
        if (native.length() > 3 && native.endsWith('\\')) {
            native.chop(1);
        }

        QString pathStr;
        if (!native.startsWith("\\\\?\\")) {
            if (native.length() >= 240) {
                if (native.startsWith("\\\\")) {
                    pathStr = "\\\\?\\UNC\\" + native.mid(2);
                } else {
                    pathStr = "\\\\?\\" + native;
                }
            } else {
                pathStr = native;
            }
        } else {
            pathStr = native;
        }

        std::wstring wPath = pathStr.toStdWString();
        HANDLE hDir = CreateFileW(
            wPath.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            NULL
        );

        if (hDir == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            Logger::log(QString("[DirectoryWatchService] Failed to open handle for %1, error: %2")
                        .arg(item->originalDirPath).arg(err));
            continue;
        }

        item->hDir = hDir;

        // 关键修正：将 item.get() 作为 CompletionKey 传给 IOCP，安全无 UB
        if (m_workerThread && m_workerThread->iocpHandle()) {
            HANDLE hPort = CreateIoCompletionPort(
                item->hDir,
                m_workerThread->iocpHandle(),
                reinterpret_cast<ULONG_PTR>(item.get()),
                0
            );

            if (!hPort) {
                DWORD err = GetLastError();
                Logger::log(QString("[DirectoryWatchService] Bind IOCP failed for %1, error: %2")
                            .arg(item->originalDirPath).arg(err));
                CloseHandle(item->hDir);
                item->hDir = INVALID_HANDLE_VALUE;
                continue;
            }
        }

        // 初次挂起监控
        if (!issueReadDirectoryChanges(item.get())) {
            DWORD err = GetLastError();
            Logger::log(QString("[DirectoryWatchService] Initial ReadDirectoryChangesW failed for %1, error: %2")
                        .arg(item->originalDirPath).arg(err));
            CloseHandle(item->hDir);
            item->hDir = INVALID_HANDLE_VALUE;
        }
    }
#else
    Q_UNUSED(starts);
    Q_UNUSED(stops);
#endif
}

} // namespace QuarkMeta