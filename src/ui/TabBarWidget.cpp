#include "TabBarWidget.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include "../meta/MetadataManager.h"

#include <QStyle>
#include <QDateTime>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

namespace QuarkMeta {

TabItemButton::TabItemButton(int index, QWidget* parent)
    : QPushButton(parent), m_index(index) {
    setObjectName("TabItem");
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(28);
    setMaximumWidth(180);
    setMinimumWidth(80);
    setCursor(Qt::PointingHandCursor);

    QHBoxLayout* itemLayout = new QHBoxLayout(this);
    itemLayout->setContentsMargins(8, 0, 6, 0);
    itemLayout->setSpacing(6);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setObjectName("TabIconLabel");
    m_iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_iconLabel->setFixedSize(14, 14);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("TabTitleLabel");
    m_titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_btnClose = new QPushButton(this);
    m_btnClose->setObjectName("TabCloseBtn");
    m_btnClose->setFocusPolicy(Qt::NoFocus);
    m_btnClose->setFixedSize(16, 16);
    m_btnClose->setIcon(UiHelper::getIcon("close", QColor("#888888")));
    m_btnClose->setIconSize(QSize(10, 10));

    connect(m_btnClose, &QPushButton::clicked, this, [this]() {
        emit closeClicked(m_index);
    });

    itemLayout->addWidget(m_iconLabel, 0, Qt::AlignVCenter);
    itemLayout->addWidget(m_titleLabel, 1, Qt::AlignVCenter);
    itemLayout->addWidget(m_btnClose, 0, Qt::AlignVCenter);
}

void TabItemButton::setTabTitle(const QString& title) {
    if (m_titleLabel) {
        QFontMetrics fm(m_titleLabel->font());
        QString elided = fm.elidedText(title, Qt::ElideRight, 110);
        m_titleLabel->setText(elided);
        setToolTip(title);
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

void TabItemButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit tabClicked(m_index);
        event->accept();
        return;
    } else if (event->button() == Qt::MiddleButton) {
        emit middleClicked(m_index);
        event->accept();
        return;
    }
    QPushButton::mousePressEvent(event);
}

void TabItemButton::contextMenuEvent(QContextMenuEvent* event) {
    emit customContextMenuRequested(m_index, event->globalPos());
    event->accept();
}

TabBarWidget::TabBarWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("TabBarWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setAcceptDrops(true);
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
    if (m_tabs.size() <= 1) {
        // 仅剩一个标签页时，重置为默认“此电脑”
        m_tabs[0].title = "此电脑";
        m_tabs[0].url = "computer://";
        updateTabsUiState();
        emit currentTabChanged(0, "computer://");
        return;
    }

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

void TabBarWidget::closeOtherTabs(int index) {
    if (index < 0 || index >= m_tabs.size()) return;
    TabInfo target = m_tabs[index];
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (i != index) {
            m_closedTabsHistory.append(m_tabs[i]);
        }
    }
    m_tabs.clear();
    m_tabs.append(target);
    m_currentIndex = 0;
    setCurrentIndex(0, true);
}

void TabBarWidget::closeRightTabs(int index) {
    if (index < 0 || index >= m_tabs.size() - 1) return;
    while (m_tabs.size() > index + 1) {
        m_closedTabsHistory.append(m_tabs.takeAt(index + 1));
    }
    if (m_currentIndex > index) {
        m_currentIndex = index;
    }
    setCurrentIndex(m_currentIndex, true);
}

void TabBarWidget::duplicateTab(int index) {
    if (index < 0 || index >= m_tabs.size()) return;
    const auto& src = m_tabs[index];
    addTab(src.title, src.url, true);
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

void TabBarWidget::selectNextTab() {
    if (m_tabs.isEmpty()) return;
    int nextIdx = (m_currentIndex + 1) % m_tabs.size();
    setCurrentIndex(nextIdx, true);
}

void TabBarWidget::selectPreviousTab() {
    if (m_tabs.isEmpty()) return;
    int prevIdx = (m_currentIndex - 1 + m_tabs.size()) % m_tabs.size();
    setCurrentIndex(prevIdx, true);
}

void TabBarWidget::openOrFocusTab(const QString& rawPath) {
    if (rawPath.isEmpty()) return;
    QString cleanTarget = QDir::cleanPath(rawPath);

    for (int i = 0; i < m_tabs.size(); ++i) {
        if (QDir::cleanPath(m_tabs[i].url) == cleanTarget) {
            setCurrentIndex(i, true);
            return;
        }
    }

    QFileInfo fi(cleanTarget);
    QString title = fi.fileName();
    if (title.isEmpty()) title = cleanTarget;
    addTab(title, cleanTarget, true);
}

void TabBarWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QWidget::dragEnterEvent(event);
    }
}

void TabBarWidget::dropEvent(QDropEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            QString path = url.toLocalFile();
            if (!path.isEmpty() && QFileInfo(path).isDir()) {
                openOrFocusTab(path);
                event->acceptProposedAction();
                return;
            }
        }
    }
    QWidget::dropEvent(event);
}

