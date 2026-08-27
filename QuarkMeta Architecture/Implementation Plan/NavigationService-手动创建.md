在深入设计之前，我们先全面梳理**`NavigationService`（导航中枢服务）**的完整状态机模型、协议规范与各组件的单向响应契约：

---

### 一、 核心架构设计：状态机与协议体系

`NavigationService` 作为全系统**唯一的路径真理源（Single Source of Truth）**，必须严格管理以下核心状态与规则：

```
                              【NavigationService 状态中枢】
┌────────────────────────────────────────────────────────────────────────────────────────┐
│ 1. 协议归一化解析器 (Protocol Normalizer)                                               │
│    • 物理磁盘协议: `file://C:/Users` 或裸路径 `C:\Users` ──► 统一归一化为 `C:/Users`     │
│    • 系统虚拟协议: `computer://` (此电脑)                                              │
│    • 回收站协议:   `trash://` (系统/磁盘回收站)                                        │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ 2. 双向历史栈状态机 (History Stack State Machine)                                      │
│    • 历史堆栈: `QList<QString> m_history` (最大深度 100)                               │
│    • 栈顶指针: `int m_currentIndex`                                                    │
│    • 截断追加规则: 当处于历史中间节点发起新导航时，自动丢弃当前指针之后的前进记录          │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ 3. 上级路径（Go Up）智能解析规则                                                        │
│    • `C:/Folder/Sub` ──► `C:/Folder`                                                   │
│    • `C:/` 盘符根目录 ──► `computer://` (退回此电脑)                                    │
│    • `computer://` / `trash://` ──► 禁用 Up 操作 (canGoUp = false)                     │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

### 二、 `NavigationService` 领域服务标准接口设计

服务内部只管**状态维护、协议解析与历史计算**，不包含任何具体的 UI 绘制：

```cpp
namespace QuarkMeta {

class NavigationService : public QObject {
    Q_OBJECT

public:
    static NavigationService& instance();

    // 1. 核心导航动作
    void navigateTo(const QString& rawUrl, bool recordHistory = true);
    void goBack();
    void goForward();
    void goUp();
    void refresh(); // 重新广播当前路径，触发各视图重载

    // 2. 状态查询接口 (供 UI 绑定或初始状态同步)
    QString currentUrl() const { return m_currentUrl; }
    bool isVirtualProtocol() const; // 是否为 computer:// 或 trash://
    bool canGoBack() const { return m_currentIndex > 0; }
    bool canGoForward() const { return m_currentIndex < m_history.size() - 1; }
    bool canGoUp() const;

signals:
    // 🚀【全局统一通知信号】：
    // 当路径变更时发射，驱动全系统所有视图单向更新
    void currentUrlChanged(const QString& url, const QString& displayPath);

    // 当前进/后退/上级状态可用性发生变化时发射，驱动工具栏按钮置灰/激活
    void navStateChanged(bool canBack, bool canForward, bool canUp);

private:
    QString m_currentUrl;
    QList<QString> m_history;
    int m_currentIndex = -1;
};

} // namespace QuarkMeta
```

---

### 三、 观察者单向联动模型（组件各司其职）

重构后，各个 UI 组件与中枢的交互全部变为**“单向监听”**，组件之间彻底切断横向依赖：

```
                               NavigationService::instance()
                                            │
               ┌────────────────────────────┼────────────────────────────┐
               │ (currentUrlChanged)        │ (currentUrlChanged)        │ (navStateChanged)
               ▼                            ▼                            ▼
        【 ContentPanel 】           【 AddressBar 】             【 工具栏按钮 】
      • 加载对应目录数据           • 更新面包屑层级按钮         • 自动设置后退按钮 enabled
      • 自动重置旧的搜索框与筛选   • 文本框显示当前路径         • 自动设置前进按钮 enabled
                                                                 • 自动设置上级按钮 enabled
```

---

### 四、 对比：重构前 vs 重构后的 `MainWindow`

| 维度 | ❌ 重构前（上帝类大仓库） | ✅ 重构后（纯净装配容器） |
| :--- | :--- | :--- |
| **路径持有** | `MainWindow` 持有 `m_currentPath`、`m_history`、`m_historyIndex` | **0 个路径变量**（全部由服务持有） |
| **跳转实现** | 80 行 `unifiedNavigateTo` 手动操作各子控件 | **0 行跳转实现**，调用方直接找 `NavigationService` |
| **按键响应** | `onBackClicked` 手动算索引再回写 | `m_btnBack` 点击只需 `NavigationService::instance().goBack()` |
| **面板耦合** | `NavPanel`、`FavoritePanel` 必须拿 `MainWindow*` 调跳转 | **彻底解耦**，各 Panel 仅依赖标准单例 |

---

### 探讨结论：
通过将导航和历史栈彻底移入 `NavigationService`：
1. **`MainWindow` 的核心业务状态正式清零**；
2. **彻底消灭多面板之间的指针穿透和友元依赖**；
3. **实现前进/后退/上级按钮状态的全局自动化管理**。

请问您对上述 `NavigationService` 的状态机设计、协议解析规则与单向响应模型是否认可？若无异议，是否可以进入实施方案阶段？

// ===================|===================

