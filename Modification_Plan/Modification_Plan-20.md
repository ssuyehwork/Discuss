# 地址栏导航历史与弹出面板解耦重构 —— Modification_Plan-20.md

## 1. 任务背景
在目前的应用架构中，地址栏（`AddressBar`）和地址历史记录悬浮面板（`AddressHistoryPanel`）在视觉呈现、用户交互与数据持久层之间存在着不必要的紧密耦合。特别是，`AddressBar` 直接承担了导航历史数据的增、删、改、查管理职责，通过直接读写全局 `AppConfig` 并手动维护数组截断上限（原第 132~140 行）。此外，`AddressBar` 还要作为中介，监听 `AddressHistoryPanel` 的元素删除与清空信号并手动同步底层配置和刷新界面。这种界面组件兼任数据持久化控制器的设计，严重违背了“单一职责原则”和“MVC 分层设计规范”。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 悬浮历史面板圆角设计（8px / 4px） | 在样式设计上明确 `AddressHistoryPanel` 外边框圆角为 8px，单条悬停 row 背景圆角为 4px | ✅ 一致 |
| 2    | 历史记录限制 15 条 | 在新业务服务中，无条件将导航历史上限统一约束为最高 15 条，并支持 AppConfig 内存极速持久化 | ✅ 一致 |
| 3    | 地址栏不接触 AppConfig 数据操作 | 地址栏 `AddressBar` 彻底剥离对 `AppConfig` 的读写与数组管理，转为纯 View-Controller | ✅ 一致 |

## 2. 问题定位
- **定位模块 1（数据持久化逻辑严重穿透 UI）**：
  在 `src/ui/AddressBar.cpp` 中，`saveToHistory` 函数直接通过 `AppConfig::instance().getValue("AddressBar/History")` 读取历史数组，进行去重、头部插入以及硬编码上限 10 条的截断操作（与用户视觉规范中的 15 条存在冲突），使得路径编辑 UI 承担了重型本地 I/O 与数据策略逻辑。
- **定位模块 2（信号与数据控制职责交叉耦合）**：
  在 `AddressBar.cpp` 的构造函数中，存在多处直接对 `m_historyPanel` 信号的强业务逻辑绑定：
  ```cpp
  connect(m_historyPanel, &AddressHistoryPanel::historyItemRemoved, this, [this](const QString& path) {
      QStringList history = AppConfig::instance().getValue("AddressBar/History").toStringList();
      history.removeAll(path);
      AppConfig::instance().setValue("AddressBar/History", history);
      m_historyPanel->setHistory(history);
  });
  ```
  这使得 `AddressBar` 沦为了历史面板的数据控制器。一旦未来引入其他形式的导航（如标签页导航或侧边栏前进/后退），该数据同步逻辑将无法复用，产生严重的散落冗余。

## 4. 详细解决方案

### 4.1 核心解耦：引入 `NavigationHistoryService`（导航历史持久化服务）
在 `src/core/`（或 `src/meta/`）下新建无状态/单例业务服务类 `NavigationHistoryService.h` 与 `NavigationHistoryService.cpp`：
- **职责范围**：专门管理、排序、约束和持久化用户的导航路径历史，提供干净的强类型接口，彻底屏蔽 `AppConfig` 细节。
- **上限统一管理**（对应用户要求："面板支持 15 条记录上限"）：
  在服务层定义静态/硬性上限常量 `MAX_HISTORY_COUNT = 15`。

```cpp
namespace ArcMeta {
class NavigationHistoryService : public QObject {
    Q_OBJECT
public:
    static NavigationHistoryService& instance();

    QStringList getHistory() const;
    void appendPath(const QString& path);
    void removePath(const QString& path);
    void clearAll();

signals:
    void historyChanged(const QStringList& newHistory); // 驱动所有订阅 UI（如地址栏和搜索历史）自动对齐

private:
    const int m_maxLimit = 15;
};
}
```

### 4.2 UI 瘦身与极速重构
1. **`AddressBar` 去数据化**：
   - 彻底删除 `AddressBar::saveToHistory` 成员方法。
   - 移除构造函数中对 `m_historyPanel` 各种数据增删信号的手动 Lambda 回调绑定。
   - 重构为直接单向调用：
     - 在跳转成功后，直接调用 `NavigationHistoryService::instance().appendPath(path)`。
     - 在双击触发弹出时，直接调用 `m_historyPanel->setHistory(NavigationHistoryService::instance().getHistory())` 并显示。
2. **`AddressHistoryPanel` 数据绑定内聚**：
   - 弹出面板在点击“全部清除”或“单条删除”时，直接向 `NavigationHistoryService` 发起修改指令，并在 `historyChanged` 信号广播时自我更新列表，无需再通过 `AddressBar` 扮演中介。
   - 这不仅使得 `AddressHistoryPanel` 具备完全独立的重用能力，也让 `AddressBar` 重新回归纯粹的面包屑/编辑控件定位。

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：
  - `src/ui/AddressBar.h` / `.cpp` （剥离 AppConfig 历史操作，转为调用 Service）
  - `src/ui/AddressHistoryPanel.h` / `.cpp` （剥离中介层，直接绑定 Service）
- [ ] 新增模块/文件：
  - `src/core/NavigationHistoryService.h` / `.cpp` （导航历史独立服务）

**明确禁止越界修改的范围：**
- [ ] 严禁修改面包屑按钮排版算法、自研 QSS 圆角边框的高分屏缩放比率。
- [ ] 严禁在 `NavigationHistoryService` 中使用任何原生 SQLite 底层 SQL 语句（导航历史只依靠 QSettings / AppConfig 轻量键值持久化）。

## 6. 实现准则与预警【核心】
1. **信号循环保护**：在 `NavigationHistoryService` 发射 `historyChanged` 信号、驱动 `AddressHistoryPanel` 自动 `rebuild` 刷新时，要防止删除动作本身由于控件的生命周期销毁而产生偶尔的悬挂引用（Dangling Pointer）。所有的删除 Action 必须在主事件循环中采用 `deleteLater` 异步回收，杜绝段错误。
2. **圆角规范精准落地**（对应用户原话："地址历史面板 8px / historyRow 4px"）：
   在 `AddressHistoryPanel.cpp` 重构中，必须精确写入：
   - `#AddressHistoryPanel { border-radius: 8px; }`
   - `QWidget#historyRow { border-radius: 4px; }`
   以严格对齐视觉红线。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 `Modification_Plan-20.md` |
| 考古原则 | 重构代码中的按钮悬浮样式必须对齐 `#3E3E42`（Style::HoverBackground） | ✅ 符合，面板内部 row 及 btn 悬浮样式严格对齐视觉标准 |
| 输入框清除 | 一律使用 Qt 原生 `setClearButtonEnabled(true)` | ✅ 符合，不涉及清除按钮改动 |
