#include "DiskItemModel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "ModelContract.h"
#include "../ContentPanel.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QThreadPool>
#include "../../meta/QuarkMetaJson.h"
#include "../MediaColorExtractor.h"
#include "../../core/CoreController.h"
#include "../../util/DiskMediaExtractor.h"
#include "../DiskBatchRenameService.h"
#include "../../meta/FileOperationHelper.h"
#include "../../util/DiskMediaExtractor.h"
#include "../../meta/MetadataManager.h"

using namespace QuarkMeta;

#include <QtConcurrent>

QThreadPool* DiskItemModel::thumbnailPool() {
    static QThreadPool pool;
    static std::once_flag flag;
    std::call_once(flag, []() {
        pool.setMaxThreadCount(2);
        pool.setThreadPriority(QThread::LowestPriority);
    });
    return &pool;
}

void DiskItemModel::incrementGeneration() {
    uint64_t oldGen = m_currentGen.load(std::memory_order_relaxed);
    {
        QMutexLocker locker(&m_genTokenMutex);
        auto it = m_genTokens.find(oldGen);
        if (it != m_genTokens.end()) {
            if (it.value()) it.value()->cancel();
            m_genTokens.erase(it);
        }
    }
    m_currentGen.fetch_add(1, std::memory_order_relaxed);
}

DiskItemModel::DiskItemModel(QObject* parent) : ItemModelBase(parent) {
    m_iconCache.setMaxCost(500);
}

DiskItemModel::~DiskItemModel() {}

int DiskItemModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_allRecords.size());
}

int DiskItemModel::columnCount(const QModelIndex&) const {
    return 7;
}

QVariant DiskItemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return QString("名称");
            case 1: return QString("状态");
            case 2: return QString("评分");
            case 3: return QString("尺寸");
            case 4: return QString("类型");
            case 5: return QString("大小");
            case 6: return QString("修改日期");
            default: break;
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

void DiskItemModel::setRecords(const std::vector<ItemRecord>& records) {
    incrementGeneration(); // 🚨 代际递增：瞬间废除上一目录的所有在途任务！
    beginResetModel();
    m_allRecords = records;
    m_pathToIndex.clear();
    m_requestedPaths.clear(); // 🚨 清空请求锁
    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        m_pathToIndex[m_allRecords[i].path] = i;
    }
    m_iconCache.setMaxCost(qMax(500, static_cast<int>(m_allRecords.size()) + 50));
    m_requestedIcons.clear();
    endResetModel();

    preloadDimensionsAsync();
}

struct SizeTarget {
    int index;
    QString path;
    QString suffix;
};

void DiskItemModel::preloadDimensionsAsync() {
    uint64_t thisGen = m_currentGen.load(std::memory_order_relaxed);

    // 主线程构建 SizeTarget 拷贝，规避多线程引用 m_allRecords 导致的竞态与崩溃
    std::vector<SizeTarget> targets;
    targets.reserve(m_allRecords.size());
    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        const auto& rec = m_allRecords[i];
        if (!rec.isDir && rec.width == 0 && UiHelper::isGraphicsFile(rec.suffix)) {
            targets.push_back({i, rec.path, rec.suffix});
        }
    }

    if (targets.empty()) return;

    QPointer<DiskItemModel> weakThis(this);

    thumbnailPool()->start([weakThis, targets = std::move(targets), thisGen]() {
        if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

        for (const auto& target : targets) {
            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

            QSize sz = DiskMediaExtractor::fastExtractImageSize(target.path);
            if (sz.isValid() && sz.width() > 0) {
                QFileInfo fi(target.path);
                QString parentDir = QDir::toNativeSeparators(fi.absolutePath());
                QString fileName = fi.fileName();

                static std::mutex s_jsonSaveMutex;
                std::lock_guard<std::mutex> lock(s_jsonSaveMutex);

                QuarkMetaJson jsonCache(parentDir.toStdWString());
                jsonCache.load();
                auto& cachedItems = jsonCache.items();
                std::wstring wFileName = fileName.toStdWString();
                if (cachedItems.find(wFileName) == cachedItems.end()) {
                    ItemMeta emptyMeta;
                    emptyMeta.type = L"file";
                    cachedItems[wFileName] = emptyMeta;
                }
                auto& fileMeta = cachedItems[wFileName];
                if (fileMeta.width != sz.width() || fileMeta.height != sz.height()) {
                    fileMeta.width = sz.width();
                    fileMeta.height = sz.height();
                    jsonCache.save();
                }

                int i = target.index;
                QString targetPath = target.path;
                QMetaObject::invokeMethod(weakThis.data(), [weakThis, i, targetPath, sz, thisGen]() {
                    if (weakThis && weakThis->currentGeneration() == thisGen && i < static_cast<int>(weakThis->m_allRecords.size())) {
                        if (weakThis->m_allRecords[i].path == targetPath) {
                            weakThis->m_allRecords[i].width = sz.width();
                            weakThis->m_allRecords[i].height = sz.height();
                            emit weakThis->dataChanged(weakThis->index(i, 3), weakThis->index(i, 3));
                        }
                    }
                }, Qt::QueuedConnection);
            }
        }
    });
}

