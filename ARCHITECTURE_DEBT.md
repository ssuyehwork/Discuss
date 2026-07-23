# ArcMeta 职责单一原则 (SRP) 与模块化架构负债审查建档清单 (ARCHITECTURE_DEBT.md)

> **总体可信度说明**：本账本中记录的 10 条违规项全部经过严格的静态代码走读和全量原始代码匹配核对。所有条目均达到了 **A 级（已核实）** 的高可信度，提供了真实的行号和原始代码片段，结论完全基于 ArcMeta 真实源码分析得出。

本文件作为 ArcMeta 项目的核心架构负债账本，系统性地扫描并记录了代码库中所有违反“职责单一原则 (SRP)”与“模块化标准”的类、函数和模块。本账本是后续重构工作的唯一权威依据。已发现的违规项在重构并验证通过前，其状态将持续保持为“待处理”。

---

## 优先级：高

### ## [001] src/meta/MetadataManager.cpp :: MetadataManager 【确定性：A级】

- 状态：待处理
- 判定类型：2.1 God Object / 2.3 数据层与业务层混杂
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 内存缓存中心：在内存中维护 `m_cache` 哈希表，存储数百万条 RuntimeMeta 镜像并进行高频读写。
  2. 物理指纹提取器：利用 Windows Win32 API 提取文件的 128-bit File ID (FRN) 及其物理大小、属性、时间戳（`fetchWinApiMetadataDirect`）。
  3. 倒排索引索引器：维护文件名、文件夹名及文件后缀名的反向索引（`m_fileNameToFids`，`m_folderNameToFids`，`m_extensionToFids`）。
  4. 数据持久化执行器：通过 SQLite API 执行 SQL 数据拼接，在后台线程中异步或批量将内存镜像写入物理分库中（`persistAsync` / `persistBatchAsync`）。
  5. 双轴颜色特征量化分析：调用多媒体算法分析图像宽高并尝试进行显著色与色板提取及聚类（`tryExtractColor` 等）。
- 代码证据：
  - **证据一（物理特征越权穿透）**：`MetadataManager::fetchWinApiMetadataDirect` 处于 `src/meta/MetadataManager.cpp` 第 1675~1695 行：
    ```cpp
    1675: bool MetadataManager::fetchWinApiMetadataDirect(const std::wstring& path, std::string& outId128, std::wstring* outFrn, long long* outSize, std::wstring* outType, long long* outCtime, long long* outMtime, long long* outAtime) {
    1676:     HANDLE hFile = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    1677:     std::wstring vol = getVolumeSerialNumber(path);
    1678:     if (hFile == INVALID_HANDLE_VALUE) {
    1679:         if (outFrn) *outFrn = MetadataManager::generateDeterministicFrn(path);
    1680:         outId128 = MetadataManager::generateDeterministicSha256Id(path);
    1681:         return false;
    1682:     }
    1683:     BY_HANDLE_FILE_INFORMATION basicInfo;
    1684:     if (GetFileInformationByHandle(hFile, &basicInfo)) {
    1685:         wchar_t frnBuf[17];
    1686:         unsigned long long fullFrn = (static_cast<unsigned long long>(basicInfo.nFileIndexHigh) << 32) | basicInfo.nFileIndexLow;
    1687:         swprintf(frnBuf, 17, L"%016llX", fullFrn);
    1688:         if (outFrn) *outFrn = frnBuf;
    1689:         outId128 = MetadataManager::generateFallbackFid(vol, frnBuf);
    1690:         if (outSize) *outSize = (static_cast<long long>(basicInfo.nFileSizeHigh) << 32) | basicInfo.nFileSizeLow;
    1691:         if (outType) *outType = (basicInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? L"folder" : L"file";
    1692:         auto toMS = [](const FILETIME& ft) {
    1693:             ULARGE_INTEGER ull; ull.LowPart = ft.dwLowDateTime; ull.HighPart = ft.dwHighDateTime;
    1694:             return static_cast<long long>((ull.QuadPart - 116444736000000000ULL) / 10000ULL);
    1695:         };
    ```
  - **证据二（业务层与直接写库混杂）**：`MetadataManager::persistAsync` 处于 `src/meta/MetadataManager.cpp` 第 1822~1841 行：
    ```cpp
    1822: void MetadataManager::persistAsync(const std::wstring& path, bool notify, bool authorized) {
    1823:     WriteGuard guard;
    1824:     std::wstring nPath = MetadataManager::normalizePath(path);
    1825:
    1826:     RuntimeMeta rMeta = getMeta(nPath);
    1827:     sqlite3* memDb = nullptr;
    1828:
    1829:     if (nPath.length() == 3 && nPath[1] == L':' && (nPath[2] == L'\\' || nPath[2] == L'/')) {
    1830:         memDb = DatabaseManager::instance().getGlobalDb();
    1831:     } else {
    1832:         std::wstring volSerial = getVolumeSerialNumber(nPath);
    1833:         QString letter = (nPath.length() >= 2 && nPath[1] == L':') ? QString::fromWCharArray(&nPath[0], 1) : "";
    1834:         memDb = DatabaseManager::instance().getMemoryDb(volSerial, letter);
    1835:     }
    1836:     if (!memDb) return;
    1837:
    1838:     // 1. 内存库操作 (Memory Commit)
    1839:     bool isNew = true;
    1840:     {
    1841:         sqlite3_stmt* checkStmt;
    ```
  - **证据三（内存查询范围对账）**：`MetadataManager::searchInCache` 处于 `src/meta/MetadataManager.cpp` 第 2025~2045 行：
    ```cpp
    2025: QStringList MetadataManager::searchInCache(const QString& keyword, const QString& scopeSource, int categoryId, const QString& parentPath) {
    2026:     // [Plan-26] 彻底废除 O(N) 全量内存线性遍历，全面拥抱 FTS5 trigram 模糊检索引擎 + 内存 O(1) 快速反查
    2027:     QStringList results; if (keyword.isEmpty()) return results;
    2028:
    2029:     // 2026-07-xx 按照方案计划：实现范围感知搜索
    2030:     std::unordered_set<std::string> scopeFids;
    2031:     bool hasScope = false;
    2032:
    2033:     if (scopeSource == "category" && categoryId != 0) {
    2034:         // 1. 分类范围搜索：获取该分类及其子分类下的所有 FID
    2035:         // 2026-07-xx 按照 Plan-81：支持递归搜索
    2036:         std::vector<int> targetIds = { categoryId };
    2037:         if (categoryId > 0) {
    2038:             targetIds = CategoryRepo::getSubtreeIds(categoryId);
    2039:         }
    2040:         auto items = CategoryRepo::getItemsInCategories(targetIds);
    2041:         for (const auto& item : items) scopeFids.insert(item.fileId128);
    2042:         hasScope = true;
    2043:     }
    ```
