# 重启全量对账重复扫描与启动白边闪烁缺陷排查及重构方案 —— Modification_Plan-118.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
本方案承接自 `Development_Plan.md` 中 [2026-07-28] “解决每次启动主程序时重复对账与无谓重新扫描元数据及主程序启动闪烁白色边框缺陷” 任务。
用户反馈，当被监控的文件夹没有新增任何文件或文件夹，并且已经将元数据储存到了磁盘里的数据库时，每次重启主程序，系统仍然会重复触发全量元数据的二次扫描、全量对账并触发冗余的多媒体特征（如调色板、宽高尺寸等）提取流水线。
同时，用户指出，在启动主程序之后，主界面会闪烁显示白色边框，随后又消失。这并不符合暗色主题的高雅设计，不应该出现该瑕疵。
本方案将对上述两项现象的底层逻辑架构与界面重绘时序进行深度静态排查，定位其根本原因，并给出极致性能的自愈过滤重构设计，彻底根治重启冗余提取开销与启动时的白色瞬时闪烁现象。

## 2. 问题定位

### 2.1 重复扫描与高级特征重新提取的定位
经过对 `src/meta/CategoryRepo.cpp`、`src/meta/MetadataManager.cpp` 以及后台特征提取管线的链路跟踪，定位到以下关键问题：
1. **`CategoryRepo::syncPhysicalDirectoryCascade` 无条件收集物理文件并注册：** 递归遍历物理树时，代码逻辑会无条件将所有物理文件塞入 `collectedFilesToProcess` 中，并在第三阶段无条件调用 `MetadataManager::registerItemsAsync`。
2. **`CategoryRepo::addItemToCategory` 无条件覆写 `category_items`：** 每次递归时均无条件执行 `INSERT OR REPLACE`，覆盖关联表记录。不仅将 `added_at` 字段强行重置为当前的毫秒级时间戳（打乱了用户原本的历史时间排序），还会高频标记内存数据库 Dirty，从而引发大量的落盘备份 I/O。
3. **`MetadataManager::registerItemsAsync` 缺失批量物理指纹准入校验：** 相比单路径注册方法 `registerItem` 的指纹双重校验，异步批量注册方法 `registerItemsAsync` 直接绕过了这一安全屏障，在 QtConcurrent 线程中无条件通过 `updateIngestionStatus(nPath, 0)` 将文件的 `ingestion_status` 重置为 0，然后强制塞入 `MediaExtractorPipeline` 进行多媒体重解析，带来高开销的 CPU/GPU 资源浪费。

