#include "CoreEngine.h"
#include "../meta/MetadataManager.h"
#include "../util/ShellHelper.h"

namespace QuarkMeta {

CoreEngine& CoreEngine::instance() {
    static CoreEngine s_instance;
    return s_instance;
}

CoreEngine::CoreEngine(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<QuarkMeta::AppCommand>("QuarkMeta::AppCommand");
}

std::shared_ptr<CancellationToken> CoreEngine::createCancellationToken() {
    return std::make_shared<CancellationToken>();
}

bool CoreEngine::executeCommand(const AppCommand& cmd) {
    if (cmd.targetPaths.isEmpty() &&
        cmd.type != AppCommandType::RecordAccess &&
        cmd.type != AppCommandType::RenameTag &&
        cmd.type != AppCommandType::RemoveGlobalTag) {
        return false;
    }

    switch (cmd.type) {
    case AppCommandType::SetRating: {
        int rating = cmd.params.value("rating", 0).toInt();
        handleSetRating(cmd.targetPaths, rating);
        break;
    }
    case AppCommandType::SetColor: {
        QString color = cmd.params.value("color").toString();
        handleSetColor(cmd.targetPaths, color);
        break;
    }
    case AppCommandType::SetTags: {
        QStringList tags = cmd.params.value("tags").toStringList();
        handleSetTags(cmd.targetPaths, tags);
        break;
    }
    case AppCommandType::AddTag: {
        QString tag = cmd.params.value("tag").toString();
        for (const QString& path : cmd.targetPaths) {
            auto meta = MetadataManager::instance().getMeta(path.toStdWString());
            QStringList curTags = meta.tags;
            if (!curTags.contains(tag)) {
                curTags.append(tag);
                MetadataManager::instance().setTags(path.toStdWString(), curTags, false);
            }
        }
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.paths = cmd.targetPaths;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::RemoveTag: {
        QString tag = cmd.params.value("tag").toString();
        for (const QString& path : cmd.targetPaths) {
            auto meta = MetadataManager::instance().getMeta(path.toStdWString());
            QStringList curTags = meta.tags;
            curTags.removeAll(tag);
            MetadataManager::instance().setTags(path.toStdWString(), curTags, false);
        }
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.paths = cmd.targetPaths;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::RenameTag: {
        QString oldTag = cmd.params.value("oldTag").toString();
        QString newTag = cmd.params.value("newTag").toString();
        MetadataManager::instance().renameTag(oldTag, newTag);
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::RemoveGlobalTag: {
        QString tag = cmd.params.value("tag").toString();
        MetadataManager::instance().removeTag(tag);
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::SetPinned: {
        bool pinned = cmd.params.value("pinned", false).toBool();
        for (const QString& path : cmd.targetPaths) {
            MetadataManager::instance().setPinned(path.toStdWString(), pinned, true);
        }
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.paths = cmd.targetPaths;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::SetNote: {
        QString note = cmd.params.value("note").toString();
        handleSetNote(cmd.targetPaths, note);
        break;
    }
    case AppCommandType::SetURL: {
        QString url = cmd.params.value("url").toString();
        handleSetURL(cmd.targetPaths, url);
        break;
    }
    case AppCommandType::DeletePermanently: {
        for (const QString& path : cmd.targetPaths) {
            MetadataManager::instance().deletePermanently(path.toStdWString());
        }
        AppEvent ev;
        ev.type = AppEventType::ItemsDeleted;
        ev.paths = cmd.targetPaths;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::RemoveBatchSync: {
        MetadataManager::instance().removeMetadataBatchSync(cmd.targetPaths);
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.paths = cmd.targetPaths;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::RecordAccess: {
        handleRecordAccess(cmd.targetPaths);
        break;
    }
    default:
        return false;
    }

    return true;
}

void CoreEngine::handleSetRating(const QStringList& paths, int rating) {
    for (const QString& path : paths) {
        MetadataManager::instance().setRating(path.toStdWString(), rating);

        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.targetPath = path;
        ev.payload["field"] = "rating";
        ev.payload["value"] = rating;
        CentralEventHub::instance().publishEvent(ev);
    }
}

void CoreEngine::handleSetColor(const QStringList& paths, const QString& color) {
    for (const QString& path : paths) {
        MetadataManager::instance().setColor(path.toStdWString(), color.toStdWString());

        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.targetPath = path;
        ev.payload["field"] = "color";
        ev.payload["value"] = color;
        CentralEventHub::instance().publishEvent(ev);
    }
}

void CoreEngine::handleSetTags(const QStringList& paths, const QStringList& tags) {
    for (const QString& path : paths) {
        MetadataManager::instance().setTags(path.toStdWString(), tags, false);

        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.targetPath = path;
        ev.payload["field"] = "tags";
        ev.payload["value"] = tags;
        CentralEventHub::instance().publishEvent(ev);
    }
}

void CoreEngine::handleSetNote(const QStringList& paths, const QString& note) {
    for (const QString& path : paths) {
        MetadataManager::instance().setNote(path.toStdWString(), note.toStdWString());

        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.targetPath = path;
        ev.payload["field"] = "note";
        ev.payload["value"] = note;
        CentralEventHub::instance().publishEvent(ev);
    }
}

void CoreEngine::handleSetURL(const QStringList& paths, const QString& url) {
    for (const QString& path : paths) {
        MetadataManager::instance().setURL(path.toStdWString(), url.toStdWString());

        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.targetPath = path;
        ev.payload["field"] = "url";
        ev.payload["value"] = url;
        CentralEventHub::instance().publishEvent(ev);
    }
}

void CoreEngine::handleRecordAccess(const QStringList& paths) {
    for (const QString& path : paths) {
        MetadataManager::instance().recordAccess(path.toStdWString());
    }
}

} // namespace QuarkMeta
