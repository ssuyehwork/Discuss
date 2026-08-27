#pragma once

#include <QObject>
#include <QPointer>

namespace QuarkMeta {

class NavPanel;
class FavoritePanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;
class AddressBar;
class SearchController;

/**
 * @brief 面板中介者
 * 负责各面板之间的信号槽连接与协同动作，彻底解耦 MainWindow
 */
class PanelMediator : public QObject {
    Q_OBJECT

public:
    explicit PanelMediator(NavPanel* navPanel,
                           FavoritePanel* favoritePanel,
                           ContentPanel* contentPanel,
                           MetaPanel* metaPanel,
                           FilterPanel* filterPanel,
                           AddressBar* addressBar,
                           SearchController* searchController = nullptr,
                           QObject* parent = nullptr);
    ~PanelMediator() override = default;

    /**
     * @brief 建立各面板间的信号槽连接
     */
    void setupConnections();

private:
    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;
    QPointer<AddressBar> m_addressBar;
    QPointer<SearchController> m_searchController;

    QString m_currentQuickLookPath;
};

} // namespace QuarkMeta
