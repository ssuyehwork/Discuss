# QuarkMeta 独立化设计理念与规划规范 (QuarkMeta Architecture and Planning)

## 0. 设计理念与架构重构总则
QuarkMeta 为纯磁盘目录直连模式独立应用。通过彻底剔除原 ArcMeta 内存托管库/镜像数据库及全量内存索引相关代码，构建轻量、高效的实时磁盘 I/O 浏览与管理体验。

---

## 1. 界面面板与五栏式视图布局规范 (UI Panel & Five-Column View Specification)

### 1.1 五栏式侧边与主体结构布局 (Five-Column Layout Architecture)
在 QuarkMeta 纯磁盘直连模式下，系统彻底废除原有的“侧边栏分类面板”（`CategoryPanel`），严禁采用任何 `.hide()` 等打补丁隐藏方式留下空白残影顶栏。独立后的 QuarkMeta 整体界面横向从左到右共有 5 栏（包含 3 栏核心主导航与内容区 + 2 栏右侧辅助属性面板）：

```
+------------------+------------------+------------------+------------------+------------------+
| 第一栏：目录导航 | 第二栏：收藏夹   | 第三栏：内容展示 | 第四栏：元数据   | 第五栏：条件筛选 |
| (DirNav / Col 1) | (Favorites/Col 2)| (Content / Col 3)| (Meta / Col 4)   | (Filter / Col 5) |
+------------------+------------------+------------------+------------------+------------------+
| - 此电脑         | - 常用快捷文件夹 | - 缩略图/列表    | - 评级/颜色/标签 | - 颜色/类型/评级 |
| - 本地盘符       | - 垂直贯通独占   | - 核心主视图区   | - 尺寸/备注编辑  | - 过滤筛选工具   |
| - 桌面/系统目录  | - 空间大更清晰   |                  |                  |                  |
+------------------+------------------+------------------+------------------+------------------+
```

#### A. 磁盘目录模式下：
一、3 栏核心主功能区（从左到右）
1. **第一栏（最左侧）：目录导航栏（Dir Tree Navigation）**
   - 包含“此电脑”、本地盘符（C/D/E/Z盘等）、桌面等标准的本地磁盘目录树结构（替换原分类面板位置）。

2. **第二栏（中间）：收藏夹独占栏（Dedicated Favorites Bar）**
   - 独占一整栏，专门展示收藏的常用快捷文件夹，垂直贯通，空间更大更清晰。

3. **第三栏（主视图区）：内容展示区（Content Panel）**
   - 展示当前文件夹内的图片/文件缩略图网格或列表视图。

二、2 栏右侧辅助工具栏（可按需展开/收起）
4. **第四栏：元数据属性栏（Meta Panel）**
   - 展示和编辑当前选中文件的评级（星标）、颜色标记、标签、备注、尺寸等元数据。

5. **第五栏（最右侧）：条件筛选栏（Filter Panel）**
   - 提供按颜色、文件类型、评级、时间等维度快速过滤当前内容的筛选工具。

---

## 4. 界面重构实施子计划索引 (Implementation Plans)
- **收藏夹独占第二栏重构方案**：详见 `Modification_Plan/FavoritePanel.md`

---

## 2. 磁盘模式离散 JSON 元数据缓存与代码类名物理重命名规范 (QuarkMetaJson Specification)

### 2.1 物理文件名与类名重命名规范
- **原物理文件名**：`src/meta/AmMetaJson.h` / `src/meta/AmMetaJson.cpp`
- **新物理文件名**：`src/meta/QuarkMetaJson.h` / `src/meta/QuarkMetaJson.cpp`
- **原 C++ 类名**：`AmMetaJson`
- **新 C++ 类名**：`QuarkMetaJson`
- **原生成离散 JSON 文件名**：`.ArcMeta.json`
- **新生成离散 JSON 文件名**：`.QuarkMeta.json`

### 2.2 处理原则
在 QuarkMeta 磁盘直连模式下：
1. 涉及磁盘离散元数据 JSON 的读写管理类统一重命名为 **`QuarkMetaJson`**，对应头文件为 **`QuarkMetaJson.h`**。
2. 用户对普通物理文件夹或文件进行标注时，落盘的离散元数据缓存文件名统一使用 **`.QuarkMeta.json`**，防止在磁盘上残留旧应用标识。
3. `.gitignore` 中原 `*.ArcMeta.json` 规则同步更新为 `*.QuarkMeta.json`。

---

## 3. 独立化应用改造与配置隔离规范 (Standalone Application & Isolation Specification)

### 3.1 依赖清场与文件彻底拔除规范 (Code Deprecation)
彻底拔除并删除专门服务于内存托管模式的代码文件及 UI 控件：
- 分类面板组件：`CategoryPanel.h / .cpp`，`CategoryModel.h / .cpp`，`CategoryDelegate.h`
- 内存资产模型：`LibraryAssetModel.h / .cpp`
- 内存重命名服务：`MemoryBatchRenameService.h / .cpp`
- 数据库与同步器：`DatabaseSynchronizer.h / .cpp`
- 自动导入与维护服务：`AutoImportManager.h / .cpp`，`LibraryMaintenanceService.h / .cpp`

### 3.2 配置与日志隔离规范 (Config & Log Isolation)
- **物理配置落盘**：`AppConfig` 的 `QSettings` 实例化强制改为 `m_settings("QuarkMeta", "QuarkMeta")`，使得注册表/INI文件存储于专属于 QuarkMeta 的全新路径，绝对禁止与 ArcMeta 共享或覆盖配置。
- **运行日志隔离**：全局运行与调试日志由 `arcmeta_debug.log` 重命名为 `quarkmeta_debug.log`。
- **可执行文件与项目重命名**：构建目标、可执行文件及 Windows 资源清单统一更名为 `QuarkMeta.exe` / `QuarkMeta.manifest` / `QuarkMeta.rc`。
