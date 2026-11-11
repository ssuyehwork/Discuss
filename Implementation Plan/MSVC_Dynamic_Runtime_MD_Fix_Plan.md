# MSVC 动态链接与运行时库不匹配彻底修复实施方案 (MSVC Dynamic Linking & Runtime Library Fix Implementation Plan)

> 本方案严格遵循《AGENTS.md》实施界限与《Standard Specifications for Implementation Plan.md》十全大补规范，旨在彻底消除 MSVC 链接器报出的所有 `LNK2038`（MT_StaticRelease 与 MD_DynamicRelease 不匹配）与 `LNK2005` / `LNK1169`（libcpmt.lib 与 msvcprt.lib 符号多重定义）致命错误，将 RapidNotes 构建系统完全归一化为官方标准动态链接版 Qt (`msvc2022_64`) 与 `/MD` 运行时库，并依托“三权分立”与 `windeployqt` 部署固化运行层。

---

## 前置：已读物理证据清单 (Gate 1 Evidence)

1. **物理读取文件列表**：
   - `RapidNotes_Source/RapidNotes/CMakeLists.txt`
   - `RapidNotes_Source/RapidNotes/CMakePresets.json`
   - `RapidNotes_Source/RapidNotes/RapidNotes_Compilation Error.md`
   - `RapidNotes_Source/RapidNotes/Memories_RapidNotes.md`
   - `T 统一编译路径.md`

2. **核心报错点物理行号核查**：
   - `RapidNotes/CMakeLists.txt:14`: `set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")` 强行指定了 `/MT` 静态运行时；
   - `RapidNotes/CMakeLists.txt:38`: `set(CMAKE_PREFIX_PATH "C:/Qt/6.10.2/msvc2022_64")` 引用了动态版 Qt（自身使用 `/MD` 构建）；
   - `RapidNotes/CMakeLists.txt:667-670`: `qt_import_plugins(RapidNotes ...)` 为静态 Qt 专属宏，在动态链接下会强行链接不存在或符号冲突的静态插件初始化 obj；
   - `RapidNotes/CMakeLists.txt:673`: `set_property(TARGET RapidNotes PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")` Target 级别强锁 `/MT`；
   - `RapidNotes/CMakeLists.txt:715`: `set_property(TARGET RapidNotesWatchdog PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")` Target 级别强锁 `/MT`；
   - `RapidNotes/CMakePresets.json:13`: `"CMAKE_PREFIX_PATH": "G:/Qt/6.10.2/msvc2022_64_static"` 依然残留旧的静态路径。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题根因分析
1. **运行时库模式分裂（MT vs MD）**：
   - RapidNotes 自身的编译单元（`.cpp`）在 CMakeLists.txt 的强制指令下，以 `/MT`（Release 下链接静态 C/C++ 运行时库 `LIBCMT.lib` 与 `LIBCPMT.lib`）进行编译；
   - 但所链接的官方标准动态版 Qt6 核心库（`Qt6Core.lib`、`Qt6Gui.lib`、`Qt6Widgets.lib`、`Qt6Sql.lib`、`Qt6Network.lib`、`Qt6Svg.lib`）在官方出厂时是以 `/MD`（Release 下链接动态 CRT `MSVCRT.lib` 与 `msvcprt.lib` / `MSVCP140.dll`）构建的；
   - MSVC 链接器检测到两套完全冲突的 C++ 运行时库依赖，触发了全量符号级 `LNK2038` 冲突与 `LNK2005` 多重定义致命错误（达数百条）。
2. **静态专属宏在动态环境下引起二次灾难**：
   - `qt_import_plugins(RapidNotes ...)` 会强行拉起各个 Qt 插件的静态初始化 obj（如 `QGifPlugin_init.cpp.obj`、`QICOPlugin_init.cpp.obj` 等），在动态链接模式下不仅完全冗余，而且是引发插件级 LNK2038 的直接祸首；
3. **预设配置与探测路径不统一**：
   - `CMakePresets.json` 写着静态路径 `G:/Qt/6.10.2/msvc2022_64_static`，而 `CMakeLists.txt` 写着动态路径 `C:/Qt/6.10.2/msvc2022_64`，路径配置自相矛盾。

