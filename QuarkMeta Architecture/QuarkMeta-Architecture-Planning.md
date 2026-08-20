# QuarkMeta 独立化设计理念与规划规范 (QuarkMeta Architecture and Planning)

## 0. 设计理念与架构重构总则
QuarkMeta 为纯磁盘目录直连模式独立应用。通过彻底剔除原 QuarkMeta 内存托管库/镜像数据库及全量内存索引相关代码，构建轻量、高效的实时磁盘 I/O 浏览与管理体验。

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
- **中央神经调度中枢与底层洗髓重构无脑实施方案**：详见 `Implementation Plan/CentralDispatcherArchitecture.md`
- **五栏视图布局与伸缩因子修复方案**：详见 `Implementation Plan/FiveColumnLayoutFix.md`
- **根目录/盘符元数据 global.db 持久化方案**：详见 `Implementation Plan/DriveRootMetaInGlobalDb.md`
- **托管库与同步按钮彻底根除方案**：详见 `Implementation Plan/RemoveManagedLibraryAndSyncButtons.md`
- **盘符栏清理、自动导入根除与“标签管理”实用按钮引入方案**：详见 `Implementation Plan/TagManagerAndLegacyCodePurge.md`
- **收藏夹独占第二栏重构方案**：详见 `Implementation Plan/FavoritePanel.md`
- **内存托管库模式彻底清理实施方案**：详见 `Implementation Plan/MemoryModeCleanup.md`
- **`.arc` 胶囊文件夹磁盘纯只读直通预览实施方案**：详见 `Implementation Plan/ArcCapsuleReadOnlyPreview.md`
- **基于 File_ID 隔离盒与创建时间权威判别回收站实施方案**：详见 `Implementation Plan/DiskTrashFileIdIsolationAndRestorePlan.md`

---

## 5. 中央神经调度中枢与三条交互铁律架构规范 (Central Dispatcher Architecture Specification)

### 5.1 面板 / 视图层通信规范
#### 5.1.1 交互组件 / 消息总线入口
##### A. 内存模式下：
> *（注：内存模式下的中央神经调度中枢架构不在 QuarkMeta 独立应用范畴内，暂时留空。）*

##### B. 磁盘目录模式下：
1. **双核调度中枢体系**：全系统建立唯一双核中央调度中枢——传声筒 `CentralEventHub`（纯消息事件分发、无数据逻辑）与中央大脑 `CoreEngine`（业务决策与逻辑编排）。
2. **遵守三条交互铁律**：
   - **铁律一**：UI 视图与控件绝对禁止直接调用 `MetadataManager`、`DatabaseManager`、`DiskIoService` 等底层服务。
   - **铁律二**：UI 上的所有用户操作必须封装为 Command 提交给 `CoreEngine`。
   - **铁律三**：UI 只能订阅 `CentralEventHub` 的增量 Event 进行局部刷新，严格禁止调用 `notifyFullUIRebuild()` 强刷全屏。
3. **底层数据纯净化与并发锁**：
   - 由于 QuarkMeta 是从原双轨架构（`Dual-mode version`）独立出的纯磁盘模式应用，中央中枢重构前必须彻底死刑、物理拔除以下残留垃圾：
     1. IOCP 监控与自动导入剪切逻辑；
     2. 创建资源库、`QuarkMeta.Library_X` 托管文件夹及右键菜单；
     3. 标题栏同步按钮 `m_btnSync` 及 `SyncStatusService` 提示；
     4. `.arc` 胶囊容器、`Base36 ID` 及 `isInsideManagedLibrary` 补丁代码。
   - `DatabaseManager` 强制采用分库递归互斥锁，杜绝跨线程 sqlite3 锁争抢。
   - 为后台耗时流水线引入原子化可中断的 `CancellationToken`，防止线程雪崩。

---

## 2. 磁盘模式离散 JSON 元数据缓存与代码类名物理重命名规范 (QuarkMetaJson Specification)

### 2.1 物理文件名与类名重命名规范
- **原物理文件名**：`src/meta/QuarkMeta.h` / `src/meta/QuarkMeta.cpp`
- **新物理文件名**：`src/meta/QuarkMetaJson.h` / `src/meta/QuarkMetaJson.cpp`
- **原 C++ 类名**：`QuarkMeta`
- **新 C++ 类名**：`QuarkMetaJson`
- **原生成离散 JSON 文件名**：`.QuarkMeta.json`
- **新生成离散 JSON 文件名**：`.QuarkMeta.json`

