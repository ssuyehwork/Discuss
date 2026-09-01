# ThemeManager 全局 QSS 样式真理源收拢与死代码根除重构方案

## 1. 核心架构问题回答 (3 Questions)

### Q1: 唯一事实源 (Single Source of Truth, SSOT) 是什么？在哪里？
全局 QSS 样式的唯一真理源 (SSOT) 应为项目资源文件 `resources/style.qss`（通过 Qt 资源系统 `:/style.qss` 加载）。
目前 `ThemeManager.cpp` 中硬编码的内联 Raw String 兜底字符串为不可达死代码，且其中包含了未收录到 `resources/style.qss` 中的关键 QSS 规则（如 `QSplitter::handle` 样式、容器 `margin: 0px 2px` 5px物理切缝规则、`QTreeView` 斑马纹与高亮规则、全局 `QMenu` 样式等）。
本次重构将把全部 QSS 样式规则统一定义并收拢于 `resources/style.qss` 中，`ThemeManager::getGlobalStyleSheet()` 仅保留从资源文件读取的单一读取路径。

### Q2: 封装完整性 (Encapsulation Integrity) 如何保证？
- **接口锁定**：`ThemeManager` 的所有公开 API（包括 `ThemeManager::instance()`, `initialize()`, `getGlobalStyleSheet()`, `applyMenuStyle()`）签名严格冻结，不修改任何参数或返回值类型。
- **内部实现收拢**：`ThemeManager::getGlobalStyleSheet()` 内部逻辑收拢为仅读取 `:/style.qss`。若读取失败则记录日志警告，根除无法触发的冗余 Raw String 文本。

### Q3: 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **表面现象**：`ThemeManager.cpp` 中存在带有 `/* 🚀【全局唯一样式真理源】... */` 注释的字符串，看似是样式定义处，但在此处修改样式无法在应用中生效。
- **根因**：`resources/style.qss` 始终存在且编译入 QRC 资源包中，导致 `ThemeManager::getGlobalStyleSheet()` 中的 `file.open(QFile::ReadOnly)` 恒为 `true` 并提前 `return`，使得 `ThemeManager.cpp` 中的 Raw String 字符串成为永远无法执行的死代码。同时，历史开发过程中部分 QSS 规则被错写在死代码字符串中而未合并至 `resources/style.qss`。

---

## 2. 方案细则

### 2.1 样式真理源合并 (`resources/style.qss`)
将 `ThemeManager.cpp` 内死代码段中缺失的 QSS 样式补充并合并至 `resources/style.qss`，确保全软件样式完整：
1. **QSplitter 分隔条样式**：添加 `QSplitter`, `QSplitter::handle:horizontal`, `QSplitter::handle:horizontal:hover` 属性（1px 宽分隔手柄 + #1E1E1E 背景 + hover #378ADD）。
2. **面板容器 5px 实体缝隙样式**：更新 `#SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer` 添加 `margin: 0px 2px; padding: 0px;` 布局边距。
3. **QMenu 菜单样式**：整合 `QMenu`, `QMenu::item`, `QMenu::item:selected`, `QMenu::item:disabled`, `QMenu::separator` 暗色精致样式。
4. **QTreeView 斑马纹与暗色高亮**：添加 `QTreeView`, `QTreeView::item`, `QTreeView::item:alternate`, `QTreeView::item:hover`, `QTreeView::item:selected` 完整规范。
5. **输入框焦点样式**：统一 `QLineEdit, QTextEdit, QPlainTextEdit` 的 `#252526` 背景、`#333333` 边框、4px 圆角及焦点 `#378ADD` 边框。

### 2.2 根除死代码 (`src/ui/ThemeManager.cpp`)
重构 `ThemeManager::getGlobalStyleSheet()` 实现：
```cpp
QString ThemeManager::getGlobalStyleSheet() const {
    QFile file(":/style.qss");
    if (file.open(QFile::ReadOnly)) {
        return QLatin1String(file.readAll());
    }
    return QString();
}
```
彻底删除后续长达 80 多行的死代码 Raw String。

---

## 3. 验证与检查计划
1. 读取验证：检查修改后的 `resources/style.qss` 与 `src/ui/ThemeManager.cpp`。
2. pre_commit 检查：运行 pre_commit 流程（代码审查与内存记录）。
