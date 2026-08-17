#ifndef ARCMETA_METADATA_DAO_H
#define ARCMETA_METADATA_DAO_H

#include <string>
#include <vector>
#include "MetadataDefs.h"

namespace ArcMeta {

/**
 * @brief 专职负责实体 metadata 表物理 CRUD SQL 业务拼接与读写的隔离数据访问器
 */
struct sqlite3;
struct sqlite3_stmt;

struct RuntimeMeta;

class MetadataDao {
public:
    static void bindMetaHelper(sqlite3_stmt* stmt, const std::wstring& path, const RuntimeMeta& meta);

    static bool insertOrReplace(sqlite3* db, const std::wstring& path, const RuntimeMeta& meta);
    static bool batchInsertOrReplace(sqlite3* db, const std::vector<std::pair<std::wstring, RuntimeMeta>>& items);
    static bool deleteByFolderIds(sqlite3* db, const std::vector<std::string>& folderIds);
    static bool updatePathByFolderId(sqlite3* db, const std::string& folderId, const std::wstring& newPath);
    static bool checkExistsByFolderId(sqlite3* db, const std::string& folderId);
};

} // namespace ArcMeta

#endif // ARCMETA_METADATA_DAO_H
