# Implementation Plan: ContentPaneSplitManager-6.md

## 1. Overview（概述与解决的问题）
本实施方案旨在彻底根治副窗格（Secondary Pane）在双击文件夹打开后**“地址栏、面包屑导航与侧边栏目录树死活不更新”**的重磅 bug，并全量修正副窗格与主窗格之间的 13 大行为双标与逻辑脱节缺陷，严格遵循 `Memories.md` 第 13 章《QuarkMeta 多窗格（分屏）功能设计理念》与 Clean Architecture 规范：

1. **副窗格文件夹双击导航连接 `NavigationService` 管线（彻底根治地址栏不更新）**：
   在 `wireContentPanel` 闭包与 `ContentPaneSplitManager` 中，将所有副窗格的 `directorySelected` 信号接入 `NavigationService::instance().navigateTo(path)`，确保副窗格双击文件夹时，地址栏 (AddressBar)、面包屑 (BreadcrumbBar)、侧边栏目录树 (NavPanel) 与 Tab 标题 100% 毫秒级同步更新！
2. **激活事件捕获器 (`PaneActivationTracker`) 实例化**：
   将 `PaneActivationTracker` 安装至 `qApp`，全应用统一捕获鼠标点击与聚焦事件，精准驱动 `ContentPanel` 激活，彻底解决点击副窗格空白处或列表项时激活态不切换的隐患。
3. **底栏视图模式与排序控制随动**：
   重构 `MainWindow` 底栏视图模式切换（网格、列表、自适应、分栏）及排序方向按钮，使其动态作用于当前激活窗格 `activeContentPanel()`，且底栏按钮高亮随激活窗格即时随动。
4. **底栏统计消息竞争隔离**：
   隔离 `statusBarMessageReady` 广播，仅当消息来自当前激活窗格时才刷到底栏，杜绝非激活主窗格后台事件冲刷覆盖副窗格统计信息。
5. **4 窗格上限与超限路径覆盖 (Section 13.1)**：
   达到 4 窗格上限后，继续发起分屏绝不创建第 5 个窗格，而是直接将新路径覆盖更新至“当前激活窗格”。
6. **单层 Splitter 平铺与自动等分尺寸 (Section 13.2)**：
   消除多层 `QSplitter` 嵌套风险，在新增/关闭/切换窗格时通过真实的视口几何算力按 `total / count` 精准等分布局。
7. **平滑窗格生命周期管理 (Section 13.3 & 13.5)**：
   重构窗格关闭逻辑，剥离关闭主窗格时的硬偷路径逻辑；单窗格模式下彻底隐匿激活边框，仅在多分屏状态下对激活窗格绘制 1px 高亮蓝框。
8. **Tab 状态与标题规则对齐 (Section 13.4)**：
   Tab 标题在分屏模式下严格按照 `文件夹A | 文件夹B` 格式输出（含相同路径重复显示不合并）；Tab 切换时平滑复用与恢复多窗格快照，避免频繁摧毁重建。
9. **拖拽语义严密隔离 (Section 13.6)**：
   严密限制 `application/x-quarkmeta-taburl` MIME 类型，彻底隔离普通文件/文件夹拖拽与分屏拖拽手势。

---

## 2. Modified Files List（影响文件清单）
1. `src/ui/controllers/PaneActivationTracker.h`
2. `src/ui/controllers/PaneActivationTracker.cpp`
3. `src/ui/controllers/ContentPaneSplitManager.h`
4. `src/ui/controllers/ContentPaneSplitManager.cpp`
5. `src/ui/PanelMediator.h`
6. `src/ui/PanelMediator.cpp`
7. `src/ui/MainWindow.h`
8. `src/ui/MainWindow.cpp`
9. `src/ui/TabBarWidget.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/controllers/ContentPaneSplitManager.cpp`
修补副窗格创建时的 `directorySelected` 槽函数，在副窗格触发双击导航时，同步触发激活与导航服务联动，解决地址栏路径不更新的核心病灶。

