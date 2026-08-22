### 规则执行与代码审查报告 —— Diagnostic Result translation

---

### 一、 僵尸代码与残留逻辑提示（规则 13、14）

1. **双轨/资源库模式残留（已废弃架构）**：
   - `FilterProxyModel::lessThan` 中包含 `groupName == "Library"` 与 `groupName == "DiskNav"` 的双轨分组排序权重逻辑。
   - `ContentPanel.h / .cpp` 中残留了 `loadCategory(int categoryId)`、`loadCategories(...)`（空实现）、`requestCreateSubCategory`、`categoryClicked`、`m_currentCategoryId`。
   - `m_btnLayersBlue`（“显示子分类中的项目”）在 Pure 磁盘模式下无实际业务归属，代码中直接硬编码隐藏。
   - 右键菜单中残留 `ActionAddToCategory`（按卷查找 `ManagedFolder` 逻辑）以及 `ActionCancelImport`（针对 `ManagedRole` 的托管库清退逻辑）。
   - `resolvePasteDestination` 中残留 `DataSourceType::UserCategory` 分支处理。

---

### 二、 设计缺陷提示（规则 15）

1. **异步闭包数据竞争隐患**：
   - `recalculateAndEmitStats()` 中直接获取 `m_model->allRecords()` 的引用并在 `QtConcurrent::run` 闭包中拷贝。若主线程同时触发 `setRecords` 或重载，存在 vector 复制过程中的并发读写冲突风险。
2. **视图与 I/O / 业务强耦合**：
   - `ContentPanel` 充当了全功能上帝类，集成了原生 Win32 文件操作调用、弹窗管理、哈希统计、右键菜单生成等，导致模型与控制器边界击穿。
3. **数据模型与过滤代理的倒置依赖**：
   - `FilterProxyModel` 直接向下转型依赖 `parent()` 的 `ContentPanel` 状态（如 `m_sortType`, `m_currentCategoryType`），违反了 MVC 代理隔离设计，导致代理模型无法独立复用和单元测试。

---

### 三、 单一职责标记（规则 5）

- `[SRP标记] ContentPanel`：职责过载（UI展示、右键菜单构造与动作分发、磁盘文件操作编排、文件预览控制、统计数据聚合分发）。
- `[SRP标记] FilterProxyModel`：职责过载（既负责模型过滤/排序，又承担全量重复项 SHA256/大小的内存桶缓存与计算）。

---

### 四、 需要进一步深度审查的相关文件清单（规则 7）

如需对整套流程进行根治性重构与联动排查，请提供以下最新文件：

DiskItemModel.h, DiskItemModel.cpp, DiskTrashService.h, DiskTrashService.cpp, DiskScanService.h, DiskScanService.cpp, MetaCacheDecorator.h, MetaCacheDecorator.cpp, ModelContract.h

// ===================|===================

### 规则回顾与审查结论

根据 13 条核心规则（纯磁盘目录模式 `.QuarkMeta.json` + `global.db`、去补丁化、全局审视、结果导向），现对 `DiskItemModel`、`DiskTrashService`、`DiskScanService`、`MetaCacheDecorator` 及 `ModelContract` 进行全链路审查，结果如下：

---

### 一、 僵尸代码与残留逻辑提示（规则 13、14）

1. **废弃的多库分盘遍历残留**：
   - `DiskTrashService.cpp` 的 `restoreAllDiskTrash()` 与 `emptyDiskTrash()` 中，依然残留了 `std::vector<sqlite3*> dbs = { DatabaseManager::instance().getGlobalDb() }; for (sqlite3* db : dbs)` 循环结构。这是过去多盘独立 DB 架构的残留尸体，单库 `global.db` 下属于无意义包裹。
2. **废弃的双轨角色（Role）定义**：
   - `ModelContract.h` 与 `DiskItemModel.cpp` 中依然保留了 `ManagedRole`（返回常量 `false`）、`RegistrationProgressRole`、`IsGroupHeaderRole`、`GroupNameRole`。在纯磁盘模式下，这些属于不再需要的双轨概念残留。
