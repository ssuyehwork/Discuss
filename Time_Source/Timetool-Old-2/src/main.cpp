#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTimer>
#include <QDateTime>
#include <QShortcut>
#include <QClipboard>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QVector>
#include <QSettings>
#include <QDebug>
#include <QLineEdit>
#include <QIcon>
#include <QKeyEvent>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

// ============================================================================
// KeyboardHook: 全局按键拦截 (支持组合键)
// ============================================================================
struct HotkeyConfig {
    int vk = 0;
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    QString name = "F1";

    bool operator==(const HotkeyConfig& other) const {
        return vk == other.vk && ctrl == other.ctrl && alt == other.alt && shift == other.shift;
    }
};
Q_DECLARE_METATYPE(HotkeyConfig)

class KeyboardHook : public QObject {
    Q_OBJECT
public:
    static KeyboardHook& instance() {
        static KeyboardHook inst;
        return inst;
    }

    void start() {
#ifdef Q_OS_WIN
        if (m_hHook) return;
        m_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, HookProc, GetModuleHandle(NULL), 0);
#endif
    }

    void stop() {
#ifdef Q_OS_WIN
        if (m_hHook) {
            UnhookWindowsHookEx(m_hHook);
            m_hHook = nullptr;
        }
#endif
    }

    void setDigitInterceptEnabled(bool enabled) { m_digitInterceptEnabled = enabled; }
    void setPaused(bool paused) { m_isPaused = paused; }
    bool isPaused() const { return m_isPaused; }
    
    void setHotkey(const HotkeyConfig& config) { m_hotkey = config; }
    HotkeyConfig hotkey() const { return m_hotkey; }

    void setRecordingMode(bool recording) { 
        m_isRecording = recording; 
        if (recording) {
            m_ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000);
            m_altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000);
            m_shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000);
        }
    }

signals:
    void digitPressed(int digit);
    void pauseToggled();
    void hotkeyRecorded(HotkeyConfig config);
    void exitRequested();

private:
    KeyboardHook() : m_hHook(nullptr), m_digitInterceptEnabled(false), m_isPaused(false), m_isRecording(false),
                     m_ctrlPressed(false), m_altPressed(false), m_shiftPressed(false) {
        m_hotkey.vk = VK_F1;
        m_hotkey.name = "F1";
    }
    ~KeyboardHook() { stop(); }

