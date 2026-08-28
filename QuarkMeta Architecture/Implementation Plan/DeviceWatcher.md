# DeviceWatcher Implementation Plan

## 1. Overview
This implementation plan encapsulates Windows hardware device plug/unplug messaging (`WM_DEVICECHANGE`) and volume mask (`dbcv_unitmask`) parsing into a standalone `DeviceWatcher` service (`src/core/DeviceWatcher.h/cpp`) inheriting from `QAbstractNativeEventFilter`.
It purges `handleDeviceChange` from `CoreController.h/cpp`, connects `NavigationService` directly to `DeviceWatcher::driveUnmounted` to achieve self-contained auto-fallback on volume removal, and completely strips `nativeEvent`, `onVolumeUnplugged`, and raw Win32 headers (`<Dbt.h>`, `<windows.h>`, `<psapi.h>`) from `MainWindow.h/cpp` to achieve zero Win32 lines in `MainWindow`.

---

## 2. Modified Files List
- `src/core/DeviceWatcher.h` *(New)*
- `src/core/DeviceWatcher.cpp` *(New)*
- `src/core/NavigationService.cpp` *(Modified)*
- `src/core/CoreController.h` *(Modified)*
- `src/core/CoreController.cpp` *(Modified)*
- `src/ui/MainWindow.h` *(Modified)*
- `src/ui/MainWindow.cpp` *(Modified)*
- `CMakeLists.txt` *(Modified)*

---

## 3. Detailed Line-by-Line Changes

### 3.1 Create `src/core/DeviceWatcher.h`
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

    void startListening();
    void stopListening();

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void driveMounted(const QString& driveLetter);
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

### 3.2 Create `src/core/DeviceWatcher.cpp`
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
        if (QCoreApplication::instance()) {
            QCoreApplication::instance()->installNativeEventFilter(this);
            m_isListening = true;
        }
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

### 3.3 Update `src/core/NavigationService.cpp`
```
<<<<<<< SEARCH
NavigationService::NavigationService(QObject* parent) : QObject(parent) {
=======
#include "DeviceWatcher.h"

NavigationService::NavigationService(QObject* parent) : QObject(parent) {
    connect(&DeviceWatcher::instance(), &DeviceWatcher::driveUnmounted, this, [this](const QString& driveLetter) {
        if (m_currentUrl.contains(driveLetter, Qt::CaseInsensitive)) {
            navigateTo("computer://");
        }
    });
>>>>>>> REPLACE
```

### 3.4 Purge `src/core/CoreController.h`
```
<<<<<<< SEARCH
    void handleDeviceChange(unsigned long wParam, unsigned long long lParam);
=======
>>>>>>> REPLACE
```

### 3.5 Purge `src/core/CoreController.cpp`
```
<<<<<<< SEARCH
#include "CoreController.h"
=======
#include "CoreController.h"
#include "DeviceWatcher.h"
>>>>>>> REPLACE
```
```
<<<<<<< SEARCH
void CoreController::initializeCoreComponents() {
    // 强制注册全局单例与日志追踪
=======
void CoreController::initializeCoreComponents() {
    DeviceWatcher::instance().startListening();
    // 强制注册全局单例与日志追踪
>>>>>>> REPLACE
```
```
<<<<<<< SEARCH
void CoreController::handleDeviceChange(unsigned long wParam, unsigned long long lParam) {
    Q_UNUSED(wParam);
    Q_UNUSED(lParam);
    // 硬件状态变动留空点位
}
=======
>>>>>>> REPLACE
```

### 3.6 Purge `src/ui/MainWindow.h`
```
<<<<<<< SEARCH
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

private slots:
    void onPinToggled(bool checked);
    void onStatusBarStatsUpdated(int fileCount, int folderCount, int totalCount);
    void onVolumeUnplugged(const QString& driveLetter);
=======
private slots:
    void onPinToggled(bool checked);
    void onStatusBarStatsUpdated(int fileCount, int folderCount, int totalCount);
>>>>>>> REPLACE
```

### 3.7 Purge `src/ui/MainWindow.cpp`
```
<<<<<<< SEARCH
#ifdef Q_OS_WIN
#include <windows.h>
#include <Dbt.h>
#include <psapi.h>
#endif
=======
>>>>>>> REPLACE
```
```
<<<<<<< SEARCH
#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    MSG* msg = static_cast<MSG*>(message);
    if (msg->message == WM_DEVICECHANGE) {
        CoreController::instance().handleDeviceChange(static_cast<unsigned long>(msg->wParam), static_cast<unsigned long long>(msg->lParam));

        if (msg->wParam == DBT_DEVICEREMOVECOMPLETE) {
            PDEV_BROADCAST_HDR pHdr = reinterpret_cast<PDEV_BROADCAST_HDR>(msg->lParam);
            if (pHdr && pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                PDEV_BROADCAST_VOLUME pVol = reinterpret_cast<PDEV_BROADCAST_VOLUME>(msg->lParam);
                DWORD unitmask = pVol->dbcv_unitmask;

                for (char i = 0; i < 26; ++i) {
                    if (unitmask & (1 << i)) {
                        QString driveLetter = QString("%1:").arg(static_cast<char>('A' + i));
                        onVolumeUnplugged(driveLetter);
                    }
                }
            }
        }
    }
    return false;
}
#endif
=======
>>>>>>> REPLACE
```
```
<<<<<<< SEARCH
void MainWindow::onVolumeUnplugged(const QString& driveLetter) {
    if (m_addressBar && m_addressBar->text().contains(driveLetter, Qt::CaseInsensitive)) {
        m_addressBar->setText("computer://");
        unifiedNavigateTo("computer://");
    }
}
=======
>>>>>>> REPLACE
```

### 3.8 Update `CMakeLists.txt`
```
<<<<<<< SEARCH
    src/core/ClipboardService.h
    src/core/ClipboardService.cpp
=======
    src/core/ClipboardService.h
    src/core/ClipboardService.cpp
    src/core/DeviceWatcher.h
    src/core/DeviceWatcher.cpp
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Verify `QuarkMeta-Architecture-Planning.md` contains the updated `DeviceWatcher` architecture specification.
2. Verify `DeviceWatcher.md` is created strictly under `QuarkMeta Architecture/Implementation Plan/` with precise 1:1 class name mapping.
3. Run pre-commit instructions checks and submit.
