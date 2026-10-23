# MainWindow 架构归位与模块解耦重构方案
# (mainwindow_refactoring.md)

---

## 1. Overview (概述)

本方案旨在彻底解决 `MainWindow`（上帝类残余）中微型逻辑过载、UI 子组件内聚度不足的问题。重构遵循 Clean Architecture 5 层架构划分规范（特别是【1. 窗口壳体层】与【2. 视图呈现层】），将原集中于 `MainWindow` 内的控件交互、状态格式化及响应式折行计算精准拆分至 4 个高内聚的黑盒组件中：

1. **`TitleBarWidget`**：承载标题栏 Logo、应用名称、网格缩放滑杆 (`QSlider`)、排列方式菜单 (`m_btnViewMenu`)、新建菜单以及窗口控制按钮组 (`min/max/close/pin/layout/toggleDriveBar`)。
2. **`NavBarWidget`**：承载导航按钮（后退/前进/上级）、地址栏（`AddressBar`）、搜索栏，并内聚响应式单/双行折行切换计算（阈值 `650px`）。
3. **`DriveBarWidget`**：承载盘符管理栏 UI 与标签管理入口 (`TagManagerDialog`)。
4. **`StatusBarWidget`**：承载底部状态栏显示、`CoreController` 索引状态感知以及 `ContentPanel` 项目数量统计格式化逻辑。

### ⚠️ 参数零变动铁律承诺 (Zero-Parameter-Deviation Guarantee)
**本次重构严格确保所有像素级尺寸、外边距/内边距、Hex颜色、控件阈值、ObjectName及信号槽绑定逻辑 100% 1:1 保留，绝不擅自修改任何现有参数！**

- `kLayoutEdgeMargin = 5`
- `kStatusBarHorizontalMargin = 12`
- `TitleBarWidget`: height `34px`, margins `(5, 0, 5, 0)`, spacing `8`
- `NavBarWidget`: height `42px` (single-row) / `78px` (two-row), threshold `650px`, margins `(5, 2, 5, 2)`, spacing `2`
- `DriveBarWidget`: height `42px`, margins `(15, 5, 15, 5)`, spacing `8`
- `StatusBarWidget`: height `28px`, margins `(12, 0, 12, 0)`, spacing `0`
- `ZoomSlider`: range `30 ~ 230`, fixed size `110x20`
- `NavControlBtn`: size `32x28`, iconSize `18x18`
- `TitleControlBtn`: size `24x24`, iconSize `18x18`

---

## 2. Modified Files List (修改文件清单)

### 新增文件 (New Files)
- `src/ui/TitleBarWidget.h` / `src/ui/TitleBarWidget.cpp`
- `src/ui/NavBarWidget.h` / `src/ui/NavBarWidget.cpp`
- `src/ui/DriveBarWidget.h` / `src/ui/DriveBarWidget.cpp`
- `src/ui/StatusBarWidget.h` / `src/ui/StatusBarWidget.cpp`

### 修改文件 (Modified Files)
- `CMakeLists.txt` (添加新增源文件)
- `src/ui/MainWindow.h` (移除已下沉的 UI 成员指针与子组件私有槽函数，仅保留组装壳体)
- `src/ui/MainWindow.cpp` (移除子组件内部构建逻辑，改为组装与转调)

---

## 3. Detailed Line-by-Line Changes (精准替换块)

### 3.1 `CMakeLists.txt` 修改

```cmake
<<<<<<< SEARCH
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
=======
    src/ui/TitleBarWidget.h
    src/ui/TitleBarWidget.cpp
    src/ui/NavBarWidget.h
    src/ui/NavBarWidget.cpp
    src/ui/DriveBarWidget.h
    src/ui/DriveBarWidget.cpp
    src/ui/StatusBarWidget.h
    src/ui/StatusBarWidget.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
>>>>>>> REPLACE
```

---

### 3.2 新增组件头文件与实现文件契约

