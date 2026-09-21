#include "TabBarWidget.h"
#include "UiHelper.h"
#include "StyleLibrary.h"

#include <QStyle>
#include <QDateTime>

namespace QuarkMeta {

TabItemButton::TabItemButton(int index, QWidget* parent)
    : QPushButton(parent), m_index(index) {
    setObjectName("TabItem");
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(28);
    setCursor(Qt::PointingHandCursor);

    QHBoxLayout* itemLayout = new QHBoxLayout(this);
    itemLayout->setContentsMargins(10, 0, 6, 0);
    itemLayout->setSpacing(6);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setObjectName("TabIconLabel");
    m_iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_iconLabel->setFixedSize(14, 14);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("TabTitleLabel");
    m_titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    m_btnClose = new QPushButton(this);
    m_btnClose->setObjectName("TabCloseBtn");
    m_btnClose->setFocusPolicy(Qt::NoFocus);
    m_btnClose->setFixedSize(16, 16);
    m_btnClose->setIcon(UiHelper::getIcon("close", QColor("#888888")));
    m_btnClose->setIconSize(QSize(10, 10));

    connect(m_btnClose, &QPushButton::clicked, this, [this]() {
        emit closeClicked(m_index);
    });

    connect(this, &QPushButton::clicked, this, [this]() {
        emit tabClicked(m_index);
    });

    itemLayout->addWidget(m_iconLabel, 0, Qt::AlignVCenter);
    itemLayout->addWidget(m_titleLabel, 0, Qt::AlignVCenter);
    itemLayout->addWidget(m_btnClose, 0, Qt::AlignVCenter);
}

void TabItemButton::setTabTitle(const QString& title) {
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void TabItemButton::setTabIcon(const QIcon& icon) {
    if (m_iconLabel) {
        m_iconLabel->setPixmap(icon.pixmap(14, 14));
    }
}

void TabItemButton::setActive(bool active) {
    setProperty("active", active);
    if (m_titleLabel) {
        m_titleLabel->setProperty("active", active);
        m_titleLabel->style()->unpolish(m_titleLabel);
        m_titleLabel->style()->polish(m_titleLabel);
    }
    style()->unpolish(this);
    style()->polish(this);
}

TabBarWidget::TabBarWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("TabBarWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(30);

    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 2, 0, 0);
    m_mainLayout->setSpacing(4);

    m_tabsLayout = new QHBoxLayout();
    m_tabsLayout->setContentsMargins(0, 0, 0, 0);
    m_tabsLayout->setSpacing(2);

    m_btnNewTab = new QPushButton(this);
    m_btnNewTab->setFocusPolicy(Qt::NoFocus);
    m_btnNewTab->setFixedSize(22, 22);
    m_btnNewTab->setIcon(UiHelper::getIcon("add", QColor("#EEEEEE")));
    m_btnNewTab->setIconSize(QSize(14, 14));
    m_btnNewTab->setObjectName("NewTabBtn");
    m_btnNewTab->setProperty("tooltipText", "新建标签页 (Ctrl+T)");

    connect(m_btnNewTab, &QPushButton::clicked, this, [this]() {
        addTab("此电脑", "computer://", true);
        emit newTabRequested();
    });

    m_mainLayout->addLayout(m_tabsLayout);
    m_mainLayout->addWidget(m_btnNewTab, 0, Qt::AlignVCenter);
    m_mainLayout->addStretch();

    // 默认添加首个“此电脑”标签页
    addTab("此电脑", "computer://", true);
}

void TabBarWidget::addTab(const QString& title, const QString& url, bool switchToNew) {
    TabInfo info;
    info.id = QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + QString::number(m_tabs.size());
    info.title = title.isEmpty() ? "此电脑" : title;
    info.url = url.isEmpty() ? "computer://" : url;
    info.active = false;

    m_tabs.append(info);
    if (switchToNew || m_currentIndex == -1) {
        setCurrentIndex(m_tabs.size() - 1, true);
    } else {
        rebuildTabsUi();
    }
}

void TabBarWidget::closeTab(int index) {
    if (index < 0 || index >= m_tabs.size()) return;
    if (m_tabs.size() <= 1) return; // 保持至少一个标签页

    m_closedTabsHistory.append(m_tabs[index]);
    m_tabs.removeAt(index);
    if (index < m_currentIndex) {
        m_currentIndex--;
    } else if (m_currentIndex >= m_tabs.size()) {
        m_currentIndex = m_tabs.size() - 1;
    }
    setCurrentIndex(m_currentIndex, true);
    emit tabClosed(index);
}

void TabBarWidget::restoreLastClosedTab() {
    if (m_closedTabsHistory.isEmpty()) return;
    TabInfo lastTab = m_closedTabsHistory.takeLast();
    addTab(lastTab.title, lastTab.url, true);
}

void TabBarWidget::setCurrentIndex(int index, bool forceNotify) {
    if (index < 0 || index >= m_tabs.size()) return;
    bool indexChanged = (m_currentIndex != index);
    m_currentIndex = index;
    for (int i = 0; i < m_tabs.size(); ++i) {
        m_tabs[i].active = (i == m_currentIndex);
    }
    updateTabsUiState();
    if (indexChanged || forceNotify) {
        emit currentTabChanged(m_currentIndex, m_tabs[m_currentIndex].url);
    }
}

void TabBarWidget::updateCurrentTabTitle(const QString& title, const QString& url) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;
    if (m_tabs[m_currentIndex].title == title && m_tabs[m_currentIndex].url == url) return;

    m_tabs[m_currentIndex].title = title.isEmpty() ? "此电脑" : title;
    m_tabs[m_currentIndex].url = url;

    if (m_currentIndex < m_tabWidgets.size()) {
        auto tabBtn = m_tabWidgets[m_currentIndex];
        tabBtn->setTabTitle(m_tabs[m_currentIndex].title);
        tabBtn->setTabIcon(UiHelper::getIcon(url.startsWith("computer://") ? "computer" : "folder_filled", QColor("#EEEEEE")));
    }
}

void TabBarWidget::updateTabsUiState() {
    if (m_tabWidgets.size() != m_tabs.size()) {
        rebuildTabsUi();
        return;
    }

    for (int i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs[i];
        auto tabBtn = m_tabWidgets[i];
        tabBtn->setIndex(i);
        tabBtn->setTabTitle(tab.title);
        tabBtn->setTabIcon(UiHelper::getIcon(tab.url.startsWith("computer://") ? "computer" : "folder_filled",
                                                tab.active ? QColor("#EEEEEE") : QColor("#888888")));
        tabBtn->setActive(tab.active);
    }
}

void TabBarWidget::rebuildTabsUi() {
    m_tabWidgets.clear();
    QLayoutItem* child;
    while ((child = m_tabsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    for (int i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs[i];
        TabItemButton* tabItem = new TabItemButton(i, this);
        tabItem->setTabTitle(tab.title);
        tabItem->setTabIcon(UiHelper::getIcon(tab.url.startsWith("computer://") ? "computer" : "folder_filled",
                                                tab.active ? QColor("#EEEEEE") : QColor("#888888")));
        tabItem->setActive(tab.active);

        connect(tabItem, &TabItemButton::closeClicked, this, [this](int idx) {
            closeTab(idx);
        });

        connect(tabItem, &TabItemButton::tabClicked, this, [this](int idx) {
            setCurrentIndex(idx, true);
        });

        m_tabWidgets.append(tabItem);
        m_tabsLayout->addWidget(tabItem);
    }
}

} // namespace QuarkMeta
