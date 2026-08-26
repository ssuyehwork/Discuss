#include "dialogs/FramelessColorPicker.h"
#include "ColorPicker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

namespace QuarkMeta {

FramelessColorPicker::FramelessColorPicker(const QString& title, QWidget* parent)
    : FramelessDialog(title, parent)
{
    setVisibleButtons(Close);
    resize(360, 480);
    setMinimumSize(320, 400);

    auto* layout = new QVBoxLayout(m_contentArea);
    layout->setContentsMargins(15, 10, 15, 15);
    layout->setSpacing(10);

    m_picker = new ColorPicker(this);
    layout->addWidget(m_picker, 1);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    auto* btnCancel = new QPushButton("取消");
    btnCancel->setFixedSize(80, 32);
    btnCancel->setCursor(Qt::PointingHandCursor);
    btnCancel->setStyleSheet(
        "QPushButton { background-color: transparent; color: #888; border: 1px solid #444; border-radius: 4px; } "
        "QPushButton:hover { color: #EEE; background-color: #333; }"
    );
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(btnCancel);

    auto* btnOk = new QPushButton("确定");
    btnOk->setFixedSize(80, 32);
    btnOk->setCursor(Qt::PointingHandCursor);
    btnOk->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; border-radius: 4px; font-weight: bold; } "
        "QPushButton:hover { background-color: #2980b9; }"
    );
    connect(btnOk, &QPushButton::clicked, this, [this]() {
        m_selectedColor = m_picker->currentColor();
        accept();
    });
    btnLayout->addWidget(btnOk);

    layout->addLayout(btnLayout);
}

void FramelessColorPicker::setCurrentColor(const QColor& color) {
    m_selectedColor = color;
    m_picker->setCurrentColor(color);
}

} // namespace QuarkMeta
