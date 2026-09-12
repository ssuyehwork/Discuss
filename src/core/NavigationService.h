#pragma once

#include <QObject>
#include <QString>
#include <QList>

namespace QuarkMeta {

struct NavTabSession {
    QString id;
    QString currentUrl;
    QString title;
    QList<QString> history;
    int historyIndex = -1;
};

class NavigationService : public QObject {
    Q_OBJECT

public:
    static NavigationService& instance();

    // 多会话 Tab 管理接口
    int createTab(const QString& url = QString());
    void closeTab(int index);
    void switchTab(int index);
    const QList<NavTabSession>& tabs() const { return m_tabs; }
    int activeTabIndex() const { return m_activeTabIndex; }

    // 核心导航调度接口
    void navigateTo(const QString& rawUrl, bool recordHistory = true);
    void goBack();
    void goForward();
    void goUp();
    void refresh();

    // 状态查询接口
    QString currentUrl() const;
    QString currentDisplayPath() const;
    bool isVirtualProtocol() const;
    bool canGoBack() const;
    bool canGoForward() const;
    bool canGoUp() const;

    static QString displayPathForUrl(const QString& url);
    static QString titleForUrl(const QString& url);

signals:
    /**
     * @brief 全局统一路径变更信号 (驱动各子面板单向加载数据)
     * @param url 标准协议 URL (如 file://C:/Users 或 computer://)
     * @param displayPath 适合 UI 面包屑展示的文本 (如 C:\Users 或 此电脑)
     */
    void currentUrlChanged(const QString& url, const QString& displayPath);

    /**
     * @brief 导航可用性状态变动信号 (驱动前进/后退/上级按钮状态)
     */
    void navStateChanged(bool canBack, bool canForward, bool canUp);

    /**
     * @brief Tab 列表与激活状态更新信号
     */
    void tabsUpdated();

private:
    explicit NavigationService(QObject* parent = nullptr);
    ~NavigationService() override = default;
    NavigationService(const NavigationService&) = delete;
    NavigationService& operator=(const NavigationService&) = delete;

    QString normalizeUrl(const QString& rawUrl) const;
    void emitNavState();

    QList<NavTabSession> m_tabs;
    int m_activeTabIndex = -1;
    static constexpr int kMaxHistoryDepth = 100;
};

} // namespace QuarkMeta