#ifdef Q_OS_WIN
    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION) {
            KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
            bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

            // 实时跟踪修饰键状态 (解决录制时吞噬按键导致 GetAsyncKeyState 状态不更新的问题)
            if (pKey->vkCode == VK_LCONTROL || pKey->vkCode == VK_RCONTROL || pKey->vkCode == VK_CONTROL) {
                KeyboardHook::instance().m_ctrlPressed = isKeyDown;
            } else if (pKey->vkCode == VK_LMENU || pKey->vkCode == VK_RMENU || pKey->vkCode == VK_MENU) {
                KeyboardHook::instance().m_altPressed = isKeyDown;
            } else if (pKey->vkCode == VK_LSHIFT || pKey->vkCode == VK_RSHIFT || pKey->vkCode == VK_SHIFT) {
                KeyboardHook::instance().m_shiftPressed = isKeyDown;
            }
            
            bool ctrl = KeyboardHook::instance().m_ctrlPressed;
            bool alt = KeyboardHook::instance().m_altPressed;
            bool shift = KeyboardHook::instance().m_shiftPressed;

            if (!(pKey->flags & LLKHF_INJECTED)) {
                // 1. 录制模式补丁：强制吞噬并转发，防止泄露给其他后台应用
                if (KeyboardHook::instance().m_isRecording) {
                    if (isKeyDown) {
                        // 过滤掉单纯的修饰键按下
                        if (pKey->vkCode != VK_CONTROL && pKey->vkCode != VK_LCONTROL && pKey->vkCode != VK_RCONTROL &&
                            pKey->vkCode != VK_MENU && pKey->vkCode != VK_LMENU && pKey->vkCode != VK_RMENU &&
                            pKey->vkCode != VK_SHIFT && pKey->vkCode != VK_LSHIFT && pKey->vkCode != VK_RSHIFT) 
                        {
                            HotkeyConfig cfg;
                            cfg.vk = pKey->vkCode;
                            cfg.ctrl = ctrl;
                            cfg.alt = alt;
                            cfg.shift = shift;
                            
                            QStringList parts;
                            if (cfg.ctrl) parts << "Ctrl";
                            if (cfg.alt) parts << "Alt";
                            if (cfg.shift) parts << "Shift";
                            
                            // 简单映射常见按键名
                            QString name;
                            if (cfg.vk >= 'A' && cfg.vk <= 'Z') name = QString(QChar(cfg.vk));
                            else if (cfg.vk >= '0' && cfg.vk <= '9') name = QString(QChar(cfg.vk));
                            else if (cfg.vk >= VK_F1 && cfg.vk <= VK_F24) name = QString("F%1").arg(cfg.vk - VK_F1 + 1);
                            else {
                                switch(cfg.vk) {
                                    case VK_SPACE: name = "Space"; break;
                                    case VK_RETURN: name = "Enter"; break;
                                    case VK_ESCAPE: name = "Esc"; break;
                                    case VK_OEM_3: name = "~"; break;
                                    default: name = QString("Key_%1").arg(cfg.vk); break;
                                }
                            }
                            parts << name;
                            cfg.name = parts.join("+");
                            
                            emit KeyboardHook::instance().hotkeyRecorded(cfg);
                        }
                    }
                    return 1; 
                }

                // 2. Ctrl + W 全局退出
                if (ctrl && pKey->vkCode == 'W' && isKeyDown) {
                    emit KeyboardHook::instance().exitRequested();
                    return 1;
                }

                // 3. 匹配热键
                const auto& target = KeyboardHook::instance().m_hotkey;
                if (pKey->vkCode == target.vk && ctrl == target.ctrl && alt == target.alt && shift == target.shift) {
                    if (isKeyDown) {
                        emit KeyboardHook::instance().pauseToggled();
                    }
                    return 1;
                }

                // 4. 数字拦截
                if (KeyboardHook::instance().m_digitInterceptEnabled && !KeyboardHook::instance().m_isPaused) {
                    if (pKey->vkCode >= 0x30 && pKey->vkCode <= 0x39 && !ctrl && !alt) {
                        if (isKeyDown) {
                            int digit = pKey->vkCode - 0x30;
                            emit KeyboardHook::instance().digitPressed(digit);
                        }
                        return 1;
                    }
                }
            }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    HHOOK m_hHook;
#endif
    bool m_digitInterceptEnabled;
    bool m_isPaused;
    HotkeyConfig m_hotkey;
    bool m_isRecording;
    bool m_ctrlPressed, m_altPressed, m_shiftPressed;
};

// ============================================================================
// StateIconBtn: 状态图标按钮 (标题栏专用)
// ============================================================================
class StateIconBtn : public QPushButton {
    Q_OBJECT
public:
    explicit StateIconBtn(QWidget* parent = nullptr) : QPushButton(parent), m_isPaused(false) {
        setFixedSize(24, 24);
        setCursor(Qt::PointingHandCursor);
        setStyleSheet("QPushButton { background-color: transparent; border: none; }");
    }

    void setPaused(bool paused) {
        m_isPaused = paused;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        if (isDown()) {
            painter.setBrush(QColor("#555555"));
        } else if (underMouse()) {
            painter.setBrush(QColor("#444444"));
        } else {
            painter.setBrush(Qt::transparent);
        }
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect(), 4, 4);

        int side = 18;
        int x = (width() - side) / 2;
        int y = (height() - side) / 2;

