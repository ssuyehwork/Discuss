# QuarkMeta 独立化改造（剥离内存模式 & 配置与应用重命名）实施方案

## 1. 所属大纲章节
- 独立应用化改造章节：1.1-QuarkMetaStandalone

## 2. 涉及代码文件
- **拟彻底删除文件**：
  - `src/ui/models/LibraryAssetModel.h`
  - `src/ui/models/LibraryAssetModel.cpp`
  - `src/ui/MemoryBatchRenameService.h`
  - `src/ui/MemoryBatchRenameService.cpp`
  - `src/core/DatabaseSynchronizer.h`
  - `src/core/DatabaseSynchronizer.cpp`
  - `src/core/AutoImportManager.h`
  - `src/core/AutoImportManager.cpp`
  - `src/core/LibraryMaintenanceService.h`
  - `src/core/LibraryMaintenanceService.cpp`
- **重命名资源与清单文件**：
  - `ArcMeta.manifest` -> `QuarkMeta.manifest`
  - `ArcMeta.rc` -> `QuarkMeta.rc`
- **需要精确定位修改的文件**：
  - `CMakeLists.txt`
  - `src/core/AppConfig.h`
  - `src/main.cpp`
  - `src/ui/Logger.h`
  - `src/ui/MainWindow.h`
  - `src/ui/MainWindow.cpp`
  - `src/ui/ContentPanel.h`
  - `src/ui/ContentPanel.cpp`
  - `src/ui/BatchRenameDialog.h`
  - `src/ui/BatchRenameDialog.cpp`
  - `src/ui/models/ItemModelBase.h`

## 3. 功能描述
本次改造旨在将纯磁盘目录直连模式单独剥离并提炼为独立应用 **QuarkMeta**。
改动包含三个维度：
1. **彻底拔除内存模式代码文件**：删除专门用于内存托管库模式的 Model、重命名服务、数据库同步器及自动导入管理器，彻底纯化代码仓库。
2. **物理配置隔离**：修改 `AppConfig` 单例的组织名与应用名为 `QuarkMeta`，使其 Windows 注册表与配置文件物理落盘路径彻底独立，避免与 ArcMeta 的配置互相覆盖或污染。
3. **应用全量重命名**：将构建目标可执行文件名、项目工程名、资源描述文件、主窗口标题、运行日志文件名等全部更新为 `QuarkMeta`。

## 4. 技术决策与精准修改方案

### 4.1 配置文件与注册表隔离 (AppConfig)
修改 `AppConfig` 单例中的 QSettings 构造参数，使得配置落盘于 `%APPDATA%/QuarkMeta/QuarkMeta.ini` 或 Windows 注册表 `HKCU\Software\QuarkMeta\QuarkMeta`。

**文件**：`src/core/AppConfig.h`
**精准定位**：Line 36 构造函数

```cpp
<<<<<<< SEARCH
    AppConfig() : m_settings("ArcMeta团队", "ArcMeta") {}
=======
    AppConfig() : m_settings("QuarkMeta", "QuarkMeta") {}
>>>>>>> REPLACE
```

---

### 4.2 日志文件名隔离与更新

**文件**：`src/main.cpp`
**精准定位**：Line 99 `rotateLogFiles` 启动点

```cpp
<<<<<<< SEARCH
    rotateLogFiles("arcmeta_debug.log");
=======
    rotateLogFiles("quarkmeta_debug.log");
>>>>>>> REPLACE
```

**文件**：`src/ui/Logger.h`
**精准定位**：Line 131-132 降级直写轮转与写入点

```cpp
<<<<<<< SEARCH
        rotateLogFiles("arcmeta_debug.log");
        QFile file("arcmeta_debug.log");
=======
        rotateLogFiles("quarkmeta_debug.log");
        QFile file("quarkmeta_debug.log");
>>>>>>> REPLACE
```

---

### 4.3 构建配置、目标名与文件列表更新

**文件**：`CMakeLists.txt`
**精准定位**：Line 2, Line 35-37, SOURCES 列表，与 Line 267/315/320/332/344 目标设置

