# 彻底移除 QMessageBox 调试弹窗与失效数据机制 —— Modification_Plan-2.md

## 1. 任务背景
基于用户的新指令，我们不但在底层恢复使用 `NativeFolderWatcher` (IOCP) 文件监控机制以替代 USN Journal，还必须将此前因调试 USN 开发而引入的所有 `QMessageBox` 调试弹窗日志彻底移除。此外，用户期望将侧边栏分类（对应用户原话：“侧边栏分类中的”）中的“失效数据”（对应用户原话：““失效数据”相关逻辑代码”）机制完全废除并彻底移除相关逻辑代码。

## 2. 问题定位
1. **QMessageBox 调试弹窗定位**：
   - 调试源头：`src/core/AutoImportManager.cpp`
   - 具体代码：包含了 `QMessageBox::information(nullptr, title, text);` 及包装它的 `safeShowMessageBox`，并在 `startListening`、`onEntryAdded`、`processImportQueue`、`isUnderManagedLibrary` 中多次调用。
   - 引入头文件：`#include <QMessageBox>`
2. **“失效数据”机制及侧边栏项定位**：
   - 侧边栏添加：`src/ui/CategoryModel.cpp` 中的 `addSystemItem("失效数据", "invalid_data", "invalid_data", "#f1c40f", -9);`
   - 视图和模型：`src/ui/InvalidDataListView.h` 与 `src/ui/InvalidDataModel.h` 负责展示和删除失效项目。
   - 关联控制：`src/ui/MainWindow.cpp` 的 `m_invalidDataListView` 的创建、控制显隐和添加到 `m_mainSplitter`。
   - 底层标记：`src/meta/MetadataManager.cpp` 中的 `setInvalid`、`setInvalidRecursive`、`setInvalidByFrn` 等标记逻辑，以及 `CategoryRepo::getSystemCategoryPaths` 的 `"invalid_data"` 路径提供。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 将相关“QMessageBox”调试日志彻底移除掉，绝不可以保留 | 在 `AutoImportManager.cpp` 中彻底清除所有 `QMessageBox` 调用、`safeShowMessageBox` 辅助函数及 `#include <QMessageBox>` 头文件。 | ✅ |
| 2    | 将侧边栏分类中的“失效数据”相关逻辑代码也彻底移除掉 | 彻底移除侧边栏分类（对应用户原话：“侧边栏分类中的”）中添加“失效数据”的节点，物理删除 `InvalidDataListView.h` / `InvalidDataModel.h` 两个文件并清理 `MainWindow` 中所有相关的引用与实例创建。 | ✅ |

## 4. 详细解决方案

### 4.1 彻底移除 QMessageBox 调试日志
1. **删除 `AutoImportManager.cpp` 中的全局辅助函数与头文件**：
   ```cpp
   // 彻底移除（对应用户原话：“彻底移除掉，绝不可以保留”）：
   // #include <QMessageBox>
   // static void safeShowMessageBox(const QString& title, const QString& text) { ... }
   ```
2. **清除所有调用点**：
   - 彻底删除 `startListening` 中的 `safeShowMessageBox("调试：托管库预热探测器", debugPrewarmLog);`。
   - 彻底删除 `processImportQueue` 中的 `safeShowMessageBox("调试：USN 批量入库触发器", ...);`。
   - 彻底删除 `isUnderManagedLibrary` 中的 `safeShowMessageBox("调试：USN 路径溯源追踪器", debugTraceLog);`。

### 4.2 彻底移除侧边栏中的“失效数据”项
1. **移除 `CategoryModel.cpp` 的系统项注册**：
   - 彻底删除下面这一行（对应用户原话：“将侧边栏分类中的“失效数据”相关逻辑代码也彻底移除掉”）：
     `addSystemItem("失效数据", "invalid_data", "invalid_data", "#f1c40f", -9);`

### 4.3 物理移除前端视图及模型文件
1. **物理删除（对应用户原话：“也彻底移除掉”）**：
   - 彻底删除 `src/ui/InvalidDataListView.h`
   - 彻底删除 `src/ui/InvalidDataModel.h`

### 4.4 清理 MainWindow.cpp 中的残留代码
1. **清理头文件引入**：
   - 移除 `#include "InvalidDataListView.h"`。
2. **清理成员变量与实例**：
   - 从 `src/ui/MainWindow.h` 中删除 `class InvalidDataListView* m_invalidDataListView = nullptr;`。
   - 从 `src/ui/MainWindow.cpp` 中删除 `m_invalidDataListView` 的创建、初始化、以及隐藏/显隐切换的所有相关逻辑代码。

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [x] 模块/文件：`src/core/AutoImportManager.cpp`（清除 QMessageBox 所有依赖）
- [x] 模块/文件：`src/ui/CategoryModel.cpp`（删除侧边栏“失效数据”项）
- [x] 模块/文件：`src/ui/MainWindow.cpp` & `src/ui/MainWindow.h`（移除 `m_invalidDataListView` 成员与调用逻辑）
- [x] 模块/文件：物理删除 `src/ui/InvalidDataListView.h` 与 `src/ui/InvalidDataModel.h`

**明确禁止越界修改的范围：**
- [ ] 模块/文件：其他系统分类（如“全部数据”、“未分类”、“回收站”）的显示逻辑及正常统计，绝不可受到任何影响。

## 6. 实现准则与预警【核心】
1. **清理 CMakeLists.txt 依赖**：如果 `CMakeLists.txt` 或相关的构建机制里包含或索引了 `InvalidDataListView.h` / `InvalidDataModel.h` 的头文件，必须对其予以彻底清理以防止引发构建中断。
2. **未分类和回收站的健壮性**：即使移除了“失效数据”的前端呈现，底层 `MetadataManager` 原本提供的 `isInvalid` 属性在短期内建议作为惰性字段保留（或者废止，但不要随意破坏数据库的 DDL 结构 `is_invalid` 字段），防止触发数据库 Schema 不匹配导致程序崩溃。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式| 严禁修改代码、创建代码、执行构建或测试 | ✅ (本案不直接修改或创建代码源文件，仅作为文档输出) |

## 8. 待确认事项（可选）
* 无。
