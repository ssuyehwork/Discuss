# QuarkMeta 全局任务进度服务实施方案 (TaskProgressService)

## 1. 目标与范围
- 新建 `TaskProgressService`（位于 `src/core/`）：建立线程安全的全局异步任务调度与进度中枢，统一管理后台任务的注册（`createJob`）、实时进度汇报（`updateProgress`）与生命周期终结（`finishJob`）。
- 彻底物理废除并删除冗余的侵入式控制器：`src/ui/TaskProgressController.h` 与 `src/ui/TaskProgressController.cpp`。
- 改造 `TaskProgressToolBar`：使底部进度工具栏退化为纯粹的观察者（Observer），直接监听 `TaskProgressService` 的信号并实现自动滑动显隐。
- 净化 `MainWindow.h/cpp`：清除 `m_taskProgressController` 字段与相关侵入式初始化代码。

---

## 2. 核心模块独立实现

### 2.1 `src/core/TaskProgressService.h`
```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QMutex>
#include <atomic>

namespace QuarkMeta {

struct TaskJobInfo {
    int id = -1;
    QString title;
    QString detail;
    int currentStep = 0;
    int totalSteps = 100;
    int percent = 0;
};

class TaskProgressService : public QObject {
    Q_OBJECT

public:
    static TaskProgressService& instance();

    /**
     * @brief 注册新后台任务 (线程安全，支持任意子线程调用)
     * @param title 任务主标题 (如: "正在提取缩略图...")
     * @param totalSteps 任务总步数
     * @return 唯一任务 ID
     */
    int createJob(const QString& title, int totalSteps = 100);

    /**
     * @brief 更新指定任务进度 (线程安全)
     */
    void updateProgress(int jobId, int currentStep, int totalSteps = -1, const QString& detail = "");

    /**
     * @brief 标记任务完成 (线程安全)
     */
    void finishJob(int jobId);

    /**
     * @brief 强制取消/移除任务 (线程安全)
     */
    void cancelJob(int jobId);

    int activeJobCount() const;
    bool hasActiveJobs() const;

signals:
    /**
     * @brief 首个任务启动时发射 (驱动进度栏平滑展开)
     */
    void jobStarted(int jobId, const QString& title);

    /**
     * @brief 进度变动广播 (含当前聚焦任务的百分比与详情)
     */
    void progressUpdated(int percent, const QString& title, const QString& detail, int activeCount);

    /**
     * @brief 单个任务完成广播
     */
    void jobFinished(int jobId);

    /**
     * @brief 全量后台任务归零时发射 (驱动进度栏平滑收起隐藏)
     */
    void allJobsFinished();

private:
    explicit TaskProgressService(QObject* parent = nullptr);
    ~TaskProgressService() override = default;
    TaskProgressService(const TaskProgressService&) = delete;
    TaskProgressService& operator=(const TaskProgressService&) = delete;

    void recalculateAndEmit();

    mutable QMutex m_mutex;
    std::atomic<int> m_nextJobId{1};
    QMap<int, TaskJobInfo> m_jobs;
};

} // namespace QuarkMeta
```

