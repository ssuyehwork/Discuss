# 品牌与 Logo 资产归一化实施方案 (Brand & Logo Assets Consolidation Implementation Plan)

> 本方案严格遵循《AGENTS.md》实施界限与《Standard Specifications for Implementation Plan.md》十全大补规范，旨在彻底落实用户“以 RapidNotes 为主，QuarkMeta 为次”的核心设计理念，消除残留的 QuarkMeta Windows 资源文件与清单描述，清除死代码 `QuarkApplication`，确保 Windows 任务管理器、UAC 提权界面及应用各窗口 100% 呈现统一纯粹的 RapidNotes 品牌形象。

---

## 前置：已读物理证据清单 (Gate 1 Evidence)

1. **物理读取文件列表**：
   - `RapidNotes_Source/RapidNotes/QuarkMeta.rc`
   - `RapidNotes_Source/RapidNotes/QuarkMeta.manifest`
   - `RapidNotes_Source/RapidNotes/CMakeLists.txt`
   - `RapidNotes_Source/RapidNotes/src/main.cpp`
   - `RapidNotes_Source/RapidNotes/src/ui/QuarkApplication.h`
   - `RapidNotes_Source/RapidNotes/src/ui/QuarkApplication.cpp`
   - `RapidNotes_Source/RapidNotes/resources/resources.qrc`
   - `RapidNotes_Source/RapidNotes/Memories_RapidNotes.md`

2. **核心事实与违规残留物理行号核查**：
   - `QuarkMeta.manifest:3-4`: 依然残留 `<assemblyIdentity name="QuarkMeta" .../>` 与 `<description>QuarkMeta File Manager</description>`；
   - `QuarkMeta.rc:5`: 注释残留 `// 1 24 "QuarkMeta.manifest"`；
   - `CMakeLists.txt:594-595`: 注册了未使用的孤儿类 `src/ui/QuarkApplication.cpp` 与 `src/ui/QuarkApplication.h`；
   - `main.cpp`: 实际实例化的是官方 `QApplication a(argc, argv);`，`QuarkApplication` 完全未被引用，为死代码。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题根因分析
1. **品牌资产主次倒错与遗留清单混淆**：
   - 在将 QuarkMeta 能力融合并入 RapidNotes 源码树后，工程根目录下直接残留了原工程的 `QuarkMeta.rc` 与 `QuarkMeta.manifest`；
   - Windows 操作系统在解析程序清单时，会读取 `<description>QuarkMeta File Manager</description>`，导致用户在系统任务管理器、UAC 授权弹窗以及程序属性中看到错误的旧名称，严重违背“以 RapidNotes 为主，QuarkMeta 为次”的品牌铁律；
2. **孤儿死代码混入构建系统**：
   - `QuarkApplication`（原用于 QuarkMeta 单实例自愈）在合并后被闲置，主入口 `main.cpp` 已使用 RapidNotes 既有的单实例与应用生命周期体系；
   - 但 `CMakeLists.txt` 仍将其打包进 `RapidNotes` 可执行文件中，不仅造成编译时间浪费与二进制体积膨胀，还存在命名冲突隐患。

### 1.2 重构治理目标
1. 将 `QuarkMeta.manifest` 与 `QuarkMeta.rc` 规范重命名/收敛为 `RapidNotes.manifest` 与 `RapidNotes.rc`；
2. 清单描述彻底修正为 `RapidNotes Desktop Application`，装配件名称对齐为 `RapidNotes`；
3. 从 `CMakeLists.txt` 中物理注销未被使用的 `QuarkApplication` 死代码；
4. 保证全工程 Windows 桌面级图标、任务栏、标题栏与托盘图标 100% 稳定引用 RapidNotes 官方资产。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `RapidNotes_Source/RapidNotes/QuarkMeta.manifest` — 修正装配件标识与程序描述为 RapidNotes。
2. `RapidNotes_Source/RapidNotes/QuarkMeta.rc` — 修正注释引用为 RapidNotes。
3. `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 移除未使用的 `QuarkApplication.cpp` / `.h` 编译条目。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `RapidNotes_Source/RapidNotes/QuarkMeta.manifest` — 归一化装配件名称与程序描述

```diff
<<<<<<< SEARCH
  <assemblyIdentity version="1.0.0.0" processorArchitecture="*" name="QuarkMeta" type="win32"/>
  <description>QuarkMeta File Manager</description>
=======
  <assemblyIdentity version="1.0.0.0" processorArchitecture="*" name="RapidNotes" type="win32"/>
  <description>RapidNotes Desktop Application</description>
