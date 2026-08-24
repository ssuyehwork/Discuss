#ifndef QuarkMeta_METADATA_MANAGER_H
#define QuarkMeta_METADATA_MANAGER_H

#include "MetadataDefs.h"
#include <QObject>
#include <QString>
#include <QTimer>
#include <QStringList>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <string>
#include <atomic>
#include <deque>
#include <mutex>
#include <memory>
#include <array>

namespace QuarkMeta {

/**
 * @brief 内存元数据镜像结构
 */
struct RuntimeMeta {
    int rating;
    std::wstring manualColor;
    std::wstring autoColor;
    QStringList tags;
    std::wstring note;
    std::wstring url;
    bool pinned;
    bool encrypted;
    bool isFolder; // 区分文件夹与文件，用于侧边栏精准统计
    int width;      // 物理尺寸：宽 (像素)
    int height;     // 物理尺寸：高 (像素)
    int thumbStatus; // 0: 正常/未处理, 1: 提取失败/跳过
    std::wstring originalPath; // 路径记忆：用于回收站还原
    std::wstring baseName; // 持久化基名，避免重复解析计算
    std::wstring ext;      // 持久化后缀名，统一小写
    std::string sha256;    // 储存文件的 SHA256 / FastHash 哈希值
    
    // 补充时间戳与大小字段
    long long ctime;
    long long mtime;
    long long atime;
    long long fileSize;
    long long added_at;

    std::vector<PaletteEntry> palettes;

    RuntimeMeta() : rating(0), pinned(false), encrypted(false), isFolder(false), width(0), height(0), thumbStatus(0), ctime(0), mtime(0), atime(0), fileSize(0), added_at(0) {}

    /**
     * @brief 判定是否有用户操作过的信息
     */
    bool hasUserOperations() const {
        return rating > 0 || !manualColor.empty() || !autoColor.empty() || !tags.isEmpty() || !note.empty() || !url.empty() || pinned || encrypted;
    }
};

struct LightMeta {
    std::wstring path;
    bool isFolder;
    bool tagsEmpty;
    double atime;
    QStringList tags;
};

/**
 * @brief 元数据管理器
 */
class MetadataManager : public QObject {
    Q_OBJECT
public:
    static MetadataManager& instance();

    bool isLoaded() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_loaded;
    }

    static std::wstring normalizePath(const std::wstring& path);
    
    void initFromDatabase();
    RuntimeMeta getMeta(const std::wstring& path);

    /**
     * @brief 在当前引擎下执行多维搜索
     * @param keyword 关键词
     * @param scopeSource 范围来源 ("category" 或 "nav")
     * @param categoryId 分类 ID (当 scopeSource 为 "category" 时有效)
     * @param parentPath 物理路径 (当 scopeSource 为 "nav" 时有效)
     * @return 匹配的物理路径列表
     */
    QStringList searchInCache(const QString& keyword, const QString& scopeSource = "", int categoryId = 0, const QString& parentPath = "");

    /**
     * @brief 获取所有标签及其引用计数
     * @return 标签名 -> 引用次数
     */
    QMap<QString, int> getAllTags() const;

    /**
     * @brief 获取权重最高的前 N 个标签
     */
    QList<QPair<QString, int>> getTopTags(int limit = 20) const;

    /**
     * @brief 物理刷新级别
     */
    enum class RefreshLevel {
        CountsOnly,   // 仅刷新侧边栏计数
        PathUpdate,   // 刷新特定文件
        FullRebuild,  // 全量 UI 重建
        CategoryOnly  // 仅物理更新分类，避免数据全量重载
    };

    /**
     * @brief 语义化 UI 信号通知
     */
    void notifyUI(RefreshLevel level, const QString& path = "");

    void notifyCategoryCountChanged();

    /**
     * @brief 语义化通知接口
     */
    void notifyFullUIRebuild();

    /**
     * @brief 一站式项目注册流程
     * @param path 物理路径
     */
    void registerItem(const std::wstring& path);

    /**
     * @brief 异步批量注册项目
     */
    void registerItemsAsync(const QStringList& paths);


    void ensureActivated(const std::wstring& nPath);

    void setRating(const std::wstring& path, int rating, bool notify = true);
    void setSha256(const std::wstring& path, const std::string& sha256, bool notify = false);
    void setAddedAt(const std::wstring& path, long long addedAt, bool notify = true);
    void setColor(const std::wstring& path, const std::wstring& color, bool notify = true);
    void setPinned(const std::wstring& path, bool pinned, bool notify = true);
    void setTags(const std::wstring& path, const QStringList& tags, bool notify = true);
    void setNote(const std::wstring& path, const std::wstring& note, bool notify = true);
    void setURL(const std::wstring& path, const std::wstring& url, bool notify = true);
    void setEncrypted(const std::wstring& path, bool encrypted, bool notify = true);

    void setPalettes(const std::wstring& path, const QVector<QPair<QColor, float>>& palettes, bool notify = true);

    struct ExtractedFeatureItem {
        std::wstring path;
        int width{0};
        int height{0};
        int64_t mtime{0};
        int64_t fileSize{0};
        std::wstring autoColor;
        QVector<QPair<QColor, float>> palettes;
    };

    void updateExtractedMediaFeatures(
        const std::wstring& path, 
        int width, 
        int height, 
        const std::wstring& autoColor, 
        const QVector<QPair<QColor, float>>& palettes
    );

    void updateExtractedMediaFeaturesBatch(const std::vector<ExtractedFeatureItem>& items);

    /**
     * @brief 全局重命名标签
     */
    void renameTag(const QString& oldName, const QString& newName);

