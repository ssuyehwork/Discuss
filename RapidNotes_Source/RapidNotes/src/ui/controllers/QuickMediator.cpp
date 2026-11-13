#include "QuickMediator.h"
#include "../QuickWindow.h"
#include "../../core/CoreEngine.h"
#include "../../core/CentralEventHub.h"
#include "../../core/DatabaseManager.h"
#include "../../core/TagCodec.h"
#include "../../core/ActionRecorder.h"
#include "../StringUtils.h"
#include "../ToolTipOverlay.h"
#include <QRegularExpression>
#include <QCursor>

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
    return DatabaseManager::instance().moveNote(id, dir, filterType, filterValue);
}

bool QuickMediator::requestMoveNotesToRow(const QList<int>& ids, int targetRow, const QString& filterType, const QVariant& filterValue) {
    return DatabaseManager::instance().moveNotesToRow(ids, targetRow, filterType, filterValue);
}

bool QuickMediator::requestReorderNotes(const QString& filterType, const QVariant& filterValue, bool ascending, const QVariantMap& criteria) {
    return DatabaseManager::instance().reorderNotes(filterType, filterValue, ascending, criteria);
}

// ---------------------------------------------------------------------
// 分类管理调度
// ---------------------------------------------------------------------

int QuickMediator::requestAddCategory(const QString& name, int parentId, const QString& color) {
    return DatabaseManager::instance().addCategory(name, parentId, color);
}

bool QuickMediator::requestRenameCategory(int catId, const QString& name) {
    return DatabaseManager::instance().renameCategory(catId, name);
}

bool QuickMediator::requestDeleteCategories(const QList<int>& catIds, bool hardDelete) {
    if (hardDelete) {
        return DatabaseManager::instance().hardDeleteCategories(catIds);
    }
    return DatabaseManager::instance().softDeleteCategories(catIds);
}

bool QuickMediator::requestToggleCategoryPinned(int catId) {
    return DatabaseManager::instance().toggleCategoryPinned(catId);
}

bool QuickMediator::requestSetCategoryColor(int catId, const QString& color) {
    return DatabaseManager::instance().setCategoryColor(catId, color);
}

bool QuickMediator::requestSetCategoryPresetTags(int catId, const QString& tags) {
    return DatabaseManager::instance().setCategoryPresetTags(catId, tags);
}

bool QuickMediator::requestMoveCategory(int id, DatabaseManager::MoveDirection direction) {
    return DatabaseManager::instance().moveCategory(id, direction);
}

bool QuickMediator::requestReorderCategories(int parentId, bool ascending) {
    return DatabaseManager::instance().reorderCategories(parentId, ascending);
}

bool QuickMediator::requestReorderAllCategories(bool ascending) {
    return DatabaseManager::instance().reorderAllCategories(ascending);
}

bool QuickMediator::requestSetCategorySortMode(int catId, const QString& mode) {
    return DatabaseManager::instance().setCategorySortMode(catId, mode);
}

} // namespace RapidNotes