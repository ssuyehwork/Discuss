# .arc 资产包封装架构、侧边栏一等公民重构与自动导入彻底根除物理净化 —— Modification_Plan-2.md

> 状态：待批准执行

## 1. 任务背景
在 DAM（数字资产管理）系统的高性能与清晰架构重构中，上一代方案 `Modification_Plan-1.md` 提出了 `.arc` 资产包物理封装、一等公民分类树状系统、添加日期、多媒体提取和“创建自动导入”的设想。

本方案承接自 `Modification_Plan-1.md` 的核心需求。为了确保系统架构高内聚、职责极其单一，并根据用户最新作出的最高共识决策：**必须对 `ArcMeta.Library_[盘符]`（资产包物理封装与侧边栏一等公民树模型）做极致的重构，但彻底弃用并物理物理根除“创建自动导入”功能（彻底不使用任何外部临时监控 In-Place Watcher，完全删除相关代码，绝不保留任何死角）**。

因此，本方案将完整保留并升级 `.arc` 物理封装、侧边栏一等公民等逻辑，同时对全站所有涉及到“自动导入”、自定义外部文件夹监控、`CustomFolderImportDialog` 等冗余功能展开**毁灭式的物理清除**。

---

## 2. 核心架构演进图谱

```text
[ 物理磁盘层 (D:\ArcMeta.Library_D\) ] ──> 极简平铺，绝对不深度嵌套
  ├── m8crzbs3jb7dj.arc/ ───> 原始文件.psd + _thumbnail.png (256x256)
  └── k9x2p1q4r8v5z.arc/ ───> 照片.jpg + _thumbnail.png (256x256)

[ 侧边栏分类树 (彻底废除“我的分类”外壳) ]
  ├── ArcMeta.Library_D (一等公民，parentId = 0)
  │   └── 文件夹 A (次等分类，parentId = Library_D)
  │       ├── 文件夹 B ───> [关联资产包：m8crzbs3jb7dj]
  │       └── 文件夹 C ───> [关联资产包：k9x2p1q4r8v5z]

[ SQLite 数据库 (唯一的真理源头) ]
  ├── metadata 表 ─────────> file_id ("m8crzbs3jb7dj") + added_at + 逻辑文件名 + 色彩 + 星级
  └── categories 表 ───────> id + parent_id + 逻辑分类名称 (重命名/移动 0.001ms 搞定)

========================================================================
🚨 自动导入（Auto-Import）及 外部 In-Place Watcher 彻底物理根除：
 - 彻底删除 CustomFolderImportDialog 弹窗与相关 UI 项；
 - 彻底切断 DriveBar/CustomMonitoredFolders 相关的读写与对账加载逻辑；
 - 移除一切针对外部临时监控路径的 IOCP Folder Watcher 点火与注销！
```

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|:---:|---|---|:---:|
| 1 | 只对ArcMeta.Library_[盘符]做重构，而“创建自动导入”的部分保持不变。 | 详见 4.1、4.2 节，对 `.arc` 资产包封装、侧边栏一等公民树模型进行全量重构。 | ✅ |
| 2 | 彻底弃用“创建自动导入”功能，也就是彻底放弃使用关于外部临时监控（In-Place Watcher），请将相关代码彻底根除，绝不可以保留 | 详见 4.3 节，将所有与自定义外部监控、新建自动导入对话框、FolderButton 解除监控以及后台对账扫描相关的代码全部根除，绝不残留。 | ✅ |

---

## 4. 详细重构解决方案

### 4.1 重构：`.arc` 物理资产包封装规范与一等公民树模型（继承自 Modification_Plan-1.md）
*   **物理封装机制**：
    所有托管库根目录 `[盘符]:/ArcMeta.Library_[盘符]/` 采用 `.arc` 后缀目录进行物理文件平铺封装：
    ```text
    D:\ArcMeta.Library_D\m8crzbs3jb7dj.arc\
        ├── 原始设计稿.psd               <-- 真实物理源文件
        └── _thumbnail.png               <-- 256x256 高清预渲染缩略图
    ```
