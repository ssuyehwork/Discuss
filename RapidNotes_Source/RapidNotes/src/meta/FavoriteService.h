#pragma once
#include <QObject>
#include <QString>
#include <QMenu>
#include <QAction>
#include "FavoriteDao.h"

namespace QuarkMeta {

class FavoriteService : public QObject {
    Q_OBJECT
public:
    static FavoriteService& instance() {
        static FavoriteService s_instance;
        return s_instance;
    }
};

} // namespace QuarkMeta
