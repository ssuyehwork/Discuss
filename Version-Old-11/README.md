# 备份备注

**备份时间**：2026-10-06 12:54:38  
**备份目录**：Buk_20261006_125434  

---

完成了列表视图 (ListView / DropTreeView) 的新增“创建日期”列以及表头自定义排序指示器绘制与高亮逻辑。

具体包含：
1. `ModelContract.h` 中添加 `FileListColumn::CreatedDate = 7`，更新 `Count = 8`；
2. `DiskItemModel.cpp` 中新增创建日期的表头名称与格式化显示逻辑；
3. `DropTreeView.h/cpp` 中实现了表头排序列高亮亮白文本与自定义绘制升降序三角箭头的逻辑，并新增了创建日期列的响应式宽列策略（宽度 >= 850px 显示）；
4. `ContentSortController.h/cpp` 实现了 `SortType` 与 `FileListColumn` 的双向转换 SSOT 静态工具函数；
5. `ContentPanel.cpp` 统一使用转换工具函数，并在初始化时同步排序列指示器。
