# QuarkMeta 独立化改造（剥离内存模式 & UI 三栏布局重构 & 配置/日志/离散 JSON 与应用重命名）实施方案

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
  - `.gitignore`
  - `src/core/AppConfig.h`
  - `src/main.cpp`
  - `src/ui/Logger.h`
  - `src/meta/AmMetaJson.h`
  - `src/meta/AmMetaJson.cpp`
  - `src/core/FileFilterService.cpp`
  - `src/util/ShellHelper.cpp`
  - `src/ui/MainWindow.h`
  - `src/ui/MainWindow.cpp`
  - `src/ui/NavPanel.h`
  - `src/ui/NavPanel.cpp`
  - `src/ui/ContentPanel.cpp`
  - `src/ui/BatchRenameDialog.cpp`

## 3. 功能描述
本次改造旨在将纯磁盘目录直连模式单独剥离并提炼为独立应用 **QuarkMeta**。
改动包含五个维度：
1. **彻底拔除内存模式代码文件**：删除专门用于内存托管库模式的 Model、重命名服务、数据库同步器及自动导入管理器，彻底纯化代码仓库。
2. **离散 JSON 缓存更名**：将普通磁盘目录下生成的隐藏元数据缓存文件名由 `.ArcMeta.json` 彻底更新为 **`.QuarkMeta.json`**。
3. **UI 三栏布局重构**：将磁盘“目录导航”树搬迁移至最左侧第一栏（替代原 `CategoryPanel`）；中间第二栏由“收藏夹”垂直独占撑满；第三栏为内容与元数据区。
4. **物理配置隔离**：修改 `AppConfig` 单例的组织名与应用名为 `QuarkMeta`，使其 Windows 注册表与配置文件物理落盘路径彻底独立，避免与 ArcMeta 的配置互相覆盖或污染。
5. **应用全量重命名**：将构建目标可执行文件名、项目工程名、资源描述文件、主窗口标题、运行日志文件名等全部更新为 `QuarkMeta`。

## 4. 技术决策与精准修改方案

### 4.1 离散 JSON 元数据缓存文件名更名 (.QuarkMeta.json)

**文件**：`src/meta/AmMetaJson.cpp`
**精准定位**：Line 22-23

```cpp
<<<<<<< SEARCH
    // 🚨 彻底废除 .am_meta.json，唯一物理文件名：.ArcMeta.json
    m_filePath = path + L".ArcMeta.json";
=======
    // 🚨 QuarkMeta 唯一物理离散缓存文件名：.QuarkMeta.json
    m_filePath = path + L".QuarkMeta.json";
>>>>>>> REPLACE
```

**文件**：`src/core/FileFilterService.cpp`
**精准定位**：Line 12

```cpp
<<<<<<< SEARCH
    if (fileName.endsWith(".ArcMeta.json", Qt::CaseInsensitive) ||
=======
    if (fileName.endsWith(".QuarkMeta.json", Qt::CaseInsensitive) ||
>>>>>>> REPLACE
```

**文件**：`.gitignore`
**精准定位**：Line 124

```text
<<<<<<< SEARCH
*.ArcMeta.json
=======
*.QuarkMeta.json
>>>>>>> REPLACE
```

---

### 4.2 配置文件与注册表隔离 (AppConfig)
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

### 4.3 日志文件名隔离与更新

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

### 4.4 构建配置、目标名与文件列表更新

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

---

### 4.5 侧边栏与 UI 三栏式重构

1. **移除 `CategoryPanel` 控件**：在 `MainWindow` 布局中移除 `CategoryPanel`（及对应头文件与成员变量），第一栏直接放置包含“目录导航”的 `NavPanel` 视图。
2. **`NavPanel` 布局调整**：将原“目录导航树”调整至最左侧第一栏，“收藏夹”调整至中间第二栏独立独占。

**文件**：`src/ui/MainWindow.cpp`
**精准定位**：Line 221 窗口标题初始化与布局绑定

```cpp
<<<<<<< SEARCH
    setWindowTitle("ArcMeta");
=======
    setWindowTitle("QuarkMeta");
>>>>>>> REPLACE
```

---

## 5. 已知问题/待办
- **旧配置不迁移策略**：ArcMeta 既有用户的偏好设置将留在原有 ArcMeta 路径下，QuarkMeta 将以后者全新的独立配置启动。
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

### 2. 新增/更新文档与资源文件 (3 个)
- `Modification_Plan/QuarkMeta-Architecture-Planning.md`
- `QuarkMeta.manifest`
- `QuarkMeta.rc`

### 3. 需要修改的文件 (12 个)
- `CMakeLists.txt`
- `.gitignore`
- `src/core/AppConfig.h`
- `src/main.cpp`
- `src/ui/Logger.h`
- `src/meta/AmMetaJson.cpp`
- `src/core/FileFilterService.cpp`
- `src/util/ShellHelper.cpp`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `src/ui/NavPanel.h`
- `src/ui/ContentPanel.cpp`
