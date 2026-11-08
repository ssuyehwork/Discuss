#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QFile>
#include <QPointer>
#include <QClipboard>
#include <QCursor>
#include <QSettings>

#include "ui/ScreenshotTool.h"
#include "ui/OCRWindow.h"
#include "ui/OCRResultWindow.h"
#include "ui/ColorPickerWindow.h"
#include "ui/PixelRulerOverlay.h"
#include "ui/ToolTipOverlay.h"
#include "ui/SettingsWindow.h"
#include "core/OCRManager.h"
#include "core/ClipboardMonitor.h"
#include "core/HotkeyManager.h"
#include <QFileDialog>

int main(int argc, char* argv[]) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setOrganizationName("SSUYEH");
    app.setApplicationName("OCR");

    // 加载全局 QSS 样式表
    QFile qssFile(":/style.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(qssFile.readAll()));
        qssFile.close();
    }

    // 初始化系统托盘图标
    QIcon appIcon(":/icons/app_logo.svg");
    if (appIcon.isNull()) {
        appIcon = QIcon(":/app_icon.png");
    }
    app.setWindowIcon(appIcon);

    QSystemTrayIcon trayIcon(appIcon, &app);
    trayIcon.setToolTip("OCR - 屏幕视觉工具集");

    // 托盘右键功能菜单
    QMenu trayMenu;

        auto* actScreenshot = trayMenu.addAction("屏幕截图");
    auto* actOCR = trayMenu.addAction("文字识别 (OCR)");
    auto* actColorPicker = trayMenu.addAction("屏幕拾色器");
    auto* actPixelRuler = trayMenu.addAction("像素标尺");
    trayMenu.addSeparator();
    auto* actSettings = trayMenu.addAction("配置");
    trayMenu.addSeparator();
    auto* actQuit = trayMenu.addAction("退出");

    // 屏幕截图流程调度
    static bool s_isCaptureActive = false;
    auto startCapture = [](bool immediateOcr = false) {
        if (s_isCaptureActive) return;
        s_isCaptureActive = true;

        auto* tool = new ScreenshotTool(nullptr);
        tool->setAttribute(Qt::WA_DeleteOnClose);
        if (immediateOcr) {
            tool->setImmediateOCRMode(true);
        }

        QObject::connect(tool, &ScreenshotTool::destroyed, []() {
            s_isCaptureActive = false;
        });

        QObject::connect(tool, &ScreenshotTool::screenshotCaptured, [](const QImage& img, bool isOcrRequest) {
            if (!isOcrRequest) {
                ClipboardMonitor::instance().skipNext();
                QApplication::clipboard()->setImage(img);
                ToolTipOverlay::instance()->showText(QCursor::pos(), "<b style='color: #2ecc71;'>[OK] 截图已复制至剪贴板</b>");
            } else {
                QSettings settings("OCR", "OCR");
                bool autoCopy = settings.value("autoCopy", false).toBool();
                bool silent = settings.value("silentCapture", false).toBool();

                auto* resWin = new OCRResultWindow(img, 0);
                QObject::connect(&OCRManager::instance(), &OCRManager::recognitionFinished,
                                 resWin, &OCRResultWindow::setRecognizedText);

                if (autoCopy || silent) {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "<b style='color: #3498db;'>[OCR] 正在识别文字...</b>");
                } else {
                    resWin->show();
                    resWin->raise();
                    resWin->activateWindow();
                }
                OCRManager::instance().recognizeAsync(img, 0);
            }
        });

        tool->show();
    };

    QObject::connect(actScreenshot, &QAction::triggered, [startCapture]() {
        startCapture(false);
    });

    // 文字识别主窗口
    static QPointer<OCRWindow> s_ocrWindow;
    QObject::connect(actOCR, &QAction::triggered, []() {
        if (!s_ocrWindow) {
            s_ocrWindow = new OCRWindow(nullptr);
        }
        s_ocrWindow->showNormal();
        s_ocrWindow->raise();
        s_ocrWindow->activateWindow();
    });

    // 屏幕拾色器
    static QPointer<ColorPickerWindow> s_colorPicker;
    QObject::connect(actColorPicker, &QAction::triggered, []() {
        if (!s_colorPicker) {
            s_colorPicker = new ColorPickerWindow(nullptr);
        }
        s_colorPicker->showNormal();
        s_colorPicker->raise();
        s_colorPicker->activateWindow();
    });

    // 像素标尺
    static QPointer<PixelRulerOverlay> s_pixelRuler;
    QObject::connect(actPixelRuler, &QAction::triggered, []() {
        if (!s_pixelRuler) {
            s_pixelRuler = new PixelRulerOverlay(nullptr);
            s_pixelRuler->setAttribute(Qt::WA_DeleteOnClose);
        }
        s_pixelRuler->show();
        s_pixelRuler->raise();
        s_pixelRuler->activateWindow();
    });

    // 配置窗口（整合快捷键与截图设置）
    static QPointer<SettingsWindow> s_settingsWindow;
    QObject::connect(actSettings, &QAction::triggered, []() {
        if (!s_settingsWindow) {
            s_settingsWindow = new SettingsWindow(nullptr);
            s_settingsWindow->setAttribute(Qt::WA_DeleteOnClose);
        }
        s_settingsWindow->show();
        s_settingsWindow->raise();
        s_settingsWindow->activateWindow();
    });

    // 托盘单击事件响应：直接激活屏幕截图
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, [startCapture](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            startCapture(false);
        }
    });

    // 全局热键管理器初始化与分发
    auto& hk = HotkeyManager::instance();
    hk.reapplyHotkeys();
    QObject::connect(&hk, &HotkeyManager::hotkeyPressed, [startCapture](int id) {
        if (id == HotkeyManager::Screenshot) {
            startCapture(false);
        } else if (id == HotkeyManager::OCR) {
            if (!s_ocrWindow) {
                s_ocrWindow = new OCRWindow(nullptr);
            }
            s_ocrWindow->showNormal();
            s_ocrWindow->raise();
            s_ocrWindow->activateWindow();
        } else if (id == HotkeyManager::ColorPicker) {
            if (!s_colorPicker) {
                s_colorPicker = new ColorPickerWindow(nullptr);
            }
            s_colorPicker->showNormal();
            s_colorPicker->raise();
            s_colorPicker->activateWindow();
        } else if (id == HotkeyManager::PixelRuler) {
            if (!s_pixelRuler) {
                s_pixelRuler = new PixelRulerOverlay(nullptr);
                s_pixelRuler->setAttribute(Qt::WA_DeleteOnClose);
            }
            s_pixelRuler->show();
            s_pixelRuler->raise();
            s_pixelRuler->activateWindow();
        }
    });

    QObject::connect(actQuit, &QAction::triggered, &app, &QApplication::quit);

    trayIcon.setContextMenu(&trayMenu);
    trayIcon.show();

    return app.exec();
}
