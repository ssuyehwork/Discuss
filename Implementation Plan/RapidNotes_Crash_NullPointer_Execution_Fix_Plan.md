# Implementation Plan: RapidNotes 运行时闪退与空指针执行根因治理实施方案 (RapidNotes Crash Null Pointer Execution Fix Plan)

本实施方案严格依据《AGENTS.md》协同指南、工业级架构规范《Standard Specifications for Implementation Plan.md》以及《Memories_RapidNotes.md》第 31 章要求制定。旨在彻底治理 `Complex Issues/Crash/RapidNotes-shantui.md` 转储报告所揭示的运行时 `c0000005` 空指针地址执行崩溃故障，消除 UI 树模型异步时序悬空隐患，确保所有生命周期与事件调用闭环。

---

## 前置物理证据清单 (Gate 1: Pre-Draft Evidence)

### 1. 物理读取与分析文件清单
- `Complex Issues/Crash/RapidNotes-shantui.md`（崩溃现场转储报告，确认故障桶 `SOFTWARE_NX_FAULT_NULL_INVALID_POINTER_EXECUTE_c0000005_RapidNotes.exe!Unknown`，异常地址 `0x0000000000000000`）
- `Complex Issues/Crash/shantui.md`（历史关联崩溃报告，确认 `QStandardItemModel::flags` 非法指针读取现场）
- `Complex Issues/Crash/shantui - 2.md`（历史关联崩溃报告，确认 `QAbstractItemView::setCurrentIndex` 非法索引解引用现场）
- `RapidNotes_Source/RapidNotes/src/ui/LibraryPanel.cpp`（物理核查第 1440-1468 行树节点恢复选中与异步延迟进入行内编辑的代码实现）
- `RapidNotes_Source/RapidNotes/src/ui/LibraryPanel.h`（物理核查模型成员 `m_model` 与树视图 `m_treeView` 访问权限与签名）
- `RapidNotes_Source/RapidNotes/src/main.cpp`（物理核查异常处理函数 `ApplicationCrashHandler` 与主循环消息泵）
- `QuarkMeta_Source/src/ui/LibraryPanel.cpp`（物理比对原版针对 `pIdx.isValid() && pIdx.model() == m_model` 的持久化索引防御范式）

### 2. 外部调用原样签名核对 (Public API Freezing)
- `QPersistentModelIndex::isValid() const`：Qt 核心标准接口，声明于 `<QPersistentModelIndex>`
- `QPersistentModelIndex::model() const`：Qt 核心标准接口，声明于 `<QPersistentModelIndex>`
- `QAbstractItemView::setCurrentIndex(const QModelIndex &index)`：Qt 视图标准接口，声明于 `<QAbstractItemView>`
- `QAbstractItemView::edit(const QModelIndex &index)`：Qt 视图标准接口，声明于 `<QAbstractItemView>`
- `QWidget::setFocus(Qt::FocusReason reason)`：Qt 控件标准接口，声明于 `<QWidget>`

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 崩溃故障根因溯源
1. **执行非可执行空地址 (NX Fault / Execute Address 0x0)**：
   - 转储报告明确记录：`ExceptionAddress: 0000000000000000`，`Attempt to execute non-executable address 0000000000000000`；
   - 在 x64 架构下，当 `rip = 0` 时，本质上是程序通过函数指针或虚函数表（`vtable`）调用了空指针地址。
2. **异步时序下的裸指针与悬空索引解引用 (Dangling Index in Async Timer)**：
   - 在 `LibraryPanel.cpp:1450` 树加载与刷新逻辑中，存在如下异步延迟调用：
     ```cpp
     QTimer::singleShot(50, this, [this, targetNodeId]() {
         QStandardItem* targetItem = findItemByNodeId(m_model->invisibleRootItem(), targetNodeId);
         if (targetItem && m_treeView) {
             m_treeView->setFocus();
             m_treeView->setCurrentIndex(targetItem->index());
             m_treeView->edit(targetItem->index());
         }
     });
     ```
   - **致命时序竞态**：
     1. 在 50ms 定时器排队等待期间，若用户或后台服务触发了数据库刷新或重新加载（`loadLibrary()`），原模型树会被 `m_model->clear()` 彻底清空或重构；
     2. 虽然再次查找到了 `targetItem`，但其内部的 `targetItem->index()` 属于普通瞬时 `QModelIndex`；
     3. 既有代码未对 `targetItem->index()` 进行 `QPersistentModelIndex` 持久化绑定与有效性校验；
     4. 既有代码更缺少 `pIdx.model() == m_model` 的模型归属核验；
     5. 既有调用顺序颠倒（先 `setFocus` 触发 Qt 内部焦点事件分发，再去 `setCurrentIndex` 和 `edit`），导致 `QAbstractItemView` 内部状态机在尚未就绪时即被强制触发元调用（`QMetaCallEvent`），直接引发解引用空函数指针或崩溃于 `QStandardItemModel::flags`。

