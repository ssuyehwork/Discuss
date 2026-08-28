# QuarkMeta 硬件设备监听解耦与主窗口净化实施方案 (DeviceWatcher)

## 1. 目标与范围
- 新建 `DeviceWatcher`（位于 `src/core/`）：基于 `QAbstractNativeEventFilter` 独立封装 Windows 硬件设备热插拔消息（`WM_DEVICECHANGE`）的底层截获与盘符掩码（`dbcv_unitmask`）解析，对外广播标准 Qt 信号。
- 净化 `CoreController.h/cpp`：彻底移除依赖 Win32 原始参数的 `handleDeviceChange(unsigned long, unsigned long long)` 非标接口。
- 完善 `NavigationService` 自治闭环：由导航服务直接监听拔盘信号并自动安全退回 `computer://`。
- 彻底净化 `MainWindow.h/cpp`：清除 `nativeEvent` 虚函数、`onVolumeUnplugged` 槽函数以及 `<Dbt.h>`、`<psapi.h>`、`<windows.h>` 等所有 Win32 底层头文件，使主窗口达成 **0 行 Win32 代码** 的 100% 跨平台纯洁性。

---

## 2. 核心模块独立实现

### 2.1 `src/core/DeviceWatcher.h`
```cpp
#pragma once

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QString>

namespace QuarkMeta {

class DeviceWatcher : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    static DeviceWatcher& instance();

    /**
     * @brief 启动全局原生硬件设备监听
     */
    void startListening();

    /**
     * @brief 停止全局原生硬件设备监听
     */
    void stopListening();

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    /**
     * @brief 盘符插入/挂载广播 (如: "D:")
     */
    void driveMounted(const QString& driveLetter);

    /**
     * @brief 盘符拔出/卸载广播 (如: "D:")
     */
    void driveUnmounted(const QString& driveLetter);

private:
    explicit DeviceWatcher(QObject* parent = nullptr);
    ~DeviceWatcher() override;
    DeviceWatcher(const DeviceWatcher&) = delete;
    DeviceWatcher& operator=(const DeviceWatcher&) = delete;

    bool m_isListening = false;
};

} // namespace QuarkMeta
```

### 2.2 `src/core/DeviceWatcher.cpp`
```cpp
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "DeviceWatcher.h"
#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dbt.h>
#endif

namespace QuarkMeta {

DeviceWatcher& DeviceWatcher::instance() {
    static DeviceWatcher s_instance;
    return s_instance;
}

DeviceWatcher::DeviceWatcher(QObject* parent) : QObject(parent) {}

DeviceWatcher::~DeviceWatcher() {
    stopListening();
}

void DeviceWatcher::startListening() {
    if (!m_isListening) {
        QCoreApplication::instance()->installNativeEventFilter(this);
        m_isListening = true;
    }
}

void DeviceWatcher::stopListening() {
    if (m_isListening) {
        if (QCoreApplication::instance()) {
            QCoreApplication::instance()->removeNativeEventFilter(this);
        }
        m_isListening = false;
    }
}

bool DeviceWatcher::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) {
    Q_UNUSED(eventType);
    Q_UNUSED(result);

#ifdef Q_OS_WIN
    MSG* msg = static_cast<MSG*>(message);
    if (msg && msg->message == WM_DEVICECHANGE) {
        WPARAM wParam = msg->wParam;
        LPARAM lParam = msg->lParam;

        if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
            PDEV_BROADCAST_HDR pHdr = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
            if (pHdr && pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                PDEV_BROADCAST_VOLUME pVol = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lParam);
                DWORD unitmask = pVol->dbcv_unitmask;

                for (char i = 0; i < 26; ++i) {
                    if (unitmask & (1 << i)) {
                        QString driveLetter = QString("%1:").arg(static_cast<char>('A' + i));
                        if (wParam == DBT_DEVICEARRIVAL) {
                            emit driveMounted(driveLetter);
                        } else {
                            emit driveUnmounted(driveLetter);
                        }
                    }
                }
            }
        }
    }
#endif

    return false;
}

} // namespace QuarkMeta
```