void TabBarWidget::updateCurrentTabTitle(const QString& title, const QString& url) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    QString folderName = title;
    if (url == "computer://" || url.isEmpty()) {
        folderName = "此电脑";
    } else if (title.contains("/") || title.contains("\\")) {
        QString cleanPath = QDir::cleanPath(url);
        QFileInfo fi(cleanPath);
        folderName = fi.fileName();
        if (folderName.isEmpty()) {
            folderName = cleanPath;
        }
    }

    QString colorHex;
    if (!url.startsWith("computer://") && !url.isEmpty()) {
        auto meta = MetadataManager::instance().getMeta(url.toStdWString());
        colorHex = QString::fromStdWString(meta.manualColor);
    }

    if (m_tabs[m_currentIndex].title == folderName && m_tabs[m_currentIndex].url == url && m_tabs[m_currentIndex].color == colorHex) return;

    m_tabs[m_currentIndex].title = folderName.isEmpty() ? "此电脑" : folderName;
    m_tabs[m_currentIndex].url = url;
    m_tabs[m_currentIndex].color = colorHex;

    if (m_currentIndex < m_tabWidgets.size()) {
        auto tabBtn = m_tabWidgets[m_currentIndex];
        tabBtn->setTabTitle(m_tabs[m_currentIndex].title);
        QColor iconColor = !colorHex.isEmpty() ? QColor(colorHex) : QColor("#EEEEEE");
        tabBtn->setTabIcon(UiHelper::getIcon(url.startsWith("computer://") ? "computer" : "folder_filled", iconColor));
    }
}

void TabBarWidget::showTabContextMenu(int index, const QPoint& globalPos) {
    if (index < 0 || index >= m_tabs.size()) return;

    QMenu menu(this);
    menu.setObjectName("TabContextMenu");
    UiHelper::applyMenuStyle(&menu);

    QAction* actRefresh = menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 16), "重新加载 (F5)");
    QAction* actDuplicate = menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 16), "复制标签页");
    menu.addSeparator();
    QAction* actClose = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 16), "关闭标签页 (Ctrl+W)");
    QAction* actCloseOthers = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 16), "关闭其他标签页");
    QAction* actCloseRight = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 16), "关闭右侧标签页");
    menu.addSeparator();
    QAction* actRestore = menu.addAction(UiHelper::getIcon("history", QColor("#EEEEEE"), 16), "重新打开关闭的标签页 (Ctrl+Shift+T)");

    actRestore->setEnabled(!m_closedTabsHistory.isEmpty());
    actCloseRight->setEnabled(index < m_tabs.size() - 1);
    actCloseOthers->setEnabled(m_tabs.size() > 1);

    connect(actRefresh, &QAction::triggered, this, [this]() {
        emit refreshRequested();
    });
    connect(actDuplicate, &QAction::triggered, this, [this, index]() {
        duplicateTab(index);
    });
    connect(actClose, &QAction::triggered, this, [this, index]() {
        closeTab(index);
    });
    connect(actCloseOthers, &QAction::triggered, this, [this, index]() {
        closeOtherTabs(index);
    });
    connect(actCloseRight, &QAction::triggered, this, [this, index]() {
        closeRightTabs(index);
    });
    connect(actRestore, &QAction::triggered, this, [this]() {
        restoreLastClosedTab();
    });

    menu.exec(globalPos);
}

void TabBarWidget::updateTabsUiState() {
    if (m_tabWidgets.size() != m_tabs.size()) {
        rebuildTabsUi();
        return;
    }

    for (int i = 0; i < m_tabs.size(); ++i) {
        auto& tab = m_tabs[i];
        if (!tab.url.startsWith("computer://") && !tab.url.isEmpty()) {
            auto meta = MetadataManager::instance().getMeta(tab.url.toStdWString());
            tab.color = QString::fromStdWString(meta.manualColor);
        } else {
            tab.color.clear();
        }

        auto tabBtn = m_tabWidgets[i];
        tabBtn->setIndex(i);
        tabBtn->setTabTitle(tab.title);

        QColor iconColor = !tab.color.isEmpty() ? QColor(tab.color) : (tab.active ? QColor("#EEEEEE") : QColor("#888888"));
        tabBtn->setTabIcon(UiHelper::getIcon(tab.url.startsWith("computer://") ? "computer" : "folder_filled", iconColor));
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
        auto& tab = m_tabs[i];
        if (!tab.url.startsWith("computer://") && !tab.url.isEmpty()) {
            auto meta = MetadataManager::instance().getMeta(tab.url.toStdWString());
            tab.color = QString::fromStdWString(meta.manualColor);
        } else {
            tab.color.clear();
        }

        TabItemButton* tabItem = new TabItemButton(i, this);
        tabItem->setTabTitle(tab.title);
        QColor iconColor = !tab.color.isEmpty() ? QColor(tab.color) : (tab.active ? QColor("#EEEEEE") : QColor("#888888"));
        tabItem->setTabIcon(UiHelper::getIcon(tab.url.startsWith("computer://") ? "computer" : "folder_filled", iconColor));
        tabItem->setActive(tab.active);

        connect(tabItem, &TabItemButton::closeClicked, this, [this](int idx) {
            closeTab(idx);
        });

        connect(tabItem, &TabItemButton::middleClicked, this, [this](int idx) {
            closeTab(idx);
        });

        connect(tabItem, &TabItemButton::tabClicked, this, [this](int idx) {
            setCurrentIndex(idx, true);
        });

        connect(tabItem, &TabItemButton::customContextMenuRequested, this, [this](int idx, const QPoint& globalPos) {
            showTabContextMenu(idx, globalPos);
        });

        m_tabWidgets.append(tabItem);
        m_tabsLayout->addWidget(tabItem);
    }
}

} // namespace QuarkMeta