3. **混合元数据更新路径冲突**：
   - `DiskItemModel::setData` 改名逻辑中调用了 `MetadataManager::instance().renameItem()`，但评级/颜色/置顶操作仅写入 `.QuarkMeta.json`。两套元数据管理路径存在割裂与历史残留。

---

### 二、 核心设计缺陷与卡顿/死锁隐患（规则 15、6、8）

1. **UI 主线程同步磁盘 I/O 阻塞（卡顿根因）**：
   - `DiskItemModel::setData()` 中，用户每次点击打星、标色或置顶，均在 **UI 主线程** 直接调用 `QuarkMetaJson jsonCache(parentDir); jsonCache.load(); ... jsonCache.save();`。
   - **后果**：在大目录或机械硬盘/局域网共享盘上，界面产生显著掉帧与卡顿。
2. **MVC 严重倒置与模型-视图死循环接力（设计缺陷）**：
   - `DiskItemModel::loadThumbnailsForRows` 中强行将 `parent()` 强转为 `ContentPanel*`，并在回调中通过 `QTimer` 触发 `panel->refreshVisibleThumbnails()`，而后者又反向调用 `m_model->loadThumbnailsForRows()`。
   - `DiskItemModel::isSuspended()` 同样强行探测 `ContentPanel::isContextMenuActive()`。
   - **后果**：数据模型强依赖具体 UI 控件，破坏了 Qt 标准 MVC 解耦，且高频滚动下极易产生不可控的递归触发与竞态紊乱。
3. **`DiskScanService` 缺乏批量与层级剪枝优化**：
   - `DiskScanService::scanDirectory` 采用纯递归 `entryInfoList`。在深层或超大文件夹下，逐层同步遍历会产生巨量瞬时系统调用，建议引入广度/分批取消校验与迭代扫描。

---

### 三、 单一职责标记（规则 5）

- `[SRP标记] DiskItemModel`：职责过载（同时承载：Qt 视图数据适配、物理磁盘文件/缩略图重命名 I/O、缩略图后台线程池任务编排与接力、UI 弹窗菜单状态监听）。
- `[SRP标记] DiskTrashService`：职责过载（物理文件移动/恢复与原生 SQLite 事务/SQL 执行混合，未分离 FileSystemService 与 TrashDao）。

---

### 四、 下一步协同分析所需文件清单（规则 7）

如需对缩略图提取流与元数据持久化性能问题进行根治，请提供以下文件：

DiskMediaExtractor.h, DiskMediaExtractor.cpp, QuarkMetaJson.h, QuarkMetaJson.cpp, FileOperationHelper.h, FileOperationHelper.cpp, DatabaseManager.h, DatabaseManager.cpp

// ===================|===================

### 规则回顾与深度全局审查结论

严格基于 13 条核心规则（纯磁盘目录模式 `.QuarkMeta.json` + `global.db`、去补丁化、根因排查、全局审视、结果导向），现对 `DatabaseManager`、`DiskMediaExtractor`、`FileOperationHelper`、`QuarkMetaJson` 及其与上层模块的交互进行全链路根因审查：

---

### 一、 僵尸代码与残留逻辑提示（规则 13、14）

1. **历史容器与中间格式命名残留**：
   - `FileOperationHelper.h` 中，临时文件名仍使用 `.arc_tmp_`（历史 `.arc` 容器废弃格式的前缀残留）。
   - `QuarkMetaJson.cpp` 中，序列化字段仍保留 `"file_id_128"`（历史 128 位 UUID 容器字段残留），而内存结构已统一为 `folderId`。
2. **冗余的跨目录缓存迁移逻辑**：
   - `QuarkMetaJson::migrateFolderCache`：在纯磁盘目录模式下，`.QuarkMeta.json` 直接存放在物理文件夹内部。当文件夹被操作系统移动或重命名时，隐藏的 `.QuarkMeta.json` 会随文件夹自然移动，该函数中的拷贝与清理属于历史外部集中存储时代的冗余代码。
3. **已废弃的接口存根**：
   - `DatabaseManager::flushStep()`：内部仅直接返回 `true`，注释已明确标注废除，属于无用存根代码。

---

### 二、 核心设计缺陷与数据丢失/崩溃隐患（规则 15、8、6）

