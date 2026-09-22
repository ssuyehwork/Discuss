# 备份备注

**备份时间**：2026-09-21 20:02:13  
**备份目录**：Buk_20260921_200211  

---

删除了 ColumnBlankCanvasWidget 构造函数中的 setFixedWidth(230)，彻底解除 Qt 对物理几何尺寸设定的硬性夹断约束，使 updatePaneWidths 中的动态弹性宽度能够顺利应用并撑满视口右侧。
