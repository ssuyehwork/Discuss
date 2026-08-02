# 双轨物理数据源 100% 隔离重构与高清预览搜寻 —— Modification_Plan-21.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
本方案专注于解决系统“模式隔离泄漏”以及 AI（Illustrator）等复杂矢量的“假图标预览与当场生成失败”漏洞：
1. **模式混淆漏洞**：内容面板在加载数据时，错误的让“磁盘目录模式（DiskNav，0）”和“内存数据库模式（Category/SystemCategory，1）”混叠在了同一个 Model 里（对应用户原话：“鼠标点击侧边栏分类，意味着进入了内存模式，走的路线模块就应该是内存模式的路线，当鼠标点击目录导航时，走的时磁盘路线，一个是0，另一个是1，为什么偏要去混淆？”）。这导致在磁盘模式下，`.arc` 文件夹也同样被解包展示、查询 SQLite 获取元数据（如评分和手动颜色）以及抓取包内缩略图。
2. **高清 AI 预览提取限制**：在 `.ai` 文件的内嵌 JPEG 预览提取逻辑中，硬编码 5MB 限制（`5 * 1024 * 1024`）导致大文件当场无法生成缩略图，而后来的 Windows 默认大图标兜底导致用户看到通用的“Ai 软件图标”而不是真实的卡片内容（对应用户原话：“Adobe Bridge 显示的是这个 AI 文件画面内容本身的缩略图，而 ArcMeta 显示的只是一个通用的 'Ai' 软件图标，不是文件内容”）。

## 2. 问题定位
- **问题 A（模式隔离泄漏）**：在 `ItemRecord::create` 中：
  ```cpp
  bool isArcPath = (wPath.find(L".arc") != std::wstring::npos);
  if (isArcPath) {
      meta = MetadataManager::instance().getMeta(wPath); // ❌ 磁盘模式下不应该访问数据库
  }
  ```
  以及在 `ContentPanel` 的数据主模型中，没有物理隔离 0 和 1 数据源，导致磁盘扫描也触发了解包和高亮元数据渲染。
- **问题 B（大文件限制与虚假图标）**：
  `extractEmbeddedAiPreview` 读取被写死在前 5MB，遇到复杂矢量时，起始标记 `\xFF\xD8\xFF` 通常存储在 5MB 偏移量后，造成读取失败。
  在 `getImageForAnalysis` 中，`getShellThumbnail` 无情地将系统给 `.ai` / `.psd` / `.eps` 配备的软件关联图当做内容缩略图写入 `.arc` 包中，导致用户看见不正确的内容画面。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 鼠标点击侧边栏分类，意味着进入了内存模式，走的路线模块就应该是内存模式的路线，当鼠标点击目录导航时，走的时磁盘路线，一个是0，另一个是1，为什么偏要去混淆？ (对应用户原话) | 4.1 - 4.6 节：重塑模型层和视图路由，将内存模型 `LibraryAssetModel` 与磁盘模型 `DiskItemModel` 彻底拆分，由 `ContentPanel` 最前端通过多态无缝分流。 | ✅ 一致 |
| 2    | 磁盘目录模式：... 哪怕打开的是 ArcMeta.Library_[盘符] 这个托管库根目录本身，看到的也就是里面原本的文件夹结构（包括所有 .arc 容器），跟打开任何一个普通文件夹没有区别。 (对应用户原话) | 4.4 节：在 `DiskItemModel` 中，彻底掐断一切关于 `.arc` 的解包、数据库查询、图标重定向与缩略图穿透提取，使其在磁盘模式下保持原本文件夹形态。 | ✅ 一致 |
| 3    | Adobe Bridge 显示的是这个 AI 文件画面内容本身的缩略图，而 ArcMeta 显示的只是一个通用的 'Ai' 软件图标，不是文件内容。 (对应用户原话) | 4.8 节：重写 `extractEmbeddedAiPreview`（游标扫描无 5MB 限制）与 `getImageForAnalysis`（对 `.ai/.psd/.eps` 在提取失败时拒绝将系统软件默认图标作伪占位符保存）。 | ✅ 一致 |

## 4. 详细解决方案

本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的 Git merge diff 代码块进行物理替换，不得做任何自由发挥或脑补改动。

### 4.1 引入 `isFromMemory` 精准路由并重构 `src/core/ItemRecord.h / .cpp`
在 `ItemRecord::create` 中加入 `bool isFromMemory` 参数：

在 `src/core/ItemRecord.h` 中：
```
<<<<<<< SEARCH
    static ItemRecord create(const QString& path, const RuntimeMeta* providedMeta = nullptr);
=======
    static ItemRecord create(const QString& path, const RuntimeMeta* providedMeta = nullptr, bool isFromMemory = false);
>>>>>>> REPLACE
```