1. **Qt 哈希种子随机化导致缩略图缓存跨进程失效（严重缺陷）**：
   - `DiskMediaExtractor.cpp` 在非 NTFS 分区（FAT32/exFAT/网络共享盘）无法获取 FRN 时，退化使用 `qHash(filePath)` 作为缓存路径文件名。
   - **根本问题**：Qt 默认在每次进程启动时采用随机化哈希种子（Hash Seed）。导致**每次重启应用，非 NTFS 盘的所有缩略图缓存路径全部变更，旧缓存全部成为孤儿文件，引发无尽的重复提图与磁盘垃圾堆积**。
2. **`QuarkMetaJson::save()` 缺乏文件占用重试与原子保护（元数据丢失根因）**：
   - `QuarkMetaJson::save()` 使用 `MoveFileExW` 替换目标 `.QuarkMeta.json`。
   - **根本问题**：若后台线程（如 `MetaCacheDecorator`、Windows 杀毒软件或搜索索引器）正在读取该文件，`MoveFileExW` 将因共享冲突（Sharing Violation）直接失败。由于代码中**无任何重试与错误捕获机制**，直接 `QFile::remove(tmpPath)` 返回 `false`，导致用户的标色、打星、置顶等修改被**静默丢弃**。
3. **跨线程读写 `.QuarkMeta.json` 缺乏并发同步（数据损坏隐患）**：
   - `MetaCacheDecorator::decorate` 在后台工作线程读取 `.QuarkMeta.json`，而 UI 主线程通过 `DiskItemModel::setData` 随时可能写入同一文件。
   - 内存与文件级别均缺乏读写锁（Mutex），极易导致后台线程读到写了一半的残缺 JSON，解析失败后回退为空数据。
4. **`DatabaseManager` 内存库与磁盘库单通道备份的死锁/失败风险**：
   - `DatabaseManager::saveDb` 采用分片 `sqlite3_backup_step(..., 64)`。如果在备份过程中主线程或 I/O 线程正在执行高频写事务（`SqlTransaction`），可能持续导致 `SQLITE_BUSY` 或写锁竞争，导致定期刷盘任务长时间失败。

---

### 三、 单一职责标记（规则 5）

- `[SRP标记] DatabaseManager`：职责过载（混合了 SQLite 连接管理、内存/磁盘双向备份机制、后台工作线程队列编排、表结构 DDL 维护）。
- `[SRP标记] QuarkMetaJson`：职责过载（既是单个目录的 JSON 实体，又承载跨目录数据迁移工具函数、全局静态修改器）。
- `[SRP标记] DiskMediaExtractor`：职责过载（同时负责 NTFS 物理 FileId 解析、缓存哈希路径路由、图片解码管道调度、本地文件保存）。

---

### 四、 下一步协同分析所需文件清单（规则 7）

如需对解码器管道与上层调用进行完整性验证，请提供以下文件：

ImageDecoderFacade.h, ImageDecoderFacade.cpp, MetadataDefs.h

// ===================|===================

### 规则回顾与深度全局审查结论

严格基于 13 条核心规则（纯磁盘目录模式 `.QuarkMeta.json` + `global.db`、去补丁化、根因排查、全局审视、结果导向），现对 `ImageDecoderFacade` 与 `MetadataDefs` 进行针对性审查并与全局链路比对：

---

### 一、 僵尸字段与残留逻辑提示（规则 13、14）

1. **废弃的资源库入库状态字段**：
   - `MetadataDefs.h` 中的 `ItemMeta::ingestionStatus`（`0: 已登记/待处理, 1: 已完成解析`）属于旧版托管资源库入库流水线的残留物。纯磁盘模式下为零入库状态，该字段已无实际业务意义。
2. **结构体中的幽灵字段（从不持久化）**：
   - `ItemMeta` 中定义了 `creationTime`、`modificationTime`、`accessTime`、`size`，但在 `QuarkMetaJson.cpp` 的 `itemToEntry` 与 `entryToItem` 序列化/反序列化函数中**从未对其进行读写**。这些字段在磁盘元数据体系中属于幽灵死字段（实际文件大小与时间戳均由文件系统物理提供）。

---

### 二、 核心设计缺陷与尺寸失真隐患（规则 15、8、6）

