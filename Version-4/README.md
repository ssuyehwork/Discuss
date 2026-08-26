# 备份备注

**备份时间**：2026-08-26 09:42:37  
**备份目录**：Buk_20260826_094232  

---

1. 彻底拔除 UI 主线程中的同步 QuarkMetaJson 读盘与 Shell/缩略图提取阻塞代码，全部改由 Model/内存结构 0 毫秒同步。
2. 废除 MetaPanel::updateControlsState 中的动态 setStyleSheet 调用，避免 Qt Style Polish 重绘风暴。
3. 优化关联网址输入框控件，移除重复 padding 截断并绑定手型光标。
4. 轻量化状态栏选区统计。
