# FilterPanel 星级与色标修改未实时同步问题重构实施方案

## 一、 问题现象与场景还原

在应用界面中，用户在中间内容区（ContentPanel）或右侧元数据属性面板（MetaPanel）对选中文件进行元数据修改（例如：将原本为 4 星级的文件更改为 1 星级）：
1. **当前表现**：元数据面板（MetaPanel）和中间卡片视图中的星级与颜色标记实时刷新显示。
2. **存在缺陷**：右侧筛选器面板（FilterPanel）中的统计数字角标（如 `4星: 1` 和 `1星: 0`）没有实时同步变更为 `4星: 0` 和 `1星: 1`。只有在用户重新加载整个文件夹或刷新视图时，筛选器面板的数字才被刷新。

---

## 二、 根因分析（Root Cause Analysis）

1. **事件订阅路由缺失（Event Hub Routing Breakpoint）**：
   - 当在 `MetaPanel` 或 `ContentPanel` 中修改文件的星级、色标或标签时，`CoreEngine` 执行数据库/配置文件写入，并由 `CentralEventHub` 发布 `AppEventType::MetadataUpdated` 事件。
   - `PanelMediator.cpp` 在接收到元数据变更或 `MetaPanel::ratingChanged` 信号时，仅调用了 `contentPanel->updateItemMetadata(path)` 更新了网格卡片/列表项的绘制，**未触发 FilterPanel 的数据重新统计或刷新回路**。

2. **筛选器统计数据（ScanStats）缺乏动态计算与增量更新**：
   - `FilterPanel` 的数量角标仅在初次载入文件夹触发 `directoryStatsReady(ScanStats)` 信号时更新一次。
   - 在用户就地（In-place）修改星级或色标时，缺少统一的统计重新计算方法 (`recalculateAndEmitStats`) 来驱动 `FilterPanel` 重新汇总当前 Model 的分级与色标分布。

---

## 三、 架构设计原则（Architecture Alignment）

1. **绝对遵循统一解耦**：
   - `FilterPanel` 不直接监听 `MetaPanel` 的 GUI 信号，所有跨面板的协作与数据流转必须通过 `PanelMediator` 统一中转与调度。
2. **保持状态（Keep State Unchanged）**：
   - 当统计数据重新计算并更新 `FilterPanel` 旁边的数量 Label 时，必须**严格保留用户当前已勾选的筛选复选框状态**，不得重置或刷新整个筛选表单。
3. **低开销重新统计（Lightweight Recalculation）**：
   - 充分利用 `ContentStatsWorker` 后台异步线程或 `ContentPanel` 现有的 Model 记录集，在内存中完成微秒级的汇总，避免不必要的全盘 IO 重新扫描。

---

## 四、 具体重构实施步骤蓝图（Blueprint）

### 1. `ContentPanel`（内容面板）增加极速重计与发射接口
- **接口定义** (`src/ui/ContentPanel.h`)：
  ```cpp
  public slots:
      /**
       * @brief 当内部模型的元数据（星级/色标/标签）发生就地变更时，重新计算分布统计并向 FilterPanel 发射 directoryStatsReady 信号
       */
      void recalculateAndEmitStats();
  ```
- **逻辑实现** (`src/ui/ContentPanel.cpp`)：
  ```cpp
  void ContentPanel::recalculateAndEmitStats() {
      if (!m_model || m_model->allRecords().empty()) return;
      if (m_statsWorker) {
          m_statsWorker->processAsync(m_model->allRecords(), m_currentFilter.showHidden);
      }
  }
  ```
- **在就地更新中触发**：
  在 `ContentPanel::updateItemMetadata(const QString& path)` 完成单个记录的元数据刷新后，自动调用 `recalculateAndEmitStats()`。

### 2. `FilterPanel`（筛选器面板）实现平滑角标更新
- **逻辑优化** (`src/ui/FilterPanel.cpp`)：
  在 `FilterPanel::populateStats(const ScanStats& stats)` 方法中：
  - 更新各个 Checkbox 旁边的数量文本（如 `1星 (1)`、`4星 (0)`）。
  - **切勿销毁或重新生成 QCheckBox 实例**，仅调用 `setText()` 更新 QLabel，确保用户当前的勾选状态（FilterState）不受任何干扰。

### 3. `PanelMediator`（中介者）闭环链路连接
- **修改文件**：`src/ui/PanelMediator.cpp`
- **信号槽绑定**：
  在 `PanelMediator::setupConnections()` 中，确保所有元数据修改路径（包括 `MetaPanel` 的 `ratingChanged`、`colorChanged`、`tagAddRequested` 以及 `CentralEventHub` 的 `MetadataUpdated` 事件）均连接至 `contentPanel->recalculateAndEmitStats()`。

---

## 五、 验证与测试方案

1. **星级修改同步验证**：
   - 打开包含多个不同星级文件的文件夹。
   - 在网格卡片或 MetaPanel 中将 4 星级文件修改为 1 星级。
   - 观察右侧 FilterPanel：4 星级计数即时 -1，1 星级计数即时 +1。
2. **筛选勾选状态保持验证**：
   - 勾选 FilterPanel 中“包含图片”或特定颜色分类。
   - 修改其中某文件的星级，验证筛选状态未被强制清空或重置。
