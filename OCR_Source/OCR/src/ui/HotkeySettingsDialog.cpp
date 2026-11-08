#include "HotkeySettingsDialog.h"
#include "ToolTipOverlay.h"
#include "../core/HotkeyManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QKeySequence>

HotkeyKeyEdit::HotkeyKeyEdit(QWidget* parent) : QLineEdit(parent) {
    setObjectName("HotkeyKeyEdit");
    setReadOnly(true);
    setPlaceholderText("点击并按下按键组合...");
    setAlignment(Qt::AlignCenter);
    setFixedHeight(32);
}

void HotkeyKeyEdit::setKeyData(uint mods, uint vk) {
    m_mods = mods;
    m_vk = vk;
    setText(keyToString(mods, vk));
}

void HotkeyKeyEdit::keyPressEvent(QKeyEvent* event) {
    int key = event->key();
    if (key == Qt::Key_Escape || key == Qt::Key_Backspace || key == Qt::Key_Delete) {
        m_mods = 0;
        m_vk = 0;
        setText("无");
        return;
    }

    if (key >= Qt::Key_Control && key <= Qt::Key_Meta) return;

    uint winMods = 0;
    if (event->modifiers() & Qt::ControlModifier) winMods |= 0x0002; // MOD_CONTROL
    if (event->modifiers() & Qt::AltModifier)     winMods |= 0x0001; // MOD_ALT
    if (event->modifiers() & Qt::ShiftModifier)   winMods |= 0x0004; // MOD_SHIFT
    if (event->modifiers() & Qt::MetaModifier)    winMods |= 0x0008; // MOD_WIN

    m_mods = winMods;
    m_vk = event->nativeVirtualKey();
    if (m_vk == 0) m_vk = key;

    setText(keyToString(m_mods, m_vk));
}

QString HotkeyKeyEdit::keyToString(uint mods, uint vk) {
    if (vk == 0) return "无";
    QStringList parts;
    if (mods & 0x0002) parts << "Ctrl";
    if (mods & 0x0001) parts << "Alt";
    if (mods & 0x0004) parts << "Shift";
    if (mods & 0x0008) parts << "Win";

    QString str;
    if (vk >= 0x70 && vk <= 0x87) { // F1 - F24
        str = QString("F%1").arg(vk - 0x70 + 1);
    } else if (vk >= 0x30 && vk <= 0x39) { // 0 - 9
        str = QString(QChar(vk));
    } else if (vk >= 0x41 && vk <= 0x5A) { // A - Z
        str = QString(QChar(vk));
    } else {
        switch (vk) {
            case 0x20: str = "Space"; break;
            case 0x0D: str = "Enter"; break;
            case 0x1B: str = "Esc"; break;
            case 0x09: str = "Tab"; break;
            case 0x08: str = "Backspace"; break;
            case 0x2E: str = "Delete"; break;
            case 0x2D: str = "Insert"; break;
            case 0x24: str = "Home"; break;
            case 0x23: str = "End"; break;
            case 0x21: str = "PageUp"; break;
            case 0x22: str = "PageDown"; break;
            case 0x25: str = "Left"; break;
            case 0x26: str = "Up"; break;
            case 0x27: str = "Right"; break;
            case 0x28: str = "Down"; break;
            case 0x2C: str = "PrintScreen"; break;
            case 0x13: str = "Pause"; break;
            case 0x90: str = "NumLock"; break;
            case 0x91: str = "ScrollLock"; break;
            default:
                str = QString("Key_0x%1").arg(vk, 2, 16, QChar('0')).toUpper();
                break;
        }
    }
    parts << str;
    return parts.join(" + ");
}

HotkeySettingsDialog::HotkeySettingsDialog(QWidget* parent)
    : FramelessDialog("全局快捷键设置", parent)
{
    setObjectName("HotkeySettingsDialog");
    setFixedSize(420, 320);
    initUI();
    loadSettings();
}

