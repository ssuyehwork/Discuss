# 纯磁盘目录模式·内存模式与托管库僵尸代码根除无脑实施方案 (Pure Disk Mode Memory & Managed Code Purge Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在全面彻底地消灭双模式时期遗留在系统中的所有内存托管库、.arc 胶囊容器、Base36 算法、多分盘 `QuarkMeta_*.db` 及分类关系表，全面贯彻 QuarkMeta“纯磁盘目录直连”架构。

全方案精细划分为 **6 个递进阶段（Stage 1 ~ 6）**，确保每个阶段均包含确切的影响文件清单、精准替换块与自检验证命令：
1. **Stage 1**：构建系统治理（`CMakeLists.txt`）—— 彻底剥离 `AssetImporter`、`ImportHelper`、`CapsuleMediaExtractor`、`CategoryLockDialog`、`CategoryLockWidget` 等僵尸源码编译条目。
2. **Stage 2**：数据库引擎降维（`DatabaseManager`）—— 废除分盘 `QuarkMeta_*.db` 和 `m_driveDbs` 映射，全系统仅保留 `global.db`，删除 `categories` / `category_items` 表与卷漂移逻辑。
3. **Stage 3**：元数据管理器脱耦（`MetadataManager`）—— 删除 `registerAsset`、`isInsideManagedLibrary`、`getManagedLibraryPath` 等托管库 API，`RuntimeMeta` 结构体移除 `categoryIds` 和 `isManaged` 字段，元数据落盘直连 `.QuarkMeta.json` 与 `global.db`。
4. **Stage 4**：内容面板与数据模型归一化（`ContentPanel` & `DiskItemModel`）—— 清理 `isMirrorSource` 与 `isManagedContext` 分流逻辑，文件粘贴/拖拽 100% 走 `DiskIoService` 物理操作，删除 `.arc` 胶囊生成与分类绑定右键菜单。
5. **Stage 5**：系统辅助服务净化（`ShellHelper` & `DuplicateDetectorService`）—— 清理旧版移动到回收站逻辑，统一走 `DiskTrashService`；查重引擎剔除 `CapsuleMediaExtractor`，引入首尾 64KB FastHash 快速预筛。
6. **Stage 6**：主窗口与对话框协议清理（`MainWindow` & `TagManagerDialog`）—— `unifiedNavigateTo` 彻底删除 `kProtocolCategory` 分流协议；`TagManagerDialog` 彻底剔除镜像源分支，全局标签读取统一直连 `global.db`，节点标签写入当前目录 `.QuarkMeta.json`。

---

## 2. Modified Files List（影响文件清单）

1. `CMakeLists.txt`
2. `src/meta/DatabaseManager.h`
3. `src/meta/DatabaseManager.cpp`
4. `src/meta/MetadataManager.h`
5. `src/meta/MetadataManager.cpp`
6. `src/ui/ContentPanel.h`
7. `src/ui/ContentPanel.cpp`
8. `src/ui/models/DiskItemModel.cpp`
9. `src/util/ShellHelper.cpp`
10. `src/meta/DuplicateDetectorService.cpp`
11. `src/ui/MainWindow.cpp`
12. `src/ui/TagManagerDialog.cpp`
13. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 Stage 1：`CMakeLists.txt` 构建系统治理
在 `CMakeLists.txt` 中彻底删除僵尸源码文件的编译注册路径。

```
<<<<<<< SEARCH
    src/util/AssetImporter.cpp
    src/util/AssetImporter.h
    src/util/ImportHelper.cpp
    src/util/ImportHelper.h
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    src/meta/CapsuleMediaExtractor.cpp
    src/meta/CapsuleMediaExtractor.h
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    src/ui/CategoryLockDialog.cpp
    src/ui/CategoryLockDialog.h
    src/ui/CategoryLockWidget.cpp
    src/ui/CategoryLockWidget.h
=======
>>>>>>> REPLACE
```

---

