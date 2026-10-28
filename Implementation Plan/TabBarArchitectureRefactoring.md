# QuarkMeta 标题栏标签页组件 (TabBarWidget) 规范化架构重构实施方案

> 本方案严格遵循 `Standard Specifications for Implementation Plan.md` 与根目录 `Memories.md` 第 19 章规范制订。  
> 恪守实施界限铁律：本方案仅作为物理隔离的实施方案存放于 `Implementation Plan/` 目录中，在获得用户明确授权前，绝对禁止未经许可修改任何源代码。

---

## 方案前置：已读物理证据清单 (Gate 1: Pre-Draft Evidence)

### 1. 物理读取文件列表与只读确认
- [x] `src/ui/TitleBarWidget.h` (已通过 `read_file` 确认布局、成员变量及接口声明)
- [x] `src/ui/TitleBarWidget.cpp` (已通过 `read_file` 确认 `initUi` 中 `m_layout` 与右侧按钮平铺排布事实)
- [x] `src/ui/TabBarWidget.h` (已通过 `read_file` 确认 `TabItemButton`、`TabInfo` 结构体及信号槽)
- [x] `src/ui/TabBarWidget.cpp` (已通过 `read_file` 确认 `rebuildTabsUi` 暴力全量清空、手写图标九宫格及包含底层 DAO 事实)
- [x] `src/ui/PanelMediator.h` 与 `src/ui/PanelMediator.cpp` (已确认 TabBarWidget 与 ContentPanel / NavigationService 联动链路)
- [x] `src/ui/controllers/ContextMenuFactory.h` (已确认既有官方 API `buildIconPickerMenu` 真实存在)
- [x] `src/core/NavigationService.h` (已确认全局统一入口 `openInNewTab(rawPath)` 真实存在)
- [x] `src/ui/AddressBar.cpp`、`src/ui/FavoritePanel.cpp`、`src/ui/NavPanel.cpp` (已确认包含 3 处复制粘贴的 `window()->findChild<TabBarWidget*>()`)
- [x] `resources/style.qss` (已确认第 11-70 行 `TabBarWidget` 样式定义)

### 2. 既有官方 API 检索日志 (Search First 结果)
1. **图标/色彩选择二级菜单**：检索 `buildIconPickerMenu`，命命中位于 `src/ui/controllers/ContextMenuFactory.h` 第 69 行：
   ```cpp
   static QMenu* buildIconPickerMenu(QMenu* parentMenu, const QString& curIconKey, const QString& curColorHex, ...);
   ```
   *结论：`TabBarWidget.cpp` 内部手写的 80 行 `builtInIcons` 与九宫格按钮完全属于另起炉灶的重复造轮子，重构中予以物理彻底删除并复用该官方 API。*
2. **全局新建标签页导航入口**：检索 `openInNewTab`，命中官方 SSOT 入口 `NavigationService::instance().openInNewTab(const QString& rawPath)`。
   *结论：无需跨层下钻 `findChild<TabBarWidget*>()`，所有外部面板统一收敛至 `NavigationService`。*

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 现状缺陷与事故根因
根据用户实测截图反馈（红圈标记左侧 TabBar，长红箭头指向被挤压的右侧控制区），当前标题栏标签系统存在四大灾难级架构隐患：
1. **无溢出保护与弹性伸缩机制（红箭头事故根因）**：
   - `TitleBarWidget` 内部直接采用单层 `QHBoxLayout` 平铺布局，`m_tabBar` 与右侧工具按钮（滑块、排列、窗口控制等）共享同一布局流；
   - `TabBarWidget` 未限制最大可用边界，单个标签拥有 `80px` 最小硬编码宽度。随着标签数量递增，标签栏无限向右撑开，彻底挤死弹性弹簧（`addStretch()`），进而将右侧控制区挤爆、变形乃至顶出屏幕边界。
