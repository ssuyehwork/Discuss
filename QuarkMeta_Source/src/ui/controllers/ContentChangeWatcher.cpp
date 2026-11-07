#include "ContentChangeWatcher.h"
#include "../ContentPanel.h"
#include "../ColumnViewWidget.h"
#include "models/DiskItemModel.h"
#include "../../core/DirectoryWatchService.h"
#include "../../core/DwsTrace.h"
#include "../../core/FileFilterService.h"
#include "../../meta/MetadataManager.h"
#include "../../meta/MetaCacheDecorator.h"
#include "../../core/ItemRecord.h"
#include <QDir>
#include <QFileInfo>
#include <QApplication>
#include <QLineEdit>
#include <QThreadPool>

namespace QuarkMeta {

ContentChangeWatcher::ContentChangeWatcher(ContentPanel* panel)
    : QObject(panel), m_panel(panel) {
    m_delayedApplyTimer = new QTimer(this);
    m_delayedApplyTimer->setSingleShot(true);
    m_delayedApplyTimer->setInterval(300);
    connect(m_delayedApplyTimer, &QTimer::timeout, this, &ContentChangeWatcher::processAccumulatedChanges);

    connect(&DirectoryWatchService::instance(), &DirectoryWatchService::directoryChanged,
            this, &ContentChangeWatcher::onDirectoryChanged);
    connect(&DirectoryWatchService::instance(), &DirectoryWatchService::watchedDirectoryLost,
            this, &ContentChangeWatcher::onWatchedDirectoryLost);

    if (m_panel && m_panel->columnView()) {
        connect(m_panel->columnView(), &ColumnViewWidget::activeColumnRecordsChanged,
                this, &ContentChangeWatcher::updateTarget);
    }
}

ContentChangeWatcher::~ContentChangeWatcher() {
    unwatchAll();
}

void ContentChangeWatcher::unwatchAll() {
    for (const QString& p : m_watchedPaths) {
        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "unwatchAll",
            QString("调用 DirectoryWatchService::unwatch; path=%1, includeSubtree=%2").arg(p).arg(m_watchedIncludeSubtree ? "true" : "false"));
        DirectoryWatchService::instance().unwatch(p, m_watchedIncludeSubtree);
    }
    m_watchedPaths.clear();
    m_watchedIncludeSubtree = false;
}

bool ContentChangeWatcher::shouldWatchPath(const QString& path) const {
    if (path.isEmpty()) return false;
    if (path.contains("://")) return false;
    QFileInfo fi(path);
    return fi.exists() && fi.isDir();
}

void ContentChangeWatcher::updateTarget() {
    if (!m_panel) return;

    if (!m_panel->getCurrentCategoryType().isEmpty() ||
        m_panel->model() != m_panel->diskModel()) {
        unwatchAll();
        return;
    }

    if (m_panel->currentViewMode() == ContentPanel::ColumnView && m_panel->columnView()) {
        QStringList colPaths = m_panel->columnView()->columnPaths();
        QSet<QString> newTargetSet;
        for (const QString& cp : colPaths) {
            if (shouldWatchPath(cp)) {
                newTargetSet.insert(cp);
            }
        }

        if (newTargetSet == m_watchedPaths && !m_watchedIncludeSubtree) {
            return;
        }

        unwatchAll();
        for (const QString& tp : newTargetSet) {
            QUARKMETA_DWS_TRACE("ContentChangeWatcher", "updateTarget",
                QString("ColumnView 模式调用 DirectoryWatchService::watch; path=%1, includeSubtree=false").arg(tp));
            DirectoryWatchService::instance().watch(tp, false);
            m_watchedPaths.insert(tp);
        }
        m_watchedIncludeSubtree = false;
        return;
    }

    QString cPath = m_panel->currentPath();
    bool recursive = m_panel->isRecursive();

    if (!shouldWatchPath(cPath)) {
        unwatchAll();
        return;
    }

    QSet<QString> newTargetSet;
    newTargetSet.insert(cPath);

    if (m_watchedPaths == newTargetSet && m_watchedIncludeSubtree == recursive) {
        return;
    }

    unwatchAll();
    QUARKMETA_DWS_TRACE("ContentChangeWatcher", "updateTarget",
        QString("标准视图调用 DirectoryWatchService::watch; path=%1, includeSubtree=%2").arg(cPath).arg(recursive ? "true" : "false"));
    DirectoryWatchService::instance().watch(cPath, recursive);
    m_watchedPaths.insert(cPath);
    m_watchedIncludeSubtree = recursive;
}

