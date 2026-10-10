#include "SidebarContainerWidget.h"
#include "UiHelper.h"

namespace QuarkMeta {

SidebarContainerWidget::SidebarContainerWidget(QWidget* parent) : QFrame(parent) {
    setObjectName("SidebarContainerWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
}

void SidebarContainerWidget::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);

    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(5, 0, 5, 0);
    headerLayout->setSpacing(0);

    m_tabBar = new QTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->addTab("收藏夹");
    m_tabBar->addTab("库");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);
    m_favoritePanel = new FavoritePanel(this);
    m_libraryPanel = new LibraryPanel(this);

    m_stackedWidget->addWidget(m_favoritePanel);
    m_stackedWidget->addWidget(m_libraryPanel);

    m_mainLayout->addWidget(m_stackedWidget, 1);

    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
        if (index == 1 && m_libraryPanel) {
            m_libraryPanel->loadLibrary();
        }
        emit sidebarTabChanged(index);
    });
}

} // namespace QuarkMeta
