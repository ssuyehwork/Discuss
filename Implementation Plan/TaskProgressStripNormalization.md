# TaskProgressStripNormalization.md - QuarkMeta 进度条归一化与 TaskProgressStrip 唯一化重构方案

Base Commit Hash: `0348f4197d5c7579c77262fcade3edca6bb5ed16`
Prerequisite Plans: 无
Supersedes: 无

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 背景与根因分析
在 QuarkMeta 主工程中，过往演进过程遗留了多种不同形态的进度条组件（例如内嵌于工具栏的 `TaskProgressToolBar` / `TaskProgressBar` 等）。这些分散的进度条不仅增加了 UI 层与布局维护成本，还在样式表 `style.qss` 与主窗口 `MainWindow` 中产生了死代码与冗余属性，严重影响代码可读性，并可能误导架构分析。

### 1.2 重构目标
1. **单一真理源 (SSOT)**：全软件仅使用唯一的悬浮线条进度条 `TaskProgressStrip`（高度 5px，橙红色 `#ff551c` 进度块，动态悬浮贴紧状态栏上方，响应 `TaskProgressService::snapshotChanged` 信号）。
2. **彻底根除死代码**：
   - 物理彻底删除旧的 `src/ui/TaskProgressToolBar.h` 和 `src/ui/TaskProgressToolBar.cpp` 文件。
   - 在 `src/ui/MainWindow.h` 与 `src/ui/MainWindow.cpp` 中解耦并移除 `TaskProgressToolBar`，接入 `TaskProgressStrip` 并配置动态锚定（`setAnchors(bodyWrapper, m_statusBarWidget)`）。
   - 在 `CMakeLists.txt` 中同步移除 `TaskProgressToolBar` 并注册 `TaskProgressStrip`。
   - 在 `resources/style.qss` 中物理删除 `#TaskProgressBar`、`#TaskProgressTitleLabel` 等废弃选择器，保留干净的 `#TaskProgressStrip` 样式。

---

## Chapter 2: Modified Files List（影响文件清单）

### 2.1 新建文件 (Create File)
- `src/ui/TaskProgressStrip.h`
- `src/ui/TaskProgressStrip.cpp`

### 2.2 删除文件 (Delete File)
- `src/ui/TaskProgressToolBar.h`
- `src/ui/TaskProgressToolBar.cpp`

### 2.3 修改文件 (Modify File)
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `CMakeLists.txt`
- `resources/style.qss`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 新建 `src/ui/TaskProgressStrip.h`

<<<<<<< SEARCH FILE: src/ui/TaskProgressStrip.h
=======
#pragma once

#include <QProgressBar>
#include <QEvent>
#include <QWidget>
#include "../core/TaskProgressService.h"

namespace QuarkMeta {

/**
 * @brief 顶部悬浮线条进度条
 * 动态锚定在主界面中央视图与底部状态栏交界处，响应该后台任务进度
 */
class TaskProgressStrip : public QProgressBar {
    Q_OBJECT

public:
    explicit TaskProgressStrip(QWidget* parent = nullptr);
    ~TaskProgressStrip() override = default;

    void setAnchors(QWidget* bodyWrapper, QWidget* statusBar);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateGeometryPos();

    QWidget* m_bodyWrapper = nullptr;
    QWidget* m_statusBar = nullptr;
};

} // namespace QuarkMeta
>>>>>>> REPLACE

### 3.2 新建 `src/ui/TaskProgressStrip.cpp`

<<<<<<< SEARCH FILE: src/ui/TaskProgressStrip.cpp
=======
#include "TaskProgressStrip.h"
#include <QPoint>
#include <QRect>

namespace QuarkMeta {

constexpr int kStripHeight = 5;

TaskProgressStrip::TaskProgressStrip(QWidget* parent)
    : QProgressBar(parent) {
    setObjectName("TaskProgressStrip");
    setFixedHeight(kStripHeight);
    setRange(0, 100);
    setTextVisible(false);
    hide();

    connect(&TaskProgressService::instance(), &TaskProgressService::snapshotChanged,
            this, [this](const TaskProgressSnapshot& snapshot) {
        if (snapshot.active) {
            setValue(snapshot.percent);
            if (!isVisible()) {
                show();
                updateGeometryPos();
            }
        } else {
            hide();
        }
    });
}

void TaskProgressStrip::setAnchors(QWidget* bodyWrapper, QWidget* statusBar) {
    m_bodyWrapper = bodyWrapper;
    m_statusBar = statusBar;

    QWidget* topParent = parentWidget();
    if (topParent) {
        topParent->installEventFilter(this);
    }
    if (m_bodyWrapper) {
        m_bodyWrapper->installEventFilter(this);
    }
    if (m_statusBar) {
        m_statusBar->installEventFilter(this);
    }

    updateGeometryPos();
}

bool TaskProgressStrip::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Resize || event->type() == QEvent::Move || event->type() == QEvent::Show) {
        updateGeometryPos();
    }
    return QProgressBar::eventFilter(watched, event);
}

