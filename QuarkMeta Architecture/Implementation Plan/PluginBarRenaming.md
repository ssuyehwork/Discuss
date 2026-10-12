# Implementation Plan: Rename DriveBar to PluginBar Tooltip (PluginBarRenaming.md)

## 1. Overview
本实施方案旨在对“盘符管理栏”进行概念与提示词升级。该区域已重构为面向非常用设置与拓展小工具（如“标签管理”等）的“插件栏（Plugin / Extension Bar）”。

本次修改将标题栏中折叠控制按钮的悬浮提示词（Tooltip）由 `"展开/收起盘符管理栏"` 精准更名为 **`"展开/收起插件栏"`**。

---

## 2. Modified Files List
* `src/ui/TitleBarWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### File: `src/ui/TitleBarWidget.cpp`

<<<<<<< SEARCH
    m_btnToggleDriveBar = createTitleBtn("chevrons_down", "展开/收起盘符管理栏");
=======
    m_btnToggleDriveBar = createTitleBtn("chevrons_down", "展开/收起插件栏");
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### 构建步骤
使用 CMake 增量编译项目：
```bash
cmake --build build --config Debug --target QuarkMeta
```

### 验证步骤
1. 启动 QuarkMeta 应用程序；
2. 将鼠标悬停在标题栏折叠按钮（双下箭头 `chevrons_down`）上方；
3. 检查悬浮提示词（Tooltip Overlay）是否正确显示为 **`"展开/收起插件栏"`**；
4. 点击按钮切换折叠/展开，确认显隐功能与图标切换逻辑保持 100% 正常。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

| 自查项 | 结论 | 说明 |
| :--- | :--- | :--- |
| **是否复用既有 SSOT 通道？** | **是** | 完全复用既有 `TitleBarWidget` 与 `m_btnToggleDriveBar` 信号槽体系 |
| **是否存在另起炉灶行为？** | **否** | 仅微调文案，零结构冗余 |

---

## 6. Header API Signature Verification

| 被调用的类/函数 | `.h` 文件真实物理签名 | 核查状态 |
| :--- | :--- | :--- |
| `TitleBarWidget::createTitleBtn` | 在 `TitleBarWidget.cpp` 内部定义为 lambda 闭包，处理设置 Tooltip 属性 | 本地闭包，匹配 |

---

## 7. Header Inclusion Chain & Type Completeness Check

| 修改文件 | 新增/引用的类与枚举 | 头文件包含状态 |
| :--- | :--- | :--- |
| `TitleBarWidget.cpp` | `QPushButton` | 头文件与包含链完整 |
