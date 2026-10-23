# Standard Specifications for Implementation Plan.md - 实施方案标准规范

语言：正文与注释只允许使用中文（标识符、代码块、标准 Git Merge Diff 标记与固定章节英文标题除外）。

---

1. 审核方式（必须遵守）：
    1.1 不得采信本方案自带的"已读证据清单"和"合规核对表"。它们是作者自己写的，作者已经漏过检查。你必须自己重新检索仓库，得到自己的证据。
    1.2  除问题一至四外，还必须逐项核对以下可验证事实，每项附检索命令与结果：
       a. 每个 SEARCH 块的原文在对应文件中真实存在，且在该文件内唯一匹配；
       b. 方案调用的每个外部成员函数，在 .h 中真实存在，签名与方案一致；
       c. 方案新增的接口，其声明、定义与全部调用点一致；
       d. 每个被修改或删除的 #include，已核对其间接包含链，.cpp 中用到的类型都有完整定义；
       e. 每个新建或删除的文件，已在 CMakeLists.txt 中同步注册或移除；
       f. 每个跨类调用的成员，其访问级别（public/protected/private）允许该调用；
       g. 方案没有触及方案范围外的文件。
    1.3 审核结论只能是「通过」或「不通过」。不通过时，逐条列出：不满足的项、位置（文件:行号）、证据原文。不得自行修改方案，不得自行折中，直接答复用户并等待回复。
    1.4 只有结论为「通过」且已向用户发送审核结论消息后，才可进入 Step 1。


## 3. 实施方案 (Implementation Plan) 规范

### 3.1 实施方案物理隔离与命名/只读/版本迭代契约
1. **物理存储与废弃归档目录**：
   - 活性实施方案必须统一放置于 `QuarkMeta Architecture/Implementation Plan/` 目录下。
   - 被新版本替代的旧方案文件，必须整文件移动至 `QuarkMeta Architecture/Implementation Plan/superseded/` 废弃归档子目录中（文件移动不视为违规编辑）。
2. **命名与 1:1 映射契约**：
   - **单文件/单类重构**：实施方案物理文件名必须与对应核心类名完全 1:1 映射（如 `NavigationService.md`）。
   - **多类/跨模块重构**：物理文件名必须以业务功能或主导服务类语义命名（如 `PresetTagPipelineRefactoring.md`）。
   - **版本迭代后缀规则**：迭代或修正版本必须使用 `-1.md`、`-2.md` 递增后缀（如 `NavigationService-1.md`）。方案文件头部必须显式声明“**本方案替代 `superseded/NavigationService.md`**”。执行者永远**只执行同一方案族中物理文件名后缀最高的最新版本**。
3. **锚点与只读锁定**：
   - **提交（Submission）锚点定义**：方案完成自检复核并在 Git 中提交独立 Commit 的那一刻，即视为“已提交”。方案头部必须记录：
     - `Base Commit Hash`（基准 Commit 哈希）；
     - `Prerequisite Plans`（前置依赖方案）；
     - `Supersedes`（所替代的旧方案相对路径，若无写“无”）。
   - **只读铁律**：方案一旦 Commit 提交，物理文件即被锁定为**只读**，绝对禁止编辑或覆盖！若基准 Commit 发生变化（如重新 `git reset` 或 `git merge`），必须核对 SEARCH 块有效性，需要更新时新建递增后缀文件并将原文件移入 `superseded/`。

### 3.2 代码替换格式 (Git Merge Diff & CRUD Standard)
所有代码改动必须使用标准 Git Merge Diff 块表达，SEARCH 块中的代码必须在当前基准 Commit 的目标文件中**真实存在且唯一匹配**（严禁缩减到可能多处匹配的通用单行），换行符统一在匹配时进行 `\r\n` 与 `\n` 归一化。

1. **局部替换/新增/删除代码**：
```
<<<<<<< SEARCH
原代码上下文 (必须唯一匹配)
=======
替换或新增的代码
>>>>>>> REPLACE
```

2. **新建完整文件 (Create File)**：
```
<<<<<<< SEARCH FILE: src/path/To/NewFile.h
=======
// 新建文件的完整内容
>>>>>>> REPLACE
```

3. **删除整个文件 (Delete File)**：
```
<<<<<<< SEARCH DELETE FILE: src/path/To/OldFile.cpp
=======
>>>>>>> REPLACE
```

4. **重命名或移动文件 (Rename File)**：
```
<<<<<<< SEARCH RENAME FILE: src/old/Path.cpp
=======
DESTINATION: src/new/Path.cpp
>>>>>>> REPLACE
```

### 3.3 构建与 CMake 注册全覆盖红线
1. **全文件注册契约**：无论新建 `.h`、`.cpp` 还是包含/不包含 `Q_OBJECT` 宏的普通 C++ 类，**必须在 `CMakeLists.txt` 的 `SOURCES` / `HEADERS` 列表中添加对应条目**。
2. **同步清理删除项**：删除或重命名文件时，**必须在 `CMakeLists.txt` 中同步移除或更新对应的文件条目**，杜绝未定义符号或文件找不到引发的链接/构建失败。

### 3.4 结构化实施方案必备章节 (10 大章节规范)

代码修改类实施方案必须完整包含以下 10 大章节：

