#include "AppLockWidget.h"
#include "IconHelper.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPainter>
#include <QKeyEvent>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

AppLockWidget::AppLockWidget(const QString& hint, QWidget* parent)
    : QWidget(parent) {
    setObjectName("AppLockWidget");
    setFocusPolicy(Qt::StrongFocus);

    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);

    auto* appTitle = new QLabel("懒人笔记");
    appTitle->setObjectName("QuickWindow_appTitle_1");
    appTitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(appTitle);

    auto* lockIcon = new QLabel();
    lockIcon->setPixmap(IconHelper::getIcon("lock_secure", "#00A650").pixmap(64, 64));
    lockIcon->setAlignment(Qt::AlignCenter);
    layout->addWidget(lockIcon);

    auto* titleLabel = new QLabel("已锁定");
    titleLabel->setObjectName("QuickWindow_titleLabel_2");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    auto* hintLabel = new QLabel(hint);
    hintLabel->setObjectName("QuickWindow_hintLabel_3");
    hintLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(hintLabel);

    m_pwdEdit = new QLineEdit();
    m_pwdEdit->setObjectName("QuickWindow_m_pwdEdit");
    m_pwdEdit->setEchoMode(QLineEdit::Password);
    m_pwdEdit->setPlaceholderText("请输入密码");
    m_pwdEdit->setFixedWidth(240);
    m_pwdEdit->setFixedHeight(36);
    m_pwdEdit->setAlignment(Qt::AlignCenter);
    connect(m_pwdEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_unlocking) return;
        emit passwordSubmitted(m_pwdEdit->text());
    });
    layout->addWidget(m_pwdEdit, 0, Qt::AlignHCenter);

    m_closeBtn = new QPushButton(this);
    m_closeBtn->setObjectName("QuickWindow_m_closeBtn");
    m_closeBtn->setIcon(IconHelper::getIcon("close", "#aaaaaa"));
    m_closeBtn->setIconSize(QSize(18, 18));
    m_closeBtn->setFixedSize(32, 32);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, []() { QApplication::quit(); });

    m_pwdEdit->setFocus();
}

void AppLockWidget::focusInput() {
    m_pwdEdit->setFocus();
    m_pwdEdit->selectAll();
}

void AppLockWidget::paintEvent(QPaintEvent*) {
    // 安全遮罩背景：自绘，不依赖样式表
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#1C1C1C"));
    p.drawRoundedRect(rect(), 10, 10);
}

void AppLockWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void AppLockWidget::resizeEvent(QResizeEvent* event) {
    m_closeBtn->move(width() - m_closeBtn->width() - 10, 10);
    QWidget::resizeEvent(event);
}

void AppLockWidget::rejectPassword() {
    m_pwdEdit->clear();
    auto* anim = new QPropertyAnimation(m_pwdEdit, "pos");
    anim->setDuration(400);
    anim->setLoopCount(1);

    const QPoint pos = m_pwdEdit->pos();
    anim->setKeyValueAt(0, pos);
    anim->setKeyValueAt(0.1, pos + QPoint(-10, 0));
    anim->setKeyValueAt(0.3, pos + QPoint(10, 0));
    anim->setKeyValueAt(0.5, pos + QPoint(-10, 0));
    anim->setKeyValueAt(0.7, pos + QPoint(10, 0));
    anim->setKeyValueAt(0.9, pos + QPoint(-10, 0));
    anim->setKeyValueAt(1, pos);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void AppLockWidget::playUnlockFade() {
    if (m_unlocking) return;
    m_unlocking = true;
    m_pwdEdit->setEnabled(false);

    auto* opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect);
    auto* animation = new QPropertyAnimation(opacityEffect, "opacity");
    animation->setDuration(300);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(animation, &QPropertyAnimation::finished, this, [this]() {
        emit unlockFinished();
    });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
