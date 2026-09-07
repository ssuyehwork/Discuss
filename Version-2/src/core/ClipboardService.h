#pragma once

#include <QObject>
#include <QStringList>
#include <QWidget>

namespace QuarkMeta {

class ClipboardService : public QObject {
    Q_OBJECT

public:
    static ClipboardService& instance();

    void copyItems(const QStringList& paths);
    void cutItems(const QStringList& paths);
    bool canPaste(const QString& targetDir) const;
    void executePaste(const QString& targetDir, QWidget* parentWidget = nullptr);

signals:
    void pasteCompleted(const QString& targetDir);

private:
    explicit ClipboardService(QObject* parent = nullptr);
    ~ClipboardService() override = default;
    ClipboardService(const ClipboardService&) = delete;
    ClipboardService& operator=(const ClipboardService&) = delete;
};

} // namespace QuarkMeta
