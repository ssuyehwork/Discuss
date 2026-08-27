# Implementation Plan - TaskProgressController Refactoring (progress.md)

## 1. Overview
This implementation plan covers **Task 1: TaskProgressController Refactoring** from MainWindow.
The goal is to extract `m_topProgressBar`, `m_elapsedTimer`, `m_syncStartTime`, `m_totalBatchCount`, and `updateProgressBarGeometry()` from `MainWindow` into a dedicated controller class `QuarkMeta::TaskProgressController`.

Key features of this refactoring:
1. `TaskProgressController` is encapsulated in `src/ui/TaskProgressController.h` and `src/ui/TaskProgressController.cpp`.
2. Progress bar positioning is changed from manual geometry floating calculation (`updateProgressBarGeometry()`) to direct layout embedding in `bodyWrapper` layout top (`insertWidget(0, m_topProgressBar)`). This deprecates `updateProgressBarGeometry()`.
3. Countdown formatting (`formatTime`) and timeout calculation (`onTick`) are completely moved into `TaskProgressController`, keeping 100% logic and string format equivalence.
4. `TaskProgressController` and its implementation are registered in `CMakeLists.txt`.

---

## 2. Modified Files List
1. `CMakeLists.txt` (Add new source files `src/ui/TaskProgressController.h` and `src/ui/TaskProgressController.cpp`)
2. `src/ui/TaskProgressController.h` (New File)
3. `src/ui/TaskProgressController.cpp` (New File)
4. `src/ui/MainWindow.h` (Remove extracted 5 members/methods, add `TaskProgressController* m_taskProgressController`)
5. `src/ui/MainWindow.cpp` (Replace old progress bar construction/timer logic with `TaskProgressController`, remove `updateProgressBarGeometry()`)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`

```diff
<<<<<<< SEARCH
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
=======
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/TaskProgressController.h
    src/ui/TaskProgressController.cpp
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/TaskProgressController.h` (New File)

```cpp
#pragma once

#include <QObject>
#include <QProgressBar>
#include <QTimer>
#include <QLabel>
#include <QWidget>
#include <QDateTime>

namespace QuarkMeta {

class TaskProgressController : public QObject {
    Q_OBJECT
public:
    explicit TaskProgressController(QWidget* parentWidget, QWidget* anchorWidget, QLabel* statusLabel, QObject* parent = nullptr);
    ~TaskProgressController() override = default;

    void start(int totalCount);
    void updateProgress(int pct);
    void finish();

private slots:
    void onTick();

private:
    void formatTime(qint64 totalSeconds, QString& out) const;

    QProgressBar* m_topProgressBar = nullptr;
    QTimer* m_elapsedTimer = nullptr;
    qint64 m_syncStartTime = 0;
    int m_totalBatchCount = 0;
    QLabel* m_statusLabel = nullptr;
};

} // namespace QuarkMeta
```

---

### 3.3 `src/ui/TaskProgressController.cpp` (New File)

```cpp
#include "TaskProgressController.h"
#include "UiHelper.h"
#include <QVBoxLayout>
#include <algorithm>

