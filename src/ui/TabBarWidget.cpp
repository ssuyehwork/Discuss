#include "TabBarWidget.h"
#include "UiHelper.h"
#include "StyleLibrary.h"

#include <QLabel>
#include <QStyle>
#include <QDateTime>
#include <QMouseEvent>

namespace QuarkMeta {

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
    m_btnNewTab->setStyleSheet(
        "QPushButton#NewTabBtn { background: transparent; border: none; border-radius: 4px; }"
        "QPushButton#NewTabBtn:hover { background-color: #3E3E42; }"
    );

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
        setCurrentIndex(m_tabs.size() - 1);
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
    setCurrentIndex(m_currentIndex);
    emit tabClosed(index);
}

void TabBarWidget::restoreLastClosedTab() {
    if (m_closedTabsHistory.isEmpty()) return;
    TabInfo lastTab = m_closedTabsHistory.takeLast();
    addTab(lastTab.title, lastTab.url, true);
}

void TabBarWidget::setCurrentIndex(int index) {
    if (index < 0 || index >= m_tabs.size()) return;
    m_currentIndex = index;
    for (int i = 0; i < m_tabs.size(); ++i) {
        m_tabs[i].active = (i == m_currentIndex);
    }
    rebuildTabsUi();
    emit currentTabChanged(m_currentIndex, m_tabs[m_currentIndex].url);
}

void TabBarWidget::updateCurrentTabTitle(const QString& title, const QString& url) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;
    m_tabs[m_currentIndex].title = title.isEmpty() ? "此电脑" : title;
    m_tabs[m_currentIndex].url = url;
    rebuildTabsUi();
}

bool TabBarWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            int clickedIdx = m_tabWidgets.indexOf(qobject_cast<QWidget*>(watched));
            if (clickedIdx != -1 && clickedIdx != m_currentIndex) {
                setCurrentIndex(clickedIdx);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
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
        QWidget* tabItem = new QWidget(this);
        tabItem->setFixedHeight(28);
        tabItem->setCursor(Qt::PointingHandCursor);

        QHBoxLayout* itemLayout = new QHBoxLayout(tabItem);
        itemLayout->setContentsMargins(10, 0, 6, 0);
        itemLayout->setSpacing(6);

        QLabel* iconLabel = new QLabel(tabItem);
        iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        iconLabel->setFixedSize(14, 14);
        iconLabel->setPixmap(UiHelper::getIcon(tab.url.startsWith("computer://") ? "computer" : "folder_filled",
                                                tab.active ? QColor("#EEEEEE") : QColor("#888888")).pixmap(14, 14));

        QLabel* titleLabel = new QLabel(tab.title, tabItem);
        titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        titleLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(tab.active ? "#FFFFFF" : "#AAAAAA"));

        QPushButton* btnClose = new QPushButton(tabItem);
        btnClose->setFocusPolicy(Qt::NoFocus);
        btnClose->setFixedSize(16, 16);
        btnClose->setIcon(UiHelper::getIcon("close", QColor("#888888")));
        btnClose->setIconSize(QSize(10, 10));
        btnClose->setStyleSheet(
            "QPushButton { background: transparent; border: none; border-radius: 8px; }"
            "QPushButton:hover { background-color: #555555; }"
        );

        connect(btnClose, &QPushButton::clicked, this, [this, i]() {
            closeTab(i);
        });

        itemLayout->addWidget(iconLabel, 0, Qt::AlignVCenter);
        itemLayout->addWidget(titleLabel, 0, Qt::AlignVCenter);
        itemLayout->addWidget(btnClose, 0, Qt::AlignVCenter);

        if (tab.active) {
            tabItem->setStyleSheet(
                "QWidget { background-color: #2D2D2D; border-top-left-radius: 6px; border-top-right-radius: 6px; border: 1px solid #3E3E42; border-bottom: none; }"
            );
        } else {
            tabItem->setStyleSheet(
                "QWidget { background-color: #1E1E1E; border-top-left-radius: 6px; border-top-right-radius: 6px; border: 1px solid transparent; }"
                "QWidget:hover { background-color: #252526; }"
            );
        }

        tabItem->installEventFilter(this);
        m_tabWidgets.append(tabItem);
        m_tabsLayout->addWidget(tabItem);
    }
}

} // namespace QuarkMeta
