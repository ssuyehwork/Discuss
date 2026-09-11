#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>
#include "ContentPanel.h"

namespace QuarkMeta {

class NavPanel;
class FavoritePanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;
class AddressBar;
class SearchController;
class TitleBarWidget;
class PanelLayoutManager;
class AppShortcutController;

/**
 * @brief 面板中介者组件依赖包
 */
struct PanelMediatorComponents {
    NavPanel* navPanel = nullptr;
    FavoritePanel* favoritePanel = nullptr;
    ContentPanel* contentPanel = nullptr;
    MetaPanel* metaPanel = nullptr;
    FilterPanel* filterPanel = nullptr;
    AddressBar* addressBar = nullptr;
    SearchController* searchController = nullptr;
    TitleBarWidget* titleBar = nullptr;
    PanelLayoutManager* layoutManager = nullptr;
    AppShortcutController* shortcutController = nullptr;
};

/**
 * @brief 面板中介者
 * 负责各面板与控制器之间的信号槽连接与协同动作，彻底解耦 MainWindow
 */
class PanelMediator : public QObject {
    Q_OBJECT

public:
    explicit PanelMediator(const PanelMediatorComponents& components, QObject* parent = nullptr);
    ~PanelMediator() override = default;

    /**
     * @brief 建立各面板间的信号槽连接
     */
    void setupConnections();

signals:
    /**
     * @brief 统一向 MainWindow 发送状态栏消息更新请求
     */
    void statusMessageRequested(const QString& message);

private:
    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;
    QPointer<AddressBar> m_addressBar;
    QPointer<SearchController> m_searchController;
    QPointer<TitleBarWidget> m_titleBar;
    QPointer<PanelLayoutManager> m_layoutManager;
    QPointer<AppShortcutController> m_shortcutController;

    QString m_currentQuickLookPath;
    QTimer* m_selectionDebounceTimer = nullptr;
    QStringList m_pendingSelectionPaths;
};

} // namespace QuarkMeta