namespace QuarkMeta {

TaskProgressController::TaskProgressController(QWidget* parentWidget, QWidget* anchorWidget, QLabel* statusLabel, QObject* parent)
    : QObject(parent), m_statusLabel(statusLabel) {
    Q_UNUSED(anchorWidget);

    if (parentWidget) {
        m_topProgressBar = new QProgressBar(parentWidget);
        m_topProgressBar->setFixedHeight(5);
        m_topProgressBar->setTextVisible(false);
        m_topProgressBar->setRange(0, 100);
        m_topProgressBar->setInvertedAppearance(false);
        m_topProgressBar->setStyleSheet(QString(
            "QProgressBar { background: transparent; border: none; max-height: 5px; }"
            "QProgressBar::chunk { background-color: %1; border-radius: 1px; }"
        ).arg(qssColor(PrimaryBlue)));
        m_topProgressBar->hide();

        if (QVBoxLayout* bodyLayout = qobject_cast<QVBoxLayout*>(parentWidget->layout())) {
            bodyLayout->insertWidget(0, m_topProgressBar);
        }
    }

    m_elapsedTimer = new QTimer(this);
    m_elapsedTimer->setInterval(100);
    connect(m_elapsedTimer, &QTimer::timeout, this, &TaskProgressController::onTick);
}

void TaskProgressController::start(int totalCount) {
    m_syncStartTime = QDateTime::currentMSecsSinceEpoch();
    m_totalBatchCount = totalCount;
    if (m_topProgressBar) {
        m_topProgressBar->setValue(0);
        m_topProgressBar->show();
    }
    if (m_elapsedTimer) {
        m_elapsedTimer->start();
    }
}

void TaskProgressController::updateProgress(int pct) {
    if (m_topProgressBar) {
        m_topProgressBar->setValue(pct);
    }
}

void TaskProgressController::finish() {
    if (m_elapsedTimer) {
        m_elapsedTimer->stop();
    }
    if (m_topProgressBar) {
        m_topProgressBar->hide();
    }
    m_syncStartTime = 0;
    m_totalBatchCount = 0;
}

void TaskProgressController::formatTime(qint64 totalSeconds, QString& out) const {
    if (totalSeconds < 0) totalSeconds = 0;
    qint64 hours = totalSeconds / 3600;
    qint64 mins = (totalSeconds % 3600) / 60;
    qint64 secs = totalSeconds % 60;
    if (hours > 0) {
        out = QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        out = QString("%1:%2")
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
}

void TaskProgressController::onTick() {
    if (!m_statusLabel || !m_topProgressBar) return;

    if (m_syncStartTime > 0 && m_totalBatchCount > 0) {
        double elapsedSec = (QDateTime::currentMSecsSinceEpoch() - m_syncStartTime) / 1000.0;
        int currentPct = m_topProgressBar->value();
        
        int completedCount = std::clamp((int)((double)currentPct / 100.0 * m_totalBatchCount), 0, m_totalBatchCount);

        QString countdownStr = "00:00";
        QString totalEstStr = "00:00";

        if (currentPct >= 5) {
            qint64 remainingSec = static_cast<qint64>(elapsedSec * (100.0 - currentPct) / (double)currentPct);
            qint64 totalEstSec = static_cast<qint64>(elapsedSec) + remainingSec;
            formatTime(remainingSec, countdownStr);
            formatTime(totalEstSec, totalEstStr);
        }

        m_statusLabel->setText(QString("扫描数据中... %1%  数量：%2/%3  |  倒计时分 %4 / 预计时分: %5")
                              .arg(currentPct)
                              .arg(completedCount)
                              .arg(m_totalBatchCount)
                              .arg(countdownStr)
                              .arg(totalEstStr));
    }
}

} // namespace QuarkMeta
```

---

### 3.4 `src/ui/MainWindow.h`

```diff
<<<<<<< SEARCH
class AddressBar;
class TaskProgressToolBar;
=======
class AddressBar;
class TaskProgressToolBar;
class TaskProgressController;
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    GlobalShortcutController* m_shortcutController = nullptr;
    PanelMediator* m_panelMediator = nullptr;

    void updateProgressBarGeometry(); // 实时计算 5px 悬浮位置函数

    QProgressBar* m_topProgressBar = nullptr; // 悬浮覆盖层进度条
    QTimer* m_elapsedTimer = nullptr;         // 耗时刷新定时器
    qint64 m_syncStartTime = 0;               // 任务开始毫秒时间戳
    int m_totalBatchCount = 0;                // 当前批次扫描的任务总项数
=======
    GlobalShortcutController* m_shortcutController = nullptr;
    PanelMediator* m_panelMediator = nullptr;
    TaskProgressController* m_taskProgressController = nullptr;
>>>>>>> REPLACE
```

---

### 3.5 `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
#include "GlobalShortcutController.h"
#include "PanelMediator.h"
=======
#include "GlobalShortcutController.h"
#include "PanelMediator.h"
#include "TaskProgressController.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    m_elapsedTimer = new QTimer(this);
    m_elapsedTimer->setInterval(100);

