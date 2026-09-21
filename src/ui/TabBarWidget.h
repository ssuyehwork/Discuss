#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
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

class TabBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit TabBarWidget(QWidget* parent = nullptr);
    ~TabBarWidget() override = default;

    void addTab(const QString& title = "此电脑", const QString& url = "computer://", bool switchToNew = true);
    void closeTab(int index);
    void restoreLastClosedTab();
    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }
    void updateCurrentTabTitle(const QString& title, const QString& url);

signals:
    void currentTabChanged(int index, const QString& url);
    void tabClosed(int index);
    void newTabRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void rebuildTabsUi();

    QHBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_tabsLayout = nullptr;
    QPushButton* m_btnNewTab = nullptr;

    QList<TabInfo> m_tabs;
    QList<TabInfo> m_closedTabsHistory;
    QList<QWidget*> m_tabWidgets;
    int m_currentIndex = -1;
};

} // namespace QuarkMeta
