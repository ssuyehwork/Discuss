#pragma once

#include <QObject>
#include <QPointer>
#include "ContentPanel.h"

namespace QuarkMeta {

class NavPanel;
class FavoritePanel;
class LibraryPanel;
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
    LibraryPanel* libraryPanel = nullptr;
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

    /**
     * @brief 获取当前焦点 ContentPanel (若为空则降级返回主窗格)
     */
    ContentPanel* activeContentPanel() const;

signals:
    /**
     * @brief 统一向 MainWindow 发送状态栏消息更新请求
     */
    void statusMessageRequested(const QString& message);

    /**
     * @brief 当前激活的内容面板改变广播信号
     */
    void activeContentPanelChanged(ContentPanel* panel);

    /**
     * @brief 焦点窗格视图模式/排序状态改变信号
     */
    void activePaneStateChanged();

private:
    class PaneActivationTracker* m_activationTracker = nullptr;
    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<LibraryPanel> m_libraryPanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<ContentPanel> m_activeContentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;
    QPointer<AddressBar> m_addressBar;
    QPointer<SearchController> m_searchController;
    QPointer<TitleBarWidget> m_titleBar;
    QPointer<PanelLayoutManager> m_layoutManager;
    QPointer<AppShortcutController> m_shortcutController;

    QString m_currentQuickLookPath;
};

} // namespace QuarkMeta
