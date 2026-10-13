# Implementation Plan - MetadataManager-EnsureActivated-Fix.md

## Overview
本实施方案解决磁盘 `.QuarkMeta.json` 文件中已存在用户元数据（如星级、颜色、标签、备注、宽高），但在界面 UI 上却显示为 0 颗星与全空标签的核心缺陷：

- **根因分析**：当 UI 首次请求某个路径的元数据（如 `MetadataManager::instance().getMeta(path)`）时，系统会调用 `MetadataManager::ensureActivated(nPath)`。在现存代码中，`ensureActivated` 在将路径放入 `MetaMemoryCache` 内存缓存时，仅调用了 OS API 获取了文件大小及创建/修改时间，**遗漏了从 `QuarkMetaJsonStore::instance().readItemMeta` 读取 `.QuarkMeta.json` 中保存的星级、颜色标记、标签、备注等用户元数据**。导致 `MetaMemoryCache` 被误初始化为全空默认对象，后续所有 UI 视图查询均命中此全空内存缓存，造成界面元数据丢失。

- **修复对策**：在 `MetadataManager::ensureActivated` 初始化 `RuntimeMeta` 结构体时，显式调用 `QuarkMetaJsonStore::instance().readItemMeta(nPath, jsonMeta)`，将磁盘隐藏配置文件中的星级、颜色、标签、备注、加密状态、尺寸及调色板等字段完整装载至 `RuntimeMeta` 并写入 `MetaMemoryCache`。

---

## Modified Files List
1. `src/meta/MetadataManager.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/meta/MetadataManager.cpp`
在 `MetadataManager::ensureActivated` 中补齐 `QuarkMetaJsonStore::instance().readItemMeta` 读取装载逻辑：

```
<<<<<<< SEARCH
    if (success) {
        MetaMemoryCache::instance().put(nPath, rm);
    }
=======
    if (success) {
        ItemMeta jsonMeta;
        if (QuarkMetaJsonStore::instance().readItemMeta(nPath, jsonMeta)) {
            rm.rating = jsonMeta.rating;
            rm.manualColor = jsonMeta.color;
            rm.pinned = jsonMeta.pinned;
            rm.note = jsonMeta.note;
            rm.url = jsonMeta.url;
            rm.encrypted = jsonMeta.encrypted;
            rm.width = jsonMeta.width;
            rm.height = jsonMeta.height;
            rm.autoColor = jsonMeta.autoColor;
            rm.thumbStatus = jsonMeta.thumbStatus;
            rm.added_at = jsonMeta.addedAt;
            rm.tags.clear();
            for (const auto& t : jsonMeta.tags) {
                rm.tags.append(QString::fromStdWString(t));
            }
            rm.palettes = jsonMeta.palettes;
        }
        MetaMemoryCache::instance().put(nPath, rm);
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **编译确认**：
   运行 CMake 编译，确保无语法与符号未定义错误：
   ```bash
   cmake --build build
   ```

2. **功能复核**：
   - 步骤 1：准备一个 `.QuarkMeta.json` 中保存有 4 星及“测试”标签的文件 `search-in-library.svg`；
   - 步骤 2：在应用中打开该文件所在的目录；
   - 步骤 3：查看视图内容区与右侧属性栏，验证星级精准显示为 4 星，标签精准显示为“测试”，与磁盘 `.QuarkMeta.json` 物理数据 100% 吻合一致。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **真理源闭环**：在内存缓存激活的源头 `ensureActivated` 中补齐与 JSON 存储源 `QuarkMetaJsonStore` 的数据桥梁；
- **防另起炉灶**：无私有或平行数据管道，完全复用现有 `readItemMeta` 接口。

---

## Header API Signature Verification

| 调用的类 / 成员函数 | `.h` 文件源头 | 物理精确签名 |
| :--- | :--- | :--- |
| `QuarkMetaJsonStore::instance()` | `src/meta/QuarkMetaJsonStore.h` | `static QuarkMetaJsonStore& instance();` |
| `QuarkMetaJsonStore::readItemMeta(...)` | `src/meta/QuarkMetaJsonStore.h` | `bool readItemMeta(const std::wstring& filePath, ItemMeta& outMeta);` |

---

## Header Inclusion Chain & Type Completeness Check

- `src/meta/MetadataManager.cpp`:
  - 已显式包含了 `#include "QuarkMetaJsonStore.h"`，类型完整闭环。