### 2.2 处理原则
在 QuarkMeta 磁盘直连模式下：
1. 涉及磁盘离散元数据 JSON 的读写管理类统一重命名为 **`QuarkMetaJson`**，对应头文件为 **`QuarkMetaJson.h`**。
2. 用户对普通物理文件夹或文件进行标注时，落盘的离散元数据缓存文件名统一使用 **`.QuarkMeta.json`**，防止在磁盘上残留旧应用标识。
3. `.gitignore` 中原 `*.QuarkMeta.json` 规则同步更新为 `*.QuarkMeta.json`。

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
- **物理配置落盘**：`AppConfig` 的 `QSettings` 实例化强制改为 `m_settings("QuarkMeta", "QuarkMeta")`，使得注册表/INI文件存储于专属于 QuarkMeta 的全新路径，绝对禁止与 QuarkMeta 共享或覆盖配置。
- **运行日志隔离**：全局运行与调试日志由 `QuarkMeta_debug.log` 重命名为 `quarkmeta_debug.log`。
- **可执行文件与项目重命名**：构建目标、可执行文件及 Windows 资源清单统一更名为 `QuarkMeta.exe` / `QuarkMeta.manifest` / `QuarkMeta.rc`。

---

## 6. `.arc` 胶囊文件夹磁盘纯只读直通预览规范 (Arc Capsule Read-Only Direct Preview Specification)

### 6.1 设计理念
在 QuarkMeta 纯磁盘直连模式下，用户可能在磁盘目录中访问历史上由托管库生成的 `.arc` 胶囊文件夹。系统采取 **“纯只读直通预览（零提取、零写盘、零缩略图生成）”** 策略：

1. **允许直通查看**：将 `.arc` 胶囊文件夹视为可正常浏览的物理文件夹，支持在目录树和内容面板中点击进入并查看其封存的主资产。
2. **过滤内部杂质**：在 `.arc` 目录内部导航时，自动识别并隐藏内部的 `meta.json`、`thumb_*.png` 等辅助缓存文件，仅将主体资产文件呈现给用户。
3. **零污染与绝对只读**：在预览 `.arc` 内部资产时，强制关闭所有后台缩略图生成、磁盘写入、Hash 计算与解包提取逻辑，确保 `.arc` 物理文件夹及其内容的绝对只读与零改动。

---

## 7. 基于 File_ID 隔离盒与创建时间权威判别回收站规范 (File_ID Trash & Creation Time Restore Specification)

### 7.1 设计理念
在 QuarkMeta 纯磁盘直连模式下，回收站彻底废除原有粗暴改名与容易冲突的直接位移逻辑，采用 **“基于项目自身 File_ID 独立盒隔离 + 原始名称 100% 保持 + 创建时间权威判别 + 连字符 `-N` 还原重命名避让”** 架构：

1. **入库隔离与零名改动**：
   - 移入回收站时，绝对禁止修改文件/文件夹本身的原始名称。
   - 在对应盘符的回收站根目录（`<盘符>:\.QuarkMeta\disk_trash\`）下，为每个移入的项目创建以该项目**自身 `File_ID`**（全局唯一 UUID）命名的独立隔离盒文件夹：`<盘符>:\.QuarkMeta\disk_trash\{File_ID}\`。
   - 将项目原封不动移入各自对应的 `{File_ID}` 盒内。全局数据库 `disk_trash` 表记录 `file_id`、`created_at`（原始创建时间毫秒戳）、`trash_path` 与 `original_path`。

2. **还原 (Restore) 智能避让与创建时间权威判别**：
   - 还原项目时，系统对比数据库中记录的原始创建时间戳 $T_{\text{trash}}$ 与目标磁盘已有同名项目的创建时间戳 $T_{\text{disk}}$。
   - **无冲突时**：原名恢复至 `original_path`。
   - **有同名冲突时**：
     - 若 $T_{\text{trash}} < T_{\text{disk}}$（被还原项目创建更早）：被还原项目作为“最早创建权威”占用原始名称 `A.txt`，磁盘上较晚创建的现有项目被自动递增重命名避让为 **`A-1.txt`**（如存在顺延至 `A-2.txt`）。
     - 若 $T_{\text{disk}} \le T_{\text{trash}}$（磁盘现有项目创建更早）：磁盘现有项目保持原名 `A.txt`，被还原项目重命名为 **`A-1.txt`** 还原移出。
   - 格式强制要求：连字符 `-` 命名避让（如 `A-1.txt` / `Folder-1`），严格禁止使用圆括号 `(1)`。