在 `src/core/ItemRecord.cpp` 中：
```
<<<<<<< SEARCH
ItemRecord ItemRecord::create(const QString& path, const RuntimeMeta* providedMeta) {
    ItemRecord r;
    std::wstring wPath = MetadataManager::normalizePath(path.toStdWString());
    QString nPath = QString::fromStdWString(wPath);

    // 1. 物理属性采样 (零 I/O 核心)
    // 🚨 [双轨不隔离违规点-1 物理隔离修复]: 磁盘导航模式下不共享、不穿透读取资源库数据库。
    // 如果没有 providedMeta，且不是 .arc 素材包路径，绝不穿透 MetadataManager。
    RuntimeMeta meta;
    bool isArcPath = (wPath.find(L".arc") != std::wstring::npos);
    if (providedMeta) {
        meta = *providedMeta;
    } else if (isArcPath) {
        meta = MetadataManager::instance().getMeta(wPath);
    }

    // Plan-124: 只有在内存缓存缺失物理时间戳时，才触发 fetchWinApiMetadataDirect
    if (meta.folderId.empty() || (meta.ctime == 0 && meta.mtime == 0)) {
        std::string fid;
        long long size = 0, ctime = 0, mtime = 0, atime = 0;
        MetadataManager::fetchWinApiMetadataDirect(wPath, fid, nullptr, &size, nullptr, &ctime, &mtime, &atime);
        r.size = size;
        r.ctime = ctime;
        r.mtime = mtime;
        r.atime = atime;

        // 🚨 内存数据库模式唯一ID体系重构：优先解析和提取 Base36 ID，如果是磁盘普通路径，则复用本轮采样已取得 of fid，彻底消除双重 I/O 冗余
        size_t pos = wPath.find(L".arc");
        if (pos != std::wstring::npos) {
            r.folderId = MetadataManager::instance().getFolderIdSync(wPath);
        } else {
            r.folderId = fid;
        }

        r.isDir = QFileInfo(nPath).isDir();
    } else {
        r.size = meta.fileSize;
        r.ctime = meta.ctime;
        r.mtime = meta.mtime;
        r.atime = meta.atime;
        r.folderId = meta.folderId;
        r.isDir = meta.isFolder;
    }

    r.path = nPath;
    {
        int lastSlash = nPath.lastIndexOf('\\');
        if (lastSlash == -1) lastSlash = nPath.lastIndexOf('/');
        r.filename = (lastSlash != -1) ? nPath.mid(lastSlash + 1) : nPath;
    }

    // 2. 核心元数据注入 (确保 width/height/palettes 物理对齐)
    if (providedMeta || isArcPath) {
        ItemRecord::fromMetadata(r, meta);
    } else {
        r.rating = 0;
        r.isManaged = false;
        r.pinned = false;
        r.encrypted = false;
        r.width = 0;
        r.height = 0;
        r.added_at = 0;
    }

    if (r.isDir) {
        // 从数据库加载持久化的进度值
        r.registrationProgress = MetadataManager::instance().getProgressFromDb(wPath);

        // 严格遵循规则：空文件夹判定只应用于磁盘模式！
        if (providedMeta || meta.isManaged) {
            r.isEmpty = false; // 镜像/托管模式下强行禁用空文件夹逻辑
        } else {
            QDir sub(nPath);
            r.isEmpty = sub.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty(); // 仅磁盘模式生效
        }
        r.suffix = ""; // 文件夹不应有扩展名后缀
    } else {
        int lastDot = nPath.lastIndexOf('.');
        r.suffix = (lastDot != -1) ? nPath.mid(lastDot + 1).toLower() : "";
    }
    return r;
}
=======
ItemRecord ItemRecord::create(const QString& path, const RuntimeMeta* providedMeta, bool isFromMemory) {
    ItemRecord r;
    std::wstring wPath = MetadataManager::normalizePath(path.toStdWString());
    QString nPath = QString::fromStdWString(wPath);

    // 1. 物理属性采样 (零 I/O 核心)
    // 🚨 [双轨不隔离极简解耦重构]: 磁盘导航模式下（isFromMemory == false）100% 拒绝穿透去读受控库数据库！
    RuntimeMeta meta;
    bool isArcPath = isFromMemory && (wPath.find(L".arc") != std::wstring::npos);
    if (providedMeta) {
        meta = *providedMeta;
    } else if (isArcPath) {
        meta = MetadataManager::instance().getMeta(wPath);
    }

    if (meta.folderId.empty() || (meta.ctime == 0 && meta.mtime == 0)) {
        std::string fid;
        long long size = 0, ctime = 0, mtime = 0, atime = 0;
        MetadataManager::fetchWinApiMetadataDirect(wPath, fid, nullptr, &size, nullptr, &ctime, &mtime, &atime);
        r.size = size;
        r.ctime = ctime;
        r.mtime = mtime;
        r.atime = atime;

        if (isFromMemory && wPath.find(L".arc") != std::wstring::npos) {
            r.folderId = MetadataManager::instance().getFolderIdSync(wPath);
        } else {
            r.folderId = fid;
        }

        r.isDir = QFileInfo(nPath).isDir();
    } else {
        r.size = meta.fileSize;
        r.ctime = meta.ctime;
        r.mtime = meta.mtime;
        r.atime = meta.atime;
        r.folderId = meta.folderId;
        r.isDir = meta.isFolder;
    }

    r.path = nPath;
    {
        int lastSlash = nPath.lastIndexOf('\\');
        if (lastSlash == -1) lastSlash = nPath.lastIndexOf('/');
        r.filename = (lastSlash != -1) ? nPath.mid(lastSlash + 1) : nPath;
    }

    // 2. 核心元数据注入 (确保 width/height/palettes 物理对齐)
    if (providedMeta || isArcPath) {
        ItemRecord::fromMetadata(r, meta);
    } else {
        r.rating = 0;
        r.isManaged = false;
        r.pinned = false;
        r.encrypted = false;
        r.width = 0;
        r.height = 0;
        r.added_at = 0;
    }

    if (r.isDir) {
        // 从数据库加载持久化的进度值
        if (isFromMemory) {
            r.registrationProgress = MetadataManager::instance().getProgressFromDb(wPath);
        } else {
            r.registrationProgress = -1.0;
        }

        if (providedMeta || (isFromMemory && meta.isManaged)) {
            r.isEmpty = false;
        } else {
            QDir sub(nPath);
            r.isEmpty = sub.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty(); // 仅磁盘模式生效
        }
        r.suffix = "";
    } else {
        int lastDot = nPath.lastIndexOf('.');
        r.suffix = (lastDot != -1) ? nPath.mid(lastDot + 1).toLower() : "";
    }

    // 3. 内存模式下彻底穿透包内查找主素材文件，将其真实文件名与扩展名注入 ItemRecord
    if (isFromMemory && r.isDir && nPath.endsWith(".arc", Qt::CaseInsensitive)) {
        QDir arcDir(nPath);
        QFileInfoList files = arcDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
        for (const QFileInfo& fi : files) {
            QString fn = fi.fileName();
            if (fn.endsWith("_thumbnail.png", Qt::CaseInsensitive)) continue;
            if (fn.compare("metadata.json", Qt::CaseInsensitive) == 0) continue;
            if (fn.compare("metadata.scch", Qt::CaseInsensitive) == 0) continue;

            r.filename = fn;
            r.suffix = fi.suffix().toLower();
            break;
        }
    }

    return r;
}
>>>>>>> REPLACE
```

