#include "DiskItemModel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "ThumbnailPipelineService.h"
#include "ModelContract.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QThreadPool>
#include "../../meta/QuarkMetaJson.h"
#include "../../meta/MetadataDefs.h"
#include "CoreController.h"
#include "DiskMediaExtractor.h"
#include "FileOperationHelper.h"
#include "MetadataManager.h"
#include "DriveMetaDao.h"
#include "../../core/LastOperationManager.h"

namespace QuarkMeta {

QThreadPool* DiskItemModel::thumbnailPool() {
    static QThreadPool pool;
    static std::once_flag flag;
    std::call_once(flag, []() {
        pool.setMaxThreadCount(qMax(2, QThread::idealThreadCount() / 2));
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
    return static_cast<int>(FileListColumn::Count);
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
    incrementGeneration();
    beginResetModel();
    m_allRecords = records;
    m_pathToIndex.clear();
    m_requestedPaths.clear();
    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        m_pathToIndex[m_allRecords[i].path] = i;
    }
    m_iconCache.setMaxCost(qMax(500, static_cast<int>(m_allRecords.size()) + 50));
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

        std::unordered_map<std::wstring, std::pair<int, int>> dimMap;
        std::vector<std::pair<QString, QSize>> resolvedSizes;

        for (const auto& target : targets) {
            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

            QSize sz = DiskMediaExtractor::fastExtractImageSize(target.path);
            if (sz.isValid() && sz.width() > 0) {
                QFileInfo fi(target.path);
                dimMap[fi.fileName().toStdWString()] = {sz.width(), sz.height()};
                resolvedSizes.push_back({target.path, sz});
            }
        }

        if (dimMap.empty() || !weakThis || weakThis->currentGeneration() != thisGen) return;

        QFileInfo firstFi(targets.front().path);
        QString parentDir = QDir::toNativeSeparators(firstFi.absolutePath());

        {
            std::lock_guard<std::mutex> lock(DiskMediaExtractor::s_jsonSaveMutex);
            QuarkMetaJson jsonCache(parentDir.toStdWString());
            jsonCache.load();
            auto& cachedItems = jsonCache.items();

            for (const auto& pair : dimMap) {
                const std::wstring& fileName = pair.first;
                const auto& dims = pair.second;
                if (cachedItems.find(fileName) == cachedItems.end()) {
                    ItemMeta emptyMeta;
                    emptyMeta.type = L"file";
                    cachedItems[fileName] = emptyMeta;
                }
                auto& fileMeta = cachedItems[fileName];
                fileMeta.width = dims.first;
                fileMeta.height = dims.second;
            }
            jsonCache.save();
        }

        QMetaObject::invokeMethod(weakThis.data(), [weakThis, resolvedSizes = std::move(resolvedSizes), thisGen]() {
            if (!weakThis || weakThis->currentGeneration() != thisGen) return;

            for (const auto& item : resolvedSizes) {
                const QString& path = item.first;
                const QSize& sz = item.second;

                auto it = weakThis->m_pathToIndex.find(path);
                if (it != weakThis->m_pathToIndex.end()) {
                    int rIdx = it->second;
                    if (rIdx >= 0 && rIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                        auto& rec = weakThis->m_allRecords[rIdx];
                        rec.width = sz.width();
                        rec.height = sz.height();
                        weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = (double)sz.width() / sz.height();
                    }
                }
            }

            if (!weakThis->m_allRecords.empty()) {
                emit weakThis->dataChanged(
                    weakThis->index(0, 0),
                    weakThis->index(static_cast<int>(weakThis->m_allRecords.size()) - 1, weakThis->columnCount() - 1),
                    {Qt::DisplayRole, AspectRatioRole}
                );
            }
        }, Qt::QueuedConnection);
    });
}

