#pragma once

#include <QObject>
#include <QStringList>
#include <QMenu>

namespace QuarkMeta {

class MarkedFolderService : public QObject {
    Q_OBJECT

public:
    static MarkedFolderService& instance();
    explicit MarkedFolderService(QObject* parent = nullptr);
    ~MarkedFolderService() override = default;

    bool isMarked(const QString& path) const;
    bool addMarked(const QString& path);
    bool removeMarked(const QString& path);
    bool toggleMarked(const QString& path);
    int markedCount() const;
    QStringList getAllMarked() const;
    QStringList getActiveMarked() const;
    bool removeFolderRecord(const QString& path);
    bool clearAllFolderRecords();
    bool clearAll();

    QAction* buildMarkAction(QMenu* parentMenu, const QString& path, QObject* receiver);

signals:
    void markedFoldersChanged();

private:
    Q_DISABLE_COPY_MOVE(MarkedFolderService)
};

} // namespace QuarkMeta
