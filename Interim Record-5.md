# 全工程第三轮循环物理符号与悬空单例 API 深度比对记录 (Interim Record-5.md)

## 一、 第三轮深度排查维度
本轮针对 `MetadataManager` 等全局核心单例中的公有 API 在全工程的外部调用覆盖率（External Calling Coverage）展开了第 3 轮深度的静态符号图谱扫描。

---

## 二、 重点排查事实清单

### 1. `MetadataManager` 悬空单例 API (Legacy Memory-Mode / Ingestion APIs)
以下公有方法仅在 `src/meta/MetadataManager.h` 中声明、并在 `MetadataManager.cpp` 中有函数体，但在全工程其他任何 UI 或 Core 模块中均 **0 次外部调用**，确为托管库/内存模式时代遗留的悬空 API：
1. **`markAsIngested`**: 旧版导入登记 API，全系统 0 次外部调用。
2. **`getFolderIdsByName` / `getSubFolderIdsByName` / `getFolderIdsByExtension`**: 旧版分类与 `folder_id` 映射查询 API，全系统 0 次外部调用。
3. **`getPathByFolderId` / `getVolumeFromFolderId`**: 旧版基于 `folder_id` 的路径解析 API，全系统 0 次外部调用。
4. **`syncPhysicalMetadata` / `syncAfterMove`**: 旧版全盘物理元数据同步对账 API，全系统 0 次外部调用。
5. **`registerQuarkMetaFrn` / `registerItemsAsync`**: 旧版 USN/FRN 自动扫描导入 API，全系统 0 次外部调用。
6. **`loadVolumeNameCache` / `unloadVolumeNameCache`**: 旧版盘符卷名内存缓存 API，全系统 0 次外部调用。
7. **`getProgressFromDb`**: 旧版 `system_stats` 解析进度查询 API，全系统 0 次外部调用。
8. **`slideRecentWindow`**: 旧版最近访问滑动视口 API，全系统 0 次外部调用。

---

## 三、 审计结论
第 3 轮循环深排揭示：`src/meta/MetadataManager` 作为历史核心单例，在转向纯磁盘独立模式后，依然残留着 30 余个托管库时代（`folder_id`、FRN 对账、`system_stats` 进度）的废弃公有 API，这些接口在全项目中已处于完全悬空、零外部调用的僵尸状态。
