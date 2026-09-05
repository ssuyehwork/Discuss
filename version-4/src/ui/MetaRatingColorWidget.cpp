#include "MetaRatingColorWidget.h"
#include "UiHelper.h"
#include "StyleLibrary.h"

namespace QuarkMeta {

MetaRatingColorWidget::MetaRatingColorWidget(QWidget* parent) : QWidget(parent) {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(6);

    // 星级布局
    QWidget* ratingRow = new QWidget(this);
    m_ratingLayout = new QHBoxLayout(ratingRow);
    m_ratingLayout->setContentsMargins(0, 0, 0, 0);
    m_ratingLayout->setSpacing(2);

    for (int i = 1; i <= 5; ++i) {
        QPushButton* star = new QPushButton(this);
        star->setFixedSize(20, 20);
        star->setIconSize(QSize(14, 14));
        star->setCursor(Qt::PointingHandCursor);
        m_starBtns.append(star);
        m_ratingLayout->addWidget(star);

        connect(star, &QPushButton::clicked, this, [this, i]() {
            int newRating = (m_currentRating == i) ? 0 : i;
            setRating(newRating);
            emit ratingChanged(newRating);
        });
    }
    m_ratingLayout->addStretch();

    // 颜色标记布局
    QWidget* colorRow = new QWidget(this);
    m_colorLayout = new QHBoxLayout(colorRow);
    m_colorLayout->setContentsMargins(0, 0, 0, 0);
    m_colorLayout->setSpacing(4);

    static const QStringList colors = {
        "#E24B4A", "#EF9F27", "#FECF0E", "#639922",
        "#1D9E75", "#378ADD", "#7F77DD", "#5F5E5A"
    };

    for (const QString& colHex : colors) {
        QPushButton* btn = new QPushButton(this);
        btn->setFixedSize(18, 18);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("colorHex", colHex.toUpper());
        btn->setObjectName("MetaRatingColorBtn");
        btn->setStyleSheet(QString("background: %1;").arg(colHex));
        m_colorBtns.append(btn);
        m_colorLayout->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, colHex]() {
            std::wstring wHex = colHex.toStdWString();
            std::wstring newColor = (m_currentColor == wHex) ? L"" : wHex;
            setColor(newColor);
            emit colorChanged(newColor);
        });
    }
    m_colorLayout->addStretch();

    m_mainLayout->addWidget(ratingRow);
    m_mainLayout->addWidget(colorRow);
}

void MetaRatingColorWidget::setRating(int rating) {
    m_currentRating = rating;
    for (int i = 0; i < m_starBtns.size(); ++i) {
        QColor col = (i < rating) ? Style::ActiveOrange : QColor("#444444");
        m_starBtns[i]->setIcon(UiHelper::getIcon("star_filled", col, 14));
    }
}

void MetaRatingColorWidget::setColor(const std::wstring& color) {
    m_currentColor = color;
    QString hex = QString::fromStdWString(color).toUpper();
    for (auto* btn : m_colorBtns) {
        btn->setText(btn->property("colorHex").toString() == hex ? "✓" : "");
    }
}

void MetaRatingColorWidget::setEnabledState(bool enabled) {
    for (auto* btn : m_starBtns) btn->setEnabled(enabled);
    for (auto* btn : m_colorBtns) btn->setEnabled(enabled);
}

} // namespace QuarkMeta