void DiskItemModel::clear() {
    incrementGeneration(); // 🚨 代际递增
    beginResetModel();
    m_allRecords.clear();
    m_pathToIndex.clear();
    m_requestedPaths.clear(); // 🚨 清空请求锁
    m_query.clear();
    m_requestedIcons.clear();
    m_aspectRatios.clear();
    endResetModel();
}

void DiskItemModel::updateRecordMetadata(const QString& path) {
    QString nPath = QDir::toNativeSeparators(path);
    auto it = m_pathToIndex.find(nPath);
    if (it != m_pathToIndex.end()) {
        int i = it->second;
        if (i >= 0 && i < static_cast<int>(m_allRecords.size())) {
            auto& record = m_allRecords[i];
            QFileInfo fileInfo(nPath);
            QString parentDir = QDir::toNativeSeparators(fileInfo.absolutePath());
            QString fileName = fileInfo.fileName();

            QuarkMetaJson jsonCache(parentDir.toStdWString());
            jsonCache.load();
            const auto& cachedItems = jsonCache.items();
            auto cachedIt = cachedItems.find(fileName.toStdWString());
            if (cachedIt != cachedItems.end()) {
                record.rating = cachedIt->second.rating;
                record.manualColor = QString::fromStdWString(cachedIt->second.color);
                record.pinned = cachedIt->second.pinned;
                record.note = QString::fromStdWString(cachedIt->second.note);
                record.url = QString::fromStdWString(cachedIt->second.url);
                record.tags.clear();
                for (const auto& t : cachedIt->second.tags) {
                    record.tags.append(QString::fromStdWString(t));
                }
                record.width = cachedIt->second.width;
                record.height = cachedIt->second.height;
                record.autoColor = QString::fromStdWString(cachedIt->second.autoColor);
                record.added_at = cachedIt->second.addedAt;

                record.palettes.clear();
                for (const auto& pe : cachedIt->second.palettes) {
                    record.palettes.push_back({pe.color, pe.ratio});
                }
            }
            emit dataChanged(index(i, 0), index(i, columnCount() - 1));
        }
    }
}

