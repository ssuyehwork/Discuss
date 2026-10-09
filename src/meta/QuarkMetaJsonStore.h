#pragma once

#include "QuarkMetaJson.h"
#include <QObject>
#include <QString>
#include <QTimer>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <string>
#include <functional>
#include <memory>

namespace QuarkMeta {

class QuarkMetaJsonStore : public QObject {
    Q_OBJECT

public:
    static QuarkMetaJsonStore& instance();

    /**
     * @brief 原子修改指定路径对应文件的 ItemMeta 记录 (自动缓冲合并落盘)
     */
    void updateItemMeta(const std::wstring& filePath, std::function<void(ItemMeta&)> updater);

    /**
     * @brief 一致性读取指定物理文件夹的所有条目 ItemMeta (大小写不敏感匹配)
     */
    QuarkMetaJson::ItemMap readFolderMeta(const std::wstring& folderPath);

    /**
     * @brief 一致性读取指定文件的 ItemMeta，成功找到返回 true
     */
    bool readItemMeta(const std::wstring& filePath, ItemMeta& outMeta);

    /**
     * @brief 物理迁移文件夹缓存文件 (.QuarkMeta.json)
     */
    bool migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath);

    /**
     * @brief 单文件重命名时同步修改条目键名
     */
    bool renameItem(const QString& folderPath, const QString& oldName, const QString& newName);

    /**
     * @brief 强制立即将所有未落盘的脏目录 JSON 刷入物理磁盘 (sync 为 true 时在当前线程同步写入)
     */
    void flushAllDirtyBuffers(bool sync = false);

private slots:
    void onFlushTimeout();

private:
    explicit QuarkMetaJsonStore(QObject* parent = nullptr);
    ~QuarkMetaJsonStore() override;
    QuarkMetaJsonStore(const QuarkMetaJsonStore&) = delete;
    QuarkMetaJsonStore& operator=(const QuarkMetaJsonStore&) = delete;

    static std::wstring normalizeFolderPath(const std::wstring& path);

    std::mutex m_storeMutex;
    // 规范化目录路径 -> 内存中缓存的 JSON 对象指针
    std::unordered_map<std::wstring, std::shared_ptr<QuarkMetaJson>> m_folderCacheMap;
    // 标记需要写盘落盘的规范化目录路径集合
    std::unordered_set<std::wstring> m_dirtyFolderPaths;

    QTimer* m_flushTimer = nullptr;
    static constexpr int kFlushDebounceMs = 50; // 50ms 防抖
};

} // namespace QuarkMeta
