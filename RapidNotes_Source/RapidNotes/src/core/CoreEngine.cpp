#include "CoreEngine.h"
#include "CentralEventHub.h"
#include "NoteRepository.h"
#include "../core/TagCodec.h"

namespace RapidNotes {

static QList<qint64> extractNoteIds(const QStringList& paths) {
    QList<qint64> ids;
    for (const QString& p : paths) {
        if (p.startsWith("note://")) {
            ids.append(p.mid(7).toLongLong());
        } else {
            bool ok = false;
            qint64 val = p.toLongLong(&ok);
            if (ok) ids.append(val);
        }
    }
    return ids;
}

CoreEngine& CoreEngine::instance() {
    static CoreEngine instance;
    return instance;
}

CoreEngine::CoreEngine(QObject* parent) : QObject(parent) {
    m_asyncTagPool.setMaxThreadCount(1);
    qRegisterMetaType<RapidNotes::AppCommand>("RapidNotes::AppCommand");
}

std::shared_ptr<CancellationToken> CoreEngine::createCancellationToken() {
    return std::make_shared<CancellationToken>();
}

bool CoreEngine::executeCommand(const AppCommand& cmd) {
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
    case AppCommandType::SetPinned: {
        bool pinned = cmd.params.value("pinned", false).toBool();
        QList<qint64> ids = extractNoteIds(cmd.targetPaths);
        for (qint64 id : ids) {
            RapidNotes::NoteRepository::instance().updatePinned(id, pinned);
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
    default:
        return false;
    }
    return true;
}

void CoreEngine::handleSetRating(const QStringList& paths, int rating) {
    QList<qint64> ids = extractNoteIds(paths);
    RapidNotes::NoteRepository::instance().batchUpdateRating(ids, rating);

    AppEvent ev;
    ev.type = AppEventType::MetadataUpdated;
    ev.paths = paths;
    ev.payload["field"] = "rating";
    ev.payload["value"] = rating;
    CentralEventHub::instance().publishEvent(ev);
}

void CoreEngine::handleSetColor(const QStringList& paths, const QString& color) {
    QList<qint64> ids = extractNoteIds(paths);
    RapidNotes::NoteRepository::instance().batchUpdateColorLabel(ids, color);

    AppEvent ev;
    ev.type = AppEventType::MetadataUpdated;
    ev.paths = paths;
    ev.payload["field"] = "color";
    ev.payload["value"] = color;
    CentralEventHub::instance().publishEvent(ev);
}

void CoreEngine::handleSetTags(const QStringList& paths, const QStringList& tags) {
    QList<qint64> ids = extractNoteIds(paths);
    QString joinedTags = RapidNotes::TagCodec::join(tags);
    RapidNotes::NoteRepository::instance().batchUpdateTags(ids, joinedTags);

    AppEvent ev;
    ev.type = AppEventType::MetadataUpdated;
    ev.paths = paths;
    ev.payload["field"] = "tags";
    CentralEventHub::instance().publishEvent(ev);
}

void CoreEngine::handleSetNote(const QStringList& paths, const QString& note) {
    QList<qint64> ids = extractNoteIds(paths);
    for (qint64 id : ids) {
        RapidNotes::NoteRepository::instance().updateRemark(id, note);
    }

    AppEvent ev;
    ev.type = AppEventType::MetadataUpdated;
    ev.paths = paths;
    ev.payload["field"] = "remark";
    CentralEventHub::instance().publishEvent(ev);
}

void CoreEngine::handleSetURL(const QStringList&, const QString&) {}
void CoreEngine::handleRecordAccess(const QStringList&) {}
void CoreEngine::handleToggleFavorite(const QStringList&) {}
void CoreEngine::addTagsToPathsAsync(const QStringList& paths, const QStringList& tags, std::function<void(int)> onFinished, int) {
    handleSetTags(paths, tags);
    if (onFinished) onFinished(paths.size());
}

} // namespace RapidNotes