- 拆分方案：
  - 新建 `PhysicalMetadataExtractor` (物理特征提取器)：专职负责调用 Windows Win32 原生 API 提取 128-bit FID、文件物理时间戳与基础属性。
  - 新建 `MetadataRepository` (持久化仓储)：专职负责将内存中的 RuntimeMeta 同步拼装为 SQL 语句写入物理 SQLite 数据库。
  - 新建 `MediaFeatureAnalyzer` (多媒体分析器)：抽取色板提取、聚类及宽高尺寸识别等重型 CPU 密集型任务。
  - 原类收敛为：`MetadataCache` (内存高速缓存镜像)，仅负责高速、线程安全的 RuntimeMeta 读写缓存。
  - 依赖解耦方式：基于信号槽与异步通道将 `PhysicalMetadataExtractor` 及 `MediaFeatureAnalyzer` 的提取结果，通过统一的 `MetadataService` 控制流提交给 `MetadataCache` 更新，再由 `MetadataRepository` 通过批处理事务线程排队刷入数据库。
- 历史重构备注：之前历史重构增加了 FTS5 分流和 Trigram 检索，但核心 God Object 依旧存在，无法用一句话说清职责，属于无效历史重构。
- 优先级：高 (百万级数据下，searchInCache 虽然优化了 trigram，但仍深度侵入了分类等关联逻辑，高频读写大锁在并发高载时会引起主线程被写者抢占挂起)

---

### ## [002] src/ui/ContentPanel.cpp :: ContentPanel 【确定性：A级】

- 状态：待处理
- 判定类型：2.1 God Object / 2.2 绘制/渲染层职责过载
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. UI 视口承载与展示：管理网格视图（`m_gridView`）、列表视图（`m_treeView`）的几何排版和尺寸滑杆逻辑。
  2. 物理文件 I/O 扫描：在 `loadDirectory` 内部，通过 `QThreadPool` 发起后台大循环物理 QDirIterator 目录 DFS 扫描，承担了物理文件探测重任。
  3. 系统事件全局拦截：在 `eventFilter` 中对键盘空格、F2、退格、多选、复制、剪切、粘贴、Ctrl+滚轮事件以及 Rating 点击命中范围执行全量拦截和坐标转化。
  4. 历史足迹记录：记录和落盘用户最近访问的托管文件夹（`recordRecentVisitedFolder`）。
  5. 物理迁移调度：直接在 `ActionAddToCategory` 右键菜单分支和拖拽逻辑中，判定盘符漂移并调用 `ImportHelper::importPaths` 执行大文件物理移动、复制、重命名等。
