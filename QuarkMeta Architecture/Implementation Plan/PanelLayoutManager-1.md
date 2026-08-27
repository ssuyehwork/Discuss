# QuarkMeta 多栏布局与显隐管理器实施方案 (PanelLayoutManager)

## 1. 目标与范围
- 新建 `PanelLayoutManager`：将五栏分割条（`QSplitter`）比例管理、5 个子面板显隐控制、230px 比例重置（`230 << 230 << 550 << 230 << 230`）、窗口动态最小宽度计算、`AppConfig` 布局持久化及布局右键菜单构建**100% 独立模块化**。
- 彻底净化 `MainWindow.h/cpp`：清除 `loadPanelVisibility`、`savePanelVisibility`、`updateDynamicMinimumSize`、`resetSplitterLayout`、`showPanelContextMenu`、`populatePanelMenu` 等非窗口外壳代码，主窗口仅持有 `PanelLayoutManager` 并进行装配。

---

## 2. 新增模块设计与完整代码实现

### 2.1 `src/ui/PanelLayoutManager.h`
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

    /**
     * @brief 初始化并恢复布局状态 (拉伸比、分栏尺寸与显隐配置)
     */
    void initLayout();

    /**
     * @brief 一键重置为 230px 默认黄金分栏比例
     */
    void resetSplitterLayout();

    /**
     * @brief 切换指定面板的显示/隐藏状态
     */
    void setPanelVisible(const QString& panelId, bool visible);
    bool isPanelVisible(const QString& panelId) const;

    /**
     * @brief 为指定菜单填充五栏显隐与重置选项
     */
    void populatePanelMenu(QMenu* menu);

    /**
     * @brief 在全局指定坐标呼出布局管理上下文菜单
     */
    void showPanelContextMenu(const QPoint& globalPos);

    /**
     * @brief 动态计算并更新主窗口的最小安全宽度
     */
    void updateDynamicMinimumSize();

    /**
     * @brief 将当前分栏尺寸与面板显隐持久化至 AppConfig
     */
    void saveLayoutState();

    /**
     * @brief 设置是否处于标签管理等临时全屏模式 (锁定布局保存)
     */
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

### 2.2 `src/ui/PanelLayoutManager.cpp`
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

    // 1. 设置标准拉伸权重：内容面板独占弹性伸缩权重 1，侧边栏保持 0
    m_mainSplitter->setStretchFactor(0, 0); // 目录导航
    m_mainSplitter->setStretchFactor(1, 0); // 收藏夹
    m_mainSplitter->setStretchFactor(2, 1); // 内容区 (弹性拉伸)
    m_mainSplitter->setStretchFactor(3, 0); // 元数据栏
    m_mainSplitter->setStretchFactor(4, 0); // 筛选栏

    // 2. 加载面板显隐状态
    loadPanelVisibility();

    // 3. 恢复分栏历史尺寸
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

    // 恢复所有面板显示
    if (m_navPanel) m_navPanel->show();
    if (m_favoritePanel) m_favoritePanel->show();
    if (m_contentPanel) m_contentPanel->show();
    if (m_metaPanel) m_metaPanel->show();
    if (m_filterPanel) m_filterPanel->show();

    // 重置 230px 黄金比例尺寸
    QList<int> sizes;
    sizes << kBasePanelWidth << kBasePanelWidth << kContentBaseWidth << kBasePanelWidth << kBasePanelWidth;
    m_mainSplitter->setSizes(sizes);

    // 重置拉伸权重
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setStretchFactor(2, 1);
    m_mainSplitter->setStretchFactor(3, 0);
    m_mainSplitter->setStretchFactor(4, 0);

    // 清理脏配置并同步
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
    else if (panelId == "content" && m_contentPanel) m_contentPanel->setVisible(true); // 物理强制：内容区不可隐藏
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
    addToggleAction("显示内容区", "content", m_contentPanel, false); // 内容区锁定勾选
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

    // 动态边界数学计算：每个可见面板保底 230px + 分割条把手宽度 + 左右安全留白 10px
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

## 3. `MainWindow.h` 与 `MainWindow.cpp` 净化与接入改造

### 3.1 `MainWindow.h` 净化
- 删除 `loadPanelVisibility`、`savePanelVisibility`、`updateDynamicMinimumSize`、`resetSplitterLayout`、`showPanelContextMenu`、`populatePanelMenu` 声明。
- 持有 `PanelLayoutManager* m_panelLayoutManager = nullptr;`。