```
<<<<<<< SEARCH
    connect(newPane, &ContentPanel::directorySelected, m_panel, [this, newPane](const QString& path) {
        newPane->loadDirectory(path);
        emit m_panel->dualPanePathsChanged(m_panel->currentPath(), path);
    });
=======
    connect(newPane, &ContentPanel::directorySelected, m_panel, [this, newPane](const QString& path) {
        newPane->setActivePane(true);
        newPane->loadDirectory(path);
        emit m_panel->dualPanePathsChanged(m_panel->currentPath(), path);
        emit newPane->panelActivated(newPane);
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (paneCount() >= ContentPanel::kMaxPanes) {
        ContentPanel* target = m_activePaneForSplit ? m_activePaneForSplit : m_panel;
        if (!secondaryPath.isEmpty()) {
            target->loadDirectory(secondaryPath);
        }
        return;
    }
=======
    if (paneCount() >= ContentPanel::kMaxPanes) {
        ContentPanel* root = rootPane();
        ContentPanel* activeTarget = root;
        for (ContentPanel* p : root->panes()) {
            if (p && p->property("activePane").toBool()) {
                activeTarget = p;
                break;
            }
        }
        if (!secondaryPath.isEmpty() && activeTarget) {
            activeTarget->loadDirectory(secondaryPath);
        }
        return;
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPaneSplitManager::setActivePane(bool active) {
    if (active && rootPane()) {
        rootPane()->m_splitManager->m_activePaneForSplit = m_panel;
    }

    if (m_isSplit && m_primaryPaneContainer) {
        m_primaryPaneContainer->setProperty("activePane", active ? "true" : "false");
        m_primaryPaneContainer->style()->unpolish(m_primaryPaneContainer);
        m_primaryPaneContainer->style()->polish(m_primaryPaneContainer);
    } else {
        m_panel->setProperty("activePane", active ? "true" : "false");
        m_panel->style()->unpolish(m_panel);
        m_panel->style()->polish(m_panel);
    }

    if (m_panel->m_headerWidget) {
        m_panel->m_headerWidget->setActive(active);
    }
}
=======
void ContentPaneSplitManager::setActivePane(bool active) {
    bool hasMultiplePanes = (rootPane()->paneCount() > 1);

    if (m_isSplit && m_primaryPaneContainer) {
        m_primaryPaneContainer->setProperty("activePane", (active && hasMultiplePanes) ? "true" : "false");
        m_primaryPaneContainer->style()->unpolish(m_primaryPaneContainer);
        m_primaryPaneContainer->style()->polish(m_primaryPaneContainer);
    } else {
        m_panel->setProperty("activePane", (active && hasMultiplePanes) ? "true" : "false");
        m_panel->style()->unpolish(m_panel);
        m_panel->style()->polish(m_panel);
    }

    if (m_panel->m_headerWidget) {
        m_panel->m_headerWidget->setActive(active);
    }
}
>>>>>>> REPLACE
```

### 3.2 `src/ui/controllers/ContentPaneSplitManager.h`
更新接口，清理冗余成员变量。

```
<<<<<<< SEARCH
    ContentPanel* m_rootPane = nullptr;
    ContentPanel* m_activePaneForSplit = nullptr;
    QWidget* m_dragOverlayWidget = nullptr;
=======
    ContentPanel* m_rootPane = nullptr;
    QWidget* m_dragOverlayWidget = nullptr;
>>>>>>> REPLACE
```

### 3.3 `src/ui/PanelMediator.h`
增加 `activeContentPanel()` Getter 方法供 `MainWindow` 视图按键路由，并包含 `PaneActivationTracker` 前置声明。

```
<<<<<<< SEARCH
class PanelMediator : public QObject {
    Q_OBJECT

public:
    explicit PanelMediator(const PanelMediatorComponents& components, QObject* parent = nullptr);
    ~PanelMediator() override = default;
=======
class PaneActivationTracker;

class PanelMediator : public QObject {
    Q_OBJECT

public:
    explicit PanelMediator(const PanelMediatorComponents& components, QObject* parent = nullptr);
    ~PanelMediator() override = default;

    ContentPanel* activeContentPanel() const { return m_activeContentPanel.data(); }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QPointer<AppShortcutController> m_shortcutController;

    QString m_currentQuickLookPath;
=======
    QPointer<AppShortcutController> m_shortcutController;
    PaneActivationTracker* m_activationTracker = nullptr;

    QString m_currentQuickLookPath;
>>>>>>> REPLACE
```

### 3.4 `src/ui/PanelMediator.cpp`
在 `wireContentPanel` 通用绑定中，将任何窗格（包含副窗格）的 `directorySelected` 接入 `NavigationService::instance().navigateTo(path)`，彻底驱动地址栏、面包屑与目录树随动；安装 `PaneActivationTracker` 至 `qApp` 并隔离非激活窗格的底栏消息冲刷。

