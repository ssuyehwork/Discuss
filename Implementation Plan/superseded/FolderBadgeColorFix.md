# Implementation Plan - FolderBadgeColorFix

Base Commit Hash: `943037f85524b65875859191f2d3c1dba8aae1c1`
Prerequisite Plans: 无
Supersedes: 无

---

### 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/util/ColorPaletteEngine.h`
   - `src/util/ColorPaletteEngine.cpp`
   - `src/ui/CardPainterHelper.cpp`
   - `src/ui/ThumbnailDelegate.cpp`
2. **头文件原样签名**：
   - `src/util/ColorPaletteEngine.h`: `static QPair<QColor, QColor> getExtensionBadgeColors(const QString& ext);`
3. **既有 API 检索日志**：
   - `grep -rn "getExtensionBadgeColors" src/`
   - 结果：`src/util/ColorPaletteEngine.cpp:76` 定义了 `getExtensionBadgeColors`，内部为 `psd`, `psb`, `eps`, `ai` 提供独占硬编码调色对。

---

## Chapter 1: Overview
在 QuarkMeta 卡片视图中，卡片左上角的扩展名徽章（Extension Badge）用于标识文件类型或文件夹。文件夹类型对应的扩展名标识字符为 `"dir"`（绘制时显示为 `"DIR"`）。原本的设计理念是不同扩展名的徽章背景颜色不可重复。然而，由于动态 Hash 颜色计算逻辑中 `qHash("dir")` 与 `qHash("md")` 计算得出的 HSL 调色参数发生碰撞，导致文件夹（DIR）与 Markdown（MD）文件拥有相同的背景颜色。

本方案在 `ColorPaletteEngine::getExtensionBadgeColors` 的“独占硬编码保护项”中，为文件夹（`"dir"`）增设专属独占调色对（深色背景 `#1C2D37`，醒目青蓝色文字 `#38BDF8`），彻底消除与 `MD` 等其它文件类型的颜色碰撞，确保文件夹徽章颜色的绝对唯一性与高辨识度。

---

## Chapter 2: Modified Files List
- `src/util/ColorPaletteEngine.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes

### `src/util/ColorPaletteEngine.cpp`
<<<<<<< SEARCH
    // 1. 独占硬编码保护项
    if (e == "psd" || e == "psb") return { QColor("#001D26"), QColor("#02B1DD") };
    if (e == "eps")               return { QColor("#35483D"), QColor("#F88025") };
    if (e == "ai")                return { QColor("#F88025"), QColor("#35483D") };
=======
    // 1. 独占硬编码保护项
    if (e == "dir" || e == "folder") return { QColor("#1C2D37"), QColor("#38BDF8") };
    if (e == "psd" || e == "psb")    return { QColor("#001D26"), QColor("#02B1DD") };
    if (e == "eps")                  return { QColor("#35483D"), QColor("#F88025") };
    if (e == "ai")                   return { QColor("#F88025"), QColor("#35483D") };
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps
1. **构建步骤**：
   在终端执行 `cmake --build build` 或使用项目默认 CMake 构建脚本进行编译。
2. **运行时验收标准**：
   - 启动 QuarkMeta，进入网格（Grid）视图。
   - 观察文件夹（DIR）卡片左上角徽章，背景为专属深青色（`#1C2D37`），文字为醒目亮青色（`#38BDF8`）。
   - 观察 Markdown（.md）文件卡片左上角徽章，背景颜色与文件夹颜色有显著区分，不再发生重复。

---

## Chapter 5: Official API Reuse & Anti-Redundancy
- 复用了既有 `ColorPaletteEngine::getExtensionBadgeColors(const QString& ext)` 独占保护项机制，未新增冗余函数或重复逻辑。

---

## Chapter 6: Data SSOT Declaration Table
| 数据项 | 唯一 SSOT 源 | 派生/缓存副本 | 同步方向 |
| :--- | :--- | :--- | :--- |
| 扩展名徽章颜色 pair | `ColorPaletteEngine::getExtensionBadgeColors` | `s_colorCache` (内存缓存) / `extension_colors` (数据库) | `getExtensionBadgeColors` -> `s_colorCache` -> UI Painter |

---

## Chapter 7: Interface & Visibility Consistency Check
- 外部调用接口：`ColorPaletteEngine::getExtensionBadgeColors(const QString& ext)`
  - 声明位置：`src/util/ColorPaletteEngine.h:33`
  - 访问权限：`public static`
- 本方案未修改任何头文件 API 签名与访问级别。

---

## Chapter 8: Header Chain & Inclusion Closure Check
- 修改仅涉及 `src/util/ColorPaletteEngine.cpp` 内部判断逻辑，未增加或删除任何 `#include` 包含指令。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 序号 | 检查项目 | 结果 | 位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否只修改实施方案（禁止直接动源码） | 满足 | 仅创建 `Implementation Plan/FolderBadgeColorFix.md` |
| 2 | SEARCH 块是否物理匹配且唯一 | 满足 | `src/util/ColorPaletteEngine.cpp` 独占硬编码保护项 |
| 3 | 是否破坏冻结的 .h 公开 API 签名 | 满足 | 零修改 .h |
| 4 | 是否包含 CMake 文件注册变更 | 不涉及 | 未新建或删除 C++ 源文件 |
| 5 | 是否遵循单向依赖流 | 满足 | Domain/Util 层未依赖 UI 层 |
| 6 | 是否零篡改既有设计数值（非修复项） | 满足 | 仅调整缺陷文件夹配色，无随手微调其它属性 |
| 7 | 是否包含完整 10 大章节 | 满足 | 包含 Chapter 1 至 Chapter 10 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table
- **行为与交互**：网格与列表视图卡片绘制、缩略图加载与双击导航交互保持 100% 原样。
- **扩展名徽章配色**：仅对发生重复碰撞的文件夹（DIR）徽章赋予专属物理独占色，PSD/AI/EPS 及其他动态扩展名配色保持 100% 相同。