```cpp
// MainWindow.h 净化后关键区段：
#pragma once

#include <QMainWindow>
#include <QPointer>

namespace QuarkMeta {

class PanelLayoutManager; // 前置声明
class NavPanel;
class FavoritePanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    PanelLayoutManager* layoutManager() const { return m_panelLayoutManager; }

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void initUi();
    void setupSplitters();
    void setupCustomTitleBarButtons();

    // 纯 UI 容器组件
    QSplitter* m_mainSplitter = nullptr;
    NavPanel* m_navPanel = nullptr;
    FavoritePanel* m_favoritePanel = nullptr;
    ContentPanel* m_contentPanel = nullptr;
    MetaPanel* m_metaPanel = nullptr;
    FilterPanel* m_filterPanel = nullptr;

    // 独立多栏布局管理器
    PanelLayoutManager* m_panelLayoutManager = nullptr;

    // 🚨 彻底删除 loadPanelVisibility, savePanelVisibility, updateDynamicMinimumSize, resetSplitterLayout!
};

} // namespace QuarkMeta
```

### 3.2 `MainWindow.cpp` 改造
- 在 `setupSplitters()` 中组装完面板后，**实例化并初始化 `m_panelLayoutManager`**。
- 标题栏“布局管理”按钮与全局右键菜单直接调用 `m_panelLayoutManager`。
- 在 `closeEvent` 中调用 `m_panelLayoutManager->saveLayoutState()`。

```cpp
// MainWindow.cpp 改造关键区段：

void MainWindow::setupSplitters() {
    // ... [创建 centralWidget, titleBar, navBar, mainSplitter 保持不变] ...

    m_navPanel = new NavPanel(this);
    m_navPanel->setObjectName("SidebarContainer");

    m_favoritePanel = new FavoritePanel(this);
    m_favoritePanel->setObjectName("FavoriteContainer");
    
    m_contentPanel = new ContentPanel(this);
    m_contentPanel->setObjectName("EditorContainer");
    
    m_metaPanel = new MetaPanel(this);
    m_metaPanel->setObjectName("MetadataContainer");
    
    m_filterPanel = new FilterPanel(this);
    m_filterPanel->setObjectName("FilterContainer");

    m_mainSplitter->addWidget(m_navPanel);
    m_mainSplitter->addWidget(m_favoritePanel);
    m_mainSplitter->addWidget(m_contentPanel);
    m_mainSplitter->addWidget(m_metaPanel);
    m_mainSplitter->addWidget(m_filterPanel);

    // 🚀【核心模块化接入】：实例化 PanelLayoutManager 统一接管五栏空间生命周期！
    m_panelLayoutManager = new PanelLayoutManager(
        this, m_mainSplitter,
        m_navPanel, m_favoritePanel, m_contentPanel, m_metaPanel, m_filterPanel,
        this
    );
    m_panelLayoutManager->initLayout();

    // ... [状态栏与进度工具栏构建保持不变] ...
}

void MainWindow::setupCustomTitleBarButtons() {
    // ... [其他标题栏按钮保持不变] ...

    m_btnLayout = createTitleBtn("layout");
    m_btnLayout->setProperty("tooltipText", "布局管理与重置");
    m_btnLayout->installEventFilter(m_hoverFilter);
    connect(m_btnLayout, &QPushButton::clicked, this, [this]() {
        QMenu menu(this);
        UiHelper::applyMenuStyle(&menu);
        // 🚀 委托 PanelLayoutManager 生成标准布局菜单
        m_panelLayoutManager->populatePanelMenu(&menu);
        menu.exec(m_btnLayout->mapToGlobal(QPoint(0, m_btnLayout->height())));
    });

    // ... [其他按钮保持不变] ...
}

void MainWindow::closeEvent(QCloseEvent* event) {
    AppConfig::instance().setValue("MainWindow/LastPath", m_currentPath);
    AppConfig::instance().setValue("MainWindow/Geometry", saveGeometry());

    // 🚀 委托 PanelLayoutManager 保存分栏状态与显隐配置
    if (m_panelLayoutManager) {
        m_panelLayoutManager->saveLayoutState();
    }
    AppConfig::instance().sync();

    QMainWindow::closeEvent(event);
}

// 🚨 彻底删除 MainWindow.cpp 中的：
// - loadPanelVisibility()
// - savePanelVisibility()
// - updateDynamicMinimumSize()
// - resetSplitterLayout()
// - showPanelContextMenu()
// - populatePanelMenu()
```

---

## 4. `CMakeLists.txt` 构建配置注册
```cmake
set(UI_SOURCES
    # ... 现有 UI 源文件 ...
    src/ui/PanelLayoutManager.h
    src/ui/PanelLayoutManager.cpp
)
```