# 标题栏浏览器多标签页重构与 "QuarkMeta" 移除及快捷键扩充实施方案 (TabBarWidget.md)

## 1. 架构三问 (The 3-Question Root-Cause Gate)

1. **第一问（真理源溯源）**：
   - 多标签页（Tab Bar）的状态SSOT权威源是谁？
   - 答：标签页的集合、当前活动标签（Active Tab Index）及各标签关联的 URL/路径，由新建的 `TabBarWidget` 作为控制与视图容器进行维护，并与全局 `NavigationService::instance()` 保持单向数据流沟通。当标签页切换或新建时，通过 `NavigationService::instance().navigateTo(url)` 驱动主窗口内容同步。

2. **第二问（黑盒完整性）**：
   - 该修改是否破坏了调用方或被调用方的封装？
   - 答：未破坏。`TitleBarWidget` 仅将 `m_appNameLabel` 移除，并引入 `TabBarWidget` 嵌入自身 `m_layout` 中。快捷键由 `AppShortcutController` 统一处理，`TitleBarWidget` 对外通过 Getter/Signal 暴露事件，且保持既有 `.h` 公开签名向下兼容，符合【契约锁】。

3. **第三问（根因 vs 症状）**：
   - 为什么需要移除 "QuarkMeta" 并引入多标签页及 Ctrl+T/Ctrl+Shift+T 快捷键？
   - 答：用户需求提升顶部标题栏的空间利用率，去除静态的软件名称文本，转而提供类似 Google Chrome 浏览器的多标签页沉浸式管理能力（默认标签页设为“此电脑 `computer://`”），并配备桌面级快捷键流（`Ctrl+T` 新建标签，`Ctrl+Shift+T` 恢复上一次关闭的标签）。

---

## 2. Overview（概述与解决的问题）

1. **移除静态 "QuarkMeta" 文本**：
   - 从 `TitleBarWidget` 中物理移除 `m_appNameLabel`（`QLabel`），释放顶部标题栏左侧空间。
2. **新增 Chrome 风格暗色多标签页组件 (`TabBarWidget`)**：
   - 创建 `src/ui/TabBarWidget.h` 和 `src/ui/TabBarWidget.cpp`；
   - 包含标准 Chrome 暗色卡片外观标签、标签图标、标签名称、标签关闭按钮 (`×`) 以及右侧新建标签按钮 (`+`)；
   - 重写 `eventFilter` 以响应鼠标点击标签页进行焦点与视角切换；
   - 包含 `#include <QDateTime>` 防止符号丢失；
   - 精准修正关闭左侧标签时的 `m_currentIndex` 偏移问题，避免当前活动标签错位；
   - 增加支持恢复上一次关闭标签页的历史栈 (`m_closedTabsHistory`)，支持 `restoreLastClosedTab()`；
   - 默认第一标签初始化为“此电脑 (`computer://`)”；
   - 在 `PanelMediator.cpp` 中将 `TabBarWidget` 的标签切换信号与 `NavigationService::instance()` 进行无缝桥接绑定。
3. **新增快捷键 `Ctrl+T` 与 `Ctrl+Shift+T` 支持**：
   - 在 `AppShortcutController.cpp` 中注册 `Ctrl+T`（新建标签页）与 `Ctrl+Shift+T`（恢复关闭的标签页）快捷键，绑定到 `TabBarWidget` 对应槽函数。
4. **注册 CMake 构建**：
   - 在 `CMakeLists.txt` 的 `SOURCES` 列表中明确增加 `src/ui/TabBarWidget.h` 与 `src/ui/TabBarWidget.cpp`。

---

## 3. Modified Files List（影响文件清单）

1. `CMakeLists.txt`
2. `src/ui/TabBarWidget.h` (新建)
3. `src/ui/TabBarWidget.cpp` (新建)
4. `src/ui/TitleBarWidget.h`
5. `src/ui/TitleBarWidget.cpp`
6. `src/ui/PanelMediator.cpp`
7. `src/ui/AppShortcutController.cpp`

---

## 4. Detailed Line-by-Line Changes（精准替换块）

### 4.1 `CMakeLists.txt` 注册新组件
```
<<<<<<< SEARCH
    src/ui/NavBarWidget.h
    src/ui/NavBarWidget.cpp
    src/ui/TitleBarWidget.h
    src/ui/TitleBarWidget.cpp
=======
    src/ui/NavBarWidget.h
    src/ui/NavBarWidget.cpp
    src/ui/TabBarWidget.h
    src/ui/TabBarWidget.cpp
    src/ui/TitleBarWidget.h
    src/ui/TitleBarWidget.cpp
>>>>>>> REPLACE
```

---

### 4.2 新建 `src/ui/TabBarWidget.h`
```
<<<<<<< SEARCH
=======
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
>>>>>>> REPLACE
```

---

### 4.3 新建 `src/ui/TabBarWidget.cpp`
```
<<<<<<< SEARCH
=======
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
        iconLabel->setFixedSize(14, 14);
        iconLabel->setPixmap(UiHelper::getIcon(tab.url.startsWith("computer://") ? "computer" : "folder_filled", 
                                                tab.active ? QColor("#EEEEEE") : QColor("#888888")).pixmap(14, 14));

        QLabel* titleLabel = new QLabel(tab.title, tabItem);
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
>>>>>>> REPLACE
```

---

