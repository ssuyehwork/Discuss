#ifndef ARCMETA_FILE_MIGRATION_HANDLER_H
#define ARCMETA_FILE_MIGRATION_HANDLER_H

#include <QStringList>

namespace ArcMeta {

class FileMigrationHandler {
public:
    static bool checkMigrationAllowed(const QStringList& sourcePaths, const QString& destinationLibraryPath);
    static void executeMigration(const QStringList& sourcePaths, const QString& destinationLibraryPath);
};

} // namespace ArcMeta

#endif // ARCMETA_FILE_MIGRATION_HANDLER_H
