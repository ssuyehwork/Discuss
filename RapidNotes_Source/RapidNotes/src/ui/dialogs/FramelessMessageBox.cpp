#include "dialogs/FramelessMessageBox.h"
#include "dialogs/FramelessConfirmDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace RapidNotes {

FramelessMessageBox::FramelessMessageBox(const QString& title, const QString& text, QWidget* parent,
                                         bool okOnly, const QString& okText)
    : FramelessDialog(title, parent)
{
    if (m_pinBtn) m_pinBtn->hide();
    if (m_minBtn) m_minBtn->hide();
    if (m_maxBtn) m_maxBtn->hide();

    resize(480, 200);
    setMinimumSize(380, 160);

    auto* layout = new QVBoxLayout(m_contentArea);
    layout->setContentsMargins(25, 20, 25, 20);
    layout->setSpacing(15);

    auto* lbl = new QLabel(text);
    lbl->setWordWrap(true);
    lbl->setTextFormat(Qt::RichText);
    lbl->setStyleSheet("color: #eee; font-size: 13px; line-height: 140%;");
    layout->addWidget(lbl, 1);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addStretch();

    if (!okOnly) {
        auto* btnCancel = new QPushButton("取消");
        btnCancel->setFixedSize(85, 30);
        btnCancel->setCursor(Qt::PointingHandCursor);
        btnCancel->setStyleSheet(
            "QPushButton { background-color: transparent; color: #888; border: 1px solid #555; border-radius: 4px; } "
            "QPushButton:hover { color: #eee; border-color: #888; background-color: rgba(255, 255, 255, 0.05); }"
        );
        connect(btnCancel, &QPushButton::clicked, this, [this](){
            emit cancelled();
            reject();
        });
        btnLayout->addWidget(btnCancel);
    }

    m_btnOk = new QPushButton(okText.isEmpty() ? "确定" : okText);
    m_btnOk->setAutoDefault(true);
    int okBtnWidth = (m_btnOk->text().length() > 2) ? 95 : 85;
    m_btnOk->setFixedSize(okBtnWidth, 30);
    m_btnOk->setCursor(Qt::PointingHandCursor);
    m_btnOk->setStyleSheet(
        "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 4px; font-weight: bold; } "
        "QPushButton:hover { background-color: #c0392b; }"
    );
    connect(m_btnOk, &QPushButton::clicked, this, [this](){
        emit confirmed();
        accept();
    });
    btnLayout->addWidget(m_btnOk);

    layout->addLayout(btnLayout);
}

void FramelessMessageBox::showEvent(QShowEvent* event) {
    FramelessDialog::showEvent(event);
    if (m_btnOk) {
        m_btnOk->setFocus();
    }
}

void FramelessMessageBox::information(QWidget* parent, const QString& title, const QString& text) {
    FramelessConfirmDialog dlg(title, text, FramelessConfirmDialog::OkOnly, "info", QColor("#3498db"), parent);
    dlg.exec();
}

void FramelessMessageBox::warning(QWidget* parent, const QString& title, const QString& text) {
    FramelessConfirmDialog dlg(title, text, FramelessConfirmDialog::OkOnly, "warning", QColor("#f1c40f"), parent);
    dlg.exec();
}

bool FramelessMessageBox::question(QWidget* parent, const QString& title, const QString& text) {
    FramelessConfirmDialog dlg(title, text, FramelessConfirmDialog::OkCancel, "help", QColor("#3498db"), parent);
    return dlg.exec() == QDialog::Accepted;
}

void FramelessMessageBox::critical(QWidget* parent, const QString& title, const QString& text) {
    FramelessConfirmDialog dlg(title, text, FramelessConfirmDialog::OkOnly, "error", QColor("#e81123"), parent);
    dlg.exec();
}

} // namespace RapidNotes
