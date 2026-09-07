#ifndef QuarkMeta_LAST_OPERATION_MANAGER_H
#define QuarkMeta_LAST_OPERATION_MANAGER_H

#include <QString>
#include <QStringList>
#include <QVariant>

namespace QuarkMeta {

enum class LastOperationType {
    None,
    SetRating,
    SetColor,
    PasteTags
};

class LastOperationManager {
public:
    static LastOperationManager& instance() {
        static LastOperationManager inst;
        return inst;
    }

    bool hasOperation() const {
        return m_type != LastOperationType::None;
    }

    LastOperationType type() const { return m_type; }
    int rating() const { return m_rating; }
    QString color() const { return m_color; }
    QStringList tags() const { return m_tags; }

    void recordSetRating(int rating) {
        m_type = LastOperationType::SetRating;
        m_rating = rating;
    }

    void recordSetColor(const QString& color) {
        m_type = LastOperationType::SetColor;
        m_color = color;
    }

    void recordPasteTags(const QStringList& tags) {
        m_type = LastOperationType::PasteTags;
        m_tags = tags;
    }

    QString displayText() const {
        switch (m_type) {
            case LastOperationType::SetRating:
                return QString("重复上一次操作 (%1 星)").arg(m_rating);
            case LastOperationType::SetColor:
                return m_color.isEmpty() ? QString("重复上一次操作 (清除色标)") : QString("重复上一次操作 (标记颜色)");
            case LastOperationType::PasteTags:
                return QString("重复上一次操作 (粘贴 %1 个标签)").arg(m_tags.size());
            default:
                return "重复上一次操作";
        }
    }

private:
    LastOperationManager() = default;
    ~LastOperationManager() = default;
    LastOperationManager(const LastOperationManager&) = delete;
    LastOperationManager& operator=(const LastOperationManager&) = delete;

    LastOperationType m_type{LastOperationType::None};
    int m_rating{0};
    QString m_color;
    QStringList m_tags;
};

} // namespace QuarkMeta

#endif // QuarkMeta_LAST_OPERATION_MANAGER_H
