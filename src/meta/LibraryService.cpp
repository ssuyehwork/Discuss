#include "LibraryService.h"

namespace QuarkMeta {

LibraryService& LibraryService::instance() {
    static LibraryService s_instance;
    return s_instance;
}

LibraryService::LibraryService(QObject* parent) : QObject(parent) {
    LibraryDao::initTable();
}

int LibraryService::createCategory(const QString& name, int parentId) {
    int id = LibraryDao::addCategory(name, parentId);
    if (id > 0) {
        emit libraryChanged();
    }
    return id;
}

bool LibraryService::removeCategory(int id) {
    bool ok = LibraryDao::removeCategoryById(id);
    if (ok) {
        emit libraryChanged();
    }
    return ok;
}

bool LibraryService::addPathsToCategory(int id, const QStringList& paths) {
    bool ok = LibraryDao::addPathsToCategory(id, paths);
    if (ok) {
        emit libraryChanged();
    }
    return ok;
}

bool LibraryService::removePathsFromCategory(int id, const QStringList& paths) {
    bool ok = LibraryDao::removePathsFromCategory(id, paths);
    if (ok) {
        emit libraryChanged();
    }
    return ok;
}

QStringList LibraryService::getCategoryPaths(int id) const {
    return LibraryDao::getCategoryPaths(id);
}

} // namespace QuarkMeta
