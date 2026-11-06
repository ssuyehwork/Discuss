#pragma once

#include <QObject>
#include <QMenu>
#include <QStringList>

namespace RapidNotes {

class FavoriteService : public QObject {
    Q_OBJECT
public:
    static FavoriteService& instance() {
        static FavoriteService s_instance;
        return s_instance;
    }

    void buildFavoriteAction(QMenu* menu, const QString& path, QObject* receiver = nullptr) {
        Q_UNUSED(menu);
        Q_UNUSED(path);
        Q_UNUSED(receiver);
    }

signals:
    void favoritesChanged();

private:
    explicit FavoriteService(QObject* parent = nullptr) : QObject(parent) {}
    ~FavoriteService() override = default;
};

} // namespace RapidNotes
