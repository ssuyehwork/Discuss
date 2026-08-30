# 备份备注

**备份时间**：2026-08-30 14:19:53  
**备份目录**：Buk_20260830_141950  

---

feat(ui): 在列表视图中为空文件夹的 1:1 方形图标槽绘制青色虚线边框

更新 TreeItemDelegate.h，在 squareRect 上为空文件夹渲染 #41F2F2 的 Qt::DashLine 圆角矩形

统一空文件夹的视觉指示器，使其在 List View 与 Grid View 中保持一致
