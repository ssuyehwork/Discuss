#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QList>
#include <string>

namespace QuarkMeta {

class MetaRatingColorWidget : public QWidget {
    Q_OBJECT
public:
    explicit MetaRatingColorWidget(QWidget* parent = nullptr);
    ~MetaRatingColorWidget() override = default;

    void setRating(int rating);
    void setColor(const std::wstring& color);
    void setEnabledState(bool enabled);

signals:
    void ratingChanged(int rating);
    void colorChanged(const std::wstring& color);

private:
    QVBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_ratingLayout = nullptr;
    QHBoxLayout* m_colorLayout = nullptr;

    QList<QPushButton*> m_starBtns;
    QList<QPushButton*> m_colorBtns;

    int m_currentRating = 0;
    std::wstring m_currentColor;
};

} // namespace QuarkMeta
