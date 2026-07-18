# 移植 FERREX-META 三视图与视口感知加载重构方案 —— Modification_Plan-12.md

## 1. 任务背景
在有 2692 个文件的大目录下，快速滑动右侧内容面板的 JustifiedView 会产生大面积灰色 `3A3A3A` 占位白条（渲染滞后）、排版重排卡顿和闪烁。
经代码走查，根因在于缩略图提取在全局线程池中无差别乱序派发，滑出视野的任务没有过期取消与丢弃机制，导致待加载任务大塞车；同时 JustifiedView 缺少视口虚拟化，且每次 AspectRatio 变更都会触发高频 dataChanged 导致全量项目在主线程重排排版。

为了彻底消除这个低效垃圾的排版和缩略图傻逼逻辑，提供丝滑流畅的媲美 Adobe Bridge 的浏览体验，我们将：
1. **参考并借鉴 `FERREX-META` 的优秀设计**，在当前工程中构建和引入**三种高精度视图**（列表视图 `ListResultView`、等高合理排版拼图视图 `JustifiedResultView`、网格卡片视图 `GridResultView`），统一由 `IScanResultView` 抽象接口驱动。
2. **重构缩略图生成与任务分发机制（Viewport-Aware Dispatcher）**：
   - 引入视口感知机制（Viewport-Aware Filter），优先为当前视口内的可见项目分配后台缩略图提取任务。
   - 引入过期丢弃机制，当快速滑动导致某项已经滑出视口时，立刻撤销或丢弃其尚未开始的异步提取任务。
3. **重构 `JustifiedView` 的排版机制**：
   - 增加布局缓存（Layout Cache），保证在 AspectRatio 变更或高频 dataChanged 时不进行全量重排。
   - 优化渲染循环，降低对主线程的消耗。

---

## 2. 问题定位与移植对账
- **视图接口抽象**：
  在 `FERREX-META` 中，视图继承自 `IScanResultView`。我们需要将该抽象接口、`ListResultView`、`JustifiedResultView` 和 `GridResultView` 移植至当前项目的 `src/ui/` 目录下，并进行客制化适配，使其基准数据模型挂接当前项目的 `FilterProxyModel`。
- **JustifiedView 布局重写**：
  `JustifiedView` 当前在 `doLayout` 存在全量排版重排卡顿，我们需要增加缓存哈希和快速返回机制。在滑动或非尺寸调整时使用缓存好的几何排版数据。
- **视口感知缩略图加载**：
  `FerrexVirtualDbModel::data` 原先收到 `DecorationRole` 请求时，在没有做视口过滤的情况下无差别执行 `QtConcurrent::run`。我们要将缩略图加载重构为**主动延迟加载/攒批视口队列机制**，在 View 滚动停止或滑动时，仅请求当前可见区域对应的 Index。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 拖拽滚动条下滑或上滑挑战显示数据性能，Adobe Bridge 非常丝滑，而当前版本灰色占位卡顿。 | 重构缩略图为主动延迟加载、视口优先及滑出任务过期丢弃机制，彻底解决后台提取大塞车导致灰色占位的逻辑缺陷。 | ✅ 一致 |
| 2    | 参考“FERREX-META”版本，共有三种视图，只可以参考、借鉴，但不要直接照搬。 | 在 `src/ui/` 目录下创建并移植 `IScanResultView.h` 以及三种客制化结果视图，统一内容容器管理，实现三视图一键丝滑切换。 | ✅ 一致 |
| 3    | 内存模式，效率如此低级。 | 优化 `JustifiedView` 的全量重排排版算法，增加几何数据缓存机制，将滑动重排的时间开销由 `O(N)` 降至近乎 `O(1)` 的局部渲染刷新。 | ✅ 一致 |

---

## 4. 详细解决方案

### 4.1 核心视图组件的参考与移植 (非照搬)
我们将在 `src/ui/` 目录下引入 `IScanResultView.h` 抽象视图接口：
```cpp
namespace ArcMeta {
class IScanResultView {
public:
    virtual ~IScanResultView() = default;
    virtual QWidget* getWidget() = 0;
    virtual QAbstractItemView* getBaseView() = 0;
    virtual void setModel(QAbstractItemModel* model) = 0;
    virtual void setIconSize(int size) = 0;
    virtual void refreshLayout() = 0;
};
}
```

随后，参考 `FERREX-META`，构建以下三个客制化视图类，挂接至当前项目的 `FilterProxyModel` 与 UI 主色调样式：
1. **`ListResultView`**：列表显示视图，主要包装 `QTreeView` 并应用 `TreeItemDelegate` 渲染文件名、评级星级、标签、修改日期等。
2. **`GridResultView`**：网格卡片显示视图，主要包装 `QListView` 并应用 `GridItemDelegate` 绘制正方形卡片和外置的文件名。
3. **`JustifiedResultView`**：等高合理排版拼图显示视图，包装当前的 `JustifiedView`，采用 Cover 的缩略图平铺模式。