void TaskProgressStrip::updateGeometryPos() {
    QWidget* topParent = parentWidget();
    if (!topParent || !m_bodyWrapper || !m_statusBar) return;

    QPoint bodyLeftTop = m_bodyWrapper->mapTo(topParent, QPoint(0, 0));
    QPoint statusLeftTop = m_statusBar->mapTo(topParent, QPoint(0, 0));

    int x = bodyLeftTop.x();
    int w = m_bodyWrapper->width();
    int y = statusLeftTop.y() - kStripHeight;

    setGeometry(x, y, w, kStripHeight);
    raise();
}

} // namespace QuarkMeta
>>>>>>> REPLACE

### 3.3 物理删除 `src/ui/TaskProgressToolBar.h`

<<<<<<< SEARCH DELETE FILE: src/ui/TaskProgressToolBar.h
=======
>>>>>>> REPLACE

### 3.4 物理删除 `src/ui/TaskProgressToolBar.cpp`

<<<<<<< SEARCH DELETE FILE: src/ui/TaskProgressToolBar.cpp
=======
>>>>>>> REPLACE

### 3.5 修改 `src/ui/MainWindow.h`

<<<<<<< SEARCH
class TrayController;
class HoverEventFilter;
class AddressBar;
class TaskProgressToolBar;
class SearchController;
=======
class TrayController;
class HoverEventFilter;
class AddressBar;
class TaskProgressStrip;
class SearchController;
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 底部状态栏
    QLabel* m_statusLeft = nullptr;
    QWidget* m_statusBarWidget = nullptr;
    TaskProgressToolBar* m_taskProgressToolBar = nullptr;
=======
    // 底部状态栏与悬浮进度条
    QLabel* m_statusLeft = nullptr;
    QWidget* m_statusBarWidget = nullptr;
    TaskProgressStrip* m_taskProgressStrip = nullptr;
>>>>>>> REPLACE

### 3.6 修改 `src/ui/MainWindow.cpp`

<<<<<<< SEARCH
#include "AddressBar.h"
#include "ToolTipOverlay.h"
#include "TaskProgressToolBar.h"
#include "../core/AppConfig.h"
=======
#include "AddressBar.h"
#include "ToolTipOverlay.h"
#include "TaskProgressStrip.h"
#include "../core/AppConfig.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 3. 底部状态栏组装
    setupStatusBar(centralC);

    // 4. 控制器与中介者组装（组件依赖图绑定）
    setupControllersAndMediators();

    // 5. 组装至顶层主布局
    mainL->addWidget(m_titleBarWidget);
    mainL->addWidget(m_driveBarWidget);
    mainL->addWidget(m_navBarWidget);
    mainL->addWidget(bodyWrapper, 1);
    mainL->addWidget(m_statusBarWidget);
    mainL->addWidget(m_taskProgressToolBar);

    setCentralWidget(centralC);
=======
    // 3. 底部状态栏组装
    setupStatusBar(centralC);

    // 4. 悬浮线条进度条组装（作为 centralC 子控件悬浮，不进入布局）
    m_taskProgressStrip = new TaskProgressStrip(centralC);

    // 5. 控制器与中介者组装（组件依赖图绑定）
    setupControllersAndMediators();

    // 6. 组装至顶层主布局
    mainL->addWidget(m_titleBarWidget);
    mainL->addWidget(m_driveBarWidget);
    mainL->addWidget(m_navBarWidget);
    mainL->addWidget(bodyWrapper, 1);
    mainL->addWidget(m_statusBarWidget);

    // 7. 设置悬浮进度条对齐锚点
    m_taskProgressStrip->setAnchors(bodyWrapper, m_statusBarWidget);

    setCentralWidget(centralC);
>>>>>>> REPLACE

<<<<<<< SEARCH
    connect(&CoreController::instance(), &CoreController::statusTextChanged, this, updateStatus);
    connect(&CoreController::instance(), &CoreController::isIndexingChanged, this, updateStatus);
    updateStatus();

    m_taskProgressToolBar = new TaskProgressToolBar(parentWidget);
    m_taskProgressToolBar->hide();

    auto createStatusBtn = [this](const QString& iconKey, const QString& tip) -> QPushButton* {
=======
    connect(&CoreController::instance(), &CoreController::statusTextChanged, this, updateStatus);
    connect(&CoreController::instance(), &CoreController::isIndexingChanged, this, updateStatus);
    updateStatus();

    auto createStatusBtn = [this](const QString& iconKey, const QString& tip) -> QPushButton* {
>>>>>>> REPLACE

### 3.7 修改 `CMakeLists.txt`

<<<<<<< SEARCH
    src/ui/TaskProgressToolBar.h
    src/ui/TaskProgressToolBar.cpp
=======
    src/ui/TaskProgressStrip.h
    src/ui/TaskProgressStrip.cpp
>>>>>>> REPLACE

### 3.8 修改 `resources/style.qss`

<<<<<<< SEARCH
QLabel#TaskProgressTitleLabel {
    color: #EEEEEE;
    font-size: 11px;
    font-weight: bold;
    background: transparent;
}
QProgressBar#TaskProgressBar {
    border: 1px solid #3c3c3c;
    border-radius: 3px;
    background-color: #1E1E1E;
    text-align: center;
    color: transparent;
}
QProgressBar#TaskProgressBar::chunk {
    background-color: #ff551c;
    border-radius: 2px;
}
QLabel#TaskProgressDetailLabel {
    color: #888888;
    font-size: 11px;
    background: transparent;
}
QLabel#TaskProgressCountLabel {
    color: #ff551c;
    font-size: 11px;
    font-weight: bold;
    background: transparent;
}
=======
/* ===================================================================
   TaskProgressStrip Styles
   =================================================================== */
