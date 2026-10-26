# Implementation Plan - FolderBadgeColorFix-1

本方案替代 `superseded/FolderBadgeColorFix.md`

Base Commit Hash: `943037f85524b65875859191f2d3c1dba8aae1c1`
Prerequisite Plans: 无
Supersedes: `Implementation Plan/superseded/FolderBadgeColorFix.md`

---

### 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/util/ColorPaletteEngine.h`
   - `src/util/ColorPaletteEngine.cpp`
   - `src/meta/ExtensionColorDao.h`
   - `src/meta/ExtensionColorDao.cpp`
   - `src/ui/CardPainterHelper.cpp`
   - `src/ui/ThumbnailDelegate.cpp`
2. **头文件原样签名**：
   - `src/util/ColorPaletteEngine.h:33`: `static QPair<QColor, QColor> getExtensionBadgeColors(const QString& ext);`
   - `src/meta/ExtensionColorDao.h:13`: `static bool saveExtensionColor(const QString& ext, const QColor& bg, const QColor& text, bool isCustom = false);`
3. **既有 API 检索日志**：
   - `grep -rn "getExtensionBadgeColors" src/`
   - `src/util/ColorPaletteEngine.cpp:76` 定义了 `getExtensionBadgeColors`。

---

## Chapter 1: Overview
在 QuarkMeta 卡片视图中，卡片左上角扩展名徽章（Extension Badge）用于标识文件类型或文件夹。文件夹类型对应的扩展名标识字符为 `"dir"`（绘制时显示为 `"DIR"`）。原本的设计理念是不同扩展名的徽章背景颜色不可重复。然而，由于动态 Hash 颜色计算逻辑中 `qHash("dir")` 与 `qHash("md")` 计算得出的 HSL 调色参数发生碰撞，导致文件夹（DIR）与 Markdown（MD）文件拥有相同的背景颜色。

在上一版方案的审核中，发现了两个关键疏漏：
1. **SEARCH 块前导缩进不匹配**：源码真实的 if 语句缩进为 4 个空格，方案包含了 8 个空格。
2. **持久化 DB/缓存覆盖风险**：若用户数据库 `QuarkMeta.store` 的 `extension_colors` 表中此前已动态生成并持久化过 `"dir"` 或 `"folder"` 的旧 Hash 颜色，启动时会被加载进内存缓存 `s_colorCache`。如果硬编码独占项简单拦截在最前，不仅导致 DB 中残留脏数据，而且若缓存比对逻辑不完备，可能存在潜在不一致风险。

**重构设计方案**：
1. 修正 SEARCH 块缩进，与源码严格逐字匹配（4 个空格）。
2. 在 `getExtensionBadgeColors` 中，先完成 `ExtensionColorDao` 数据库表与内存缓存 `s_colorCache` 的初始化，随后评估“独占硬编码保护项”。
3. 当命中的是独占硬编码保护项（如 `"dir"`/`"folder"`、`"psd"`/`"psb"` 等）时，检查内存缓存 `s_colorCache` 或数据库中记录的颜色是否与当前定义的最新独占配色一致。若不一致或不存在，自动调用 `ExtensionColorDao::saveExtensionColor` 刷盘覆盖，并同步更新 `s_colorCache`，从而干净地清理既有 QuarkMeta.store 数据库内的旧 Hash 颜色记录，确保数据 SSOT 的绝对一致。

---