>>>>>>> REPLACE
```

### 3.2 `RapidNotes_Source/RapidNotes/QuarkMeta.rc` — 修正清单引用说明

```diff
<<<<<<< SEARCH
// 1 24 "QuarkMeta.manifest"
=======
// 1 24 "RapidNotes.manifest"
>>>>>>> REPLACE
```

### 3.3 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 物理移除 QuarkApplication 死代码构建条目

```diff
<<<<<<< SEARCH
    src/ui/NavPanel.cpp
    src/ui/NavPanel.h
    src/ui/QuarkApplication.cpp
    src/ui/QuarkApplication.h
    src/ui/QuickLookGraphicsView.cpp
=======
    src/ui/NavPanel.cpp
    src/ui/NavPanel.h
    src/ui/QuickLookGraphicsView.cpp
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译与生成验证步骤
1. 执行 CMake 构建：
   ```cmd
   cmake --build build --config Release
   ```
2. 预期生成结果：
   - `QuarkApplication.cpp.obj` 不再被编译与链接；
   - 目标文件 `RapidNotes.exe` 顺利输出至固化运行层；
3. 检查生成的可执行文件：
   - 右键 `RapidNotes.exe` -> 属性 -> 详细信息：
     - 文件说明应正确呈现为 `RapidNotes Desktop Application`；
   - 在任务管理器与托盘区中，均显示清晰统一的 RapidNotes 品牌与 Logo。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 资产/要素 | 官方 SSOT 规范 | 消除的冗余与历史包袱 |
|---|---|---|
| 程序入口与应用类 | `QApplication` (`src/main.cpp`) | 消除未使用的孤儿类 `QuarkApplication` |
| 程序图标 | `resources/app_icon.ico` / `app_icon.png` | 统一收敛至 RapidNotes 官方唯一图标 |
| Windows 清单 | `RapidNotes.manifest` | 消除 "QuarkMeta File Manager" 描述混淆 |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 品牌资产项 | 唯一 SSOT 源 | 派生展示点 | 说明 |
|---|---|---|---|
| 主品牌名称 | `"RapidNotes"` | 任务栏、窗口标题栏、托盘 Tooltip | 唯一主品牌 |
| 应用程序图标 | `resources/app_icon.ico` | Windows Shell、任务管理器、标题栏 | 唯一主图标 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

本方案针对资源清单与构建注销，不修改公共 C++ 头文件与冻结接口签名。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

移除 `QuarkApplication` 在 `CMakeLists.txt` 中的编译注册后，已检索全工程：除其自身 `.cpp` 外，没有任何其他头文件或源码包含 `QuarkApplication.h`，包含链保持 100% 闭合完好。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 说明 |
|---|---|---|---|
| 1 | 方案已通过需求确认 | ✅ 满足 | 用户明确批准品牌以 RapidNotes 为主进行归一化 |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见前置证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 经由 verify_plan.py 自动化校验通过 |
| 4 | 新增接口声明、定义、调用点一致 | ✅ 不涉及 | 未增减业务接口 |
| 5 | 被修改的 `#include` 闭合 | ✅ 满足 | 全工程无对 QuarkApplication 的间接包含 |
| 6 | 新建/删除文件在 CMakeLists 同步 | ✅ 满足 | 在 CMakeLists.txt 中注销移除项 |
| 7 | 跨类调用访问级别符合要求 | ✅ 不涉及 | 无跨类调用变更 |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 严格限制在品牌清单与对应构建配置 |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | 物理清除无用编译单元，节约构建算力 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 彻底消除品牌混淆根因 |
| 11 | 遵循 Memories_RapidNotes.md 规范 | ✅ 满足 | 严格对齐第 25 章品牌资产规范 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 不涉及 | 不涉及 ToolTip |
| 13 | 100% 语义化矢量图标 | ✅ 满足 | 保持矢量图标统一性 |
| 14 | 异步回调按值捕获 | ✅ 不涉及 | 无异步回调 |
| 15 | 无 Lambda 循环引用 | ✅ 不涉及 | 无 Lambda |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 不涉及 | 无跨线程 UI |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 不涉及 | 不涉及信号槽连接 |
| 18 | 对外 `.h` 签名冻结 | ✅ 满足 | 保持对外头文件冻结不变 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明
| 既有功能 | 本方案影响 | 结论 |
|---|---|---|
| 快捷悬浮窗与主窗口启动 | 启动流与生命周期保持 100% 不变 | ✅ 行为完全一致 |
| 系统托盘交互 | 托盘图标、右键菜单保持 100% 不变 | ✅ 行为完全一致 |

### 数值零篡改声明
| 属性项 | 当前设定 | 结论 |
|---|---|---|
| 图标尺寸 | 16x16 / 18x18 / 28x28 | ✅ 100% 原样保留 |
| 窗体与菜单尺寸 | 保持既有数值 | ✅ 100% 原样保留 |
