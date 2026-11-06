#pragma once

#include <QObject>
#include <QString>

namespace RapidNotes {

/**
 * @brief 纯笔记预览服务桩 (对齐 QuarkMeta 预览接口，无物理文件/外壳依赖)
 */
class QuickLookWindow : public QObject {
    Q_OBJECT
public:
    static QuickLookWindow& instance() {
        static QuickLookWindow s_instance;
        return s_instance;
    }

    void preview(const QString& path) { m_currentPath = path; }
    void closePreview() { m_currentPath.clear(); }
    QString currentPath() const { return m_currentPath; }
    void setIgnoreDeactivate(bool ignore) { Q_UNUSED(ignore); }

signals:
    void prevRequested();
    void nextRequested();
    void ratingRequested(int rating);
    void colorRequested(const QString& color);
    void deleteRequested(const QString& path);

private:
    explicit QuickLookWindow(QObject* parent = nullptr) : QObject(parent) {}
    ~QuickLookWindow() override = default;

    QString m_currentPath;
};

} // namespace RapidNotes