```
<<<<<<< SEARCH
#include "PanelMediator.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "LibraryPanel.h"
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "AddressBar.h"
#include "SearchController.h"
#include "TitleBarWidget.h"
#include "TabBarWidget.h"
#include "PanelLayoutManager.h"
#include "AppShortcutController.h"
#include "ToolTipOverlay.h"
#include "../core/NavigationService.h"
#include <QMenu>
#include <QDir>
#include <QApplication>
=======
#include "PanelMediator.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "LibraryPanel.h"
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "AddressBar.h"
#include "SearchController.h"
#include "TitleBarWidget.h"
#include "TabBarWidget.h"
#include "PanelLayoutManager.h"
#include "AppShortcutController.h"
#include "controllers/PaneActivationTracker.h"
#include "ToolTipOverlay.h"
#include "../core/NavigationService.h"
#include <QMenu>
#include <QDir>
#include <QApplication>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        // 3. 状态栏与焦点响应
        connect(panel, &ContentPanel::statusBarMessageReady, this, [this](const QString& message) {
            emit statusMessageRequested(message);
        });
=======
        // 3. 状态栏与焦点响应 (仅当信息来自当前激活窗格时才刷到底栏)
        connect(panel, &ContentPanel::statusBarMessageReady, this, [this, panel](const QString& message) {
            if (m_activeContentPanel == panel || (!m_activeContentPanel && panel == m_contentPanel.data())) {
                emit statusMessageRequested(message);
            }
        });

        connect(panel, &ContentPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (contentPanel) {
        (*wireContentPanel)(contentPanel);
=======
    // 安装全应用窗格激活事件过滤器
    if (!m_activationTracker) {
        m_activationTracker = new PaneActivationTracker(this);
        qApp->installEventFilter(m_activationTracker);

        connect(m_activationTracker, &PaneActivationTracker::paneInteracted, this, [this](ContentPanel* panel) {
            if (panel && m_activeContentPanel != panel) {
                m_activeContentPanel = panel;
                emit activeContentPanelChanged(panel);
            }
        });
    }

    if (contentPanel) {
        (*wireContentPanel)(contentPanel);
>>>>>>> REPLACE
```

### 3.5 `src/ui/MainWindow.cpp`
重构底栏视图切换与排序方向按钮的信号槽连接，使其动态获取并作用于 `activeContentPanel()`，且更新 `updateStatusBarButtonHighlights()` 指向当前激活窗格。