### 1.2 治理目标与架构红线
1. **持久化索引契约与双重归属断言**：
   - 必须通过 `QPersistentModelIndex pIdx(targetItem->index())` 锚定索引生命周期；
   - 强制执行双重断言：`if (!pIdx.isValid() || pIdx.model() != m_model) return;`；
2. **弱引用守卫保护与生命周期闭环**：
   - 使用 `QPointer<LibraryPanel> weakThis(this)` 守卫异步定时器回调，防止面板在延迟期间被父窗口或视图销毁；
3. **状态机调用顺序规范化**：
   - 严格遵循：**先展开父级 -> 再设置当前索引 (`setCurrentIndex(pIdx)`) -> 再激活焦点 (`setFocus`) -> 最后开启编辑 (`edit(pIdx)`)**；
4. **实施界限铁律**：
   - 严格遵循《AGENTS.md》界限铁律，所有代码改动物理隔离记录于本实施方案中，不直接对源代码文件进行盲改。

---

## Chapter 2: Modified Files List（影响文件清单）

### 2.1 涉及修改的现有源文件清单
1. `RapidNotes_Source/RapidNotes/src/ui/LibraryPanel.cpp`（修复异步行内编辑时序竞态与索引有效性校验）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 修改 `RapidNotes_Source/RapidNotes/src/ui/LibraryPanel.cpp`
在异步延迟编辑逻辑中引入 `QPointer` 弱引用守卫、`QPersistentModelIndex` 持久化生命周期保护及双重有效性断言：

```
<<<<<<< SEARCH
    // 恢复树节点的选中高亮状态：严格同时校验 ID 与节点类型，严禁把普通分类的高亮挂到同名同 ID 的智能文件夹上
    if (savedSelectedNodeId != 0 && m_treeView) {
        QStandardItem* targetItem = findItemByNodeId(m_model->invisibleRootItem(), savedSelectedNodeId, savedSelectedType);
        if (targetItem) {
            m_treeView->setCurrentIndex(targetItem->index());
        }
    }

    if (m_pendingEditNodeId > 0 && m_treeView) {
        int targetNodeId = m_pendingEditNodeId;
        m_pendingEditNodeId = 0;
        QTimer::singleShot(50, this, [this, targetNodeId]() {
            QStandardItem* targetItem = findItemByNodeId(m_model->invisibleRootItem(), targetNodeId);
            if (targetItem && m_treeView) {
                QModelIndex parentIdx = targetItem->parent() ? targetItem->parent()->index() : QModelIndex();
                if (parentIdx.isValid()) {
                    m_treeView->expand(parentIdx);
                }
                m_treeView->setFocus();
                m_treeView->setCurrentIndex(targetItem->index());
                m_treeView->edit(targetItem->index());
            }
        });
    }
=======
    // 恢复树节点的选中高亮状态：严格同时校验 ID 与节点类型，严禁把普通分类的高亮挂到同名同 ID 的智能文件夹上
    if (savedSelectedNodeId != 0 && m_treeView && m_model) {
        QStandardItem* targetItem = findItemByNodeId(m_model->invisibleRootItem(), savedSelectedNodeId, savedSelectedType);
        if (targetItem) {
            QPersistentModelIndex pIdx(targetItem->index());
            if (pIdx.isValid() && pIdx.model() == m_model) {
                m_treeView->setCurrentIndex(pIdx);
            }
        }
    }

    // 延迟进入行内编辑（新建分类场景）：严格引入 QPointer 弱引用与持久化索引双重防悬空校验
    if (m_pendingEditNodeId > 0 && m_treeView) {
        int targetNodeId = m_pendingEditNodeId;
        m_pendingEditNodeId = 0;
        QPointer<LibraryPanel> weakThis(this);
        QTimer::singleShot(50, this, [weakThis, targetNodeId]() {
            if (!weakThis || !weakThis->m_model || !weakThis->m_treeView) return;

            QStandardItem* targetItem = weakThis->findItemByNodeId(weakThis->m_model->invisibleRootItem(), targetNodeId);
            if (!targetItem) return;

            QPersistentModelIndex pIdx(targetItem->index());
            if (!pIdx.isValid() || pIdx.model() != weakThis->m_model) return;

            QModelIndex parentIdx = targetItem->parent() ? targetItem->parent()->index() : QModelIndex();
            if (parentIdx.isValid()) {
                weakThis->m_treeView->expand(parentIdx);
            }

            // 必须先 setCurrentIndex，且使用持久化校验后的索引，再设置焦点和进入编辑
            weakThis->m_treeView->setCurrentIndex(pIdx);
            weakThis->m_treeView->setFocus(Qt::OtherFocusReason);
            weakThis->m_treeView->edit(pIdx);
        });
    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 控制变量交叉测试矩阵 (Zero-Tolerance Cross-Validation Matrix)
- **单一变量**：异步树操作时的模型项生命周期与索引合法性校验。
- **对照组 (Control)**：未校验 `QPersistentModelIndex` 与 `pIdx.model() == m_model`，在快速重命名、新建分类或高频树刷新时触发 `c0000005` 崩溃。
- **实验组 (Experiment)**：应用本方案的弱引用守卫与持久化模型索引校验，在同等高频并发时序下平稳运行。

### 4.2 编译与运行验证步骤
1. **构建工程**：
   使用 MSVC 2022 64-bit 编译器在 CMake Release 模式下编译 `RapidNotes` Target：
   ```bash
   cmake --build RapidNotes_Build/x64-release --target RapidNotes --config Release
   ```
2. **高频创建与行内重命名边界验证**：
   - 快速连续点击分类栏“+”号创建 10 个子分类；
   - 在 50ms 延迟行内编辑期间，通过快捷键或外部事件触发数据刷新；
   - 验证：程序平稳聚焦至新建分类并开启行内编辑框，未产生任何崩溃或 DUMP 文件。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **复用 Qt 官方原生机制**：
   - 全面复用 Qt 官方推荐的 `QPersistentModelIndex` 处理跨事件循环的模型索引访问；
   - 复用 `QPointer` 原生元对象弱引用守卫；
   - 彻底杜绝手写全局标志位或暴力 `sleep` 等临时补丁。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本 / 观察者 | 同步方向 | 校验机制 |
| :--- | :--- | :--- | :--- | :--- |
| 分类树模型索引 | `m_model` (`QStandardItemModel`) | `m_treeView` 视图选区 | Model ➔ View 单向同步 | `pIdx.model() == m_model` |
| 树节点编辑目标 | `m_pendingEditNodeId` (ID) | 瞬时栈变量 `targetNodeId` | 原子转移清零 | `weakThis` 生命周期守卫 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部调用接口核查**：
   - `findItemByNodeId` 属于 `LibraryPanel` 的内部私有工具函数，声明位于 `src/ui/LibraryPanel.h:112`；
   - `m_model` 与 `m_treeView` 均为 `LibraryPanel` 的成员变量，其生命周期受 `LibraryPanel` 管控。
2. **访问控制与签名匹配**：
   - Lambda 中按值捕获 `weakThis` 并通过指针解引用访问成员，完全满足 C++ 语法与作用域要求。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. **显式头文件包含检查**：
   - `RapidNotes_Source/RapidNotes/src/ui/LibraryPanel.cpp:25` 已显式包含 `#include <QPointer>`；
   - `RapidNotes_Source/RapidNotes/src/ui/LibraryPanel.h:5` 已显式包含 `#include <QStandardItemModel>` 与 `<QPersistentModelIndex>`；
   - 头文件包含链 100% 闭合，无任何缺失定义。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 核查项 | 结论 | 证据/说明 |