bool DiskItemModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() >= static_cast<int>(m_allRecords.size())) return false;

    // 处理 F2 / 右键菜单行内重命名提交
    if (role == Qt::EditRole && index.column() == 0) {
        QString newName = value.toString().trimmed();
        if (newName.isEmpty()) return false;

        auto& record = m_allRecords[index.row()];
        QString oldPath = record.path;
        QFileInfo oldInfo(oldPath);

        // 如果名字没有改变，直接返回
        if (newName == oldInfo.fileName() || newName == oldInfo.completeBaseName()) return true;

        // 补全后缀
        QString suffix = oldInfo.suffix();
        if (!suffix.isEmpty() && !newName.endsWith("." + suffix, Qt::CaseInsensitive)) {
            newName += "." + suffix;
        }

        // 调用磁盘模式物理改名与索引及缩略图同步
        bool success = false;
        QString destDir = oldInfo.absolutePath();
        QString newPathStr = QDir(destDir).filePath(newName);

        if (oldPath == newPathStr) {
            success = true;
        } else if (FileOperationHelper::safeRename(oldPath, newPathStr)) {
            success = true;

            // 同步对 .QuarkMeta/disk_thumbs/ 中的哈希缩略图进行重命名
            QString oldThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(oldPath);
            QString newThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(newPathStr);

            if (QFile::exists(oldThumbHashPath)) {
                FileOperationHelper::safeRename(oldThumbHashPath, newThumbHashPath);
            }

            std::wstring oldW = oldInfo.absoluteFilePath().toStdWString();
            std::wstring newW = QDir(destDir).absoluteFilePath(newPathStr).toStdWString();

            MetadataManager::instance().renameItem(oldW, newW);
        }

        if (success) {
            QString newPath = QDir(oldInfo.absolutePath()).filePath(newName);
            record.path = newPath;
            record.filename = newName;
            
            // 更新路径映射索引
            m_pathToIndex.erase(oldPath);
            m_pathToIndex[newPath] = index.row();
            
            emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount() - 1));
            return true;
        }
        return false;
    }

    auto& record = m_allRecords[index.row()];
    QString path = record.path;
    QFileInfo fileInfo(path);
    QString parentDir = QDir::toNativeSeparators(fileInfo.absolutePath());
    QString fileName = fileInfo.fileName();

    bool metaUpdated = false;

    QuarkMetaJson jsonCache(parentDir.toStdWString());
    jsonCache.load();
    auto& cachedItems = jsonCache.items();
    
    std::wstring wFileName = fileName.toStdWString();
    if (cachedItems.find(wFileName) == cachedItems.end()) {
        ItemMeta emptyMeta;
        emptyMeta.type = record.isDir ? L"folder" : L"file";
        cachedItems[wFileName] = emptyMeta;
    }
    auto& fileMeta = cachedItems[wFileName];

    if (role == RatingRole) {
        int newRating = value.toInt();
        if (record.rating != newRating) {
            record.rating = newRating;
            fileMeta.rating = newRating;
            metaUpdated = true;
        }
    } else if (role == ColorRole) {
        QString newColor = value.toString();
        if (record.manualColor != newColor) {
            record.manualColor = newColor;
            fileMeta.color = newColor.toStdWString();
            metaUpdated = true;
        }
    } else if (role == IsLockedRole || role == PinnedRole) {
        bool pinned = value.toBool();
        if (record.pinned != pinned) {
            record.pinned = pinned;
            fileMeta.pinned = pinned;
            metaUpdated = true;
        }
    }

    if (metaUpdated) {
        jsonCache.save();
        emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount() - 1));
        return true;
    }
    return false;
}

void DiskItemModel::migrateCache(const QString& oldPath, const QString& newPath) {
    QString nativeOld = QDir::toNativeSeparators(oldPath);
    QString nativeNew = QDir::toNativeSeparators(newPath);
    QIcon* oldIconPtr = m_iconCache.take(oldPath);
    if (oldIconPtr) {
        m_iconCache.insert(nativeNew, oldIconPtr);
    }
    if (m_aspectRatios.contains(nativeOld)) {
        double ratio = m_aspectRatios.take(nativeOld);
        m_aspectRatios[nativeNew] = ratio;
    }
}

void DiskItemModel::clearCacheForFolder(const QString& folderPath) {
    QString nativeFolder = QDir::toNativeSeparators(folderPath);
    QString prefix = nativeFolder;
    if (!prefix.endsWith(QDir::separator())) prefix += QDir::separator();

    for (auto it = m_aspectRatios.begin(); it != m_aspectRatios.end(); ) {
        if (it.key() == nativeFolder || it.key().startsWith(prefix)) {
            it = m_aspectRatios.erase(it);
        } else {
            ++it;
        }
    }
}