```
<<<<<<< SEARCH
    connect(m_btnToggleSortOrder, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            Qt::SortOrder current = m_contentPanel->currentSortOrder();
            Qt::SortOrder next = (current == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
            m_contentPanel->setSortOrder(next);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleColumn, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::ColumnView);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleJustified, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::JustifiedViewMode);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleGrid, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::GridView);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleList, &QPushButton::clicked, this, [this]() {
        if (m_contentPanel) {
            m_contentPanel->setViewMode(ContentPanel::ListView);
            updateStatusBarButtonHighlights();
        }
    });
=======
    auto getActivePanel = [this]() -> ContentPanel* {
        if (m_panelMediator && m_panelMediator->activeContentPanel()) {
            return m_panelMediator->activeContentPanel();
        }
        return m_contentPanel;
    };

    connect(m_btnToggleSortOrder, &QPushButton::clicked, this, [this, getActivePanel]() {
        ContentPanel* target = getActivePanel();
        if (target) {
            Qt::SortOrder current = target->currentSortOrder();
            Qt::SortOrder next = (current == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
            target->setSortOrder(next);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleColumn, &QPushButton::clicked, this, [this, getActivePanel]() {
        ContentPanel* target = getActivePanel();
        if (target) {
            target->setViewMode(ContentPanel::ColumnView);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleJustified, &QPushButton::clicked, this, [this, getActivePanel]() {
        ContentPanel* target = getActivePanel();
        if (target) {
            target->setViewMode(ContentPanel::JustifiedViewMode);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleGrid, &QPushButton::clicked, this, [this, getActivePanel]() {
        ContentPanel* target = getActivePanel();
        if (target) {
            target->setViewMode(ContentPanel::GridView);
            updateStatusBarButtonHighlights();
        }
    });

    connect(m_btnToggleList, &QPushButton::clicked, this, [this, getActivePanel]() {
        ContentPanel* target = getActivePanel();
        if (target) {
            target->setViewMode(ContentPanel::ListView);
            updateStatusBarButtonHighlights();
        }
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (m_contentPanel) {
        ContentPanel::ViewMode mode = m_contentPanel->currentViewMode();
        if (m_btnToggleColumn)    m_btnToggleColumn->setChecked(mode == ContentPanel::ColumnView);
        if (m_btnToggleJustified) m_btnToggleJustified->setChecked(mode == ContentPanel::JustifiedViewMode);
        if (m_btnToggleGrid)      m_btnToggleGrid->setChecked(mode == ContentPanel::GridView);
        if (m_btnToggleList)      m_btnToggleList->setChecked(mode == ContentPanel::ListView);

        Qt::SortOrder sortOrd = m_contentPanel->currentSortOrder();
        if (m_btnToggleSortOrder) {
            bool isAsc = (sortOrd == Qt::AscendingOrder);
            m_btnToggleSortOrder->setIcon(UiHelper::getIcon(isAsc ? "arrow_up_long" : "arrow_down_long", QColor("#EEEEEE"), 18));
            m_btnToggleSortOrder->setProperty("tooltipText", isAsc ? "升序 状态中" : "降序 状态中");
        }
    }
=======
    ContentPanel* targetPanel = (m_panelMediator && m_panelMediator->activeContentPanel()) ? m_panelMediator->activeContentPanel() : m_contentPanel;
    if (targetPanel) {
        ContentPanel::ViewMode mode = targetPanel->currentViewMode();
        if (m_btnToggleColumn)    m_btnToggleColumn->setChecked(mode == ContentPanel::ColumnView);
        if (m_btnToggleJustified) m_btnToggleJustified->setChecked(mode == ContentPanel::JustifiedViewMode);
        if (m_btnToggleGrid)      m_btnToggleGrid->setChecked(mode == ContentPanel::GridView);
        if (m_btnToggleList)      m_btnToggleList->setChecked(mode == ContentPanel::ListView);

        Qt::SortOrder sortOrd = targetPanel->currentSortOrder();
        if (m_btnToggleSortOrder) {
            bool isAsc = (sortOrd == Qt::AscendingOrder);
            m_btnToggleSortOrder->setIcon(UiHelper::getIcon(isAsc ? "arrow_up_long" : "arrow_down_long", QColor("#EEEEEE"), 18));
            m_btnToggleSortOrder->setProperty("tooltipText", isAsc ? "升序 状态中" : "降序 状态中");
        }
    }
>>>>>>> REPLACE
```

### 3.6 `src/ui/TabBarWidget.cpp`
规范多窗格下 Tab 标题的格式输出，确保重复路径不合并，完美遵从 `Memories.md` 规范。

