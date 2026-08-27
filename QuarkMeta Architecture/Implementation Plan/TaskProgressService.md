# TaskProgressService Implementation Plan

## 1. Overview
This implementation plan establishes `TaskProgressService` as a thread-safe, centralized background task progress service located in `src/core/`. It unifies job creation, progress updates, and completion notifications across arbitrary worker threads. 
Simultaneously, it physically purges the legacy UI-side `TaskProgressController` (`src/ui/TaskProgressController.h/cpp`), converts `TaskProgressToolBar` (`src/ui/TaskProgressToolBar.h/cpp`) into a pure observer listening to `TaskProgressService` signals for auto-show/hide behavior, and cleans up host pointers in `MainWindow.h/cpp`.

---

## 2. Modified Files List
- `src/core/TaskProgressService.h` *(New)*
- `src/core/TaskProgressService.cpp` *(New)*
- `src/ui/TaskProgressController.h` *(Deleted)*
- `src/ui/TaskProgressController.cpp` *(Deleted)*
- `src/ui/TaskProgressToolBar.h` *(Modified)*
- `src/ui/TaskProgressToolBar.cpp` *(Modified)*
- `src/ui/MainWindow.h` *(Modified)*
- `src/ui/MainWindow.cpp` *(Modified)*
- `CMakeLists.txt` *(Modified)*

---

## 3. Detailed Line-by-Line Changes

### 3.1 Create `src/core/TaskProgressService.h`
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

    int createJob(const QString& title, int totalSteps = 100);
    void updateProgress(int jobId, int currentStep, int totalSteps = -1, const QString& detail = "");
    void finishJob(int jobId);
    void cancelJob(int jobId);

    int activeJobCount() const;
    bool hasActiveJobs() const;

signals:
    void jobStarted(int jobId, const QString& title);
    void progressUpdated(int percent, const QString& title, const QString& detail, int activeCount);
    void jobFinished(int jobId);
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

### 3.2 Create `src/core/TaskProgressService.cpp`
```cpp
#include "TaskProgressService.h"
#include <QMutexLocker>
#include <QCoreApplication>
#include <algorithm>

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
            info.percent = std::clamp(static_cast<int>(static_cast<double>(info.currentStep) / info.totalSteps * 100.0), 0, 100);
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
    int activeCount = 0;
    QString lastTitle;
    QString lastDetail;
    int averagePercent = 100;

    {
        QMutexLocker locker(&m_mutex);
        if (m_jobs.isEmpty()) {
            emit progressUpdated(100, "", "", 0);
            return;
        }

        const auto& lastJob = m_jobs.last();
        activeCount = m_jobs.size();
        lastTitle = lastJob.title;
        lastDetail = lastJob.detail;

        double totalPercentSum = 0.0;
        for (const auto& job : m_jobs) {
            totalPercentSum += job.percent;
        }
        averagePercent = static_cast<int>(totalPercentSum / activeCount);
    }

    emit progressUpdated(averagePercent, lastTitle, lastDetail, activeCount);
}

} // namespace QuarkMeta
```

### 3.3 Update `src/ui/TaskProgressToolBar.h`
```
<<<<<<< SEARCH
class TaskProgressToolBar : public QWidget {
    Q_OBJECT
public:
    explicit TaskProgressToolBar(QWidget* parent = nullptr);

    void updateProgress(int processed, int total, int remainingSeconds);
    void showCompleted(int processed, int total);

signals:
    void cancelRequested();

private:
    QLabel* m_lblStatus = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_lblTime = nullptr;
    QPushButton* m_btnCancel = nullptr;
};
=======
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
>>>>>>> REPLACE
```

