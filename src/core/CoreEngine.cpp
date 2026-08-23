#include "CoreEngine.h"
#include "../meta/MetadataManager.h"
#include "../meta/TagRepository.h"
#include "../meta/QuarkMetaJson.h"
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
        QString tag = cmd.params.value("tag").toString().trimmed();
        if (tag.isEmpty()) break;

        // 🚨 铁律第一步：先将新标签登记到 global.db 主词典（默认归入未分类/全局）
        TagRepository::addTagToGroup(tag, -1);

        // 🚨 铁律第二步：再绑定到各个选中项目的 .QuarkMeta.json 与内存
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
        QString tag = cmd.params.value("tag").toString().trimmed();
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
        QString oldTag = cmd.params.value("oldTag").toString().trimmed();
        QString newTag = cmd.params.value("newTag").toString().trimmed();
        if (oldTag.isEmpty() || newTag.isEmpty() || oldTag == newTag) break;

        // 1. global.db 主词典重命名
        TagRepository::removeTagFromGroup(oldTag, -1);
        TagRepository::addTagToGroup(newTag, -1);

        // 2. 内存与磁盘 .QuarkMeta.json 级联更新
        MetadataManager::instance().renameTag(oldTag, newTag);

        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::RemoveGlobalTag: {
        QString tag = cmd.params.value("tag").toString().trimmed();
        if (tag.isEmpty()) break;

        // 1. global.db 主词典彻底擦除
        TagRepository::removeTagFromGroup(tag, -1);

        // 2. 内存与磁盘 .QuarkMeta.json 级联擦除
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
    // 🚨 铁律第一步：确保这一批标签全部已在 global.db 主词典中登记
    for (const QString& t : tags) {
        QString cleanTag = t.trimmed();
        if (!cleanTag.isEmpty()) {
            TagRepository::addTagToGroup(cleanTag, -1);
        }
    }

    // 🚨 铁律第二步：物理落盘到各文件 .QuarkMeta.json
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
