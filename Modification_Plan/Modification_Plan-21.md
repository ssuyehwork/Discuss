# 搜索历史面板与多处交互解耦重构 —— Modification_Plan-21.md

## 1. 任务背景
在目前的架构设计中，搜索历史记录面板（`SearchHistoryPanel`）被复用在两个完全不同的业务场景下：主窗口顶部的全局搜索（`MainWindow.cpp`）和侧边过滤面板中针对具体属性的分组检索（`FilterPanel.cpp`）。然而，目前这两处调用点都深度穿透了 UI 边界，通过直接在各自类中组装 Lambda 回调读写 `AppConfig` 或手动拼接历史列表。由于缺乏一个中心化的检索历史持久层，导致两处 UI 控制器内部存在大量重复的历史去重、数量截断以及配置同步逻辑，严重违背了“单一职责原则”和“高内聚低耦合”的模块化准则。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 搜索历史面板圆角设计（8px / 4px） | 在样式设计上明确 `SearchHistoryPanel` 外边框圆角为 8px，单条悬停 row 背景圆角为 4px | ✅ 一致 |
| 2    | 历史记录限制 10 条 | 在新业务服务中，默认将单个搜索属性或全局搜索的历史上限约束为最高 10 条 | ✅ 一致 |
| 3    | 移除多处 UI 重复的 AppConfig 操作 | 通过引入统一的 `SearchHistoryService`，将全局和局部搜索历史的读写、截断与持久化彻底剥离 UI | ✅ 一致 |

## 2. 问题定位
- **定位模块 1（全局搜索历史逻辑散落耦合）**：
  在 `src/ui/MainWindow.cpp`（第 613~630 行）中，主窗口不仅要处理复杂的视图跳转和状态机，还要通过 `AppConfig` 手动维护一个名为 `SearchHistory` 的列表，执行去重、插入以及截断上限（10 条）的操作。
- **定位模块 2（局部筛选面板历史逻辑高度重复）**：
  在 `src/ui/FilterPanel.cpp`（第 481~505 行）中，过滤面板为了支持标签、路径等输入框的历史记录，对每一个输入过滤器属性（key）单独监听 `SearchHistoryPanel` 的清理和项点击信号，并在内部手动读写 `AppConfig::instance().getValue(QString("SearchHistory/%1").arg(key))`，代码逻辑与 `MainWindow.cpp` 完全重复，属于典型的数据逻辑在表示层（View）硬拷贝。

## 4. 详细解决方案

### 4.1 核心解耦：引入 `SearchHistoryService`（搜索历史多键值管理服务）
在 `src/core/`（或 `src/meta/`）下新建无状态/单例业务服务类 `SearchHistoryService.h` 与 `SearchHistoryService.cpp`：
- **多维度 Key 值隔离管理**：
  由于搜索历史存在“全局搜索（key='global'）”与“按列过滤（key='tags'、'note' 等）”多个维度，服务层提供基于维度参数（category）的管理能力：
- **上限硬性控制**（对应用户要求："搜索历史面板支持 10 条记录上限"）：
  在服务层定义上限常量 `MAX_SEARCH_LIMIT = 10`。

```cpp
namespace ArcMeta {
class SearchHistoryService : public QObject {
    Q_OBJECT
public:
    static SearchHistoryService& instance();

    // 根据分类维度获取对应的历史列表 (如 category = "global", "tags", "note" 等)
    QStringList getHistory(const QString& category) const;
    void appendSearch(const QString& category, const QString& keyword);
    void removeSearch(const QString& category, const QString& keyword);
    void clearAll(const QString& category);

signals:
    // 数据变更广播，驱动对应的历史弹出面板自动刷新
    void searchHistoryChanged(const QString& category, const QStringList& newHistory);

private:
    const int m_maxLimit = 10;
};
}
```

### 4.2 双向解耦与极速重构
1. **表示层 `MainWindow` 与 `FilterPanel` 去配置化**：
   - 彻底删除两处 UI 内部关于 `AppConfig` 的搜索历史读写、`removeAll`、`prepend` 等物理数组截断逻辑。
   - 替换为单向高层 Service 调用：
     - 在执行搜索时，直接调用 `SearchHistoryService::instance().appendSearch(category, text)`。
     - 在弹出下拉时，直接通过 `m_historyPanel->setHistory(SearchHistoryService::instance().getHistory(category), ...)` 渲染。
2. **`SearchHistoryPanel` 自我事务化绑定**：
   - 面板内部维护当前管理的属性维度 `m_category` 字符串。
   - 在用户点击“全部清除”或“关闭删除”时，面板直接调派 `SearchHistoryService::instance().removeSearch(m_category, keyword)`，并在 `searchHistoryChanged` 广播中对齐 m_category 刷新，完全解脱外部 UI 控制器的同步负担。

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：
  - `src/ui/MainWindow.h` / `.cpp` （剥离全局 `SearchHistory` 读写）
  - `src/ui/FilterPanel.h` / `.cpp` （剥离局部输入框多维度历史读写）
  - `src/ui/SearchHistoryPanel.h` / `.cpp` （内部封装 m_category，直接通信 Service）
- [ ] 新增模块/文件：
  - `src/core/SearchHistoryService.h` / `.cpp` （多维度搜索历史管理中心）

**明确禁止越界修改的范围：**
- [ ] 严禁在除 `SearchHistoryService` 之外的任何 UI 控件中读取 `SearchHistory` 相关的 AppConfig 本地配置。
- [ ] 严禁修改 QLineEdit 的任何其他原生过滤信号。

## 6. 实现准则与预警【核心】
1. **多端并发干扰防护**：当用户在主窗口搜索，或者在过滤面板频繁退格时，服务层可能高频触发数据存储。服务层必须基于 `AppConfig`（内部通常是无锁或已同步的 `QSettings`）做好写时内存防抖，或者使用轻量读写锁保护内存缓存副本。
2. **圆角规范精准落地**（对应用户原话："搜索历史面板 8px / historyRow 4px"）：
   在 `SearchHistoryPanel.cpp` 重构中，必须精确写入：
   - `#SearchHistoryPanel { border-radius: 8px; }`
   - `QWidget#historyRow { border-radius: 4px; }`
   以严格符合视觉红线。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 `Modification_Plan-21.md` |
| 考古原则 | 面板右上角“全部清除”按钮的样式和交互必须对齐 `AddressHistoryPanel` | ✅ 符合，按钮字体、颜色与悬停蓝色完全一致 |
| 输入框清除 | 一律使用 Qt 原生 `setClearButtonEnabled(true)` | ✅ 符合，不涉及清除按钮改动 |
