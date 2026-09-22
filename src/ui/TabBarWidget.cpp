#include "TabBarWidget.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include "ColorPicker.h"
#include "HoverEventFilter.h"
#include "../meta/MetadataManager.h"
#include "../core/CoreEngine.h"
#include "../meta/FavoriteDao.h"
#include "../core/AppConfig.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QStyle>
#include <QDateTime>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QWidgetAction>
#include <QGridLayout>
#include <QFileInfo>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDrag>
#include <QMimeData>
#include <QUrl>
#include <QApplication>

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
        m_dragStartPos = event->pos();
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

void TabItemButton::mouseMoveEvent(QMouseEvent* event) {
    if ((event->buttons() & Qt::LeftButton) && !m_dragStartPos.isNull()) {
        if ((event->pos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
            QDrag* drag = new QDrag(this);
            QMimeData* mimeData = new QMimeData();
            mimeData->setData("application/x-quarkmeta-tabindex", QByteArray::number(m_index));
            drag->setMimeData(mimeData);

            QPixmap pixmap = grab();
            drag->setPixmap(pixmap);
            drag->setHotSpot(event->pos());

            drag->exec(Qt::MoveAction);
            m_dragStartPos = QPoint();
            return;
        }
    }
    QPushButton::mouseMoveEvent(event);
}

void TabItemButton::contextMenuEvent(QContextMenuEvent* event) {
    emit customContextMenuRequested(m_index, event->globalPos());
    event->accept();
}

TabBarWidget::TabBarWidget(QWidget* parent, HoverEventFilter* hoverFilter)
    : QWidget(parent), m_hoverFilter(hoverFilter) {
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
    m_btnNewTab->setAttribute(Qt::WA_Hover, true);
    m_btnNewTab->setFixedSize(22, 22);
    m_btnNewTab->setIcon(UiHelper::getIcon("add", QColor("#EEEEEE")));
    m_btnNewTab->setIconSize(QSize(14, 14));
    m_btnNewTab->setObjectName("NewTabBtn");
    m_btnNewTab->setProperty("tooltipText", "新建标签页 (Ctrl+T)");
    if (m_hoverFilter) {
        m_btnNewTab->installEventFilter(m_hoverFilter);
    }

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
    saveStateToConfig();
}

void TabBarWidget::closeTab(int index) {
    if (index < 0 || index >= m_tabs.size()) return;
    if (m_tabs.size() <= 1) {
        // 仅剩一个标签页时，重置为默认“此电脑”
        m_tabs[0].title = "此电脑";
        m_tabs[0].url = "computer://";
        updateTabsUiState();
        emit currentTabChanged(0, "computer://");
        saveStateToConfig();
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
    saveStateToConfig();
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
    saveStateToConfig();
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
    saveStateToConfig();
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

void TabBarWidget::saveStateToConfig() {
    if (m_isInitializing) return;

    QJsonArray tabArray;
    for (const auto& tab : m_tabs) {
        QJsonObject obj;
        obj["title"] = tab.title;
        obj["url"] = tab.url;
        obj["color"] = tab.color;
        obj["iconKey"] = tab.iconKey;
        tabArray.append(obj);
    }

    QJsonObject stateObj;
    stateObj["tabs"] = tabArray;
    stateObj["currentIndex"] = m_currentIndex;

    QString jsonStr = QString::fromUtf8(QJsonDocument(stateObj).toJson(QJsonDocument::Compact));
    AppConfig::instance().setValue("TabBar/SavedState", jsonStr);
    AppConfig::instance().sync();
}

bool TabBarWidget::restoreStateFromConfig() {
    m_isInitializing = true;
    QString jsonStr = AppConfig::instance().getValue("TabBar/SavedState").toString();
    if (jsonStr.isEmpty()) {
        m_isInitializing = false;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isObject()) {
        m_isInitializing = false;
        return false;
    }

    QJsonObject stateObj = doc.object();
    QJsonArray tabArray = stateObj["tabs"].toArray();
    if (tabArray.isEmpty()) {
        m_isInitializing = false;
        return false;
    }

    m_tabs.clear();
    for (const auto& val : tabArray) {
        QJsonObject obj = val.toObject();
        TabInfo info;
        info.id = QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + QString::number(m_tabs.size());
        info.title = obj["title"].toString("此电脑");
        info.url = obj["url"].toString("computer://");
        info.color = obj["color"].toString();
        info.iconKey = obj["iconKey"].toString();
        info.active = false;
        m_tabs.append(info);
    }

    int savedIndex = stateObj["currentIndex"].toInt(0);
    if (savedIndex < 0 || savedIndex >= m_tabs.size()) {
        savedIndex = 0;
    }

    m_currentIndex = savedIndex;
    for (int i = 0; i < m_tabs.size(); ++i) {
        m_tabs[i].active = (i == m_currentIndex);
    }

    rebuildTabsUi();
    m_isInitializing = false;

    if (m_currentIndex >= 0 && m_currentIndex < m_tabs.size()) {
        emit currentTabChanged(m_currentIndex, m_tabs[m_currentIndex].url);
    }
    return true;
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
    saveStateToConfig();
}

void TabBarWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && (event->mimeData()->hasFormat("application/x-quarkmeta-tabindex") || event->mimeData()->hasUrls())) {
        event->acceptProposedAction();
    } else {
        QWidget::dragEnterEvent(event);
    }
}

void TabBarWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData() && (event->mimeData()->hasFormat("application/x-quarkmeta-tabindex") || event->mimeData()->hasUrls())) {
        event->acceptProposedAction();
    } else {
        QWidget::dragMoveEvent(event);
    }
}

