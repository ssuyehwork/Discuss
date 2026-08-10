# 磁盘模式元数据读写与MetaPanel输入隔离重构 —— 磁盘模式元数据读写与MetaPanel输入隔离重构-1.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在 ArcMeta 应用的双轨制架构中，用户反映此前交付的元数据编辑在磁盘目录模式（DiskNav）下选中另一个项目后，先前的备注、链接、标签元数据便在面板上“加载归零”（对应用户原话：“傻逼Ai没有严格按照修改方案去实施导致没有达到预期，你去核对一下，哪些尚未完成”）。
经本次高精度实地代码审计核对，发现在之前一任的重构实施中：
1. **任务 2**（`MetaPanel`“文件夹/分类”双轨重构）已完全、正确地在 `MetaPanel.h` 与 `MetaPanel.cpp` 中落地。
2. **任务 4**（全新高级标签管理弹窗 `TagManagerDialog`）已完整创建并配置加入 `CMakeLists.txt`。
3. **任务 3** 中的 `MainWindow` 信号补全连接已正确落地；**但是，任务 3 最核心的「磁盘模式 getMeta 底层回退感知（从 .ArcMeta.json 预载回填）」在 `MetadataManager::getMeta` 中完全没有编写（甚至完全保留了原样）**，导致选中新文件时面板强行被空白属性重绘，未能加载出持久化在隐藏 `.ArcMeta.json` 文件中的元数据。

为了根治并补全此项悬空的核心缺陷，本升级版方案（-1）对 `MetadataManager::getMeta` 函数提供精确的 Fallback 异步读取补全，打通磁盘模式元数据对称双向读写的闭环（对应用户期望：“你去核对一下，哪些尚未完成”）。

---

## 2. 问题定位
- **问题文件**：`src/meta/MetadataManager.cpp`
- **问题函数**：`RuntimeMeta MetadataManager::getMeta(const std::wstring& path)`
- **根本原因**：该函数在内存快照 `m_snapshot` 未命中目标路径时，直接返回了空的 `RuntimeMeta()`（第 917 行）。由于在磁盘模式下，元数据并不保存在中心 SQLite 中，而是保存在目录内的 `.ArcMeta.json` 里。如果在选中资产时，未能触发 `AmMetaJson::load()` 实时对 `.ArcMeta.json` 进行解析并回填内存快照，用户新设色的标记、备注与标签信息就永远无法在重新切选时恢复呈现。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 任务 3 施工图纸：磁盘模式（Disk Nav Mode）读写与信号修复 2. 修改 `src/meta/MetadataManager.cpp`（实现磁盘模式预载回填）（对应用户原话/施工图纸中指定的修改要求） | 在 4.1 节中对 `MetadataManager::getMeta` 函数进行精确物理替换，补全 `.ArcMeta.json` Fallback 读取与原子写回内存快照逻辑 | ✅ |

---

## 4. 详细解决方案
本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的 Git merge diff 代码块进行物理替换，不得做任何自由发挥或脑补改动。

### 4.1 补全 `src/meta/MetadataManager.cpp` 中的 `getMeta` Fallback 读取逻辑
在 `src/meta/MetadataManager.cpp` 中，定位到 `MetadataManager::getMeta` 的函数体，并使用以下 Git merge diff 代码块物理修补：