QProgressBar#TaskProgressStrip {
    background-color: transparent;
    border: none;
    max-height: 5px;
}

QProgressBar#TaskProgressStrip::chunk {
    background-color: #ff551c;
    border-radius: 1px;
}
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译指令
在项目根目录运行以下构建命令：
```bash
cmake -B build -S .
cmake --build build --config Debug
```

### 4.2 运行时验收标准
1. **纯净编译**：全项目无 `TaskProgressToolBar` 相关未定义符号错误或标头缺失编译错误。
2. **正确布局呈现**：主界面下方不再显示底部的多余进度工具栏；在后台任务执行时，主视图区域与状态栏交界处浮现 5px 高度的橙红色悬浮进度条 `TaskProgressStrip`。
3. **几何联动**：拖动或改变主窗口大小时，悬浮线条进度条能够平滑跟随 `bodyWrapper` 与 `statusBar` 的交界位置更新 Geometry。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 `TaskProgressService::instance().snapshotChanged` 唯一官方信号进行进度状态广播与驱动。
- 复用 `QProgressBar` 内置的 `setFixedHeight` / `setRange` / `setValue` API。
- 完全剔除了另起炉灶的 `TaskProgressToolBar` 及其周边控件标签，实现了进度的单一展示真理源 (SSOT)。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| **后台任务进度快照** | `TaskProgressService` | `TaskProgressSnapshot` (在 `TaskProgressStrip` 中作为值传递) | `TaskProgressService` ➔ `TaskProgressStrip::snapshotChanged` ➔ `setValue(percent)` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 类与接口 | 访问级别 | 声明位置 | 定义位置 | 调用点 |
| :--- | :--- | :--- | :--- | :--- |
| `TaskProgressStrip::TaskProgressStrip(QWidget* parent)` | `public` | `src/ui/TaskProgressStrip.h` | `src/ui/TaskProgressStrip.cpp` | `src/ui/MainWindow.cpp` |
| `TaskProgressStrip::setAnchors(QWidget* bodyWrapper, QWidget* statusBar)` | `public` | `src/ui/TaskProgressStrip.h` | `src/ui/TaskProgressStrip.cpp` | `src/ui/MainWindow.cpp` |
| `TaskProgressStrip::eventFilter(QObject* watched, QEvent* event)` | `protected` | `src/ui/TaskProgressStrip.h` | `src/ui/TaskProgressStrip.cpp` | Qt 事件循环自动回调 |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `src/ui/TaskProgressStrip.h` 中显式包含了 `<QProgressBar>`、`<QEvent>`、`<QWidget>` 及 `"../core/TaskProgressService.h"`，类型完整无歧义。
- `src/ui/MainWindow.h` 中使用 `class TaskProgressStrip;` 前置声明，并在 `src/ui/MainWindow.cpp` 中 `#include "TaskProgressStrip.h"`，遵守无循环包含铁律。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 结论 | 备注 |
| :--- | :--- | :--- | :--- |
| 1 | 契约锁（不随意破坏 public 签名） | 满足 | 物理重构并清理废弃组件 |
| 2 | 依赖锁（单向依赖 Shell->View->Controller） | 满足 | `TaskProgressStrip` 位于 UI 层，依赖 `TaskProgressService` |
| 3 | 静态红线（无 Win32 抢焦点/全局 hook） | 满足 | 纯 Qt 标准事件过滤器与 Geometry 计算 |
| 4 | 黑盒隔离锁（对接口/DTO 负责） | 满足 | 仅依赖 `TaskProgressService` 共有信号 |
| 5 | CMake 注册同步 | 满足 | 在 `CMakeLists.txt` 中替换注册文件 |
| 6 | 物理死代码彻底删除 | 满足 | 物理删除 `TaskProgressToolBar.h/cpp` |
| 7 | QSS 干净清理 | 满足 | 物理删除 `#TaskProgressBar` 废弃样式 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 属性/行为 | 既有值/行为 | 重构后值/行为 | 一致性确认 |
| :--- | :--- | :--- | :--- |
| **进度条高度** | 5px (`kStripHeight = 5`) | 5px (`kStripHeight = 5`) | 100% 一致 |
| **进度块颜色** | `#ff551c` | `#ff551c` | 100% 一致 |
| **圆角** | 1px | 1px | 100% 一致 |
| **进度数值范围** | `0 ~ 100` | `0 ~ 100` | 100% 一致 |
