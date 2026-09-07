#include "MetaPreviewWidget.h"

namespace QuarkMeta {

MetaPreviewWidget::MetaPreviewWidget(QWidget* parent) : QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(6);

    m_lblImagePreview = new QLabel(this);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setFixedHeight(160);
    m_lblImagePreview->setStyleSheet("background: transparent; border: none;");

    QWidget* paletteWidget = new QWidget(this);
    m_paletteFlowLayout = new FlowLayout(paletteWidget, 0, 4, 4);

    m_layout->addWidget(m_lblImagePreview);
    m_layout->addWidget(paletteWidget);
}

void MetaPreviewWidget::setImagePreview(const QPixmap& pixmap) {
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
    } else {
        m_lblImagePreview->setPixmap(pixmap.scaled(240, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_lblImagePreview->show();
    }
}

void MetaPreviewWidget::setPalettes(const QVector<QPair<QColor, float>>& palette) {
    for (auto* pill : m_colorPool) {
        pill->hide();
    }

    int idx = 0;
    for (const auto& pair : palette) {
        ColorPill* pill = nullptr;
        if (idx < m_colorPool.size()) {
            pill = m_colorPool[idx];
            pill->setData(pair.first, pair.second);
        } else {
            pill = new ColorPill(pair.first, pair.second, this);
            m_paletteFlowLayout->addWidget(pill);
            m_colorPool.append(pill);
            connect(pill, &ColorPill::colorSelected, this, [this](const QColor& col) {
                emit searchByColor(col);
            });
        }
        pill->show();
        idx++;
    }
}

bool MetaPreviewWidget::hasContent() const {
    QPixmap pm = m_lblImagePreview ? m_lblImagePreview->pixmap() : QPixmap();
    return (!pm.isNull() && pm.height() > 0) || !m_colorPool.isEmpty();
}

} // namespace QuarkMeta