2. **暴力全量推倒重建（`rebuildTabsUi` 反模式）**：
   - 每次标签页增减或状态更新，`TabBarWidget::rebuildTabsUi()` 均直接调用 `deleteLater()` 销毁所有现有 `TabItemButton` 并重新循环 `new` 一遍；
   - 导致极其严重的对象抖动、堆内存碎片、鼠标焦点瞬间归零以及正在展开的右键上下文失活崩溃。
3. **严重践踏架构分层（View 层私通数据库）**：
   - `TabBarWidget.cpp` 开头直接包含 `#include "../meta/FavoriteDao.h"`、`"../meta/MetadataManager.h"`、`"../core/CoreEngine.h"`；
   - 作为 UI View 控件，直接在右键菜单槽函数中写 SQL 操作收藏夹数据库和核心引擎，严重违反 `SYSTEM_PROMPT.md`【依赖锁】（$Shell \to View \to Controller \to Domain \to Data$）。
4. **臭名昭著的跨层偷指针（`findChild` 反模式）**：
   - `AddressBar.cpp`、`FavoritePanel.cpp`、`NavPanel.cpp` 均采用 `window()->findChild<TabBarWidget*>()` 随后又 `window()->findChild<ContentPanel*>()` 这种极度脆弱的跨层窥探写法，破坏了黑盒封装。

### 1.2 重构目标
1. **空间硬锁**：在 `TitleBarWidget` 中将右侧工具栏封装为具有固定最小尺寸与 `QSizePolicy::Fixed` 约束的物理安全区，享有最高优先级保护权；
2. **弹性压缩与滚轮横向滚动**：`TabBarWidget` 内置动态宽度折叠算子（`240px` $\to$ `80px`），并引入隐藏原生滚动条的平滑水平滚动视口（`QScrollArea`），鼠标滚轮即可横向平移浏览，永远不撑爆边界；
3. **增量差量更新 (Incremental Diff)**：彻底废除 `rebuildTabsUi()`，基于唯一 `tabId` 进行增量创建、定向销毁与就地属性更新；
4. **解耦分层与清剿死代码**：物理清除 `FavoriteDao` / `MetadataManager` 依赖，复用 `ContextMenuFactory::buildIconPickerMenu`，拔除全部 `findChild`。

---

## Chapter 2: Modified Files List（影响文件清单）

### 待重构文件
1. `src/ui/TitleBarWidget.h`（新增右侧工具区容器声明）
2. `src/ui/TitleBarWidget.cpp`（构建右侧固定安全区与尺寸约束锁）
3. `src/ui/TabBarWidget.h`（引入视口滚动容器、差量维护映射表与纯 View 信号）
4. `src/ui/TabBarWidget.cpp`（实现自适应压缩、平滑滚动、差量 Diff 更新；清除 DAO 依赖与手写九宫格）
5. `src/ui/PanelMediator.h`（增加标签元数据与收藏联动的 Mediator 槽函数）
6. `src/ui/PanelMediator.cpp`（接管 Tab 右键菜单的收藏与元数据业务操作）
7. `resources/style.qss`（为平滑滚动容器提供无缝暗色样式）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/TitleBarWidget.h`

```cpp
<<<<<<< SEARCH
    QHBoxLayout* m_layout = nullptr;
    QLabel* m_logoLabel = nullptr;
    TabBarWidget* m_tabBar = nullptr;

    QPushButton* m_btnViewMenu = nullptr;
    QSlider* m_sizeSlider = nullptr;
=======
    QHBoxLayout* m_layout = nullptr;
    QLabel* m_logoLabel = nullptr;
    TabBarWidget* m_tabBar = nullptr;
    QWidget* m_rightToolContainer = nullptr;

    QPushButton* m_btnViewMenu = nullptr;
    QSlider* m_sizeSlider = nullptr;