### 1.2 重构治理目标
1. 彻底将 `CMakeLists.txt` 全局及 Target 级别的 MSVC 运行时库从 `MultiThreaded`（`/MT`）修正为 **`MultiThreadedDLL$<$<CONFIG:Debug>:Debug>`（`/MD` 与 `/MDd`）**；
2. 彻底拔除静态专属的 `qt_import_plugins` 宏；
3. 规范统一动态 Qt6 路径探测（支持环境变量 `QT_DIR` 与默认 `C:/Qt/6.10.2/msvc2022_64`）；
4. 同步更新 `CMakePresets.json` 中的 `CMAKE_PREFIX_PATH` 为动态版路径；
5. 保证构建生成的二进制输出到“固化运行层” `RapidNotes_Build/RapidNotes/`，配合 `windeployqt` 终身部署。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 切换全局与 Target 为 `/MD` 动态运行时，拔除 `qt_import_plugins`，对齐动态路径探测。
2. `RapidNotes_Source/RapidNotes/CMakePresets.json` — 修正 CMake 预设中的 `CMAKE_PREFIX_PATH` 为动态版 `msvc2022_64`。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 全局运行时切换为 /MD 并优化动态 Qt 探测

```diff
<<<<<<< SEARCH
# 全局设置 MSVC 为静态运行时库 (/MT Release, /MTd Debug)，匹配静态 Qt6 (msvc2022_64_static)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
=======
# 全局设置 MSVC 为动态运行时库 (/MD Release, /MDd Debug)，严格匹配动态版 Qt6 (msvc2022_64)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")
>>>>>>> REPLACE
```

### 3.2 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 修正默认动态 Qt6 探测路径优先级

```diff
<<<<<<< SEARCH
# 2026-03-xx 按照用户要求：增加环境自检提示，协助解决 Qt6 路径未找到问题
if(NOT Qt6_DIR AND NOT CMAKE_PREFIX_PATH)
    message(STATUS "🔍 [环境检测] 正在尝试从默认路径定位 Qt6...")
    set(CMAKE_PREFIX_PATH "C:/Qt/6.10.2/msvc2022_64")
endif()
=======
# 优先探测 QT_DIR 环境变量，若无则使用标准动态版 Qt6 (msvc2022_64) 路径
if(DEFINED ENV{QT_DIR})
    set(CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
elseif(NOT Qt6_DIR AND NOT CMAKE_PREFIX_PATH)
    message(STATUS "🔍 [环境检测] 正在尝试从默认路径定位动态版 Qt6...")
    set(CMAKE_PREFIX_PATH "C:/Qt/6.10.2/msvc2022_64")
endif()
>>>>>>> REPLACE
```

### 3.3 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 移除静态插件导入宏与 Target 级别 /MT 强锁

```diff
<<<<<<< SEARCH
# 从静态 Qt 中排除无用的 PostgreSQL 驱动，彻底消除对 libpq.dll、libssl-3-x64.dll、libintl-9.dll 的依赖
qt_import_plugins(RapidNotes
    INCLUDE_BY_DEFAULT
    EXCLUDE Qt6::QPSQLDriverPlugin
)

# 显式指定 Target 级别的静态运行时
set_property(TARGET RapidNotes PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
=======
# 显式指定 Target 级别的动态运行时 (/MD)
set_property(TARGET RapidNotes PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")
>>>>>>> REPLACE
```

### 3.4 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 看门狗 Target 级别运行时切换为 /MD

```diff
<<<<<<< SEARCH
set_property(TARGET RapidNotesWatchdog PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
=======
set_property(TARGET RapidNotesWatchdog PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")
>>>>>>> REPLACE
```

### 3.5 `RapidNotes_Source/RapidNotes/CMakePresets.json` — 修正预设为动态版 Qt 路径

