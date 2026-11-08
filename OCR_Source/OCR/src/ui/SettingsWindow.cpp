#include "SettingsWindow.h"
#include "ToolTipOverlay.h"
#include "../core/HotkeyManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QStackedWidget>
#include <QScrollArea>
#include <QFileDialog>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <QCheckBox>

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

SettingsWindow::SettingsWindow(QWidget* parent)
    : FramelessDialog("系统设置", parent)
{
    setObjectName("SettingsWindow");
    setFixedSize(620, 440);
    initUI();
    loadSettings();
}

void SettingsWindow::initUI() {
    auto* mainLayout = new QHBoxLayout(m_contentArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 左侧导航栏 (严密对应参考图样式)
    m_navBar = new QListWidget();
    m_navBar->setObjectName("SettingsNavBar");
    m_navBar->setFixedWidth(150);
    m_navBar->addItem("全局热键");
    m_navBar->addItem("截图设置");
    m_navBar->addItem("通用设置");

    connect(m_navBar, &QListWidget::currentRowChanged, this, &SettingsWindow::onCategoryChanged);

    // 右侧内容区
    m_contentStack = new QStackedWidget();
    m_contentStack->setObjectName("SettingsContentStack");
    m_contentStack->addWidget(createHotkeyPage());
    m_contentStack->addWidget(createScreenshotPage());
    m_contentStack->addWidget(createGeneralPage());

    auto* scrollArea = new QScrollArea();
    scrollArea->setObjectName("SettingsScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidget(m_contentStack);

    auto* rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(20, 18, 20, 20);
    rightLayout->setSpacing(20);
    rightLayout->addWidget(scrollArea, 1);

    // 底部操作按钮
    auto* btnLayout = new QHBoxLayout();
    auto* resetBtn = new QPushButton("恢复默认设置");
    resetBtn->setObjectName("SettingsResetBtn");
    resetBtn->setFixedSize(110, 34);
    connect(resetBtn, &QPushButton::clicked, this, &SettingsWindow::onRestoreDefaults);

    auto* saveBtn = new QPushButton("保存并生效");
    saveBtn->setObjectName("SettingsSaveBtn");
    saveBtn->setFixedSize(110, 34);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsWindow::onSaveClicked);

    btnLayout->addWidget(resetBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    rightLayout->addLayout(btnLayout);

    mainLayout->addWidget(m_navBar);
    mainLayout->addLayout(rightLayout);

    m_navBar->setCurrentRow(0);
}

QWidget* SettingsWindow::createHotkeyPage() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto addRow = [layout](const QString& labelText, HotkeyKeyEdit*& edit) {
        auto* row = new QHBoxLayout();
        auto* label = new QLabel(labelText);
        label->setObjectName("SettingsItemLabel");
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
    hintLabel->setObjectName("SettingsHintLabel");
    layout->addWidget(hintLabel);

    layout->addStretch();
    return page;
}

QWidget* SettingsWindow::createScreenshotPage() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto* titleLabel = new QLabel("截图自动保存路径：");
    titleLabel->setObjectName("SettingsItemLabel");
    layout->addWidget(titleLabel);

    auto* pathLayout = new QHBoxLayout();
    m_editSavePath = new QLineEdit();
    m_editSavePath->setObjectName("SettingsPathEdit");
    m_editSavePath->setFixedHeight(32);
    m_editSavePath->setPlaceholderText("选择保存路径...");

    auto* browseBtn = new QPushButton("浏览...");
    browseBtn->setObjectName("SettingsBrowseBtn");
    browseBtn->setFixedSize(80, 32);
    connect(browseBtn, &QPushButton::clicked, this, &SettingsWindow::onBrowseSavePath);

    pathLayout->addWidget(m_editSavePath, 1);
    pathLayout->addWidget(browseBtn);
    layout->addLayout(pathLayout);

    auto* defaultTip = new QLabel("提示：若未设置或路径无效，默认保存至程序目录下的 /RPN_screenshot");
    defaultTip->setObjectName("SettingsHintLabel");
    layout->addWidget(defaultTip);

    layout->addSpacing(10);
    auto* ocrTitle = new QLabel("截图取文 (OCR) 交互设置：");
    ocrTitle->setObjectName("SettingsItemLabel");
    layout->addWidget(ocrTitle);

    m_checkOcrAutoCopy = new QCheckBox("OCR 识别后自动复制识别文本并隐藏结果窗口");
    m_checkOcrAutoCopy->setObjectName("SettingsCheckBox");
    layout->addWidget(m_checkOcrAutoCopy);

    m_checkSilentCapture = new QCheckBox("开启静默采集模式 (识别完成仅显示气泡提示，不弹出大窗口)");
    m_checkSilentCapture->setObjectName("SettingsCheckBox");
    layout->addWidget(m_checkSilentCapture);

    layout->addStretch();
    return page;
}

QWidget* SettingsWindow::createGeneralPage() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    m_checkAutoStart = new QCheckBox("开机自动启动");
    m_checkAutoStart->setObjectName("SettingsCheckBox");
    layout->addWidget(m_checkAutoStart);

    m_checkConflictWarning = new QCheckBox("快捷键注册失败时在托盘弹出气泡告警");
    m_checkConflictWarning->setObjectName("SettingsCheckBox");
    layout->addWidget(m_checkConflictWarning);

    auto* tip = new QLabel("提示：软件配置实时保存在本地注册表/配置文件中。");
    tip->setObjectName("SettingsHintLabel");
    layout->addWidget(tip);

    layout->addStretch();
    return page;
}