void DiskItemModel::clear() {
    incrementGeneration();
    beginResetModel();
    m_allRecords.clear();
    m_pathToIndex.clear();
    m_requestedPaths.clear();
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

            // 针对盘符的元数据刷新
            if (fileInfo.isRoot() || nPath.endsWith(":\\") || nPath.endsWith(":/") || (nPath.length() == 2 && nPath.endsWith(':'))) {
                std::wstring normWPath = MetadataManager::normalizePath(nPath.toStdWString());
                auto driveRec = DriveMetaDao::getDriveMeta(normWPath);
                record.rating = driveRec.rating;
                record.manualColor = QString::fromStdWString(driveRec.color);
                record.pinned = driveRec.pinned;
                record.note = QString::fromStdWString(driveRec.note);
                record.url = QString::fromStdWString(driveRec.url);
                emit dataChanged(index(i, 0), index(i, columnCount() - 1));
                return;
            }

            // 🚀【核心根治】：直接从内存缓存真理源 MetadataManager 读取实时最新数据，彻底消灭读脏盘与时序竞态！
            RuntimeMeta meta = MetadataManager::instance().getMeta(nPath.toStdWString());

            record.rating = meta.rating;
            record.manualColor = QString::fromStdWString(meta.manualColor);
            record.pinned = meta.pinned;
            record.note = QString::fromStdWString(meta.note);
            record.url = QString::fromStdWString(meta.url);
            record.tags = meta.tags;
            record.width = meta.width;
            record.height = meta.height;
            record.autoColor = QString::fromStdWString(meta.autoColor);
            record.added_at = meta.added_at;

            record.palettes.clear();
            for (const auto& pe : meta.palettes) {
                record.palettes.push_back({pe.color, pe.ratio});
            }

            emit dataChanged(index(i, 0), index(i, columnCount() - 1));
        }
    }
}

