# QuarkMeta 纯磁盘独立化设计理念与架构规范 (QuarkMeta Standalone Architecture Specification)

## 0. 架构设计理念与总则
QuarkMeta 为纯磁盘目录直连模式独立应用。通过彻底剔除内存托管库/镜像数据库及全量内存索引，构建轻量、高效、无感落盘的实时磁盘 I/O 浏览与管理体验。

### 0.1 纯磁盘模式数据落盘与内存托管库僵尸彻底根除铁律
1. **元数据落盘唯一事实源（SSOT）**：磁盘文件的星级、颜色、标签、备注等元数据，**必须且只能**离散落盘持久化至各级目录下的 `.QuarkMeta.json` 文件中（通过 `QuarkMetaJson` 管理类）。严禁将非根目录/非盘符的项目元数据写入 `global.db` 的 `metadata` 数据表中。`global.db` 仅保留 `disk_trash`（回收站）、盘符/根目录系统元数据及 `tag_groups` 全局标签配置。
2. **全量内存托管库与全盘监控彻底物理清退**：全系统严格清退双模（Dual-mode）与内存托管库时代遗留的一切僵尸架构与数据表，包括但不限于：
   - `system_stats` 表中的全盘解析进度记录（`PROGRESS:<路径>`）与全盘指标快照；
   - `CategoryLockManager` 分类锁机制与 `NativeFolderWatcher`（IOCP 全盘自动监控）；
   - `Base36 ID`、分类树（`CategoryPanel` / `UserCategory` / `SystemCategory`）等历史判定分支；
   - `global.db` 中的 `metadata` 表与 `metadata_fts` 全文索引双重记账残留。

### 0.2 模型契约与组件数据结构纯洁性规范
1. **模型契约（ModelContract）纯洁性**：模型契约 `ModelContract` 必须严格反映当前纯磁盘直连架构的物理角色，彻底剔除所有托管库/双轨时代遗留的的角色定义（如 `ManagedRole`、`RegistrationProgressRole`、`IsGroupHeaderRole`、`GroupNameRole`、`IdRole` 等），严禁保留返回假值或无用逻辑的僵尸 Role 分支。
2. **数据结构与组件瘦身（ItemRecord / ContentPanel / Views）**：`ItemRecord` 与 `ContentPanel` 等视图/模型层必须保持绝对的单一职责与高内聚，物理清除所有悬空无定义的孤儿函数（如 `addItemsFromDirectory`）、无读写逻辑的幽灵缓存（如 `ScanCacheEntry` / `m_recursiveCache`）、已被 QuickLook 替代的悬空内嵌预览组件（`m_textPreview` / `m_imagePreview`），以及文件间的幽灵 `#include` 引用，确保代码库干净纯洁、无任何未引用残留。

---

## 1. 界面面板与五栏式视图布局规范 (UI Panel & Five-Column View Specification)

### 1.1 五栏式侧边与主体结构布局 (Five-Column Layout Architecture)
在 QuarkMeta 纯磁盘直连模式下，系统彻底废除原有的“侧边栏分类面板”（`CategoryPanel`），严禁采用任何 `.hide()` 等打补丁隐藏方式留下空白残影顶栏。QuarkMeta 整体界面横向从左到右共有 5 栏（包含 3 栏核心主导航与内容区 + 2 栏右侧辅助属性面板）：

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

一、3 栏核心主功能区（从左到右）
1. **第一栏（最左侧）：目录导航栏（Dir Tree Navigation）**
   - 包含“此电脑”、本地盘符（C/D/E/Z盘等）、桌面等标准的本地磁盘目录树结构。

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