---

## 3. `NavigationService.cpp` 自治闭环

在 `NavigationService` 构造函数中，直接监听 `DeviceWatcher::driveUnmounted` 信号，完成热拔盘时的自动安全退回：

```cpp
// src/core/NavigationService.cpp 构造函数中追加：
#include "DeviceWatcher.h"

NavigationService::NavigationService(QObject* parent) : QObject(parent) {
    // 🚀【自动拔盘防护】：当前所在盘符被拔出时，自动安全回退到“此电脑”
    connect(&DeviceWatcher::instance(), &DeviceWatcher::driveUnmounted, this, [this](const QString& driveLetter) {
        if (m_currentUrl.contains(driveLetter, Qt::CaseInsensitive)) {
            navigateTo("computer://");
        }
    });
}
```

---

## 4. `CoreController.h` 与 `CoreController.cpp` 净化

### 4.1 `CoreController.h` 净化
- 删除 `handleDeviceChange` 接口声明。

```cpp
// CoreController.h 净化后：
public:
    static CoreController& instance();
    static void initializeCoreComponents();
    static void requestShutdown();
    static bool isShuttingDown();
    static uint64_t incrementNavigationGeneration();
    static uint64_t currentNavigationGeneration();

    void startSystem();
    bool isIndexing() const { return m_isIndexing; }
    QString statusText() const { return m_statusText; }

    void performSearch(const QString& keyword, const QString& scopeSource = "", int categoryId = 0, const QString& parentPath = "");
    void abortSearch();

    // 🚨 彻底删除 handleDeviceChange(unsigned long wParam, unsigned long long lParam);
```

### 4.2 `CoreController.cpp` 净化
- 删除 `handleDeviceChange` 实现代码。
- 在 `initializeCoreComponents()` 中启动 `DeviceWatcher`。

```cpp
#include "DeviceWatcher.h"

void CoreController::initializeCoreComponents() {
    // 启动全局硬件设备监听器
    DeviceWatcher::instance().startListening();
    // ... 其余初始化保持不变 ...
}

// 🚨 彻底删除 CoreController::handleDeviceChange 整个函数体！
```

---

## 5. `MainWindow.h` 与 `MainWindow.cpp` 彻底脱敏（达成 0 行 Win32 代码）

### 5.1 `MainWindow.h` 净化
- 删除 `nativeEvent` 虚函数覆盖。
- 删除 `onVolumeUnplugged` 槽函数。

```cpp
// MainWindow.h 净化后关键区段：
protected:
    void keyPressEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

    // 🚨 彻底删除 #ifdef Q_OS_WIN bool nativeEvent(...) #endif !

private slots:
    void onPinToggled(bool checked);
    void onStatusBarStatsUpdated(int fileCount, int folderCount, int totalCount);

    // 🚨 彻底删除 void onVolumeUnplugged(const QString& driveLetter);
```

### 5.2 `MainWindow.cpp` 净化
- 删除头部包含的 `<windows.h>`、`<Dbt.h>`、`<psapi.h>`。
- 删除 `MainWindow::nativeEvent` 与 `MainWindow::onVolumeUnplugged` 实现。

```cpp
#include "MainWindow.h"
// ... 标准头文件包含 ...

// 🚨 彻底删除：
// #ifdef Q_OS_WIN
// #include <windows.h>
// #include <Dbt.h>
// #include <psapi.h>
// #endif

// 🚨 彻底删除 MainWindow::nativeEvent 函数体！
// 🚨 彻底删除 MainWindow::onVolumeUnplugged 函数体！
```

---

## 6. `CMakeLists.txt` 构建配置注册
```cmake
set(CORE_SOURCES
    # ... 现有 core 源文件 ...
    src/core/DeviceWatcher.h
    src/core/DeviceWatcher.cpp
)
```