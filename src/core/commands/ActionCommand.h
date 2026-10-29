#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace QuarkMeta {

enum class CommandType {
    SetRating,
    SetColor,
    SetTags,
    SetNote,
    SetUrl,
    MoveItems,
    RenameItem,
    DeleteItems
};

struct ActionCommand {
    CommandType type;
    QStringList targetPaths;
    QVariantMap payload;
};

} // namespace QuarkMeta