### 2.2 `src/core/TaskProgressService.cpp`
```cpp
#include "TaskProgressService.h"
#include <QMutexLocker>
#include <QCoreApplication>

namespace QuarkMeta {

TaskProgressService& TaskProgressService::instance() {
    static TaskProgressService s_instance;
    return s_instance;
}

TaskProgressService::TaskProgressService(QObject* parent) : QObject(parent) {}

int TaskProgressService::createJob(const QString& title, int totalSteps) {
    int id = m_nextJobId.fetch_add(1);

    {
        QMutexLocker locker(&m_mutex);
        TaskJobInfo info;
        info.id = id;
        info.title = title;
        info.totalSteps = totalSteps > 0 ? totalSteps : 100;
        info.currentStep = 0;
        info.percent = 0;
        m_jobs.insert(id, info);
    }

    QMetaObject::invokeMethod(this, [this, id, title]() {
        emit jobStarted(id, title);
        recalculateAndEmit();
    }, Qt::QueuedConnection);

    return id;
}

void TaskProgressService::updateProgress(int jobId, int currentStep, int totalSteps, const QString& detail) {
    {
        QMutexLocker locker(&m_mutex);
        if (!m_jobs.contains(jobId)) return;

        auto& info = m_jobs[jobId];
        if (totalSteps > 0) info.totalSteps = totalSteps;
        info.currentStep = currentStep;
        if (!detail.isEmpty()) info.detail = detail;

        if (info.totalSteps > 0) {
            info.percent = qBound(0, static_cast<int>(static_cast<double>(info.currentStep) / info.totalSteps * 100.0), 100);
        }
    }

    QMetaObject::invokeMethod(this, &TaskProgressService::recalculateAndEmit, Qt::QueuedConnection);
}

void TaskProgressService::finishJob(int jobId) {
    bool wasEmpty = false;
    {
        QMutexLocker locker(&m_mutex);
        m_jobs.remove(jobId);
        wasEmpty = m_jobs.isEmpty();
    }

    QMetaObject::invokeMethod(this, [this, jobId, wasEmpty]() {
        emit jobFinished(jobId);
        recalculateAndEmit();
        if (wasEmpty) {
            emit allJobsFinished();
        }
    }, Qt::QueuedConnection);
}

void TaskProgressService::cancelJob(int jobId) {
    finishJob(jobId);
}

int TaskProgressService::activeJobCount() const {
    QMutexLocker locker(&m_mutex);
    return m_jobs.size();
}

bool TaskProgressService::hasActiveJobs() const {
    QMutexLocker locker(&m_mutex);
    return !m_jobs.isEmpty();
}

void TaskProgressService::recalculateAndEmit() {
    QMutexLocker locker(&m_mutex);
    if (m_jobs.isEmpty()) {
        emit progressUpdated(100, "", "", 0);
        return;
    }

    // 提取最新的或权重最大的活动任务展示
    const auto& lastJob = m_jobs.last();
    int activeCount = m_jobs.size();

    // 综合加权平均百分比
    double totalPercentSum = 0.0;
    for (const auto& job : m_jobs) {
        totalPercentSum += job.percent;
    }
    int averagePercent = static_cast<int>(totalPercentSum / activeCount);

    emit progressUpdated(averagePercent, lastJob.title, lastJob.detail, activeCount);
}

} // namespace QuarkMeta
```

---

## 3. `TaskProgressToolBar` 纯粹化改造

### 3.1 `src/ui/TaskProgressToolBar.h`
```cpp
#pragma once

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

namespace QuarkMeta {

class TaskProgressToolBar : public QWidget {
    Q_OBJECT

public:
    explicit TaskProgressToolBar(QWidget* parent = nullptr);
    ~TaskProgressToolBar() override = default;

private:
    void initUi();
    void bindService();

    QProgressBar* m_progressBar = nullptr;
    QLabel* m_lblTitle = nullptr;
    QLabel* m_lblDetail = nullptr;
    QLabel* m_lblCount = nullptr;
    QPushButton* m_btnCancel = nullptr;
};

} // namespace QuarkMeta
```