## 2. 界面重构实施子计划索引 (Implementation Plans)
- **中央神经调度中枢与底层洗髓重构无脑实施方案**：详见 `Implementation Plan/CentralDispatcherArchitecture.md`
- **五栏视图布局与伸缩因子修复方案**：详见 `Implementation Plan/FiveColumnLayoutFix.md`
- **根目录/盘符元数据 global.db 持久化方案**：详见 `Implementation Plan/DriveRootMetaInGlobalDb.md`
- **盘符栏清理、自动导入根除与“标签管理”实用按钮引入方案**：详见 `Implementation Plan/tag_manager_and_drive_bar_purge.md`
- **收藏夹独占第二栏重构方案**：详见 `Implementation Plan/FavoritePanel.md`
- **`.arc` 胶囊文件夹磁盘纯只读直通预览实施方案**：详见 `Implementation Plan/ArcCapsuleReadOnlyPreview.md`
- **回收站与 File_ID 隔离盒实施方案**：详见 `Implementation Plan/trash.md`
- **右键菜单调整、Base36 ID 彻底根除与 LoadingWindow 引入实施方案**：详见 `Implementation Plan/menu_and_base36_purge.md`
- **Undo/Redo 核心 ActionCommand 指令体系实施方案**：详见 `Implementation Plan/action_commands.md`
- **恢复 TagManagerDialog 对话框并弃用 TagManagerView 实施方案**：详见 `Implementation Plan/tag_manager_dialog_restore.md`
- **全量僵尸代码与废弃历史负债物理彻底清退无脑实施方案**：详见 `Implementation Plan/full_zombie_code_purge.md`
- **元数据面板标签按钮化与 TagSelectorOverlay 悬浮选择器实时联动无脑实施方案**：详见 `Implementation Plan/meta_panel_tag_selector_overlay.md`
- **TagSelectorOverlay 界面精细化改造无脑实施方案**：详见 `Implementation Plan/tag_selector_overlay_refinement.md`
- **纯磁盘目录模式·内存模式与托管库僵尸代码根除无脑实施方案**：详见 `Implementation Plan/memory_mode_purge.md`
- **筛选面板颜色高级筛选复合控件清退与纯净化方案**：详见 `Implementation Plan/filter_color_purge.md`

---

## 3. 中央神经调度中枢与三条交互铁律架构规范 (Central Dispatcher Architecture Specification)

### 3.1 面板 / 视图层通信规范与调度体系
1. **双核调度中枢体系**：全系统建立唯一双核中央调度中枢——传声筒 `CentralEventHub`（纯消息事件分发、无数据逻辑）与中央大脑 `CoreEngine`（业务决策与逻辑编排）。
2. **遵守三条交互铁律**：
   - **铁律一**：UI 视图与控件绝对禁止直接调用 `MetadataManager`、`DatabaseManager`、`DiskIoService` 等底层服务。
   - **铁律二**：UI 上的所有用户操作必须封装为 Command 提交给 `CoreEngine`。
   - **铁律三**：UI 只能订阅 `CentralEventHub` 的增量 Event 进行局部刷新，严格禁止调用 `notifyFullUIRebuild()` 强刷全屏。
3. **底层数据纯净化与并发锁**：
   - QuarkMeta 核心架构禁止保留任何历史僵尸逻辑与监控：
     1. IOCP 监控与自动导入剪切逻辑；
     2. 标题栏同步按钮 `m_btnSync` 及 `SyncStatusService` 提示；
     3. `.arc` 胶囊容器、`Base36 ID` 等历史判定代码（Base36 ID 必须彻底根除）。
   - `DatabaseManager` 强制采用分库递归互斥锁，杜绝跨线程 sqlite3 锁争抢。
   - 为后台耗时流水线引入原子化可中断的 `CancellationToken`，防止线程雪崩。

---

## 4. 磁盘模式离散 JSON 元数据缓存与代码类名物理重命名规范 (QuarkMetaJson Specification)

### 4.1 物理文件名与类名重命名规范
- **物理文件名**：`src/meta/QuarkMetaJson.h` / `src/meta/QuarkMetaJson.cpp`
- **C++ 类名**：`QuarkMetaJson`
- **离散 JSON 文件名**：`.QuarkMeta.json`

### 4.2 处理原则
在 QuarkMeta 磁盘直连模式下：
1. 涉及磁盘离散元数据 JSON 的读写管理类统一命名为 **`QuarkMetaJson`**，对应头文件为 **`QuarkMetaJson.h`**。
2. 用户对普通物理文件夹或文件进行标注时，落盘的离散元数据缓存文件名统一使用 **`.QuarkMeta.json`**。
3. `.gitignore` 中规则统一配置为 `*.QuarkMeta.json`。

---

## 5. 配置隔离规范 (Config & Log Isolation)

