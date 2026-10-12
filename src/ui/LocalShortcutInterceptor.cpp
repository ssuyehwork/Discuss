#include "LocalShortcutInterceptor.h"
#include "AppShortcutController.h"
#include <QApplication>
#include <QMetaObject>
#include <QKeyCombination>
#include <QDebug>

namespace QuarkMeta {

LocalShortcutInterceptor& LocalShortcutInterceptor::instance() {
    static LocalShortcutInterceptor s_instance;
    return s_instance;
}

LocalShortcutInterceptor::LocalShortcutInterceptor(QObject* parent) : QObject(parent) {
    connect(qApp, &QApplication::aboutToQuit, this, [this]() {
        uninstallHook();
    });
}

LocalShortcutInterceptor::~LocalShortcutInterceptor() {
    uninstallHook();
}

ShortcutKeyCombo LocalShortcutInterceptor::parseQtKey(int key, Qt::KeyboardModifiers modifiers) {
    ShortcutKeyCombo combo;
    combo.ctrl = (modifiers & Qt::ControlModifier) != 0;
    combo.shift = (modifiers & Qt::ShiftModifier) != 0;
    combo.alt = (modifiers & Qt::AltModifier) != 0;

    int cleanKey = key & ~Qt::KeyboardModifierMask;

    if (cleanKey >= Qt::Key_A && cleanKey <= Qt::Key_Z) {
        combo.vkCode = 'A' + (cleanKey - Qt::Key_A);
    } else if (cleanKey >= Qt::Key_0 && cleanKey <= Qt::Key_9) {
        combo.vkCode = '0' + (cleanKey - Qt::Key_0);
    } else if (cleanKey >= Qt::Key_F1 && cleanKey <= Qt::Key_F24) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_F1 + (cleanKey - Qt::Key_F1);
#endif
    } else if (cleanKey == Qt::Key_Tab || cleanKey == Qt::Key_Backtab) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_TAB;
#endif
    } else if (cleanKey == Qt::Key_Space) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_SPACE;
#endif
    } else if (cleanKey == Qt::Key_Escape) {
#ifdef Q_OS_WIN
        combo.vkCode = VK_ESCAPE;
#endif
    }

    return combo;
}

ShortcutKeyCombo LocalShortcutInterceptor::parseKeySequence(const QKeySequence& sequence) {
    if (sequence.isEmpty()) return {};
    QKeyCombination combo = sequence[0];
    return parseQtKey(combo.key(), combo.keyboardModifiers());
}

void LocalShortcutInterceptor::registerProtectedShortcut(const QKeySequence& sequence, std::function<void()> callback) {
    ShortcutKeyCombo combo = parseKeySequence(sequence);
    if (combo.vkCode != 0 && callback) {
        m_callbacks.insert(combo, callback);
    }
}

void LocalShortcutInterceptor::registerProtectedShortcut(int key, Qt::KeyboardModifiers modifiers, std::function<void()> callback) {
    ShortcutKeyCombo combo = parseQtKey(key, modifiers);
    if (combo.vkCode != 0 && callback) {
        m_callbacks.insert(combo, callback);
    }
}

void LocalShortcutInterceptor::unregisterAllShortcuts() {
    m_callbacks.clear();
}

void LocalShortcutInterceptor::updateFocusState(bool isActive) {
    if (m_isActive == isActive) return;
    m_isActive = isActive;

    if (m_isActive) {
        installHook();
    } else {
        uninstallHook();
    }
}

void LocalShortcutInterceptor::installHook() {
#ifdef Q_OS_WIN
    if (m_hookInstalled || !m_isActive) return;
    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (m_hook) {
        m_hookInstalled = true;
    }
#endif
}

void LocalShortcutInterceptor::uninstallHook() {
#ifdef Q_OS_WIN
    if (m_hookInstalled && m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
        m_hookInstalled = false;
    }
#endif
}

#ifdef Q_OS_WIN
LRESULT CALLBACK LocalShortcutInterceptor::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        PKBDLLHOOKSTRUCT p = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);

        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

        if (LocalShortcutInterceptor::instance().handleLowLevelKey(static_cast<int>(p->vkCode), ctrl, shift, alt)) {
            return 1;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
#endif

bool LocalShortcutInterceptor::handleLowLevelKey(int vkCode, bool ctrl, bool shift, bool alt) {
    if (!m_isActive) return false;

    ShortcutKeyCombo combo;
    combo.vkCode = vkCode;
    combo.ctrl = ctrl;
    combo.shift = shift;
    combo.alt = alt;

    if (m_callbacks.contains(combo)) {
        if (AppShortcutController::isEditingFocus() && (vkCode == 'C' || vkCode == 'V' || vkCode == 'X' || vkCode == 'A' || vkCode == VK_TAB)) {
            return false;
        }

        auto callback = m_callbacks.value(combo);
        if (callback) {
            QMetaObject::invokeMethod(this, [callback]() {
                callback();
            }, Qt::QueuedConnection);
            return true;
        }
    }
    return false;
}

} // namespace QuarkMeta