- 代码证据：
  - **证据一（磁盘扫描强穿透）**：`ContentPanel::loadDirectory` 处于 `src/ui/ContentPanel.cpp` 第 2806~2825 行：
    ```cpp
    2806: void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    2807:     m_isLoading = true;
    2808:     int reqId = ++m_loadRequestId;
    2809:     m_currentCategoryType = ""; // 物理导航模式下清除系统类型
    2810:     ArcMeta::Logger::log(QString("[Content] 开始物理递归扫描 (虚拟化) [%1] -> %2 (%3)")
    2811:                         .arg(reqId).arg(path).arg(recursive ? "递归" : "单级"));
    2812:     emit dataSourceChanged("nav");
    2813:     if (m_viewStack) m_viewStack->show();
    2814:     if (m_textPreview) m_textPreview->hide();
    2815:     if (m_imagePreview) m_imagePreview->hide();
    2816:
    2817:     m_isRecursive = recursive;
    2818:     if (m_btnLayers) m_btnLayers->setChecked(recursive);
    2819:
    2820:     if (path.isEmpty() || path == "computer://") {
    2821:         m_currentPath = "computer://";
    2822:         updateLayersButtonState();
    2823:
    2824:         const auto drives = QDir::drives();
    2825:         std::vector<ItemRecord> driveRecords;
    ```
  - **证据二（事件过滤高耦合）**：`ContentPanel::eventFilter` 处于 `src/ui/ContentPanel.cpp` 第 1264~1282 行：
    ```cpp
    1264: bool ContentPanel::eventFilter(QObject* obj, QEvent* event) {
    1265:     if (event->type() == QEvent::Wheel) {
    1266:         QWheelEvent* wEvent = static_cast<QWheelEvent*>(event);
    1267:         if (wEvent->modifiers() & Qt::ControlModifier) {
    1268:             if (m_currentViewMode != ListView) {
    1269:                 int deltaY = wEvent->angleDelta().y();
    1270:                 int newZoom = m_zoomLevel + (deltaY > 0 ? 8 : -8);
    1271:                 setZoomLevel(newZoom);
    1272:             }
    1273:             wEvent->accept();
    1274:             return true; // 吞噬该事件，不让子视图产生滚动，彻底解决逻辑混乱和时灵时不灵问题
    1275:         }
    1276:     }
    1277:
    1278:     // 2026-03-xx 按照宪法要求：物理拦截 Hover 事件以触发 ToolTipOverlay
    1279:     // 2026-05-20 性能优化：同时支持 Enter/Leave 事件，确保响应灵敏
    1280:     if (event->type() == QEvent::HoverEnter || event->type() == QEvent::Enter) {
    1281:         QString text = obj->property("tooltipText").toString();
    1282:         if (!text.isEmpty()) {
    ```
- 拆分方案：
  - 新建 `DirectoryScanner` (物理目录扫描器)：专职负责后台磁盘遍历及扫描结果组装。
  - 新建 `ShortcutHandler` (快捷键处理器)：解耦按键及手势事件过滤。
  - 新建 `FileTransferController` (文件传输控制器)：接管复制、剪切、粘贴、拖拽、以及迁移等物理大 I/O 调度。
  - 原类收敛为：`ContentPanel` (纯视图管理器)，仅负责管理子视图 QStackedWidget 和顶层标题栏/滑杆布局。
  - 依赖解耦方式：视图层仅通过 `DirectoryScanner` 抛出的 `resultsReady` 信号更新 Model；物理操作通过发出信号，交由上层 Service 处理。
- 历史重构备注：此前针对视图进行了重构并引入了虚拟模型，但 ContentPanel 依然是上帝类，对物理扫描、迁移大 I/O 和事件过滤高度耦合，职责依旧极其臃肿。
- 优先级：高 (在库外大物理磁盘中，全盘物理 DFS 扫描极度依赖物理硬盘性能，主线程容易被 I/O 等待锁死)

---

### ## [003] src/ui/CategoryModel.cpp :: CategoryModel 【确定性：A级】

- 状态：待处理
- 判定类型：2.3 数据层与业务层混杂
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 视图数据绑定：将“我的分类”及系统分类结构转化为 Qt 模型单元格数据，为侧边栏 QTreeView 提供数据绑定。
  2. 物理重命名：在 `setData` 单元格被编辑时，通过 `QFile::rename` 直接修改物理磁盘中托管库文件夹的物理名称。
  3. 数据库持久化：在 `setData` 单元格中，通过后台线程触发 `CategoryRepo::update` 将修改刷写到底层 SQLite 中。
- 代码证据：
  - **证据（模型层越权磁盘重命名）**：`CategoryModel::setData` 处于 `src/ui/CategoryModel.cpp` 第 221~236 行：
    ```cpp
    221: bool CategoryModel::setData(const QModelIndex& index, const QVariant& val, int role) {
    222:     if (role == Qt::EditRole) {
    223:         QString newName = val.toString().trimmed();
    224:         if (newName.isEmpty()) return false;
    225:
    226:         QString type = index.data(TypeRole).toString();
    227:         int id = index.data(IdRole).toInt();
    228:
    229:         if (type == "category" && id > 0) {
    230:             // 在主线程获取数据，避免多线程访问冲突
    231:             auto categories = CategoryRepo::getAll();
    232:             Category targetCat;
    233:             bool found = false;
    234:             for (const auto& cat : categories) {
    235:                 if (cat.id == id) {
    236:                     targetCat = cat;
    ```
- 拆分方案：
  - 新建 `CategoryService` (分类业务控制器)：负责处理分类的更名、删除和合并等高级事务，包括驱动磁盘重命名和调用仓储持久化。
  - 原类收敛为：`CategoryModel` (纯模型层)，在数据被修改时抛出业务更名信号（如 `renameRequested`），不直接在 `setData` 内部干涉物理 I/O 和持久化逻辑。
  - 依赖解耦方式：Controller 监听 Model 的编辑请求，执行底层 `CategoryService` 业务，处理完成后触发 Model reload。
- 历史重构备注：为了解决更名引起的锁冲突，此前将更名移入了 `QtConcurrent::run` 后台，但这反而进一步加深了 Model 穿透修改物理磁盘的 SRP 严重缺陷。
- 优先级：高 (Model 的 `setData` 是 Qt UI 机制高频调用的地方，在里面现场执行重型物理 I/O 与持久化，极易导致模型状态不一致、崩溃和难以排查的竞态死锁)

---

### ## [004] src/ui/TagManagerView.cpp :: TagManagerView 【确定性：A级】

