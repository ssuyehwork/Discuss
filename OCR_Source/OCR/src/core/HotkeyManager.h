#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class HotkeyManager : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    static HotkeyManager& instance();

    enum HotkeyId {
        Screenshot = 101,
        OCR = 102,
        ColorPicker = 103,
        PixelRuler = 104
    };

    bool registerHotkey(int id, uint modifiers, uint vk);
    void unregisterHotkey(int id);
    void reapplyHotkeys();

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    void hotkeyPressed(int id);

private:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager() override;
    HotkeyManager(const HotkeyManager&) = delete;
    HotkeyManager& operator=(const HotkeyManager&) = delete;
};

#endif // HOTKEYMANAGER_H
