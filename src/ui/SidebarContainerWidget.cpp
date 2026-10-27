#include "SidebarContainerWidget.h"
#include "UiHelper.h"
#include "../core/AppConfig.h"
#include "../core/SearchHistoryService.h"
#include <QMouseEvent>

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
    m_tabBar->setUsesScrollButtons(false);
    m_tabBar->setIconSize(QSize(16, 16));
    m_tabBar->addTab(UiHelper::getIcon("Favorites.svg", "#888888", 16), "收藏夹");
    m_tabBar->addTab(UiHelper::getIcon("Search-in-library", "#888888", 16), "库");
    m_tabBar->addTab(UiHelper::getIcon("Destination.svg", "#ff551c", 16), "标记");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);
    m_favoritePanel = new FavoritePanel(this);
    m_libraryPanel = new LibraryPanel(this);
    m_markedPanel = new MarkedPanel(this);

    m_stackedWidget->addWidget(m_favoritePanel);
    m_stackedWidget->addWidget(m_libraryPanel);
    m_stackedWidget->addWidget(m_markedPanel);

    m_mainLayout->addWidget(m_stackedWidget, 1);

    // 底部统一搜索过滤框部件创建与配置
    QWidget* filterContainer = new QWidget(this);
    filterContainer->setObjectName("SidebarFilterContainer");
    QVBoxLayout* filterLayout = new QVBoxLayout(filterContainer);
    filterLayout->setContentsMargins(8, 8, 8, 8);
    filterLayout->setSpacing(0);

    m_filterEdit = new QLineEdit(filterContainer);
    m_filterEdit->setObjectName("SidebarFilterEdit");
    m_filterEdit->setPlaceholderText("筛选分类");
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setFixedHeight(32);
    m_filterEdit->addAction(UiHelper::getIcon("filter_funnel", QColor("#888888"), 14), QLineEdit::LeadingPosition);
    m_filterEdit->installEventFilter(this);
    filterLayout->addWidget(m_filterEdit);

    m_historyPanel = new SearchHistoryPanel(this);
    m_historyPanel->setCategory("sidebar_filter");

    connect(m_historyPanel, &SearchHistoryPanel::historyItemClicked, this, [this](const QString& keyword) {
        if (m_filterEdit) {
            m_filterEdit->setText(keyword);
        }
    });

    connect(m_filterEdit, &QLineEdit::returnPressed, this, [this]() {
        if (!m_filterEdit) return;
        QString text = m_filterEdit->text().trimmed();
        if (!text.isEmpty()) {
            SearchHistoryService::instance().appendSearch("sidebar_filter", text);
        }
    });

    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        QString kw = text.trimmed();
        if (m_favoritePanel) {
            m_favoritePanel->filterTreeNodes(kw);
        }
        if (m_libraryPanel) {
            if (m_libraryPanel->delegate()) {
                m_libraryPanel->delegate()->setFilterKeyword(kw);
            }
            m_libraryPanel->filterTreeNodes(kw);
            if (m_libraryPanel->treeView() && m_libraryPanel->treeView()->viewport()) {
                m_libraryPanel->treeView()->viewport()->update();
            }
        }
    });

    m_mainLayout->addWidget(filterContainer, 0);

    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
        if (index == 1 && m_libraryPanel) {
            m_libraryPanel->loadLibrary();
        }
        if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
            QString kw = m_filterEdit->text().trimmed();
            if (index == 0 && m_favoritePanel) m_favoritePanel->filterTreeNodes(kw);
            if (index == 1 && m_libraryPanel) m_libraryPanel->filterTreeNodes(kw);
        }
        AppConfig::instance().setValue("Sidebar/CurrentTabIndex", index);
        AppConfig::instance().sync();
        emit sidebarTabChanged(index);
    });

    int savedIndex = AppConfig::instance().getValue("Sidebar/CurrentTabIndex", 0).toInt();
    if (savedIndex >= 0 && savedIndex < m_tabBar->count()) {
        m_tabBar->setCurrentIndex(savedIndex);
    }
}

bool SidebarContainerWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_filterEdit && event->type() == QEvent::MouseButtonDblClick) {
        if (m_historyPanel && m_filterEdit) {
            QStringList history = SearchHistoryService::instance().getHistory("sidebar_filter");
            if (!history.isEmpty()) {
                m_historyPanel->setHistory(history, "最近筛选");
                m_historyPanel->showBelow(m_filterEdit);
            }
        }
        return true;
    }
    return QFrame::eventFilter(watched, event);
}

} // namespace QuarkMeta