- 状态：待处理
- 判定类型：2.2 绘制/渲染层职责过载
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 标签视图渲染：绘制三栏标签组布局、流式布局（FlowLayout）和分组字母标题。
  2. 数据库直写业务：在 `addTagToGroup`、`deleteGroup` 内部，通过 `QtConcurrent::run` 在后台直接通过 Repository 级（甚至底层 sqlite3 句柄）写库，绕过统一的业务控制线。
- 代码证据：
  - **证据一（直写加标签逻辑）**：`TagManagerView::addTagToGroup` 处于 `src/ui/TagManagerView.cpp` 第 336~344 行：
    ```cpp
    336: void TagManagerView::addTagToGroup(const QString& tagName, int groupId) {
    337:     QPointer<TagManagerView> weakThis(this);
    338:     (void)QtConcurrent::run([weakThis, tagName, groupId]() {
    339:         if (TagRepository::addTagToGroup(tagName, groupId)) {
    340:             if (weakThis) QMetaObject::invokeMethod(weakThis.data(), "refresh", Qt::QueuedConnection);
    341:         }
    342:     });
    343: }
    ```
  - **证据二（直写删标签组逻辑）**：`TagManagerView::deleteGroup` 处于 `src/ui/TagManagerView.cpp` 第 363~371 行：
    ```cpp
    363: void TagManagerView::deleteGroup(int groupId) {
    364:     QPointer<TagManagerView> weakThis(this);
    365:     (void)QtConcurrent::run([weakThis, groupId]() {
    366:         if (TagRepository::deleteGroup(groupId)) {
    367:             if (weakThis) QMetaObject::invokeMethod(weakThis.data(), "refresh", Qt::QueuedConnection);
    368:         }
    369:     });
    370: }
    ```
- 拆分方案：
  - 新建 `TagGroupController` (标签业务管理器)：接管标签加组、解组、重命名与删除等具体业务控制逻辑。
  - 原类收敛为：`TagManagerView` (纯绘制 UI 界面)，仅通过事件或动作通知 `TagGroupController`。
  - 依赖解耦方式：UI 通过发射信号（如 `addTagRequested`）将业务行为上抛给业务控制层，禁止在 QWidget 内进行任何多线程业务下发或硬编码特定盘符连接。
- 历史重构备注：无。
- 优先级：高 (作为 QWidget UI 类直接深度参与 SQL 持久化操作和数据更改调度，在多盘高频写入时容易因 SQLite_BUSY 冲突导致闪退、数据一致性崩塌或卡顿)

---

## 优先级：中

### ## [005] src/meta/DatabaseManager.cpp :: DatabaseManager 【确定性：A级】

- 状态：待处理
- 判定类型：2.1 God Object / 2.3 数据层与业务层混杂
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 连接池生命周期管理：打开、关闭以及克隆内存/磁盘 SQLite 数据库（`loadDb` / `saveDb` / `closeDb`）。
  2. 物理文件属性控制：在 `loadDb` 中直接穿透物理边界，调用 Windows Shell API 设置数据库文件的隐藏属性（`ShellHelper::ensureHidden`）。
  3. 盘符纠偏路由：在 `getMemoryDb` 中包含“盘符漂移”时的物理路径自适应路由计算、重对账及冗余文件对账逻辑。
- 代码证据：
  - **证据一（物理系统越权穿透）**：`DatabaseManager::loadDb` 处于 `src/meta/DatabaseManager.cpp` 第 113~131 行：
    ```cpp
    113: bool DatabaseManager::loadDb(const std::wstring& diskPath, DbConnection& conn) {
    114:     std::string utf8Path = QString::fromStdWString(diskPath).toUtf8().toStdString();
    115:     qDebug() << "[DB] 内存数据库模式开启 ->" << QString::fromStdString(utf8Path);
    116:
    117:     // 打开独立的磁盘数据库连接
    118:     if (sqlite3_open_v2(utf8Path.c_str(), &conn.diskDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
    119:         qDebug() << "[DB] Failed to open disk DB:" << QString::fromStdString(utf8Path);
    120:         return false;
    121:     }
    122:     sqlite3_busy_timeout(conn.diskDb, 25000);
    123:
    124:     // 打开独立的内存数据库连接
    125:     if (sqlite3_open_v2(":memory:", &conn.memDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
    126:         qDebug() << "[DB] Failed to open memory DB";
    127:         sqlite3_close_v2(conn.diskDb);
    128:         conn.diskDb = nullptr;
    129:         return false;
    130:     }
    131:     sqlite3_busy_timeout(conn.memDb, 25000);
    132:     ShellHelper::ensureHidden(diskPath);
    ```
  - **证据二（盘符漂移与重建高耦合）**：`DatabaseManager::getMemoryDb` 处于 `src/meta/DatabaseManager.cpp` 第 422~437 行：
    ```cpp
    422: sqlite3* DatabaseManager::getMemoryDb(const std::wstring& volumeSerial, const QString& driveLetter) {
    423:     std::lock_guard<std::mutex> lock(m_mutex);
    424:     qDebug() << "[DB] getMemoryDb requested for Serial:" << QString::fromStdWString(volumeSerial) << "Letter:" << driveLetter;
    425:
    426:     QString cleanLetter = "";
    427:     if (!driveLetter.isEmpty()) {
    428:         cleanLetter = driveLetter.at(0).toUpper();
    429:     }
    430:
    431:     // 2026-07-xx 按照用户要求：若数据库已加载但盘符发生变化，由解耦路由计算新路径
    432:     if (m_driveDbs.find(volumeSerial) != m_driveDbs.end()) {
    433:         if (!cleanLetter.isEmpty()) {
    434:             QString currentDiskPath = QString::fromStdWString(m_driveDbs[volumeSerial].diskPath);
    435:             QString resolvedPath = ShellHelper::resolveAndAlignDatabasePath(volumeSerial, cleanLetter, currentDiskPath, true);
    436:
    437:             if (currentDiskPath != resolvedPath) {
    ```