### 4.4 `TitleBarWidget.h` 修改
```
<<<<<<< SEARCH
    QLabel* m_logoLabel = nullptr;
    QLabel* m_appNameLabel = nullptr;

    QPushButton* m_btnViewMenu = nullptr;
=======
    TabBarWidget* tabBar() const { return m_tabBar; }

    QLabel* m_logoLabel = nullptr;
    TabBarWidget* m_tabBar = nullptr;

    QPushButton* m_btnViewMenu = nullptr;
>>>>>>> REPLACE
```

---

### 4.5 `TitleBarWidget.cpp` 修改
```
<<<<<<< SEARCH
#include "TitleBarWidget.h"
#include "UiHelper.h"
=======
#include "TitleBarWidget.h"
#include "TabBarWidget.h"
#include "UiHelper.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_appNameLabel = new QLabel("QuarkMeta", this);
    m_appNameLabel->setObjectName("AppNameLabel");
    m_layout->addWidget(m_appNameLabel);
    m_layout->addStretch();
=======
    m_tabBar = new TabBarWidget(this);
    m_layout->addWidget(m_tabBar);
    m_layout->addStretch();
>>>>>>> REPLACE
```

---

### 4.6 `PanelMediator.cpp` 关联事件与导航驱动
```
<<<<<<< SEARCH
    if (m_titleBar && m_layoutManager) {
        TitleBarWidget* titleBar = m_titleBar;
=======
    if (m_titleBar && m_layoutManager) {
        TitleBarWidget* titleBar = m_titleBar;

        if (titleBar->tabBar()) {
            connect(titleBar->tabBar(), &TabBarWidget::currentTabChanged, this, [](int index, const QString& url) {
                Q_UNUSED(index);
                NavigationService::instance().navigateTo(url);
            });
            connect(&NavigationService::instance(), &NavigationService::currentUrlChanged, this, [titleBar](const QString& url, const QString& displayPath) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->updateCurrentTabTitle(displayPath, url);
                }
            });
        }
>>>>>>> REPLACE
```

---

### 4.7 `AppShortcutController.cpp` 注册 `Ctrl+T` 与 `Ctrl+Shift+T` 快捷键
```
<<<<<<< SEARCH
    // 6. Ctrl+W: 全局关闭/响应式退出主窗口
    QShortcut* scClose = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), m_window);
    scClose->setContext(Qt::WindowShortcut);
    connect(scClose, &QShortcut::activated, this, [this]() {
        if (m_window) {
            m_window->close();
        }
    });
=======
    // 6. Ctrl+W: 全局关闭/响应式退出主窗口
    QShortcut* scClose = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), m_window);
    scClose->setContext(Qt::WindowShortcut);
    connect(scClose, &QShortcut::activated, this, [this]() {
        if (m_window) {
            m_window->close();
        }
    });

    // 7. Ctrl+T: 新建标签页
    QShortcut* scNewTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), m_window);
    scNewTab->setContext(Qt::WindowShortcut);
    connect(scNewTab, &QShortcut::activated, this, [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->addTab("此电脑", "computer://", true);
                }
            }
        }
    });

    // 8. Ctrl+Shift+T: 恢复关闭的标签页
    QShortcut* scRestoreTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), m_window);
    scRestoreTab->setContext(Qt::WindowShortcut);
    connect(scRestoreTab, &QShortcut::activated, this, [this]() {
        if (m_window) {
            if (auto titleBar = m_window->findChild<TitleBarWidget*>()) {
                if (titleBar->tabBar()) {
                    titleBar->tabBar()->restoreLastClosedTab();
                }
            }
        }
    });
>>>>>>> REPLACE
```

---

## 5. Build & Verification Steps（编译命令与验证方法）

1. **构建流程**：
   在 MSVC 64-bit 环境或沙盒环境中，运行：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```
2. **功能验证步骤**：
   - **移除验证**：启动程序后核查顶部标题栏，确认静态文本 `"QuarkMeta"` 已彻底消失。
   - **标签页展现**：确认顶部标题栏展示类似 Chrome 的暗色标签页组件，且首个默认标签页显示为“此电脑”。
   - **新建标签与快捷键 `Ctrl+T`**：按下 `Ctrl+T` 或点击标签页右侧的 `+` 按钮，确认成功新建一个“此电脑”标签页。
   - **恢复标签与快捷键 `Ctrl+Shift+T`**：关闭某标签后按下 `Ctrl+Shift+T`，确认成功恢复最近一次关闭的标签页。
   - **切换标签**：鼠标左键点击不同标签页，触发 `eventFilter` 捕获事件，驱动 `NavigationService::instance().navigateTo(url)` 进行界面视图导航联动。
   - **关闭标签**：点击标签上的 `×` 按钮，确认标签平滑关闭且活动状态自动接替。

---

## 6. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- **SSOT API 复用说明**：
  新添加的标签页在切换或新建时，统一通过 `NavigationService::instance().navigateTo(...)` 通道驱动全局视图与路径变更，彻底禁止私自调用 `loadDirectory` 另起炉灶。

---

## 7. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用类与方法签名 | 头文件物理源 | 核查状态 |
| :--- | :--- | :--- |
| `NavigationService::instance()` | `src/core/NavigationService.h` | 物理存在 100% 匹配 |
| `NavigationService::navigateTo(const QString&, bool)` | `src/core/NavigationService.h` | 物理存在 100% 匹配 |
| `NavigationService::currentUrlChanged(const QString&, const QString&)` | `src/core/NavigationService.h` | 物理存在 100% 匹配 |
| `UiHelper::getIcon(const QString&, const QColor&)` | `src/ui/UiHelper.h` | 物理存在 100% 匹配 |
