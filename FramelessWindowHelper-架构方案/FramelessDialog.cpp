#include "FramelessDialog.h"
#include "UiHelper.h"
#include "FramelessWindowHelper.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QShortcut>
#include <QKeySequence>
#include <QCheckBox>
#include <QLineEdit>
#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

namespace QuarkMeta {

FramelessDialog::FramelessDialog(const QString& title, QWidget* parent) 
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Window) 
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    setWindowTitle(title);

#ifdef Q_OS_WIN
    DWORD attribute = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(reinterpret_cast<HWND>(winId()), 33, &attribute, sizeof(attribute));
#endif

    m_outerLayout = new QVBoxLayout(this);
    m_outerLayout->setContentsMargins(0, 0, 0, 0);

    m_container = new QWidget(this);
    m_container->setObjectName("DialogContainer");
    m_container->setAttribute(Qt::WA_StyledBackground);
    m_outerLayout->addWidget(m_container);

    m_mainLayout = new QVBoxLayout(m_container);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_titleBar = new QWidget();
    m_titleBar->setObjectName("TitleBar");
    m_titleBar->setFixedHeight(34);
    m_titleBar->setObjectName("FramelessTitleBar");
    m_titleLayout = new QHBoxLayout(m_titleBar);
    m_titleLayout->setContentsMargins(12, 0, 5, 0);
    m_titleLayout->setSpacing(4);

    m_titleLabel = new QLabel(title);
    m_titleLabel->setObjectName("FramelessTitleLabel");
    m_titleLayout->addWidget(m_titleLabel);
    m_titleLayout->addStretch();

    auto createTitleBtn = [this](const QString& iconName, const QString& tooltip, const QString& hoverColor) {
        Q_UNUSED(hoverColor);
        QPushButton* btn = new QPushButton();
        btn->setFixedSize(20, 20);
        btn->setIcon(UiHelper::getIcon(iconName, QColor("#CCCCCC"), 16));
        btn->setIconSize(QSize(16, 16));
        btn->setAutoDefault(false);
        btn->setProperty("tooltipText", tooltip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setObjectName("FramelessTitleBtn");
        btn->installEventFilter(this);
        return btn;
    };

    m_pinBtn = createTitleBtn("pin_tilted", "置顶", "#3E3E42");
    m_pinBtn->setCheckable(true);
    m_pinBtn->setObjectName("FramelessPinBtn");
    connect(m_pinBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_pinBtn->setIcon(UiHelper::getIcon(checked ? "pin_vertical" : "pin_tilted", 
                                            checked ? QColor("#FF551C") : QColor("#CCCCCC"), 18));
        FramelessWindowHelper::setAlwaysOnTop(this, checked);
    });

    m_minBtn = createTitleBtn("minimize", "最小化", "#3E3E42");
    connect(m_minBtn, &QPushButton::clicked, this, &QWidget::showMinimized);

    m_maxBtn = createTitleBtn("maximize", "最大化", "#3E3E42");
    connect(m_maxBtn, &QPushButton::clicked, this, [this]() {
        if (isMaximized()) {
            showNormal();
            m_maxBtn->setIcon(UiHelper::getIcon("maximize", QColor("#CCCCCC"), 18));
        } else {
            showMaximized();
            m_maxBtn->setIcon(UiHelper::getIcon("restore_line", QColor("#CCCCCC"), 18));
        }
    });

    m_closeBtn = new QPushButton();
    m_closeBtn->setFixedSize(20, 20);
    m_closeBtn->setIcon(UiHelper::getIcon("close", QColor("#FFFFFF"), 16));
    m_closeBtn->setIconSize(QSize(16, 16));
    m_closeBtn->setAutoDefault(false);
    m_closeBtn->setProperty("tooltipText", "关闭");
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setObjectName("FramelessCloseBtn");
    m_closeBtn->installEventFilter(this);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_titleLayout->addWidget(m_pinBtn);
    m_titleLayout->addWidget(m_minBtn);
    m_titleLayout->addWidget(m_maxBtn);
    m_titleLayout->addWidget(m_closeBtn);

    m_mainLayout->addWidget(m_titleBar);
    m_mainLayout->addSpacing(4);

    auto* line = new QFrame();
    line->setFixedHeight(1);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    line->setObjectName("FramelessLine");
    m_mainLayout->addWidget(line);

    m_contentArea = new QWidget();
    m_contentArea->setObjectName("DialogContentArea");
    m_mainLayout->addWidget(m_contentArea, 1);

    QShortcut* scClose = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
    scClose->setContext(Qt::WindowShortcut);
    connect(scClose, &QShortcut::activated, this, &QDialog::reject);

    // 统一接入 FramelessWindowHelper——对话框角色：标题栏拖拽 + 双击最大化 + 边缘缩放
    // 全部沿用与 MainWindow 相同的原生实现，不再各自维护一套拖拽逻辑
    m_framelessHelper = FramelessWindowHelper::apply(this, WindowRole::Dialog, m_titleBar);
}

void FramelessDialog::setVisibleButtons(int flags) {
    if (m_pinBtn) m_pinBtn->setVisible(flags & Pin);
    if (m_minBtn) m_minBtn->setVisible(flags & Min);
    if (m_maxBtn) m_maxBtn->setVisible(flags & Max);
    if (m_closeBtn) m_closeBtn->setVisible(flags & Close);
}

void FramelessDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
}

namespace {
bool isInteractiveWidget(QWidget* widget) {
    while (widget) {
        if (qobject_cast<QAbstractButton*>(widget) ||
            qobject_cast<QLineEdit*>(widget) ||
            qobject_cast<QCheckBox*>(widget)) {
            return true;
        }
        widget = widget->parentWidget();
    }
    return false;
}
} // namespace

void FramelessDialog::mousePressEvent(QMouseEvent* event) {
#ifndef Q_OS_WIN
    // Windows 平台的拖拽/缩放已由 FramelessWindowHelper 通过原生 WM_NCHITTEST/HTCAPTION 统一处理，
    // 这里只保留非 Windows 平台的 Qt 层兜底拖拽实现。
    if (event->button() == Qt::LeftButton) {
        QWidget* child = childAt(event->pos());
        if (!child || !isInteractiveWidget(child)) {
            m_isDragging = true;
            m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
#endif
    QDialog::mousePressEvent(event);
}

void FramelessDialog::mouseMoveEvent(QMouseEvent* event) {
#ifndef Q_OS_WIN
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
        return;
    }
#endif
    QDialog::mouseMoveEvent(event);
}

void FramelessDialog::mouseReleaseEvent(QMouseEvent* event) {
    m_isDragging = false;
    QDialog::mouseReleaseEvent(event);
}

void FramelessDialog::keyPressEvent(QKeyEvent* event) {
    if ((event->key() == Qt::Key_W && (event->modifiers() & Qt::ControlModifier))) {
        reject();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(event);
    }
}

bool FramelessDialog::eventFilter(QObject* watched, QEvent* event) {
    return QDialog::eventFilter(watched, event);
}

bool FramelessDialog::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (m_framelessHelper && m_framelessHelper->handleNativeEvent(message, result)) {
        return true;
    }
    return QDialog::nativeEvent(eventType, message, result);
}

} // namespace QuarkMeta
