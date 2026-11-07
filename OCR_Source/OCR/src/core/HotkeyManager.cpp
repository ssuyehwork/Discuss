#include "HotkeyManager.h"
#include <QCoreApplication>
#include <QDebug>
#include <QSettings>

HotkeyManager& HotkeyManager::instance() {
    static HotkeyManager inst;
    return inst;
}

HotkeyManager::HotkeyManager(QObject* parent) : QObject(parent) {
    qApp->installNativeEventFilter(this);
}

HotkeyManager::~HotkeyManager() {
#ifdef Q_OS_WIN
    unregisterHotkey(Screenshot);
    unregisterHotkey(OCR);
    unregisterHotkey(ColorPicker);
    unregisterHotkey(PixelRuler);
#endif
}

bool HotkeyManager::registerHotkey(int id, uint modifiers, uint vk) {
#ifdef Q_OS_WIN
    if (RegisterHotKey(nullptr, id, modifiers, vk)) {
        return true;
    }
    qWarning().noquote() << QString("[HotkeyManager] 注册热键失败 ID=%1, winErr=%2").arg(id).arg(GetLastError());
#else
    Q_UNUSED(id);
    Q_UNUSED(modifiers);
    Q_UNUSED(vk);
#endif
    return false;
}

void HotkeyManager::unregisterHotkey(int id) {
#ifdef Q_OS_WIN
    UnregisterHotKey(nullptr, id);
#else
    Q_UNUSED(id);
#endif
}

void HotkeyManager::reapplyHotkeys() {
    QSettings hotkeys("OCR", "Hotkeys");

    unregisterHotkey(Screenshot);
    unregisterHotkey(OCR);
    unregisterHotkey(ColorPicker);
    unregisterHotkey(PixelRuler);

    // 默认截图快捷键: F1 (vk: 0x70)
    uint s_mods = hotkeys.value("screenshot_mods", 0).toUInt();
    uint s_vk   = hotkeys.value("screenshot_vk", 0x70).toUInt();
    if (s_vk != 0) {
        registerHotkey(Screenshot, s_mods, s_vk);
    }

    // 默认 OCR 快捷键: F2 (vk: 0x71)
    uint o_mods = hotkeys.value("ocr_mods", 0).toUInt();
    uint o_vk   = hotkeys.value("ocr_vk", 0x71).toUInt();
    if (o_vk != 0) {
        registerHotkey(OCR, o_mods, o_vk);
    }

    // 屏幕拾色器快捷键: 可选
    uint c_mods = hotkeys.value("picker_mods", 0).toUInt();
    uint c_vk   = hotkeys.value("picker_vk", 0).toUInt();
    if (c_vk != 0) {
        registerHotkey(ColorPicker, c_mods, c_vk);
    }

    // 像素标尺快捷键: 可选
    uint r_mods = hotkeys.value("ruler_mods", 0).toUInt();
    uint r_vk   = hotkeys.value("ruler_vk", 0).toUInt();
    if (r_vk != 0) {
        registerHotkey(PixelRuler, r_mods, r_vk);
    }
}

bool HotkeyManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) {
    Q_UNUSED(result);
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_HOTKEY) {
            int id = static_cast<int>(msg->wParam);
            emit hotkeyPressed(id);
            return true;
        }
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
#endif
    return false;
}
