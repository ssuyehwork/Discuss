#pragma once

#include <QObject>
#include "LibraryDao.h"

namespace QuarkMeta {

class LibraryService : public QObject {
    Q_OBJECT

public:
    static LibraryService& instance();

    int createCategory(const QString& name, int parentId = 0);
    bool removeCategory(int id);
    bool addPathsToCategory(int id, const QStringList& paths);
    bool removePathsFromCategory(int id, const QStringList& paths);
    QStringList getCategoryPaths(int id) const;

signals:
    void libraryChanged();

private:
    explicit LibraryService(QObject* parent = nullptr);
    ~LibraryService() override = default;
};

} // namespace QuarkMeta
