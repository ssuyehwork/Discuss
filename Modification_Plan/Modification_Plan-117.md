# 重启全量对账重复扫描与多媒体提取缺陷排查及重构方案 —— Modification_Plan-117.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
本方案承接自 `Development_Plan.md` 中 [2026-07-28] “解决每次启动主程序时重复对账与无谓重新扫描元数据缺陷” 任务。
用户反馈，当被监控的文件夹没有新增任何文件或文件夹，并且已经将元数据储存到了磁盘里的数据库时，每次重启主程序，系统仍然会重复触发全量元数据的二次扫描、全量对账、并触发冗余的多媒体特征（如调色板、宽高尺寸等）提取流水线。
本方案将对该现象的底层逻辑架构进行深度静态排查，找出无谓重复扫描、多媒体高级特征重新提取的根本原因，并给出极致性能的自愈与过滤重构设计，以彻底避免主程序每次启动时冗余对账及多媒体高级特征流水线的二次开销，保护机械磁盘并极大削减 CPU 占用。

## 2. 问题定位

通过对 `src/core/CoreController.cpp`、`src/core/AutoImportManager.cpp`、`src/meta/CategoryRepo.cpp` 以及 `src/meta/MetadataManager.cpp` 开展链路跟踪，我们定位到以下三处关键缺陷：

### 缺陷一：`CategoryRepo::syncPhysicalDirectoryCascade` 启动即对每个文件无条件触发 `addItemToCategory` 和 `registerItem`
在 `CategoryRepo::syncPhysicalDirectoryCascade()` 递归遍历物理树时，代码逻辑无条件执行了以下操作：
```cpp
        // 2. 收集此节点下的文件供批量多媒体提取与注册使用
        for (const auto& fPath : node.files) {
            collectedFilesToProcess.push_back(fPath);
            if (parentCatId > 0) {
                std::string fid;
                if (MetadataManager::fetchWinApiMetadataDirect(fPath, fid)) {
                    CategoryRepo::addItemToCategory(parentCatId, fid, fPath);
                }
            }
        }
```
随后，该函数在尾部无条件调用了多媒体高级特征提取的异步排队方法：
```cpp
    if (!collectedFilesToProcess.empty()) {
        QStringList qPathsToRegister;
        for (const auto& fp : collectedFilesToProcess) {
            qPathsToRegister.append(QString::fromStdWString(fp));
        }
        // 调用 registerItemsAsync，完美一键批处理在后台将文件塞入多媒体解析提取队列 (enqueueBatch)
        MetadataManager::instance().registerItemsAsync(qPathsToRegister, true);
    }
```
**分析**：这导致即使文件在以前已经扫描导入成功，数据库中已经完备地存有其多媒体数据和 status = 1 的记录，在每次重启时，仍然会被重新放入 `collectedFilesToProcess` 队列，强行触发 `registerItemsAsync` 与 `MediaExtractorPipeline` 后台解析。

### 缺陷二：`CategoryRepo::addItemToCategory` 无条件写入带来海量无谓 I/O 与脏标记
在 `CategoryRepo::addItemToCategory` 中，系统会无条件在 SQLite 全局内存库上执行 `INSERT OR REPLACE INTO category_items` 覆写操作，并且因为关联发生变化，会重新调用：
```cpp
            // 如果之前未分类，增加后变成有分类，则减去 uncategorizedCount，增加 categorizedCount 并持久化
            if (getItemCategoryIds(fileId128).size() == 1) {
                s_uncategorizedCount.fetch_sub(1);
                s_categorizedCount.fetch_add(1);
                updatePersistentStat(STAT_CATEGORIZED, 1);
            }
```
**分析**：即使这个文件早就已经归属于对应的 `categoryId`，每次启动时的 `addItemToCategory` 仍旧会重新执行一次 `INSERT OR REPLACE`。这会导致 `category_items` 的 `added_at` 字段被重置为启动时的毫秒时间戳（破坏了用户原有的历史时间排序，变为重启即全部打乱排序），并且频繁标记数据库脏（`m_isDirty`），进而强行触发后台 `DatabaseManager` 落盘备份，造成完全不必要的巨大磁盘 I/O。