### 3.4 Update `src/ui/TaskProgressToolBar.cpp`
```
<<<<<<< SEARCH
TaskProgressToolBar::TaskProgressToolBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(28);
    setStyleSheet("background-color: #252526; border-top: 1px solid #333; color: #CCC;");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(10);

    m_lblStatus = new QLabel("正在导入项目...", this);
    m_lblStatus->setStyleSheet("font-size: 11px;");
    layout->addWidget(m_lblStatus);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setRange(0, 100);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { background: #333; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: #378ADD; border-radius: 3px; }"
    );
    layout->addWidget(m_progressBar, 1);

    m_lblTime = new QLabel("计算中...", this);
    m_lblTime->setStyleSheet("font-size: 11px; color: #888;");
    layout->addWidget(m_lblTime);

    m_btnCancel = new QPushButton("×", this);
    m_btnCancel->setFixedSize(16, 16);
    m_btnCancel->setCursor(Qt::PointingHandCursor);
    m_btnCancel->setStyleSheet("QPushButton { border: none; color: #888; font-weight: bold; } QPushButton:hover { color: #FFF; }");
    layout->addWidget(m_btnCancel);

    connect(m_btnCancel, &QPushButton::clicked, this, &TaskProgressToolBar::cancelRequested);
}

void TaskProgressToolBar::updateProgress(int processed, int total, int remainingSeconds) {
    if (total <= 0) return;
    int pct = static_cast<int>((double)processed / total * 100.0);
    m_progressBar->setValue(pct);
    m_lblStatus->setText(QString("正在导入项目 (%1/%2)...").arg(processed).arg(total));

    if (remainingSeconds >= 0) {
        m_lblTime->setText(QString("剩余约 %1 秒").arg(remainingSeconds));
    } else {
        m_lblTime->setText("计算中...");
    }
}

void TaskProgressToolBar::showCompleted(int processed, int total) {
    m_progressBar->setValue(100);
    m_lblStatus->setText(QString("处理完成 (%1/%2)").arg(processed).arg(total));
    m_lblTime->setText("已就绪");
}
=======
TaskProgressToolBar::TaskProgressToolBar(QWidget* parent)
    : QWidget(parent) {
    initUi();
    bindService();
    hide();
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
>>>>>>> REPLACE
```

### 3.5 Clean Up `src/ui/MainWindow.h`
```
<<<<<<< SEARCH
class TaskProgressToolBar;
class TaskProgressController;
=======
class TaskProgressToolBar;
>>>>>>> REPLACE
```
```
<<<<<<< SEARCH
    TaskProgressToolBar* m_taskProgressToolBar = nullptr;
    TaskProgressController* m_taskProgressController = nullptr;
=======
    TaskProgressToolBar* m_taskProgressToolBar = nullptr;
>>>>>>> REPLACE
```

### 3.6 Clean Up `src/ui/MainWindow.cpp`
```
<<<<<<< SEARCH
#include "TaskProgressController.h" 
=======
>>>>>>> REPLACE
```
```
<<<<<<< SEARCH
    m_taskProgressToolBar = new TaskProgressToolBar(centralC);

    mainL->addWidget(m_titleBarWidget);
    mainL->addWidget(m_driveBarWidget);
    mainL->addWidget(m_navBarWidget);
    mainL->addWidget(bodyWrapper, 1);
    mainL->addWidget(m_statusBarWidget);
    mainL->addWidget(m_taskProgressToolBar);

    m_taskProgressController = new TaskProgressController(bodyWrapper, m_statusBarWidget, m_statusLeft, this);
=======
    m_taskProgressToolBar = new TaskProgressToolBar(centralC);

    mainL->addWidget(m_titleBarWidget);
    mainL->addWidget(m_driveBarWidget);
    mainL->addWidget(m_navBarWidget);
    mainL->addWidget(bodyWrapper, 1);
    mainL->addWidget(m_statusBarWidget);
    mainL->addWidget(m_taskProgressToolBar);
>>>>>>> REPLACE
```

### 3.7 Update `CMakeLists.txt`
```
<<<<<<< SEARCH
    src/ui/TaskProgressController.h
    src/ui/TaskProgressController.cpp
=======
>>>>>>> REPLACE
```
```
<<<<<<< SEARCH
    src/core/ClipboardService.h
    src/core/ClipboardService.cpp
=======
    src/core/ClipboardService.h
    src/core/ClipboardService.cpp
    src/core/TaskProgressService.h
    src/core/TaskProgressService.cpp
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Verify `QuarkMeta-Architecture-Planning.md` contains the updated top-level task progress architecture specifications.
2. Verify `TaskProgressService.md` is strictly placed under `QuarkMeta Architecture/Implementation Plan/` with precise 1:1 class name mapping.
3. Run pre-commit instructions checks and submit.
