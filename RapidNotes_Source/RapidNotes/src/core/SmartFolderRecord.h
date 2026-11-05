#pragma once
#include <QString>

namespace QuarkMeta {
struct SmartFolderRecord {
    int id = 0;
    QString name;
    QString rules;
};
}

namespace RapidNotes {
using SmartFolderRecord = QuarkMeta::SmartFolderRecord;
}