### 5.1 配置与日志隔离规范
- **物理配置落盘**：`AppConfig` 的 `QSettings` 实例化强制使用 `m_settings("QuarkMeta", "QuarkMeta")`，使得注册表/INI文件存储于专属于 QuarkMeta 的全新路径。
- **运行日志隔离**：全局运行与调试日志统一命名为 `quarkmeta_debug.log`。
- **可执行文件与项目重命名**：构建目标、可执行文件及 Windows 资源清单统一更名为 `QuarkMeta.exe` / `QuarkMeta.manifest` / `QuarkMeta.rc`。

---

## 6. `.arc` 胶囊文件夹磁盘纯只读直通预览规范 (Arc Capsule Read-Only Direct Preview Specification)

### 6.1 设计理念
在 QuarkMeta 纯磁盘直连模式下，用户可能在磁盘目录中访问历史上生成的 `.arc` 胶囊文件夹。系统采取 **“纯只读直通预览（零提取、零写盘、零缩略图生成）”** 策略：

1. **允许直通查看**：将 `.arc` 胶囊文件夹视为可正常浏览的物理文件夹，支持在目录树和内容面板中点击进入并查看其封存的主资产。
2. **过滤内部杂质**：在 `.arc` 目录内部导航时，自动识别并隐藏内部的 `meta.json`、`thumb_*.png` 等辅助缓存文件，仅将主体资产文件呈现给用户。
3. **零污染与绝对只读**：在预览 `.arc` 内部资产时，强制关闭所有后台缩略图生成、磁盘写入、Hash 计算与解包提取逻辑，确保 `.arc` 物理文件夹及其内容的绝对只读与零改动。

---

## 8. Undo/Redo 核心 ActionCommand 规范 (Move / Rename / Metadata / SecureDelete Command Specification)

### 8.1 MoveCommand (移动命令)
1. **核心职责与逆向控制**：
   - **Execute**：将文件/文件夹从 `oldPath` 移动到 `newPath`。
   - **Undo (Ctrl+Z)**：逆向搬回，从 `newPath` 恢复到 `oldPath`。
   - **Redo (Ctrl+Shift+Z)**：再次从 `oldPath` 移动到 `newPath`。
2. **纯磁盘 JSON 元数据原子同步**：
   - 在物理移动文件的同时，将源目录 `.QuarkMeta.json` 中的元数据条目原子迁移至目标目录的 `.QuarkMeta.json`。撤销时逆向搬回，确保移动撤销后属性 100% 完好无损。

### 8.2 RenameCommand (重命名命令)
1. **核心职责与逆向改名**：
   - 处理单项与批量重命名及其逆向改回。
2. **`.QuarkMeta.json` 键名同步**：
   - 离散 JSON 元数据以“文件名”为 Key 存储。物理改名时同步将 JSON 中的旧 Key 替换为新 Key；撤销时将新 Key 改回旧 Key。
3. **缓存平滑迁移**：
   - 同步迁移缩略图缓存 Key，防止改名或撤销改名后卡片闪烁或白图。

### 8.3 MetadataCommand (元数据变更命令)
1. **属性修改逆向还原**：
   - 记录修改前后的 OldState 与 NewState 快照（支持星级、颜色、标签、备注、置顶等）。
   - **Execute**：将 NewState 写入 `.QuarkMeta.json`。
   - **Undo (Ctrl+Z)**：将 OldState 写回 `.QuarkMeta.json`，秒级回退属性。
2. **批量操作原子化**：
   - 批量修改多项属性时，必须打包为唯一一个历史原子命令，单次 `Ctrl+Z` 即可批量撤销。

### 8.4 SecureDeleteCommand (粉碎删除命令)
1. **底层物理粉碎**：
   - 调用深层数据抹除算法，覆写物理扇区后销毁文件指针。
2. **元数据与缩略图彻底抹除**：
   - 物理抹除 `.QuarkMeta.json` 中该文件对应的所有属性条目，并清退磁盘缩略图缓存。
3. **不可撤销与撤销栈清退铁律**：
   - **绝对不可撤销 (No Undo)**。
   - 文件粉碎后，`UndoManager` 必须销毁并清退所有与该路径相关的历史命令（如之前的改名或修改元数据指令），杜绝悬空指针与崩溃。

---

## 9. 基于 File_ID 隔离盒与创建时间权威判别回收站规范 (File_ID Trash & Creation Time Restore Specification)

### 9.1 设计理念
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

---

