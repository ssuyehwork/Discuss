#ifndef ARCMETA_DB_FILE_SYSTEM_HELPER_H
#define ARCMETA_DB_FILE_SYSTEM_HELPER_H

#include <QString>
#include <string>

namespace ArcMeta {

class DbFileSystemHelper {
public:
    static void ensureFileHidden(const std::wstring& path);
    static QString handleDriveDriftRename(const std::wstring& volumeSerial, const QString& driveLetter, const QString& currentPath, const QString& appDir);
    static void cleanupInvalidDatabases(const std::wstring& volumeSerial, const QString& appDir);
};

} // namespace ArcMeta

#endif // ARCMETA_DB_FILE_SYSTEM_HELPER_H