        painter.setBrush(QColor("#FF8C00"));
        if (m_isPaused) {
            QPainterPath path;
            path.moveTo(x, y);
            path.lineTo(x + side, y + side / 2);
            path.lineTo(x, y + side);
            path.closeSubpath();
            painter.drawPath(path);
        } else {
            painter.drawRect(x, y, 5, side);
            painter.drawRect(x + side - 5, y, 5, side);
        }
    }

private:
    bool m_isPaused;
};

// ============================================================================
// HotkeyEdit: 专业热键编辑器 (修复焦点与按键泄露)
// ============================================================================
class HotkeyEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit HotkeyEdit(QWidget* parent = nullptr) : QLineEdit(parent) {
        setReadOnly(true);
        setAlignment(Qt::AlignCenter);
        setStyleSheet(
            "QLineEdit {"
            "  color: #FF8C00;"
            "  background-color: transparent;"
            "  border: none;"
            "  font-size: 12px;"
            "  font-weight: bold;"
            "}"
            "QLineEdit:focus { background-color: rgba(255, 140, 0, 0.1); }"
        );
        connect(&KeyboardHook::instance(), &KeyboardHook::hotkeyRecorded, this, &HotkeyEdit::onGlobalKeyRecorded);
    }

signals:
    void hotkeyChanged(HotkeyConfig config);

private slots:
    void onGlobalKeyRecorded(HotkeyConfig cfg) {
        if (!hasFocus()) return;
        setText(cfg.name);
        emit hotkeyChanged(cfg);
        clearFocus();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        setFocus();
        QLineEdit::mousePressEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override {
        // 全局钩子已接管录制，此处仅屏蔽默认行为
        if (event->key() == Qt::Key_Escape) clearFocus();
    }

    void focusInEvent(QFocusEvent* event) override {
        QLineEdit::focusInEvent(event);
        setText("请按下组合键...");
#ifdef Q_OS_WIN
        HWND hwnd = (HWND)window()->winId();
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~WS_EX_NOACTIVATE);
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
#endif
        KeyboardHook::instance().setRecordingMode(true);
    }

    void focusOutEvent(QFocusEvent* event) override {
        QLineEdit::focusOutEvent(event);
        KeyboardHook::instance().setRecordingMode(false);
        HotkeyConfig cfg = KeyboardHook::instance().hotkey();
        setText(cfg.name);
#ifdef Q_OS_WIN
        HWND hwnd = (HWND)window()->winId();
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_NOACTIVATE);
#endif
    }
};

