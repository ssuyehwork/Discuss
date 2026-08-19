# 全盘残留僵尸代码彻底物理拔除无脑实施方案 —— PurgeAllZombieLegacyCode

本实施方案旨在对 QuarkMeta 全局代码库进行彻底洗髓，物理拔除所有历史内存托管库模式遗留的僵尸 UI 控件、废弃数据模型、同步状态服务及 Base36 判定函数，使代码库彻底回归纯净的磁盘直连模式。

---

## 彻底物理删除的文件清单（5 个模块，7 个源码文件）

1. `src/ui/CategoryPanel.h`
2. `src/ui/CategoryPanel.cpp`
3. `src/ui/CategoryModel.h`
4. `src/ui/CategoryModel.cpp`
5. `src/ui/CategoryDelegate.h`
6. `src/ui/models/LibraryAssetModel.h`
7. `src/core/SyncStatusService.h`
8. `src/core/SyncStatusService.cpp`

---

## 物理修改与清洗关联引用的文件清单

### 1. 修改 `CMakeLists.txt`
**修改目的**：从构建工程中彻底删除已移除的 7 个僵尸文件。

**精准替换 Diff**：
```cmake
<<<<<<< SEARCH
    src/core/SyncStatusService.cpp
    src/core/SyncStatusService.h
=======
>>>>>>> REPLACE
```

```cmake
<<<<<<< SEARCH
    src/ui/CategoryPanel.cpp
    src/ui/CategoryPanel.h
    src/ui/CategoryModel.cpp
    src/ui/CategoryModel.h
    src/ui/CategoryDelegate.h
=======
>>>>>>> REPLACE
```

```cmake
<<<<<<< SEARCH
    src/ui/models/LibraryAssetModel.h
=======
>>>>>>> REPLACE
```

---

### 2. 修改 `src/ui/MainWindow.cpp`
**修改目的**：清除 `MainWindow` 中关于 `SyncStatusService` 的包含与信号槽绑定。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
#include "../core/SyncStatusService.h"
=======
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    connect(&SyncStatusService::instance(), &SyncStatusService::statusUpdated,
            this, [this](int pending) {
                // ... 旧版同步状态显示逻辑
            });
=======
>>>>>>> REPLACE
```

---

### 3. 修改 `src/meta/MediaExtractorPipeline.cpp`
**修改目的**：清除媒体提取流水线中对 `SyncStatusService` 的空转调用。

**精准替换 Diff**：
```cpp
<<<<<<< SEARCH
#include "../core/SyncStatusService.h"
=======
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    SyncStatusService::instance().updateMediaPending(remaining);
=======
>>>>>>> REPLACE
```

---

### 4. 修改 `src/meta/MetadataManager.h` & `src/meta/MetadataManager.cpp`
**修改目的**：清除 `isInsideManagedLibrary` 与 `extractBase36Id` 托管库逻辑判定，路径校验直接走纯物理磁盘校验。

**`MetadataManager.h` 精准替换 Diff**：
```cpp
<<<<<<< SEARCH
    static bool isInsideManagedLibrary(const std::wstring& path);
=======
>>>>>>> REPLACE
```

**`MetadataManager.cpp` 精准替换 Diff**：
```cpp
<<<<<<< SEARCH
bool MetadataManager::isInsideManagedLibrary(const std::wstring& path) {
    // 托管库路径匹配逻辑
    return path.find(L"QuarkMeta.Library_") != std::wstring::npos;
}
=======
>>>>>>> REPLACE
```

---

## 验证与测试步骤

1. **构建工程重载**：重新运行 `cmake -B build` 更新构建依赖列表。
2. **完整编译**：运行编译，确保无任何未定义符号（Undefined Symbol）或缺失头文件报错。
3. **性能与内存对比**：启动 QuarkMeta 观察运行内存与线程数量，确认系统线程池不再包含空转线程，后台开销显著降低。
