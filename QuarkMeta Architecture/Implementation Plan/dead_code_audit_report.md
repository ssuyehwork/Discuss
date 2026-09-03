# 全工程与 MainWindow 僵尸代码 (Dead Code) 深度排查诊断报告

> **排查工具**：自研 C++ AST & Lexical 静态特征分析器（针对只写不读变量、孤立声明、无用槽函数及遗留字段进行全扫描）。  
> **排查范围**：`src/` 目录下全部 `.h` 与 `.cpp` 源文件。

---

## 一、 `MainWindow` 内部僵尸代码专项清单 (重点关注)

在 `src/ui/MainWindow.h` 与 `src/ui/MainWindow.cpp` 中，共排查出 **7 项明确的僵尸代码/遗留声明**：

### 1. 僵尸成员变量 (Dead Member Variables)

| 变量名 | 类型 | 诊断情况说明 | 清理建议 |
| :--- | :--- | :--- | :--- |
| `m_currentDataSource` | `QString` | **只写不读 (Write-Only)**。<br>`MainWindow.cpp` 中赋值 `m_currentDataSource = source;`，但全工程无任何读取逻辑。 | 彻底删除。由 `PanelMediator` 监听并处理高亮。 |
| `m_currentQuickLookPath` | `QString` | **孤立声明 (Shadowed Declaration)**。<br>`MainWindow.h` 中有声明，但实际 QuickLook 路由早已转移至 `PanelMediator::m_currentQuickLookPath`，`MainWindow` 中的完全没用。 | 从 `MainWindow.h` 中彻底删除。 |
| `m_toolbar` | `QToolBar*` | **仅声明未分配 (Unused Header Decl)**。<br>`MainWindow.h` 中定义 `QToolBar* m_toolbar = nullptr;`，但构造和 `initToolbar()` 中从未 `new` 或引用过。 | 彻底删除。 |
| `m_sidebarRefreshTimer` | `QTimer*` | **仅声明未分配 (Unused Header Decl)**。<br>已弃用的侧边栏刷新定时器指针，零使用。 | 彻底删除。 |

### 2. 僵尸函数与方法 (Dead Methods)

| 函数名 | 诊断情况说明 | 清理建议 |
| :--- | :--- | :--- |
| `void unifiedNavigateTo(...)` | **废弃代理转调**。<br>函数体仅转调 `NavigationService::instance().navigateTo(...)`，`MainWindow` 内部没有任何地方调用它。 | 彻底删除。 |
| `void resetSplitterLayout()` | **遗留重复声明**。<br>重置 Splitter 布局逻辑早已下沉至 `PanelLayoutManager::resetSplitterLayout()`，`MainWindow.h` 中仅残留了一个未调用的声明。 | 从 `MainWindow.h` 中彻底删除。 |
| `void onDriveBarContextMenu(...)` | **空处理槽函数 (No-op Slot)**。<br>函数体为空 `Q_UNUSED(pos)`，右键菜单响应逻辑未启用。 | 结合 `DriveBarWidget` 抽离时彻底清理。 |

---

## 二、 全工程其他模块僵尸代码汇总 (Other Modules)

### 1. 只写不读/僵尸变量 (Write-Only Member Variables)

* **`FilterPanel` 中的分组容器指针组**：
  * `m_groupRating`
  * `m_groupColor`
  * `m_groupLink`
  * `m_groupTag`
  * `m_groupRatio`
  * `m_groupDuplicate`
  * `m_groupNote`  
  * *诊断*：在 `FilterPanel.cpp` 初始化时被赋值 `m_groupXXX = g;`，但在后续过滤折叠与刷新控制中从未被读取。
* **`TreeItemDelegate` 中的状态控制位**：
  * `m_showStatus`（构造函数中赋值，但绘制逻辑 `paint()` 中未引用）。
* **`ShellProtectionCommand` 中的日志参数**：
  * `m_pwd`（构造函数初始化列表中赋值，但 `execute()` 中未读取）。

### 2. 仅声明未使用的成员变量 (Unused Header Declarations)

* **`TagSelectorOverlay.h`**: `bool m_wasActivated = false;`
* **`TaskProgressToolBar.h`**: `QPushButton* m_btnCancel = nullptr;`
* **`MetaPanel.h`**: `QWidget* m_linkBox = nullptr;`
* **`DropTreeView.h`**: `QTimer* m_autoExpandTimer = nullptr;` 与 `QModelIndex m_hoverIndex;`

---

## 三、 僵尸代码清理与重构集成策略

1. **第一阶段 (当前先导)**：在执行 `MainWindow` 解耦重构前或重构过程中，将 `MainWindow` 内部的 7 项僵尸变量与方法一并安全移除。
2. **第二阶段 (系统性清理)**：在后续各模块重构（如 `FilterPanel` 重构）时，按照本报告清单对其他模块的僵尸代码进行逐一清理，保持代码库的极高纯洁度。
