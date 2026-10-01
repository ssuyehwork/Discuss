#pragma once

#include "QuarkMetaJson.h"
#include <QObject>
#include <QString>
#include <QTimer>
#include <unordered_map>
#include <mutex>
#include <string>
#include <functional>

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
     * @brief 物理迁移文件夹缓存文件 (.QuarkMeta.json)
     */
    bool migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath);

    /**
     * @brief 单文件重命名时同步修改条目键名
     */
    bool renameItem(const QString& folderPath, const QString& oldName, const QString& newName);

    /**
     * @brief 强制立即将所有未落盘的脏目录 JSON 刷入物理磁盘
     */
    void flushAllDirtyBuffers();

private slots:
    void onFlushTimeout();

private:
    explicit QuarkMetaJsonStore(QObject* parent = nullptr);
    ~QuarkMetaJsonStore() override;
    QuarkMetaJsonStore(const QuarkMetaJsonStore&) = delete;
    QuarkMetaJsonStore& operator=(const QuarkMetaJsonStore&) = delete;

    std::mutex m_storeMutex;
    // 目录路径 -> 内存中待合并的 JSON 对象
    std::unordered_map<std::wstring, QuarkMetaJson> m_dirtyBufferMap;
    QTimer* m_flushTimer = nullptr;
    static constexpr int kFlushDebounceMs = 50; // 50ms 内同目录修改自动合流为 1 次原子写盘
};

} // namespace QuarkMeta