*   **13 位 Base36 ID 产生器**：
    在 `src/util/ShellHelper.h` 封装 13 位唯一 ID 产生器，基于毫秒时间戳 + 计数器，将物理文件与逻辑名称彻底解耦。
*   **一等公民设定**：
    在 `CategoryModel` 和 `CategoryPanel` 中，将 `ArcMeta.Library_[盘符]`（如 `ArcMeta.Library_C`）提拔为 `parentId = 0` 的直属根分类节点挂载在分类树最顶层，彻底剔除 `我的分类` 的过度包装。
*   **内容面板过滤**：
    在 `ContentPanel` 中的 `loadCategory` 视图与 `scanDir` 物理扫描中拦截过滤 `.arc` 后缀文件夹，使用户在 UI 视图上完全感知不到 `.arc` 容器的存在。
*   **加入 added_at 字段与排序**：
    在 SQLite 数据库 `metadata` 表中加入 `added_at INTEGER DEFAULT 0`，全量同步到 Model 与排序菜单中。

### 4.2 智能拖拽/导入分流器（`AssetImporter`）
*   当用户向内容面板拖拽单文件、散落的多文件或者整个物理文件夹时，分流进入 `AssetImporter`。
*   **单文件导入**：
    在 D 盘托管库下自动建立 `m8crzbs3jb7dj.arc` 文件夹，将文件移入并提取 `_thumbnail.png` 存储其中。写入 SQLite 数据库，标记其 `added_at` 为当前时间戳。其分类 `id` 归为未分类。
*   **文件夹导入**：
    在 `categories` 逻辑树中递归新建此文件夹及其子目录的逻辑树（其 `parentId` 指向托管库 Root ID）。文件夹里的所有实体文件统一平铺导入至托管库下的 `.arc` 资产包，并在 `category_items` 关系表内将新生成的资产 `file_id` 和逻辑分类 `id` 建立映射绑定。

### 4.3 物理毁灭：彻底净化根除“创建自动导入”与 In-Place Watcher
为了彻底在项目中废除外部临时文件夹的自动导入和变动监听（In-Place Watcher），必须执行以下物理代码删除和重构：

#### 4.3.1 彻底根除 `CustomFolderImportDialog` 对话框
*   在 `src/ui/MainWindow.h` 中彻底删除类 `CustomFolderImportDialog` 的整个类声明。
*   在 `src/ui/MainWindow.cpp` 中彻底删除 `CustomFolderImportDialog::CustomFolderImportDialog` 的构造函数与 `onBrowse`、`selectedPath` 等所有成员函数定义。

#### 4.3.2 彻底根除主窗口中的自动导入 UI 入口与处理
*   **删除盘符栏空白处右键菜单项**：
    在 `MainWindow::onDriveBarContextMenu`（L2101 起）中，彻底删除 QAction “新建自动导入” 及其触发显示 `showNewAutoImportDialog()` 的槽连接。整个 `onDriveBarContextMenu` 可简化为空实现或直接去除。
*   **删除 FolderButton 的监控关联右键菜单**：
    在 `MainWindow::onFolderButtonContextMenu`（L2151 起）中，彻底删除 “新建自动导入” 菜单项。
*   **删除 `showNewAutoImportDialog()`**：
    在 `MainWindow.h` 和 `MainWindow.cpp` 中物理删除成员函数 `showNewAutoImportDialog` 的声明与定义。

#### 4.3.3 彻底根除配置文件和 App 启动对自定义监控路径的监听点火
*   在 `MainWindow::updateCustomFolderButtons()` 中，不再处理 `DriveBar/CustomMonitoredFolders` 配置，彻底删除任何对 customFolders 的读取、存在性对账、写入和点火逻辑。
*   在 `CoreController::initializeCoreComponents`（或 `CoreController::cpp` 起始对账段）中，彻底删除对 `DriveBar/CustomMonitoredFolders` 配置的循环读取、`addWatch()`、以及对 `AutoImportManager::instance().handleRecursiveIngestion()` 的多线程并发加载。
*   在 `SystemBootstrapper` 中，确保仅点火激活合法激活的 `.Library_[盘符]` 的 NativeFolderWatcher，不再点火任何外部 custom folders。

