#ifndef ARCMETA_METADATA_REPOSITORY_H
#define ARCMETA_METADATA_REPOSITORY_H

#include "MetadataDefs.h"
#include "MetadataManager.h"
#include "sqlite3.h"

namespace ArcMeta {

class MetadataRepository {
public:
    static bool saveMeta(sqlite3* db, const std::wstring& path, const RuntimeMeta& meta);
    static bool getMeta(sqlite3* db, const std::wstring& path, RuntimeMeta& outMeta);
    static bool deleteMeta(sqlite3* db, const std::wstring& path);
};

} // namespace ArcMeta

#endif // ARCMETA_METADATA_REPOSITORY_H