```diff
<<<<<<< SEARCH
        "CMAKE_PREFIX_PATH": "G:/Qt/6.10.2/msvc2022_64_static"
=======
        "CMAKE_PREFIX_PATH": "C:/Qt/6.10.2/msvc2022_64"
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译与生成验证步骤
1. **清理历史旧缓存**：
   在开始前，手动或命令删除 `RapidNotes_Build/` 中的旧构建目录，确保无历史 `/MT` 生成的 `.obj` 残留；
2. **执行 CMake 配置与生成**：
   ```cmd
   cmake --preset x64-release
   cmake --build --preset x64-release
   ```
3. **预期编译与链接结果**：
   - `LNK2038: MT_StaticRelease 不匹配 MD_DynamicRelease` 彻底归零（0 条）；
   - `LNK2005 / LNK1169: 已经在 msvcprt.lib 中定义` 彻底归零（0 条）；
   - 链接器成功输出 `RapidNotes.exe` 与 `RapidNotesWatchdog.exe` 到 `RapidNotes_Build/RapidNotes/` 固化运行层；
4. **运行层部署 (windeployqt)**：
   ```cmd
   cd RapidNotes_Build/RapidNotes/
   windeployqt.exe RapidNotes.exe --no-translations --compiler-runtime
   ```
   一次性补齐所需的动态库与平台插件，双击 `RapidNotes.exe` 能够流畅秒开运行。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 构建要素 | 官方 SSOT 规范 | 消除的冗余/冲突 |
|---|---|---|
| 运行时库 | CMake 官方标准 `CMAKE_MSVC_RUNTIME_LIBRARY` | 消除手动拼接 `/MT`、`/MD` 编译选项引发的配置混淆 |
| 动态插件加载 | Qt6 官方自动插件加载机制 | 彻底消除 `qt_import_plugins` 静态硬链接导致的冗余符号冲突 |
| 运行层输出 | `CMAKE_RUNTIME_OUTPUT_DIRECTORY` | 严格收敛至三权分立固化运行层，消除重复部署痛点 |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 配置项 | 唯一 SSOT 源 | 派生点 | 说明 |
|---|---|---|---|
| Qt6 SDK 路径 | `QT_DIR` 环境变量 或 `C:/Qt/6.10.2/msvc2022_64` | `CMAKE_PREFIX_PATH` | 全局统一指向动态版 Qt SDK |
| 运行时库契约 | `MultiThreadedDLL` (`/MD`) | `RapidNotes` / `RapidNotesWatchdog` | 全工程严格对齐动态 CRT |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

本方案为构建工程脚本（CMakeLists.txt / CMakePresets.json）的配置重构，不修改 C++ 类访问级别与接口签名。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

本方案不修改 C++ 头文件与源代码包含链，消除所有由于宏未闭合或库冲突引起的底层符号未定义问题。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 说明 |
|---|---|---|---|
| 1 | 方案已通过需求确认 | ✅ 满足 | 用户明确选择“B. 动态链接版 (/MD + windeployqt)” |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见前置证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 经由 verify_plan.py 自动化校验通过 |
| 4 | 新增接口声明、定义、调用点一致 | ✅ 不涉及 | 纯构建配置重构 |
| 5 | 被修改的 `#include` 闭合 | ✅ 不涉及 | 未修改头文件包含 |
| 6 | 新建/删除文件在 CMakeLists 同步 | ✅ 满足 | 未增删 C++ 文件，仅修正构建属性 |
| 7 | 跨类调用访问级别符合要求 | ✅ 不涉及 | 无跨类调用 |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 严格限制在 RapidNotes 构建配置内 |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | 根治符号冲突与编译器策略 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 从根源上消除运行时库不匹配 |
| 11 | 遵循 Memories_RapidNotes.md 规范 | ✅ 满足 | 严格对齐第 24 章构建规范 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 不涉及 | 不涉及 UI 显示 |
| 13 | 100% 语义化矢量图标 | ✅ 不涉及 | 不涉及图标 |
| 14 | 异步回调按值捕获 | ✅ 不涉及 | 不涉及异步操作 |
| 15 | 无 Lambda 循环引用 | ✅ 不涉及 | 无 Lambda |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 不涉及 | 纯构建配置 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 不涉及 | 不涉及信号槽 |
| 18 | 对外 `.h` 签名冻结 | ✅ 满足 | 未触碰既有公共头文件签名 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明
| 既有功能 | 本方案影响 | 结论 |
|---|---|---|
| 应用程序所有功能逻辑 | 代码逻辑 100% 保持不变，仅底层 C++ 运行时库与动态链接方式对齐官方标准 | ✅ 行为完全一致 |
| 启动性能与内存占用 | 动态链接共享系统 DLL，启动更轻快，内存占用更低 | ✅ 纯性能与稳定性收益 |

### 数值零篡改声明
| 参数项 | 当前设定 | 结论 |
|---|---|---|
| 目标应用输出目录 | `RapidNotes_Build/RapidNotes` | ✅ 100% 原样保留 |
| C++ 标准规范 | C++20 | ✅ 100% 原样保留 |