bool DiskItemModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() >= static_cast<int>(m_allRecords.size())) return false;

    // 1. 处理 F2 / 右键菜单行内重命名提交
    if (role == Qt::EditRole && index.column() == 0) {
        QString newName = value.toString().trimmed();
        if (newName.isEmpty()) return false;

        auto& record = m_allRecords[index.row()];
        QString oldPath = record.path;
        QFileInfo oldInfo(oldPath);

        if (newName == oldInfo.fileName() || newName == oldInfo.completeBaseName()) return true;

        QString suffix = oldInfo.suffix();
        if (!suffix.isEmpty() && !newName.endsWith("." + suffix, Qt::CaseInsensitive)) {
            newName += "." + suffix;
        }

        bool success = false;
        QString destDir = oldInfo.absolutePath();
        QString newPathStr = QDir(destDir).filePath(newName);

        if (oldPath == newPathStr) {
            success = true;
        } else if (FileOperationHelper::safeRename(oldPath, newPathStr)) {
            success = true;

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

    // 2. 物理驱动器/盘符根目录（如 C:/, D:/）
    bool isDriveRoot = fileInfo.isRoot() || path.endsWith(":\\") || path.endsWith(":/") || (path.length() == 2 && path.endsWith(':'));
    if (isDriveRoot) {
        std::wstring normWPath = MetadataManager::normalizePath(path.toStdWString());
        DriveMetaRecord driveRec = DriveMetaDao::getDriveMeta(normWPath);
        bool driveUpdated = false;

        if (role == RatingRole) {
            int newRating = value.toInt();
            if (record.rating != newRating) {
                record.rating = newRating;
                driveRec.rating = newRating;
                driveUpdated = true;
                LastOperationManager::instance().recordSetRating(newRating);
            }
        } else if (role == ColorRole) {
            QString newColor = value.toString();
            if (record.manualColor != newColor) {
                record.manualColor = newColor;
                driveRec.color = newColor.toStdWString();
                driveUpdated = true;
                LastOperationManager::instance().recordSetColor(newColor);
            }
        } else if (role == IsLockedRole || role == PinnedRole) {
            bool pinned = value.toBool();
            if (record.pinned != pinned) {
                record.pinned = pinned;
                driveRec.pinned = pinned;
                driveUpdated = true;
            }
        }

        if (driveUpdated) {
            DriveMetaDao::saveDriveMeta(driveRec);
            emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount() - 1));
            return true;
        }
        return false;
    }

    // 3. 常规普通文件与目录：统一通过 MetadataManager 内存门面更新
    std::wstring wpath = path.toStdWString();
    bool metaUpdated = false;

    if (role == RatingRole) {
        int newRating = value.toInt();
        if (record.rating != newRating) {
            record.rating = newRating;
            MetadataManager::instance().setRating(wpath, newRating, true);
            metaUpdated = true;
            LastOperationManager::instance().recordSetRating(newRating);
        }
    } else if (role == ColorRole) {
        QString newColor = value.toString();
        if (record.manualColor != newColor) {
            record.manualColor = newColor;
            MetadataManager::instance().setColor(wpath, newColor.toStdWString(), true);
            metaUpdated = true;
            LastOperationManager::instance().recordSetColor(newColor);
        }
    } else if (role == IsLockedRole || role == PinnedRole) {
        bool pinned = value.toBool();
        if (record.pinned != pinned) {
            record.pinned = pinned;
            MetadataManager::instance().setPinned(wpath, pinned, true);
            metaUpdated = true;
        }
    } else if (role == TagsRole) {
        QStringList newTags = value.toStringList();
        record.tags = newTags;
        MetadataManager::instance().setTags(wpath, newTags, true);
        metaUpdated = true;
        LastOperationManager::instance().recordPasteTags(newTags);
    }

    if (metaUpdated) {
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

    QStringList pathsToLoad;
    for (int r : rows) {
        if (r < 0 || r >= static_cast<int>(m_allRecords.size())) continue;
        const auto& rec = m_allRecords[r];

        QString ext = rec.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        if (rec.isDir || !isGraphic) continue;

        QString path = rec.path;
        if (m_iconCache.contains(path) || m_requestedPaths.contains(path)) continue;

        m_requestedPaths.insert(path);
        pathsToLoad << path;
    }

    if (pathsToLoad.isEmpty()) return;

    QPointer<DiskItemModel> weakThis(this);
    ThumbnailPipelineService::instance().loadBatchAsync(pathsToLoad, 230, [weakThis, thisGen](const QString& path, const QPixmap& pixmap) {
        if (!weakThis || weakThis->currentGeneration() != thisGen) return;

        weakThis->m_requestedPaths.remove(path);
        if (!pixmap.isNull()) {
            QIcon icon(pixmap);
            weakThis->m_iconCache.insert(path, new QIcon(icon));
            double ar = (double)pixmap.width() / pixmap.height();
            weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = ar;

            auto it = weakThis->m_pathToIndex.find(path);
            if (it != weakThis->m_pathToIndex.end()) {
                int rIdx = it->second;
                emit weakThis->dataChanged(weakThis->index(rIdx, 0), weakThis->index(rIdx, 0), 
                                          {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
                emit weakThis->thumbnailLoaded(rIdx);
            }
        }
    });
}

Qt::ItemFlags DiskItemModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return QAbstractTableModel::flags(index);
    Qt::ItemFlags f = QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled;
    if (index.column() == 0) {
        f |= Qt::ItemIsEditable;
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
                if (name.isEmpty() && path.length() >= 2 && path[1] == ':') return path;
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
        // 如果 record.tags 为空，尝试从 MetadataManager 读取最新数据
        if (record.tags.isEmpty()) {
            std::wstring wpath = path.toStdWString();
            RuntimeMeta meta = MetadataManager::instance().getMeta(wpath);
            if (!meta.tags.isEmpty()) {
                return meta.tags;
            }
        }
        return record.tags;
    } else if (role == NoteRole) {
        return record.note;
    } else if (role == UrlRole) {
        return record.url;
    } else if (role == IsEmptyRole) {
        return record.isDir && record.isEmpty;
    } else if (role == AspectRatioRole) {
        if (record.width > 0 && record.height > 0) return (double)record.width / record.height;
        double ratio = m_aspectRatios.value(QDir::toNativeSeparators(path), 1.0);
        return ratio > 0.0 ? ratio : 1.0;
    } else if (role == HasThumbnailRole) {
        static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
        QString ext = record.suffix.toLower();
        if (iconOnlyExts.contains(ext)) return false;
        if (UiHelper::isGraphicsFile(ext)) return true;
        if (record.width > 0 && record.height > 0) return true;
        return m_aspectRatios.contains(QDir::toNativeSeparators(path)) && m_aspectRatios.value(QDir::toNativeSeparators(path)) > 0.0;
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        QString cacheKey = path;
        QIcon* cached = m_iconCache.object(cacheKey);
        if (cached) return *cached;

        QString ext = record.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        
        if (isGraphic) return QIcon();
        QIcon icon = ShellIconManager::getFileIconFast(path, record.isDir, ext);
        if (ShellIconManager::isIconCached(path, record.isDir, ext)) {
            m_iconCache.insert(cacheKey, new QIcon(icon));
        }
        return icon;
    }

    return QVariant();
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
        loadThumbnailsForRows({rIdx});
        emit dataChanged(
            index(rIdx, 0), 
            index(rIdx, columnCount() - 1), 
            {Qt::DecorationRole, Qt::DisplayRole, AspectRatioRole, HasThumbnailRole}
        );
    }
}

} // namespace QuarkMeta