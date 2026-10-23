# Implementation Plan: PaneActivationTrackerAndMediatorRewiring.md

## 1. Overview
本方案为多窗格架构重构【第 2 步：窗格激活与接线修正】：
解决副窗格点击无法被激活、侧边栏/库/地址栏/新建/搜索/筛选操作始终作用于主窗格，以及状态栏与广播刷新忽略副窗格的架构缺陷。

## 2. Modified Files List
- `CMakeLists.txt`
- `src/ui/controllers/PaneActivationTracker.h` (新建)
- `src/ui/controllers/PaneActivationTracker.cpp` (新建)
- `src/ui/controllers/ContentViewCoordinator.h`
- `src/ui/controllers/ContentViewCoordinator.cpp`
- `src/ui/PanelMediator.h`
- `src/ui/PanelMediator.cpp`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/controllers/PaneActivationTracker.h` (新建文件)

```cpp
#pragma once

#include <QObject>

namespace QuarkMeta {

class ContentPanel;

/**
 * @brief 全应用窗格激活事件追踪器
 * 在 qApp 上安装事件过滤器，处理 MouseButtonPress 与 FocusIn 事件，
 * 精准捕获用户点击或聚焦的 ContentPanel 并触发激活。
 */
class PaneActivationTracker : public QObject {
    Q_OBJECT
public:
    explicit PaneActivationTracker(QObject* parent = nullptr);
    ~PaneActivationTracker() override = default;

signals:
    void paneInteracted(ContentPanel* panel);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

} // namespace QuarkMeta
```

### `src/ui/controllers/PaneActivationTracker.cpp` (新建文件)

```cpp
#include "PaneActivationTracker.h"
#include "../ContentPanel.h"
#include <QEvent>
#include <QWidget>

namespace QuarkMeta {

PaneActivationTracker::PaneActivationTracker(QObject* parent)
    : QObject(parent) {
}

bool PaneActivationTracker::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::FocusIn) {
        QWidget* widget = qobject_cast<QWidget*>(watched);
        if (widget) {
            QWidget* curr = widget;
            ContentPanel* foundPanel = nullptr;
            while (curr) {
                ContentPanel* panel = qobject_cast<ContentPanel*>(curr);
                if (panel) {
                    foundPanel = panel; // 持续向上寻找最近的 ContentPanel (副窗格位于根窗格内部，取最近的)
                    break;
                }
                curr = curr->parentWidget();
            }
            if (foundPanel) {
                emit paneInteracted(foundPanel);
            }
        }
    }
    return false; // 始终返回 false，不吞掉任何事件
}

} // namespace QuarkMeta
```

### `CMakeLists.txt`

```
<<<<<<< SEARCH
    src/ui/controllers/ContentPaneSplitManager.h
    src/ui/controllers/ContentPaneSplitManager.cpp
=======
    src/ui/controllers/ContentPaneSplitManager.h
    src/ui/controllers/ContentPaneSplitManager.cpp
    src/ui/controllers/PaneActivationTracker.h
    src/ui/controllers/PaneActivationTracker.cpp
>>>>>>> REPLACE
```

### `src/ui/controllers/ContentViewCoordinator.h` / `ContentViewCoordinator.cpp`
彻底删除没有调用方的 `installActivationFilters()` 及其成员 `m_filteredObjects`。

```
<<<<<<< SEARCH
    // 激活态事件过滤器统一安装入口：修复 panelActivated 从未被真正触发的问题
    void installActivationFilters();

private:
    ContentPanel* m_panel = nullptr;
    QSet<QObject*> m_filteredObjects;
=======
private:
    ContentPanel* m_panel = nullptr;
>>>>>>> REPLACE
```

### `src/ui/PanelMediator.h`

```
<<<<<<< SEARCH
    void setupConnections();

signals:
    /**
     * @brief 统一向 MainWindow 发送状态栏消息更新请求
     */
    void statusMessageRequested(const QString& message);

    /**
     * @brief 当前激活的内容面板改变广播信号
     */
    void activeContentPanelChanged(ContentPanel* panel);

private:
=======
    void setupConnections();

    /**
     * @brief 获取当前焦点 ContentPanel (若为空则降级返回主窗格)
     */
    ContentPanel* activeContentPanel() const;

signals:
    /**
     * @brief 统一向 MainWindow 发送状态栏消息更新请求
     */
    void statusMessageRequested(const QString& message);

    /**
     * @brief 当前激活的内容面板改变广播信号
     */
    void activeContentPanelChanged(ContentPanel* panel);

    /**
     * @brief 焦点窗格视图模式/排序状态改变信号
     */
    void activePaneStateChanged();