void ContentChangeWatcher::onLoadFinished() {
    QTimer::singleShot(0, this, [this]() {
        if (!m_pendingDirtyNames.isEmpty()) {
            processAccumulatedChanges();
        }
    });
}

bool ContentChangeWatcher::isDelayed() const {
    if (!m_panel) return false;
    if (m_panel->isLoading()) return true;
    if (m_panel->isContextMenuActive()) return true;

    QWidget* fw = QApplication::focusWidget();
    if (fw && qobject_cast<QLineEdit*>(fw)) {
        QAbstractItemView* view = m_panel->activeItemView();
        if (view && view->viewport() && view->viewport()->isAncestorOf(fw)) {
            return true;
        }
    }

    return false;
}

void ContentChangeWatcher::onWatchedDirectoryLost(const QString& dirPath) {
    // 收到 watchedDirectoryLost 时，不做导航与弹窗，由现有逻辑负责
    QUARKMETA_DWS_TRACE("ContentChangeWatcher", "onWatchedDirectoryLost",
        QString("收到 watchedDirectoryLost 信号; dirPath=%1").arg(dirPath));
}

void ContentChangeWatcher::beginSuppress() {
    m_suppressCount++;
    m_pendingDirtyNames.clear();
    if (m_delayedApplyTimer) {
        m_delayedApplyTimer->stop();
    }
}

void ContentChangeWatcher::endSuppress() {
    if (m_suppressCount > 0) {
        m_suppressCount--;
    }
    if (m_suppressCount == 0) {
        for (const QString& wp : m_watchedPaths) {
            QUARKMETA_DWS_TRACE("ContentChangeWatcher", "endSuppress",
                QString("调用 DirectoryWatchService::discardPending; path=%1, includeSubtree=%2").arg(wp).arg(m_watchedIncludeSubtree ? "true" : "false"));
            DirectoryWatchService::instance().discardPending(wp, m_watchedIncludeSubtree);
        }
    }
}

