#include "NavTabBar.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include "../core/NavigationService.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileInfo>
#include <QDir>
#include <QMouseEvent>

namespace QuarkMeta {

bool NavTabBar::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QVariant idxVar = watched->property("tabIndex");
            if (idxVar.isValid()) {
                int index = idxVar.toInt();
                NavigationService::instance().switchTab(index);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

NavTabBar::NavTabBar(QWidget* parent) : QWidget(parent) {
    setObjectName("NavTabBar");
    setFixedHeight(34);
    setAttribute(Qt::WA_StyledBackground, true);

    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(4);

    m_tabsLayout = new QHBoxLayout();
    m_tabsLayout->setContentsMargins(0, 0, 0, 0);
    m_tabsLayout->setSpacing(3);

    m_btnAddTab = new QPushButton(this);
    m_btnAddTab->setFocusPolicy(Qt::NoFocus);
    m_btnAddTab->setAttribute(Qt::WA_Hover);
    m_btnAddTab->setFixedSize(22, 22);
    m_btnAddTab->setIcon(UiHelper::getIcon("add", QColor("#CCCCCC"), 14));
    m_btnAddTab->setIconSize(QSize(14, 14));
    m_btnAddTab->setObjectName("AddTabBtn");
    m_btnAddTab->setToolTip("新建标签页");
    m_btnAddTab->setStyleSheet(
        "QPushButton#AddTabBtn { border: none; background: transparent; border-radius: 4px; }"
        "QPushButton#AddTabBtn:hover { background-color: #383838; }"
    );

    connect(m_btnAddTab, &QPushButton::clicked, this, []() {
        NavigationService::instance().createTab();
    });

    m_mainLayout->addLayout(m_tabsLayout);
    m_mainLayout->addWidget(m_btnAddTab, 0, Qt::AlignVCenter);

    connect(&NavigationService::instance(), &NavigationService::tabsUpdated, this, &NavTabBar::rebuildTabs);

    rebuildTabs();
}

void NavTabBar::rebuildTabs() {
    const auto& tabs = NavigationService::instance().tabs();
    int activeIdx = NavigationService::instance().activeTabIndex();

    // 🚀【智能增量对比】：若 Tab 数量未改变，则原地平滑更新现有控件，杜绝闪烁
    if (m_tabsLayout->count() == tabs.size()) {
        for (int i = 0; i < tabs.size(); ++i) {
            QLayoutItem* item = m_tabsLayout->itemAt(i);
            if (item && item->widget()) {
                QWidget* tabW = item->widget();
                bool isActive = (i == activeIdx);
                tabW->setObjectName(isActive ? "NavTabActive" : "NavTabInactive");
                tabW->setProperty("tabIndex", i);

                QLabel* titleLabel = tabW->findChild<QLabel*>("TabTitleLabel");
                if (titleLabel) {
                    titleLabel->setText(tabs[i].title);
                    titleLabel->setStyleSheet(isActive ? "color: #FFFFFF; font-weight: bold; font-size: 12px;" : "color: #AAAAAA; font-size: 12px;");
                }

                if (isActive) {
                    tabW->setStyleSheet("QWidget#NavTabActive { background-color: #2D2D2D; border-top: 2px solid #FF551C; border-top-left-radius: 4px; border-top-right-radius: 4px; }");
                } else {
                    tabW->setStyleSheet("QWidget#NavTabInactive { background-color: transparent; border-top: 2px solid transparent; border-top-left-radius: 4px; border-top-right-radius: 4px; } QWidget#NavTabInactive:hover { background-color: #252526; }");
                }
            }
        }
        return;
    }

    // 数量变动时执行重排
    QLayoutItem* item;
    while ((item = m_tabsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    for (int i = 0; i < tabs.size(); ++i) {
        bool isActive = (i == activeIdx);
        QWidget* tabW = createTabWidget(i, tabs[i].title, tabs[i].currentUrl, isActive);
        m_tabsLayout->addWidget(tabW);
    }
}

QWidget* NavTabBar::createTabWidget(int index, const QString& title, const QString& url, bool isActive) {
    QWidget* tab = new QWidget(this);
    tab->setObjectName(isActive ? "NavTabActive" : "NavTabInactive");
    tab->setFixedHeight(28);
    tab->setCursor(Qt::PointingHandCursor);

    QHBoxLayout* layout = new QHBoxLayout(tab);
    layout->setContentsMargins(8, 0, 6, 0);
    layout->setSpacing(6);

    QLabel* iconLabel = new QLabel(tab);
    iconLabel->setFixedSize(16, 16);
    if (url == "computer://") {
        iconLabel->setPixmap(UiHelper::getIcon("computer", Style::BrandOrange).pixmap(14, 14));
    } else if (url == "trash://") {
        iconLabel->setPixmap(UiHelper::getIcon("delete_forever", QColor("#E81123")).pixmap(14, 14));
    } else {
        iconLabel->setPixmap(UiHelper::getIcon("folder_filled", Style::BrandOrange).pixmap(14, 14));
    }

    QLabel* titleLabel = new QLabel(title, tab);
    titleLabel->setObjectName("TabTitleLabel");
    titleLabel->setMaximumWidth(130);
    titleLabel->setStyleSheet(isActive ? "color: #FFFFFF; font-weight: bold; font-size: 12px;" : "color: #AAAAAA; font-size: 12px;");

    QPushButton* closeBtn = new QPushButton(tab);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setFixedSize(16, 16);
    closeBtn->setIcon(UiHelper::getIcon("close", QColor("#888888"), 10));
    closeBtn->setIconSize(QSize(10, 10));
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setObjectName("TabCloseBtn");
    closeBtn->setStyleSheet(
        "QPushButton#TabCloseBtn { border: none; background: transparent; border-radius: 8px; }"
        "QPushButton#TabCloseBtn:hover { background-color: #E81123; }"
    );

    layout->addWidget(iconLabel);
    layout->addWidget(titleLabel, 1);
    layout->addWidget(closeBtn);

    if (isActive) {
        tab->setStyleSheet(
            "QWidget#NavTabActive { background-color: #2D2D2D; border-top: 2px solid #FF551C; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
        );
    } else {
        tab->setStyleSheet(
            "QWidget#NavTabInactive { background-color: transparent; border-top: 2px solid transparent; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
            "QWidget#NavTabInactive:hover { background-color: #252526; }"
        );
    }

    connect(closeBtn, &QPushButton::clicked, this, [index]() {
        NavigationService::instance().closeTab(index);
    });

    tab->installEventFilter(this);
    tab->setProperty("tabIndex", index);

    return tab;
}

} // namespace QuarkMeta