#pragma once
#include <QObject>
#include <QMenu>
#include <QAction>
#include "LibraryDao.h"

namespace RapidNotes {

class LibraryService : public QObject {
    Q_OBJECT
public:
    static LibraryService& instance() {
        static LibraryService s_instance;
        return s_instance;
    }
};

} // namespace RapidNotes