void TabBarWidget::dropEvent(QDropEvent* event) {
    if (event->mimeData()) {
        if (event->mimeData()->hasFormat("application/x-quarkmeta-tabindex")) {
            int fromIdx = event->mimeData()->data("application/x-quarkmeta-tabindex").toInt();
            if (fromIdx >= 0 && fromIdx < m_tabs.size()) {
                QPoint dropPos = event->position().toPoint();
                int toIdx = m_tabs.size() - 1;
                for (int i = 0; i < m_tabWidgets.size(); ++i) {
                    QRect rect = m_tabWidgets[i]->geometry();
                    if (dropPos.x() < rect.center().x()) {
                        toIdx = i;
                        break;
                    }
                }

                if (fromIdx != toIdx) {
                    TabInfo movedTab = m_tabs.takeAt(fromIdx);
                    m_tabs.insert(toIdx, movedTab);

                    if (m_currentIndex == fromIdx) {
                        m_currentIndex = toIdx;
                    } else if (m_currentIndex > fromIdx && m_currentIndex <= toIdx) {
                        m_currentIndex--;
                    } else if (m_currentIndex < fromIdx && m_currentIndex >= toIdx) {
                        m_currentIndex++;
                    }

                    for (int i = 0; i < m_tabs.size(); ++i) {
                        m_tabs[i].active = (i == m_currentIndex);
                    }

                    rebuildTabsUi();
                    saveStateToConfig();
                }
            }
            event->acceptProposedAction();
            return;
        } else if (event->mimeData()->hasUrls()) {
            for (const QUrl& url : event->mimeData()->urls()) {
                QString path = url.toLocalFile();
                if (!path.isEmpty() && QFileInfo(path).isDir()) {
                    openOrFocusTab(path);
                    event->acceptProposedAction();
                    return;
                }
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

    QString colorHex = m_tabs[m_currentIndex].color;
    if (colorHex.isEmpty() && !url.startsWith("computer://") && !url.isEmpty()) {
        auto meta = MetadataManager::instance().getMeta(url.toStdWString());
        colorHex = QString::fromStdWString(meta.manualColor);
    }

    m_tabs[m_currentIndex].title = folderName.isEmpty() ? "此电脑" : folderName;
    m_tabs[m_currentIndex].url = url;
    if (!colorHex.isEmpty()) {
        m_tabs[m_currentIndex].color = colorHex;
    }

    if (m_currentIndex < m_tabWidgets.size()) {
        auto tabBtn = m_tabWidgets[m_currentIndex];
        tabBtn->setTabTitle(m_tabs[m_currentIndex].title);
        QColor iconColor = !colorHex.isEmpty() ? QColor(colorHex) : QColor("#EEEEEE");
        QString iconKey = !m_tabs[m_currentIndex].iconKey.isEmpty() ? m_tabs[m_currentIndex].iconKey : (url.startsWith("computer://") ? "computer" : "folder_filled");
        tabBtn->setTabIcon(UiHelper::getIcon(iconKey, iconColor));
    }
    saveStateToConfig();
}

void TabBarWidget::showTabContextMenu(int index, const QPoint& globalPos) {
    if (index < 0 || index >= m_tabs.size()) return;

    const auto& tab = m_tabs[index];

    QMenu menu(this);
    menu.setObjectName("TabContextMenu");
    UiHelper::applyMenuStyle(&menu);

    // 1. 颜色条组件（所有标签页无条件展示）
    QString curColorHex = tab.color.isEmpty() ? "#888888" : tab.color;
        QWidgetAction* colorPickerAction = new QWidgetAction(&menu);
        ColorStripPicker* colorPickerWidget = new ColorStripPicker(curColorHex, &menu);
        colorPickerAction->setDefaultWidget(colorPickerWidget);
        menu.addAction(colorPickerAction);

        // 2. 图标九宫格子菜单
        QMenu* iconMenu = menu.addMenu(UiHelper::getIcon(tab.iconKey.isEmpty() ? "folder_filled" : tab.iconKey, QColor("#EEEEEE")), "切换图标");
        UiHelper::applyMenuStyle(iconMenu);

        QWidgetAction* pickerAction = new QWidgetAction(iconMenu);
        QWidget* pickerWidget = new QWidget(iconMenu);
        QGridLayout* pickerLayout = new QGridLayout(pickerWidget);
        pickerLayout->setContentsMargins(6, 6, 6, 6);
        pickerLayout->setSpacing(6);

        static const QList<QPair<QString, QString>> builtInIcons = {
            {"默认文件夹", "folder_filled"}, {"照片媒体", "image_filled"}, {"相册图片", "image_picture"},
            {"时钟历史", "clock_filled"}, {"星标收藏", "star_filled"}, {"实心星标", "star_001"},
            {"空心星标", "star_002"}, {"爱心常用", "heart_filled"}, {"加密安全", "lock_filled"},
            {"图书文档", "book"}, {"附加文档", "document_attach"}, {"配置管理", "settings_filled"},
            {"网络球体", "globe_filled"}, {"主页主路径", "home_filled"}, {"标签标记", "tag_filled"},
            {"书签指示", "bookmark_filled"}, {"音频音乐", "music_filled"}, {"视频影视", "video_filled"},
            {"摄影相机", "camera_filled"}, {"盾牌防护", "shield_filled"}, {"物理硬盘", "hard_drive"},
            {"云端同步", "cloud_filled"}, {"闪电极速", "zap_filled"}, {"魔法火花", "sparkles_filled"},
            {"旗帜标记", "flag_filled"}, {"旗帜标示", "flag"}, {"礼物珍藏", "gift_filled"},
            {"奖星勋章", "award_filled"}, {"回收废弃", "trash_filled"}, {"邮件通信", "mail_filled"},
            {"消息通知", "message_filled"}, {"电话联系", "phone_filled"}, {"地理定位", "map_pin_filled"},
            {"日光白天", "sun_filled"}, {"夜间月亮", "moon_filled"}, {"日历日程", "calendar_filled"},
            {"今日任务", "today_filled"}, {"九宫网格", "grid_filled"}, {"布局排版", "layout_filled"},
            {"数据表格", "table_filled"}, {"磁盘保存", "save_filled"}, {"魔棒工具", "wand_filled"},
            {"附件剪辑", "paperclip"}, {"归档文件", "archive"},
            {"OneNote笔记", "onenote"}, {"下载中心", "download"}
        };

        QColor catColor = QColor(curColorHex);
        QList<QPair<QPushButton*, QString>> iconButtons;
        int row = 0, col = 0;
        for (const auto& pair : builtInIcons) {
            QString iconKey = pair.second;
            QPushButton* btn = new QPushButton(pickerWidget);
            btn->setFixedSize(28, 28);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setObjectName("FavPickerIconBtn");
            btn->setIcon(UiHelper::getIcon(iconKey, catColor, 18));
            btn->setIconSize(QSize(18, 18));
            pickerLayout->addWidget(btn, row, col);

            iconButtons.append({btn, iconKey});

            connect(btn, &QPushButton::clicked, this, [this, index, iconKey]() {
                if (index >= 0 && index < m_tabs.size()) {
                    m_tabs[index].iconKey = iconKey;
                    updateTabsUiState();
                    FavoriteDao::updateFavorite(m_tabs[index].url, iconKey, m_tabs[index].color);
                }
            });

            col++;
            if (col >= 5) { col = 0; row++; }
        }

        pickerWidget->setLayout(pickerLayout);
        pickerAction->setDefaultWidget(pickerWidget);
        iconMenu->addAction(pickerAction);

        connect(colorPickerWidget, &ColorStripPicker::colorSelected, this, [this, index, iconMenu, iconButtons](const QString& hexColor) {
            if (index < 0 || index >= m_tabs.size()) return;

            QString finalColor = hexColor.isEmpty() ? "#888888" : hexColor.toUpper();
            m_tabs[index].color = finalColor;

            QString iconKey = m_tabs[index].iconKey.isEmpty() ? "folder_filled" : m_tabs[index].iconKey;
            QString targetPath = m_tabs[index].url;

            iconMenu->setIcon(UiHelper::getIcon(iconKey, QColor(finalColor)));
            for (const auto& btnPair : iconButtons) {
                btnPair.first->setIcon(UiHelper::getIcon(btnPair.second, QColor(finalColor), 18));
            }

            if (!targetPath.isEmpty() && !targetPath.startsWith("computer://")) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetColor;
                cmd.targetPaths = {targetPath};
                cmd.params["color"] = finalColor;
                CoreEngine::instance().executeCommand(cmd);
            }

            updateTabsUiState();
        });

        menu.addSeparator();

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
        if (tab.color.isEmpty() && !tab.url.startsWith("computer://") && !tab.url.isEmpty()) {
            auto meta = MetadataManager::instance().getMeta(tab.url.toStdWString());
            tab.color = QString::fromStdWString(meta.manualColor);
        }

        auto tabBtn = m_tabWidgets[i];
        tabBtn->setIndex(i);
        tabBtn->setTabTitle(tab.title);

        QColor iconColor = !tab.color.isEmpty() ? QColor(tab.color) : (tab.active ? QColor("#EEEEEE") : QColor("#888888"));
        QString iconKey = !tab.iconKey.isEmpty() ? tab.iconKey : (tab.url.startsWith("computer://") ? "computer" : "folder_filled");
        tabBtn->setTabIcon(UiHelper::getIcon(iconKey, iconColor));
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
        if (tab.color.isEmpty() && !tab.url.startsWith("computer://") && !tab.url.isEmpty()) {
            auto meta = MetadataManager::instance().getMeta(tab.url.toStdWString());
            tab.color = QString::fromStdWString(meta.manualColor);
        }

        TabItemButton* tabItem = new TabItemButton(i, this);
        tabItem->setTabTitle(tab.title);
        QColor iconColor = !tab.color.isEmpty() ? QColor(tab.color) : (tab.active ? QColor("#EEEEEE") : QColor("#888888"));
        QString iconKey = !tab.iconKey.isEmpty() ? tab.iconKey : (tab.url.startsWith("computer://") ? "computer" : "folder_filled");
        tabItem->setTabIcon(UiHelper::getIcon(iconKey, iconColor));
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