---

### 4.2 视口感知（Viewport-Aware）与任务过期丢弃的缩略图管线重构
为了彻底消灭灰色占位卡片：
1. **停止在 `data()` 接口中高频无差别发起 `QtConcurrent::run`**。当模型收到 `DecorationRole` 请求时，若缩略图不在缓存中，仅将该项的 File ID / Path 注册进等待队列，返回空图标。
2. **在 `JustifiedView` 和 `GridResultView` 中监听垂直滚动条的 `valueChanged` 信号和视图的 `resizeEvent`**，并在滑动或尺寸改变后，启动一个短周期的 `m_visibleTimer`（如 50ms 延时）。
3. **计算当前视口可见的项目索引（Visible Indexes）**：
   通过 `viewport()->rect()` 以及 `indexAt()`，找出当前屏幕上真正可见的 `QModelIndex` 范围。
4. **优先且只派发可见区域的任务**：
   后台缩略图引擎（或专属线程池）按“从中心到两边”的可见顺序进行加载。
5. **过期丢弃**：
   如果滚动条继续滚动导致某个项目脱离了“可见区域”，立刻将该项目的 Path 从任务待加载队列中剔除，或通过原子状态作废其回调，释放后台提取线程，让位给最新滑入的可见项目。这保证了无论滑动多快，新滑入的项目都能在 10-50ms 内最优先获得计算资源，消除排队，极速转绿显示。

---

### 4.3 `JustifiedView` 排版计算缓存化重构 (O(N) -> O(1))
原先在 AspectRatio（宽高比）高频变更或 dataChanged 被通知时，`doLayout()` 都会盲目地对所有 N 项执行重新对齐和计算。
我们将重塑其排版管线：
1. **引入几何几何缓存 `std::vector<ItemGeometry> m_layoutCache`**。
2. **精细化控制 `doLayout` 触发时机**：
   - 只有在窗口实际被 Resize、或者添加/删除了项目时，才判定缓存失效并真正重跑全量对齐计算。
   - 当仅仅是某个文件的缩略图异步加载完成、触发 `dataChanged`（由于缩略图生成只是更新了该项的渲染内容，而不改变其原有的 AspectRatio ），或者进行滑动滚动时，**绝对直接跳过 `doLayout()` 几何计算**，直接利用缓存几何数据进行快速局部 `viewport()->update(visualRect(idx))` 绘图渲染。这使得滑动时的计算开销骤降为 `O(1)`，完全解放了 CPU 的主线程。

---

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/IScanResultView.h` (新建)
- [ ] 模块/文件：`src/ui/ListResultView.cpp/h`、`src/ui/JustifiedResultView.cpp/h`、`src/ui/GridResultView.cpp/h` (新建并从 FERREX-META 借鉴后客制化)
- [ ] 模块/文件：`src/ui/JustifiedView.cpp/h` (重构：引入排版几何缓存机制，限制 rowsRemoved、dataChanged 的高频全量重算)
- [ ] 模块/文件：`src/ui/ContentPanel.cpp/h` (重构：管理三种结果视图的实例化、无缝切换、视口感知与延时加载定时器)

**明确禁止越界修改的范围：**
- [ ] 严禁修改加密模块、MFT 文件库、USN Journal 监听等任何非视图相关代码。
- [ ] 严禁修改全局数据库文件 `global.db` 中的表结构定义。

---

## 6. 实现准则与预警【核心】
1. **防止跨线程 UI 冲突**：
   缩略图提取线程获取到 `QIcon` 后，必须通过 `QMetaObject::invokeMethod` 将其投递回主线程更新，绝不能直接在后台线程操作数据模型 `FerrexVirtualDbModel` 的数据结构或缓存集合，避免造成多线程写冲突导致程序闪退。
2. **双轴筛选与三视图对位一致性**：
   切换视图时，要确保新激活的目标视图挂接到当前的 `FilterProxyModel`。无论视图怎么切换，筛选面板（如颜色、评级星级）的状态在三视图中都必须保持绝对对齐和一致，不得产生多套独立的 Filter 状态。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 缩略图平滑加载 | 图形文件异步加载缩略图期间 data 接口必须返回空图标，由 Delegate 绘制灰色占位背景，消除闪烁 | ✅ 符合。本方案将完美践行第 9 条规范。 |
| 快速预览 (QuickLook) | 针对文件夹及不可预览文件严禁激活 QuickLook | ✅ 符合。不涉及该模块变动。 |
| 清除按钮样式 | 一律使用原生 setClearButtonEnabled(true) | ✅ 符合。本方案中如有输入框均严格执行。 |
