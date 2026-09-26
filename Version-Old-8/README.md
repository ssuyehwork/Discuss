# 备份备注

**备份时间**：2026-09-18 19:04:28  
**备份目录**：Buk_20260918_190425  

---

在 ContentContextMenu 中，将右键菜单置顶（Pin）逻辑重构为通过 CoreEngine 提交 AppCommandType::SetPinned 命令，彻底归一化至 Command 管线，恢复撤销/重做支持与跨视图 0ms 事件广播。
