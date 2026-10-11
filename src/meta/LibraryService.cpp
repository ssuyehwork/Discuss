#include "LibraryService.h"
#include "../ui/UiHelper.h"
#include "../ui/ToolTipOverlay.h"
#include <QCursor>

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

bool LibraryService::indexItem(int categoryId, const QString& filePath, const ItemMeta& meta) {
    bool ok = LibraryDao::indexItemMetadata(categoryId, filePath, meta);
    if (ok) emit libraryChanged();
    return ok;
}

bool LibraryService::unindexItem(int categoryId, const QString& filePath) {
    bool ok = LibraryDao::removeIndexedItem(categoryId, filePath);
    if (ok) emit libraryChanged();
    return ok;
}

QList<ItemMeta> LibraryService::getIndexedItems(int categoryId) const {
    return LibraryDao::getCategoryIndexedItems(categoryId);
}

QAction* LibraryService::buildLibraryAction(QMenu* parentMenu, const QStringList& targetPaths, QObject* receiver) {
    if (!parentMenu || targetPaths.isEmpty()) return nullptr;

    QIcon libIcon = UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 18);
    QAction* action = parentMenu->addAction(libIcon, "添加至“库”");

    connect(action, &QAction::triggered, receiver ? receiver : parentMenu, [this, targetPaths]() {
        if (addPathsToCategory(-2, targetPaths)) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已将 %1 个项目归入未分类").arg(targetPaths.size()), 1500, QColor("#2ecc71"));
        } else {
            ToolTipOverlay::instance()->showText(QCursor::pos(), "归入失败", 1500, QColor("#e81123"));
        }
    });

    return action;
}

} // namespace QuarkMeta
