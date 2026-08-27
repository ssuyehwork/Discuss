# QuarkMeta 架构蓝图与重构总大纲
# (QuarkMeta Architecture Outline & Refactoring Plan.md)

---

## 🏛️ 第一章：顶层架构哲学与五层分层规范 (Clean Architecture)

`QuarkMeta` 定位于工业级、高性能、纯磁盘架构的桌面文件管理与元数据处理系统。为彻底根治“上帝类膨胀”、“隐式跨层耦合”、“平台级 Hack 补丁”及“事件风暴”等架构顽疾，全工程强制划分为严格单向依赖的**标准五层逻辑架构**：

```
┌────────────────────────────────────────────────────────────┐
│ 1. 窗口壳体层 (Native Window Shell Layer)                    │
│    - 仅负责 OS 原生无边框拉伸、拖拽、标题栏按钮组装与窗口 Geometry  │
├────────────────────────────────────────────────────────────┤
│ 2. 视图呈现层 (View / UI Panels Layer)                       │
│    - 仅负责 QWidget 布局、绘图渲染、状态高亮与用户原始输入捕获   │
│    - 内部 UI 状态必须 private，对外通过标准 Getter/Setter/Signal 交互│
├────────────────────────────────────────────────────────────┤
│ 3. 控制协调层 (Controller / Mediator / Handlers Layer)       │
│    - 仅负责事件路由、动作分发、跨面板状态中介与上下文菜单构建   │
│    - 严禁包含 HTML 字符串渲染、屏幕坐标几何计算与物理文件 I/O    │
├────────────────────────────────────────────────────────────┤
│ 4. 业务领域层 (Domain Service / Core Engine Layer)           │
│    - 状态单一持有者 (如 NavigationService 持有历史栈与当前 URL)│
│    - 统一接收 AppCommand 校验，执行业务计算并广播 AppEvent      │
├────────────────────────────────────────────────────────────┤
│ 5. 数据与基础设施层 (Repository / IO / Database Layer)       │
│    - 负责 SQLite(global.db)、.QuarkMeta.json 存储与 LRU 内存缓存 │
│    - 重型物理 I/O、扫描与媒体提取必须异步隔离，UI 主线程零阻塞  │
└────────────────────────────────────────────────────────────┘
```

---

## 🛑 第二章：架构绝对红线与重构铁律

为保证代码库的高内聚、低耦合与长期可维护性，所有开发与重构行为必须无条件遵守以下 **6 大架构红线**：

1. **【严禁盲改对外签名（Public API Freezing）】**：已有的 `public` 函数名称、参数列表及返回值不可随意变更。扩展功能必须采用「函数重载」、「添加默认参数」或提供平滑转发函数，绝不破坏既有调用方的编译契约。
2. **【严禁友元侵入与伪解耦（No Friend Classes）】**：严禁为了访问私有成员声明 `friend class`。拆分出的 Controller / Mediator 严禁持有上帝类的裸指针并直接读写其私有字段。模块间通信**只能通过标准接口、标准信号槽或 DTO 实体**进行。
3. **【严禁平台 Hack 级补丁（Zero Patch Tolerance）】**：严禁在未查明根因的情况下，引入操作系统底层 API（如 Win32 `SetForegroundWindow`）来“暴力掩盖”上层焦点与激活问题；严禁在渲染/样式工具类中篡改控件顶层窗口标志（如破坏 `Qt::Popup`）导致原生事件链断裂。
4. **【严禁 Model-View 抽象穿透】**：严禁在 View 外部或 Mediator 中将 `QAbstractItemModel` 强转为具体模型类并直接遍历其内部 `private` 数据容器。数据获取必须严格通过标准 `model->data(index, role)` 协议进行。
5. **【严禁批量循环发射事件（防止事件风暴）】**：在处理 $N$ 个项目的批量操作（星级、颜色、标签、删除等）时，严禁在 `for` 循环内逐个发射全局事件。必须先完成全量数据操作，最后只发射 1 次携带完整 `paths` 列表的聚合事件。
6. **【严禁全局事件过滤器无校验侵入】**：严禁在未做 `watched == m_targetWindow` 的白名单校验下将事件过滤器挂载至 `qApp` / `QCoreApplication`。所有拉伸/快捷键过滤器必须精确作用于目标控件，绝不干扰 `QMenu` 等 Popup 控件。

---

## 命名治理与代码规范 (Name Governance)

全工程所有新设及重构的文件、类、函数与变量必须严格对齐以下命名规范：

| 元素类型 | 命名风格 | 示例 / 规范 | 违规禁止 |
| :--- | :--- | :--- | :--- |
| **类名 (Class)** | PascalCase (大驼峰) | `TaskProgressController`, `NavigationService` | 缩写混乱或缺乏语义 (如 `TPCtrl`) |
| **文件名 (File)** | PascalCase (1:1 严格映射) | `TaskProgressController.h / .cpp` | 文件名与类名不对应、下划线小写混用 |
| **函数名 (Method)** | camelCase (小驼峰) | `bindContentPanel()`, `calculateRemainingTime()` | 动词模糊命名 (如 `doIt()`, `proc1()`) |
| **私有成员 (Member)**| camelCase + `m_` 前缀 | `m_searchEdit`, `m_taskProgressController` | 无 `m_` 前缀或强行 `public` 裸露 |
| **局部变量 (Local)** | camelCase (含义明确) | `selectedPaths`, `completedCount` | 单字母或无意义拼写 (如 `a`, `tmp2`) |

