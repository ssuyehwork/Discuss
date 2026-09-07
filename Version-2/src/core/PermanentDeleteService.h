#pragma once

#include <QObject>
#include <QStringList>
#include <QWidget>
#include <vector>
#include <utility>

namespace QuarkMeta {

class PermanentDeleteService : public QObject {
    Q_OBJECT

public:
    static PermanentDeleteService& instance();

    bool execute(const QStringList& paths, QWidget* parentWidget = nullptr, bool isSecureShred = true);
    bool executeTrashItems(const std::vector<std::pair<int, QString>>& trashItems, QWidget* parentWidget = nullptr);

signals:
    void permanentDeleteCompleted();

private:
    explicit PermanentDeleteService(QObject* parent = nullptr);
    ~PermanentDeleteService() override = default;
    PermanentDeleteService(const PermanentDeleteService&) = delete;
    PermanentDeleteService& operator=(const PermanentDeleteService&) = delete;
};

} // namespace QuarkMeta
