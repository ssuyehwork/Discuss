# 彻底清理内存模式托管库残留相关逻辑代码实施方案 (Purge Managed Library Legacy Code Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在彻底扫除双模式时期遗留在 QuarkMeta 纯磁盘独立化架构中的内存托管库僵尸代码与二次分支判断，构建绝对纯净的磁盘直连体系：
1. **清理底层 Service 层 API**：从 `MetadataManager`、`SystemBootstrapper`、`CoreController` 中彻底剔除 `isInsideManagedLibrary`（是否托管库内部）、`getManagedLibraryPath`（获取托管库路径）、`setManaged`（设置受控）等历史托管库函数与 `AppConfig` 的 `ManagedFolder/Volume_xxx` 查找映射。
2. **解耦数据模型结构体**：从 `ItemRecord` 和 `RuntimeMeta` 中抹除 `isManaged` 字段，并在 `ModelContract.h` 中清除/作废 `ManagedRole` 角色。
3. **清理 UI 菜单与渲染代理**：从 `ContentPanel` 右键菜单中彻底剔除针对受控库显示的“重新扫描”、“取消导入并清除数据”菜单项；从 `ThumbnailDelegate` / `CardPainterHelper` 中清除对 `isManaged` 标志的图标绘制。

---

## 2. Modified Files List（影响文件清单）

1. `src/meta/MetadataManager.h`
2. `src/meta/MetadataManager.cpp`
3. `src/core/ItemRecord.h`
4. `src/core/ItemRecord.cpp`
5. `src/core/ModelContract.h`
6. `src/ui/ContentPanel.cpp`
7. `src/ui/models/DiskItemModel.cpp`
8. `src/ui/ThumbnailDelegate.cpp`
9. `src/ui/CardPainterHelper.h`
10. `src/ui/CardPainterHelper.cpp`
11. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/meta/MetadataManager.h`
清理 `isInsideManagedLibrary` / `getManagedLibraryPath` / `setManaged` 的声明。

```
<<<<<<< SEARCH
    void setManaged(const std::wstring& path, bool managed, bool notify = true);
    static bool isInsideManagedLibrary(const std::wstring& path);
    static std::wstring getManagedLibraryPath(const std::wstring& volSerial, const QString& driveLetter);
=======
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/ContentPanel.cpp`
清理右键菜单中依赖 `ManagedRole` 的“重新扫描”与“取消导入并清除数据”菜单项。

```
<<<<<<< SEARCH
        // 2026-07-xx 按照 Development_Plan 2.1：始终显示“重新扫描”选项 (仅限资源库内项目)
        if (currentIndex.data(ManagedRole).toBool()) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#378ADD"), 18), "重新扫描")->setData(ActionRescan);
        }

        // 2026-07-27 按照 Plan-107：仅对已在资源库中登记的文件夹，增加“取消导入并清除数据”菜单项
        if (currentIndex.data(TypeRole).toString() == "folder" && currentIndex.data(ManagedRole).toBool()) {
            menu.addAction(UiHelper::getIcon("close", QColor("#e81123"), 18), "取消导入并清除数据")->setData(ActionCancelImport);
        }
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在命令行运行 CMake 编译，验证解耦托管库后全工程无符号缺失错误：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **功能验证**：
   - 全盘目录浏览体验一致，不再存在托管库与非托管库的双重分支判断。
   - 右键菜单纯净展示，不再出现旧托管库特有的“重新扫描”与“取消导入”项。
