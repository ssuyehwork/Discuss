# 彻底拔除 IOCP 监控与 NativeFolderWatcher 僵尸代码无脑实施方案 —— PurgeIocpWatcherZombieCode

本实施方案旨在对 QuarkMeta 纯磁盘直连模式下的 IOCP 监控及 `NativeFolderWatcher` 历史僵尸代码进行彻底的物理拔除，释放其在后台开辟的 CPU 线程池与定时器资源，实现底层零垃圾、零空转。

---

## 清理文件清单

### 1. 需彻底删除的文件
- `src/core/NativeFolderWatcher.h`
- `src/core/NativeFolderWatcher.cpp`

### 2. 需物理修改与清洗关联引用的文件
- `CMakeLists.txt`
- `src/core/CoreController.cpp`
- `src/core/SystemBootstrapper.h`
- `src/core/SystemBootstrapper.cpp`
- `src/ui/MainWindow.cpp`

---

## 阶段一：彻底物理删除僵尸源码文件

彻底删除以下两个已经无任何实际监控业务的 IOCP 代码文件：
1. `src/core/NativeFolderWatcher.h`
2. `src/core/NativeFolderWatcher.cpp`

---

## 阶段二：从构建工程文件（CMakeLists.txt）中彻底拔除

### 1. 修改 `CMakeLists.txt`
**修改文件**：`CMakeLists.txt`
**修改目的**：从构建列表中剔除 `NativeFolderWatcher.cpp` 和 `NativeFolderWatcher.h`。

**精准替换 Diff**：
```cmake
<<<<<<< SEARCH
    src/core/NativeFolderWatcher.cpp
    src/core/NativeFolderWatcher.h
=======
>>>>>>> REPLACE
```

---

## 阶段三：清理业务控制器与系统引导器中的僵尸逻辑

### 2. 修改 `src/core/CoreController.cpp`
**修改文件**：`src/core/CoreController.cpp`
**修改目的**：删除头文件引用 `#include "NativeFolderWatcher.h"`、彻底移除构造函数中对 `NativeFolderWatcher::instance()` 信号槽的无效绑定，以及删除 `startSystem()` 中死掉的 IOCP 循环遍历逻辑。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
#include "NativeFolderWatcher.h"
=======
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
CoreController::CoreController(QObject* parent) : QObject(parent) {
    // [Plan-115] 注册 Qt 元类型，防止 QueuedConnection 因未注册自定义类型而分发失败
    qRegisterMetaType<QList<QuarkMeta::FileWatcherEvent>>("QList<QuarkMeta::FileWatcherEvent>");

    // [Plan-115] 绑定 NativeFolderWatcher 纯净自定义批次变动信号到具体业务单例，彻底断开两端硬编码耦合
    connect(&NativeFolderWatcher::instance(), &NativeFolderWatcher::filesChanged, this, [this](const QList<QuarkMeta::FileWatcherEvent>& events) {
        for (const auto& ev : events) {
            std::wstring normNewPath = MetadataManager::normalizePath(ev.newPath.toStdWString());
            QString qNewPath = QString::fromStdWString(normNewPath);

            // 常规文件的物理磁盘变动响应逻辑
            if (ev.action == QuarkMeta::WatcherAction::Added || ev.action == QuarkMeta::WatcherAction::Modified) {
                if (!ev.isDirectory) {
                    MetadataManager::instance().registerItemsAsync(QStringList() << ev.newPath, true);
                }
            } else if (ev.action == QuarkMeta::WatcherAction::Removed) {
                emit NativeFolderWatcher::instance().managedFolderRemoved(normNewPath);
                MetadataManager::instance().removeMetadataSync(normNewPath);
            } else if (ev.action == QuarkMeta::WatcherAction::Renamed) {
                std::wstring normOldPath = MetadataManager::normalizePath(ev.oldPath.toStdWString());
                MetadataManager::instance().syncAfterMove(normOldPath, normNewPath);
            }
        }
    }, Qt::QueuedConnection);
}
=======
CoreController::CoreController(QObject* parent) : QObject(parent) {
}
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
            // 启动原生监控服务 (对应用户原话："采用NativeFolderWatcher (IOCP) 机制的方式")
            // 资源库无需开启 IOCP 监控（已取消）
            const auto drives = QDir::drives();
            for (const QFileInfo& d : drives) {
                std::wstring wPath = d.absolutePath().toStdWString();
                std::wstring volSerial = MetadataManager::getVolumeSerialNumber(wPath);
                QString letter = d.absolutePath().left(1).toUpper();

                if (volSerial != L"UNKNOWN") {
                    std::wstring managedAbsW = MetadataManager::getManagedLibraryPath(volSerial, letter);
                    if (!managedAbsW.empty()) {
                        // NativeFolderWatcher::instance().addWatch(managedAbsW);
                    }
                }
            }
=======
>>>>>>> REPLACE
```

---

### 3. 修改 `src/core/SystemBootstrapper.h` & `src/core/SystemBootstrapper.cpp`
**修改文件**：`src/core/SystemBootstrapper.h` 与 `src/core/SystemBootstrapper.cpp`
**修改目的**：清理 `SystemBootstrapper` 中关于 IOCP 监控卡死与点火的遗留垃圾。

**`SystemBootstrapper.h` 精准替换 Diff**：
```cpp
<<<<<<< SEARCH
    /**
     * @brief 驱动多盘符资源库并开启底层 NativeFolderWatcher IOCP 监控 (从 MainWindow 移出)
     */
    void bootstrapMonitors();
=======
>>>>>>> REPLACE
```

**`SystemBootstrapper.cpp` 精准替换 Diff**：
```cpp
<<<<<<< SEARCH
#include "NativeFolderWatcher.h"
=======
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
void SystemBootstrapper::bootstrapMonitors() {
    qDebug() << "[Boot] SystemBootstrapper 开始点火底层 IOCP 监控...";
    const auto drives = QDir::drives();
    for (const QFileInfo& d : drives) {
        std::wstring wPath = d.absolutePath().toStdWString();
        std::wstring volSerial = MetadataManager::getVolumeSerialNumber(wPath);
        QString letter = d.absolutePath().left(1).toUpper();

        if (volSerial != L"UNKNOWN") {
            std::wstring managedAbsW = MetadataManager::getManagedLibraryPath(volSerial, letter);
            if (!managedAbsW.empty()) {
                qDebug() << "[Boot] 资源库无需点火 IOCP 监控（已取消）:" << QString::fromStdWString(managedAbsW);
                // 取消监控 QuarkMeta.Library_[盘符] 文件夹
                // NativeFolderWatcher::instance().addWatch(managedAbsW);
            }
        }
    }
}
=======
>>>>>>> REPLACE
```

---

### 4. 修改 `src/ui/MainWindow.cpp`
**修改文件**：`src/ui/MainWindow.cpp`
**修改目的**：删除文件头包含的废弃 `#include "../core/NativeFolderWatcher.h"`。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
#include "../core/NativeFolderWatcher.h"
=======
>>>>>>> REPLACE
```

---

## 阶段四：验证与编译测试步骤

1. **清除临时构建缓存**：清理旧有构建中间文件。
2. **编译验证**：运行 `cmake` 构建，确保在完全不引入 `NativeFolderWatcher.h/cpp` 的情况下无任何编译或链接错误。
3. **运行时验证**：启动 QuarkMeta，观察日志中不再输出 `[Watcher] 初始化高吞吐量 IOCP 监控`，确保系统无多余线程后台空转，资源消耗彻底降低。