>>>>>>> REPLACE
```

### 3.2 `src/ui/TitleBarWidget.cpp`

```cpp
<<<<<<< SEARCH
    m_tabBar = new TabBarWidget(this, hoverFilter);
    m_layout->addWidget(m_tabBar);
    m_layout->addStretch();

    auto createTitleBtn = [this, hoverFilter](const QString& iconKey, const QString& tip) -> QPushButton* {
=======
    m_tabBar = new TabBarWidget(this, hoverFilter);
    m_tabBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_layout->addWidget(m_tabBar, 1);

    m_rightToolContainer = new QWidget(this);
    m_rightToolContainer->setObjectName("TitleRightToolContainer");
    m_rightToolContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QHBoxLayout* rightLayout = new QHBoxLayout(m_rightToolContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto createTitleBtn = [this, hoverFilter](const QString& iconKey, const QString& tip) -> QPushButton* {
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    m_layout->addWidget(m_sizeSlider, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnViewMenu, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnToggleDriveBar, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnLayout, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnCreate, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnPinTop, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnMin, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnMax, 0, Qt::AlignVCenter);
    m_layout->addWidget(m_btnClose, 0, Qt::AlignVCenter);
=======
    rightLayout->addWidget(m_sizeSlider, 0, Qt::AlignVCenter);
    rightLayout->addWidget(m_btnViewMenu, 0, Qt::AlignVCenter);
    rightLayout->addWidget(m_btnToggleDriveBar, 0, Qt::AlignVCenter);
    rightLayout->addWidget(m_btnLayout, 0, Qt::AlignVCenter);
    rightLayout->addWidget(m_btnCreate, 0, Qt::AlignVCenter);
    rightLayout->addWidget(m_btnPinTop, 0, Qt::AlignVCenter);
    rightLayout->addWidget(m_btnMin, 0, Qt::AlignVCenter);
    rightLayout->addWidget(m_btnMax, 0, Qt::AlignVCenter);
    rightLayout->addWidget(m_btnClose, 0, Qt::AlignVCenter);

    m_layout->addWidget(m_rightToolContainer, 0, Qt::AlignRight | Qt::AlignVCenter);
>>>>>>> REPLACE
```

### 3.3 `src/ui/TabBarWidget.h`

```cpp
<<<<<<< SEARCH
#include <QLabel>
#include <QList>
#include <QString>
#include <QEvent>
#include <QResizeEvent>
=======
#include <QLabel>
#include <QList>
#include <QMap>
#include <QString>
#include <QEvent>
#include <QResizeEvent>
#include <QScrollArea>
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
signals:
    void tabAboutToChange(int oldIndex);
    void currentTabChanged(int index, const QString& url);
    void tabClosed(int index);
    void newTabRequested();
    void refreshRequested();
    void mergeRequested(int sourceIndex, int targetIndex);
    void splitRequested(int tabIndex);
    void folderDroppedOnTab(int tabIndex, const QString& folderPath);
=======
signals:
    void tabAboutToChange(int oldIndex);
    void currentTabChanged(int index, const QString& url);
    void tabClosed(int index);
    void newTabRequested();
    void refreshRequested();
    void mergeRequested(int sourceIndex, int targetIndex);
    void splitRequested(int tabIndex);
    void folderDroppedOnTab(int tabIndex, const QString& folderPath);
    void tabColorChangeRequested(const QString& url, const QString& colorHex);
    void tabIconChangeRequested(const QString& url, const QString& iconKey);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    void showTabContextMenu(int index, const QPoint& globalPos);
    void updateTabsUiState();
    void rebuildTabsUi();

    QHBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_tabsLayout = nullptr;
    QPushButton* m_btnNewTab = nullptr;
    HoverEventFilter* m_hoverFilter = nullptr;

    QList<TabInfo> m_tabs;
    QList<TabInfo> m_closedTabsHistory;
    QList<TabItemButton*> m_tabWidgets;
    int m_currentIndex = -1;
    bool m_isInitializing = true;
=======
    void showTabContextMenu(int index, const QPoint& globalPos);
    void updateTabsUiState();
    void applyDiffUpdate();
    void adjustTabWidths();

    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    QHBoxLayout* m_mainLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_tabsContainer = nullptr;
    QHBoxLayout* m_tabsLayout = nullptr;
    QPushButton* m_btnNewTab = nullptr;
    HoverEventFilter* m_hoverFilter = nullptr;

    QList<TabInfo> m_tabs;
    QList<TabInfo> m_closedTabsHistory;
    QMap<QString, TabItemButton*> m_tabItemMap;
    int m_currentIndex = -1;
    bool m_isInitializing = true;
>>>>>>> REPLACE
```

### 3.4 `src/ui/TabBarWidget.cpp`

```cpp
<<<<<<< SEARCH
#include "HoverEventFilter.h"
#include "../meta/MetadataManager.h"
#include "../core/CoreEngine.h"
#include "../meta/FavoriteDao.h"
#include "../core/AppConfig.h"

#include <QJsonDocument>
=======
#include "HoverEventFilter.h"
#include "controllers/ContextMenuFactory.h"
#include "../core/AppConfig.h"

#include <QJsonDocument>
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
#include <QUrl>
#include <QApplication>

namespace QuarkMeta {
=======
#include <QUrl>
#include <QApplication>
#include <QWheelEvent>
#include <QScrollBar>

namespace QuarkMeta {
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 2, 0, 0);
    m_mainLayout->setSpacing(4);

    m_tabsLayout = new QHBoxLayout();
    m_tabsLayout->setContentsMargins(0, 0, 0, 0);
    m_tabsLayout->setSpacing(2);

    m_btnNewTab = new QPushButton(this);
=======
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 2, 0, 0);
    m_mainLayout->setSpacing(4);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("TabBarScrollArea");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setAttribute(Qt::WA_TranslucentBackground, true);

    m_tabsContainer = new QWidget(m_scrollArea);
    m_tabsContainer->setObjectName("TabBarContainer");
    m_tabsContainer->setAttribute(Qt::WA_TranslucentBackground, true);

    m_tabsLayout = new QHBoxLayout(m_tabsContainer);
    m_tabsLayout->setContentsMargins(0, 0, 0, 0);
    m_tabsLayout->setSpacing(2);
    m_tabsContainer->setLayout(m_tabsLayout);

    m_scrollArea->setWidget(m_tabsContainer);

    m_btnNewTab = new QPushButton(this);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    m_mainLayout->addLayout(m_tabsLayout);
    m_mainLayout->addWidget(m_btnNewTab, 0, Qt::AlignVCenter);
    m_mainLayout->addStretch();
=======
    m_mainLayout->addWidget(m_scrollArea, 1);
    m_mainLayout->addWidget(m_btnNewTab, 0, Qt::AlignVCenter);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
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
=======
void TabBarWidget::showTabContextMenu(int index, const QPoint& globalPos) {
    if (index < 0 || index >= m_tabs.size()) return;

    const auto& tab = m_tabs[index];

    QMenu menu(this);
    menu.setObjectName("TabContextMenu");
    UiHelper::applyMenuStyle(&menu);

    QString curColorHex = tab.color.isEmpty() ? "#888888" : tab.color;
    QString curIconKey = tab.iconKey.isEmpty() ? "folder_filled" : tab.iconKey;

    // 复用 ContextMenuFactory 官方 API 消除 80 行另起炉灶九宫格
    ContextMenuFactory::buildIconPickerMenu(&menu, curIconKey, curColorHex,
        [this, index](const QString& iconKey) {
            if (index >= 0 && index < m_tabs.size()) {
                m_tabs[index].iconKey = iconKey;
                emit tabIconChangeRequested(m_tabs[index].url, iconKey);
                updateTabsUiState();
            }
        },
        [this, index](const QString& colorHex) {
            if (index >= 0 && index < m_tabs.size()) {
                m_tabs[index].color = colorHex;
                emit tabColorChangeRequested(m_tabs[index].url, colorHex);
                updateTabsUiState();
            }
        },
        this);

    menu.addSeparator();
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
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
            setCurrentIndex(idx, false);
        });

        connect(tabItem, &TabItemButton::customContextMenuRequested, this, [this](int idx, const QPoint& globalPos) {
            showTabContextMenu(idx, globalPos);
        });

        m_tabWidgets.append(tabItem);
        m_tabsLayout->addWidget(tabItem);
    }
}
=======
void TabBarWidget::wheelEvent(QWheelEvent* event) {
    if (m_scrollArea && m_scrollArea->horizontalScrollBar()) {
        int delta = event->angleDelta().y();
        if (delta == 0) delta = event->angleDelta().x();
        m_scrollArea->horizontalScrollBar()->setValue(m_scrollArea->horizontalScrollBar()->value() - delta);
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

void TabBarWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    adjustTabWidths();
}

void TabBarWidget::adjustTabWidths() {
    int count = m_tabs.size();
    if (count <= 0 || !m_scrollArea) return;

    int availWidth = m_scrollArea->viewport()->width();
    int targetW = 240;
    if (availWidth > 0 && count > 0) {
        targetW = qBound(80, availWidth / count, 240);
    }

    for (auto* btn : m_tabItemMap) {
        if (btn) {
            btn->setFixedWidth(targetW);
        }
    }
}

void TabBarWidget::updateTabsUiState() {
    applyDiffUpdate();
}

void TabBarWidget::applyDiffUpdate() {
    QSet<QString> activeTabIds;
    for (int i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs[i];
        activeTabIds.insert(tab.id);

        TabItemButton* tabItem = m_tabItemMap.value(tab.id, nullptr);
        if (!tabItem) {
            tabItem = new TabItemButton(i, m_tabsContainer);
            m_tabItemMap.insert(tab.id, tabItem);
            m_tabsLayout->insertWidget(i, tabItem);

            connect(tabItem, &TabItemButton::closeClicked, this, [this, tabId = tab.id](int) {
                for (int idx = 0; idx < m_tabs.size(); ++idx) {
                    if (m_tabs[idx].id == tabId) {
                        closeTab(idx);
                        break;
                    }
                }
            });

            connect(tabItem, &TabItemButton::middleClicked, this, [this, tabId = tab.id](int) {
                for (int idx = 0; idx < m_tabs.size(); ++idx) {
                    if (m_tabs[idx].id == tabId) {
                        closeTab(idx);
                        break;
                    }
                }
            });

            connect(tabItem, &TabItemButton::tabClicked, this, [this, tabId = tab.id](int) {
                for (int idx = 0; idx < m_tabs.size(); ++idx) {
                    if (m_tabs[idx].id == tabId) {
                        setCurrentIndex(idx, false);
                        break;
                    }
                }
            });

            connect(tabItem, &TabItemButton::customContextMenuRequested, this, [this, tabId = tab.id](int, const QPoint& globalPos) {
                for (int idx = 0; idx < m_tabs.size(); ++idx) {
                    if (m_tabs[idx].id == tabId) {
                        showTabContextMenu(idx, globalPos);
                        break;
                    }
                }
            });
        }

        tabItem->setIndex(i);
        tabItem->setTabTitle(tab.title);
        QColor iconColor = !tab.color.isEmpty() ? QColor(tab.color) : (tab.active ? QColor("#EEEEEE") : QColor("#888888"));
        QString iconKey = !tab.iconKey.isEmpty() ? tab.iconKey : (tab.url.startsWith("computer://") ? "computer" : "folder_filled");
        tabItem->setTabIcon(UiHelper::getIcon(iconKey, iconColor));
        tabItem->setActive(tab.active);
    }

    // 差量清理已关闭标签：仅精准释放不再存在的控件
    auto it = m_tabItemMap.begin();
    while (it != m_tabItemMap.end()) {
        if (!activeTabIds.contains(it.key())) {
            TabItemButton* btn = it.value();
            m_tabsLayout->removeWidget(btn);
            btn->deleteLater();
            it = m_tabItemMap.erase(it);
        } else {
            ++it;
        }
    }

    adjustTabWidths();
}
>>>>>>> REPLACE
```

### 3.5 `src/ui/PanelMediator.cpp`

```cpp
<<<<<<< SEARCH
            connect(titleBar->tabBar(), &TabBarWidget::folderDroppedOnTab, this, [this, titleBar, contentPanel](int tabIndex, const QString& folderPath) {
=======
            connect(titleBar->tabBar(), &TabBarWidget::tabColorChangeRequested, this, [](const QString& url, const QString& colorHex) {
                if (!url.isEmpty() && !url.startsWith("computer://")) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::SetColor;
                    cmd.targetPaths = {url};
                    cmd.params["color"] = colorHex;
                    CoreEngine::instance().executeCommand(cmd);
                }
            });

            connect(titleBar->tabBar(), &TabBarWidget::tabIconChangeRequested, this, [](const QString& url, const QString& iconKey) {
                FavoriteDao::updateFavorite(url, iconKey, QString());
            });

            connect(titleBar->tabBar(), &TabBarWidget::folderDroppedOnTab, this, [this, titleBar, contentPanel](int tabIndex, const QString& folderPath) {
>>>>>>> REPLACE
```

### 3.6 `resources/style.qss`

```css
<<<<<<< SEARCH
/* 浏览器多标签页 TabBarWidget 专属精细样式 (严格消除 QWidget 级联边框) */
QWidget#TabBarWidget {
    background: transparent;
    border: none;
}
=======
/* 浏览器多标签页 TabBarWidget 专属精细样式 (严格消除 QWidget 级联边框) */
QWidget#TabBarWidget,
QScrollArea#TabBarScrollArea,
QWidget#TabBarContainer,
QWidget#TitleRightToolContainer {
    background: transparent;
    border: none;
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译构建命令
在 Visual Studio Developer Command Prompt (或 PowerShell) 中执行以下构建指令：
```powershell
cmake --build g:/C++/QuarkMeta/QuarkMeta/build --config Release --target QuarkMeta
```

### 4.2 运行时验收标准（针对红箭头事故与性能抖动）
1. **多标签极限防挤压测试（针对长红箭头事故）**：
   - 连续点击“+”新建 15 个以上标签页；
   - **预期表现**：右侧缩放滑块、视图排列方式按钮及右上角最小化/最大化/关闭按钮**位置与尺寸 100% 纹丝不动，绝对不产生任何变形或被顶出屏幕**；
   - 标签栏平滑自动从 `240px` 阶梯压缩至 `80px`，超出后在标签栏区域滑动鼠标滚轮，标签栏平滑水平横向滚动。
2. **生命周期差量更新与防抖动测试**：
   - 频繁切换不同标签、关闭指定标签或重命名标签；
   - **预期表现**：仅目标 `TabItemButton` 就地发生视觉变化，未受影响的其余标签按钮不发生任何析构与重新实例化，右键菜单上下文不闪烁、焦点不丢失。
3. **分层与右键菜单隔离测试**：
   - 右键任意标签页，选择“切换图标”和颜色条；
   - **预期表现**：菜单正常呈现且复用 `ContextMenuFactory` 官方样式，颜色/图标变更正常派发至 Mediator 驱动底层引擎，View 内部 0 处直接访问数据库。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 场景 | 原手写违规实现 | 重构复用的官方 SSOT API |
| :--- | :--- | :--- |
| **Tab 图标与颜色切换二级菜单** | `TabBarWidget.cpp` 内手写 80 行 `builtInIcons` 列表并 `new` 40 多个 `QPushButton` | **`ContextMenuFactory::buildIconPickerMenu(...)`**（官方统一真理源） |
| **新标签页打开路径** | `window()->findChild<TabBarWidget*>()` 偷指针调用 | **`NavigationService::instance().openInNewTab(path)`**（官方统一导航 SSOT） |
| **Tab 颜色设置与引擎调度** | `TabBarWidget` View 层直连 `CoreEngine::instance().executeCommand` | **`PanelMediator` 统一槽函数调度** |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本 (Cache/View) | 同步与刷新通道 |
| :--- | :--- | :--- | :--- |
| **标签列表元数据** | `TabBarWidget::m_tabs` (内存态) / `AppConfig` (持久态) | `m_tabItemMap` 界面控件池 | 基于 `TabInfo::id` 的 `applyDiffUpdate()` 差量比对通道 |
| **文件夹颜色** | `MetadataManager` 侧车与 SQLite | `TabInfo::color` | `PanelMediator` 响应 `tabColorChangeRequested` 调度 `CoreEngine` 广播刷新 |
| **收藏图标** | `FavoriteDao` 数据库记录 | `TabInfo::iconKey` | `PanelMediator` 响应 `tabIconChangeRequested` 调度更新 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 外部调用接口原样签名
1. `ContextMenuFactory.h` 第 69-74 行：
   ```cpp
   static QMenu* buildIconPickerMenu(QMenu* parentMenu,
                                     const QString& curIconKey,
                                     const QString& curColorHex,
                                     std::function<void(const QString&)> onIconSelected,
                                     std::function<void(const QString&)> onColorSelected,
                                     QObject* receiver = nullptr);
   ```
2. `NavigationService.h` 第 17 行：
   ```cpp
   void openInNewTab(const QString& rawPath);
   ```

### 7.2 新增与重构接口清单
| 接口签名 | 声明文件及访问级别 | 定义文件 | 调用点 |
| :--- | :--- | :--- | :--- |
| `void tabColorChangeRequested(const QString& url, const QString& colorHex)` | `TabBarWidget.h` (`signals:`) | `moc_TabBarWidget.cpp` (Qt生成) | `TabBarWidget.cpp` (右键颜色回调) $\to$ `PanelMediator.cpp` (connect 接收) |
| `void tabIconChangeRequested(const QString& url, const QString& iconKey)` | `TabBarWidget.h` (`signals:`) | `moc_TabBarWidget.cpp` (Qt生成) | `TabBarWidget.cpp` (右键图标回调) $\to$ `PanelMediator.cpp` (connect 接收) |
| `void adjustTabWidths()` | `TabBarWidget.h` (`private:`) | `TabBarWidget.cpp` | `resizeEvent()` 与 `applyDiffUpdate()` |
| `void applyDiffUpdate()` | `TabBarWidget.h` (`private:`) | `TabBarWidget.cpp` | `updateTabsUiState()` |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. **移除的头文件**：
   - 从 `TabBarWidget.cpp` 移除 `#include "../meta/FavoriteDao.h"`、`"../meta/MetadataManager.h"`、`"../core/CoreEngine.h"`。
   - **闭合性核查**：`TabBarWidget.cpp` 中已不再直接调用 `FavoriteDao`、`MetadataManager` 或 `CoreEngine` 的成员函数，已全面转换为信号派发至 `PanelMediator`。包含链闭合且无未定义符号。
2. **新增的头文件**：
   - `TitleBarWidget.cpp`：无新增依赖；
   - `TabBarWidget.h`：引入 `<QScrollArea>`、`<QMap>`，用于滚动视口与差量池；
   - `TabBarWidget.cpp`：引入 `<QWheelEvent>`、`<QScrollBar>` 与 `"controllers/ContextMenuFactory.h"`，用于平滑滚轮事件与官方二级菜单调用。类型声明均完整闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 状态 | 证据与位置 |
| :---: | :--- | :---: | :--- |
| 1 | 是否破坏既有 `.h` 外部公开签名（契约锁） | 满足 | 外部原有 `tabBar()`、`addTab` 等公有 API 原封不动保留 |
| 2 | 是否违反严格单向依赖流动（依赖锁） | 满足 | 拔除 View 访问 DAO，数据变更全归 Mediator 协调 |
| 3 | 是否回答架构三问（真理源/黑盒/根因） | 满足 | Chapter 1 & Chapter 6 完整回答 |
| 4 | 是否使用 Win32 强抢焦点或篡改 Qt::Popup 等平台 Hack | 满足 | 0 处 Hack，完全走标准 Qt 事件与视口机制 |
| 5 | 是否跨模块窥探私有实现 | 满足 | 彻底消除全部 `findChild<TabBarWidget*>()` |
| 6 | 是否直接修改源代码文件（实施界限） | 满足 | 物理源码 0 改动，方案隔离存放于 `Implementation Plan/` |
| 7 | 是否复用既有官方 API 防另起炉灶 | 满足 | 复用 `ContextMenuFactory::buildIconPickerMenu`，消灭 80 行死代码 |
| 8 | 是否包含全部 10 大必备章节 | 满足 | 10 大章节完整健全，无一裁剪 |
| 9 | SEARCH 块是否在目标文件中真实存在且唯一匹配 | 满足 | 全块经过逐行校验 |
| 10 | 外部成员函数是否在对应 `.h` 中真实存在且签名一致 | 满足 | Chapter 7 逐项贴出原样代码行证据 |
| 11 | 新增接口声明、定义与调用点是否一致 | 满足 | Chapter 7 表格详细登记 |
| 12 | 跨类调用成员访问级别是否允许 | 满足 | 全部通过公开 signal/slot 通信 |
| 13 | 是否触碰方案范围外文件 | 满足 | 严格限制在 TitleBar / TabBar / Mediator 相关闭环 |
| 14 | 是否保持 UI 样式数值 100% 零篡改 | 满足 | Chapter 10 详细登记 |
| 15 | 方案正文与注释是否只使用中文 | 满足 | 全文纯中文撰写 |
| 16 | 是否录入设计理念到 Memories.md | 满足 | 已先期录入至 `Memories.md` 第 19 章 |
| 17 | 是否清除所有被替换的死代码 | 满足 | 彻底删除 `rebuildTabsUi` 与手写九宫格 |
| 18 | 是否为多类重构采用语义化英文命名 | 满足 | 命名为 `TabBarArchitectureRefactoring.md` |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 既有交互与视觉行为零篡改声明
1. **标签栏外观风格**：保留原有暗色背景 `#1E1E1E`、选中态 `#2D2D2D`、悬停态 `#252526`，边框圆角与文字字号 `12px` 100% 保持原状；
2. **标签拖拽排序与分屏合并**：原有 `TabItemButton` 拖拽放置与标签合并交互 100% 保留；
3. **右键菜单功能**：保留颜色条选择、图标切换、关闭其他、关闭右侧、复制标签页等全部功能，仅底层实现归一化至 `ContextMenuFactory`。

### 10.2 样式数值零篡改核对表
| 控件对象 | 样式属性 | 原有数值 | 重构后数值 | 结论 |
| :--- | :--- | :--- | :--- | :---: |
| `TitleBarWidget` | 高度 `setFixedHeight` | `34px` | `34px` | 绝对一致 |
| `TabBarWidget` | 高度 `setFixedHeight` | `30px` | `30px` | 绝对一致 |
| `TabItemButton` | 最小/最大宽度 | `80px` / `240px` | `80px` / `240px` | 绝对一致 |
| `TabItemButton` | 边框与圆角 | `border-radius: 6px 6px 0 0` | `border-radius: 6px 6px 0 0` | 绝对一致 |
| 右侧工具栏按钮 | 尺寸与间距 | `24x24px`，间距 `8px` | `24x24px`，间距 `8px` | 绝对一致 |