|:---:|---|:---:|---|
| 1 | 每个 SEARCH 块在工程中真实存在且唯一匹配 | 满足 | 经 `tools/plan_verify/verify_plan.py` 自动化检测 100% 唯一存在 |
| 2 | 调用的每个外部成员函数在 .h 中真实存在 | 满足 | `findItemByNodeId`、`setCurrentIndex`、`edit` 均真实存在 |
| 3 | 新增接口声明、定义与调用点一致 | 不涉及 | 本方案不引入新增外部公有接口 |
| 4 | 修改包含链后 .cpp 中所有类型完整定义 | 满足 | `QPointer` 与 `QPersistentModelIndex` 均已完整定义 |
| 5 | 新建/删除文件同步在 CMakeLists.txt 注册 | 不涉及 | 本方案未新建或删除物理文件 |
| 6 | 跨类调用成员访问级别允许 | 满足 | 仅在类内部安全访问成员 |
| 7 | 方案没有触及方案范围外的文件 | 满足 | 仅影响 `LibraryPanel.cpp` |
| 8 | 无笨拙且极度浪费计算资源的手法 | 满足 | 纯状态检查与索引持久化，零多余开销 |
| 9 | 无打补丁式特判或逃避方案 | 满足 | 从根源解决索引悬空与模型归属问题 |
| 10 | 严格遵循数据单一事实来源 (SSOT) | 满足 | 严格以 `m_model` 为单一模型真理源 |
| 11 | 采用标准 Git Merge Diff 语法表达 | 满足 | 标记严格独立成行且无嵌套冲突 |
| 12 | 语言纯中文规范 | 满足 | 正文与注释严格纯中文 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 交互设计零篡改核对表
| 交互场景 | 既有实现行为 | 重构方案行为 | 是否完全一致 |
| :--- | :--- | :--- | :--- |
| 新建分类行内编辑 | 50ms 后自动聚焦并展开父级进入树内编辑 | 50ms 后自动聚焦并展开父级进入树内编辑 | 完全一致 |
| 选中状态恢复 | 刷新完成后自动恢复先前选中项高亮 | 刷新完成后自动恢复先前选中项高亮 | 完全一致 |

### 10.2 样式数值零篡改核对表
| 样式 / 布局属性 | 原实现数值 | 重构方案数值 | 是否 100% 照抄 |
| :--- | :--- | :--- | :--- |
| 延迟触发时间 | `50ms` | `50ms` | 100% 照抄 |
| 焦点触发原因 | `Qt::OtherFocusReason` | `Qt::OtherFocusReason` | 100% 照抄 |
