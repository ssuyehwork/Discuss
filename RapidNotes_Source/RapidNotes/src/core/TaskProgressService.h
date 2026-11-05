#pragma once
#include <QObject>
#include <QString>

namespace QuarkMeta {

class TaskProgressService : public QObject {
    Q_OBJECT
public:
    static TaskProgressService& instance() {
        static TaskProgressService s_instance;
        return s_instance;
    }

signals:
    void progressUpdated(int percentage, const QString& statusText);
    void taskFinished();
};

} // namespace QuarkMeta