### 2.2 启动闪烁白色边框的定位
经过对 `src/ui/NavPanel.cpp`（导航面板）以及 `src/ui/MainWindow.cpp` 窗口绘制和 QSS 应用生命周期的跟踪，定位到闪烁白色边框的根因：
1. **异步填充与 QSS 级联生效的延迟窗期：** 导航面板 `NavPanel` 的树模型、收藏夹以及 `QSplitter` 的延迟初始化均通过 `QTimer::singleShot` 异步触发。
2. **QSplitter 及其子组件缺乏默认暗色底色样式：** 在主窗口的全局 QSS 样式表（`style.qss`）加载并级联应用（Cascade）完成前，`NavPanel` 及内部的 `m_splitter` 未设定显式背景颜色。此时 Qt 默认以操作系统的窗体背景底色（在普通浅色模式系统下为浅灰色/白色）绘制组件的外边框及对齐间隙（Margin/Padding）。
3. **未填充空档期的瞬时重排：** 延迟加载导致的 `QSplitter` 重排（Relayout）以及数据未填充前的空白空隙，让底层的浅色底色暴露出来，形成一瞬间的“白色边框/白色闪烁”视觉抖动，数据填充且全局样式表覆盖后即消失。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 被监控的文件夹没有新增任何文件或文件夹，元数据也已经持久化至磁盘数据库，但每次重启主程序时都会重新对账并触发全量文件元数据扫描与多媒体特征提取。（对应用户原话） | 在 `CategoryRepo::syncPhysicalDirectoryCascade` 和 `addItemToCategory` 的对账链路中增加存在性检索与过滤。 | ✅ |
| 2    | 在启动主程序进行全量对账时，跳过已成功导入且指纹/修改时间未发生变化的完备文件，杜绝每次重启时的冗余扫描和特征提取，保护物理磁盘并大幅减少 CPU 占用。（对应用户原话） | 在 `MetadataManager::registerItemsAsync` 与 `addItemToCategory` 中加装物理指纹与特征完备性双重比对安全屏障。 | ✅ |
| 3    | 在 CategoryRepo::addItemToCategory 中增加重复关联预检，防止每次重启时 category_items 被无谓 INSERT OR REPLACE 覆盖并导致 added_at 乱序和数据库频繁标记 Dirty。（对应用户原话） | 重构 `CategoryRepo::addItemToCategory`，执行 SQL 查询检测，若已关联则提前拦截直接返回。 | ✅ |
| 4    | 在 MetadataManager::registerItemsAsync 中增加文件物理指纹与高级特征双重准入检查，若文件已完备且大小/修改时间一致，直接跳过登记和提取。（对应用户原话） | 重构 `MetadataManager::registerItemsAsync`，在压入多媒体提取队列前对每一项执行物理指纹（`fileSize`/`mtime`）和多媒体完整性核对。 | ✅ |
| 5    | 启动主程序之后会显示这白色边框，然后又消失，它本不该出现的，排查一下原因，是不是样式导致的（对应用户原话） | 排查定位出 QSS 延迟生效期 QSplitter 未明确设置暗色背景，而在空档期露出系统默认浅色底层。 | ✅ |
| 6    | 请给出相应的修改方案（对应用户原话） | 在 `NavPanel.cpp` 的构造函数和 `m_splitter` 初始化部分中，显式、静态地设置与主题匹配的暗色底色样式（`#1E1E1E`）。 | ✅ |

---

## 4. 详细解决方案

### 4.1 重构 `CategoryRepo::addItemToCategory` 引入重复关联快速拦截（对应用户原话："在 CategoryRepo::addItemToCategory 中增加重复关联预检"）
在写入操作前执行一次预检。只有不存在时才执行 `INSERT OR REPLACE`：
```cpp
    // 检查是否已经存在该关联以避免无谓的 INSERT OR REPLACE 写入与 Dirty 标记
    bool exists = false;
    const char* checkSql = "SELECT 1 FROM category_items WHERE category_id = ? AND file_id = ?";
    sqlite3_stmt* checkStmt = nullptr;
    if (sqlite3_prepare_v2(memDb, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(checkStmt, 1, categoryId);
        sqlite3_bind_text(checkStmt, 2, fileId128.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(checkStmt) == SQLITE_ROW) {
            exists = true;
        }
        sqlite3_finalize(checkStmt);
    }
    if (exists) {
        return true; // 已存在关联，安全跳过，防止重写 added_at 丢失用户历史排序
    }
```

### 4.2 重构 `MetadataManager::registerItemsAsync` 引入物理指纹与特征完整性核对（对应用户原话："在 MetadataManager::registerItemsAsync 中增加文件物理指纹与高级特征双重准入检查"）
在后台线程遍历 paths 列表时，提取物理文件指纹并与内存缓存中的 `RuntimeMeta` 对比，剔除未发生改变且元数据完整合规的完备项，杜绝重复的多媒体高级特征提取：
```cpp
void MetadataManager::registerItemsAsync(const QStringList& paths, bool authorized) {
    if (paths.isEmpty()) return;
    (void)authorized;

    (void)QtConcurrent::run([this, paths]() {
        std::vector<std::wstring> stdPaths;
        for (const auto& qp : paths) {
            std::wstring nPath = normalizePath(qp.toStdWString());

            // 物理指纹与高级多媒体特征双重比对拦截
            std::string pFid;
            long long pSize = 0, pMtime = 0;
            bool skip = false;
            if (fetchWinApiMetadataDirect(nPath, pFid, nullptr, &pSize, nullptr, nullptr, &pMtime, nullptr)) {
                std::shared_lock<std::shared_mutex> lock(m_mutex);
                auto it = m_cache.find(nPath);
                if (it != m_cache.end()) {
                    bool metadataValid = true;
                    QFileInfo info(QString::fromStdWString(nPath));
                    if (info.isFile() && MediaColorExtractor::isGraphicsFile(info.suffix().toLower())) {
                        if (it->second.width <= 0 || it->second.height <= 0 || it->second.autoColor.empty()) {
                            metadataValid = false;
                        }
                    }
                    if (it->second.ingestionStatus == 1 && it->second.fileSize == pSize && it->second.mtime == pMtime && metadataValid) {
                        skip = true; // 完全完备，直接跳过多媒体特征重新提取
                    }
                }
            }

            if (!skip) {
                ensureActivated(nPath);
                updateIngestionStatus(nPath, 0);
                stdPaths.push_back(nPath);
            }
        }
        if (!stdPaths.empty()) {
            MediaExtractorPipeline::instance().enqueueBatch(stdPaths);
        }
    });
}
```

