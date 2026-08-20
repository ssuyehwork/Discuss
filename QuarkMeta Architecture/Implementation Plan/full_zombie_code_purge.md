# 全量僵尸代码与废弃历史负债物理彻底清退无脑实施方案 (Full Zombie Code Purge Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在对项目中全量 30 个物理存在但未包含在 CMake 中的孤立僵尸源码文件，以及 10 个在 CMake 中编译但外部零调用的僵尸组件进行**彻底物理删除与 CMakeLists.txt 解绑**（特别注意：`TagSelectorOverlay` 已复用作为元数据面板属性打标选择器，不属于僵尸代码，继续予以保留）：

### 一、 物理存在但未包含在 CMake 中的孤立僵尸源码文件（物理彻底删除 30 个项目文件）
1. **废弃的旧 UI 模块（11 个文件）**：
   - `src/ui/CategoryPanel.h` & `CategoryPanel.cpp`
   - `src/ui/CategoryModel.h` & `CategoryModel.cpp`
   - `src/ui/CategoryDelegate.h`
   - `src/ui/ProgressDialog.h`
   - `src/ui/PresetTagsDialog.h`
   - `src/ui/ScanStats.h`
   - `src/ui/StyleLibrary.h`
   - `src/ui/SvgIcons.h`
   - `src/ui/models/LibraryAssetModel.h`
2. **废弃的数据库/元数据/导入服务（10 个文件）**：
   - `src/meta/AmMetaJson.h` & `AmMetaJson.cpp`
   - `src/meta/MetadataDao.h` & `MetadataDao.cpp`
   - `src/meta/IngestionProgressEngine.h` & `IngestionProgressEngine.cpp`
   - `src/meta/PhysicalDataExtractor.h` & `PhysicalDataExtractor.cpp`
   - `src/meta/FileOperationHelper.h`
   - `src/meta/sqlite3ext.h`
3. **废弃的核心逻辑与引导服务（6 个文件）**：
   - `src/core/DiskIngestionService.h` & `DiskIngestionService.cpp`
   - `src/core/SystemBootstrapper.h` & `SystemBootstrapper.cpp`
   - `src/core/CategoryLockManager.h`
   - `src/core/ModelContract.h`
4. **废弃的工具类（3 个文件）**：
   - `src/util/AppDirectoryInitializer.h`
   - `src/util/DiskIoService.h`
   - `src/util/SecureFileEraser.h`

---

### 二、 在 CMake 中编译但外部零调用的“虚挂/僵尸组件与头文件”（CMake 解绑并物理删除 10 个项目文件）
1. `src/ui/CategoryFilterProxyModel.h`
2. `src/meta/CategoryBindingManager.h` & `CategoryBindingManager.cpp`
3. `src/ui/ListResultView.h`
4. `src/ui/GridResultView.h`
5. `src/ui/JustifiedResultView.h`
6. `src/ui/TagManagerDialog.h` & `TagManagerDialog.cpp`
7. `src/ui/CategorySetPasswordDialog.h`
8. `src/ui/IconCacheManager.h`
9. `src/core/IndexedEntry.h`

---

## 2. Modified Files List（影响文件清单）

1. `CMakeLists.txt`
2. 上述 40 个物理僵尸源码文件（物理执行 `git rm` 删除）
3. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `CMakeLists.txt`
从 `CMakeLists.txt` 中清除所有残留零调用的僵尸头文件与实现文件注册（保留 `TagSelectorOverlay`）：

```
<<<<<<< SEARCH
    src/ui/CategorySetPasswordDialog.h
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **物理清退命令**：
   在终端执行物理清理，彻底移除全量 40 个僵尸源代码文件：
   ```bash
   rm -f src/ui/CategoryPanel.* src/ui/CategoryModel.* src/ui/CategoryDelegate.h \
         src/ui/ProgressDialog.h src/ui/PresetTagsDialog.h src/ui/ScanStats.h \
         src/ui/StyleLibrary.h src/ui/SvgIcons.h src/ui/models/LibraryAssetModel.h \
         src/meta/AmMetaJson.* src/meta/MetadataDao.* src/meta/IngestionProgressEngine.* \
         src/meta/PhysicalDataExtractor.* src/meta/FileOperationHelper.h src/meta/sqlite3ext.h \
         src/core/DiskIngestionService.* src/core/SystemBootstrapper.* src/core/CategoryLockManager.h \
         src/core/ModelContract.h src/util/AppDirectoryInitializer.h src/util/DiskIoService.h \
         src/util/SecureFileEraser.h src/ui/CategoryFilterProxyModel.h src/meta/CategoryBindingManager.* \
         src/ui/ListResultView.h src/ui/GridResultView.h src/ui/JustifiedResultView.h \
         src/ui/TagManagerDialog.* src/ui/CategorySetPasswordDialog.h \
         src/ui/IconCacheManager.h src/core/IndexedEntry.h
   ```
2. **编译确认**：
   运行 CMake 重构并编译，验证无任何符号缺少或 MOC 报错：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
3. **功能验证**：
   全系统 100% 正常运行，无任何悬空调用与崩溃，源码体积与构建干净度达到顶峰。