---

## 🗺️ 第三章：五阶段演进重构路线图 (Refactoring Roadmap)

为防止重构断层与节奏混乱，全工程重构划分为 **5 个循序渐进的演进阶段**：

### 📍 阶段 1：窗口壳体层收拢与全局事件降权 (Native Shell & Event Isolation)
- **目标**：收拢无边框拉伸、拖拽及标题栏交互，清除“全局事件侵入”。
- **关键任务**：
  1. 将拉伸/拖拽逻辑收拢至 `FramelessWindowHelper`，并确保 `ResizeEventFilter` 仅对主窗口生效。
  2. 修复所有组件对 `QMenu` (`Qt::Popup`) 窗口标志与事件抓取的破坏。

### 📍 阶段 2：主窗口（上帝类）瘦身与控制协调层重建 (Controllers & Mediators Decoupling)
- **目标**：将 `MainWindow` 瘦身为纯粹的组装壳体，充实【3. 控制协调层】。
- **关键任务**：
  1. **进度与搜索控制** (已完成)：`TaskProgressController` (进度条与倒计时)、`SearchController` (搜索框与防抖)。
  2. **导航控制器** (`NavigationController`)：接管 `file://`、`computer://`、`trash://` 统一协议路由与历史栈管理。
  3. **右键上下文菜单控制器** (`ContextMenuController`)：接管 4 种物理场景的右键 Action 构建与响应分发。
  4. **盘符栏控制器** (`DriveBarController`)：接管热插拔感应、卷标加载与标签管理入口。

### 📍 阶段 3：Model-View 协议规范与伪解耦清理 (Model-View & Interface Standardization)
- **目标**：消除【2. 视图呈现层】对【4. 业务领域层】的抽象穿透与友元侵入。
- **关键任务**：
  1. 彻底清理全工程所有 `friend class` 声明。
  2. 强制视图与中介者通过 `model->data(index, role)` 获取数据。
  3. 重构批量操作逻辑，实现 $O(1)$ 次数的聚合事件广播与 UI 刷新。

### 📍 阶段 4：数据基础设施落盘与异步线程隔离 (Data Repository & Thread Isolation)
- **目标**：规范【5. 数据与基础设施层】，确保 UI 主线程零阻塞。
- **关键任务**：
  1. 梳理 SQLite (`global.db`)、`.QuarkMeta.json` 与 `MetaMemoryCache` 三级数据流。
  2. 将磁盘物理扫描、深层缩略图解码、色彩提取彻底隔离在后台工作线程链中（如 `MediaExtractorPipeline`）。

### 📍 阶段 5：全局接口契约冻结与历史 Legacy 清场 (Dead Code Purge & Final Audit)
- **目标**：确保代码库纯洁性与向后兼容性。
- **关键任务**：
  1. 清理所有废弃的无用函数、冗余头文件及历史残留字段（如旧版分类逻辑）。
  2. 按照 `SYSTEM_PROMPT.md` 标准，对全工程进行最终静态规范审计与编译契约校验。

---

## ⚡ 第四章：数据流、并发线程与事件总线规范

1. **命令-事件单向数据流 (Command-Event Pattern)**：
   - UI 面板产生用户意图 -> 构建 `AppCommand` 提交给 `CoreEngine` -> `CoreEngine` 处理完毕后通过 `CentralEventHub` 广播 `AppEvent` -> 各面板监听 `AppEvent` 进行增量 UI 刷盘。
2. **并发与线程安全模型**：
   - UI 线程仅负责 QWidget 的轻量渲染与事件捕获。
   - 所有数据库事务 (`DatabaseManager`) 与文件 IO/解码统一运行在后台线程池；跨线程数据传递必须使用 Qt 强类型值拷贝或标准 Smart Pointer。
3. **选区响应熔断保护机制**：
   - 当用户选中项目数量突破阈值（如 50 项）时，选区广播自动触发熔断保护，禁止打包深拷贝数千个路径字符串，降级为仅传输首项索引进行元数据预览，防止全选操作引发卡死。

---

## 📋 第五章：重构执行与防断层防混乱约定

为了保证后续任何助手与开发者接手时均能无缝衔接、不发生断层：

1. **唯一顶层规划指导**：本大纲文档 (`QuarkMeta Architecture Outline & Refactoring Plan.md`) 与 `QuarkMeta-Architecture-Planning.md` 为顶层架构规划的唯一权威依据。
2. **无脑实施方案制作**：每一个具体重构任务在实施前，**必须且只能**在 `QuarkMeta Architecture/Implementation Plan/` 目录下创建只读的实施方案文档（英文小写命名，如 `navigation.md`）。
3. **四章节严格格式**：每个实施方案文档必须包含：
   - `1. Overview` (概述)
   - `2. Modified Files List` (修改文件清单)
   - `3. Detailed Line-by-Line Changes` (包含 CMakeLists.txt 在内的精准替换块)
   - `4. Build & Verification Steps` (编译与验证步骤)
