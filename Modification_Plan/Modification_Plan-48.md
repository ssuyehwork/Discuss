# 文件夹高级属性离散写盘双向自愈重构方案 —— Modification_Plan-48.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
本方案专门覆盖并治愈磁盘模式下，`AmMetaJson` 机制在处理文件夹时的持久化设计缺陷（其它垃圾回收站重构、编辑器定位等问题已被独立记录在 `Modification_Plan-47.md` 中，本方案不作任何夹带）。
在现有物理磁盘导航模式下，`.ArcMeta.json` 隐藏文件具有以下重要特征：
1. **不执行递归**：它只可以对某个具体文件夹里的文件夹和文件进行单级持久化，绝不向下执行递归（对应用户原话：“只可以对某个文件夹里的文件夹和文件持久化... 只记录‘G:\测试’文件夹里的文件夹和文件，不执行递归 对不”）。
2. **读写时空发生脱节**：`AmMetaJson.cpp` 默认对文件夹自身不能同步将高级元数据写进其自身目录配置中（对应用户原话：“它是不是只对文件起作用，对文件夹完全不起作用不持久化 对不”）。
这导致：当用户在当前目录下对一个物理“子文件夹”设色、标星、写备注时，该信息仅被作为 `ItemMeta` 保存到了父级目录的 `.ArcMeta.json` 中。一旦用户双击进入该子目录（加载自身作为根目录），系统会读取该子目录自己内藏的 `.ArcMeta.json`，由于其自身的 `folder` 节点从未被写入，元数据直接变为全空、发生消失，引起严重的数据流脱节。此外，在重命名/移动文件夹时，其内部配置未发生物理移动。

本方案致力于通过“双向写盘同步”与“物理转移自愈”彻底根除该离散缓存硬伤。

## 2. 问题定位
1. **读写位置不一致**：对子文件夹操作时仅作为 items 条目写在父级配置中，而没有写进子文件夹自己目录下的 `folder` 节点中。双击进入该文件夹后其颜色、备注直接丢失。需要让 `MetadataManager::saveToDiskModeJson` 执行双向同步落盘。
2. **物理重命名时配置断裂**：物理重命名/移动文件夹时，隐藏的 `.ArcMeta.json` 留在原地没有得到迁移，导致重命名后缓存全部失效。需要重构并激活 `AmMetaJson::migrateFolderCache` 实现原子迁移。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 只对当前目录文件和文件夹持久化，不递归（对应用户原话：“只可以对某个文件夹里的文件夹和文件持久化... 只记录‘G:\测试’文件夹里的文件夹和文件，不执行递归 对不”） | 保持 `.ArcMeta.json` 单级、不执行递归的单文件高性能读写设计不变 | ✅ 一致 |
| 2    | 对文件夹持久化完全不起作用，对子目录脱节（对应用户原话：“它是不是只对文件起作用，对文件夹完全不起作用不持久化 对不”） | 在 `saveToDiskModeJson` 设色标星等时执行双向同步持久化（父级 items 节点 + 子级 folder 节点同时落盘），彻底对齐一致性 | ✅ 一致 |

## 4. 详细解决方案

本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的 Git merge diff 代码块进行物理替换，不得做任何自由发挥或脑补改动。

### 4.1 治愈文件夹设色/标星进入子目录后数据全空消失 Bug (`MetadataManager.cpp` 双向持久化同步)

