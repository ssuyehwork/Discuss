#pragma once

#include <string>
#include "DriveMetaDao.h"

namespace RapidNotes {

class MetaDbRepository {
public:
    static MetaDbRepository& instance();

    void initDatabase();
    DriveMetaRecord getDriveMeta(const std::wstring& drivePath);
    void saveDriveMeta(const DriveMetaRecord& record);

private:
    MetaDbRepository() = default;
    ~MetaDbRepository() = default;
};

} // namespace RapidNotes
