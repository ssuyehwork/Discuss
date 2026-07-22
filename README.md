# 备份备注

**备份时间**：2026-07-22 19:38:23  
**备份目录**：Buk_20260722_193817  

---

彻底物理删除了 ContentPanel 中原有的 Ctrl+滚轮键 缩放/切换视图的逻辑。具体修改了 ContentPanel::eventFilter 中针对 QEvent::Wheel 的 Qt::ControlModifier 的拦截代码块，使其不再拦截该事件；同时修改了 ContentPanel::wheelEvent，删除了其内针对 Qt::ControlModifier 的判断和复杂分流代码，使其直接退化为仅调用基类的原生滚轮事件。这能有效提升程序运行与交互的稳定性，避免非预期的视图缩放和卡顿。
