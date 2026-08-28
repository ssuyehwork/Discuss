# 备份备注

**备份时间**：2026-08-28 21:03:24  
**备份目录**：Buk_20260828_210322  

---

已成功修复 Windows 平台下系统托盘右键菜单显示为 Win32 浅色原生框的问题：\n1. 在 ThemeManager::applyMenuStyle 中为传入的 QMenu 显式调用 setStyleSheet(...) 注入暗黑样式，强制 Qt 绕过 Win32 原生 HMENU 机制，走 Qt 自绘 QMenu（背景 #252526，6px 圆角，1px 细边框 #333333）。\n2. 在 TrayController.cpp 中为 m_trayMenu 传入 mainWindow 父指针。\n3. 创建了对应的实施方案 QuarkMeta Architecture/Implementation Plan/ThemeManager-1.md。