private:
    class PaneActivationTracker* m_activationTracker = nullptr;
>>>>>>> REPLACE
```

### `src/ui/PanelMediator.cpp`
1. 实例化 `PaneActivationTracker` 并安装到 `qApp`；
2. 监听 `paneInteracted(ContentPanel*)`，当与 `m_activeContentPanel` 不一致时调用 `panel->setActivePane(true)`；
3. 新增 `activeContentPanel()` 访问器；
4. 对 `wireContentPanel` 扩展：
   - 监听每个窗格的 `viewModeChanged` 和 `sortController()->sortCriteriaChanged`，仅当该窗格是 `activeContentPanel()` 时触发 `emit activePaneStateChanged()`；
   - 监听每个窗格的 `zoomLevelChanged`，仅当是 `activeContentPanel()` 时更新标题栏缩放滑块及 AppConfig 缩放级别；
   - 监听每个窗格的 `statusBarMessageReady`，仅当是 `activeContentPanel()` 时转发给 MainWindow；
   - 针对分栏视图（ColumnView），为每个窗格（包括后续动态创建的副窗格）连接 `columnView()->pathNavigated` 驱动地址栏与标签页；
5. 修改 `CentralEventHub` 的响应（`MetadataUpdated`、`ItemsDeleted`、`ItemsRenamed`、`UndoRedoPerformed`），通过遍历辅助函数对根窗格及其所有副窗格均调用 `updateItemMetadata` / `refreshAll`。

### `src/ui/MainWindow.h` / `MainWindow.cpp`
1. 状态栏视图模式按钮（分栏/自适应/网格/列表）和排序方向按钮，修改为对 `m_panelMediator->activeContentPanel()` 进行控制，不再直接硬编码 `m_contentPanel`；
2. 状态栏高亮与图标更新依据 `activeContentPanel()` 的属性；
3. 连接 `m_panelMediator->activePaneStateChanged` 刷新右下角按钮高亮；
4. 删掉 MainWindow 中直接连 `m_contentPanel->viewModeChanged` 与 `statusBarMessageReady` 的冗余直连代码。

### `src/ui/ContentPanel.h` / `ContentPanel.cpp`
1. 新增公开函数 `void refreshStatusBar()`，内部调用 `recalculateAndEmitStats()` 刷新一次状态栏描述；
2. 焦点窗格变更时，在 `PanelMediator` 中调用该函数刷新状态栏。

## 4. Build & Verification Steps
1. 运行 `cmake --build build` 确保无编译错误；
2. 启动应用，开启多分屏（2/3/4 窗格）；
3. 点击任意副窗格的任意区域（标题栏、文件项、空白区域、分栏列），校验该窗格高亮边框即刻跟随，成为焦点窗格；
4. 点击侧边栏文件夹、库分类、地址栏输入，校验内容精准加载在选中的焦点窗格中；
5. 执行新建文件夹（Ctrl+Shift+N）、搜索、筛选、QuickLook，校验全部精准作用于焦点窗格；
6. 检查右下角视图模式/排序按钮、缩放滑块、状态栏文字，校验其完全随焦点窗格的切换而同步刷新；
7. 删除或重命名文件后，校验显示该目录的所有窗格（主窗格与副窗格）全部同步原位刷新。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **复用既有函数**：复用了 `CentralEventHub` 的状态事件通知，`ContentPanel::recalculateAndEmitStats()`，`ContextMenuFactory` 等；
- **消除另起炉灶与分散逻辑**：通过 `PaneActivationTracker` 全局统一捕获窗格激活事件，删除了 `ContentViewCoordinator` 中未生效的冗余事件过滤器代码；统一收归 `activeContentPanel()`，彻底解决了 MainWindow 与各 Controller 硬编码主窗格的问题。

## 6. Header API Signature Verification
- `PaneActivationTracker::paneInteracted(ContentPanel* panel)` (信号)
- `PanelMediator::activeContentPanel() const` -> `ContentPanel*`
- `PanelMediator::activePaneStateChanged()` (信号)
- `ContentPanel::refreshStatusBar()` -> `void`

## 7. Header Inclusion Chain & Type Completeness Check
- `CMakeLists.txt` 注册新增的 `.h/.cpp`；
- `PanelMediator.cpp` 正确包含 `#include "controllers/PaneActivationTracker.h"` 与 `#include <QApplication>`；
- 类型完整性闭合。
