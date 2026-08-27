# QuarkMeta 多栏布局与显隐管理器实施方案 (layout.md)

## 1. Overview
本方案旨在解决 `MainWindow.cpp` 中混杂 `QSplitter` 分栏比例恢复、230px 比例重置、五栏显隐状态持久化及动态最小宽度计算的问题。通过抽离并新建 `PanelLayoutManager` 布局控制类，彻底将分栏空间管理与显隐控制收拢至独立模块，使 `MainWindow` 仅充当 Shell 装配壳体。

---

## 2. Modified Files List
1. **新建** `src/ui/PanelLayoutManager.h`
2. **新建** `src/ui/PanelLayoutManager.cpp`
3. **修改** `src/ui/MainWindow.h`
4. **修改** `src/ui/MainWindow.cpp`
5. **修改** `CMakeLists.txt`

---

## 3. Detailed Line-by-Line Changes

### 3.1 新建 `src/ui/PanelLayoutManager.h`
```cpp
#pragma once

#include <QObject>
#include <QSplitter>
#include <QMainWindow>
#include <QMenu>
#include <QPoint>
#include <QPointer>

namespace QuarkMeta {

class NavPanel;
class FavoritePanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;

class PanelLayoutManager : public QObject {
    Q_OBJECT

public:
    explicit PanelLayoutManager(QMainWindow* mainWindow,
                                QSplitter* mainSplitter,
                                NavPanel* navPanel,
                                FavoritePanel* favoritePanel,
                                ContentPanel* contentPanel,
                                MetaPanel* metaPanel,
                                FilterPanel* filterPanel,
                                QObject* parent = nullptr);
    ~PanelLayoutManager() override = default;

    void initLayout();
    void resetSplitterLayout();
    void setPanelVisible(const QString& panelId, bool visible);
    bool isPanelVisible(const QString& panelId) const;
    void populatePanelMenu(QMenu* menu);
    void showPanelContextMenu(const QPoint& globalPos);
    void updateDynamicMinimumSize();
    void saveLayoutState();
    void setTagManagerMode(bool isMode) { m_isTagManagerMode = isMode; }

signals:
    void layoutResetCompleted();
    void panelVisibilityChanged(const QString& panelId, bool visible);

private:
    void loadPanelVisibility();

    QPointer<QMainWindow> m_mainWindow;
    QPointer<QSplitter> m_mainSplitter;

    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;

    bool m_isTagManagerMode = false;
    static constexpr int kBasePanelWidth = 230;
    static constexpr int kContentBaseWidth = 550;
    static constexpr int kSplitterHandleWidth = 5;
};

} // namespace QuarkMeta
```