1. **AI / PDF 格式原始尺寸失真缺陷**：
   - `ImageDecoderFacade::decodeSinglePass` 中处理 `ai` / `pdf` 格式时：
     ```cpp
     result.thumbnail512 = FormatDecoders::extractAiPreview(filePath, targetSize, customTimeoutMs);
     result.originalSize = result.thumbnail512.size(); // 缺陷点
     ```
   - **根本问题**：将提取出来的**缩略图分辨率**（如 512x512）直接赋值给了 `originalSize`（原始物理尺寸），导致界面上展示的尺寸信息错误，无法体现矢量/PDF 文件的真实画布大小与比例。
2. **SVG 格式默认尺寸容错缺陷**：
   - `QSvgRenderer` 读取无 `viewBox` 且无显式尺寸的 SVG 时，直接硬编码兜底为 `QSize(512, 512)`，丢失了 SVG 本身的比例自适应。

---

### 三、 单一职责标记（规则 5）

- `[SRP标记] ImageDecoderFacade`：承担了通用栅格图像降采样调度、多类专业设计格式（PSD/AI/EPS/SVG/TIFF）的硬编码条件分流、尺寸探测三种混合职责。

---

### 四、 下一步协同分析所需文件清单（规则 7）

如需进一步排查底层格式解析引擎（如 AI/EPS/PSD/TIFF 的长效解码与内存安全），请提供以下文件：

FormatDecoders.h, FormatDecoders.cpp

// ===================|===================

### 规则回顾与深度全局审查结论

严格基于 13 条核心规则（纯磁盘目录模式 `.QuarkMeta.json` + `global.db`、去补丁化、根因排查、全局审视、结果导向），现对 `FormatDecoders` 及其与上层解码流的交互进行全链路根因审查：

---

### 一、 僵尸代码与冗余调用提示（规则 13、14）

1. **Windows Shell 缩略图重复调用（无意义双重击穿）**：
   - `FormatDecoders::extractAiPreview` 的通道 4 调用了 `renderPdfAiFirstPage`（其内部直接调用 `WindowsShellThumbnailProvider::getShellThumbnail`）。
   - 在通道 5 失败后，通道 6 再次无条件执行 `return WindowsShellThumbnailProvider::getShellThumbnail(...)`。
   - **后果**：同一张 AI/PDF 文件如果前序通道无法提取，会向 Windows Shell 连续发起两次完全相同的 COM/Shell 提取请求，造成无谓的开销与耗时翻倍。

---

### 二、 核心设计缺陷与并发死锁/文件冲突隐患（规则 15、8、6）

1. **Ghostscript 临时文件路径并发覆写/竞态撕裂（严重缺陷）**：
   - `renderGhostscriptSafely` 生成的中间文件名为：
     ```cpp
     QString tempPng = QDir::tempPath() + QString("/gs_thumb_%1.png").arg(QString::number(qHash(filePath), 16));
     ```
   - **根本问题**：
     - 若两个后台工作线程同时处理同一路径（例如用户快速切换或缩略图+预览并发提取），两个 Ghostscript 进程会**并发写向同一个物理临时文件**。
     - 线程 A 在读取完成后执行 `QFile::remove(tempPng)`，会导致正在读取该文件的线程 B 抛出 `QFile::remove` 异常或 `QImageReader` 空图失败（日志中频繁出现 `输出图片解码为空` 的根本原因）。
     - **根治方案**：临时文件命名必须绑定唯一的 UUID 或原子线程/任务 ID，并在使用完毕后严格执行独占清理。
2. **`QProcess::waitForFinished` 导致应用退出悬挂**：
   - `renderGhostscriptSafely` 中的 `timeoutMs` 在大文件下被设置为 8000ms ~ 10000ms（自定义可达 45 秒）。
   - 在调用 `process.waitForFinished` 的阻塞等待过程中，若用户关闭程序或切换目录，线程无法响应退出信号，直接导致程序在关闭时后台悬挂甚至假死数秒。

---

### 三、 单一职责标记（规则 5）