// ============================================================================
// FramelessDialog: 基础无边框窗口 (修复按钮点击拦截)
// ============================================================================
class FramelessDialog : public QDialog {
    Q_OBJECT
public:
    explicit FramelessDialog(const QString& title, QWidget* parent = nullptr) 
        : QDialog(parent, Qt::FramelessWindowHint | Qt::Window) 
    {
        setAttribute(Qt::WA_TranslucentBackground);
        
        m_outerLayout = new QVBoxLayout(this);
        m_outerLayout->setContentsMargins(12, 12, 12, 12);

        m_container = new QWidget(this);
        m_container->setObjectName("DialogContainer");
        m_container->setStyleSheet(
            "#DialogContainer {"
            "  background-color: #1e1e1e;"
            "  border: 1px solid #333333;"
            "  border-radius: 8px;"
            "}"
        );
        m_outerLayout->addWidget(m_container);

        auto* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(15);
        shadow->setXOffset(0); shadow->setYOffset(2);
        shadow->setColor(QColor(0, 0, 0, 90));
        m_container->setGraphicsEffect(shadow);

        m_mainLayout = new QVBoxLayout(m_container);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(0);

        auto* titleBar = new QWidget();
        titleBar->setFixedHeight(36);
        titleBar->setStyleSheet("background-color: transparent; border-bottom: 1px solid #2D2D2D;");
        auto* titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(10, 0, 5, 0);
        titleLayout->setSpacing(8);

        auto* logoLabel = new QLabel();
        QPixmap logoPixmap(":/icons/app_logo.svg");
        if (logoPixmap.isNull()) {
            logoPixmap = QIcon(":/app_icon.png").pixmap(18, 18);
        }
        logoLabel->setPixmap(logoPixmap.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        logoLabel->setFixedSize(18, 18);
        logoLabel->setStyleSheet("border: none; background: transparent;");
        titleLayout->addWidget(logoLabel);

        auto* titleLabel = new QLabel(title);
        titleLabel->setStyleSheet("color: #888; font-size: 12px; font-weight: bold; border: none;");
        titleLayout->addWidget(titleLabel);
        titleLayout->addStretch();

        m_stateBtn = new StateIconBtn();
        connect(m_stateBtn, &QPushButton::clicked, this, &FramelessDialog::onStateBtnClicked);
        titleLayout->addWidget(m_stateBtn);

        auto* closeBtn = new QPushButton("×");
        closeBtn->setFixedSize(24, 24);
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setStyleSheet(
            "QPushButton { background-color: #E81123; color: white; border: none; border-radius: 4px; font-size: 16px; } "
            "QPushButton:hover { background-color: #D71520; border-radius: 4px; }"
        );
        connect(closeBtn, &QPushButton::clicked, this, &QApplication::quit);
        titleLayout->addWidget(closeBtn);

        m_mainLayout->addWidget(titleBar);

        m_contentArea = new QWidget();
        m_mainLayout->addWidget(m_contentArea, 1);
        
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }

protected:
    void onStateBtnClicked() { emit stateToggleRequested(); }

#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_NCHITTEST) {
            int x = GET_X_LPARAM(msg->lParam);
            int y = GET_Y_LPARAM(msg->lParam);
            QPoint pos = mapFromGlobal(QPoint(x, y));
            
            QList<QPushButton*> btns = findChildren<QPushButton*>();
            for (auto* b : btns) {
                if (b->isVisible()) {
                    QPoint btnPos = b->mapTo(this, QPoint(0, 0));
                    QRect btnRect(btnPos, b->size());
                    if (btnRect.contains(pos)) return false; 
                }
            }

            if (pos.y() < 36) {
                *result = HTCAPTION;
                return true;
            }
        }
        return QDialog::nativeEvent(eventType, message, result);
    }
#endif

signals:
    void stateToggleRequested();

protected:
    QWidget* m_container;
    QVBoxLayout* m_outerLayout;
    QVBoxLayout* m_mainLayout;
    QWidget* m_contentArea;
    StateIconBtn* m_stateBtn;
};

// ============================================================================
// TimePasteWindow: 主窗口 (极简布局 + 绝对底部热键栏)
// ============================================================================
class TimePasteWindow : public FramelessDialog {
    Q_OBJECT
public:
    explicit TimePasteWindow(QWidget* parent = nullptr) : FramelessDialog("时间输出工具", parent) {
        setFixedSize(380, 380);
        setWindowIcon(QIcon(":/icons/app_logo.svg"));
        initUI();

        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &TimePasteWindow::updateDateTime);
        m_timer->start(100);

        connect(&KeyboardHook::instance(), &KeyboardHook::digitPressed, this, &TimePasteWindow::onDigitPressed, Qt::QueuedConnection);
        connect(&KeyboardHook::instance(), &KeyboardHook::pauseToggled, this, &TimePasteWindow::togglePause, Qt::QueuedConnection);
        connect(&KeyboardHook::instance(), &KeyboardHook::exitRequested, qApp, &QApplication::quit, Qt::QueuedConnection);
        connect(this, &FramelessDialog::stateToggleRequested, this, &TimePasteWindow::togglePause);
        
        loadSettings();
    }

