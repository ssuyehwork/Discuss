#pragma once

#include <QObject>
#include <QKeyEvent>

namespace QuarkMeta {

class MainWindow;

class GlobalShortcutController : public QObject {
    Q_OBJECT
public:
    explicit GlobalShortcutController(MainWindow* mainWindow, QObject* parent = nullptr);
    ~GlobalShortcutController() override = default;

    bool handleKeyPress(QKeyEvent* event);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    MainWindow* m_mainWindow = nullptr;
};

} // namespace QuarkMeta