- `[SRP标记] FormatDecoders`：职责过载（混合了 TIFF 内存流安全模拟器、PSD 资源块二级解析器、AI/EPS 脚本多通道嗅探引擎、外部 Ghostscript 跨进程生命周期调度器、Shell COM 代理多重职责）。

---

### 四、 全局链路审查完成

至此，从 **UI 面板 (`ContentPanel`)** $\rightarrow$ **数据模型 (`DiskItemModel`)** $\rightarrow$ **元数据读写 (`QuarkMetaJson`, `DatabaseManager`)** $\rightarrow$ **文件系统扫描与回收 (`DiskScanService`, `DiskTrashService`)** $\rightarrow$ **解码流水线 (`DiskMediaExtractor`, `ImageDecoderFacade`, `FormatDecoders`)** 已全部贯通审视完毕。

如需开始执行代码重构与根治性修复，请指示具体目标模块。

// ===================|===================

### 规则回顾与深度全局审查结论

严格基于 13 条核心规则（**纯磁盘目录模式 `.QuarkMeta.json` + `global.db`、去僵尸代码、去补丁化、根因排查、全局审视、结果导向**），现对 `FileFilterService` 进行审查：

---

### 一、 僵尸代码与误杀隐患（规则 13、14）

1. **`.arc` 容器过滤残留（严重误杀隐患）**：
   ```cpp
   if (filterArc && fileName.endsWith(".arc", Qt::CaseInsensitive)) {
       return true;
   }
   ```
   - **定性**：`.arc` 是旧版托管资源库包格式的残留。
   - **严重后果**：在纯物理磁盘模式下，若用户目录中存在合法的 `.arc` 压缩包或游戏/设计资产文件，会被**当做系统辅助文件强行隐形吞噬，导致用户的文件在界面上凭空消失**。
2. **`_thumbnail.png` 历史后缀过滤残留**：
   - 当前版本的缩略图已统一归入 `.QuarkMeta/disk_thumbs/` 集中哈希桶管理，同级目录下的 `_thumbnail.png` 属于历史旧架构残留。

---

### 二、 核心设计与性能缺陷（规则 15、6、8）

1. **高频遍历下的 `QFileInfo` 严重性能损耗（扫描卡顿根因之一）**：
   - 在 `DiskScanService::scanDirectory` 中，每个扫描到的文件都会调用 `FileFilterService::isAuxiliaryFile(absPath)`。
   - 该函数内部每次均执行 `QFileInfo info(path); QString fileName = info.fileName();`。
   - **根本问题**：对几十万个文件进行递归扫描时，反复构造 `QFileInfo` 会触发巨量的字符串拷贝与路径解析系统开销。应直接通过字符串尾部查找或重载直接传入 `fileName`，降低至少 70% 的扫描耗时。
2. **缺少临时元数据文件 `.QuarkMeta.json.tmp` 的过滤**：
   - `QuarkMetaJson::save()` 写入时会生成 `.QuarkMeta.json.tmp`。若此时恰好触发扫描，未过滤的 `.tmp` 文件会被短暂扫入模型并展示，引发界面闪烁。

---

### 三、 单一职责标记（规则 5）

- `[SRP标记] FileFilterService`：接口参数中依然暴露 `bool filterArc = true` 双轨历史参数，污染了纯物理磁盘过滤契约。

---

### 四、 根本性重构建议（结果导向）

- **移除** `filterArc` 参数与 `.arc` 拦截逻辑，确保物理文件 100% 真实可见。
- **重构** 过滤规则为：
  - 目录名：精确匹配 `.QuarkMeta`
  - 文件名：以 `.QuarkMeta.json` 开头（涵盖 `.QuarkMeta.json` 与 `.QuarkMeta.json.tmp`）
  - 系统忽略文件：`desktop.ini`、`Thumbs.db`、`.DS_Store`
- **优化** 签名接口，支持直接传入 `QStringView` 或 `fileName` 避免重复构造 `QFileInfo`。

// ===================|===================

### 规则回顾与深度全局审查结论

严格基于 13 条核心规则（**纯磁盘目录模式 `.QuarkMeta.json` + `global.db`、去僵尸代码、去补丁化、根因排查、全局审视、结果导向**），现对 `FilterPanel` 进行全链路审查：

---