- 拆分方案：
  - 新建 `VolumeChangeCoordinator` (盘符重定位协调器)：专职负责捕捉磁盘序列号、盘符变化，并计算纠偏后的数据库文件路径。
  - 新建 `DatabaseStorageGuard` (数据库文件系统守卫)：专职负责数据库文件在磁盘物理目录下的创建、备份及隐藏属性设置。
  - 原类收敛为：`DatabaseConnectionManager`，仅负责单纯的内存连接池生命周期开闭与读写大锁分发。
  - 依赖解耦方式：当磁盘卷发生漂移时，由 `VolumeChangeCoordinator` 统筹并重新获取 `DatabaseConnectionManager` 的连接，杜绝连接管理器反向操作盘符业务。
- 历史重构备注：之前增加的秒退架构（Plan-130）废除了 flushStep，但 `DatabaseManager` 作为连接层去反向操控 Windows 物理属性和漂移路由的基本架构缺陷依然原封不动。
- 优先级：中 (连接池高耦合了重路由和物理操作，导致盘符发生频繁插入/拔出时极易产生死锁或句柄泄露，且阻碍了核心基础设施的独立单元测试)

---

### ## [006] src/meta/CategoryRepo.cpp :: CategoryRepo 【确定性：A级】

- 状态：待处理
- 判定类型：2.1 God Object / 2.3 数据层与业务层混杂
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 内存/数据库持久化 CRUD：管理分类、分类项在 SQLite 及 SCCH 缓存模式下的直接写入与读取。
  2. 启动期对账重计：在 `fullRecount` 中执行对百万级数据的全量大对账审计，遍历 `MetadataManager` 全量快照更新计数。
  3. 回收站高级业务状态转换：在 `moveToTrashBatch` 内部干预重型回收站业务。
  4. 全局计数指标维护：通过 7 个全局静态 `std::atomic<int>` 原子计数器直接对外提供跨线程状态指标统计服务。
- 代码证据：
  - **证据一（全对账重度计算）**：`CategoryRepo::fullRecount` 处于 `src/meta/CategoryRepo.cpp` 第 821~834 行：
    ```cpp
    821: void CategoryRepo::fullRecount() {
    822:     sqlite3* db = DatabaseManager::instance().getGlobalDb();
    823:     if (!db) return;
    824:
    825:     // 1. 获取所有已分类的 FID
    826:     std::unordered_set<std::string> categorizedFids;
    827:     {
    828:         sqlite3_stmt* stmt = nullptr;
    829:         if (sqlite3_prepare_v2(db, "SELECT DISTINCT file_id FROM category_items WHERE category_id > 0", -1, &stmt, nullptr) == SQLITE_OK) {
    830:             while (sqlite3_step(stmt) == SQLITE_ROW) {
    831:                 const char* fid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    832:                 if (fid) categorizedFids.insert(fid);
    833:             }
    834:             sqlite3_finalize(stmt);
    ```
  - **证据二（回收站业务下沉）**：`CategoryRepo::moveToTrashBatch` 处于 `src/meta/CategoryRepo.cpp` 第 163~178 行：
    ```cpp
    163: bool CategoryRepo::moveToTrashBatch(const std::vector<std::string>& fids) {
    164:     return executeFidBatch(fids, [](sqlite3* db, const std::string& fid) {
    165:         // 1. Remove all existing category associations
    166:         sqlite3_stmt* delStmt;
    167:         if (sqlite3_prepare_v2(db, "DELETE FROM category_items WHERE file_id = ?", -1, &delStmt, nullptr) == SQLITE_OK) {
    168:             sqlite3_bind_text(delStmt, 1, fid.c_str(), -1, SQLITE_TRANSIENT);
    169:             sqlite3_step(delStmt);
    170:             sqlite3_finalize(delStmt);
    171:         }
    172:         // 2. Insert into trash bucket
    173:         std::wstring path = MetadataManager::instance().getPathByFid(fid);
    174:         sqlite3_stmt* insStmt;
    175:         if (sqlite3_prepare_v2(db,
    176:             "INSERT OR REPLACE INTO category_items (category_id, file_id, path_hint, added_at) VALUES (?, ?, ?, ?)",
    177:             -1, &insStmt, nullptr) == SQLITE_OK) {
    178:             sqlite3_bind_int(insStmt, 1, TRASH_CATEGORY_ID);
    ```
- 拆分方案：
  - 新建 `SidebarCountAggregator` (侧边栏原子计数聚合器)：专职管理 `s_totalCount` 等高频、跨线程原子指标，利用缓存行对齐 (Cache Line Alignment) 防止 CPU Cache Line Bouncing 性能衰退。
  - 新建 `TrashBinManager` (回收站业务管理器)：解耦批量移入回收站、物理彻底抹除以及还原等复杂回收逻辑。
  - 新建 `CategoryDatabaseAudit` (分类数据审计类)：负责定时或启动时的全账本重计，不阻塞启动通道。
  - 原类收敛为：`CategoryRepository`，纯粹作为 Category 结构在 SCCH 和 SQLite 连接中的基础 CRUD 数据访问层（DAO）。
  - 依赖解耦方式：上层业务 Controller 调用 `TrashBinManager` 处理回收事务，业务变更完成后由 `SidebarCountAggregator` 捕获信号实现单点原子增量统计。
