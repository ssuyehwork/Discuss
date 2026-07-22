# 品牌更名与名称统一替换方案 —— Analysis_Modification_Plan-160.md

## 1. 任务背景
在当前版本中，应用的系统标题栏和自定义标题栏品牌 Label 所显示的软件名称仍为 “FERREX”。为了统一项目品牌标识，符合最新的产品命名规约，用户委托我们将全应用中的 “FERREX” 品牌名称替换为 “ArcMeta”。

## 2. 问题定位
在当前活跃代码树中，“FERREX” 的硬编码及文档描述主要定位在以下位置：
1. **任务栏/系统窗口标题设置**：
   `src/ui/MainWindow.cpp` 第 95 行：`setWindowTitle("FERREX");`
2. **主窗口自定义标题栏品牌 Label**：
   `src/ui/MainWindow.cpp` 第 944 行：`m_appNameLabel = new QLabel("FERREX", m_titleBarWidget);`
3. **元数据管理器与缓存模块注释描述**：
   `src/core/CacheManager.h` 第 13 行与 `src/core/CacheManager.cpp` 第 28 行的缓存格式注释。
4. **品牌橙色配色注释描述**：
   `src/ui/StyleLibrary.h` 第 18 行的色彩定义注释。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 请将“FERREX”替换成“ArcMeta”（对应用户原话） | 将系统任务栏窗口标题、标题栏品牌 Label、以及相关活跃源码注释中出现的所有 “FERREX” 精准替换为 “ArcMeta”。 | ✅        |

## 4. 详细解决方案

Jules 作为**纯分析师角色**，在此仅提供精准修改位置方案，绝不直接执行任何代码文件的写操作。

### 4.1 窗口与品牌文本替换 (MainWindow)
在 `src/ui/MainWindow.cpp` 中定位并修改以下两处代码：
* **修改点 1（系统窗口/任务栏标题）**：
  ```cpp
  <<<<<<< SEARCH
      setWindowTitle("FERREX");
  =======
      setWindowTitle("ArcMeta");
  >>>>>>> REPLACE
  ```
* **修改点 2（标题栏左侧品牌名）**：
  ```cpp
  <<<<<<< SEARCH
      m_appNameLabel = new QLabel("FERREX", m_titleBarWidget);
  =======
      m_appNameLabel = new QLabel("ArcMeta", m_titleBarWidget);
  >>>>>>> REPLACE
  ```

### 4.2 注释与常熟规范说明更新 (CacheManager & StyleLibrary)
更新开发活跃类中的注释说明，确保不再残留旧名称：
* **修改点 3（缓存管理器头文件注释）**：
  在 `src/core/CacheManager.h` 第 13 行进行更新：
  ```cpp
  <<<<<<< SEARCH
  // 2026-05-09 按照用户要求：实现高效缓存机制，参考 FERREX 的 FIDX 格式
  =======
  // 2026-05-09 按照用户要求：实现高效缓存机制，参考 ArcMeta 的 FIDX 格式
  >>>>>>> REPLACE
  ```
* **修改点 4（缓存管理器源文件注释）**：
  在 `src/core/CacheManager.cpp` 第 28 行进行更新：
  ```cpp
  <<<<<<< SEARCH
  // 2026-05-09 按照用户要求：实现高效缓存管理器，参考 FERREX 的 FIDX 格式
  =======
  // 2026-05-09 按照用户要求：实现高效缓存管理器，参考 ArcMeta 的 FIDX 格式
  >>>>>>> REPLACE
  ```
* **修改点 5（品牌颜色常量注释说明）**：
  在 `src/ui/StyleLibrary.h` 第 18 行进行更新：
  ```cpp
  <<<<<<< SEARCH
  const QColor BrandOrange    = QColor("#cb7208"); // 核心品牌色 (FERREX)
  =======
  const QColor BrandOrange    = QColor("#cb7208"); // 核心品牌色 (ArcMeta)
  >>>>>>> REPLACE
  ```

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/MainWindow.cpp` (窗口标题与自定义 Label)、`src/core/CacheManager.h/cpp` (注释)、`src/ui/StyleLibrary.h` (注释)。

**明确禁止越界修改的范围：**
- [ ] **严禁触碰或修改任何历史存档及参考文件夹**，包括：`FERREX-Rust-原版/`、`旧版本-X/`、`mainwindowUI参数/`、`程序崩溃日志/` 目录。
- [ ] 严禁修改资源文件名 `resources/app_icon.ico` 及 CMake 配置文件中的第三方引用描述，确保编译纯净。

## 6. 实现准则与预警【核心】

1. **头文件依赖与编译预警**：
   * 本次修改仅是纯文本/字符串替换，完全不引入、不修改任何外部库或未定义的 Qt 对象，零编译风险。
2. **文本一致性保障**：
   * 在进行替换时，应完全注意大小写匹配，确保除注释外，代码中的字符常量完全匹配 `"ArcMeta"`。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 品牌橙色定义 | 品牌色 (BrandOrange) 物理色值为 `#cb7208`（仅用于品牌 Logo 及名称文字） | ✅ 符合。本次仅修改 StyleLibrary.h 中常量的说明注释，完全不触动 `#cb7208` 这一物理色值常量，确保核心品牌纯正。 |
| Jules 语言规范 | 全程使用中文，不允许使用英文答复用户。 | ✅ 符合。 |

## 8. 待确认事项（可选）
* **关于 FERREX.txt 描述文本**：
  在项目根目录下存在一个名为 `FERREX.txt` 的纯文本文档（内容仅一行 `FERREX`），我们推测其可能属于历史残留或者是辅助打包标识。由于不属于活跃编译代码源，我们建议保留不动，若用户有需要，可在后续阶段统一删除。
