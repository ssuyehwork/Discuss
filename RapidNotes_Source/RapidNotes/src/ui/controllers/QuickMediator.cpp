#include "QuickMediator.h"
#include "../QuickWindow.h"
#include "../../core/CoreEngine.h"
#include "../../core/CentralEventHub.h"
#include "../../core/DatabaseManager.h"
#include "../../core/TagCodec.h"
#include "../../core/ActionRecorder.h"
#include "../StringUtils.h"
#include <QRegularExpression>
#include <QSet>

namespace RapidNotes {

QuickMediator::QuickMediator(QuickWindow* window, QObject* parent)
    : QObject(parent), m_window(window) {
    auto& hub = CentralEventHub::instance();
    connect(&hub, &CentralEventHub::eventOccurred, this, [this](const AppEvent& event) {
        if (event.type == AppEventType::MetadataUpdated ||
            event.type == AppEventType::ItemsDeleted ||
            event.type == AppEventType::FavoritesUpdated) {
            onCentralEventTriggered();
        }
    });
}

void QuickMediator::onCentralEventTriggered() {
    if (m_window) {
        m_window->scheduleRefresh();
    }
}

// ---------------------------------------------------------------------
// 笔记核心元数据写操作 (通过 CoreEngine 命令通道下发)
// ---------------------------------------------------------------------

void QuickMediator::requestMoveToCategory(const QList<int>& ids, int categoryId) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::MoveToCategory;
    cmd.targetPaths.reserve(ids.size());
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    cmd.params["categoryId"] = categoryId;
    CoreEngine::instance().executeCommand(cmd);

    ActionRecorder::instance().recordMoveToCategory(categoryId);
    if (categoryId > 0) {
        StringUtils::recordRecentCategory(categoryId);
    }
}

void QuickMediator::requestToggleFavorite(const QList<int>& ids) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::ToggleFavorite;
    cmd.targetPaths.reserve(ids.size());
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestSetFavorite(const QList<int>& ids, bool favorite) {
    if (ids.isEmpty()) return;
    DatabaseManager::instance().updateNoteStateBatch(
        ids, "is_favorite", favorite ? 1 : 0);
    AppEvent event;
    event.type = AppEventType::MetadataUpdated;
    CentralEventHub::instance().publishEvent(event);
}