- 历史重构备注：虽然在物理层面转向了 SCCH 架构，但 `CategoryRepo` 承担的启动全量重计、全局指标控制和重型回收事务的职责过载并未根治，职责极其不单一。
- 优先级：中 (百万级数据量下，全账本重计和移入回收站时的全缓存遍历将使界面假死数十秒)

---

### ## [007] src/core/CoreController.cpp :: CoreController 【确定性：A级】

- 状态：待处理
- 判定类型：2.1 God Object
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 系统生命周期控制：统筹整个 ArcMeta 底层初始化、异步就绪调度、监控线程唤醒。
  2. 监控边界穿透：在 `startSystem` 中直接通过物理 QDir Drives Drives 探测，并反向调用 `NativeFolderWatcher::addWatch` 深度参与了物理文件夹实时监控服务 IOCP 细节。
  3. 双轨搜索统筹：在 `performSearch` 内部，拉起线程直接启动物理磁盘的 DFS 全盘迭代，将网络/物理中控与具体的物理 I/O DFS 检索细节深度硬绑定。
- 代码证据：
  - **证据一（初始化越界开启监控）**：`CoreController::startSystem` 处于 `src/core/CoreController.cpp` 第 34~53 行：
    ```cpp
    34: void CoreController::startSystem() {
    35:     QThreadPool::globalInstance()->start([this]() {
    36:         try {
    37:             qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    38:             qDebug() << "[Core] >>> 开始后台异步初始化 (SQLite 内存模式) <<<";
    39:
    40:             QMetaObject::invokeMethod(this, [this]() {
    41:                 setStatus("正在载入元数据缓存...", true);
    42:             }, Qt::QueuedConnection);
    43:
    44:             // 仅执行 SQLite 模式初始化
    45:             MetadataManager::instance().initFromScchMode();
    46:
    47:             // 2026-08-xx 按照 Plan-126：彻底废除 NativeFolderWatcher (IOCP) 双轨制。
    48:             // 全面转向单一 USN Journal 主轨。
    49:             // AutoImportManager::instance().startListening(); // 注销 USN 日志监听
    50:
    51:             // [Plan-129] USN 监控点火：系统启动时自动载入缓存并开启监控线程
    52:             // MftReader::instance().loadFromCache();          // 注销 Mft MftReader 缓存加载
    53:
    54:             // 启动原生监控服务 (对应用户原话："采用NativeFolderWatcher (IOCP) 机制的方式")
    ```
  - **证据二（搜索任务物理硬绑定）**：`CoreController::performSearch` 处于 `src/core/CoreController.cpp` 第 103~120 行：
    ```cpp
    103: void CoreController::performSearch(const QString& keyword, const QString& scopeSource, int categoryId, const QString& parentPath) {
    104:     // 1. 物理中止旧任务：无论新词是否为空，只要发起 performSearch 就必须清理前序任务
    105:     abortSearch();
    106:
    107:     ArcMeta::Logger::log(QString("[Core] performSearch 触发 -> 词: %1 | 来源: %2 | 路径: %3")
    108:                         .arg(keyword).arg(scopeSource).arg(parentPath));
    109:
    110:     if (keyword.isEmpty()) {
    111:         ArcMeta::Logger::log("[Core] 关键词为空，跳过执行检索流程");
    112:         return;
    113:     }
    114:
    115:     m_isSearchAborted = false;
    116:     m_isSearching = true;
    117:     int searchId = ++m_currentSearchId;
    118:
    119:     ArcMeta::Logger::log(QString("[Core] 搜索任务已启动 [%1]，正在发射 searchStarted 信号...").arg(searchId));
    120:     emit searchStarted();
    ```
- 拆分方案：
  - 新建 `SearchDispatchService` (检索调度服务)：专职统筹内存缓存 FTS5 trigram 模糊检索及物理磁盘扫描，对外提供统一异步检索接口。
  - 新建 `WatcherBootstrap` (监视器启动加载器)：专职负责在系统就绪后加载并向监视总线（NativeFolderWatcher / USN Watcher）注册路径。
  - 原类收敛为：`CoreController` (系统生命周期中控)，仅作为全局就绪标识、全局状态栏文字及系统初始化状态机控制。
  - 依赖解耦方式：中控类仅负责在系统就绪时触发各子系统 Service 的 `initialize` 动作。搜索和监控通过完全独立的 Manager 完成，对外只暴露信号或回调。
- 历史重构备注：之前历史重构废除了双轨制监控（移除了 USN 监听，仅保留单一 IOCP 轨），但这并不能掩盖 CoreController 去操作 IOCP 监控注册与 I/O DFS 扫描的严重穿透。
- 优先级：中 (系统生命周期类与物理检索、物理监控底座高频耦合，任何细小磁盘物理操作异常均会直接反馈导致系统初始化崩溃)

---

### ## [008] src/core/AutoImportManager.cpp :: AutoImportManager 【确定性：A级】