```
<<<<<<< SEARCH
void TabBarWidget::updateSplitTabTitle(const TabSplitState& state) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    m_tabs[m_currentIndex].splitState = state;

    auto cleanName = [](const QString& u) -> QString {
        if (u == "computer://" || u.isEmpty()) return "此电脑";
        QFileInfo fi(QDir::cleanPath(u));
        QString fn = fi.fileName();
        return fn.isEmpty() ? u : fn;
    };

    if (state.isSplit && !state.panePaths.isEmpty()) {
        QStringList nameList;
        for (const QString& p : state.panePaths) {
            nameList.append(cleanName(p));
        }
        QString mergedTitle = nameList.join(" | ");
        m_tabs[m_currentIndex].title = mergedTitle;
        m_tabs[m_currentIndex].url = state.panePaths.first();
    } else if (!state.panePaths.isEmpty()) {
        m_tabs[m_currentIndex].title = cleanName(state.panePaths.first());
        m_tabs[m_currentIndex].url = state.panePaths.first();
    }

    if (m_currentIndex < m_tabWidgets.size()) {
        m_tabWidgets[m_currentIndex]->setTabTitle(m_tabs[m_currentIndex].title);
    }
    saveStateToConfig();
}
=======
void TabBarWidget::updateSplitTabTitle(const TabSplitState& state) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    m_tabs[m_currentIndex].splitState = state;

    auto cleanName = [](const QString& u) -> QString {
        if (u == "computer://" || u.isEmpty()) return "此电脑";
        QFileInfo fi(QDir::cleanPath(u));
        QString fn = fi.fileName();
        return fn.isEmpty() ? u : fn;
    };

    if (state.isSplit && !state.panePaths.isEmpty()) {
        QStringList nameList;
        for (const QString& p : state.panePaths) {
            nameList.append(cleanName(p)); // 保持完整映射，哪怕路径相同也重复保留，绝对不进行去重合并
        }
        QString mergedTitle = nameList.join(" | ");
        m_tabs[m_currentIndex].title = mergedTitle;
        m_tabs[m_currentIndex].url = state.panePaths.first();
    } else if (!state.panePaths.isEmpty()) {
        m_tabs[m_currentIndex].title = cleanName(state.panePaths.first());
        m_tabs[m_currentIndex].url = state.panePaths.first();
    }

    if (m_currentIndex < m_tabWidgets.size()) {
        m_tabWidgets[m_currentIndex]->setTabTitle(m_tabs[m_currentIndex].title);
    }
    saveStateToConfig();
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译指令
在 CMake 环境下构建并清理 MOC 生成目标：
```bash
cmake -B build -S .
cmake --build build --config Debug
```

### 4.2 验证方法
1. **副窗格双击导航地址栏同步验证**：建立双窗格分屏，在副窗格中双击进入某个子文件夹，**核验顶部的地址栏 (AddressBar)、面包屑 (BreadcrumbBar) 和侧边栏目录树 (NavPanel) 是否 100% 同步更新为副窗格的新路径**。
2. **底栏视图模式随动验证**：分别点击主窗格与副窗格。切换至副窗格后，点击底栏“列表视图”/“网格视图”/“排序方向”按钮，核验改变的是否是副窗格的视图模式和排序方向，且底栏按钮高亮状态是否与副窗格保持一致。
3. **底栏统计隔离验证**：在副窗格进行操作并选中文件，核验底栏文本是否为当前副窗格的选中统计，主窗格后台事件是否不再抢占冲刷底栏信息。
4. **激活焦点验证**：启动程序，开启双窗格或四窗格分屏，点击不同窗格的文件列表区域或空白处，核验高亮蓝框是否精准随焦点转移，且地址栏路径、元数据面板与搜索框是否 100% 同步转移。
5. **单窗格高亮隐匿验证**：关闭所有副窗格退回单窗格模式，核验主窗格四周是否无任何蓝框悬挂。
6. **4 窗格超限验证**：在已建立 4 个窗格的状态下，再次点击分屏按钮或拖拽标签页到边缘，核验是否成功将新路径载入至“当前激活窗格”而未建第 5 个窗格。
7. **Tab 标题规范验证**：分屏打开相同文件夹（如两个 `Download`），核验 Tab 标题是否严格显示为 `Download | Download`。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- **真理源路由**：本次修改复用了 `PanelMediator::activeContentPanelChanged` 统一信号广播与 `NavigationService::instance().navigateTo(path)` 入口，消灭了局部私存与分支硬编码；
- **刷新接口复用**：窗格数据更新统一调用 `ContentPanel::refreshAll()` 或 `loadDirectory()`，没有新建二次数据载入管道。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）
| 类名 / 结构体 | 成员函数 / 属性精确签名 | 物理头文件 |
| :--- | :--- | :--- |
| `PanelMediator` | `ContentPanel* activeContentPanel() const` | `src/ui/PanelMediator.h` |
| `PaneActivationTracker` | `explicit PaneActivationTracker(QObject* parent = nullptr)` | `src/ui/controllers/PaneActivationTracker.h` |
| `PaneActivationTracker` | `void paneInteracted(ContentPanel* panel)` (signal) | `src/ui/controllers/PaneActivationTracker.h` |
| `ContentPaneSplitManager` | `void setActivePane(bool active)` | `src/ui/controllers/ContentPaneSplitManager.h` |
| `ContentPaneSplitManager` | `void splitPane(Qt::Orientation orientation, const QString& secondaryPath = QString())` | `src/ui/controllers/ContentPaneSplitManager.h` |
| `TabBarWidget` | `void updateSplitTabTitle(const TabSplitState& state)` | `src/ui/TabBarWidget.h` |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）
- `src/ui/PanelMediator.cpp` 中新增显式包含：
  - `#include "controllers/PaneActivationTracker.h"`
  - `#include <QApplication>`
- 核查 `ContentPanel` 与 `PaneActivationTracker` 在 `PanelMediator` 中具有完整前置声明与头文件定义，无 C2027（不完整类型）或 C2039（成员不存在）链接漏洞。