### 4.2 修正 `DiskScanService` 扫盘与 `CategoryLoadService` 资产拉取的对位传参

在 `src/ui/DiskScanService.cpp` 与 `src/core/DiskScanService.cpp` 中（磁盘模式强制传 `false`）：
```
<<<<<<< SEARCH
            QString absPath = info.absoluteFilePath();
            ItemRecord itemRec = ItemRecord::create(absPath);
=======
            QString absPath = info.absoluteFilePath();
            ItemRecord itemRec = ItemRecord::create(absPath, nullptr, false);
>>>>>>> REPLACE
```

在 `src/ui/CategoryLoadService.cpp` 与 `src/core/CategoryLoadService.cpp` 中（内存模式强制传 `true`）：
```
<<<<<<< SEARCH
        if (!wPath.empty()) {
            allRecords.push_back(ItemRecord::create(QString::fromStdWString(wPath)));
        }
    }

    return allRecords;
}

std::vector<ItemRecord> CategoryLoadService::loadPathItems(const QStringList& paths) {
    std::vector<ItemRecord> records;
    records.reserve(static_cast<int>(paths.size()));
    for (const QString& p : paths) {
        if (!p.isEmpty()) {
            records.push_back(ItemRecord::create(p));
        }
    }
    return records;
}
=======
        if (!wPath.empty()) {
            allRecords.push_back(ItemRecord::create(QString::fromStdWString(wPath), nullptr, true));
        }
    }

    return allRecords;
}

std::vector<ItemRecord> CategoryLoadService::loadPathItems(const QStringList& paths) {
    std::vector<ItemRecord> records;
    records.reserve(static_cast<int>(paths.size()));
    for (const QString& p : paths) {
        if (!p.isEmpty()) {
            records.push_back(ItemRecord::create(p, nullptr, true));
        }
    }
    return records;
}
>>>>>>> REPLACE
```

### 4.3 重构 `src/ui/models/ItemModelBase.h` 基类接口合约
添加必要的纯虚函数生命周期契约，使其成为 `DiskItemModel` 与 `LibraryAssetModel` 的通用接口：

```
<<<<<<< SEARCH
class ItemModelBase : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit ItemModelBase(QObject* parent = nullptr) : QAbstractTableModel(parent) {}
    virtual ~ItemModelBase() override = default;

    // 只暴露 allRecords() 接口，供 FilterProxyModel 统一操作
    virtual const std::vector<ArcMeta::ItemRecord>& allRecords() const = 0;
};
=======
class ItemModelBase : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit ItemModelBase(QObject* parent = nullptr) : QAbstractTableModel(parent) {}
    virtual ~ItemModelBase() override = default;

    virtual const std::vector<ArcMeta::ItemRecord>& allRecords() const = 0;
    virtual void setRecords(const std::vector<ArcMeta::ItemRecord>& records) = 0;
    virtual void clear() = 0;
    virtual void setQuery(const QString& query) = 0;
    virtual void updateRecordMetadata(const QString& path) = 0;
    virtual void loadThumbnailsForRows(const QList<int>& rows) = 0;
    virtual void migrateCache(const QString& oldPath, const QString& newPath) = 0;
    virtual void clearCacheForFolder(const QString& folderPath) = 0;
};
>>>>>>> REPLACE
```

### 4.4 完善 `src/ui/models/DiskItemModel.h / .cpp` 纯物理磁盘导航模型
该模型是 100% 纯物理，不含有解包或任何 SQLite 数据库逻辑：

在 `src/ui/models/DiskItemModel.h` 中：
```
<<<<<<< SEARCH
class DiskItemModel : public ItemModelBase {
    Q_OBJECT
public:
    explicit DiskItemModel(QObject* parent = nullptr);
    virtual ~DiskItemModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    const std::vector<ArcMeta::ItemRecord>& allRecords() const override { return m_allRecords; }
    std::vector<ArcMeta::ItemRecord>& mutableRecords() { return m_allRecords; }

protected:
    std::vector<ArcMeta::ItemRecord> m_allRecords;
    mutable QCache<QString, QIcon> m_iconCache;
    mutable QMap<QString, double> m_aspectRatios;
};
=======
#include <unordered_map>
#include <QSet>

namespace ArcMeta {
    struct QStringHash {
        size_t operator()(const QString& key) const {
            return qHash(key);
        }
    };
}

class DiskItemModel : public ItemModelBase {
    Q_OBJECT
public:
    explicit DiskItemModel(QObject* parent = nullptr);
    virtual ~DiskItemModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    const std::vector<ArcMeta::ItemRecord>& allRecords() const override { return m_allRecords; }
    void setRecords(const std::vector<ArcMeta::ItemRecord>& records) override;
    void clear() override;
    void setQuery(const QString& query) override { m_query = query; }
    void updateRecordMetadata(const QString& path) override;
    void loadThumbnailsForRows(const QList<int>& rows) override;
    void migrateCache(const QString& oldPath, const QString& newPath) override;
    void clearCacheForFolder(const QString& folderPath) override;

protected:
    std::vector<ArcMeta::ItemRecord> m_allRecords;
    std::unordered_map<QString, int, ArcMeta::QStringHash> m_pathToIndex;
    mutable QCache<QString, QIcon> m_iconCache;
    mutable QSet<QString> m_requestedIcons;
    mutable QMap<QString, double> m_aspectRatios;
    QString m_query;
};
>>>>>>> REPLACE
```