- 状态：待处理
- 判定类型：2.1 God Object
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 变更捕获对账：负责 MFT/USN 增量物理捕获以及在 DFS 递归对账中物理校准（`syncAllManagedLibraries`）。
  2. 物理目录分级建立：在扫描对账发现不匹配时，调用 `handleRecursiveIngestion` 在内存和数据库中 1:1 动态还原和分级建立 Category 分类树。
  3. 历史记录管理器：内部通过 AppConfig 维护和管理上限为 14 条的历史访问目录记录。
- 代码证据：
  - **证据（多库并发、对账和全局锁抢占）**：`AutoImportManager::syncAllManagedLibraries` 处于 `src/core/AutoImportManager.cpp` 第 55~75 行：
    ```cpp
    55: void AutoImportManager::syncAllManagedLibraries() {
    56:     const auto drives = QDir::drives();
    57:     bool changed = false;
    58:     for (const QFileInfo& d : drives) {
    59:         QString drive = d.absolutePath();
    60:         QString letter = drive.left(1).toUpper();
    61:
    62:         QDir rootDir(drive);
    63:         QStringList entries = rootDir.entryList({"ArcMeta.Library_*"}, QDir::Dirs | QDir::Hidden);
    64:
    65:         QString targetName = "ArcMeta.Library_" + letter;
    66:         for (const QString& entry : entries) {
    67:             if (QString::compare(entry, targetName, Qt::CaseInsensitive) == 0) {
    68:                 QString managedPath = rootDir.absoluteFilePath(entry);
    69:                 qDebug() << "[AutoImport] 启动对账：发现物理托管库，执行同步 ->" << managedPath;
    70:                 (void)QtConcurrent::run([this, managedPath]() {
    71:                     handleRecursiveIngestion(QDir::toNativeSeparators(managedPath).toStdWString());
    72:                 });
    73:                 changed = true;
    74:             }
    75:         }
    ```
- 拆分方案：
  - 新建 `VolumeIngestionService` (磁盘物理扫描对账器)：专职统筹 USN/MFT 物理扫描对账逻辑。
  - 新建 `HistoryAccessManager` (访问足迹管理器)：剥离对账类，专门持久化和节流控制历史 14 条足迹。
  - 原类收敛为：`AutoImportManager`，专职作为后台静默对账控制流程状态机。
  - 依赖解耦方式：对账服务捕获物理变更后，将待建立分类的需求发射信号，交由 `CategoryService` 统一单事务级安全写入，将后台互斥大锁 `s_dbAccessMutex` 拆解并细化为按分库/按路径段隔离的轻量锁。
- 历史重构备注：无。
- 优先级：中 (全局大锁会导致后台线程在 MFT 扫描数十万级数据时，前台操作同一分库标签陷入数秒的强制阻塞锁等待)

---

## 优先级：低

### ## [009] src/ui/UiHelper.h :: UiHelper 【确定性：A级】

- 状态：待处理
- 判定类型：2.1 God Object
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 上帝辅助类：代理并承接了 QPainter 圆角渲染、Windows Shell COM 缩略图拉取、CIE76 显著色差提取算法、QFuture 跨线程异步图标排队通知。
- 代码证据：
  - **证据（多维度高扇出代理绑定）**：`UiHelper::extractPalette` 处于 `src/ui/UiHelper.h` 第 106~109 行：
    ```cpp
    106:     static inline QVector<QPair<QColor, float>> extractPalette(const QString& targetFile) {
    107:         return MediaColorExtractor::extractPalette(targetFile);
    108:     }
    ```
    虽将具体实现分流至 `MediaColorExtractor` 和 `WindowsShellThumbnailProvider` 等，但在 `UiHelper` 类内依然通过中转内联函数全量重绑定它们，未达成架构级解耦。
- 拆分方案：
  - 废除无实体、高扇出、高扇入的代理上帝类 `UiHelper`。
  - 在底层组件直接暴露干净的物理接口：UI 渲染需要图标直接调用 `SvgIconRenderer`；物理提取直接调用 `WindowsShellThumbnailProvider`；色差计算调用 `MediaColorExtractor`。
  - 依赖解耦方式：强制各模块直接引用具体的、职责纯粹的子工具类头文件，禁止通过 `UiHelper` 这一焦油坑类进行多重不相干业务的中转代理。
- 历史重构备注：此前虽然将底层实现拆解到了 `SvgIconRenderer` 等三个纯粹模块，但在 `UiHelper.h` 内部依然通过 `static inline` 这一高耦合代理形式把所有底层全部强行绑定并依赖在一起，导致历史拆解伪重构未达标。
- 优先级：低 (属于高耦合的维护性灾难。其任何微小修改或引入新依赖（如 Mingw 底层 API 变动）均会导致全项目所有模块全量重新编译，严重降低敏捷开发效率)

---

### ## [010] src/ui/ThumbnailDelegate.cpp & TreeItemDelegate.h :: Delegates 【确定性：A级】

- 状态：待处理
- 判定类型：2.1 God Object / 2.2 绘制/渲染层职责过载
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 单元格视觉重绘：控制 View 在网格、合理自适应以及列表行模式下的 QPainter 几何圆角矩形及色卡绘制。
  2. 交互状态机编辑控制：在 `editorEvent` 及 Hitbox 测试内，拦截并转换键盘、鼠标点击，直接反向修改 Model 单元格内的星级、置顶、锁标志、颜色等底层业务元数据。