### 缺陷三：`MetadataManager::registerItemsAsync` 的批量准入检查缺失
在 `MetadataManager::registerItem(path)` 中，系统确实具备部分单路径双重准入检查：
```cpp
    std::string pFid;
    long long pSize = 0, pMtime = 0;
    if (fetchWinApiMetadataDirect(nPath, pFid, nullptr, &pSize, nullptr, nullptr, &pMtime, nullptr)) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_cache.find(nPath);
        if (it != m_cache.end()) {
            bool metadataValid = true;
            ...
            if (it->second.ingestionStatus == 1 && it->second.fileSize == pSize && it->second.mtime == pMtime && metadataValid) {
                return; // 物理指纹及高级多媒体特征完备且未发生改变，安全返回
            }
        }
    }
```
然而，在 `CategoryRepo::syncPhysicalDirectoryCascade` 递归中使用的**批量管道** `MetadataManager::registerItemsAsync`，则是完全绕过了这一安全检查：
```cpp
void MetadataManager::registerItemsAsync(const QStringList& paths, bool authorized) {
    if (paths.isEmpty()) return;
    (void)authorized;

    (void)QtConcurrent::run([this, paths]() {
        std::vector<std::wstring> stdPaths;
        for (const auto& qp : paths) {
            std::wstring nPath = normalizePath(qp.toStdWString());
            ensureActivated(nPath);
            updateIngestionStatus(nPath, 0); // <--- 这里一律无条件将 ingestion_status 强制刷回 0（待处理状态）！
            stdPaths.push_back(nPath);
        }
        MediaExtractorPipeline::instance().enqueueBatch(stdPaths);
    });
}
```
**分析**：这就导致批量注册路径成了“法外之地”：无论文件是否解析过，在每次启动对账时，只要被 `registerItemsAsync` 批量传入，这些已经完备的元数据的 ingestionStatus 就会被**强制重置为 0**，然后强制推入 `MediaExtractorPipeline` 的大批次提取队列，再次耗费数十秒乃至数分钟的高开销 CPU/GPU/IO 多媒体解析，从而形成严重的性能故障。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 被监控的文件夹并没有新增任何文件或文件夹，而且已经将元数据储存到了磁盘里的数据库，为什么每次重启主程序时又再次扫描数据？ | 在 `CategoryRepo::syncPhysicalDirectoryCascade` 物理递归收集及 `addItemToCategory` 关联覆写逻辑中引入精细完备性判定，跳过已导入成功且未发生物理改变的文件。 | ✅ |
| 2    | 在启动主程序进行全量对账时，跳过已成功导入且指纹/修改时间未发生变化的完备文件，杜绝每次重启时的冗余扫描和特征提取，保护物理磁盘并大幅减少 CPU 占用。 | 在 `registerItemsAsync` 与 `addItemToCategory` 中重构物理指纹及特征完备对账拦截，阻断多媒体二次提取和冗余 I/O 写入。 | ✅ |

---

## 4. 详细解决方案

为了实现“启动对账无感化、零磁盘 I/O 开销与零 CPU 冗余提取”，我们将对递归对账逻辑和批量注册拦截逻辑进行重构加固：

### 第一步：重构 `CategoryRepo::addItemToCategory` 引入重复关联快速拦截 (对齐用户原话："在 CategoryRepo::addItemToCategory 中增加重复关联预检")
1. 在向 `category_items` 表插入之前，安全检索是否已存在该 `(category_id, file_id)` 的记录。
2. 只有在不存在时才执行 `INSERT OR REPLACE INTO category_items` 覆写并重算统计，已存在时直接通过，保留原本的 `added_at` 历史时序不被打乱，彻底阻断由于对账对数据库造成的脏标记。

### 第二步：重构 `MetadataManager::registerItemsAsync` 实施“双重物理指纹与高级特征完备性”批量过滤 (对齐用户原话："在 MetadataManager::registerItemsAsync 中增加文件物理指纹与高级特征双重准入检查")
1. 升级 `registerItemsAsync` 异步流。
2. 在循环处理每一个路径时，先通过极速磁盘检索提取物理大小 `pSize` 和修改时间 `pMtime`。
3. 比对内存缓存中的 RuntimeMeta：
   - 物理大小 `fileSize == pSize` 且 修改时间 `mtime == pMtime`。
   - 文件状态 `ingestionStatus == 1`（代表以前已完备处理）。
   - 图形/SVG 文件的尺寸、宽高比例及主颜色属性是否非空、有效（防脏数据与破损缓存）。
4. 只有对于指纹不符（发生过改动）或者状态不完备（历史提取中途崩溃、字段不齐等破损项）的文件，才调用 `ensureActivated`、重置 `ingestionStatus` 为 0 并推入后台多媒体特征提取流水线。其余项在后台直接通行剔除。

---

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/meta/CategoryRepo.cpp` （具体修改 `addItemToCategory` 重复关联预检）
- [ ] 模块/文件：`src/meta/MetadataManager.cpp` （具体修改 `registerItemsAsync` 批量对账拦截与高级特征过滤）

**明确禁止越界修改的范围：**
- [ ] 物理 MFT 读取模块 `src/core/MftReader.cpp` —— 不修改
- [ ] 数据库连接管理器 `src/meta/DatabaseManager.cpp` —— 不修改

---

## 6. 实现准则与预警【核心】

1. **重入安全性**：对账过程在后台 QtConcurrent 线程中高并发进行，而在执行校验和缓存查询时，`registerItemsAsync` 的过滤逻辑必须先调用 `MetadataManager` 现有的 `m_mutex` 读锁（`shared_lock`）来获取缓存状态，随后仅在确需对脏项或新项做重置时，才发起写锁，以此保证高性能高并发读写。
2. **免编译依赖声明**：本优化完全在原有的元数据和分类服务中执行对账逻辑自愈，不需要引入任何外部工具类或外部文件依赖，保证原有架构的极高纯净度与开箱即用性。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| **侧边栏分类模式** | 所有具备“作用域”的功能（包括统计、监控反馈），其执行范围必须与 UI 顶部的 Focus Line 实时对齐，对托管文件夹及库进行精确过滤和响应。 | ✅ 符合。本方案没有破坏任何 Focus Line 对齐逻辑，仅优化了对账启动时对 `category_items` 和元数据的自愈过滤速度。 |

---

## 8. 待确认事项（可选）
- **无**。
