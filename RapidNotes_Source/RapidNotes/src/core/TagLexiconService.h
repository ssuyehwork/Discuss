#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

namespace QuarkMeta {

struct TagLexiconItem {
    int id = 0;
    QString name;
    QString color;
};

class TagLexiconService : public QObject {
    Q_OBJECT
public:
    static TagLexiconService& instance() {
        static TagLexiconService s_instance;
        return s_instance;
    }

    QList<TagLexiconItem> getAllTags() const { return {}; }
};

} // namespace QuarkMeta
