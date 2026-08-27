#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace QuarkMeta {

class MainWindow;
class NavPanel;
class FavoritePanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;
class AddressBar;

class PanelMediator : public QObject {
    Q_OBJECT
public:
    explicit PanelMediator(MainWindow* mainWindow, QObject* parent = nullptr);
    ~PanelMediator() override = default;

    void setupConnections();

private:
    MainWindow* m_mainWindow = nullptr;
};

} // namespace QuarkMeta
