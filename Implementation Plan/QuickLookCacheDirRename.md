# Implementation Plan - 快速预览缓存目录名从 quicklook_previews_v2 改回 quicklook_previews (QuickLookCacheDirRename.md)

- **Base Commit Hash**: 064a8b932918e95757175b4cdbee5915839e14fa
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## Chapter 1: Overview

### 需求与重构目标
将 QuarkMeta 中空格键 QuickLook 快速预览的高清大图缓存目录路径名从 `quicklook_previews_v2` 改回 `quicklook_previews`：
1. **全局修改**：将全局唯一出现的 `quicklook_previews_v2` 替换为 `quicklook_previews`。
2. **规则零变动**：SHA256 哈希命名规则、两级分桶策略（前2位/第3-4位）、PNG 格式、修改时间比对、高清保存条件等所有缓存读写与比对逻辑 100% 保持原样不动。
3. **注释一致性**：`src/ui/QuickLookWindow.cpp` 中的路径代码与已有的 `quicklook_previews` 注释保持 100% 完全一致。

---

## Chapter 2: Modified Files List

- `src/ui/QuickLookWindow.cpp` (修改 1 处：第 285 行缓存根路径字符串)

---

## Chapter 3: Detailed Line-by-Line Changes

### 1. `src/ui/QuickLookWindow.cpp`

```
<<<<<<< SEARCH
    QString baseCacheDir = AppConfig::instance().getGlobalCacheDir() + "/quicklook_previews_v2";
=======
    QString baseCacheDir = AppConfig::instance().getGlobalCacheDir() + "/quicklook_previews";
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps

1. **构建验证**：运行 `cmake --build build` 确保无语法或编译错误。
2. **搜索验证**：运行 `grep -rn "quicklook_previews_v2" .` 确认全局命中数为 0。

---

## Chapter 5: Official API Reuse & Anti-Redundancy

- 统一复用 `AppConfig::instance().getGlobalCacheDir()` 接口作为全局缓存根路径。

---

## Chapter 6: Data SSOT Declaration Table

| 数据类型 | 缓存根目录路径 | 管理类 |
| :--- | :--- | :--- |
| **QuickLook 高清大图预览缓存** | `<GlobalCacheDir>/quicklook_previews` | `QuickLookWindow` |

---

## Chapter 7: Interface & Visibility Consistency Check

- 修改仅触及 `QuickLookWindow.cpp` 函数体内部变量定义，未改变任何类结构或函数签名。

---

## Chapter 8: Header Chain & Inclusion Closure Check

- 修改仅发生在 `.cpp` 内部，未变动任何 `#include` 包含链。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 检查项 | 是否满足 | 说明 |
| :--- | :--- | :--- |
| 1. 命中文件唯一性 | 满足 | 全局仅 `src/ui/QuickLookWindow.cpp` 第 285 行 1 处命中 |
| 2. 源代码未直接修改 | 满足 | 实施方案规范放置于 `Implementation Plan/` 目录 |
| 3. 无多余清理或二次变动 | 满足 | 不清理旧物理目录，不改动 `disk_thumbs` 逻辑 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table

- 保留 SHA256 散列规则与 2048px 预览缓存全套逻辑，零参数或数值篡改。