void DiskItemModel::loadThumbnailsForRows(const QList<int>& rows) {
    if (rows.isEmpty() || CoreController::isShuttingDown()) return;

    uint64_t thisGen = m_currentGen.load(std::memory_order_relaxed);

    // 1. 严格锁定单批次只取 2 张！
    QStringList pathsToLoad;
    for (int r : rows) {
        if (pathsToLoad.size() >= 2) break; // 🚨 物理红线：严格死锁 2 张！

        if (r < 0 || r >= static_cast<int>(m_allRecords.size())) continue;
        const auto& rec = m_allRecords[r];
        if (rec.isDir || !UiHelper::isGraphicsFile(rec.suffix)) continue;

        QString path = rec.path;
        if (m_iconCache.contains(path) || m_requestedPaths.contains(path)) continue;

        m_requestedPaths.insert(path);
        pathsToLoad << path;
    }

    // 如果视口内没有需要加载的，直接退出
    if (pathsToLoad.isEmpty()) return;

    std::shared_ptr<QuarkMeta::CancellationToken> token;
    {
        QMutexLocker locker(&m_genTokenMutex);
        auto it = m_genTokens.find(thisGen);
        if (it == m_genTokens.end()) {
            token = std::make_shared<QuarkMeta::CancellationToken>();
            m_genTokens[thisGen] = token;
        } else {
            token = it.value();
        }
    }

    QPointer<DiskItemModel> weakThis(this);

    // 2. 并发处理这 2 张 (使用专属独立低优先级线程池)
    for (const QString& path : pathsToLoad) {
        thumbnailPool()->start([weakThis, path, thisGen, token]() {
            if (!weakThis || weakThis->currentGeneration() != thisGen || (token && token->isCanceled()) || CoreController::isShuttingDown()) return;

            DiskMediaExtractor::ExtractResult res = DiskMediaExtractor::getCapsuleExtractResult(path, 512, token);

            if (!weakThis || weakThis->currentGeneration() != thisGen || (token && token->isCanceled()) || CoreController::isShuttingDown()) return;

            QMetaObject::invokeMethod(weakThis.data(), [weakThis, path, res, thisGen]() {
                if (weakThis && weakThis->currentGeneration() == thisGen) {
                    QIcon icon = res.thumbnail512.isNull() ? ShellIconManager::getFileIcon(path, 128) : QIcon(QPixmap::fromImage(res.thumbnail512));
                    weakThis->m_iconCache.insert(path, new QIcon(icon));

                    auto it = weakThis->m_pathToIndex.find(path);
                    if (it != weakThis->m_pathToIndex.end()) {
                        int rIdx = it->second;
                        if (rIdx >= 0 && rIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                            auto& rec = weakThis->m_allRecords[rIdx];
                            if (res.originalSize.isValid() && res.originalSize.width() > 0) {
                                rec.width = res.originalSize.width();
                                rec.height = res.originalSize.height();
                                weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = (double)rec.width / rec.height;
                            } else if (!res.thumbnail512.isNull()) {
                                weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = (double)res.thumbnail512.width() / res.thumbnail512.height();
                            } else {
                                weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = -1.0;
                            }
                        }

                        weakThis->m_requestedPaths.remove(path);

                        if (weakThis->isSuspended()) {
                            weakThis->m_pendingUpdateRows.insert(rIdx);
                        } else {
                            emit weakThis->dataChanged(weakThis->index(rIdx, 0), weakThis->index(rIdx, weakThis->columnCount() - 1),
                                                      {Qt::DecorationRole, Qt::DisplayRole, AspectRatioRole, HasThumbnailRole});
                        }
                    }

                    // 🚨【核心自驱动接力】：解完这一张后，如果还没切走目录，自动触发一个 20ms 延时去接力解下 2 张！
                    auto* panel = qobject_cast<ContentPanel*>(weakThis->parent());
                    if (panel && !CoreController::isShuttingDown()) {
                        QTimer::singleShot(20, panel, [panel, thisGen, weakThis]() {
                            if (weakThis && weakThis->currentGeneration() == thisGen) {
                                panel->refreshVisibleThumbnails();
                            }
                        });
                    }
                }
            }, Qt::QueuedConnection);
        });
    }
}

Qt::ItemFlags DiskItemModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return QAbstractTableModel::flags(index);
    Qt::ItemFlags f = QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled;
    if (index.column() == 0) {
        f |= Qt::ItemIsEditable; // 🚨 解封第 0 列（文件名列）的行内编辑权限！
    }
    return f;
}

