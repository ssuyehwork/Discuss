#include "MetaDbRepository.h"
#include "../core/DatabaseManager.h"

namespace RapidNotes {

MetaDbRepository& MetaDbRepository::instance() {
    static MetaDbRepository inst;
    return inst;
}

void MetaDbRepository::initDatabase() {
    DatabaseManager::instance().init();
}

DriveMetaRecord MetaDbRepository::getDriveMeta(const std::wstring& drivePath) {
    return DriveMetaDao::getDriveMeta(drivePath);
}

void MetaDbRepository::saveDriveMeta(const DriveMetaRecord& record) {
    DriveMetaDao::saveDriveMeta(record);
}

} // namespace RapidNotes