在 `src/meta/MetadataManager.cpp` 中：
```
<<<<<<< SEARCH
void MetadataManager::saveToDiskModeJson(const std::wstring& nPath, std::function<void(ItemMeta&)> updater) {
    QFileInfo info(QString::fromStdWString(nPath));
    std::wstring folderPath = info.absolutePath().toStdWString();
    std::wstring fileName = info.fileName().toStdWString();

    AmMetaJson jsonCache(folderPath);
    jsonCache.load();
    ItemMeta& meta = jsonCache.items()[fileName];
    meta.type = info.isDir() ? L"folder" : L"file";
    updater(meta);
    jsonCache.save(); // 物理落盘写进 ArcMeta.cache/*.json，零 SQLite 污染！
}
=======
void MetadataManager::saveToDiskModeJson(const std::wstring& nPath, std::function<void(ItemMeta&)> updater) {
    QFileInfo info(QString::fromStdWString(nPath));
    std::wstring folderPath = info.absolutePath().toStdWString();
    std::wstring fileName = info.fileName().toStdWString();

    // 1. 同步写入到父级目录的 .ArcMeta.json 中（保证父级视图渲染出子目录的颜色和星级）
    AmMetaJson parentJson(folderPath);
    parentJson.load();
    ItemMeta& meta = parentJson.items()[fileName];
    meta.type = info.isDir() ? L"folder" : L"file";
    updater(meta);
    parentJson.save();

    // 2. 极致治愈：如果是文件夹，将其对应的高级属性同步写入该文件夹自身的 .ArcMeta.json 中的 folder 节点！
    // 这样当双击进入该子目录作为主视图时，子目录加载自己作为根，颜色和星级 100% 对等保留，绝对不会丢失！
    if (info.isDir()) {
        std::wstring selfPath = nPath;
        AmMetaJson selfJson(selfPath);
        selfJson.load();

        FolderMeta& fMeta = selfJson.folder();
        ItemMeta dummyItem; // 用 dummyItem 桥接 ItemMeta 与 FolderMeta 字段更新
        dummyItem.rating = fMeta.rating;
        dummyItem.color = fMeta.color;
        dummyItem.pinned = fMeta.pinned;
        dummyItem.note = fMeta.note;
        dummyItem.url = fMeta.url;
        dummyItem.encrypted = fMeta.encrypted;
        dummyItem.folderId = fMeta.folderId;
        dummyItem.tags = fMeta.tags;
        dummyItem.palettes = fMeta.palettes;

        updater(dummyItem); // 触发业务更新器

        fMeta.rating = dummyItem.rating;
        fMeta.color = dummyItem.color;
        fMeta.pinned = dummyItem.pinned;
        fMeta.note = dummyItem.note;
        fMeta.url = dummyItem.url;
        fMeta.encrypted = dummyItem.encrypted;
        fMeta.folderId = dummyItem.folderId;
        fMeta.tags = dummyItem.tags;
        fMeta.palettes = dummyItem.palettes;

        selfJson.save(); // 双向原子同步写入落盘！
    }
}
>>>>>>> REPLACE
```

### 4.2 治愈物理重命名时离散配置文件遗留 Bug (`AmMetaJson.cpp` 物理迁移自愈)

在 `src/meta/AmMetaJson.cpp` 中，实现物理重命名移动文件夹时内藏隐藏配置文件的转移：
```
<<<<<<< SEARCH
bool AmMetaJson::migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath) {
    Q_UNUSED(oldFolderPath);
    Q_UNUSED(newFolderPath);
    return true;
}
=======
bool AmMetaJson::migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath) {
    if (oldFolderPath == newFolderPath) return true;

    // 物理自愈：当发生物理文件夹重命名时，自动原子迁移其目录内部隐藏的 .ArcMeta.json 配置
    QString oldMetaFile = oldFolderPath + "/.ArcMeta.json";
    QString newMetaFile = newFolderPath + "/.ArcMeta.json";

    if (QFile::exists(oldMetaFile)) {
        // 创建新物理目录（如果不存在）
        QDir().mkpath(newFolderPath);

        if (QFile::exists(newMetaFile)) {
            QFile::remove(newMetaFile);
        }
        if (QFile::copy(oldMetaFile, newMetaFile)) {
            QFile::remove(oldMetaFile);

            // 赋予 Windows 环境隐藏文件属性
            SetFileAttributesW(newMetaFile.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
            return true;
        }
    }
    return true;
}
>>>>>>> REPLACE
```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] `src/meta/MetadataManager.cpp` （重构 `saveToDiskModeJson` 保证子文件夹元数据双向写盘同步落盘）
- [ ] `src/meta/AmMetaJson.cpp` （实现 `migrateFolderCache` 子目录隐藏配置文件物理原子转移）

**明确禁止越界修改的范围：**
- [ ] 父级及其它不涉及 `AmMetaJson` 持久化的其它重构和物理文件读写。

## 6. 实现准则与预警【核心】
1. **重入安全与 Windows 文件系统兼容**：隐藏属性写入必须通过 Windows 原生 API `SetFileAttributesW` 并传入 `FILE_ATTRIBUTE_HIDDEN`。
2. **零编译报错**：不引入不必要的外部头文件，确保变量桥接和命名空间绝对一致。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 双轨路由物理隔离 | 磁盘模式产生的任何设色、星级、加备注和打标等写操作，100% 绝对禁止写入 SQLite 本地数据库，必须独占读写元数据缓存 `AmMetaJson`。 | ✅ 符合（完全保留对 `AmMetaJson` 单级配置读写，只是双向同步写盘并迁移，物理磁盘模式依然 100% 保持对本地 sqlite3 零写库、零污染，完美对齐双轨隔离原则） |

## 8. 待确认事项（可选）
*无*
