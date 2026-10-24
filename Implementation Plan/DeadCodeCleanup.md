# DeadCodeCleanup.md - 僵尸死代码类 DriveButton 物理彻底清除与注释修正方案

Base Commit Hash: `b46fdcfe017faaed42950107b45a2200ab05f9bf`
Prerequisite Plans: 无
Supersedes: 无

---

## Chapter 1: Overview（概述与根因分析）

### 1.0 Gate 1 已读物理证据清单 (Pre-Draft Physical Evidence)

#### 1. 物理读取文件列表
- `src/ui/DriveButton.h` / `src/ui/DriveButton.cpp`（只读核查：未被全仓库任何地方 `#include` 或实例化，属于僵尸死代码类）
- `src/ui/DriveBarWidget.h` / `src/ui/DriveBarWidget.cpp`（只读核查：真实职责为“扩展与组件管理工具栏”，但头文件注释保留了历史过时遗留描述“盘符与标签管理栏”）
- `CMakeLists.txt`（只读核查：第 174~175 行注册了 `src/ui/DriveButton.cpp` 与 `src/ui/DriveButton.h`）

#### 2. 头文件原样签名
- `class DriveButton : public QPushButton` (`src/ui/DriveButton.h:9`)
- `class DriveBarWidget : public QWidget` (`src/ui/DriveBarWidget.h:12`)

#### 3. 既有 API 检索日志
- `grep -rn "DriveButton" src/ CMakeLists.txt`
  - `src/ui/DriveButton.cpp`（类定义）
  - `src/ui/DriveButton.h`（类声明）
  - `CMakeLists.txt:174` (`src/ui/DriveButton.cpp`)
  - `CMakeLists.txt:175` (`src/ui/DriveButton.h`)
  - 确认全仓库无任何其他包含或调用点！

### 1.1 问题根因分析
在 QuarkMeta 代码库演进过程中，部分历史弃用的控件类（如 `DriveButton`）未被及时彻底清除，而仅停留在代码库中。与此同时，`DriveBarWidget` 随着功能重构演变为“扩展与组件管理工具栏”，但其头文件注释仍保留了旧版的“盘符”陈述。

这种与实际实现脱节的僵尸死代码和误导性注释，不仅增加了代码库的编译负担与空间占用，还会对架构分析与开发者沟通造成误导，违背了《源码纯净》与物理彻底清除死代码的规范契约。

### 1.2 架构重构设计目标
1. **物理彻底删除僵尸死代码 `DriveButton`**：删除 `src/ui/DriveButton.h` 与 `src/ui/DriveButton.cpp` 物理文件，并在 `CMakeLists.txt` 中同步移除其编译列表注册。
2. **修正 `DriveBarWidget.h` 的陈旧注释**：将 `DriveBarWidget.h` 中的过时类注释更正为其真实物理职责“扩展与组件管理工具栏组件”。
3. **保持构建与运行时零回归**：确保 CMake 正常重新生成并编译通过，项目运行不受任何副作用影响。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/DriveButton.h`（物理删除）
2. `src/ui/DriveButton.cpp`（物理删除）
3. `src/ui/DriveBarWidget.h`（修改注释）
4. `CMakeLists.txt`（修改注册列表）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 物理删除 `src/ui/DriveButton.h`

```
<<<<<<< SEARCH DELETE FILE: src/ui/DriveButton.h
=======
>>>>>>> REPLACE
```

### 3.2 物理删除 `src/ui/DriveButton.cpp`

```
<<<<<<< SEARCH DELETE FILE: src/ui/DriveButton.cpp
=======
>>>>>>> REPLACE
```

### 3.3 修正 `src/ui/DriveBarWidget.h` 头文件注释

```
<<<<<<< SEARCH
/**
 * @brief 盘符与标签管理栏独立组件
 */
class DriveBarWidget : public QWidget {
=======
/**
 * @brief 扩展与组件管理工具栏组件
 */
class DriveBarWidget : public QWidget {
>>>>>>> REPLACE
```

### 3.4 修改 `CMakeLists.txt` 注销死代码条目

```
<<<<<<< SEARCH
    src/ui/ContentPanel.cpp
    src/ui/ContentPanel.h
    src/ui/DriveButton.cpp
    src/ui/DriveButton.h
    src/ui/ViewDragDropHelper.cpp
=======
    src/ui/ContentPanel.cpp
    src/ui/ContentPanel.h
    src/ui/ViewDragDropHelper.cpp
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建命令
在项目根目录运行以下 CMake 构建命令进行编译核验：
```bash
cmake -B build -G Ninja
cmake --build build --config Release
```

### 4.2 运行时验收标准
1. **构建与链接成功**：项目构建不产生未定义符号（Undefined Symbol）错误或找不到文件的提示。
2. **界面运行完全正常**：`MainWindow` 及其下属 `DriveBarWidget`（扩展工具栏）正常初始化并呈现，工具栏按钮悬停提示与功能完全不受影响。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **死代码物理删除契约**：严格履行《AGENTS.md》2.4 节物理彻底清除死代码契约，将完全无用的僵尸类物理擦除。
2. **零重复造轮子**：未引入任何新控件或重置逻辑。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 / 缓存位置 | 同步 / 刷新方向 |
| :--- | :--- | :--- | :--- |
| 构建源文件列表 | `CMakeLists.txt` | CMake 构建系统缓存 | CMake 重新配置 ➔ 编译列表 |
| 扩展工具栏组件声明 | `src/ui/DriveBarWidget.h` | 编译器 C++ 符号 | 头文件解析 ➔ `MainWindow` 调用 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **接口移除**：彻底物理移除了未使用类 `DriveButton` 的所有类声明与成员函数；
2. **既有接口保护**：`DriveBarWidget` 的所有公有/私有 API 签名（`refreshPinnedButtons` 等）保持 100% 只读/冻结。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. **包含链无损核查**：确认全仓库无任何文件 `#include "DriveButton.h"`，删除 `DriveButton.h` 不会导致任何编译单元中断。
2. **类型完整性**：项目内其余 C++ 头文件与源文件包含链保持闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 说明 |
| :--- | :--- | :--- | :--- |
| 1 | 严禁盲改对外签名（Public API Freezing） | 满足 | 既有有效类签名冻结 |
| 2 | 依赖单向流动（Shell->View->Controller->Domain->Data） | 满足 | 物理删除废弃 View |
| 3 | 查重优先与秒级检索契约（Search First） | 满足 | 确认 `DriveButton` 零引用 |
| 4 | 核心行为 SSOT 入口唯一 | 满足 | 保持 `DriveBarWidget` 唯一性 |
| 5 | 物理彻底清除死代码 | 满足 | 物理删除 `DriveButton.h/.cpp` 并注销 CMake |
| 6 | 无内联硬编码样式 | 满足 | 不涉及 UI 样式变动 |
| 7 | 跨模块无全局事件过滤器滥用 | 满足 | 不涉及事件过滤器 |
| 8 | 零值与行为零篡改 | 满足 | 既有功能运行 100% 一致 |
| 9 | 10 大章节规范完整无缺 | 满足 | 本方案包含 Chapter 1 至 10 全章节 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 既有 UI / 交互行为 | 重构后表现 | 是否 100% 一致 | 备注 |
| :--- | :--- | :--- | :--- |
| 扩展工具栏按钮及提示 | 按钮悬停提示与菜单触发完全一致 | 100% 一致 | 移除冗余死代码 |
| 主窗口界面布局 | 主窗口显示与构建配置正常运行 | 100% 一致 | 构建效率微提升 |
