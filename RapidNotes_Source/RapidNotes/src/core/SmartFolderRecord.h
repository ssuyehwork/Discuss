#pragma once
#include <QString>

namespace RapidNotes {

struct SmartFolderRecord {
    int id = 0;
    QString name;
    QString rules;
};

} // namespace RapidNotes
