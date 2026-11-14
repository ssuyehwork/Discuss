#include "QuickMediator.h"
#include "../QuickWindow.h"
#include "../../core/CoreEngine.h"
#include "../../core/CentralEventHub.h"

namespace RapidNotes {

QuickMediator::QuickMediator(QuickWindow* window, QObject* parent)
    : QObject(parent), m_window(window) {
    auto& hub = CentralEventHub::instance();
    connect(&hub, &CentralEventHub::eventOccurred, this, [this](const AppEvent& event) {
        if (event.type == AppEventType::MetadataUpdated) {
            onMetadataUpdated(event.paths);
        } else if (event.type == AppEventType::ItemsDeleted) {
            onItemsDeleted(event.paths);
        }
    });
}

void QuickMediator::requestMoveToCategory(const QList<int>& ids, int categoryId) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::MoveToCategory;
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    cmd.params["categoryId"] = categoryId;
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestToggleFavorite(const QList<int>& ids) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::ToggleFavorite;
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestMoveToTrash(const QList<int>& ids) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::MoveToTrash;
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestPermanentDelete(const QList<int>& ids) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::DeletePermanently;
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestSetRating(const QList<int>& ids, int rating) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::SetRating;
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    cmd.params["rating"] = rating;
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestSetColor(const QList<int>& ids, const QString& color) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::SetColor;
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    cmd.params["color"] = color;
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestSetTags(const QList<int>& ids, const QStringList& tags) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::SetTags;
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    cmd.params["tags"] = tags;
    CoreEngine::instance().executeCommand(cmd);
}

static AppCommand noteCmd(AppCommandType type, const QList<int>& ids) {
    AppCommand cmd;
    cmd.type = type;
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    return cmd;
}
static QVariantList toVariantList(const QList<int>& ids) {
    QVariantList l; for (int i : ids) l << i; return l;
}

void QuickMediator::requestAddTags(const QList<int>& ids, const QStringList& tags) {
    if (ids.isEmpty() || tags.isEmpty()) return;
    AppCommand cmd = noteCmd(AppCommandType::AddTag, ids);
    cmd.params["tags"] = tags;
    CoreEngine::instance().executeCommand(cmd);
}
void QuickMediator::requestSetFavorite(const QList<int>& ids, bool favorite) {
    if (ids.isEmpty()) return;
    AppCommand cmd = noteCmd(AppCommandType::SetFavorite, ids);
    cmd.params["favorite"] = favorite;
    CoreEngine::instance().executeCommand(cmd);
}
void QuickMediator::requestSetPinned(const QList<int>& ids, bool pinned) {
    if (ids.isEmpty()) return;
    AppCommand cmd = noteCmd(AppCommandType::SetPinned, ids);
    cmd.params["pinned"] = pinned;
    CoreEngine::instance().executeCommand(cmd);
}
void QuickMediator::requestRestoreFromTrash(const QList<int>& ids) {
    if (ids.isEmpty()) return;
    CoreEngine::instance().executeCommand(noteCmd(AppCommandType::RestoreFromTrash, ids));
}
void QuickMediator::requestRenameNote(int id, const QString& newTitle) {
    AppCommand cmd = noteCmd(AppCommandType::RenameItems, {id});
    cmd.params["newName"] = newTitle;
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestAddCategory(const QString& name, int parentId) {
    AppCommand cmd; cmd.type = AppCommandType::AddCategory;
    cmd.params["name"] = name; cmd.params["parentId"] = parentId;
    CoreEngine::instance().executeCommand(cmd);
}
void QuickMediator::requestToggleCategoryPinned(int categoryId) {
    AppCommand cmd; cmd.type = AppCommandType::ToggleCategoryPinned;
    cmd.params["categoryId"] = categoryId;
    CoreEngine::instance().executeCommand(cmd);
}
void QuickMediator::requestSetCategoryColor(int categoryId, const QString& color) {
    AppCommand cmd; cmd.type = AppCommandType::SetCategoryColor;
    cmd.params["categoryId"] = categoryId; cmd.params["color"] = color;
    CoreEngine::instance().executeCommand(cmd);
}
void QuickMediator::requestSetCategoryPresetTags(int categoryId, const QString& tags) {
    AppCommand cmd; cmd.type = AppCommandType::SetCategoryPresetTags;
    cmd.params["categoryId"] = categoryId; cmd.params["tags"] = tags;
    CoreEngine::instance().executeCommand(cmd);
}
void QuickMediator::requestSetCategorySortMode(int categoryId, const QString& mode) {
    AppCommand cmd; cmd.type = AppCommandType::SetCategorySortMode;
    cmd.params["categoryId"] = categoryId; cmd.params["mode"] = mode;
    CoreEngine::instance().executeCommand(cmd);
}
void QuickMediator::requestSoftDeleteCategories(const QList<int>& ids) {
    if (ids.isEmpty()) return;
    AppCommand cmd; cmd.type = AppCommandType::SoftDeleteCategories;
    cmd.params["categoryIds"] = toVariantList(ids);
    CoreEngine::instance().executeCommand(cmd);
}
void QuickMediator::requestHardDeleteCategories(const QList<int>& ids) {
    if (ids.isEmpty()) return;
    AppCommand cmd; cmd.type = AppCommandType::HardDeleteCategories;
    cmd.params["categoryIds"] = toVariantList(ids);
    CoreEngine::instance().executeCommand(cmd);
}
void QuickMediator::requestRestoreCategories(const QList<int>& ids) {
    if (ids.isEmpty()) return;
    AppCommand cmd; cmd.type = AppCommandType::RestoreCategories;
    cmd.params["categoryIds"] = toVariantList(ids);
    CoreEngine::instance().executeCommand(cmd);
}
bool QuickMediator::requestMoveCategory(int categoryId, int direction) {
    AppCommand cmd; cmd.type = AppCommandType::MoveCategory;
    cmd.params["categoryId"] = categoryId; cmd.params["direction"] = direction;
    return CoreEngine::instance().executeCommand(cmd);
}
bool QuickMediator::requestReorderCategories(int parentId, bool ascending) {
    AppCommand cmd; cmd.type = AppCommandType::ReorderCategories;
    cmd.params["parentId"] = parentId; cmd.params["ascending"] = ascending;
    return CoreEngine::instance().executeCommand(cmd);
}
bool QuickMediator::requestReorderAllCategories(bool ascending) {
    AppCommand cmd; cmd.type = AppCommandType::ReorderAllCategories;
    cmd.params["ascending"] = ascending;
    return CoreEngine::instance().executeCommand(cmd);
}
bool QuickMediator::requestEmptyTrash() {
    AppCommand cmd; cmd.type = AppCommandType::EmptyTrash;
    return CoreEngine::instance().executeCommand(cmd);
}
bool QuickMediator::requestRestoreAllFromTrash() {
    AppCommand cmd; cmd.type = AppCommandType::RestoreAllFromTrash;
    return CoreEngine::instance().executeCommand(cmd);
}
bool QuickMediator::requestUndo() { return CoreEngine::instance().undo(); }
bool QuickMediator::requestRedo() { return CoreEngine::instance().redo(); }

void QuickMediator::onMetadataUpdated(const QStringList& paths) {
    Q_UNUSED(paths);
    if (m_window) m_window->scheduleRefresh();
}

void QuickMediator::onItemsDeleted(const QStringList& paths) {
    Q_UNUSED(paths);
    if (m_window) m_window->scheduleRefresh();
}

} // namespace RapidNotes