    auto formatTime = [](qint64 totalSeconds) -> QString {
        if (totalSeconds < 0) totalSeconds = 0;
        qint64 hours = totalSeconds / 3600;
        qint64 mins = (totalSeconds % 3600) / 60;
        qint64 secs = totalSeconds % 60;
        if (hours > 0) {
            return QString("%1:%2:%3")
                .arg(hours, 2, 10, QChar('0'))
                .arg(mins, 2, 10, QChar('0'))
                .arg(secs, 2, 10, QChar('0'));
        }
        return QString("%1:%2")
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    };

    connect(m_elapsedTimer, &QTimer::timeout, this, [this, formatTime]() {
        if (m_syncStartTime > 0 && m_totalBatchCount > 0) {
            double elapsedSec = (QDateTime::currentMSecsSinceEpoch() - m_syncStartTime) / 1000.0;
            int currentPct = m_topProgressBar->value();
            
            int completedCount = qBound(0, (int)((double)currentPct / 100.0 * m_totalBatchCount), m_totalBatchCount);

            QString countdownStr = "00:00";
            QString totalEstStr = "00:00";

            if (currentPct >= 5) {
                qint64 remainingSec = static_cast<qint64>(elapsedSec * (100.0 - currentPct) / (double)currentPct);
                qint64 totalEstSec = static_cast<qint64>(elapsedSec) + remainingSec;
                countdownStr = formatTime(remainingSec);
                totalEstStr = formatTime(totalEstSec);
            }

            m_statusLeft->setText(QString("扫描数据中... %1%  数量：%2/%3  |  倒计时分 %4 / 预计时分: %5")
                                  .arg(currentPct)
                                  .arg(completedCount)
                                  .arg(m_totalBatchCount)
                                  .arg(countdownStr)
                                  .arg(totalEstStr));
        }
    });
=======
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    mainL->addWidget(m_titleBarWidget);
    mainL->addWidget(m_driveBarWidget);
    mainL->addWidget(m_navBarWidget);
    mainL->addWidget(bodyWrapper, 1);
    mainL->addWidget(m_statusBarWidget);
    mainL->addWidget(m_taskProgressToolBar);

    m_topProgressBar = new QProgressBar(centralC);
    m_topProgressBar->setFixedHeight(5);
    m_topProgressBar->setTextVisible(false);
    m_topProgressBar->setRange(0, 100);
    m_topProgressBar->setInvertedAppearance(false);
    m_topProgressBar->setStyleSheet(QString(
        "QProgressBar { background: transparent; border: none; max-height: 5px; }"
        "QProgressBar::chunk { background-color: %1; border-radius: 1px; }"
    ).arg(qssColor(PrimaryBlue)));
    m_topProgressBar->hide();
=======
    mainL->addWidget(m_titleBarWidget);
    mainL->addWidget(m_driveBarWidget);
    mainL->addWidget(m_navBarWidget);
    mainL->addWidget(bodyWrapper, 1);
    mainL->addWidget(m_statusBarWidget);
    mainL->addWidget(m_taskProgressToolBar);

    m_taskProgressController = new TaskProgressController(bodyWrapper, m_statusBarWidget, m_statusLeft, this);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    updateProgressBarGeometry();
}

void MainWindow::updateProgressBarGeometry() {
    if (!m_topProgressBar || !m_mainSplitter || !m_statusLeft) return;

    QWidget* bodyWrapper = m_mainSplitter->parentWidget();
    QWidget* statusBar = m_statusLeft->parentWidget();

    if (bodyWrapper && statusBar) {
        int x = bodyWrapper->geometry().left();
        int y = statusBar->geometry().top() - 5;
        int width = bodyWrapper->geometry().width();

        m_topProgressBar->setGeometry(x, y, width, 5);
        m_topProgressBar->raise();
    }
}
=======
void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### 4.1 Build Verification
Execute standard CMake build:
```bash
cmake --build build --config Debug
```
Confirm zero compilation errors and zero MOC undefined symbol link errors for `TaskProgressController`.

### 4.2 Behavior Verification
1. Launch app and start a background scan task.
2. Confirm progress bar is visible above `bodyWrapper` content area (5px height).
3. Confirm status bar displays scan count and countdown strings formatted identically (`00:00` / `00:00:00`).
4. Resize main window; confirm progress bar automatically repositions and resizes along with `bodyWrapper` without layout misalignment or lag.
