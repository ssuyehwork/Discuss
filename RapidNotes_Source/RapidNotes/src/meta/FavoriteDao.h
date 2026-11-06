#pragma once

#include <QString>
#include <QStringList>

namespace RapidNotes {

class FavoriteDao {
public:
    static bool updateFavorite(const QString& url, const QString& iconKey, const QString& extra) {
        Q_UNUSED(url);
        Q_UNUSED(iconKey);
        Q_UNUSED(extra);
        return true;
    }
};

} // namespace RapidNotes