1. **Chapter 1: Overview（概述与根因分析）**：必须阐明问题根因（拒绝盲目打补丁），说明重构设计目标。
2. **Chapter 2: Modified Files List（影响文件清单）**：列出所有新建、修改、重命名或删除的文件相对路径。
3. **Chapter 3: Detailed Line-by-Line Changes（精准替换块）**：包含所有 `.h`、`.cpp` 及 `CMakeLists.txt` 的 Git Merge Diff 块。
4. **Chapter 4: Build & Verification Steps（构建与运行时验收标准）**：包含分步编译命令、用户可观察的运行时验收标准及边界测试用例。
5. **Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）**：明确说明复用的官方 API（如 `refreshAll()`），确认消除重复造轮子。
6. **Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）**：列出本次重构涉及的数据项、唯一 SSOT 数据源、派生副本及同步/刷新方向。
7. **Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）**：
   - 外部调用接口：粘贴物理 `.h` 中的完整签名行；
   - 新增/重构接口：列出声明位置、定义位置及所有调用点签名一致性；
   - 跨类访问控制：检查调用符号的 `public`/`private`/`protected` 访问权限，提升私有级别或补齐 Getter。
8. **Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）**：核查改/删 `#include` 后的间接包含链，确保指针/枚举/前置声明类型在 `.cpp` 中闭合包含。
9. **Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）**：置于方案末尾，包含 18 项质量与流程检查，每项必须标明“满足/不满足/不涉及”及具体位置。
10. **Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）**：
    - 行为与交互零篡改：逐项列出既有 UI/交互设计（如行内重命名、弹窗逻辑），确认保持不变；
    - 样式数值零篡改：逐项列出迁移的具体数值（颜色 Hex、px 尺寸等），新旧值完全一致。

#### 裁剪版适用界限
- 仅当实施方案**完全不触碰 `.h`、`.cpp` 及 `CMakeLists.txt`**（如纯文本文档、沙箱环境配置、只读调研报告）时，方可使用裁剪版；
- 凡涉及任何代码或构建脚本修改的方案，**严禁使用裁剪版**，必须包含全部 10 大章节。

### 3.5 接口与类型签名物理核查与跨类契约
1. **物理阅读 `.h` 源码**：撰写 Diff 前，必须使用只读工具（`read_file` / `grep`）精准阅读物理 `.h` 头文件，禁止凭直觉脑补 API 命名。
2. **新增/修改接口全链核查**：方案中新增的每一个函数或类，必须在第 7 章列出其声明文件行号、定义文件行号及全部调用点，确保参数类型与返回值 100% 匹配。
3. **跨类访问权限全量普查**：对所有跨类调用的变量与函数进行访问级别检查，若原始为 `private`/`protected`，必须在 Diff 中提供公有 Getter 或提升为 `public`。
4. **Qt 信号槽与多线程跨界红线**：
   - `QObject::connect` 必须使用 C++11 函数指针语法，显式校验信号槽参数匹配；
   - 严禁在后台线程（如 `QtConcurrent::run`、线程池）中直接操作 `QWidget` UI 控件，必须通过 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 或信号槽调度回 GUI 主线程。

### 3.6 四道关质量管控流程 (Four-Gate Plan Protocol)

#### 第 0 关：需求与假设确认关 (Gate 0: Requirements Confirmation)
- **时机**：起草方案之前。
- **要求**：必须与用户明确沟通需求，消除任何模糊点或未决假设。在需求完全清晰且达成共识前，禁止盲目起草方案。

#### 第 1 关：起草前物理证据关 (Gate 1: Pre-Draft Evidence)
- **产出物**：方案 Chapter 1 前置的“已读物理证据清单”：
  1. **物理读取文件列表**：涉及的每一个 `.h` / `.cpp` 相对路径及只读读取确认；
  2. **头文件原样签名**：调用的外部成员函数在物理 `.h` 中的原样声明行；
  3. **既有 API 检索日志**：防重复造轮子的 `grep` 检索关键词与命中结果。

#### 第 2 关：交付前自动化自检关 (Gate 2: Automated Pre-Delivery Check)
- **自动化工具**：方案自检**强制使用仓库自动化脚本 `tools/plan_verify/verify_plan.py`**（代码库内版本控制）进行校验。脚本自动扫描：
  - SEARCH 块在仓库中的存在性与唯一匹配性；
  - 头文件包含链闭合性；
  - `CMakeLists.txt` 全文件注册完整性；
  - 10 大章节与合规表格填写的完整性。
- **产出物**：方案末尾的第 9 章“合规与质量核对表”，并附上 `tools/plan_verify/verify_plan.py` 的**自动化验证输出日志**。

#### 第 3 关：独立复核与提交锁住关 (Gate 3: Independent Review & Lock)
- **时机**：高风险方案（涉及架构重构、接口变动、数据库修改）交付给用户前。
- **执行者**：独立复核角色/会话。
- **复核通过后**：提交 Git Commit 固化锚点（写入 Base Commit Hash），文件归档锁定为只读。

#### 流程图
```
[第0关：需求确认与共识] —未达成—→ 请求用户澄清
         ↓ 达成共识
[第1关：已读物理证据清单] —缺失—→ 禁止撰写正文
         ↓ 齐全
[撰写方案正文]
         ↓
[第2关：运行 tools/plan_verify 自动化自检] —失败—→ 修改方案 → 重新运行自检
         ↓ 100% PASS
[第3关：独立复核通过 → 提交 Git Commit 锁定只读]
         ↓
[交付给用户审批 → 获批后方可执行动码]
```

### 3.7 编译报错归因迭代与工具版本化
1. **报错即时方案迭代**：遇到编译报错（如 MSVC C2248、C2039 等），必须新建递增后缀方案（如 `Xxx-1.md`），在 Overview 中进行报错归因分析，将原方案移入 `superseded/` 目录，禁止未改方案盲目修代码。
2. **验证工具仓库内版本管理**：所有用于方案校验、代码扫描、架构核查的自动化工具，**必须统一保存在项目仓库 `tools/plan_verify/` 目录下**，随 Git 提交进行版本控制，禁止保存在 sandbox 临时路径。
