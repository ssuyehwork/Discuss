#include "dialogs/FramelessConfirmDialog.h"
#include "UiHelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace QuarkMeta {

FramelessConfirmDialog::FramelessConfirmDialog(const QString& title, const QString& message, 
                                               ButtonType type, const QString& iconName, 
                                               const QColor& iconColor, QWidget* parent)
    : FramelessDialog(title, parent)
{
    setVisibleButtons(Close);
    resize(420, 180);
    setMinimumSize(380, 160);

    auto* layout = new QVBoxLayout(m_contentArea);
    layout->setContentsMargins(25, 20, 25, 20);
    layout->setSpacing(15);

    auto* msgLayout = new QHBoxLayout();
    msgLayout->setSpacing(15);

    if (!iconName.isEmpty()) {
        auto* iconLbl = new QLabel();
        iconLbl->setPixmap(UiHelper::getIcon(iconName, iconColor, 32).pixmap(32, 32));
        msgLayout->addWidget(iconLbl, 0, Qt::AlignTop);
    }

    auto* lbl = new QLabel(message);
    lbl->setObjectName("FramelessConfirmLabel");
    lbl->setWordWrap(true);
    lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    msgLayout->addWidget(lbl, 1);
    
    layout->addLayout(msgLayout, 1);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addStretch();
    
    if (type == OkCancel) {
        auto* btnCancel = new QPushButton("取消");
        btnCancel->setFixedSize(85, 30);
        btnCancel->setCursor(Qt::PointingHandCursor);
        btnCancel->setStyleSheet(
            "QPushButton { background-color: transparent; color: #999; border: 1px solid #444; border-radius: 4px; } "
            "QPushButton:hover { color: #EEE; background-color: #333; }"
        );
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        btnLayout->addWidget(btnCancel);
    }

    auto* btnOk = new QPushButton("确定");
    btnOk->setFixedSize(85, 30);
    btnOk->setCursor(Qt::PointingHandCursor);
    btnOk->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; border-radius: 4px; font-weight: bold; } "
        "QPushButton:hover { background-color: #2980b9; }"
    );
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(btnOk);

    layout->addLayout(btnLayout);
}

} // namespace QuarkMeta
