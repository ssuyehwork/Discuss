#include "dialogs/FramelessInputDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

namespace QuarkMeta {

FramelessInputDialog::FramelessInputDialog(const QString& title, const QString& label, 
                                           const QString& initial, QWidget* parent)
    : FramelessDialog(title, parent) 
{
    setVisibleButtons(Close);
    resize(500, 210);
    setMinimumSize(400, 190);
    
    auto* layout = new QVBoxLayout(m_contentArea);
    layout->setContentsMargins(20, 15, 20, 20);
    layout->setSpacing(7);

    auto* lbl = new QLabel(label);
    lbl->setStyleSheet("color: #EEEEEE; font-size: 13px;");
    layout->addWidget(lbl);

    m_edit = new QLineEdit(initial);
    m_edit->setMinimumHeight(38);
    m_edit->setStyleSheet(
        "QLineEdit {"
        "  background-color: #2D2D2D; border: 1px solid #444; border-radius: 6px;"
        "  padding: 0px 10px; color: white; selection-background-color: #3498db;"
        "  font-size: 14px;"
        "}"
        "QLineEdit:focus { border: 1px solid #3498db; }"
    );
    layout->addWidget(m_edit);

    connect(m_edit, &QLineEdit::returnPressed, this, &QDialog::accept);

    layout->addStretch();

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
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(btnOk);

    layout->addLayout(btnLayout);

    m_edit->setFocus();
    m_edit->selectAll();
}

void FramelessInputDialog::showEvent(QShowEvent* event) {
    FramelessDialog::showEvent(event);
    QTimer::singleShot(50, m_edit, qOverload<>(&QWidget::setFocus));
}

void FramelessInputDialog::setEchoMode(QLineEdit::EchoMode mode) {
    if (m_edit) {
        m_edit->setEchoMode(mode);
    }
}

} // namespace QuarkMeta