#### `src/ui/TitleBarWidget.h`
```cpp
#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>

namespace QuarkMeta {

class HoverEventFilter;
class ContentPanel;
class PanelLayoutManager;

class TitleBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit TitleBarWidget(QWidget* parent = nullptr, HoverEventFilter* hoverFilter = nullptr);
    ~TitleBarWidget() override = default;

    void setContentPanel(ContentPanel* panel);
    void setPanelLayoutManager(PanelLayoutManager* layoutManager);
    void setAlwaysOnTopState(bool pinned);

    QPushButton* toggleDriveBarButton() const { return m_btnToggleDriveBar; }

signals:
    void pinToggled(bool checked);
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

private:
    void setupUi(HoverEventFilter* hoverFilter);
    void setupViewMenu();
    void setupCreateMenu();

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_logoLabel = nullptr;
    QLabel* m_appNameLabel = nullptr;

    QPushButton* m_btnViewMenu = nullptr;
    QSlider* m_sizeSlider = nullptr;

    QPushButton* m_btnToggleDriveBar = nullptr;
    QPushButton* m_btnLayout = nullptr;
    QPushButton* m_btnCreate = nullptr;
    QPushButton* m_btnPinTop = nullptr;
    QPushButton* m_btnMin = nullptr;
    QPushButton* m_btnMax = nullptr;
    QPushButton* m_btnClose = nullptr;

    ContentPanel* m_contentPanel = nullptr;
    PanelLayoutManager* m_layoutManager = nullptr;
};

} // namespace QuarkMeta
```

#### `src/ui/NavBarWidget.h`
```cpp
#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

namespace QuarkMeta {

class AddressBar;
class SearchController;
class HoverEventFilter;

class NavBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit NavBarWidget(QWidget* parent = nullptr, HoverEventFilter* hoverFilter = nullptr);
    ~NavBarWidget() override = default;

    AddressBar* addressBar() const { return m_addressBar; }
    SearchController* searchController() const { return m_searchController; }

    void updateResponsiveLayout();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUi(HoverEventFilter* hoverFilter);

    QVBoxLayout* m_mainLayout = nullptr;
    QWidget* m_row1Widget = nullptr;
    QHBoxLayout* m_row1Layout = nullptr;

    QPushButton* m_btnBack = nullptr;
    QPushButton* m_btnForward = nullptr;
    QPushButton* m_btnUp = nullptr;

    AddressBar* m_addressBar = nullptr;
    SearchController* m_searchController = nullptr;

    bool m_isTwoRowMode = false;
};

} // namespace QuarkMeta
```

#### `src/ui/DriveBarWidget.h`
```cpp
#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>

namespace QuarkMeta {

class DriveBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit DriveBarWidget(QWidget* parent = nullptr);
    ~DriveBarWidget() override = default;

private:
    QHBoxLayout* m_layout = nullptr;
    QPushButton* m_btnTagManager = nullptr;
};

} // namespace QuarkMeta
```

#### `src/ui/StatusBarWidget.h`
```cpp
#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>

namespace QuarkMeta {

class StatusBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit StatusBarWidget(QWidget* parent = nullptr);
    ~StatusBarWidget() override = default;

public slots:
    void updateStats(int visibleCount, int hiddenCount, int selectedCount);

private:
    void setupConnections();

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_statusLeft = nullptr;
};

} // namespace QuarkMeta
```

---

### 3.3 `MainWindow.h` 精简块