### 4.3 修复主程序启动瞬时闪烁白色边框现象（对应用户原话："启动主程序之后会显示这白色边框... 排查一下原因，是不是样式导致的"、"请给出相应的修改方案"）
在 `NavPanel` 构造函数和 `m_splitter` 初始化时显式强制声明暗色背景，防止未载入 `style.qss` 期间和异步数据空档期透露出操作系统的默认浅色底层：
1. **NavPanel 构造函数：**
   ```cpp
   // 核心修正：设置明确的背景色与前景色，防止 QSS 异步加载时露出系统默认浅色背景
   setStyleSheet("NavPanel { background-color: #1E1E1E; color: #EEEEEE; }");
   ```
2. **QSplitter 初始化：**
   ```cpp
   m_splitter = new QSplitter(Qt::Vertical, this);
   m_splitter->setHandleWidth(1);
   m_splitter->setStyleSheet("QSplitter { background-color: #1E1E1E; border: none; } QSplitter::handle { background: #333333; }");
   ```

---

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/meta/CategoryRepo.cpp` （具体为 `addItemToCategory` 新增重复关联快速预检）
- [ ] 模块/文件：`src/meta/MetadataManager.cpp` （具体为 `registerItemsAsync` 批量多媒体物理指纹及特征校验）
- [ ] 模块/文件：`src/ui/NavPanel.cpp` （具体为 `NavPanel` 构造函数及 `m_splitter` 样式表强化）

**明确禁止越界修改的范围：**
- [ ] 磁盘 MFT 解析与监控模块 `src/core/` 目录 —— 不修改
- [ ] 数据库驱动管理层 `src/meta/DatabaseManager.cpp` —— 不修改

---

## 6. 实现准则与预警【核心】

1. **头文件与标识符依赖核对**：批量过滤中使用到了 `QFileInfo` 和 `MediaColorExtractor::isGraphicsFile`，需在 `MetadataManager.cpp` 顶部确保引入了对应的头文件。
2. **多线程安全性**：在 `registerItemsAsync` 执行校验时，必须先持有 `m_mutex` 读锁（`shared_lock`）来核验 RuntimeMeta，在进行 `ensureActivated` 与 `updateIngestionStatus` 等写操作时安全释放读锁并走原函数内部的写锁，防止死锁或跨线程竞态。
3. **开箱即用性**：在 `NavPanel` 本地强制注入 `#1E1E1E` 完美底色不会与主窗口全局 `style.qss` 冲突，因为全局 QSS 加载后会通过 `setObjectName("ListContainer")` 级联应用高优先级样式，从而做到完全平滑无闪烁切换。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| **UI 异步加载与防闪烁规范** | 在数据就绪前的空窗期内避免出现“白屏/黑屏/浅色”视觉抖动，保留原本样式并对数据行进行原子级毫秒替换。 | ✅ 符合。本方案通过在 QSplitter 和 QWidget 构造时静态绑定暗色样式（#1E1E1E），在延迟初始化的异步填充空窗期内彻底屏蔽了浅色操作系统的底色虚影。 |
| **元数据管理与对账规范** | 所有高级元数据在物理指纹（fileSize/mtime）和多媒体完整性不相符时，才能通过提取管线异步提取。 | ✅ 符合。本方案完美落实了物理指纹与高级特征双重准入检查，跳过完备无修改项。 |

---

## 8. 待确认事项（可选）
- **无**。