#### 4.3.4 彻底切断 `AutoImportManager` 对自定义文件夹的数据同步
*   在 `src/core/AutoImportManager.h` 和 `AutoImportManager.cpp` 中，彻底切断外部监控队列。由于外部临时监控被彻底弃用，`AutoImportManager` 的生命期与事件接收槽仅服务于原生托管库（`ArcMeta.Library_[盘符]`）的基础同步，彻底删除 `processImportQueue()` 相关的外部队列防抖和临时导入数据库对账链路。

---

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] `src/ui/MainWindow.h`（删除 `CustomFolderImportDialog` 声明，删除 `showNewAutoImportDialog()` 声明）
- [ ] `src/ui/MainWindow.cpp`（物理删除 `CustomFolderImportDialog` 极其成员函数的实现；彻底删除 `showNewAutoImportDialog()`；重写 `onDriveBarContextMenu` 和 `onFolderButtonContextMenu` 移除“新建自动导入”；净化 `updateCustomFolderButtons` 移除一切对 customFolders 的处理和对 Watcher 的 addWatch 逻辑）
- [ ] `src/core/CoreController.cpp`（完全清理启动对账和初始化时，对外部自定义监控配置的读取和 `addWatch` / `handleRecursiveIngestion`）
- [ ] `src/core/AutoImportManager.h` / `src/core/AutoImportManager.cpp`（物理清除服务于自定义临时文件夹导入的所有重对账及导入队列处理逻辑）
- [ ] `src/meta/DatabaseManager.cpp`（扩展 `metadata` 表 `added_at` 字段及索引，保证升级兼容）
- [ ] `src/ui/CategoryModel.cpp` / `src/ui/CategoryPanel.cpp`（实现 `ArcMeta.Library_[盘符]` 提拔为一等公民根节点挂载在侧边栏顶层）
- [ ] `src/ui/ContentPanel.cpp`（实现 `.arc` 资产目录的绝对隐形过滤和 `AssetImporter` 单文件/文件夹导入重构分流）

**明确禁止越界修改的范围：**
- [ ] 物理 MFT 底盘扫描驱动 —— 不修改
- [ ] IOCP 底层 `NativeFolderWatcher` 框架驱动 —— 不修改（保持 NativeFolderWatcher 纯粹的文件变化通知功能，不夹带任何自动导入或 In-Place 监控业务）

---

## 6. 实现准则与安全预警【核心】

1.  **代码编译依赖安全**：彻底清除 `CustomFolderImportDialog` 后，必须检查全站所有文件（如 `main.cpp`、`MainWindow.cpp` 等）是否包含其相关的 include 或者是强转逻辑，防止出现编译失败。
2.  **避免死锁与竞态**：将 `AutoImportManager` 简化为纯托管库的被动同步器，移除外部高防抖队列，确保不与 `NativeFolderWatcher` 的高并发变更产生冲突。
3.  **数据库升级自愈**：在更新库结构时，对 `added_at` 字段使用 `ALTER TABLE` 自愈迁移保护，并确保默认返回值为首创时间戳或 `0`，保障历史库加载安全。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 视图渲染与分类 | 物理 `.arc` 容器 100% 隐形，虚拟子分类和文件正常在内容面板加载展示；侧边栏 `ArcMeta.Library_[盘符]` 为一等公民。 | ✅ 符合 |
| 自动导入彻底根除 | 彻底无死角清理“创建自动导入” QAction 菜单项及 CustomFolderImportDialog 界面，不留下任何相关死代码。 | ✅ 符合 |

---

## 8. 待确认事项（可选）
暂无。本方案已达成最高共识，可随时批准并启动代码重构实施！