## Chapter 2: Modified Files List
- `src/util/ColorPaletteEngine.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes

### `src/util/ColorPaletteEngine.cpp`
<<<<<<< SEARCH
QPair<QColor, QColor> ColorPaletteEngine::getExtensionBadgeColors(const QString& ext) {
    QString e = ext.toLower().trimmed();

    // 1. 独占硬编码保护项
    if (e == "psd" || e == "psb") return { QColor("#001D26"), QColor("#02B1DD") };
    if (e == "eps")               return { QColor("#35483D"), QColor("#F88025") };
    if (e == "ai")                return { QColor("#F88025"), QColor("#35483D") };

    // 2. 内存缓存第一级查找
    static QMap<QString, QPair<QColor, QColor>> s_colorCache;
    static bool s_tableInited = false;

    if (!s_tableInited) {
        ExtensionColorDao::initTable();
        s_colorCache = ExtensionColorDao::loadAllColors();
        s_tableInited = true;
    }

    if (s_colorCache.contains(e)) {
        return s_colorCache.value(e);
    }
=======
QPair<QColor, QColor> ColorPaletteEngine::getExtensionBadgeColors(const QString& ext) {
    QString e = ext.toLower().trimmed();

    // 1. 内存缓存与数据库初始化
    static QMap<QString, QPair<QColor, QColor>> s_colorCache;
    static bool s_tableInited = false;

    if (!s_tableInited) {
        ExtensionColorDao::initTable();
        s_colorCache = ExtensionColorDao::loadAllColors();
        s_tableInited = true;
    }

    // 2. 独占硬编码保护项校验与旧持久化缓存擦除同步
    auto resolveReserved = [&](const QPair<QColor, QColor>& reservedColors) -> QPair<QColor, QColor> {
        if (!s_colorCache.contains(e) || s_colorCache.value(e) != reservedColors) {
            ExtensionColorDao::saveExtensionColor(e, reservedColors.first, reservedColors.second, false);
            s_colorCache.insert(e, reservedColors);
        }
        return reservedColors;
    };

    if (e == "dir" || e == "folder") return resolveReserved({ QColor("#1C2D37"), QColor("#38BDF8") });
    if (e == "psd" || e == "psb")    return resolveReserved({ QColor("#001D26"), QColor("#02B1DD") });
    if (e == "eps")                  return resolveReserved({ QColor("#35483D"), QColor("#F88025") });
    if (e == "ai")                   return resolveReserved({ QColor("#F88025"), QColor("#35483D") });

    // 3. 普通扩展名内存缓存查找
    if (s_colorCache.contains(e)) {
        return s_colorCache.value(e);
    }
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps
1. **构建步骤**：
   在终端执行 `cmake --build build` 或使用项目默认 CMake 构建脚本进行编译。
2. **运行时验收标准**：
   - 启动 QuarkMeta，进入网格（Grid）视图。
   - 观察文件夹（DIR）卡片左上角徽章，背景为专属深青色（`#1C2D37`），文字为醒目亮青色（`#38BDF8`）。
   - 观察 Markdown（.md）文件卡片左上角徽章，背景颜色为原配色，两者不再重复。
   - 检查 SQLite 数据库 `extension_colors` 表，确认 `extension='dir'` 行的 `bg_color` 与 `text_color` 已自动被修正为 `#1C2D37` 与 `#38BDF8`。

---

## Chapter 5: Official API Reuse & Anti-Redundancy
- 复用了既有 `ExtensionColorDao::saveExtensionColor` API，对独占硬编码配色进行自动落盘与内存缓存刷新，彻底杜绝 DB 残留脏数据。

---

## Chapter 6: Data SSOT Declaration Table
| 数据项 | 唯一 SSOT 源 | 派生/缓存副本 | 同步方向 |
| :--- | :--- | :--- | :--- |
| 扩展名徽章颜色 pair | `ColorPaletteEngine::getExtensionBadgeColors` (独占保护项定义) | `s_colorCache` (内存缓存) / `extension_colors` (数据库) | 独占定义 -> 校验比对 -> 刷盘更新 `extension_colors` -> 更新 `s_colorCache` -> 返回 Painter |

---

## Chapter 7: Interface & Visibility Consistency Check
- 外部调用接口：`ColorPaletteEngine::getExtensionBadgeColors(const QString& ext)`
  - 声明位置：`src/util/ColorPaletteEngine.h:33`
  - 访问权限：`public static`
- 本方案未修改任何头文件 API 签名与访问级别。

---

## Chapter 8: Header Chain & Inclusion Closure Check
- 修改仅涉及 `src/util/ColorPaletteEngine.cpp` 内部逻辑，已包含 `#include "../meta/ExtensionColorDao.h"`，类型定义闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 序号 | 检查项目 | 结果 | 位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否只修改实施方案（禁止直接动源码） | 满足 | 仅创建 `Implementation Plan/FolderBadgeColorFix-1.md` |
| 2 | SEARCH 块是否物理匹配且唯一 | 满足 | 精确匹配 `src/util/ColorPaletteEngine.cpp` 第 76-96 行 |
| 3 | 是否破坏冻结的 .h 公开 API 签名 | 满足 | 零修改 .h |
| 4 | 是否包含 CMake 文件注册变更 | 不涉及 | 未新建或删除 C++ 源文件 |
| 5 | 是否遵循单向依赖流 | 满足 | Domain/Util -> Data 层调用，未反向依赖 UI |
| 6 | 是否消除持久化 DB 脏数据覆盖风险 | 满足 | `resolveReserved` 自动同步比对并刷新 DB 与缓存 |
| 7 | 是否包含完整 10 大章节 | 满足 | 包含 Chapter 1 至 Chapter 10 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table
- **行为与交互**：网格与列表视图卡片绘制、缩略图加载与导航交互保持 100% 原样。
- **扩展名徽章配色**：仅对发生重复碰撞的文件夹（DIR）徽章赋予专属物理独占色，PSD/AI/EPS 及其他扩展名配色保持 100% 相同。