void ContentChangeWatcher::onDirectoryChanged(const QString& dirPath, const QStringList& dirtyNames, bool overflow) {
    QUARKMETA_DWS_TRACE("ContentChangeWatcher", "onDirectoryChanged",
        QString("槽函数入口: dirPath=%1, dirtyNames数量=%2, overflow=%3")
            .arg(dirPath).arg(dirtyNames.size()).arg(overflow ? "true" : "false"));

    if (!m_panel) {
        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "onDirectoryChanged", "过滤原因: m_panel 为空，直接 return");
        return;
    }
    if (m_suppressCount > 0) {
        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "onDirectoryChanged",
            QString("过滤原因: m_suppressCount=%1 > 0(正在执行内部文件操作)，忽略变更并 return").arg(m_suppressCount));
        return;
    }

    QString normDirPath = DirectoryWatchService::normalizePathKey(dirPath);
    bool hit = false;
    for (const QString& wp : m_watchedPaths) {
        if (DirectoryWatchService::normalizePathKey(wp) == normDirPath) {
            hit = true;
            break;
        }
    }
    if (!hit && !m_watchedIncludeSubtree) {
        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "onDirectoryChanged",
            QString("过滤原因: 该目录未命中当前面板监听路径(m_watchedPaths)，且未开启子树监听; eventDir=%1, watchedPathsCount=%2")
                .arg(normDirPath).arg(m_watchedPaths.size()));
        return;
    }

    // 先过滤辅助文件：只含辅助文件的变化，不触发任何刷新，包括递归与分栏视图
    QStringList effectiveNames;
    if (!overflow) {
        for (const QString& rawName : dirtyNames) {
            QString absP = QDir(dirPath).filePath(rawName);
            if (!FileFilterService::isAuxiliaryFile(absP)) {
                effectiveNames.append(rawName);
            }
        }
        if (effectiveNames.isEmpty()) {
            QUARKMETA_DWS_TRACE("ContentChangeWatcher", "onDirectoryChanged",
                QString("过滤原因: 变更项均为辅助文件(如 .QuarkMeta.json/缩略图/临时文件)，全部被过滤，return; dirPath=%1").arg(dirPath));
            return;
        }
    }

    if (m_panel->isRecursive()) {
        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "onDirectoryChanged",
            QString("当前面板处于递归模式(recursive=true)，调用 m_panel->refreshAll() 触发全量刷新并 return; dirPath=%1").arg(dirPath));
        m_panel->refreshAll();
        return;
    }

    if (m_panel->currentViewMode() == ContentPanel::ColumnView && m_panel->columnView()) {
        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "onDirectoryChanged",
            QString("当前面板处于分栏视图模式(ColumnView)，调用 columnView()->refreshAllColumns() 并 return; dirPath=%1").arg(dirPath));
        m_panel->columnView()->refreshAllColumns();
        return;
    }

    if (overflow || effectiveNames.size() > 500) {
        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "onDirectoryChanged",
            QString("事件溢出或有效变更项数量超限(overflow=%1, effectiveNames=%2 > 500)，调用 m_panel->refreshAll() 全量刷新并 return")
                .arg(overflow ? "true" : "false").arg(effectiveNames.size()));
        m_pendingDirtyNames.clear();
        m_panel->refreshAll();
        return;
    }

    for (const QString& rawName : effectiveNames) {
        m_pendingDirtyNames.insert(rawName);
    }

    m_pendingDirPath = dirPath;

    if (isDelayed() || m_isTaskRunning.load()) {
        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "onDirectoryChanged",
            QString("当前界面处于延迟状态或后台任务进行中(isDelayed=%1, isTaskRunning=%2)，启动 m_delayedApplyTimer 延迟合并处理")
                .arg(isDelayed() ? "true" : "false").arg(m_isTaskRunning.load() ? "true" : "false"));
        if (!m_delayedApplyTimer->isActive()) {
            m_delayedApplyTimer->start();
        }
        return;
    }

    processAccumulatedChanges();
}

