#ifndef HOTKEYSETTINGSDIALOG_H
#define HOTKEYSETTINGSDIALOG_H

#include "FramelessDialog.h"
#include <QLineEdit>
#include <QKeyEvent>

class HotkeyKeyEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit HotkeyKeyEdit(QWidget* parent = nullptr);
    void setKeyData(uint mods, uint vk);
    uint mods() const { return m_mods; }
    uint vk() const { return m_vk; }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    static QString keyToString(uint mods, uint vk);
    uint m_mods = 0;
    uint m_vk = 0;
};

class HotkeySettingsDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit HotkeySettingsDialog(QWidget* parent = nullptr);
    ~HotkeySettingsDialog() override = default;

private slots:
    void onSaveClicked();
    void onResetDefaultsClicked();

private:
    void initUI();
    void loadSettings();

    HotkeyKeyEdit* m_editScreenshot = nullptr;
    HotkeyKeyEdit* m_editOCR = nullptr;
    HotkeyKeyEdit* m_editColorPicker = nullptr;
    HotkeyKeyEdit* m_editPixelRuler = nullptr;
};

#endif // HOTKEYSETTINGSDIALOG_H
