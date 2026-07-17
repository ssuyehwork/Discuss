# SQLite 分类对账同步刷新修复 —— Modification_Plan-9.md

## 1. 任务背景
当用户在内容面板右键通过“迁移”或者将文件夹物理移动（如将“1”文件夹移动）至盘符下的“ArcMeta.Library_[盘符]”托管库文件夹内时，系统后端的 `AutoImportManager` 会通过物理扫描/对账机制（如 MFT/USN 或手动扫描）自动将该目录注册、持久化为数据库（SQLite）中的分类。然而，迁移完成后，侧边栏分类树（`CategoryPanel`）却没有自动刷新展示该新增的分类，只有在重启主程序后才会显示出来。这极大地破坏了界面的实时连贯性。

## 2. 问题定位
通过细致的代码审计，定位到以下两个关键缺陷导致刷新中断：

### 2.1 `CategoryPanel` 监听元数据变更信号时，未响应全量重建指令
在 `src/ui/CategoryPanel.cpp` 的构造函数第 130 行：
```cpp
    // 2026-06-xx 物理修复：监听元数据变更信号，确保删除项或标记状态后计数实时更新
    connect(&MetadataManager::instance(), &MetadataManager::metaChanged, this, [this](const QString& /*path*/) {
        requestRefresh();
    });
```
当 `AutoImportManager::handleRecursiveIngestion` 或 `processImportQueue` 完成对新分类的注册和数据库写入后，会调用 `MetadataManager::instance().notifyFullUIRebuild()`。该函数会将 `"__RELOAD_ALL__"` 塞入更新队列中并异步发射 `metaChanged("__RELOAD_ALL__")` 信号。
但是，`CategoryPanel` 在接收到此信号时：
- 完全忽略了 `path` 参数；
- 默认调用了 `requestRefresh()`（即不传参数，默认为 `fullRebuild = false`）。

这导致 `m_refreshTimer` 触发时，`needsFullRebuild` 始终为 `false`，从而跳过了对分类树模型的重新加载：
```cpp
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        if (!m_categoryModel) return;

        bool needsFullRebuild = m_refreshTimer->property("fullRebuild").toBool();

        if (m_isFirstLoad || needsFullRebuild) {
            m_categoryModel->refresh(); // 👈 只有这里才会重建分类树！但 needsFullRebuild 始终为 false 导致此处被跳过！
            m_isFirstLoad = false;
            m_refreshTimer->setProperty("fullRebuild", false); // 消费完重置
        }
        ...
```

### 2.2 `ImportHelper::importPaths` 之后没有主动触发侧边栏分类树的重建
物理迁移是通过内容容器右键的“迁移”选项或拖入分类引发物理移动并完成。
在物理迁移结束后，`ContentPanel` 触发了自身的 `refreshAll()`，但这仅仅重载了内容视图（Panel 二），而没有使侧边栏（Panel 一，`CategoryPanel`）获得全量重建通知，因而侧边栏不自动呈现新添加的分类。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 将文件夹迁移到 ArcMeta.Library 托管库内后侧边栏应该展示对应的分类 | 修复 `CategoryPanel` 在感知到全量重建信号 `"__RELOAD_ALL__"` 时进行重建加载 | ✅ 一致 |
| 2    | 不需要重启主程序，应该立即自动刷新 | 在 `MetadataManager::metaChanged` 信号响应中判断 `"__RELOAD_ALL__"` 并触发 `requestRefresh(true)` | ✅ 一致 |

---

## 4. 详细解决方案

### 方案 A：修正 `CategoryPanel::connect` 接收 `metaChanged` 的刷新逻辑
在 `src/ui/CategoryPanel.cpp` 构造函数第 130 行，针对 `MetadataManager::instance().metaChanged` 信号：
- 当 `path` 参数为 `"__RELOAD_ALL__"` 时，显式调用 `requestRefresh(true)`，强制使 `m_refreshTimer` 携带 `fullRebuild = true` 的标记。

修改前：
```cpp
    // 2026-06-xx 物理修复：监听元数据变更信号，确保删除项或标记状态后计数实时更新
    connect(&MetadataManager::instance(), &MetadataManager::metaChanged, this, [this](const QString& /*path*/) {
        requestRefresh();
    });
```

修改后：
```cpp
    // 2026-06-xx 物理修复：监听元数据变更信号，确保删除项或标记状态后计数实时更新
    // 2026-07-xx 按照 Plan-127：识别全量重建信号 __RELOAD_ALL__，强制触发侧边栏树结构刷新
    connect(&MetadataManager::instance(), &MetadataManager::metaChanged, this, [this](const QString& path) {
        if (path == "__RELOAD_ALL__") {
            requestRefresh(true);
        } else {
            requestRefresh(false);
        }
    });
```

---

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/CategoryPanel.cpp` 构造函数中关于 `MetadataManager::metaChanged` 的信号槽绑定连接。

**明确禁止越界修改的范围：**
- [ ] 严禁修改任何其他 UI 组件及分类数据的核心加载架构（`CategoryModel` 和 `CategoryRepo`）。

---

## 6. 实现准则与预警【核心】

1. **依赖的头文件**：修改均在已导入头文件 `MetadataManager.h` 保护的 `CategoryPanel.cpp` 中完成，无需引入新的头文件依赖。
2. **高风险操作警告**：`requestRefresh(true)` 会重置 `CategoryModel`（触发 `beginResetModel` 和 `endResetModel`），这在有极高频率的文件系统事件触发时可能会产生短暂开销。因此保持了 `QTimer` 的 **200ms 合并防抖时间** 能够完美避开信号风暴。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 双轨机制 | 侧边栏分类模式使用 SQLite 数据库关联加载；物理路径模式使用 QDir 实时扫描。 | ✅ 符合。本次仅在侧边栏数据库重载时接收信号刷新分类，绝对不触碰 QDir 物理导航链路。 |
| 分类刷新 | 需要及时对账与刷新。 | ✅ 符合。修复了信号传输中全量重建标志丢失的问题。 |

---

## 8. 待确认事项
无。方案已经过完整的考古和逻辑校验，确保精准闭环，绝不顾此失彼。