在 `src/ui/models/DiskItemModel.cpp` 中（实现其全部接口，对 `.arc` 采取 100% 盲区不读取原则）：
```
<<<<<<< SEARCH
#include "DiskItemModel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "ModelContract.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>

using namespace ArcMeta;

DiskItemModel::DiskItemModel(QObject* parent) : ItemModelBase(parent) {
    m_iconCache.setMaxCost(500);
}

DiskItemModel::~DiskItemModel() {}

int DiskItemModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_allRecords.size());
}

int DiskItemModel::columnCount(const QModelIndex&) const {
    return 7; // 文件基本属性列：名、大小、格式、修改时间等数
}

QVariant DiskItemModel::data(const QModelIndex& index, int role) const {
=======
#include "DiskItemModel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "ModelContract.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QtConcurrent>

using namespace ArcMeta;

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

void DiskItemModel::setRecords(const std::vector<ItemRecord>& records) {
    beginResetModel();
    m_allRecords = records;
    m_pathToIndex.clear();
    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        m_pathToIndex[m_allRecords[i].path] = i;
    }
    m_iconCache.setMaxCost(qMax(500, static_cast<int>(m_allRecords.size()) + 50));
    m_requestedIcons.clear();
    endResetModel();
}

void DiskItemModel::clear() {
    beginResetModel();
    m_allRecords.clear();
    m_pathToIndex.clear();
    m_query.clear();
    m_requestedIcons.clear();
    m_aspectRatios.clear();
    endResetModel();
}

void DiskItemModel::updateRecordMetadata(const QString& path) {
    // 磁盘物理模型拒绝响应任何逻辑库元数据局部更新，静默放行
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
    // 磁盘模型：缩略图提取流 100% 盲区拦截：对 .arc、文件夹和非标准图形直接忽略，不生成任何 _thumbnail.png，不穿透
    std::vector<std::pair<QString, QString>> newQueue;
    for (int r : rows) {
        if (r < 0 || r >= static_cast<int>(m_allRecords.size())) continue;
        const auto& rec = m_allRecords[r];
        if (rec.isDir) continue; // 绝对不穿透文件夹

        QString path = rec.path;
        if (!UiHelper::isGraphicsFile(rec.suffix)) continue;
        if (m_iconCache.contains(path)) continue;
        newQueue.push_back({path, path});
    }

    if (newQueue.empty()) return;

    QPointer<DiskItemModel> weakThis(this);
    (void)QtConcurrent::run([weakThis, newQueue]() {
        for (const auto& task : newQueue) {
            if (!weakThis) break;
            QString path = task.first;
            QImage img = ShellIconManager::getShellThumbnail(path, 128);
            double ar = 1.0;
            bool hasThumb = false;
            if (!img.isNull()) {
                ar = (double)img.width() / img.height();
                hasThumb = true;
            }

            QMetaObject::invokeMethod(weakThis.data(), [weakThis, path, img, ar, hasThumb]() {
                if (weakThis) {
                    QIcon icon = img.isNull() ? ShellIconManager::getFileIcon(path, 128) : QIcon(QPixmap::fromImage(img));
                    weakThis->m_iconCache.insert(path, new QIcon(icon));
                    weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = hasThumb ? ar : -1.0;

                    auto it = weakThis->m_pathToIndex.find(path);
                    if (it != weakThis->m_pathToIndex.end()) {
                        int rIdx = it->second;
                        emit weakThis->dataChanged(weakThis->index(rIdx, 0), weakThis->index(rIdx, 0), {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
                    }
                }
            });
        }
    });
}

QVariant DiskItemModel::data(const QModelIndex& index, int role) const {
>>>>>>> REPLACE
```

### 4.5 完善 `src/ui/models/LibraryAssetModel.h / .cpp` 内存数据库受控资产模型
该模型是 100% 内存受控的，承担着解包展示及数据库互动的使命：

在 `src/ui/models/LibraryAssetModel.h` 中：
```
<<<<<<< SEARCH
class LibraryAssetModel : public ItemModelBase {
    Q_OBJECT
public:
    explicit LibraryAssetModel(QObject* parent = nullptr);
    virtual ~LibraryAssetModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    const std::vector<ArcMeta::ItemRecord>& allRecords() const override { return m_allRecords; }
    std::vector<ArcMeta::ItemRecord>& mutableRecords() { return m_allRecords; }

protected:
    std::vector<ArcMeta::ItemRecord> m_allRecords;
    mutable QCache<QString, QIcon> m_iconCache;
    mutable QMap<QString, double> m_aspectRatios;
};
=======
#include <unordered_map>
#include <QSet>

namespace ArcMeta {
    struct QStringHash;
}

class LibraryAssetModel : public ItemModelBase {
    Q_OBJECT
public:
    explicit LibraryAssetModel(QObject* parent = nullptr);
    virtual ~LibraryAssetModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    const std::vector<ArcMeta::ItemRecord>& allRecords() const override { return m_allRecords; }
    void setRecords(const std::vector<ArcMeta::ItemRecord>& records) override;
    void clear() override;
    void setQuery(const QString& query) override { m_query = query; }
    void updateRecordMetadata(const QString& path) override;
    void loadThumbnailsForRows(const QList<int>& rows) override;
    void migrateCache(const QString& oldPath, const QString& newPath) override;
    void clearCacheForFolder(const QString& folderPath) override;

signals:
    void recordRenamed(const QString& oldPath, const QString& newPath, const QString& newName);

protected:
    std::vector<ArcMeta::ItemRecord> m_allRecords;
    std::unordered_map<QString, int, ArcMeta::QStringHash> m_pathToIndex;
    mutable QCache<QString, QIcon> m_iconCache;
    mutable QSet<QString> m_requestedIcons;
    mutable QMap<QString, double> m_aspectRatios;
    mutable QCache<QString, ArcMeta::RuntimeMeta> m_metaCache;
    QString m_query;
};
>>>>>>> REPLACE
```