### 3.2 新建 `src/ui/PanelLayoutManager.cpp`
```cpp
#include "PanelLayoutManager.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "../core/AppConfig.h"
#include <QAction>
#include <QCursor>
#include <QTimer>
#include <QList>
#include <QStringList>

namespace QuarkMeta {

PanelLayoutManager::PanelLayoutManager(QMainWindow* mainWindow,
                                       QSplitter* mainSplitter,
                                       NavPanel* navPanel,
                                       FavoritePanel* favoritePanel,
                                       ContentPanel* contentPanel,
                                       MetaPanel* metaPanel,
                                       FilterPanel* filterPanel,
                                       QObject* parent)
    : QObject(parent),
      m_mainWindow(mainWindow),
      m_mainSplitter(mainSplitter),
      m_navPanel(navPanel),
      m_favoritePanel(favoritePanel),
      m_contentPanel(contentPanel),
      m_metaPanel(metaPanel),
      m_filterPanel(filterPanel) {
}

void PanelLayoutManager::initLayout() {
    if (!m_mainSplitter) return;

    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setStretchFactor(2, 1);
    m_mainSplitter->setStretchFactor(3, 0);
    m_mainSplitter->setStretchFactor(4, 0);

    loadPanelVisibility();

    QByteArray state = AppConfig::instance().getValue("MainWindow/SplitterState").toByteArray();
    if (!state.isEmpty()) {
        QTimer::singleShot(0, this, [this, state]() {
            if (m_mainSplitter) {
                m_mainSplitter->restoreState(state);
            }
        });
    } else {
        QList<int> sizes;
        sizes << kBasePanelWidth << kBasePanelWidth << kContentBaseWidth << kBasePanelWidth << kBasePanelWidth;
        m_mainSplitter->setSizes(sizes);
    }

    updateDynamicMinimumSize();
}

void PanelLayoutManager::resetSplitterLayout() {
    if (!m_mainSplitter) return;

    m_isTagManagerMode = false;

    if (m_navPanel) m_navPanel->show();
    if (m_favoritePanel) m_favoritePanel->show();
    if (m_contentPanel) m_contentPanel->show();
    if (m_metaPanel) m_metaPanel->show();
    if (m_filterPanel) m_filterPanel->show();

    QList<int> sizes;
    sizes << kBasePanelWidth << kBasePanelWidth << kContentBaseWidth << kBasePanelWidth << kBasePanelWidth;
    m_mainSplitter->setSizes(sizes);

    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setStretchFactor(2, 1);
    m_mainSplitter->setStretchFactor(3, 0);
    m_mainSplitter->setStretchFactor(4, 0);

    AppConfig::instance().remove("MainWindow/SplitterState");
    AppConfig::instance().remove("MainWindow/PanelVisibility");
    AppConfig::instance().sync();

    ToolTipOverlay::instance()->showText(QCursor::pos(), "分栏布局已重置为默认值", 1500, QColor("#2ecc71"));
    updateDynamicMinimumSize();
    emit layoutResetCompleted();
}

void PanelLayoutManager::setPanelVisible(const QString& panelId, bool visible) {
    if (panelId == "nav" && m_navPanel) m_navPanel->setVisible(visible);
    else if (panelId == "favorite" && m_favoritePanel) m_favoritePanel->setVisible(visible);
    else if (panelId == "content" && m_contentPanel) m_contentPanel->setVisible(true);
    else if (panelId == "meta" && m_metaPanel) m_metaPanel->setVisible(visible);
    else if (panelId == "filter" && m_filterPanel) m_filterPanel->setVisible(visible);

    updateDynamicMinimumSize();
    emit panelVisibilityChanged(panelId, visible);
}

bool PanelLayoutManager::isPanelVisible(const QString& panelId) const {
    if (panelId == "nav" && m_navPanel) return m_navPanel->isVisible();
    if (panelId == "favorite" && m_favoritePanel) return m_favoritePanel->isVisible();
    if (panelId == "content" && m_contentPanel) return m_contentPanel->isVisible();
    if (panelId == "meta" && m_metaPanel) return m_metaPanel->isVisible();
    if (panelId == "filter" && m_filterPanel) return m_filterPanel->isVisible();
    return false;
}

void PanelLayoutManager::populatePanelMenu(QMenu* menu) {
    if (!menu) return;

    auto addToggleAction = [this, menu](const QString& text, const QString& panelId, QWidget* panel, bool canHide = true) {
        if (!panel) return;
        QAction* action = menu->addAction(text);
        action->setCheckable(true);
        action->setChecked(panel->isVisible());
        action->setEnabled(canHide);

        connect(action, &QAction::toggled, this, [this, panelId](bool visible) {
            setPanelVisible(panelId, visible);
        });
    };

    addToggleAction("显示目录导航", "nav", m_navPanel);
    addToggleAction("显示收藏夹", "favorite", m_favoritePanel);
    addToggleAction("显示内容区", "content", m_contentPanel, false);
    addToggleAction("显示元数据栏", "meta", m_metaPanel);
    addToggleAction("显示筛选栏", "filter", m_filterPanel);

    menu->addSeparator();
    QAction* resetAct = menu->addAction("重置分栏");
    connect(resetAct, &QAction::triggered, this, &PanelLayoutManager::resetSplitterLayout);
}

void PanelLayoutManager::showPanelContextMenu(const QPoint& globalPos) {
    QMenu menu;
    UiHelper::applyMenuStyle(&menu);
    populatePanelMenu(&menu);
    menu.exec(globalPos);
}

void PanelLayoutManager::updateDynamicMinimumSize() {
    if (!m_mainWindow) return;

    int visibleCount = 0;
    if (m_navPanel && m_navPanel->isVisible()) visibleCount++;
    if (m_favoritePanel && m_favoritePanel->isVisible()) visibleCount++;
    if (m_contentPanel && m_contentPanel->isVisible()) visibleCount++;
    if (m_metaPanel && m_metaPanel->isVisible()) visibleCount++;
    if (m_filterPanel && m_filterPanel->isVisible()) visibleCount++;

    if (visibleCount <= 0) visibleCount = 1;

    int calculatedMinW = (visibleCount * kBasePanelWidth) + ((visibleCount - 1) * kSplitterHandleWidth) + 10;
    int finalMinW = qMax(465, calculatedMinW);

    m_mainWindow->setMinimumWidth(finalMinW);
}

void PanelLayoutManager::loadPanelVisibility() {
    QVariant val = AppConfig::instance().getValue("MainWindow/PanelVisibility");
    if (val.isValid()) {
        QStringList hiddenPanels = val.toStringList();
        if (hiddenPanels.contains("nav") && m_navPanel)           m_navPanel->hide();
        if (hiddenPanels.contains("favorite") && m_favoritePanel) m_favoritePanel->hide();
        if (hiddenPanels.contains("meta") && m_metaPanel)         m_metaPanel->hide();
        if (hiddenPanels.contains("filter") && m_filterPanel)     m_filterPanel->hide();
    }
    updateDynamicMinimumSize();
}

void PanelLayoutManager::saveLayoutState() {
    if (m_isTagManagerMode) return;

    if (m_mainSplitter) {
        AppConfig::instance().setValue("MainWindow/SplitterState", m_mainSplitter->saveState());
    }

    QStringList hiddenPanels;
    if (m_navPanel && !m_navPanel->isVisible())           hiddenPanels << "nav";
    if (m_favoritePanel && !m_favoritePanel->isVisible()) hiddenPanels << "favorite";
    if (m_metaPanel && !m_metaPanel->isVisible())         hiddenPanels << "meta";
    if (m_filterPanel && !m_filterPanel->isVisible())     hiddenPanels << "filter";

    AppConfig::instance().setValue("MainWindow/PanelVisibility", hiddenPanels);
    AppConfig::instance().sync();
}

} // namespace QuarkMeta
```