void QuickMediator::requestTogglePinned(const QList<int>& ids, bool pinned) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::SetPinned;
    cmd.targetPaths.reserve(ids.size());
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    cmd.params["pinned"] = pinned;
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestMoveToTrash(const QList<int>& ids) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::MoveToTrash;
    cmd.targetPaths.reserve(ids.size());
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestPermanentDelete(const QList<int>& ids) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::DeletePermanently;
    cmd.targetPaths.reserve(ids.size());
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestSetRating(const QList<int>& ids, int rating) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::SetRating;
    cmd.targetPaths.reserve(ids.size());
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    cmd.params["rating"] = rating;
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestSetColor(const QList<int>& ids, const QString& color) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::SetColor;
    cmd.targetPaths.reserve(ids.size());
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    cmd.params["color"] = color;
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestSetTags(const QList<int>& ids, const QStringList& tags) {
    if (ids.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::SetTags;
    cmd.targetPaths.reserve(ids.size());
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    cmd.params["tags"] = TagCodec::normalize(tags);
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestAddTags(const QList<int>& ids, const QStringList& tags) {
    if (ids.isEmpty() || tags.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::AddTag;
    cmd.targetPaths.reserve(ids.size());
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    cmd.params["tags"] = TagCodec::normalize(tags);
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestRemoveTags(const QList<int>& ids, const QStringList& tags) {
    if (ids.isEmpty() || tags.isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::RemoveTag;
    cmd.targetPaths.reserve(ids.size());
    for (int id : ids) cmd.targetPaths.append(QString("note://%1").arg(id));
    cmd.params["tags"] = TagCodec::normalize(tags);
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestRenameNote(int id, const QString& newTitle) {
    if (id <= 0 || newTitle.trimmed().isEmpty()) return;
    AppCommand cmd;
    cmd.type = AppCommandType::RenameItems;
    cmd.targetPaths = { QString("note://%1").arg(id) };
    cmd.params["newName"] = newTitle.trimmed();
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestSetNoteRemark(int id, const QString& remark) {
    if (id <= 0) return;
    AppCommand cmd;
    cmd.type = AppCommandType::SetNote;
    cmd.targetPaths = { QString("note://%1").arg(id) };
    cmd.params["note"] = remark;
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestSetNoteURL(int id, const QString& url) {
    if (id <= 0) return;
    AppCommand cmd;
    cmd.type = AppCommandType::SetURL;
    cmd.targetPaths = { QString("note://%1").arg(id) };
    cmd.params["url"] = url;
    CoreEngine::instance().executeCommand(cmd);
}

void QuickMediator::requestRecordAccess(int id) {
    if (id <= 0) return;
    AppCommand cmd;
    cmd.type = AppCommandType::RecordAccess;
    cmd.targetPaths = { QString("note://%1").arg(id) };
    CoreEngine::instance().executeCommand(cmd);
}

// ---------------------------------------------------------------------
// 笔记基础创建与入库
// ---------------------------------------------------------------------

int QuickMediator::requestAddNote(const QString& title, const QString& content, const QStringList& tags,
                                  const QString& remark, int categoryId,
                                  const QString& itemType, const QByteArray& dataBlob) {
    int newId = DatabaseManager::instance().addNote(title, content, tags, remark, categoryId, itemType, dataBlob);
    if (newId > 0) {
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        CentralEventHub::instance().publishEvent(ev);
    }
    return newId;
}

// ---------------------------------------------------------------------
// 回收站专项操作
// ---------------------------------------------------------------------

void QuickMediator::requestRestoreFromTrash(const QList<int>& noteIds, const QList<int>& catIds) {
    if (!noteIds.isEmpty()) {
        DatabaseManager::instance().updateNoteStateBatch(noteIds, "is_deleted", 0);
    }
    if (!catIds.isEmpty()) {
        DatabaseManager::instance().restoreCategories(catIds);
    }
    AppEvent ev;
    ev.type = AppEventType::MetadataUpdated;
    CentralEventHub::instance().publishEvent(ev);
}

void QuickMediator::requestRestoreAllTrash() {
    if (DatabaseManager::instance().restoreAllFromTrash()) {
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        CentralEventHub::instance().publishEvent(ev);
    }
}

void QuickMediator::requestEmptyTrash() {
    if (DatabaseManager::instance().emptyTrash()) {
        AppEvent ev;
        ev.type = AppEventType::ItemsDeleted;
        CentralEventHub::instance().publishEvent(ev);
    }
}

// ---------------------------------------------------------------------
// 高级业务逻辑调度
// ---------------------------------------------------------------------

int QuickMediator::requestCreateByLine(const QString& text, int targetCategoryId) {
    if (text.trimmed().isEmpty()) return 0;

    QStringList lines = text.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
    if (lines.isEmpty()) return 0;

    auto& db = DatabaseManager::instance();
    db.beginBatch();
    int count = 0;
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        QString title, content;
        StringUtils::smartSplitLanguage(trimmed, title, content);
        db.addNote(title, content, {}, "", targetCategoryId);
        count++;
    }
    db.endBatch();

    AppEvent ev;
    ev.type = AppEventType::MetadataUpdated;
    CentralEventHub::instance().publishEvent(ev);
    return count;
}

bool QuickMediator::requestMergeNotes(const QList<int>& ids, int targetCategoryId) {
    if (ids.size() < 2) return false;

    auto& db = DatabaseManager::instance();
    QList<QVariantMap> notes = db.getNotesByIds(ids);
    if (notes.isEmpty()) return false;

    QString firstTitle = notes.first().value("title").toString();
    if (firstTitle.trimmed().isEmpty()) firstTitle = "合并后的灵感";
    QString finalTitle = firstTitle + " (合并)";

    QStringList mergedContents;
    QSet<QString> tagsSet;

    for (const auto& note : notes) {
        QString content = note.value("content").toString();
        mergedContents << StringUtils::htmlToPlainText(content).trimmed();

        QString tags = note.value("tags").toString();
        QStringList parts = TagCodec::split(tags);
        for (const QString& t : parts) tagsSet.insert(t.trimmed());
    }

    QString finalContent = mergedContents.join("\n\n");
    QStringList finalTags = TagCodec::normalize(tagsSet.values());

    int newId = db.addNote(finalTitle, finalContent, finalTags, "", targetCategoryId);
    if (newId > 0) {
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        CentralEventHub::instance().publishEvent(ev);
        return true;
    }
    return false;
}

bool QuickMediator::requestPasteTagsBatch(const QList<int>& ids, const QStringList& tags) {
    if (ids.isEmpty() || tags.isEmpty()) return false;
    requestSetTags(ids, tags);
    ActionRecorder::instance().recordPasteTags(tags);
    return true;
}

bool QuickMediator::requestRepeatLastAction(const QList<int>& targetIds) {
    if (targetIds.isEmpty()) return false;

    auto actionType = ActionRecorder::instance().getLastActionType();
    if (actionType == ActionRecorder::ActionType::None) {
        return false;
    }

    if (actionType == ActionRecorder::ActionType::PasteTags) {
        QStringList tagsList = ActionRecorder::instance().getLastActionData().toStringList();
        if (tagsList.isEmpty()) return false;
        requestSetTags(targetIds, tagsList);
        return true;
    } else if (actionType == ActionRecorder::ActionType::MoveToCategory) {
        int catId = ActionRecorder::instance().getLastActionData().toInt();
        requestMoveToCategory(targetIds, catId);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------
// 列表排序与位移调度
// ---------------------------------------------------------------------

bool QuickMediator::requestMoveNote(int id, DatabaseManager::MoveDirection dir, const QString& filterType, const QVariant& filterValue) {
    bool ok = DatabaseManager::instance().moveNote(id, dir, filterType, filterValue);
    if (ok) {
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        CentralEventHub::instance().publishEvent(ev);
    }
    return ok;
}

bool QuickMediator::requestMoveNotesToRow(const QList<int>& ids, int targetRow, const QString& filterType, const QVariant& filterValue) {
    bool ok = DatabaseManager::instance().moveNotesToRow(ids, targetRow, filterType, filterValue);
    if (ok) {
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        CentralEventHub::instance().publishEvent(ev);
    }
    return ok;
}

bool QuickMediator::requestReorderNotes(const QString& filterType, const QVariant& filterValue, bool ascending, const QVariantMap& criteria) {
    bool ok = DatabaseManager::instance().reorderNotes(filterType, filterValue, ascending, criteria);
    if (ok) {
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        CentralEventHub::instance().publishEvent(ev);
    }
    return ok;
}

// ---------------------------------------------------------------------
// 分类管理调度
// ---------------------------------------------------------------------

int QuickMediator::requestAddCategory(const QString& name, int parentId, const QString& color) {
    int catId = DatabaseManager::instance().addCategory(name, parentId, color);
    if (catId > 0 && m_window) {
        m_window->refreshSidebar();
    }
    return catId;
}

bool QuickMediator::requestRenameCategory(int catId, const QString& name) {
    bool ok = DatabaseManager::instance().renameCategory(catId, name);
    if (ok && m_window) {
        m_window->refreshSidebar();
    }
    return ok;
}

bool QuickMediator::requestDeleteCategories(const QList<int>& catIds, bool hardDelete) {
    bool ok = hardDelete ? DatabaseManager::instance().hardDeleteCategories(catIds)
                         : DatabaseManager::instance().softDeleteCategories(catIds);
    if (ok && m_window) {
        m_window->refreshSidebar();
        m_window->scheduleRefresh();
    }
    return ok;
}

bool QuickMediator::requestToggleCategoryPinned(int catId) {
    bool ok = DatabaseManager::instance().toggleCategoryPinned(catId);
    if (ok && m_window) {
        m_window->refreshSidebar();
    }
    return ok;
}

bool QuickMediator::requestSetCategoryColor(int catId, const QString& color) {
    bool ok = DatabaseManager::instance().setCategoryColor(catId, color);
    if (ok && m_window) {
        m_window->refreshSidebar();
    }
    return ok;
}

bool QuickMediator::requestSetCategoryPresetTags(int catId, const QString& tags) {
    bool ok = DatabaseManager::instance().setCategoryPresetTags(catId, tags);
    if (ok && m_window) {
        m_window->refreshSidebar();
    }
    return ok;
}

bool QuickMediator::requestMoveCategory(int id, DatabaseManager::MoveDirection direction) {
    bool ok = DatabaseManager::instance().moveCategory(id, direction);
    if (ok && m_window) {
        m_window->refreshSidebar();
    }
    return ok;
}

bool QuickMediator::requestReorderCategories(int parentId, bool ascending) {
    bool ok = DatabaseManager::instance().reorderCategories(parentId, ascending);
    if (ok && m_window) {
        m_window->refreshSidebar();
    }
    return ok;
}

bool QuickMediator::requestReorderAllCategories(bool ascending) {
    bool ok = DatabaseManager::instance().reorderAllCategories(ascending);
    if (ok && m_window) {
        m_window->refreshSidebar();
    }
    return ok;
}

bool QuickMediator::requestSetCategorySortMode(int catId, const QString& mode) {
    bool ok = DatabaseManager::instance().setCategorySortMode(catId, mode);
    if (ok && m_window) {
        m_window->scheduleRefresh();
    }
    return ok;
}

// ---------------------------------------------------------------------
// 分类密码保护与锁定状态调度
// ---------------------------------------------------------------------

bool QuickMediator::requestSetCategoryPassword(int catId, const QString& password, const QString& hint) {
    bool ok = DatabaseManager::instance().setCategoryPassword(catId, password, hint);
    if (ok && m_window) {
        m_window->refreshSidebar();
        m_window->scheduleRefresh();
    }
    return ok;
}

bool QuickMediator::requestVerifyCategoryPassword(int catId, const QString& password) {
    return DatabaseManager::instance().verifyCategoryPassword(catId, password);
}

bool QuickMediator::requestRemoveCategoryPassword(int catId) {
    bool ok = DatabaseManager::instance().removeCategoryPassword(catId);
    if (ok && m_window) {
        m_window->refreshSidebar();
        m_window->scheduleRefresh();
    }
    return ok;
}

void QuickMediator::requestLockCategory(int catId) {
    DatabaseManager::instance().lockCategory(catId);
    if (m_window) {
        m_window->refreshSidebar();
        m_window->scheduleRefresh();
    }
}

void QuickMediator::requestLockAllCategories() {
    DatabaseManager::instance().lockAllCategories();
    if (m_window) {
        m_window->refreshSidebar();
        m_window->scheduleRefresh();
    }
}

bool QuickMediator::requestToggleLockedCategoriesVisibility() {
    DatabaseManager::instance().toggleLockedCategoriesVisibility();
    bool isHidden = DatabaseManager::instance().isLockedCategoriesHidden();
    if (m_window) {
        m_window->refreshSidebar();
        m_window->scheduleRefresh();
    }
    return isHidden;
}

// ---------------------------------------------------------------------
// 自动归档配置调度
// ---------------------------------------------------------------------

void QuickMediator::requestSetAutoCategorize(bool enabled, int targetCatId) {
    auto& db = DatabaseManager::instance();
    db.setAutoCategorizeEnabled(enabled);
    if (targetCatId > 0) {
        db.setActiveCategoryId(targetCatId);
    }
}

} // namespace RapidNotes