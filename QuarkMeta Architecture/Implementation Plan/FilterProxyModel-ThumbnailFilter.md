# Implementation Plan - FilterProxyModel-ThumbnailFilter.md

## 1. Overview

### 架构三问回答
1. **第一问（真理源溯源 SSOT）**：
   - 过滤条件 DTO（`FilterState`）由 `FilterStateModel` / `FilterPanel` 产生，作为筛选条件的唯一依据。
   - 本次修改在代理模型 `FilterProxyModel` 的 `filterAcceptsRow` 中真正响应并执行 `FilterState::thumbnailPresence` 条件判别，补充缺失的过滤分支，未引入额外状态。
2. **第二问（黑盒完整性）**：
   - 完全利用 `FilterProxyModel` 原有的 `filterAcceptsRow` 虚函数与 `ItemRecord` DTO 进行判断。
   - 未破坏封装、未使用友元或私有暴露，符合 Clean Architecture。
3. **第三问（根因 vs 症状）**：
   - **根因分析**：`FilterPanel` 侧已将 `thumbnailPresence` 写入 `FilterState`，但底层的代理模型 `FilterProxyModel::filterAcceptsRow` 压根没有编写针对 `currentFilter.thumbnailPresence` 的分支判别代码，导致筛选逻辑静默失效。
   - **解决方案**：在 `FilterProxyModel::filterAcceptsRow` 的第 6 节（附加属性过滤）中，补全对 `thumbnailPresence`（`HasThumbnail` 与 `NoThumbnail`）的过滤判定分支。

---

## 2. Modified Files List
- `src/ui/models/FilterProxyModel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File: `src/ui/models/FilterProxyModel.cpp`

```
<<<<<<< SEARCH
    if (currentFilter.duplicatePresence != FilterState::DupAll) {
        if (record.isDir) return false;
        bool isDuplicate = m_cachedDuplicatePaths.contains(record.path);
        if (currentFilter.duplicatePresence == FilterState::DuplicateOnly && !isDuplicate) return false;
        if (currentFilter.duplicatePresence == FilterState::UniqueOnly && isDuplicate) return false;
    }

    // 7. 搜索关键词匹配
=======
    if (currentFilter.duplicatePresence != FilterState::DupAll) {
        if (record.isDir) return false;
        bool isDuplicate = m_cachedDuplicatePaths.contains(record.path);
        if (currentFilter.duplicatePresence == FilterState::DuplicateOnly && !isDuplicate) return false;
        if (currentFilter.duplicatePresence == FilterState::UniqueOnly && isDuplicate) return false;
    }

    if (currentFilter.thumbnailPresence != FilterState::ThumbAll) {
        if (record.isDir) return false;
        static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
        QString ext = record.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext) || (record.width > 0 && record.height > 0);
        bool hasThumb = (record.thumbStatus != 1) && isGraphic && !iconOnlyExts.contains(ext);

        if (currentFilter.thumbnailPresence == FilterState::HasThumbnail && !hasThumb) return false;
        if (currentFilter.thumbnailPresence == FilterState::NoThumbnail && (hasThumb || record.thumbStatus != 1 || !isGraphic)) return false;
    }

    // 7. 搜索关键词匹配
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### 构建步骤
在项目根目录运行 CMake 构建命令：
```bash
cmake -B build -S .
cmake --build build --config Debug
```

### 验证方法
1. **“有缩略图”筛选测试**：
   - 打开包含正常图片与损坏/无缩略图文件的文件夹；
   - 在右侧 FilterPanel 勾选“缩略图状态”下的 **“有缩略图”**；
   - 观察视图是否仅留存能正常展示缩略图的图形图像文件。
2. **“无缩略图 (提取失败)”筛选测试**：
   - 勾选 **“无缩略图 (提取失败)”**；
   - 观察视图是否精准筛选出 `thumbStatus == 1` 的异常图片；
   - 右键选中该文件点击“重新提取缩略图”，修复成功后观察该文件是否自动从该筛选结果中消失并回归“有缩略图”。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- [x] **复用性检查**：复用 `UiHelper::isGraphicsFile` 与 `record.thumbStatus` 既有判定字段，无另起炉灶。
- [x] **零参数篡改**：完全照抄 `ContentStatsWorker` 中关于 `hasThumbnailCount` 与 `noThumbnailCount` 的一致判别逻辑，符合《Zero-Value-Alteration Contract》。

---

## 6. Header API Signature Verification

| 调用的成员/类 | 头文件物理声明源 | 头文件物理精确签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `UiHelper::isGraphicsFile` | `src/ui/UiHelper.h` | `static inline bool isGraphicsFile(const QString& ext)` | 物理核实一致 |
| `FilterState::thumbnailPresence` | `src/ui/FilterStateModel.h` | `ThumbnailPresence thumbnailPresence` | 物理核实一致 |