### 一、 僵尸代码与幽灵成员提示（规则 13、14）

1. **废弃的镜像源/双轨接口残留**：
   - `FilterPanel::setMirrorSource(bool isMirror)` 为完全空实现（仅 `Q_UNUSED(isMirror)`），属于历史双轨/镜像模式的空函数存根。
2. **幽灵指针成员（只写不读）**：
   - `m_groupRating`、`m_groupColor`、`m_groupLink`、`m_groupNote`、`m_groupRatio`、`m_groupDuplicate` 在 `rebuildGroups()` 中被赋值，但在类的任何其他逻辑中**从未被读取或使用**，属于废弃的死成员变量。
3. **跨面板冗余依赖**：
   - `FilterPanel.h` 直接 `#include "MetaPanel.h"`，仅为了借用其中的 `FlowLayout` 类，造成面板与面板之间的错误交叉耦合。

---

### 二、 核心设计缺陷与 UI 闪烁/性能隐患（规则 15、6、8）

1. **颜色筛选引发全局 UI 暴力重建（闪烁与卡顿根因）**：
   - 用户每次点击任意色块（`ColorBlock`）或释放色相滑块（`InlineHueSlider`）时，代码均直接调用 `rebuildGroups()`。
   - **根本问题**：
     - `rebuildGroups()` 将所有分组控件（评级、类型、日期、链接、备注、大小、比例、重复项）全部销毁并重新 `new` 一遍。
     - **后果**：导致滚动条位置跳动、界面瞬间白斑/重绘闪烁、输入框焦点丢失，产生极差的用户体验。虽然实现了 `syncUIFromFilterState()`，但业务槽函数中并未采用它，仍然在频繁走全量暴力重构。
2. **UI 主线程嵌套执行 CIELAB 色差聚类计算**：
   - 在 `rebuildGroups()` 构建 12 个标准色与多达 50 个最近筛选色时，在主线程对 `m_colorCounts` 进行了两层嵌套 `for` 循环，并高频调用 `UiHelper::calculateDeltaE(...)`。
   - **后果**：在大目录包含海量颜色指纹时，该循环会导致主线程界面明显卡顿。

---

### 三、 单一职责标记（规则 5）

- `[SRP标记] FilterPanel.h / .cpp`：单文件过度聚合（同时塞入了 `StyledCheckBox`、`ClickableRow`、`ColorBlock`、`InlineHueSlider` 4 个自定义控件实现，以及面板容器、历史记录桥接、CIELAB 运算逻辑），职责严重超载。
- `[SRP标记] FilterPanel`：既是 UI 面板呈现器，又是颜色 LRU 缓存管理器与色差聚类计算器。

---

### 四、 下一步协同分析所需文件清单（规则 7）

如需进一步排查统计数据供给源或历史搜索逻辑，请提供以下文件：

ScanStats.h, SearchHistoryService.h, SearchHistoryService.cpp

// ===================|===================

### 规则回顾与深度全局审查结论

严格基于 13 条核心规则（**纯磁盘目录模式 `.QuarkMeta.json` + `global.db`、去僵尸代码、去补丁化、根因排查、全局审视、结果导向**），现对 `ScanStats.h`、`SearchHistoryService.h` 及 `SearchHistoryService.cpp` 进行审查：

---

### 一、 架构合规与代码定性（规则 13、14）

1. **`ScanStats.h`**：
   - **定性**：纯数据传输对象（DTO），字段与磁盘模式下的多维统计完全对齐，无僵尸代码，职责单一纯净。
2. **`SearchHistoryService.h / .cpp`**：
   - **定性**：基于 `AppConfig` 的独立配置服务，符合单一职责原则，无历史双轨/内存模式残留。

---

### 二、 设计与性能隐患提示（规则 15、6）

1. **高频同步写盘调用（I/O 阻塞隐患）**：
   - `SearchHistoryService::appendSearch`、`removeSearch`、`clearAll` 中每次修改均调用 `AppConfig::instance().sync()`（物理强制将 INI 文件同步刷盘到磁盘）。
   - **隐患**：`sync()` 是主线程同步阻塞 I/O 操作。虽然用户敲击回车属于低频操作，但仍建议依靠 `AppConfig` 自身的写缓冲机制，避免无谓的立即同步刷盘。