在 `src/ui/models/LibraryAssetModel.cpp` 中（实现其大宗数据更改接口，对 `.arc` 采取 100% 深度穿透展现）：
```
<<<<<<< SEARCH
#include "LibraryAssetModel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "ModelContract.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QFileIconProvider>

using namespace ArcMeta;

LibraryAssetModel::LibraryAssetModel(QObject* parent) : ItemModelBase(parent) {
    m_iconCache.setMaxCost(500);
}

LibraryAssetModel::~LibraryAssetModel() {}

int LibraryAssetModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_allRecords.size());
}

int LibraryAssetModel::columnCount(const QModelIndex&) const {
    return 7;
}

QVariant LibraryAssetModel::data(const QModelIndex& index, int role) const {
=======
#include "LibraryAssetModel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "ModelContract.h"
#include "../meta/MetadataManager.h"
#include "../meta/CategoryRepo.h"
#include "../core/UndoManager.h"
#include "../core/BasicCommands.h"
#include "MediaColorExtractor.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QFileIconProvider>
#include <QtConcurrent>

using namespace ArcMeta;

LibraryAssetModel::LibraryAssetModel(QObject* parent) : ItemModelBase(parent) {
    m_iconCache.setMaxCost(500);
}

LibraryAssetModel::~LibraryAssetModel() {}

int LibraryAssetModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_allRecords.size());
}

int LibraryAssetModel::columnCount(const QModelIndex&) const {
    return 7;
}

void LibraryAssetModel::setRecords(const std::vector<ItemRecord>& records) {
    beginResetModel();
    m_allRecords = records;
    m_pathToIndex.clear();
    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        m_pathToIndex[m_allRecords[i].path] = i;
    }
    m_iconCache.setMaxCost(qMax(500, static_cast<int>(m_allRecords.size()) + 50));
    m_requestedIcons.clear();
    m_metaCache.clear();
    endResetModel();
}

void LibraryAssetModel::clear() {
    beginResetModel();
    m_allRecords.clear();
    m_pathToIndex.clear();
    m_query.clear();
    m_requestedIcons.clear();
    m_aspectRatios.clear();
    m_metaCache.clear();
    endResetModel();
}

void LibraryAssetModel::updateRecordMetadata(const QString& path) {
    QString nPath = QDir::toNativeSeparators(path);
    auto it = m_pathToIndex.find(nPath);
    if (it != m_pathToIndex.end()) {
        int i = it->second;
        if (i >= 0 && i < static_cast<int>(m_allRecords.size())) {
            auto meta = MetadataManager::instance().getMeta(nPath.toStdWString());
            ItemRecord::fromMetadata(m_allRecords[i], meta);
            m_metaCache.remove(nPath);
            emit dataChanged(index(i, 0), index(i, columnCount() - 1));
        }
    }
}

void LibraryAssetModel::migrateCache(const QString& oldPath, const QString& newPath) {
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

void LibraryAssetModel::clearCacheForFolder(const QString& folderPath) {
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

bool LibraryAssetModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() >= static_cast<int>(m_allRecords.size())) return false;

    const auto& record = m_allRecords[index.row()];
    QString path = record.path;

    if (role == Qt::EditRole && index.column() == 0) {
        return false; // 内存模式重命名由 ContentPanel 统一处理
    }

    bool metaUpdated = false;
    if (role == RatingRole) {
        int oldRating = index.data(RatingRole).toInt();
        int newRating = value.toInt();
        if (oldRating != newRating) {
            if (record.isCategory) {
                auto& mutableRec = m_allRecords[index.row()];
                mutableRec.rating = newRating;
                metaUpdated = true;
            } else {
                MetadataManager::instance().setRating(path.toStdWString(), newRating);
                UndoManager::instance().pushCommand(std::make_unique<MetadataCommand>(path, MetadataCommand::Rating, oldRating, newRating));
                metaUpdated = true;
            }
        }
    } else if (role == ColorRole) {
        QString oldColor = index.data(ColorRole).toString();
        QString newColor = value.toString();
        if (oldColor != newColor) {
            auto& mutableRec = m_allRecords[index.row()];
            if (record.isCategory) {
                auto all = CategoryRepo::getAll();
                for (auto& c : all) {
                    if (c.id == record.categoryId) {
                        c.color = newColor.toUpper().toStdWString();
                        CategoryRepo::update(c);
                        if (!c.physicalPath.empty()) {
                            MetadataManager::instance().setColor(c.physicalPath, c.color, false);
                        }
                        break;
                    }
                }
                mutableRec.categoryColor = newColor;
                metaUpdated = true;
            } else {
                MetadataManager::instance().setColor(path.toStdWString(), newColor.toStdWString(), false);
                if (record.isDir) {
                    std::wstring normPath = MetadataManager::normalizePath(path.toStdWString());
                    CategoryRepo::updateCategoryColorByPath(normPath, newColor.toUpper().toStdWString());
                }
                UndoManager::instance().pushCommand(std::make_unique<MetadataCommand>(path, MetadataCommand::Color, oldColor, newColor));
                metaUpdated = true;
            }
        }
    } else if (role == IsLockedRole || role == PinnedRole) {
        bool pinned = value.toBool();
        if (record.isCategory) {
            auto all = CategoryRepo::getAll();
            for (auto& c : all) {
                if (c.id == record.categoryId) {
                    c.pinned = pinned;
                    CategoryRepo::update(c);
                    auto& mutableRec = m_allRecords[index.row()];
                    mutableRec.pinned = pinned;
                    metaUpdated = true;
                    break;
                }
            }
        } else {
            MetadataManager::instance().setPinned(path.toStdWString(), pinned);
            metaUpdated = true;
        }
    }

    if (metaUpdated) {
        if (!record.isCategory) {
            m_metaCache.remove(path);
            updateRecordMetadata(path);
        } else {
            emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount() - 1));
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::CategoryOnly);
        }
        return true;
    }
    return false;
}

void LibraryAssetModel::loadThumbnailsForRows(const QList<int>& rows) {
    // 内存模式：穿透 .arc 搜寻高清缩略图与宽高比
    std::vector<std::pair<QString, QString>> newQueue;
    for (int r : rows) {
        if (r < 0 || r >= static_cast<int>(m_allRecords.size())) continue;
        const auto& rec = m_allRecords[r];
        if (rec.isCategory) continue;

        QString path = rec.path;
        bool isArcContainer = rec.isDir && rec.path.endsWith(".arc", Qt::CaseInsensitive);
        bool needLoad = !m_iconCache.contains(path);
        if ((UiHelper::isGraphicsFile(rec.suffix) || isArcContainer) && !m_aspectRatios.contains(QDir::toNativeSeparators(path))) {
            needLoad = true;
        }
        if (needLoad) {
            newQueue.push_back({path, path});
        }
    }

    if (newQueue.empty()) return;

    QPointer<LibraryAssetModel> weakThis(this);
    (void)QtConcurrent::run([weakThis, newQueue]() {
        for (const auto& task : newQueue) {
            if (!weakThis) break;
            QString path = task.first;
            QFileInfo info(path);
            QString ext = info.suffix().toLower();

            QImage img;
            double ar = 1.0;
            bool hasThumb = false;

            if (ext == "svg") {
                QSvgRenderer renderer(path);
                if (renderer.isValid()) {
                    QImage svgImg(128, 128, QImage::Format_ARGB32);
                    svgImg.fill(Qt::transparent);
                    QPainter painter(&svgImg);
                    renderer.render(&painter);
                    img = svgImg;
                    ar = 1.0;
                    hasThumb = true;
                }
            } else if (ext == "ai") {
                img = MediaColorExtractor::extractEmbeddedAiPreview(path);
                if (!img.isNull()) {
                    ar = (double)img.width() / img.height();
                    hasThumb = true;
                } else {
                    ar = -1.0;
                    hasThumb = false;
                }
            } else if (UiHelper::isGraphicsFile(ext) && ext != "cur" && ext != "ico" && ext != "ani" && ext != "ai") {
                img = ShellIconManager::getShellThumbnail(path, 128);
                if (!img.isNull()) {
                    ar = (double)img.width() / img.height();
                    hasThumb = true;
                }
            } else if (ext == "cur" || ext == "ico" || ext == "ani") {
                ar = 1.0;
                hasThumb = false;
            } else if (ext == "arc" && info.isDir()) {
                QDir arcDir(path);
                QStringList thumbFiles = arcDir.entryList({"*_thumbnail.png"}, QDir::Files);
                if (!thumbFiles.isEmpty()) {
                    QString thumbPath = path + "/" + thumbFiles.first();
                    img = QImage(thumbPath);
                    if (!img.isNull()) {
                        ar = (double)img.width() / img.height();
                        hasThumb = true;
                    }
                }
            }

            QMetaObject::invokeMethod(weakThis.data(), [weakThis, path, img, ar, hasThumb]() {
                if (weakThis) {
                    QIcon icon;
                    if (!img.isNull()) {
                        icon = QIcon(QPixmap::fromImage(img));
                    } else {
                        QString iconTarget = path;
                        QFileInfo localInfo(path);
                        if (localInfo.suffix().toLower() == "arc" && localInfo.isDir()) {
                            QDir arcDir(path);
                            QFileInfoList files = arcDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
                            for (const QFileInfo& fi : files) {
                                QString fn = fi.fileName();
                                if (fn.endsWith("_thumbnail.png", Qt::CaseInsensitive)) continue;
                                if (fn.compare("metadata.json", Qt::CaseInsensitive) == 0) continue;
                                iconTarget = QDir::toNativeSeparators(fi.absoluteFilePath());
                                break;
                            }
                        }
                        icon = ShellIconManager::getFileIcon(iconTarget, 128);
                    }

                    weakThis->m_iconCache.insert(path, new QIcon(icon));
                    weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = hasThumb ? ar : -1.0;

                    auto it = weakThis->m_pathToIndex.find(path);
                    if (it != weakThis->m_pathToIndex.end()) {
                        int rIdx = it->second;
                        emit weakThis->dataChanged(weakThis->index(rIdx, 0), weakThis->index(rIdx, 0), {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
                    }
                }
            });
        }
    });
}

QVariant LibraryAssetModel::data(const QModelIndex& index, int role) const {
>>>>>>> REPLACE
```