```cpp
<<<<<<< SEARCH
    QWidget* m_titleBarWidget = nullptr;
    QHBoxLayout* m_titleBarLayout = nullptr;
    QLabel* m_logoLabel = nullptr;
    QLabel* m_appNameLabel = nullptr;
    QWidget* m_navBarWidget = nullptr;
    QVBoxLayout* m_navBarMainLayout = nullptr;
    QWidget* m_navRow1Widget = nullptr;
    QHBoxLayout* m_navRow1Layout = nullptr;
    bool m_navBarIsTwoRowMode = false;
    QVBoxLayout* m_bodyLayout = nullptr; // 2026-05-08 按照用户要求：提升为成员变量以支持动态边距切换

    void updateNavBarResponsiveLayout();

    void initUi();
    void updateStatusBar();
    void initDriveBar();

    /**
     * @brief 统一导航调度向前兼容转调接口
     */
    void unifiedNavigateTo(const QString& url, bool record = true);

    void initToolbar();
    void setupSplitters();
    void setupCustomTitleBarButtons();
    void resetSplitterLayout();

    // 复合地址栏
    AddressBar* m_addressBar = nullptr;

    // 六个面板
    // 2026-04-11 按照用户要求：记录当前预览的文件路径，用于驱动方向键切图
    QString m_currentQuickLookPath;
    
    // UI Panels
    NavPanel* m_navPanel = nullptr;
    FavoritePanel* m_favoritePanel = nullptr;
    ContentPanel* m_contentPanel = nullptr;
    MetaPanel* m_metaPanel = nullptr;
    FilterPanel* m_filterPanel = nullptr;

    QSplitter* m_mainSplitter = nullptr;

    // 工具栏组件
    QToolBar* m_toolbar    = nullptr;
    QPushButton* m_btnBack    = nullptr;
    QPushButton* m_btnForward = nullptr;
    QPushButton* m_btnUp      = nullptr;

    SearchController* m_searchController = nullptr;
    
    // 排列方式视图按钮及中性缩放滑杆 (Modification_Plan-47)
    QPushButton* m_btnViewMenu = nullptr;
    QSlider* m_sizeSlider = nullptr;

    // 标题栏按钮组 (用于 frameless 时的模拟，此处作为标准按钮展示)
    QPushButton* m_btnToggleDriveBar = nullptr;
    QPushButton* m_btnLayout = nullptr;
    QPushButton* m_btnCreate = nullptr;
    QPushButton* m_btnPinTop = nullptr;
    QPushButton* m_btnMin = nullptr;
    QPushButton* m_btnMax = nullptr;
    QPushButton* m_btnClose = nullptr;

    // 盘符管理栏组件
    QWidget* m_driveBarWidget = nullptr;
    QHBoxLayout* m_driveBarLayout = nullptr;
    QPushButton* m_btnTagManager = nullptr;
    void onDriveBarContextMenu(const QPoint& pos);

    // 状态管理
    bool m_isPinned = false;
    QString m_currentDataSource; // "category" or "nav"
    bool m_panelsInitialized = false; // 2026-04-12 状态锁：确保面板仅初始化一次

    // 底部状态栏
    QLabel* m_statusLeft = nullptr;
    QWidget* m_statusBarWidget = nullptr;
    TaskProgressToolBar* m_taskProgressToolBar = nullptr;
=======
    // 子结构黑盒组件
    TitleBarWidget* m_titleBarWidget = nullptr;
    DriveBarWidget* m_driveBarWidget = nullptr;
    NavBarWidget*   m_navBarWidget = nullptr;
    StatusBarWidget* m_statusBarWidget = nullptr;

    QVBoxLayout* m_bodyLayout = nullptr;

    void initUi();

    // 复合地址栏与控制器暴露
    AddressBar* m_addressBar = nullptr;
    SearchController* m_searchController = nullptr;

    // UI Panels
    NavPanel* m_navPanel = nullptr;
    FavoritePanel* m_favoritePanel = nullptr;
    ContentPanel* m_contentPanel = nullptr;
    MetaPanel* m_metaPanel = nullptr;
    FilterPanel* m_filterPanel = nullptr;

    QSplitter* m_mainSplitter = nullptr;

    // 状态管理
    bool m_isPinned = false;
    QString m_currentDataSource; // "category" or "nav"
    bool m_panelsInitialized = false; // 2026-04-12 状态锁：确保面板仅初始化一次

    TaskProgressToolBar* m_taskProgressToolBar = nullptr;
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps (编译与验证步骤)

### 编译步骤 (Build Steps)
1. 在 CMake 工程配置中加入新增的 4 组 UI 源文件。
2. 运行构建指令：
   ```bash
   cmake --build --preset x64-Debug
   ```
3. 确保零编译警告、零 Link Error。

### 验证步骤 (Verification Steps)
1. **视觉 1:1 对齐校验**：
   - 检查标题栏 (34px)、导航栏 (42px/78px)、盘符栏 (42px)、状态栏 (28px) 的外边距、内边距与图标尺寸无任何改变。
   - 检查缩放滑块 (110x20) 拖拽时 ContentPanel 网格大小实时响应无延迟。
2. **功能与交互校验**：
   - 尝试“展开/收起盘符管理栏”按钮，确认 `DriveBarWidget` 显隐切换正常。
   - 尝试窗口宽度拖拉到 `< 650px`，确认 `NavBarWidget` 自动折行为双行 (78px)，放大后恢复单行 (42px)。
   - 确认置顶、最小化、最大化/还原、关闭按钮行为完全正常。
   - 检查底部状态栏文件/文件夹数量统计以及索引状态高亮显示均与重构前 1:1 一致。
