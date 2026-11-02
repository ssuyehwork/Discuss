#ifndef KEYBOARDHOOK_H
#define KEYBOARDHOOK_H

#include <QObject>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

class KeyboardHook : public QObject {
    Q_OBJECT
public:
    static KeyboardHook& instance();
    void start();
    void stop();
    bool isActive() const { return m_active; }

    void setCapsLockToEnterEnabled(bool enabled) { m_capsLockToEnterEnabled = enabled; }

signals:
    void f4PressedInExplorer();
    void globalLockRequested();

private:
    bool m_capsLockToEnterEnabled = false;
    KeyboardHook();
    ~KeyboardHook();
    bool m_active = false;

#ifdef Q_OS_WIN
    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif
};

#endif // KEYBOARDHOOK_H