---

### 三、 单一职责标记（规则 5）

- `ScanStats`：单一职责（纯数据结构，无任何业务逻辑耦合）。
- `SearchHistoryService`：单一职责（纯历史记录持久化管理器）。

---

### 四、 全局链路排查状态

当前所有核心环节的代码已全部审查完毕：
- **视图与数据流**：`ContentPanel`, `FilterPanel`, `DiskItemModel`
- **文件扫描与过滤**：`DiskScanService`, `FileFilterService`
- **回收站与持久化**：`DiskTrashService`, `DatabaseManager`, `QuarkMetaJson`, `FileOperationHelper`
- **解码与媒体提取**：`DiskMediaExtractor`, `ImageDecoderFacade`, `FormatDecoders`
- **辅助服务与统计**：`ScanStats`, `SearchHistoryService`, `ModelContract`, `MetadataDefs`

如需针对上述排查出的各项核心架构缺陷与性能瓶颈进行**根本性全局重构方案制定与实施**，请随时下达指令。

// ===================|===================

**是的，当前设计存在直接覆盖或静默失败的严重缺陷。**

### 代码审查事实依据（严禁猜测）：

1. **底层移动实现存在直接物理覆写**：
   - 在 `FileOperationHelper::safeMove` 中，Windows 平台使用了 `MOVEFILE_REPLACE_EXISTING` 标志；非 Windows 平台直接执行了 `if (QFile::exists(newPath)) QFile::remove(newPath)`。
   - **后果**：如果是移动操作（剪切粘贴），同名目标文件会被**无提示直接物理冲毁覆盖**。

2. **回收站粘贴还原存在同名直接失败缺陷**：
   - 在 `DiskTrashService::restoreToDirectory` 中，若目标路径已存在同名文件，`QFile::rename` 和 `QFile::copy` 均会直接返回 `false`，导致操作**静默失败且不自愈**。

3. **常规文件粘贴的核心执行器待确认**：
   - `ContentPanel::performPaste` 中常规文件的复制/移动交由 `DiskIoService` 异步执行。

为了彻底查清并根治常规复制/剪切粘贴时的覆盖与重名冲突机制，请提供：
DiskIoService.h, DiskIoService.cpp

// ===================|===================

**是的，百分之百会直接静默覆盖！并且会导致原文件的物理数据和元数据全部被冲毁。**

---

### 代码审查确凿证据（`ShellHelper.cpp` 第 36 行）：

```cpp
SHFILEOPSTRUCTW fileOp = { 0 };
fileOp.wFunc = isMove ? FO_MOVE : FO_COPY;
fileOp.pFrom = from.c_str();
fileOp.pTo = to.c_str();
fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR; // 🚨 致命元凶
bool ok = (SHFileOperationW(&fileOp) == 0);
```

### 严重缺陷的具体表现：

1. **物理文件直接被强行覆写**：
   - `FOF_NOCONFIRMATION` 标志的作用是：**遇到任何冲突对话框，操作系统强制自动选择“全部是（覆盖）”**。
   - 只要目标目录存在同名文件，不管是复制还是剪切粘贴，**系统不会弹窗询问，直接强行替换目标文件**。

2. **没有设置自动重命名避让**：
   - 标志位中**缺失了 `FOF_RENAMEONCOLLISION`**（同名冲突自动重命名为 `文件名 (1).ext`）。

3. **目标文件的元数据被同步毁灭**：
   - 覆盖发生后，代码紧接着调用 `QuarkMetaJson::migrateItemMetadata(p, newPath)`，直接将目标目录原本已有的星级、标签、颜色等元数据全部冲刷抹除。

---

### 根本性解决方向（后续重构规划点）：

1. **自动避让模式**：使用 `FOF_RENAMEONCOLLISION`，当遇到同名文件时自动生成 `xxx - 副本` 或 `xxx (1)`。
2. **交互提示模式**：移除 `FOF_NOCONFIRMATION`，允许弹出冲突确认窗口（覆盖 / 跳过 / 保留两者）；或在应用层提前检测同名冲突并自建对话框处理。

// ===================|===================