```cpp
<<<<<<< SEARCH
project(ArcMeta)

# 2026-03-xx 按照用户要求：将可执行文件直接输出到指定项目子目录 ArcMeta 下，解决 \out 文件夹查找不便的问题
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/ArcMeta")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/ArcMeta")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/ArcMeta")
=======
project(QuarkMeta)

# 2026-03-xx 将 QuarkMeta 可执行文件直接输出到项目子目录 QuarkMeta 下
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/QuarkMeta")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/QuarkMeta")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/QuarkMeta")
>>>>>>> REPLACE
```

*在 SOURCES 列表中移除删除的文件，并更新 RC 文件与 Executable 目标*：

```cpp
<<<<<<< SEARCH
    ArcMeta.rc
=======
    QuarkMeta.rc
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
add_executable(ArcMeta WIN32 ${SOURCES})

target_link_libraries(ArcMeta PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Svg
)

target_include_directories(ArcMeta PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/ui
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src/meta
    ${PROJECT_SOURCE_DIR}/src/crypto
    ${PROJECT_SOURCE_DIR}/src/third_party/libtiff
)

if(MSVC)
    target_link_options(ArcMeta PRIVATE "/MANIFESTUAC:level='asInvoker' uiAccess='false'")
endif()
=======
add_executable(QuarkMeta WIN32 ${SOURCES})

target_link_libraries(QuarkMeta PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Svg
)

target_include_directories(QuarkMeta PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/ui
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src/meta
    ${PROJECT_SOURCE_DIR}/src/crypto
    ${PROJECT_SOURCE_DIR}/src/third_party/libtiff
)

if(MSVC)
    target_link_options(QuarkMeta PRIVATE "/MANIFESTUAC:level='asInvoker' uiAccess='false'")
endif()
>>>>>>> REPLACE
```

---

### 4.4 资源描述文件（QuarkMeta.manifest 与 QuarkMeta.rc）

**文件**：`QuarkMeta.manifest`

```xml
<<<<<<< SEARCH
  <assemblyIdentity version="1.0.0.0" processorArchitecture="*" name="ArcMeta" type="win32"/>
  <description>ArcMeta File Manager</description>
=======
  <assemblyIdentity version="1.0.0.0" processorArchitecture="*" name="QuarkMeta" type="win32"/>
  <description>QuarkMeta File Manager</description>
>>>>>>> REPLACE
```

---

### 4.5 主窗口标题重命名与清场

**文件**：`src/ui/MainWindow.cpp`
**精准定位**：Line 221 窗口标题初始化设置

```cpp
<<<<<<< SEARCH
    setWindowTitle("ArcMeta");
=======
    setWindowTitle("QuarkMeta");
>>>>>>> REPLACE
```

---

### 4.6 界面模型多态纯化 (ContentPanel 与 BatchRenameDialog)

在 `ContentPanel` 中完全移除 `LibraryAssetModel* m_libraryModel` 成员定义及相关头文件，将 UI 完全绑定于专为磁盘直连打造的 `DiskItemModel`；在 `BatchRenameDialog` 中移除 `MemoryBatchRenameService` 并全量切换为 `DiskBatchRenameService`。

---

## 5. 已知问题/待办
- **旧配置不迁移策略**：ArcMeta 既有用户的偏好设置（如窗口位置、颜色偏好等）将留在原有 ArcMeta 路径下，QuarkMeta 将以后者全新的独立配置启动。
- **图标与美术资源**：按照要求，本次改造保持现有 `.ico/.png` 等图标资源不动。

---

## 涉及文件清单
### 1. 拟彻底删除文件 (10 个)
- `src/ui/models/LibraryAssetModel.h`
- `src/ui/models/LibraryAssetModel.cpp`
- `src/ui/MemoryBatchRenameService.h`
- `src/ui/MemoryBatchRenameService.cpp`
- `src/core/DatabaseSynchronizer.h`
- `src/core/DatabaseSynchronizer.cpp`
- `src/core/AutoImportManager.h`
- `src/core/AutoImportManager.cpp`
- `src/core/LibraryMaintenanceService.h`
- `src/core/LibraryMaintenanceService.cpp`

### 2. 重命名/新增配置文件 (2 个)
- `QuarkMeta.manifest`
- `QuarkMeta.rc`

### 3. 需要修改的文件 (8 个)
- `CMakeLists.txt`
- `src/core/AppConfig.h`
- `src/main.cpp`
- `src/ui/Logger.h`
- `src/ui/MainWindow.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/BatchRenameDialog.cpp`