### 3.2 `src/ui/TaskProgressToolBar.cpp`
```cpp
#include "TaskProgressToolBar.h"
#include "../core/TaskProgressService.h"
#include "UiHelper.h"

namespace QuarkMeta {

TaskProgressToolBar::TaskProgressToolBar(QWidget* parent)
    : QWidget(parent) {
    initUi();
    bindService();
    hide(); // 默认无任务时处于隐藏状态
}

void TaskProgressToolBar::initUi() {
    setFixedHeight(36);
    setStyleSheet("QWidget { background-color: #252526; border-top: 1px solid #333333; }");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 0, 15, 0);
    layout->setSpacing(10);

    m_lblTitle = new QLabel("正在处理任务...", this);
    m_lblTitle->setStyleSheet("color: #EEEEEE; font-size: 11px; font-weight: bold; background: transparent;");

    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setRange(0, 100);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { background-color: #3E3E42; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background-color: #378ADD; border-radius: 3px; }"
    );

    m_lblDetail = new QLabel("", this);
    m_lblDetail->setStyleSheet("color: #888888; font-size: 11px; background: transparent;");

    m_lblCount = new QLabel("", this);
    m_lblCount->setStyleSheet("color: #378ADD; font-size: 11px; font-weight: bold; background: transparent;");

    layout->addWidget(m_lblTitle);
    layout->addWidget(m_progressBar, 1);
    layout->addWidget(m_lblDetail);
    layout->addWidget(m_lblCount);
}

void TaskProgressToolBar::bindService() {
    connect(&TaskProgressService::instance(), &TaskProgressService::jobStarted, this, [this](int, const QString& title) {
        m_lblTitle->setText(title);
        show();
    });

    connect(&TaskProgressService::instance(), &TaskProgressService::progressUpdated, this, 
            [this](int percent, const QString& title, const QString& detail, int activeCount) {
        m_progressBar->setValue(percent);
        if (!title.isEmpty()) m_lblTitle->setText(title);
        m_lblDetail->setText(detail);
        m_lblCount->setText(activeCount > 1 ? QString("(%1 项并发任务)").arg(activeCount) : "");
        if (!isVisible()) show();
    });

    connect(&TaskProgressService::instance(), &TaskProgressService::allJobsFinished, this, &QWidget::hide);
}

} // namespace QuarkMeta
```

---

## 4. `MainWindow.h` 与 `MainWindow.cpp` 净化

### 4.1 `MainWindow.h` 净化
- 删除 `class TaskProgressController;` 前置声明。
- 删除 `TaskProgressController* m_taskProgressController = nullptr;` 成员。

```cpp
// MainWindow.h 净化后关键区段：
#pragma once

#include <QMainWindow>
#include <QPointer>

namespace QuarkMeta {

class TaskProgressToolBar; // 仅保留工具栏组件声明
// 🚨 彻底删除 TaskProgressController 声明！
```

### 4.2 `MainWindow.cpp` 净化
- 彻底移除 `m_taskProgressController` 实例化代码。
- `TaskProgressToolBar` 保持放入底部布局，其内部已自动与单例服务绑定。

```cpp
// MainWindow.cpp 净化 setupSplitters() 末端：

void MainWindow::setupSplitters() {
    // ... [前置布局组装保持不变] ...

    m_taskProgressToolBar = new TaskProgressToolBar(centralC);
    // TaskProgressToolBar 内部已自动直连 TaskProgressService，0 行外部胶水代码！

    mainL->addWidget(m_titleBarWidget);
    mainL->addWidget(m_driveBarWidget);
    mainL->addWidget(m_navBarWidget);
    mainL->addWidget(bodyWrapper, 1);
    mainL->addWidget(m_statusBarWidget);
    mainL->addWidget(m_taskProgressToolBar);

    // 🚨 彻底删除：m_taskProgressController = new TaskProgressController(...) !

    setCentralWidget(centralC);
}
```

---

## 5. 物理清理与构建配置更新

### 5.1 物理删除废弃文件
- 删除 `src/ui/TaskProgressController.h`
- 删除 `src/ui/TaskProgressController.cpp`

### 5.2 `CMakeLists.txt` 构建配置注册
```cmake
set(CORE_SOURCES
    # ... 现有 core 源文件 ...
    src/core/TaskProgressService.h
    src/core/TaskProgressService.cpp
)

set(UI_SOURCES
    # ... 现有 UI 源文件 ...
    # 🚨 移除 TaskProgressController.h 与 TaskProgressController.cpp
    src/ui/TaskProgressToolBar.h
    src/ui/TaskProgressToolBar.cpp
)
```