### 4.6 清理 `src/ui/ContentPanel.h / .cpp` 中混杂的 `ArcMetaVirtualDbModel` 并更换为多态双轨指针
彻底消除 `ContentPanel` 内混杂了 0 和 1 的单一 `m_model`，改用在 UI 装载时实时重组路由：

在 `src/ui/ContentPanel.h` 中：
```
<<<<<<< SEARCH
    // 视图组件
    QAbstractItemView* m_gridView = nullptr;
    QTreeView* m_treeView = nullptr;
    ArcMetaVirtualDbModel* m_model = nullptr;
=======
    // 视图组件
    QAbstractItemView* m_gridView = nullptr;
    QTreeView* m_treeView = nullptr;
    DiskItemModel* m_diskModel = nullptr;       // 负责纯物理磁盘导航模型 (0)
    LibraryAssetModel* m_libraryModel = nullptr; // 负责内存托管逻辑资产模型 (1)
    ItemModelBase* m_model = nullptr;           // 当前多态激活指针合约
>>>>>>> REPLACE
```

在 `src/ui/ContentPanel.cpp` 中：

1. 修改 `ContentPanel` 构造函数，初始化两个独立模型并将 `FilterProxyModel` 的映射指向公共基类合约：
```
<<<<<<< SEARCH
    m_model = new ArcMetaVirtualDbModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_visibleTimer = new QTimer(this);
    m_visibleTimer->setSingleShot(true);
    m_visibleTimer->setInterval(100); // 100ms 黄金防抖视口延迟
    connect(m_visibleTimer, &QTimer::timeout, this, &ContentPanel::refreshVisibleThumbnails);

    // 2026-05-17 新增：当模型数据发生改变时，自动触发统计重新计算并推送至 FilterPanel
    connect(m_model, &ArcMetaVirtualDbModel::dataChanged, this, [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
        Q_UNUSED(topLeft); Q_UNUSED(bottomRight);
        if (roles.isEmpty() || roles.contains(ColorRole) || roles.contains(RatingRole) || roles.contains(TagsRole)) {
            recalculateAndEmitStats();
        }
    });

    // 🚀【方案 A 核心】：监听模型层的 recordRenamed 信号，进行增量更新与选中重新对齐，绝对不触发全量 loadDirectory
    connect(m_model, &ArcMetaVirtualDbModel::recordRenamed, this, [this](const QString& oldPath, const QString& newPath, const QString& newName) {
        Q_UNUSED(oldPath);
        this->setPendingSelectName(newName, false);

        // 通知视图重新定位并同步元数据面板状态
        this->selectAndScrollToPath(newPath);
        this->onSelectionChanged();
    });
=======
    m_diskModel = new DiskItemModel(this);
    m_libraryModel = new LibraryAssetModel(this);
    m_model = m_libraryModel; // 默认挂载受控逻辑库模型

    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_visibleTimer = new QTimer(this);
    m_visibleTimer->setSingleShot(true);
    m_visibleTimer->setInterval(100);
    connect(m_visibleTimer, &QTimer::timeout, this, &ContentPanel::refreshVisibleThumbnails);

    auto onDataChanged = [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
        Q_UNUSED(topLeft); Q_UNUSED(bottomRight);
        if (roles.isEmpty() || roles.contains(ColorRole) || roles.contains(RatingRole) || roles.contains(TagsRole)) {
            recalculateAndEmitStats();
        }
    };
    connect(m_diskModel, &ItemModelBase::dataChanged, this, onDataChanged);
    connect(m_libraryModel, &ItemModelBase::dataChanged, this, onDataChanged);

    connect(m_libraryModel, &LibraryAssetModel::recordRenamed, this, [this](const QString& oldPath, const QString& newPath, const QString& newName) {
        Q_UNUSED(oldPath);
        this->setPendingSelectName(newName, false);
        this->selectAndScrollToPath(newPath);
        this->onSelectionChanged();
    });
>>>>>>> REPLACE
```

