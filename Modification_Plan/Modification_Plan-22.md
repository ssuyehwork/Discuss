# 修复导入或拖拽资产包后侧边栏分类计数顽固显示为 0 的大 Bug —— Modification_Plan-22.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
当用户将文件或文件夹导入或拖拽到侧边栏及快速访问（例如 `G:/ArcMeta.Library_G/`）时，物理磁盘能够成功建立解包并标记好的受控 `.arc` 资产文件夹（例如 `00msbhuw71000.arc`）。然而，左侧侧边栏中分类的括号计数（包括“全部数据”、“未分类”、“快速访问 - ArcMeta.Library_G”）却始终显示为 `(0)`，与实际数据严重脱节。

## 2. 问题定位
- 报错模块：`src/meta/CategoryRepo.cpp` 中的计数重算盘点机制。
- 问题根因：
  1. `AssetImporter::importSingleFile` 在导入时，因为资产包物理上是一个文件夹（`.arc`），为了维护底层文件属性，会将该元数据记录的 `is_folder` 值设为 `1`。
  2. 然而，在重新盘点统计总数的 `CategoryRepo::fullRecount()` 函数中，有如下一刀切的代码过滤：
     ```cpp
     for (const auto& meta : snapshot) {
         if (meta.folderId.empty()) continue;
         if (meta.isFolder) continue; // 🚨 此处无脑剔除了所有的文件夹
         ...
     }
     ```
     在最上层的受控物理防火墙 `isInsideManagedLibrary` 的拦截过滤下，所有能够通过审查的项目皆是托管库内的合规 `.arc` 资产文件夹。由于上面的判定，它们全部因 `isFolder == true` 而被中途无脑 `continue` 过滤掉，从而导致对账计数得出 `0` 这一极其严重的逻辑大打架冲突。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 拖拽一个文件到侧边栏空白处之后，虽然成功创建了“00msbhuw71000.arc”文件夹，但是侧边栏分类的计数为何显示为0 | 修改 CategoryRepo::fullRecount() 盘点对账逻辑，使得托管库内以 .arc 结尾的资产包不受 isFolder 的错误剔除并正确计入分类统计中。 | ✅ |

## 4. 详细解决方案
本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的 Git merge diff 代码块进行物理替换，不得做任何自由发挥或脑补改动。

### 4.1 调整 CategoryRepo.cpp 内的计数过滤逻辑
我们对 `src/meta/CategoryRepo.cpp` 的 `fullRecount()` 循环中过滤进行重构升级。不再无脑直接剔除 `meta.isFolder`，而是只有当条目不以 `.arc` 结尾时才认定它是普通子文件夹并予以过滤跳过；如果是合法的 `.arc` 文件夹资产容器，则放行并作为资产单元全量统计。

在 `src/meta/CategoryRepo.cpp` 中：

```
<<<<<<< SEARCH
    auto snapshot = MetadataManager::instance().getLightweightCacheSnapshot();
    for (const auto& meta : snapshot) {
        if (meta.folderId.empty()) continue;
        if (meta.isFolder) continue;

        // 🚨 核心物理防火墙：如果是普通的磁盘导航模式下激活的库外普通项目，绝对禁止其污染侧边栏计数！
        // 各自执行各自的逻辑，两者相互不产生任何关联。
        if (!MetadataManager::instance().isInsideManagedLibrary(meta.path)) {
            continue;
        }
=======
    auto snapshot = MetadataManager::instance().getLightweightCacheSnapshot();
    for (const auto& meta : snapshot) {
        if (meta.folderId.empty()) continue;

        // 🚨 核心物理防火墙：如果是普通的磁盘导航模式下激活的库外普通项目，绝对禁止其污染侧边栏计数！
        // 各自执行各自的逻辑，两者相互不产生任何关联。
        if (!MetadataManager::instance().isInsideManagedLibrary(meta.path)) {
            continue;
        }

        // 仅对不是以 .arc 结尾的普通子文件夹进行剔除，确保合法的受控 .arc 资产包文件夹能够正常计入
        if (meta.isFolder && !meta.path.endsWith(".arc", Qt::CaseInsensitive)) {
            continue;
        }
>>>>>>> REPLACE
```

## 5. 修改边界声明【范围】
本次方案涉及范围：
- [ ] 模块/文件：`src/meta/CategoryRepo.cpp`

明确禁止越界修改的范围：
- [ ] 磁盘模式（DiskNav）相关的物理磁盘文件系统扫描逻辑 —— 不修改

## 6. 实现准则与预警【核心】
1. 依赖 Qt 核心基础：`endsWith` 需要支持大小写不敏感判断，统一采用 `Qt::CaseInsensitive` 进行保护，杜绝因大写 `.ARC` 的拼写导致过滤遗漏。
2. 保持受控防火墙 `isInsideManagedLibrary` 的最高判定权。库内所有的计数逻辑完全归拢且与库外磁盘独立隔离，确保数据流 100% 独立。
3. 纯物理无痛重构对账盘点边界，开箱即用，避免产生多余的全局并发读锁竞争。

## 7. Memories.md 合规检查
本次修改针对分类与受控库的核心计数重构盘点对账逻辑。

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 双轨物理隔离 | 托管库镜像加速模式与磁盘导航物理路径浏览模式各自独立运行。磁盘导航浏览产生的数据绝不写入 SQLite 数据库，而库内统计计数亦完全通过防火墙限制在托管库范围内，不得受外部库外目录干扰。 | ✅ |

## 8. 待确认事项（可选）
无。
