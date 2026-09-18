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

    // 标签剪贴板方法
    void setCopiedTags(const QStringList& tags);
    QStringList copiedTags() const;
    bool hasCopiedTags() const;
    void clearCopiedTags();

signals:
    void pasteCompleted(const QString& targetDir);

private:
    QStringList m_copiedTags;
    explicit ClipboardService(QObject* parent = nullptr);
    ~ClipboardService() override = default;
    ClipboardService(const ClipboardService&) = delete;
    ClipboardService& operator=(const ClipboardService&) = delete;
};

} // namespace QuarkMeta