2. 物理掐断 `FilterProxyModel` 中对老旧混杂模型的 `qobject_cast`：
```
<<<<<<< SEARCH
    // 2026-06-xx 性能优化：提前获取 ItemRecord，避免重复查询并为下方过滤提供数据支撑
    const auto* sourceModelPtr = qobject_cast<const ArcMetaVirtualDbModel*>(sourceModel());
=======
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
>>>>>>> REPLACE
```

3. 在 `loadDirectory` (磁盘物理路线 0) 与 `loadCategory`/`loadPaths` (内存逻辑路线 1) 中自动分流多态模型绑定：
```
<<<<<<< SEARCH
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    // =========================================================================
    // 【彻底解耦与隔离】：干掉越界的劫持与重定向逻辑，保持磁盘模式 100% 的纯粹性！
    // =========================================================================

    m_isLoading = true;
    int reqId = ++m_loadRequestId;
=======
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    // 🚨 0 与 1 彻底断连多态自动分流：物理切断
    if (m_model != m_diskModel) {
        m_model = m_diskModel;
        m_proxyModel->setSourceModel(m_model);
    }

    m_isLoading = true;
    int reqId = ++m_loadRequestId;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::loadCategory(int categoryId) {
    // 2026-07-xx 物理防护：防重入机制。如果已经在加载同一个分类，则直接拦截，防止重复 clear() 导致的闪烁
    if (m_isLoading && m_currentCategoryId == categoryId && m_currentCategoryType == "user_category") {
        return;
    }

    m_isLoading = true;
    int reqId = ++m_loadRequestId;
=======
void ContentPanel::loadCategory(int categoryId) {
    // 🚨 0 与 1 彻底断连多态自动分流：逻辑切断
    if (m_model != m_libraryModel) {
        m_model = m_libraryModel;
        m_proxyModel->setSourceModel(m_model);
    }

    if (m_isLoading && m_currentCategoryId == categoryId && m_currentCategoryType == "user_category") {
        return;
    }

    m_isLoading = true;
    int reqId = ++m_loadRequestId;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::loadPaths(const QStringList& paths, int reqId) {
    // 2026-07-xx 物理强化：如果路径列表为空，直接执行同步清理并返回
    // 理由：这防止了搜索启动时的清空动作（异步）与随后到达的结果加载（异步）发生竞态。
    if (paths.isEmpty()) {
=======
void ContentPanel::loadPaths(const QStringList& paths, int reqId) {
    // 🚨 0 与 1 彻底断连多态自动分流：逻辑切断
    if (m_model != m_libraryModel) {
        m_model = m_libraryModel;
        m_proxyModel->setSourceModel(m_model);
    }

    if (paths.isEmpty()) {
>>>>>>> REPLACE
```

### 4.7 彻底清除 `ContentPanel.cpp` / `ContentPanel.h` 中已经彻底退役的旧模型 `ArcMetaVirtualDbModel` 实现
彻底移除 `ArcMetaVirtualDbModel` 的定义与实现（让整个 `ContentPanel` 成为纯粹的、仅用于承载视图布局的外观架构）。

在 `src/ui/ContentPanel.cpp` 中（直接删除以下内容，或将其原有的 class 实现彻底移去，该部分已被新模型彻底平替继承）：
在执行代码修改时，直接删去原有的 `ArcMetaVirtualDbModel` 类声明和成员方法实现（即从 `// --- ArcMetaVirtualDbModel 实现 ---` 到 `QVariant ArcMetaVirtualDbModel::data(const QModelIndex& index, int role) const` 终点的全部块）。

### 4.8 升级并重构 `src/ui/MediaColorExtractor.cpp` 的高清流式搜寻与默认大图标拦截

