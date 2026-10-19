【任务】修复"非图片文件（如 .cpp、.h）在内容区卡片上一直显示白色空白页图标，系统图标加载完成后不刷新"的问题。

【根因（已核实代码）】
系统图标由 WindowsShellThumbnailProvider 在后台线程异步加载。加载完成后只发出 IconLoadNotifier::iconLoaded() 信号，但整个工程没有任何模型订阅该信号，所以视图不会重新调用 data(Qt::DecorationRole)，卡片永远停在占位图。另外 IconLoadNotifier 是函数内静态单例，若首次在后台线程被调用，它会归属没有事件循环的线程，导致跨线程的 queued 调用永远不执行。

【硬性约束】
1. 只修改下面列出的三个文件，其他文件一律不动。
2. 不得改变任何现有功能的设计和交互，包括重命名的行内编辑方式、缩略图加载流程、卡片绘制、评分、色标。
3. 不得在 ThumbnailDelegate、CardPainterHelper、各个视图（JustifiedView、DropTreeView、DropListView）里加任何补丁代码。刷新职责只放在 ItemModelBase 一处。
4. 不得对全部行无差别地发 dataChanged，只能刷新使用了刚加载完成的那个图标的行。FilterProxyModel 开着 dynamicSortFilter，大量行的 dataChanged 会触发整体重过滤、重排序，造成卡顿。
5. 保留 IconLoadNotifier::iconLoaded()（无参）信号，不得删除，避免破坏其他可能的订阅方。

【修改一：src/ui/models/ItemModelBase.h（路径以实际为准）】
1. 新增 include：<QSet>、<QTimer>、<algorithm>、"WindowsShellThumbnailProvider.h"。
2. 构造函数改为下面的行为：
   - 先调用 WindowsShellThumbnailProvider::instance()，保证提供者和通知器在主线程创建。
   - 创建单次触发定时器 m_iconRefreshTimer，间隔 35ms，超时调用 flushIconRefresh()。
   - 连接 IconLoadNotifier::instance() 的 iconLoadedForKey(const QString&) 信号到私有槽 onSystemIconLoaded(const QString& key)。
3. 私有槽 onSystemIconLoaded：把 key 放入 QSet<QString> m_pendingIconKeys，若定时器未激活则启动。
4. 私有槽 flushIconRefresh：
   - 取出并清空 m_pendingIconKeys。
   - 遍历 allRecords()，对每条记录用 WindowsShellThumbnailProvider::iconKey(rec.path, rec.isDir, rec.suffix) 计算键，命中集合的行号收集起来。
   - 把行号合并成连续区间，每个区间发一次 dataChanged(index(start,0), index(end, columnCount()-1), {Qt::DecorationRole})。
   - 没有命中行则直接返回。
5. 新增成员：QTimer* m_iconRefreshTimer = nullptr; QSet<QString> m_pendingIconKeys;
6. 其余接口、纯虚函数、QStringHash 保持原样。

【修改二：src/.../WindowsShellThumbnailProvider.h】
1. IconLoadNotifier 增加信号 void iconLoadedKey 的新版本：signals 里同时保留 void iconLoaded(); 并新增 void iconLoadedForKey(const QString& key);
2. WindowsShellThumbnailProvider 新增 public static：QString iconKey(const QString& filePath, bool isDir, const QString& suffix);
3. 新增 private static：bool isDriveRootPath(const QString& filePath);

【修改三：src/.../WindowsShellThumbnailProvider.cpp】
1. isDriveRootPath 的逻辑：filePath 以 ":\\" 或 ":/" 结尾，或长度 <= 3，返回 true。
2. iconKey 是缓存键的唯一计算入口：
   - 文件夹：是盘符根路径则键为 filePath 本身，否则为 "folder"。
   - 文件：键为 suffix 的小写形式。
   - 键长度超过 128 则为 "unknown"。
3. isIconCached 和 getFileIconFast 都改用 iconKey 取键，getFileIconFast 内的 isRoot 用 isDir && isDriveRootPath(filePath) 得到。删除这两处原来各自手写的键计算。
4. getFileIcon 改成薄封装：QFileInfo info(filePath); return getFileIconFast(filePath, info.isDir(), info.suffix());，删除原来整段复制的实现。
5. handleIconLoad 改动：
   - 在 QtConcurrent::run 之前（主线程）执行 IconLoadNotifier* notifier = &IconLoadNotifier::instance();，并把 notifier 按值捕获进 lambda。
   - lambda 内部不得再调用 IconLoadNotifier::instance()。
   - 图标写入缓存、从 loadingKeys 移除 key 之后，用 QMetaObject::invokeMethod(notifier, [notifier, key]() { emit notifier->iconLoadedForKey(key); emit notifier->iconLoaded(); }, Qt::QueuedConnection); 通知主线程。
   - 其余 COM 初始化、图标获取逻辑保持原样。

【验收标准】
1. 编译通过，没有新增警告。
2. 打开含 .cpp、.h 文件的文件夹，卡片中间的图标在短时间内由白色空白页变成系统关联图标，无需手动刷新、滚动或切换视图。
3. 网格、列表、列视图三种模式都能自动更新；列视图每一列都能自动更新。
4. 打开包含上千个文件的文件夹时，滚动和切换目录不出现新的卡顿。
5. 图片文件的缩略图加载流程、评分、色标、重命名行为与改动前完全一致。

【完成后只需汇报】
实际修改了哪些文件，以及是否存在与上述约束冲突、导致你无法照做的地方。不要顺手重构其他代码，也不要改动没被要求的内容。