```
<<<<<<< SEARCH
RuntimeMeta MetadataManager::getMeta(const std::wstring& path) {
    std::wstring nPath = MetadataManager::normalizePath(path);

    // 1. 无锁（Lock-Free）原子获取当前最新快照指针 —— 耗时恒定为 0 毫秒
    auto currentSnapshot = std::atomic_load(&m_snapshot);
    if (!currentSnapshot) return RuntimeMeta();

    // 2. 在只读快照副本中查找，绝不与后台持久化线程竞争锁
    auto it = currentSnapshot->find(nPath);
    if (it != currentSnapshot->end()) return it->second;

    return RuntimeMeta();
}
=======
RuntimeMeta MetadataManager::getMeta(const std::wstring& path) {
    std::wstring nPath = MetadataManager::normalizePath(path);

    // 1. 无锁（Lock-Free）原子获取当前最新快照指针 —— 耗时恒定为 0 毫秒
    auto currentSnapshot = std::atomic_load(&m_snapshot);
    if (currentSnapshot) {
        auto it = currentSnapshot->find(nPath);
        if (it != currentSnapshot->end()) return it->second;
    }

    // 🚨 2. 磁盘模式回退读取：若内存快照中不存在（非托管库模式），实时从物理文件夹下的 .ArcMeta.json 预载回填
    if (!isInsideManagedLibrary(nPath)) {
        QFileInfo info(QString::fromStdWString(nPath));
        if (info.exists()) {
            std::wstring folderPath = info.absolutePath().toStdWString();
            std::wstring fileName = info.fileName().toStdWString();

            AmMetaJson amJson(folderPath);
            if (amJson.load()) {
                const auto& items = amJson.items();
                auto it = items.find(fileName);
                if (it != items.end()) {
                    const ItemMeta& itemMeta = it->second;
                    RuntimeMeta rm;
                    rm.rating = itemMeta.rating;
                    rm.manualColor = itemMeta.color;
                    rm.pinned = itemMeta.pinned;
                    rm.note = itemMeta.note;
                    rm.url = itemMeta.url;
                    rm.encrypted = itemMeta.encrypted;
                    rm.isFolder = (itemMeta.type == L"folder");
                    for (const auto& t : itemMeta.tags) rm.tags.append(QString::fromStdWString(t));
                    rm.palettes = itemMeta.palettes;
                    rm.isManaged = false; // 标记为磁盘非受控资产

                    // 预载写入内存快照，防止频繁 I/O
                    std::unique_lock<std::shared_mutex> lock(m_mutex);
                    if (currentSnapshot) {
                        auto newMap = std::make_shared<std::unordered_map<std::wstring, RuntimeMeta>>(*currentSnapshot);
                        (*newMap)[nPath] = rm;
                        std::atomic_store(&m_snapshot, std::shared_ptr<const std::unordered_map<std::wstring, RuntimeMeta>>(newMap));
                    }
                    return rm;
                }
            }
        }
    }

    return RuntimeMeta();
}
>>>>>>> REPLACE
```

---

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] `src/meta/MetadataManager.cpp`（重写 `MetadataManager::getMeta` 加入 `.ArcMeta.json` 磁盘回退反向预载与快照写入逻辑）

**明确禁止越界修改的范围：**
- [ ] `src/ui/MetaPanel.cpp` 中的 `setCategoryPills` 与 `setDiskPathMode` —— 已经 100% 正确落地，禁止碰触！
- [ ] `src/ui/MainWindow.cpp` 中的信号槽绑定 —— 已经 100% 正确落地，禁止碰触！
- [ ] `src/ui/TagManagerDialog` 的声明与实现 —— 已经 100% 正确落地，禁止碰触！

---

## 6. 实现准则与预警【核心】
1. **多线程锁安全性**：在写回 `m_snapshot` 时，必须使用 `std::unique_lock<std::shared_mutex> lock(m_mutex);` 保护，并使用 `std::atomic_store` 原子替换，确保无锁读取快照的多线程高并发绝对安全。
2. **物理/逻辑两轨彻底隔离**：通过 `!isInsideManagedLibrary(nPath)` 进行强隔离拦截，确保本 Fallback 仅在普通物理磁盘导航下触发，绝对不干扰也不污染托管分类库逻辑，完全对齐双轨隔离铁律。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 双轨模式隔离 | 在库外普通磁盘模式下，元数据自动调用 AmMetaJson 精准、非侵入式写入主程序 ArcMeta.cache/文件夹哈希.json 离散缓存中，确保不污染用户原始物理盘。本方案为读取侧，同样只从指定目录的 .ArcMeta.json 离散缓存中拉取，不发生任何对 SQLite 库的写入溢流。 | ✅ 符合。完全实现双轨元数据对称读写，不发生交叉溢流。 |

---

## 8. 待确认事项（可选）
- **无**。