protected:
    void showEvent(QShowEvent* event) override {
        FramelessDialog::showEvent(event);
        KeyboardHook::instance().setDigitInterceptEnabled(true);
#ifdef Q_OS_WIN
        HWND hwnd = (HWND)winId();
        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle | WS_EX_NOACTIVATE | WS_EX_APPWINDOW);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
#endif
    }

    void hideEvent(QHideEvent* event) override {
        KeyboardHook::instance().setDigitInterceptEnabled(false);
        saveSettings();
        FramelessDialog::hideEvent(event);
    }

private slots:
    void onHotkeyChanged(HotkeyConfig cfg) {
        KeyboardHook::instance().setHotkey(cfg);
    }

    void togglePause() {
        bool newState = !KeyboardHook::instance().isPaused();
        KeyboardHook::instance().setPaused(newState);
        m_stateBtn->setPaused(newState);
        m_timeLabel->setStyleSheet(newState ? "color: #666; font-size: 32px; font-weight: bold; font-family: 'Consolas';" : "color: #FF8C00; font-size: 32px; font-weight: bold; font-family: 'Consolas';");
    }

    void updateDateTime() {
        QDateTime now = QDateTime::currentDateTime();
        m_dateLabel->setText(now.toString("yyyy-MM-dd"));
        m_timeLabel->setText(now.toString("HH:mm:ss"));
    }

    void onDigitPressed(int digit) {
        if (!isVisible() || KeyboardHook::instance().isPaused()) return;
        QDateTime target = QDateTime::currentDateTime().addSecs((m_radioPrev->isChecked() ? -digit : digit) * 60);
        QString timeStr = target.toString("HH:mm");
#ifdef Q_OS_WIN
        QTimer::singleShot(10, [timeStr]() {
            INPUT releaseInputs[8]; memset(releaseInputs, 0, sizeof(releaseInputs));
            BYTE keys[] = { VK_LCONTROL, VK_RCONTROL, VK_LSHIFT, VK_RSHIFT, VK_LMENU, VK_RMENU, VK_LWIN, VK_RWIN };
            for (int i = 0; i < 8; ++i) { releaseInputs[i].type = INPUT_KEYBOARD; releaseInputs[i].ki.wVk = keys[i]; releaseInputs[i].ki.dwFlags = KEYEVENTF_KEYUP; }
            SendInput(8, releaseInputs, sizeof(INPUT));
            int len = timeStr.length(); QVector<INPUT> inputs(len * 2);
            for (int i = 0; i < len; ++i) {
                inputs[i*2].type = INPUT_KEYBOARD; inputs[i*2].ki.wScan = timeStr[i].unicode(); inputs[i*2].ki.dwFlags = KEYEVENTF_UNICODE;
                inputs[i*2+1] = inputs[i*2]; inputs[i*2+1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
            }
            SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
        });
#endif
    }

private:
    void initUI() {
        auto* contentLayout = new QVBoxLayout(m_contentArea);
        contentLayout->setContentsMargins(25, 15, 25, 10);
        contentLayout->setSpacing(15);

        m_dateLabel = new QLabel(); m_dateLabel->setAlignment(Qt::AlignCenter);
        m_dateLabel->setStyleSheet("color: #888; font-size: 16px;");
        contentLayout->addWidget(m_dateLabel);

        m_timeLabel = new QLabel(); m_timeLabel->setAlignment(Qt::AlignCenter);
        m_timeLabel->setStyleSheet("color: #FF8C00; font-size: 32px; font-weight: bold; font-family: 'Consolas';");
        contentLayout->addWidget(m_timeLabel);

        auto* sep = new QLabel(); sep->setFixedHeight(1); sep->setStyleSheet("background-color: #333;");
        contentLayout->addWidget(sep);

        // 复选框/单选框样式 (16x16, 2px 圆角)
        auto radioStyle = "QRadioButton, QCheckBox { color: #BBB; font-size: 14px; spacing: 10px; } "
                          "QRadioButton::indicator, QCheckBox::indicator { width: 16px; height: 16px; border-radius: 2px; border: 2px solid #444; background: #252525; } "
                          "QRadioButton::indicator:checked, QCheckBox::indicator:checked { background-color: #FF8C00; border: 2px solid #FF8C00; }";

        m_radioPrev = new QRadioButton("退 (往前 N 分钟)"); m_radioPrev->setChecked(true);
        m_radioPrev->setStyleSheet(radioStyle);
        contentLayout->addWidget(m_radioPrev);

        m_radioNext = new QRadioButton("进 (往后 N 分钟)");
        m_radioNext->setStyleSheet(radioStyle);
        contentLayout->addWidget(m_radioNext);

        m_buttonGroup = new QButtonGroup(this);
        m_buttonGroup->addButton(m_radioPrev, 0); m_buttonGroup->addButton(m_radioNext, 1);

        auto* tip = new QLabel("按 0-9 输出时间 | Ctrl+W 退出程序");
        tip->setAlignment(Qt::AlignCenter);
        tip->setStyleSheet("color: #555; font-size: 11px;");
        contentLayout->addWidget(tip);

        // 绝对底部热键栏 (HotkeyEdit)
        auto* hotkeyBar = new QWidget();
        hotkeyBar->setFixedHeight(42);
        hotkeyBar->setStyleSheet("background-color: #222; border-top: 1px solid #333; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px;");
        auto* hbLayout = new QHBoxLayout(hotkeyBar);
        hbLayout->setContentsMargins(20, 0, 20, 0);
        hbLayout->setSpacing(10);

        auto* label = new QLabel("自定义热键:");
        label->setStyleSheet("color: #888; font-size: 12px;");
        hbLayout->addWidget(label);

        m_hotkeyEdit = new HotkeyEdit();
        connect(m_hotkeyEdit, &HotkeyEdit::hotkeyChanged, this, &TimePasteWindow::onHotkeyChanged);
        hbLayout->addWidget(m_hotkeyEdit, 1);

        m_mainLayout->addWidget(hotkeyBar);
    }

    void loadSettings() {
        QSettings s("RapidTimeTool", "Window");
        if (s.contains("pos")) move(s.value("pos").toPoint());
        m_buttonGroup->button(s.value("mode", 0).toInt())->setChecked(true);
        HotkeyConfig cfg;
        cfg.vk = s.value("hotkeyVK", VK_F1).toInt();
        cfg.ctrl = s.value("hotkeyCtrl", false).toBool();
        cfg.alt = s.value("hotkeyAlt", false).toBool();
        cfg.shift = s.value("hotkeyShift", false).toBool();
        cfg.name = s.value("hotkeyName", "F1").toString();
        KeyboardHook::instance().setHotkey(cfg);
        m_hotkeyEdit->setText(cfg.name);
    }

    void saveSettings() {
        QSettings s("RapidTimeTool", "Window");
        s.setValue("pos", pos());
        s.setValue("mode", m_buttonGroup->checkedId());
        const auto& cfg = KeyboardHook::instance().hotkey();
        s.setValue("hotkeyVK", cfg.vk); s.setValue("hotkeyCtrl", cfg.ctrl);
        s.setValue("hotkeyAlt", cfg.alt); s.setValue("hotkeyShift", cfg.shift);
        s.setValue("hotkeyName", cfg.name);
    }

    QLabel *m_dateLabel, *m_timeLabel;
    QRadioButton *m_radioPrev, *m_radioNext;
    QButtonGroup* m_buttonGroup;
    QTimer* m_timer;
    HotkeyEdit* m_hotkeyEdit;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    qRegisterMetaType<HotkeyConfig>("HotkeyConfig");
    KeyboardHook::instance().start();
    TimePasteWindow w; w.show();
    int r = a.exec();
    KeyboardHook::instance().stop();
    return r;
}

#include "main.moc"