1. 在 `MediaColorExtractor::extractEmbeddedAiPreview` 中使用高速搜寻游标流（同 3.1.2 节的实现规范一致，不作任何硬编码 5MB 检索限制）：
```
<<<<<<< SEARCH
QImage MediaColorExtractor::extractEmbeddedAiPreview(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[MediaColorExtractor][AI] 文件打开失败：" << filePath;
        return QImage();
    }

    QByteArray data = file.read(5 * 1024 * 1024);
    file.close();

    int start = data.indexOf("\xFF\xD8\xFF");
    if (start == -1) {
        qWarning() << "[MediaColorExtractor][AI] 未找到 JPEG 起始标记(FFD8FF)，该文件可能未内嵌兼容性预览：" << filePath;
        return QImage();
    }

    int end = data.indexOf("\xFF\xD9", start);
    if (end == -1) {
        qWarning() << "[MediaColorExtractor][AI] 找到起始标记但未找到 JPEG 结束标记(FFD9)，读取范围内数据不完整：" << filePath;
        return QImage();
    }

    QByteArray imgData = data.mid(start, (end - start) + 2);
    QImage img;
    if (!img.loadFromData(imgData)) {
        qWarning() << "[MediaColorExtractor][AI] 已提取出 JPEG 字节流但解码失败，数据长度：" << imgData.size() << "：" << filePath;
        return QImage();
    }

    qDebug() << "[MediaColorExtractor][AI] 内嵌预览提取成功：" << filePath;
    return img;
}
=======
QImage MediaColorExtractor::extractEmbeddedAiPreview(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[MediaColorExtractor][AI] 文件打开失败：" << filePath;
        return QImage();
    }

    const qint64 chunkSize = 1024 * 1024; // 1MB chunk size
    QByteArray buffer;
    qint64 startOffset = -1;

    while (!file.atEnd()) {
        qint64 currentChunkOffset = file.pos();
        QByteArray chunk = file.read(chunkSize);
        if (chunk.isEmpty()) break;

        if (!buffer.isEmpty()) {
            buffer = buffer.right(2) + chunk;
            currentChunkOffset -= 2;
        } else {
            buffer = chunk;
        }

        int idx = buffer.indexOf("\xFF\xD8\xFF");
        if (idx != -1) {
            startOffset = currentChunkOffset + idx;
            break;
        }
    }

    if (startOffset == -1) {
        qWarning() << "[MediaColorExtractor][AI] 未找到 JPEG 起始标记(FFD8FF)，该文件可能未内嵌兼容性预览：" << filePath;
        return QImage();
    }

    if (!file.seek(startOffset)) {
        qWarning() << "[MediaColorExtractor][AI] 重定向文件指针失败：" << filePath;
        return QImage();
    }

    QByteArray imgData;
    buffer.clear();
    bool foundEnd = false;

    while (!file.atEnd() && imgData.size() < 50 * 1024 * 1024) {
        qint64 currentChunkOffset = file.pos();
        QByteArray chunk = file.read(chunkSize);
        if (chunk.isEmpty()) break;

        if (!buffer.isEmpty()) {
            buffer = buffer.right(1) + chunk;
            currentChunkOffset -= 1;
        } else {
            buffer = chunk;
        }

        int idx = buffer.indexOf("\xFF\xD9");
        if (idx != -1) {
            qint64 endOffset = currentChunkOffset + idx + 2;
            qint64 totalLen = endOffset - startOffset;
            if (totalLen > 0) {
                if (file.seek(startOffset)) {
                    imgData = file.read(totalLen);
                    foundEnd = true;
                }
            }
            break;
        }
    }

    file.close();

    if (!foundEnd || imgData.isEmpty()) {
        qWarning() << "[MediaColorExtractor][AI] 找到起始标记但未找到 JPEG 结束标记(FFD9)或数据不完整：" << filePath;
        return QImage();
    }

    QImage img;
    if (!img.loadFromData(imgData)) {
        qWarning() << "[MediaColorExtractor][AI] 已提取出 JPEG 字节流但解码失败，数据长度：" << imgData.size() << "：" << filePath;
        return QImage();
    }

    qDebug() << "[MediaColorExtractor][AI] 内嵌预览提取成功：" << filePath;
    return img;
}
>>>>>>> REPLACE
```

2. 拦截设计格式默认软件关联大图标：
```
<<<<<<< SEARCH
    if (img.isNull()) {
        img = WindowsShellThumbnailProvider::getShellThumbnail(path, size);
        if (img.isNull()) img.load(path);
    }

    if (!img.isNull()) {
        img.save(cachePath, "PNG");
    }
    return img;
}
=======
    if (img.isNull()) {
        static const QStringList rawDesignExts = {"psd", "psb", "ai", "eps"};
        if (rawDesignExts.contains(ext)) {
            qWarning() << "[MediaColorExtractor][BLOCK] 已强制拦截该文件的系统默认图标兜底，标记为无真实内容预览：" << path;
            return QImage();
        }

        img = WindowsShellThumbnailProvider::getShellThumbnail(path, size);
        if (img.isNull()) img.load(path);
    }

    if (!img.isNull()) {
        img.save(cachePath, "PNG");
    }
    return img;
}
>>>>>>> REPLACE
```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/core/ItemRecord.h / .cpp` (注入 `isFromMemory` 判定及内存解包逻辑)
- [ ] 模块/文件：`src/core/DiskScanService.cpp` 与 `src/ui/DiskScanService.cpp` (物理强制 `isFromMemory = false`)
- [ ] 模块/文件：`src/core/CategoryLoadService.cpp` 与 `src/ui/CategoryLoadService.cpp` (逻辑强制 `isFromMemory = true`)
- [ ] 模块/文件：`src/ui/models/ItemModelBase.h` (多态基础函数声明)
- [ ] 模块/文件：`src/ui/models/DiskItemModel.h / .cpp` (纯物理磁盘模型全量实现，禁用 `.arc` 解析)
- [ ] 模块/文件：`src/ui/models/LibraryAssetModel.h / .cpp` (受控逻辑资产模型全量实现，启用 `.arc` 解析)
- [ ] 模块/文件：`src/ui/ContentPanel.h / .cpp` (移除旧模型定义，实现多态指针指向，并在装载时进行 `0` 与 `1` 的物理分流)
- [ ] 模块/文件：`src/ui/MediaColorExtractor.cpp` (流式搜寻及大图标拦截)

**明确禁止越界修改的范围：**
- [ ] 模块/文件：`WindowsShellThumbnailProvider.cpp` —— 不作任何物理改动，其底层的 IImageFactory 接口保持原样。

## 6. 实现准则与预警【核心】
1. **防止未定义标识符**：由于旧的 `ArcMetaVirtualDbModel` 被平替剔除，内容面板内部关于数据行的读取和修改必须全部对齐到 `ItemModelBase` 定义的纯虚函数契约下，防止出现成员变量未暴露导致的编译错误。
2. **零多重 I/O 冗余**：1MB 分块流搜寻中，缓冲区和重叠标记通过 `buffer.right(2)` 处理完美规避了大文件在拼接搜寻时的重复 I/O。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|--------------------------------------------|----------------|
| 双轨物理隔离 | 磁盘导航浏览模式下产生的设色星级，100% 绝对禁止写入 SQLite 本地数据库，必须写入 `.arcmeta`。 | ✅ 符合。本方案彻底断连了磁盘模型对 MetadataManager 的数据库调用。 |

## 8. 待确认事项（可选）
- **无**。
