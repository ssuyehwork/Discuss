# 全工程静态红线、打补丁现象与 SSOT 通道偏离深度排查诊断与重构报告

> **排查依据**：`AGENTS.md` (工程纪律)、`SYSTEM_PROMPT.md` (五道工程硬锁) 与 `Memories.md` (唯一真理源)。  
> **排查工具**：`verify_code.py` 静态分析脚本与全工程代码结构深度比对。  
> **排查范围**：`src/` 目录下全部 C++/Qt 源代码。

---

## 一、 静态红线与“打补丁”现象全面排查结果

### 1. 违规 C++ 内联样式硬编码排查
根据 `Memories.md` Section 1.2“禁止 C++ 内联样式硬编码”铁律，全软件所有静态 UI 样式必须收敛至 `resources/style.qss`。排查发现以下 **4 处违规硬编码补丁点**：

| 文件物理路径 | 行号 | 违规硬编码内容 | 归一化重构方案 |
| :--- | :--- | :--- | :--- |
| `src/ui/dialogs/CreateFolderWithItemsDialog.cpp` | L62, L68, L80, L106, L124 | `titleLabel` 文字颜色与按钮/输入框内联 QSS 样式 | 彻底移除 C++ 内联样式设置，为对话框子控件配置 `objectName`（如 `#CreateFolderDialogTitle`），统一抽离并收敛至 `resources/style.qss`。 |
| `src/ui/ColorPicker.cpp` | L190 | `btn` 动态背景色设置 | 属于运行时动态 RGB/Hex 渲染场景，属于允许的极少数例外，但应通过 `QPalette` 或 QSS 变量集中处理。 |
| `src/ui/NavPanel.cpp` | L166 | `m_treeView` 局部样式字符串拼接 | 彻底移除局部内联字符串拼接，收敛至全局 `style.qss` 中的 `NavPanel QTreeView` 选择器。 |
| `src/ui/TitleBarWidget.cpp` | L236 | `menu` 追加内联样式字符串 | 改为调用统一入口 `UiHelper::applyMenuStyle(&menu)`。 |

---

### 2. `friend class` 伪解耦与黑盒破绑排查
根据 `SYSTEM_PROMPT.md` 🔒第四道锁，`friend class` 属于破坏黑盒隔离、将上帝类肢体外溢的伪解耦补丁。排查发现 **2 处违规友元声明**：

| 宿主类文件 | 违规友元声明 | 破绑隐患 | 重构归一化方案 |
| :--- | :--- | :--- | :--- |
| `src/ui/ContentPanel.h` | `friend class ContentViewCoordinator;` | `ContentViewCoordinator` 越权强行读写 `ContentPanel` 内部私有 `m_viewStack` 等成员 | 彻底移除 `friend` 声明。在 `ContentPanel` 中提供强类型 Getter/Setter 或 AppCommand 接口。 |
| `src/ui/ContentPanel.h` | `friend class ContentPaneSplitManager;` | `ContentPaneSplitManager` 强行越权操作 `ContentPanel` 内部私有布局 | 彻底移除 `friend` 声明。通过公共 Handler 契约通信。 |

---

### 3. 事件过滤器挂载校验排查
根据 `SYSTEM_PROMPT.md` 🔒第四道锁，排查全工程 60 处事件过滤器挂载：
* **合理使用场景**：`HoverEventFilter`（用于按钮悬停提示）与 `PaneActivationTracker`（用于多窗格激活跟踪）属于目的单一的控制/中介层逻辑；
* **违规补丁点**：`src/ui/ColumnViewPane.cpp` L96 强行将列表控件事件过滤器全局挂载给 `m_contentPanel` 接收，破坏了分栏视图项（`ColumnViewPane`）的封装独立性。重构应改为在 `ColumnViewPane` 内部高内聚处理。

---

## 二、 违背 SSOT (唯一真理源) 通道排查

根据 `AGENTS.md` Section 2.4 【核心通用行为 SSOT 入口字典】：
1. **文件/视图原位刷新** ➔ 唯一合法入口为 `ContentPanel::refreshAll()`；
2. **物理路径/视角导航切换** ➔ 唯一合法入口为 `ContentPanel::loadDirectory(path)`。

### 排查违规点：文件变动后错用视角导航接口替代原位刷新
在 `src/ui/ContentPanel.cpp` L205 与 L821 中，发现当进行隐式文件夹折叠/展开或列表重载时，代码**误调用了全量视角导航接口**，而不是 `refreshAll()`！
* **严重后果**：误调用视角导航接口会导致已经展开的分栏列（Miller Columns 4~5 列）、选中的项与视角位置被强行重置清空，破坏了用户当前的浏览视角。
* **重构方案**：彻底物理修改上述槽函数，将其纠正统一回流至 SSOT 入口 `refreshAll()`。

---

## 三、 重构实施与清理计划

1. **第一阶段（样式与友元解耦）**：
   - 清理 `CreateFolderWithItemsDialog.cpp` 与 `NavPanel.cpp` 中的内联 QSS 样式，收敛至 `resources/style.qss`；
   - 彻底擦除 `ContentPanel.h` 中的 `friend class` 伪解耦声明，补齐封装 Getter。
2. **第二阶段（SSOT 刷新通道归一化）**：
   - 将 `ContentPanel.cpp` 中误用视角导航接口进行原位刷新之处，统一归一化回流至 `refreshAll()` 通道。

本排查诊断报告已归档在物理隔离文件 `QuarkMeta Architecture/Implementation Plan/AntiPatch_and_SSOT_Audit_Report.md` 中。