## 13. 筛选面板全多维统计真实同步与无缩略图过滤规范 (Filter Panel Multi-dimension Stats & Thumbnail Failure Filter Specification)

### 13.1 设计理念与真实统计同步
1. **图像比例（Aspect Ratio）真实统计**：
   - 图像比例（横图/竖图/方形/16:9）依赖于项目的物理尺寸信息 (`width` 和 `height`)。在纯磁盘模式下，`DiskItemModel::preloadDimensionsAsync` 快速提取文件头并实时更新记录，`recalculateAndEmitStats()` 实时同步计算并在目录加载或数据变更时触发 `directoryStatsReady` 信号，确保筛选面板各比例计数不再归零。
2. **重复状态（Duplicate Status）信号与 UI 真实绑定**：
   - `ContentPanel` 计算 `duplicateCount`（重复项）与 `uniqueCount`（未重复项）并填充至 `ScanStats`，传递给 `FilterPanel` 的 `populateStats` / `populate` 方法，同步更新界面标签（Label），消除数据传递到 UI 呈现层之间的绑定断层。
3. **“无缩略图 (失败/跳过)” 筛选扩展**：
   - 在第五栏筛选面板的“文件类型”或专用条件组中新增“无缩略图 (失败/跳过)”复选选项。
   - 筛选逻辑：当勾选该复选框时，依据 `thumbStatus == 1`（缩略图提取失败/跳过）进行匹配过滤，便于用户一键定位破坏损坏的图片或渲染失败的格式文件。

---

## 14. 筛选面板界面纯净化与颜色高级筛选复合组件清退规范 (Filter Panel UI Simplification & Color Selector Purge Specification)

### 14.1 设计理念与控件纯洁性铁律
1. **筛选面板五栏布局风格统一铁律**：
   - 第五栏筛选面板（Filter Panel）所有条件分组（包含评级、颜色标记、文件类型、日期、链接/备注等）**必须统一保持极其简洁、高密度、直观的分类复选框列表形式**。
2. **严禁过度设计与复合控件堆叠**：
   - 严禁在筛选面板中引入或复原任何过度设计的复杂复合控件，包括但不限于：色相/渐变滑块（`InlineHueSlider`）、准确度/容差滑块（`m_accuracySlider`）、颜色占比滑块（`m_areaSlider`）、快速颜色文本输入框（`m_editColor`）、标准 12 色矩阵网格以及最近筛选颜色历史块网格（`m_recentColors`）。
3. **颜色标记分组表现形式**：
   - “颜色标记”分组必须恢复为与其他分组完全一致的标准纵向复选框列表（例如“无色标”、“红色”、“黄色”等基础类别），确保右侧筛选面板纵向布局平整高效、无冗余留白、性能极其轻量。

---

## 15. 单一职责物理拆分与功能扩展架构规范 (Single Responsibility Principle & Feature Expansion Specification)

### 15.1 单一职责物理隔离 5 大铁律
1. **一文件一类一职责（头文件彻底解耦）**：
   - 严禁在同一个 `.h` / `.cpp` 中定义多个独立的类。如 `BasicCommands.h` 拆分为独立的命令文件；`FramelessDialog.h` 拆分为独立的对话框文件；`DriveButton.h` 拆分为 `DriveButton` 与 `FolderButton`。
2. **视图只管 UI，不碰业务与磁盘（UI 纯粹化）**：
   - UI 控件仅负责界面布局、样式绘制和事件接收。所有磁盘 I/O、JSON 解析、文件加解密、物理粉碎、后台线程调度，统一移入 Core 控制层与 Service 服务层。
3. **数据管理与模型过滤解耦（MVC 职责清界）**：
   - 状态栏统计、文件隐藏过滤、排序逻辑完全由 `QSortFilterProxyModel` 与 `DiskItemModel` 处理，UI 控件仅监听模型信号。
4. **系统原生消息与应用逻辑剥离（平台解耦）**：
   - Win32 原生硬件消息（`WM_DEVICECHANGE`）剥离至独立的设备监听器，`MainWindow` 不再直接处理平台底层硬件消息。
5. **未开发功能与扩展接口留白**：
   - 右键菜单“外壳保护”（旧“加密”）及快捷键系统通过统一的 `ActionCommand` 中枢解耦挂载，确保未来功能扩充物理隔离。
