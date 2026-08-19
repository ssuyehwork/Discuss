# 备份备注

**备份时间**：2026-08-19 17:46:57  
**备份目录**：Buk_20260819_174652  

---

更新了 DiskItemModel::loadThumbnailsForRows：

严格限制每批次只处理 2 个条目，并在主线程完成后触发一个 20ms 自驱动链式中继，调用 ContentPanel::refreshVisibleThumbnails。

将 ContentPanel::refreshVisibleThumbnails 改为 public，以便 DiskItemModel 可以访问。

所有后台任务都会验证 generation tokens，在目录导航时能够立即熔断。
