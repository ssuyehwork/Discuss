#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

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

class QListWidget;
class QStackedWidget;
class QPushButton;

class SettingsWindow : public FramelessDialog {
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget* parent = nullptr);
    ~SettingsWindow() override = default;

private slots:
    void onCategoryChanged(int index);
    void onSaveClicked();
    void onRestoreDefaults();
    void onBrowseSavePath();

private:
    void initUI();
    void loadSettings();
    QWidget* createHotkeyPage();
    QWidget* createScreenshotPage();

    QListWidget* m_navBar = nullptr;
    QStackedWidget* m_contentStack = nullptr;

    // 全局热键
    HotkeyKeyEdit* m_editScreenshot = nullptr;
    HotkeyKeyEdit* m_editOCR = nullptr;
    HotkeyKeyEdit* m_editColorPicker = nullptr;
    HotkeyKeyEdit* m_editPixelRuler = nullptr;

    // 截图保存路径
    QLineEdit* m_editSavePath = nullptr;
};

#endif // SETTINGSWINDOW_H
