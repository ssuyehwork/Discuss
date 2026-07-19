#ifndef ARCMETA_SHELL_ICON_MANAGER_H
#define ARCMETA_SHELL_ICON_MANAGER_H

#include <QObject>
#include <QIcon>
#include <QMap>
#include <QSet>
#include <shared_mutex>
#include <mutex>

namespace ArcMeta {

class ShellIconManager : public QObject {
    Q_OBJECT
private:
    ShellIconManager(QObject* parent = nullptr);
    ~ShellIconManager();

    QMap<QString, QIcon> m_fileIconCache;
    QSet<QString> m_loadingKeys;
    mutable std::shared_mutex m_cacheLock;
    std::mutex m_loadingLock;

public:
    static ShellIconManager& instance();

    QIcon getFileIcon(const QString& filePath, int size = 18);
    void clearCache();

signals:
    void iconLoaded();
};

} // namespace ArcMeta

#endif // ARCMETA_SHELL_ICON_MANAGER_H