void HotkeySettingsDialog::initUI() {
    auto* layout = new QVBoxLayout(m_contentArea);
    layout->setContentsMargins(20, 15, 20, 20);
    layout->setSpacing(12);

    auto addRow = [this, layout](const QString& labelText, HotkeyKeyEdit*& edit) {
        auto* row = new QHBoxLayout();
        auto* label = new QLabel(labelText);
        label->setFixedWidth(130);
        edit = new HotkeyKeyEdit();
        row->addWidget(label);
        row->addWidget(edit, 1);
        layout->addLayout(row);
    };

    addRow("屏幕截图快捷键:", m_editScreenshot);
    addRow("文字识别 (OCR):", m_editOCR);
    addRow("屏幕拾色器:", m_editColorPicker);
    addRow("像素标尺:", m_editPixelRuler);

    auto* hintLabel = new QLabel("提示：按 Backspace 或 Delete 可清除按键配置");
    hintLabel->setObjectName("HotkeyHintLabel");
    layout->addWidget(hintLabel);

    layout->addStretch();

    auto* btnLayout = new QHBoxLayout();
    auto* resetBtn = new QPushButton("恢复默认");
    resetBtn->setObjectName("HotkeyResetBtn");
    resetBtn->setFixedSize(96, 32);
    connect(resetBtn, &QPushButton::clicked, this, &HotkeySettingsDialog::onResetDefaultsClicked);

    auto* saveBtn = new QPushButton("保存并生效");
    saveBtn->setObjectName("HotkeySaveBtn");
    saveBtn->setFixedSize(100, 32);
    connect(saveBtn, &QPushButton::clicked, this, &HotkeySettingsDialog::onSaveClicked);

    btnLayout->addWidget(resetBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    layout->addLayout(btnLayout);
}

void HotkeySettingsDialog::loadSettings() {
    QSettings hotkeys("OCR", "Hotkeys");
    m_editScreenshot->setKeyData(
        hotkeys.value("screenshot_mods", 0).toUInt(),
        hotkeys.value("screenshot_vk", 0x70).toUInt()
    );
    m_editOCR->setKeyData(
        hotkeys.value("ocr_mods", 0).toUInt(),
        hotkeys.value("ocr_vk", 0x71).toUInt()
    );
    m_editColorPicker->setKeyData(
        hotkeys.value("picker_mods", 0).toUInt(),
        hotkeys.value("picker_vk", 0).toUInt()
    );
    m_editPixelRuler->setKeyData(
        hotkeys.value("ruler_mods", 0).toUInt(),
        hotkeys.value("ruler_vk", 0).toUInt()
    );
}

void HotkeySettingsDialog::onResetDefaultsClicked() {
    m_editScreenshot->setKeyData(0, 0x70); // F1
    m_editOCR->setKeyData(0, 0x71);        // F2
    m_editColorPicker->setKeyData(0, 0);
    m_editPixelRuler->setKeyData(0, 0);
}

void HotkeySettingsDialog::onSaveClicked() {
    QSettings hotkeys("OCR", "Hotkeys");
    hotkeys.setValue("screenshot_mods", m_editScreenshot->mods());
    hotkeys.setValue("screenshot_vk", m_editScreenshot->vk());

    hotkeys.setValue("ocr_mods", m_editOCR->mods());
    hotkeys.setValue("ocr_vk", m_editOCR->vk());

    hotkeys.setValue("picker_mods", m_editColorPicker->mods());
    hotkeys.setValue("picker_vk", m_editColorPicker->vk());

    hotkeys.setValue("ruler_mods", m_editPixelRuler->mods());
    hotkeys.setValue("ruler_vk", m_editPixelRuler->vk());

    HotkeyManager::instance().reapplyHotkeys();

    ToolTipOverlay::instance()->showText(QCursor::pos(), "<b style='color: #2ecc71;'>[OK] 快捷键设置已保存并立即生效</b>", 700);
    accept();
}