void SettingsWindow::onCategoryChanged(int index) {
    if (m_contentStack && index >= 0 && index < m_contentStack->count()) {
        m_contentStack->setCurrentIndex(index);
    }
}

void SettingsWindow::loadSettings() {
    // 载入热键配置
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

    // 载入截图保存路径
    QSettings ssSettings("OCR", "Screenshot");
    QString defaultPath = QCoreApplication::applicationDirPath() + "/RPN_screenshot";
    QString currentPath = ssSettings.value("savePath", defaultPath).toString();
    m_editSavePath->setText(currentPath);

    // 载入 OCR 选项
    QSettings ocrSettings("OCR", "OCR");
    m_checkOcrAutoCopy->setChecked(ocrSettings.value("autoCopy", false).toBool());
    m_checkSilentCapture->setChecked(ocrSettings.value("silentCapture", false).toBool());

    // 载入通用设置
    QSettings generalSettings("OCR", "General");
    m_checkAutoStart->setChecked(generalSettings.value("autoStart", false).toBool());
    m_checkConflictWarning->setChecked(generalSettings.value("conflictWarning", true).toBool());
}

void SettingsWindow::onBrowseSavePath() {
    QString current = m_editSavePath->text().trimmed();
    if (current.isEmpty() || !QDir(current).exists()) {
        current = QCoreApplication::applicationDirPath();
    }
    QString dir = QFileDialog::getExistingDirectory(this, "选择截图保存目录", current);
    if (!dir.isEmpty()) {
        m_editSavePath->setText(QDir::toNativeSeparators(dir));
    }
}

void SettingsWindow::onRestoreDefaults() {
    m_editScreenshot->setKeyData(0, 0x70); // F1
    m_editOCR->setKeyData(0, 0x71);        // F2
    m_editColorPicker->setKeyData(0, 0);
    m_editPixelRuler->setKeyData(0, 0);

    QString defaultPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/RPN_screenshot");
    m_editSavePath->setText(defaultPath);

    m_checkOcrAutoCopy->setChecked(false);
    m_checkSilentCapture->setChecked(false);
    m_checkAutoStart->setChecked(false);
    m_checkConflictWarning->setChecked(true);
}

void SettingsWindow::onSaveClicked() {
    // 保存热键
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

    // 保存截图目录
    QString savePath = m_editSavePath->text().trimmed();
    if (!savePath.isEmpty()) {
        QDir().mkpath(savePath);
        QSettings ssSettings("OCR", "Screenshot");
        ssSettings.setValue("savePath", savePath);
    }

    // 保存 OCR 交互选项
    QSettings ocrSettings("OCR", "OCR");
    ocrSettings.setValue("autoCopy", m_checkOcrAutoCopy->isChecked());
    ocrSettings.setValue("silentCapture", m_checkSilentCapture->isChecked());

    // 保存通用设置
    QSettings generalSettings("OCR", "General");
    generalSettings.setValue("autoStart", m_checkAutoStart->isChecked());
    generalSettings.setValue("conflictWarning", m_checkConflictWarning->isChecked());

#ifdef Q_OS_WIN
    // 同步写入 Windows 开机启动注册表项
    QSettings bootSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (m_checkAutoStart->isChecked()) {
        bootSettings.setValue("SSUYEH_OCR", QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    } else {
        bootSettings.remove("SSUYEH_OCR");
    }
#endif

    ToolTipOverlay::instance()->showText(QCursor::pos(), "<b style='color: #2ecc71;'>[OK] 系统设置已保存并立即生效</b>", 800);
    accept();
}
