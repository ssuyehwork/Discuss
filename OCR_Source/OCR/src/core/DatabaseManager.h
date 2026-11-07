#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    static DatabaseManager& instance() {
        static DatabaseManager s_instance;
        return s_instance;
    }

    void addNoteAsync(const QString& title, const QString& content, const QStringList& tags = QStringList(),
                      const QString& color = "", int categoryId = -1,
                      const QString& itemType = "text", const QByteArray& dataBlob = QByteArray(),
                      const QString& sourceApp = "", const QString& sourceTitle = "",
                      const QString& remark = "") {
        Q_UNUSED(title);
        Q_UNUSED(content);
        Q_UNUSED(tags);
        Q_UNUSED(color);
        Q_UNUSED(categoryId);
        Q_UNUSED(itemType);
        Q_UNUSED(dataBlob);
        Q_UNUSED(sourceApp);
        Q_UNUSED(sourceTitle);
        Q_UNUSED(remark);
    }

private:
    DatabaseManager(QObject* parent = nullptr) : QObject(parent) {}
    ~DatabaseManager() override = default;
};

#endif // DATABASEMANAGER_H
