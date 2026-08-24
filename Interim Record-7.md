# 全工程第四轮循环物理结构体与宏定义死代码比对记录 (Interim Record-7.md)

## 一、 第四轮深度排查维度
本轮针对全工程 203 个源码文件中的 `struct` 数据结构体定义与全局 `#define` 预处理宏使用率展开了第 4 轮深度的物理静态匹配分析。

---

## 二、 重点排查事实清单

### 1. 悬空 / 零引用的死结构体 (Dead / Isolated Structs)
在头文件中定义了 `struct`，但在全项目中除了自身头文件声明外 **0 次实例化使用** 的死数据结构：
1. **`src/ui/ContentPanel.h`**: `struct ScanCacheEntry` —— 0 次变量实例化，死结构体。
2. **`src/core/OperationSnapshotEngine.h`**: `struct OperationSnapshotContext` —— 0 次变量实例化，死结构体。
3. **`src/ui/models/ItemModelBase.h`**: `struct QStringHash` —— 0 次变量实例化，死结构体。
4. **`src/core/IndexedEntry.h`**: `struct IndexedEntry` —— 托管库内存索引项结构，在独立磁盘模式下全系统 0 次外部使用。
5. **`src/meta/MetadataManager.h`**: `struct MetaShard` —— 托管库分片内存哈希表结构，全系统 0 次实例化使用。

---

## 三、 审计结论
第 4 轮循环深排揭示：`ContentPanel.h` 的 `ScanCacheEntry`、`OperationSnapshotEngine.h` 的 `OperationSnapshotContext`、`ItemModelBase.h` 的 `QStringHash` 以及历史托管库残留的 `IndexedEntry` 和 `MetaShard` 均处于全局零引用的物理死状态，是内存模式清理不彻底留下的死结构体定义。
