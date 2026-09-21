#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include <QString>
#include <QEvent>

namespace QuarkMeta {

struct TabInfo {
    QString id;
    QString title;
    QString url;
    bool active = false;
};

class TabItemButton : public QPushButton {
    Q_OBJECT
public:
    explicit TabItemButton(int index, QWidget* parent = nullptr);
    int index() const { return m_index; }
    void setIndex(int index) { m_index = index; }

    void setTabTitle(const QString& title);
    void setTabIcon(const QIcon& icon);
    void setActive(bool active);

signals:
    void tabClicked(int index);
    void closeClicked(int index);

private:
    int m_index = -1;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QPushButton* m_btnClose = nullptr;
};

class TabBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit TabBarWidget(QWidget* parent = nullptr);
    ~TabBarWidget() override = default;

    void addTab(const QString& title = "此电脑", const QString& url = "computer://", bool switchToNew = true);
    void closeTab(int index);
    void restoreLastClosedTab();
    void setCurrentIndex(int index, bool forceNotify = false);
    int currentIndex() const { return m_currentIndex; }
    void updateCurrentTabTitle(const QString& title, const QString& url);

signals:
    void currentTabChanged(int index, const QString& url);
    void tabClosed(int index);
    void newTabRequested();

private:
    void updateTabsUiState();
    void rebuildTabsUi();

    QHBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_tabsLayout = nullptr;
    QPushButton* m_btnNewTab = nullptr;

    QList<TabInfo> m_tabs;
    QList<TabInfo> m_closedTabsHistory;
    QList<TabItemButton*> m_tabWidgets;
    int m_currentIndex = -1;
    bool m_isUpdatingFromNav = false;
};

} // namespace QuarkMeta