void ContentChangeWatcher::processAccumulatedChanges() {
    if (m_pendingDirtyNames.isEmpty() || !m_panel) {
        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "processAccumulatedChanges",
            QString("无需处理并 return: pendingDirtyNamesEmpty=%1, panelNull=%2")
                .arg(m_pendingDirtyNames.isEmpty() ? "true" : "false").arg(!m_panel ? "true" : "false"));
        return;
    }

    if (isDelayed() || m_isTaskRunning.load()) {
        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "processAccumulatedChanges",
            QString("当前处于延迟状态(isDelayed=%1, isTaskRunning=%2)，重新启动 m_delayedApplyTimer")
                .arg(isDelayed() ? "true" : "false").arg(m_isTaskRunning.load() ? "true" : "false"));
        if (!m_delayedApplyTimer->isActive()) {
            m_delayedApplyTimer->start();
        }
        return;
    }

    m_isTaskRunning.store(true);

    QStringList names = m_pendingDirtyNames.values();
    m_pendingDirtyNames.clear();
    QString dirPath = m_pendingDirPath;

    int initialReqId = m_panel->loadRequestId();
    QString initialPath = m_panel->currentPath();

    QUARKMETA_DWS_TRACE("ContentChangeWatcher", "processAccumulatedChanges",
        QString("投递至后台线程池 QThreadPool 处理前; dirPath=%1, 累积文件数=%2, initialReqId=%3, initialPath=%4")
            .arg(dirPath).arg(names.size()).arg(initialReqId).arg(initialPath));

    QThreadPool::globalInstance()->start([this, dirPath, names, initialReqId, initialPath]() {
        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "processAccumulatedChanges[Worker]",
            QString("后台线程开始解析变更项目; dirPath=%1, 项目数=%2").arg(dirPath).arg(names.size()));

        std::vector<ItemRecord> records;
        QStringList removedPaths;

        for (const QString& relName : names) {
            QString absPath = QDir(dirPath).filePath(relName);
            absPath = QFileInfo(absPath).absoluteFilePath();

            if (QFile::exists(absPath) || QDir(absPath).exists()) {
                records.push_back(ItemRecord::create(absPath, nullptr));
            } else {
                removedPaths.append(absPath);
            }
        }

        MetaCacheDecorator::decorate(records);

        QUARKMETA_DWS_TRACE("ContentChangeWatcher", "processAccumulatedChanges[Worker]",
            QString("后台解析装饰完毕，通过 invokeMethod 派发回主线程; records数量=%1, removedPaths数量=%2")
                .arg(records.size()).arg(removedPaths.size()));

        QMetaObject::invokeMethod(this, [this, records = std::move(records), removedPaths = std::move(removedPaths), initialReqId, initialPath]() {
            m_isTaskRunning.store(false);

            if (!m_panel) {
                QUARKMETA_DWS_TRACE("ContentChangeWatcher", "processAccumulatedChanges[MainThread]", "返回主线程后 m_panel 为空，丢弃结果");
                return;
            }
            if (m_panel->loadRequestId() != initialReqId || m_panel->currentPath() != initialPath) {
                QUARKMETA_DWS_TRACE("ContentChangeWatcher", "processAccumulatedChanges[MainThread]",
                    QString("发生过整体加载或目录切换，丢弃结果; currentReqId=%1, initialReqId=%2, currentPath=%3, initialPath=%4")
                        .arg(m_panel->loadRequestId()).arg(initialReqId).arg(m_panel->currentPath()).arg(initialPath));
                // 发生过整体加载或目录切换，丢弃结果
                if (!m_pendingDirtyNames.isEmpty()) {
                    processAccumulatedChanges();
                }
                return;
            }

            QStringList addedPaths = m_panel->diskModel()->applyIncrementalChanges(records, removedPaths);

            QUARKMETA_DWS_TRACE("ContentChangeWatcher", "processAccumulatedChanges[MainThread]",
                QString("完成增量应用到 DiskItemModel; records=%1, removed=%2, addedPaths数量=%3")
                    .arg(records.size()).arg(removedPaths.size()).arg(addedPaths.size()));

            if (!addedPaths.isEmpty()) {
                QStringList addedFiles;
                for (const QString& ap : addedPaths) {
                    if (!QFileInfo(ap).isDir()) {
                        addedFiles.append(ap);
                    }
                }
                if (!addedFiles.isEmpty()) {
                    QUARKMETA_DWS_TRACE("ContentChangeWatcher", "processAccumulatedChanges[MainThread]",
                        QString("调用 MetadataManager::registerItemsAsync; 新增文件数=%1").arg(addedFiles.size()));
                    MetadataManager::instance().registerItemsAsync(addedFiles);
                } else {
                    QUARKMETA_DWS_TRACE("ContentChangeWatcher", "processAccumulatedChanges[MainThread]",
                        "新增路径均为文件夹，无需调用 registerItemsAsync");
                }
            }

            m_panel->notifyRecordsChanged();

            if (!m_pendingDirtyNames.isEmpty()) {
                processAccumulatedChanges();
            }
        }, Qt::QueuedConnection);
    });
}

} // namespace QuarkMeta
