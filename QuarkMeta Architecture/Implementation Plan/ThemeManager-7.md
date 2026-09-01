# Guide & Preference.md 引入与 AGENTS.md 规范引导实施方案

## 1. 核心架构问题回答 (3 Questions)

### Q1: 唯一事实源 (Single Source of Truth, SSOT) 是什么？在哪里？
- **样式定义 SSOT**：全局静态 QSS 样式规则声明的唯一真理源为 `resources/style.qss`。
- **主题控制 SSOT**：全局主题加载、窗口级属性设置（半透明/无边框）及动态覆盖入口的唯一事实源为 `src/ui/ThemeManager.h/cpp`。
- **开发规范 SSOT**：关于 UI 样式架构分工与开发准则的唯一真理源将收拢在根目录新增的 `Guide & Preference.md` 中。

### Q2: 封装完整性 (Encapsulation Integrity) 如何保证？
- 不修改任何既有 C++ 类或 UI 控件的公开 API 签名。
- 通过在 `Guide & Preference.md` 中规范 `style.qss` 与 `ThemeManager.cpp` 的职责分工，并在 `AGENTS.md` 中建立显式读取引导，确保后续任何 UI 样式修改均遵循统一架构标准，严禁在 C++ 控件内部滥用硬编码内联 `setStyleSheet(...)`。

### Q3: 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **表面现象**：样式管理在 `style.qss`、`ThemeManager.cpp` 和散落的控件内联 `setStyleSheet` 中存在职责边界不清的隐患，缺少一份针对 AI Agent 与开发者的权威规范引导文件。
- **根因**：缺少明确界定“静态 QSS”与“C++ 窗口属性控制”分工的规范文档，且 `AGENTS.md` 未包含针对 UI 样式修改的专属指引。

---

## 2. 方案细则

### 2.1 创建 `Guide & Preference.md`
在项目根目录下新建 `Guide & Preference.md`，内容包含：
1. **`style.qss` 职责**：负责全局统一样式（背景色、边框、滚动条、菜单配色等），集中外联定义。
2. **`ThemeManager.cpp` 职责**：负责 QSS 无法完成的代码层控制（窗口半透明/无边框/阴影控制、动态修改、统一加载与局部覆盖入口）。
3. **开发红线**：涉及 UI 样式问题时，必须严格按此分工实施，**严格禁止在控件 C++ 代码中采用内联 `setStyleSheet` 方式硬编码样式**（纯动态计算色彩除外）。

### 2.2 更新 `AGENTS.md`
在 `AGENTS.md` 中新增样式指导章节：
- 显式引导 Agent 在处理 UI 样式相关任务时，必须先读取并遵守 `Guide & Preference.md`。
- 强调任何 UI 样式变更必须遵循 `style.qss` 与 `ThemeManager` 分工原则，禁止随意内联。

---

## 3. 验证与检查计划
1. 读取验证 `Guide & Preference.md` 和 `AGENTS.md`。
2. 执行 Pre-commit 流程（代码审查与内存记录）。
