#pragma once

#include <QObject>
#include <QStringList>
#include <QSet>
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
    bool unmarkFolder(const QString& path);
    bool unmarkAllFolders();
    bool removeFolderRecord(const QString& path);
    bool clearAllFolderRecords();
    bool toggleMarked(const QString& path);

    int markedCount() const;
    QStringList getAllMarked() const;
    QStringList getActiveMarkedFolders() const;

    QAction* buildMarkAction(QMenu* parentMenu, const QString& path, QObject* receiver);

signals:
    void markedFoldersChanged();

private:
    void refreshCache();

    mutable QSet<QString> m_activeMarkedCache;
    Q_DISABLE_COPY_MOVE(MarkedFolderService)
};

} // namespace QuarkMeta
