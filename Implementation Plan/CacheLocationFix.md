# Implementation Plan - 缩略图与缓存存储路径一致性修复方案 (CacheLocationFix.md)

- **Base Commit Hash**: 064a8b932918e95757175b4cdbee5915839e14fa
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## Chapter 1: Overview

### 问题现象与根因分析
在 `CacheLocationDialog`（缩略图与缓存位置设置对话框）中选择新路径并保存（或进行数据迁移）后，系统会出现以下严重逻辑漏洞与路径不一致问题：
1. **缩略图缓存全盘失效**：用户将缓存目录迁移至新路径 `newDir` 后，`CacheLocationDialog` 将物理文件复制到 `newDir/disk_thumbs` 等子目录下。但 `AppConfig::getGlobalCacheDir()` 在读取 `Cache/CustomLocation` 时，硬编码多追加了一层 `+ "/cache"`，导致 `DiskMediaExtractor` 和 `QuickLookWindow` 去 `newDir/cache/disk_thumbs` 查找缓存。路径不匹配引发缓存全盘未命中，已迁移数据无法读取，必须重新解构提取缩略图。
2. **路径无限嵌套**：每次再次打开 `CacheLocationDialog` 并保存，路径都会被多追加一层 `/cache`，导致路径变成 `.../cache/cache`（如用户截图中的多重嵌套现象）。

### 重构修复设计目标
消除 `AppConfig::getGlobalCacheDir()` 中硬编码追加的 `+ "/cache"` 后缀，使自定义缓存路径直接等于 `Cache/CustomLocation`。确保“用户设置路径”、“数据迁移目标路径”、“缩略图与预览图存取路径”三者 100% 保持完全统一。

---

## Chapter 2: Modified Files List

- `src/core/AppConfig.h` (修改：移除 `getGlobalCacheDir()` 中的 `+ "/cache"` 硬编码后缀)

---

## Chapter 3: Detailed Line-by-Line Changes

### 1. `src/core/AppConfig.h`

```
<<<<<<< SEARCH
    QString getGlobalCacheDir() const {
        QMutexLocker lock(&m_mutex);
        QString customPath = m_settings.value("Cache/CustomLocation").toString();
        if (!customPath.isEmpty() && QDir(customPath).exists()) {
            return customPath + "/cache";
        }
        return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    }
=======
    QString getGlobalCacheDir() const {
        QMutexLocker lock(&m_mutex);
        QString customPath = m_settings.value("Cache/CustomLocation").toString();
        if (!customPath.isEmpty() && QDir(customPath).exists()) {
            return customPath;
        }
        return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps

### 构建步骤
1. 运行 CMake 配置：
   ```bash
   cmake -B build -S . -G "Ninja"
   ```
2. 编译整个项目：
   ```bash
   cmake --build build
   ```

### 运行时验收标准
1. **路径一致性验证**：打开“缩略图与缓存位置设置”对话框，更改缓存目录至自定义位置（如 `D:/CustomCache`）并勾选迁移数据后保存。
2. **缓存读取验证**：确认 `DiskMediaExtractor` 能够直接从 `D:/CustomCache/disk_thumbs` 中正确读取并显示缩略图，不再产生额外的 `/cache/cache` 嵌套目录。
3. **二次打开设置验证**：再次打开“缩略图与缓存位置设置”对话框，确认显示的路径仍为 `D:/CustomCache`，无多余 `/cache` 后缀增加。

---

## Chapter 5: Official API Reuse & Anti-Redundancy

- 统一复用 `AppConfig::getGlobalCacheDir()` 作为全局唯一获取缓存根目录的 SSOT 接口。
- 消除任何在 `getGlobalCacheDir()` 外部或内部另起炉灶进行路径二次拼接硬编码的行为。

---

## Chapter 6: Data SSOT Declaration Table

| 数据项 | 唯一 SSOT 数据源 | 派生副本 / 调用方 | 同步与刷新方向 |
| :--- | :--- | :--- | :--- |
| **自定义缓存存储根路径** | `AppConfig` 中的 `Cache/CustomLocation` | `CacheLocationDialog`, `DiskMediaExtractor`, `QuickLookWindow` | 由 `CacheLocationDialog` 写入，各提取服务只读读取 |

---

## Chapter 7: Interface & Visibility Consistency Check

- **`AppConfig::getGlobalCacheDir()`**:
  - 声明与实现位置：`src/core/AppConfig.h`
  - 函数签名：`QString getGlobalCacheDir() const`
  - 访问级别：`public`
  - 接口变动：签名未改变，仅修改内部实现逻辑，不破坏外部调用契约。

---

## Chapter 8: Header Chain & Inclusion Closure Check

- 本次修改仅触及 `src/core/AppConfig.h` 方法体内部返回值，未修改或删除任何 `#include` 包含指令。头文件闭合性完备。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 检查项 | 是否满足 | 说明 |
| :--- | :--- | :--- |
| 1. 唯一 SSOT 数据源声明 | 满足 | 统一归口于 `AppConfig` 的 `Cache/CustomLocation` |
| 2. 物理签名未破坏 | 满足 | `getGlobalCacheDir()` 签名保持不变 |
| 3. 无 Win32 / Qt Popup 恶意 Hack | 满足 | 纯 C++/Qt 逻辑修复 |
| 4. 无跨线程操作 UI | 满足 | `getGlobalCacheDir()` 带有 `QMutexLocker` 保证线程安全 |
| 5. 零篡改既有数值 | 满足 | 保持既有 UI 与配置参数原样 |
| 6. CMake 注册全覆盖 | 满足 | 未新增/删除文件，现有 CMakeLists.txt 包含 `AppConfig.h` |
| 7. SEARCH 块匹配唯一性 | 满足 | SEARCH 块在 `src/core/AppConfig.h` 中精准唯一匹配 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table

- **交互与行为保留**：缓存设置窗口的弹窗、浏览、迁移复选框、保存与恢复默认逻辑保持完全一致。
- **数值零篡改**：窗口尺寸（520x240）、边距（20,20,20,20）、间距（15）等所有 UI 样式参数 100% 保持不变。