---

### 3.3 `src/ui/MainWindow.h` 净化

```cpp
<<<<<<< SEARCH
class MainWindow : public QMainWindow {
=======
class PanelLayoutManager;

class MainWindow : public QMainWindow {
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
=======
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    PanelLayoutManager* layoutManager() const { return m_panelLayoutManager; }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    PanelLayoutManager* m_panelLayoutManager = nullptr;
=======
    PanelLayoutManager* m_panelLayoutManager = nullptr;
>>>>>>> REPLACE
```

---

### 3.4 `src/ui/MainWindow.cpp` 净化与接入

```cpp
<<<<<<< SEARCH
#include "FramelessWindowHelper.h"
=======
#include "FramelessWindowHelper.h"
#include "PanelLayoutManager.h"
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    m_mainSplitter->addWidget(m_navPanel);
    m_mainSplitter->addWidget(m_favoritePanel);
    m_mainSplitter->addWidget(m_contentPanel);
    m_mainSplitter->addWidget(m_metaPanel);
    m_mainSplitter->addWidget(m_filterPanel);

    setupCustomTitleBarButtons();
=======
    m_mainSplitter->addWidget(m_navPanel);
    m_mainSplitter->addWidget(m_favoritePanel);
    m_mainSplitter->addWidget(m_contentPanel);
    m_mainSplitter->addWidget(m_metaPanel);
    m_mainSplitter->addWidget(m_filterPanel);

    m_panelLayoutManager = new PanelLayoutManager(
        this, m_mainSplitter,
        m_navPanel, m_favoritePanel, m_contentPanel, m_metaPanel, m_filterPanel,
        this
    );
    m_panelLayoutManager->initLayout();

    setupCustomTitleBarButtons();
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    m_btnLayout = createTitleBtn("layout");
    m_btnLayout->setProperty("tooltipText", "布局管理与重置");
    m_btnLayout->installEventFilter(m_hoverFilter);
=======
    m_btnLayout = createTitleBtn("layout");
    m_btnLayout->setProperty("tooltipText", "布局管理与重置");
    m_btnLayout->installEventFilter(m_hoverFilter);
    connect(m_btnLayout, &QPushButton::clicked, this, [this]() {
        if (m_panelLayoutManager) {
            m_panelLayoutManager->showPanelContextMenu(m_btnLayout->mapToGlobal(QPoint(0, m_btnLayout->height())));
        }
    });
>>>>>>> REPLACE
```

---

### 3.5 `CMakeLists.txt` 构建注册

```cmake
<<<<<<< SEARCH
    src/ui/FramelessWindowHelper.cpp
    src/ui/FramelessWindowHelper.h
=======
    src/ui/FramelessWindowHelper.cpp
    src/ui/FramelessWindowHelper.h
    src/ui/PanelLayoutManager.cpp
    src/ui/PanelLayoutManager.h
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **构建与 MOC 注册验证**：
   - 验证 `PanelLayoutManager.h/cpp` 被 CMake 正确捕获并参与 MOC 编译。
   - 验证无未解析的成员方法链接错误。

2. **布局恢复与重置测试**：
   - **230px 黄金比例重置**：触发“重置分栏”菜单项，验证所有面板均重置为 230px 宽度。
   - **面板显隐与不可隐藏防护**：测试隐藏/显示各个侧栏面板，确认“显示内容区”无法被取消勾选。
   - **最小安全宽度重算**：隐藏面板时自动缩小 `minimumWidth`，显示面板时平滑扩展。