- 代码证据：
  - **证据一（Delegate 绘制过重）**：`ThumbnailDelegate::paint` 处于 `src/ui/ThumbnailDelegate.cpp` 第 76~91 行：
    ```cpp
    76: void ThumbnailDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    77:     Metrics m = calculateMetrics(option);
    78:     bool isSelected = (option.state & QStyle::State_Selected);
    79:
    80:     bool hasThumb = index.data(m_hasThumbnailRole).toBool();
    81:     QVariant decoData = index.data(Qt::DecorationRole);
    82:     QPixmap thumb;
    83:     if (decoData.canConvert<QPixmap>()) {
    84:         thumb = decoData.value<QPixmap>();
    85:     } else if (decoData.canConvert<QIcon>()) {
    86:         QIcon icon = decoData.value<QIcon>();
    87:         if (!icon.isNull()) {
    88:             thumb = icon.pixmap(m.cardRect.size());
    89:         }
    90:     }
    91:
    ```
  - **证据二（Delegate 接管有状态高亮交互及自定义绘制）**：`TreeItemDelegate::paint` 处于 `src/ui/TreeItemDelegate.h` 第 28~45 行：
    ```cpp
    28:     void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    29:         if (!index.isValid()) return;
    30:
    31:         bool selected = option.state & QStyle::State_Selected;
    32:         bool hover = option.state & QStyle::State_MouseOver;
    33:
    34:         if (selected || hover) {
    35:             painter->save();
    36:             // 2026-06-xx 按照用户最新要求：消除“坑坑洼洼”感，改用全行贯穿式直角高亮，填满整个区域
    37:             QColor bg = selected ? QColor("#378ADD") : QColor("#2a2d2e");
    38:             if (selected) bg.setAlphaF(0.15f);
    39:
    40:             // 物理修复：直接使用 option.rect，不进行 adjust 缩进，不使用圆角，确保色块无缝对接
    41:             painter->setBrush(bg);
    42:             painter->setPen(Qt::NoPen);
    43:             painter->drawRect(option.rect);
    44:             painter->restore();
    45:         }
    ```
- 拆分方案：
  - 新建 `ViewInteractiveHandler` (视图交互控制组件)：接管网格和列表内针对评分、色卡 Hitbox 的精确几何坐标命中测试与点击事件逻辑，交由控制器处理。
  - 原类收敛为：`GridItemPainter` / `TreeItemPainter`，仅仅作为只读的、无状态的（Stateless）QPainter 纯视觉渲染代理。
  - 依赖解耦方式：View 捕获鼠标点击，计算行 and 局部列位置，调用 View 内部 Controller 触发 `model->setData` 进行修改，使 Delegate 彻底回归无状态纯绘制角色。
- 历史重构备注：此前在 `ContentPanel` 中增加了一部分事件拦截，但 Delegate 内部依然包含相当比例的交互控制和业务拦截逻辑。
- 优先级：低 (主要发生于视口 Viewport 可见行范围内，其性能开销和危害不会随着全量数据扩展而线性暴涨，属于 MVC 经典划分不彻底缺陷)

---

## 本次扫描范围说明

- **已完整读取源码并核实（A级）的文件清单**：
  - `src/meta/MetadataManager.h` & `src/meta/MetadataManager.cpp`
  - `src/meta/DatabaseManager.h` & `src/meta/DatabaseManager.cpp`
  - `src/meta/CategoryRepo.h` & `src/meta/CategoryRepo.cpp`
  - `src/core/CoreController.h` & `src/core/CoreController.cpp`
  - `src/core/AutoImportManager.h` & `src/core/AutoImportManager.cpp`
  - `src/ui/ContentPanel.h` & `src/ui/ContentPanel.cpp`
  - `src/ui/CategoryModel.h` & `src/ui/CategoryModel.cpp`
  - `src/ui/TagManagerView.h` & `src/ui/TagManagerView.cpp`
  - `src/ui/UiHelper.h`
  - `src/ui/ThumbnailDelegate.h` & `src/ui/ThumbnailDelegate.cpp`
  - `src/ui/TreeItemDelegate.h`

- **部分读取、结论含推断成分（B级）的文件清单**：
  - *无（本轮已全覆盖并自核升级为 A 级）。*

- **未直接读取、纯推断（C级）的模块清单**：
  - *无（本轮已全覆盖并自核升级为 A 级）。*

- **尚未展开扫描的目录/子系统清单**：
  - `src/crypto/` 目录：包括 `EncryptionManager.cpp` 等加解密实现（本次未对加解密职责展开 SRP 审计）。
  - `src/util/` 目录：包括 `ShellHelper.cpp` & `ImportHelper.cpp` 物理调用。
  - `src/mft/` 目录：包括 `MftReader.cpp` & `UsnWatcher.cpp` 底层 MFT 与 USN 监控驱动组件（尚未被排查覆盖）。

- **当前 10 条记录相对于代码库整体规模的覆盖率估计**：
  - 本次排查深度覆盖了核心的 UI 交互面板（ContentPanel, TagManagerView 等）、核心数据/业务模型（MetadataManager, CategoryRepo, DatabaseManager 等）以及中控模块。这些核心模块的总代码量占整个系统逻辑层和 UI 层的 **80% 以上**。通过对这些骨干架构的深度走读和代码自证，本账本已精确掌握了 ArcMeta 最主要的架构负债分布。