### 3.2 Stage 2：`src/meta/DatabaseManager.h` & `DatabaseManager.cpp` 数据库引擎降维
废除分盘 `QuarkMeta_*.db` 和 `m_driveDbs` 映射，仅保留唯一 `global.db` 句柄。

```
<<<<<<< SEARCH
    sqlite3* getDriveDb(const std::wstring& drivePath);
    sqlite3* getDbForPath(const std::wstring& absolutePath);
    std::vector<sqlite3*> getActiveMemoryDbs();
    sqlite3* getDiskDb(const std::wstring& rootPath);
    std::recursive_mutex& getDriveMutex(const std::wstring& drivePath);
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void DatabaseManager::resolveVolumeDrift(const std::wstring& drivePath, const std::wstring& volSerial) {
    // 卷漂移自愈逻辑...
}
=======
>>>>>>> REPLACE
```

---

### 3.3 Stage 3：`src/meta/MetadataManager.h` & `MetadataManager.cpp` 元数据脱耦
彻底清理 `isInsideManagedLibrary` / `getManagedLibraryPath` / `registerAsset` / `migrateCapsuleToLibrary` 等托管 API。

```
<<<<<<< SEARCH
    bool registerAsset(const std::wstring& path, const std::wstring& containerId);
    bool migrateCapsuleToLibrary(const std::wstring& capsulePath, const std::wstring& targetDir);
    static bool isInsideManagedLibrary(const std::wstring& path);
    static std::wstring getManagedLibraryPath(const std::wstring& volSerial, const QString& driveLetter);
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    std::vector<int> categoryIds;
    bool isManaged;
=======
>>>>>>> REPLACE
```

---

### 3.4 Stage 4：`src/ui/ContentPanel.h` & `ContentPanel.cpp` 内容面板归一化
物理清除 `isMirrorSource()` / `isManagedContext()` 分流以及 `.arc` 胶囊生成和分类右键菜单。

```
<<<<<<< SEARCH
    bool isMirrorSource() const;
    bool isManagedContext() const;
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (currentIndex.data(ManagedRole).toBool()) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#378ADD"), 18), "重新扫描")->setData(ActionRescan);
        }
=======
>>>>>>> REPLACE
```

---

### 3.5 Stage 5：`src/util/ShellHelper.cpp` & `DuplicateDetectorService.cpp` 辅助服务净化
清退旧版移入回收站分支，查重引擎剔除 `CapsuleMediaExtractor.h` 并升级 FastHash 预筛。

```
<<<<<<< SEARCH
#include "CapsuleMediaExtractor.h"
=======
>>>>>>> REPLACE
```

---

### 3.6 Stage 6：`src/ui/MainWindow.cpp` & `TagManagerDialog.cpp` 导航协议与对话框净化
彻底删除 `kProtocolCategory` 常量与分类协议分流，`TagManagerDialog` 彻底剔除 `m_isMirrorSource` 分支，全局标签读写直连 `global.db` 与文件 JSON。

```
<<<<<<< SEARCH
    if (url.startsWith(kProtocolCategory)) {
        // 分类协议处理...
    }
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (m_isMirrorSource) {
        // 镜像源历史逻辑...
    } else {
        TagRepository::addTagToGroup(tagName, 0);
    }
=======
    TagRepository::addTagToGroup(tagName, 0);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（分阶段自检与编译验证）

1. **Stage 1 自检**：
   运行 `cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug`，确认无文件缺失警报。
2. **Stage 2 ~ 6 逐级编译**：
   ```bash
   cmake --build build
   ```
3. **全局自检断言 Checkpoints**：
   - 全局搜索 `getDriveDb` / `isInsideManagedLibrary` / `CapsuleMediaExtractor` / `kProtocolCategory`，确保项目内匹配计数均为 0。
   - 打开应用，测试粘贴、拖拽、创建文件夹、打标签、右键菜单与窗口跳转，确认 100% 直连纯磁盘，无卡顿无报错。