    /**
     * @brief 全局删除标签
     */
    void removeTag(const QString& tagName);
    
    /**
     * @brief 原子化设置视觉元数据（颜色与色板），仅触发一次信号
     */
    void setItemVisualMetadata(const std::wstring& path, const std::wstring& color, const QVector<QPair<QColor, float>>& palettes, bool notify = true);

    /**
     * @brief 设置图片/媒体的宽高
     */
    void setItemDimensions(const std::wstring& path, int width, int height);

    QVector<QColor> getPalettes(const std::wstring& path);

    void renameItem(const std::wstring& oldPath, const std::wstring& newPath);

    /**
     * @brief 异步批量重命名元数据
     */
    void renameBatchAsync(
        const std::vector<std::pair<std::wstring, std::wstring>>& rawPathPairs,
        std::function<void(int successCount)> onCompleted = nullptr
    );

    void removeMetadataSync(const std::wstring& path);

    /**
     * @brief 在物理移动/剪切完成后，统一调用该函数进行元数据和统计对账
     */
    void syncAfterMove(const std::wstring& oldPath, const std::wstring& newPath);


    /**
     * @brief 物理同步元数据
     */
    void syncPhysicalMetadata(const std::wstring& path, bool notify = true);

    /**
     * @brief 激活并初始化项的元数据
     */
    static void activateItem(const std::wstring& path);

    /**
     * @brief 获取路径所在磁盘的卷序列号
     */
    static std::wstring getVolumeSerialNumber(const std::wstring& path);

    /**
     * @brief 物理操作原子事务计数，精确闭锁生命周期
     */
    void beginInternalOperation() {
        m_internalOpsCount.fetch_add(1);
        m_isInternalOperating.store(true);
    }

    void endInternalOperation() {
        int count = m_internalOpsCount.fetch_sub(1) - 1;
        if (count <= 0) {
            m_internalOpsCount.store(0);
            m_isInternalOperating.store(false);
        }
    }

    /**
     * @brief 设置内部操作标志位，用于抑制冗余信号刷新
     */
    void setInternalOperating(bool operating) { 
        if (operating) {
            beginInternalOperation();
        } else {
            endInternalOperation();
        }
    }
    bool isInternalOperating() const { return m_isInternalOperating.load(); }

    /**
     * @brief 安全解析路径组件
     * @param normalizedPath 标准化后的路径
     * @param isFolder 是否为文件夹
     * @param outName 输出名称（文件含后缀，文件夹仅名称）
     * @param outExt 输出后缀（仅文件，统一小写）
     */
    void parsePathComponents(const std::wstring& normalizedPath, bool isFolder, std::wstring& outName, std::wstring& outExt);

    void recordAccess(const std::wstring& path);
    void slideRecentWindow();
    double getCachedAtime(const std::wstring& path);

    /**
     * @brief 只读遍历内存缓存，用于统计等场景
     */
    template<typename Func>
    void forEachCachedItem(Func&& fn) const {
        for (size_t i = 0; i < NUM_SHARDS; ++i) {
            std::shared_lock<std::shared_mutex> lock(m_shards[i].mutex);
            for (const auto& pair : m_shards[i].items) {
                fn(pair.first, pair.second);
            }
        }
    }

    /**
     * @brief 获取极轻量级内存缓存快照以避免长持读锁对账
     */
    std::vector<LightMeta> getLightweightCacheSnapshot() const;
    std::shared_mutex& getMutex() const { return m_mutex; }

    /**
     * @brief 内部辅助：通过 WinAPI 获取基础物理元数据
     */
    static bool fetchWinApiMetadataDirect(const std::wstring& path, long long* outSize = nullptr, std::wstring* outType = nullptr, long long* outCtime = nullptr, long long* outMtime = nullptr, long long* outAtime = nullptr);

    /**
     * @brief 异步持久化项元数据
     */
    void persistAsync(const std::wstring& path, bool notify = true);

signals:
    void metaChanged(const QString& path);

    /**
     * @brief 待同步状态变更信号
     * @param hasPending 是否存在待处理数据
     */
    void pendingSyncChanged(bool hasPending);

private slots:
    void triggerUiSignalTimer() {
        if (m_uiSignalTimer && !m_uiSignalTimer->isActive()) {
            m_uiSignalTimer->start();
        }
    }

private:
    MetadataManager(QObject* parent = nullptr);
    ~MetadataManager() override = default;

    // 256 分片并发哈希容器：替代全量深拷贝 COW 快照
    struct MetaShard {
        mutable std::shared_mutex mutex;
        std::unordered_map<std::wstring, RuntimeMeta> items;
    };
    static constexpr size_t NUM_SHARDS = 256;
    std::array<MetaShard, NUM_SHARDS> m_shards;

    inline size_t getShardIndex(const std::wstring& path) const {
        return std::hash<std::wstring>{}(normalizePath(path)) % NUM_SHARDS;
    }


    std::deque<std::wstring> m_recentVisitedQueue;
    std::unordered_set<std::wstring> m_recentVisitedSet;
    std::mutex m_recentMutex;

    mutable std::shared_mutex m_mutex;
    bool m_loaded = false;
    std::atomic<bool> m_isInternalOperating{false};
    std::atomic<int> m_internalOpsCount{0};
    
    QTimer* m_uiSignalTimer = nullptr;
    std::unordered_set<QString> m_pendingUiPaths;

    /**
     * @brief 异步批量持久化元数据
     */
    void persistBatchAsync(const std::vector<std::wstring>& paths);
};

} // namespace QuarkMeta

#endif // QuarkMeta_METADATA_MANAGER_H