QVariant DiskItemModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(m_allRecords.size())) return QVariant();

    const auto& record = m_allRecords[index.row()];
    QString path = record.path;

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
            case 0: {
                int lastSlash = std::max(path.lastIndexOf('\\'), path.lastIndexOf('/'));
                if (lastSlash == -1) return path;
                QString name = path.mid(lastSlash + 1);
                if (name.isEmpty() && path.length() >= 2 && path[1] == ':') return path; // 盘符根目录安全保护
                return name;
            }
            case 3: {
                if (record.isDir) return "-";
                if (record.width > 0 && record.height > 0) {
                    return QString("%1 x %2").arg(record.width).arg(record.height);
                }
                return "-";
            }
            case 4: {
                if (record.isDir) return "文件夹";
                int lastDot = path.lastIndexOf('.');
                return (lastDot != -1) ? path.mid(lastDot + 1).toUpper() : "";
            }
            case 5: {
                if (record.isDir) return "-";
                if (record.size < 1024) return QString::number(record.size) + " B";
                if (record.size < 1024 * 1024) return QString::number(record.size / 1024.0, 'f', 1) + " KB";
                return QString::number(record.size / (1024.0 * 1024.0), 'f', 1) + " MB";
            }
            case 6: {
                return QDateTime::fromMSecsSinceEpoch(record.mtime).toString("dd-MM-yyyy HH:mm");
            }
        }
    } else if (role == PathRole) {
        return path;
    } else if (role == TypeRole) {
        return record.isDir ? "folder" : "file";
    } else if (role == RatingRole) {
        return record.rating;
    } else if (role == ColorRole) {
        return record.manualColor;
    } else if (role == IsLockedRole || role == PinnedRole) {
        return record.pinned;
    } else if (role == EncryptedRole) {
        return record.encrypted;
    } else if (role == TagsRole) {
        return record.tags;
    } else if (role == ManagedRole) {
        return false;
    } else if (role == RegistrationProgressRole) {
        return record.registrationProgress;
    } else if (role == IsEmptyRole) {
        return record.isDir && record.isEmpty; // 磁盘模式专享：物理判断是否为空文件夹
    } else if (role == AspectRatioRole) {
        if (record.width > 0 && record.height > 0) return (double)record.width / record.height;
        double ratio = m_aspectRatios.value(QDir::toNativeSeparators(path), 1.0);
        return ratio > 0.0 ? ratio : 1.0;
    } else if (role == HasThumbnailRole) {
        static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
        if (iconOnlyExts.contains(record.suffix.toLower())) return false;
        if (UiHelper::isGraphicsFile(record.suffix)) return true;
        if (record.width > 0 && record.height > 0) return true;
        return m_aspectRatios.contains(QDir::toNativeSeparators(path)) && m_aspectRatios.value(QDir::toNativeSeparators(path)) > 0.0;
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        QString cacheKey = path;
        QIcon* cached = m_iconCache.object(cacheKey);
        if (cached) return *cached;

        QString ext = record.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext) || ext == "svg";
        
        if (isGraphic) return QIcon(); // 图形文件等待异步加载时返回空图标
        QIcon icon = ShellIconManager::getFileIconFast(path, record.isDir, ext);
        if (ShellIconManager::isIconCached(path, record.isDir, ext)) {
            m_iconCache.insert(cacheKey, new QIcon(icon));
        }
        return icon;
    }

    return QVariant();
}

bool DiskItemModel::isSuspended() const {
    auto* cp = qobject_cast<ContentPanel*>(parent());
    return cp && cp->isContextMenuActive();
}

void DiskItemModel::flushPendingUpdates() {
    if (m_pendingUpdateRows.isEmpty()) return;
    for (int rIdx : m_pendingUpdateRows) {
        emit dataChanged(index(rIdx, 0), index(rIdx, 0), {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
    }
    m_pendingUpdateRows.clear();
}

void DiskItemModel::reloadThumbnailForPath(const QString& path) {
    QString nPath = QDir::toNativeSeparators(path);
    m_iconCache.remove(nPath);
    m_iconCache.remove(path);
    m_aspectRatios.remove(nPath);
    m_requestedPaths.remove(nPath);
    m_requestedPaths.remove(path);

    auto it = m_pathToIndex.find(nPath);
    if (it != m_pathToIndex.end()) {
        int rIdx = it->second;
        // 重新异步加载该行的缩略图
        loadThumbnailsForRows({rIdx});
        emit dataChanged(index(rIdx, 0), index(rIdx, 0), {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
    }
}