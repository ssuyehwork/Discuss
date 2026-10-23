# Implementation Plan - ThemeManager-1

## 1. Overview（概述与解决的问题）

### 解决的问题
目前在 `resources/resources.qrc` 和 `resources/style.qss` 中，存在对物理图标目录 `Icon/arrow_right.svg`、`Icon/arrow_down.svg`、`Icon/check.svg` 的直接引用：
- `QCheckBox::indicator:checked { image: url(:/Icon/check.svg); }`
- `QTreeView::branch:has-children:closed { image: url(:/Icon/arrow_right.svg); }`
- `QTreeView::branch:has-children:open { image: url(:/Icon/arrow_down.svg); }`

由于这 3 个物理 SVG 图标在 `src/ui/SvgIcons.h` 中已经存在标准的内联矢量数据定义，且 QuarkMeta 架构中已有成熟规范的动态样式注入机制（`ThemeManager::getGlobalStyleSheet()` 结合 `UiHelper::getSvgTempFilePath()` / `SvgIconRenderer`），因此必须彻底清除对物理 `Icon/` 路径的依赖，实现样式图标来源 100% 归一化收敛至 `SvgIcons.h`。

### 实施路线
1. **清理 `resources/style.qss`**：物理删除 `style.qss` 中对 `:/Icon/check.svg`、`:/Icon/arrow_right.svg`、`:/Icon/arrow_down.svg` 的直接 `image: url(...)` 硬编码；
2. **清理 `resources/resources.qrc`**：物理删除 `<file>Icon/arrow_right.svg</file>`、`<file>Icon/arrow_down.svg</file>`、`<file>Icon/check.svg</file>` 的打包声明；
3. **在 `ThemeManager::getGlobalStyleSheet()` 统一注入动态样式**：
   - 使用既有官方 SSOT 入口 `UiHelper::getSvgTempFilePath()`，从 `SvgIcons.h` 渲染对应的图标（复用 `ThemeManager` 已有的 `QComboBox::down-arrow` 注入模式）；
   - 对齐现有设计规范中的精确物理参数（原样照抄）：
     - `QCheckBox::indicator:checked`: 对勾颜色 `#378ADD`（对应 `check` 图标）；
     - `QTreeView::branch:has-children:closed`: 箭头颜色 `#378ADD`（对应 `chevron_right` / `arrow_right`）；
     - `QTreeView::branch:has-children:open`: 箭头颜色 `#378ADD`（对应 `chevron_down` / `arrow_down`）。

---

## 2. Modified Files List（影响文件清单）

1. `resources/style.qss`
2. `resources/resources.qrc`
3. `src/ui/ThemeManager.cpp`

---

## 3. Detailed Line-by-Line Changes（包含精准替换块）

### 3.1 `resources/style.qss`
移除 `QCheckBox::indicator:checked` 中的硬编码 `image: url(:/Icon/check.svg);`，以及 `QTreeView::branch` 中两处 `image: url(:/Icon/arrow_*.svg);`。

```
<<<<<<< SEARCH
/* 统一复选框样式 */
QCheckBox { color: #EEEEEE; font-size: 12px; spacing: 5px; }
QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid #444; border-radius: 2px; background: #1E1E1E; }
QCheckBox::indicator:hover { border: 1px solid #666; }
QCheckBox::indicator:checked { 
    border: 1px solid #378ADD; 
    background: #1E1E1E; 
    image: url(:/Icon/check.svg);
}
=======
/* 统一复选框样式 */
QCheckBox { color: #EEEEEE; font-size: 12px; spacing: 5px; }
QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid #444; border-radius: 2px; background: #1E1E1E; }
QCheckBox::indicator:hover { border: 1px solid #666; }
QCheckBox::indicator:checked { 
    border: 1px solid #378ADD; 
    background: #1E1E1E; 
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QTreeView::branch {
    background: transparent;
}
QTreeView::branch:has-children:closed {
    image: url(:/Icon/arrow_right.svg);
}
QTreeView::branch:has-children:open {
    image: url(:/Icon/arrow_down.svg);
}
QTreeView::item {
=======
QTreeView::branch {
    background: transparent;
}
QTreeView::item {
>>>>>>> REPLACE
```

---

### 3.2 `resources/resources.qrc`
从资源包定义中移除已被彻底废弃的物理文件条目。

```
<<<<<<< SEARCH
<RCC>
    <qresource prefix="/">
        <file>style.qss</file>
        <file>app_icon.png</file>
        <file>app_icon.ico</file>
        <file>Icon/arrow_right.svg</file>
        <file>Icon/arrow_down.svg</file>
        <file>Icon/check.svg</file>
    </qresource>
</RCC>
=======
<RCC>
    <qresource prefix="/">
        <file>style.qss</file>
        <file>app_icon.png</file>
        <file>app_icon.ico</file>
    </qresource>
</RCC>
>>>>>>> REPLACE
```

---

### 3.3 `src/ui/ThemeManager.cpp`
在 `ThemeManager::getGlobalStyleSheet()` 中统一从 `SvgIcons.h` 提取并注入动态图片 URL。

```
<<<<<<< SEARCH
QString ThemeManager::getGlobalStyleSheet() const {
    QFile file(":/style.qss");
    if (file.open(QFile::ReadOnly)) {
        QString content = QLatin1String(file.readAll());
        QString arrowPath = UiHelper::getSvgTempFilePath("chevron_down", QColor("#AAAAAA"));
        content += QString("\nQComboBox::down-arrow { image: url(\"%1\"); }\n").arg(arrowPath);
        return content;
    }

    return QString();
}
=======
QString ThemeManager::getGlobalStyleSheet() const {
    QFile file(":/style.qss");
    if (file.open(QFile::ReadOnly)) {
        QString content = QLatin1String(file.readAll());

        // 从 SvgIcons 统一动态提取图标，彻底解除对物理 Icon/*.svg 文件的依赖
        QString comboArrowPath = UiHelper::getSvgTempFilePath("chevron_down", QColor("#AAAAAA"));
        QString checkPath = UiHelper::getSvgTempFilePath("check", QColor("#378ADD"));
        QString treeArrowRight = UiHelper::getSvgTempFilePath("chevron_right", QColor("#378ADD"));
        QString treeArrowDown = UiHelper::getSvgTempFilePath("chevron_down", QColor("#378ADD"));

        content += QString(
            "\nQComboBox::down-arrow { image: url(\"%1\"); }\n"
            "QCheckBox::indicator:checked { image: url(\"%2\"); }\n"
            "QTreeView::branch:has-children:closed { image: url(\"%3\"); }\n"
            "QTreeView::branch:has-children:open { image: url(\"%4\"); }\n"
        ).arg(comboArrowPath, checkPath, treeArrowRight, treeArrowDown);

        return content;
    }

    return QString();
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. 执行工程编译：
   ```powershell
   cmake --build . --config Release --target QuarkMeta
   ```
2. 验证点：
   - 检查启动时 `QCheckBox` 被勾选时能够正确显示蓝色勾选图标；
   - 检查 `NavPanel` 以及其他使用了 `QTreeView` 的展开/折叠节点，能够正常显示展开向下、折叠向右的箭头；
   - 检查无任何找不到 `:/Icon/check.svg` 等 qrc 资源的警告日志。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- **复用官方既有机制**：完全复用 `UiHelper::getSvgTempFilePath(key, color)`（底层为 `SvgIconRenderer::getSvgTempFilePath` 与 `SvgIcons::icons`），未另写图标读取逻辑。
- **复用样式真理源**：全部收敛在 `ThemeManager::getGlobalStyleSheet()` 这一应用全局唯一的样式表入口，完全杜绝了零散在各个业务控件中硬编码 `setStyleSheet` 的行为。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用点 | 头文件路径 | 物理精确签名 |
| :--- | :--- | :--- |
| `UiHelper::getSvgTempFilePath` | `src/ui/UiHelper.h` | `static inline QString getSvgTempFilePath(const QString& key, const QColor& color = QColor("#3498db"))` |
| `SvgIconRenderer::getSvgTempFilePath` | `src/ui/SvgIconRenderer.h` | `static QString getSvgTempFilePath(const QString& key, const QColor& color);` |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `ThemeManager.cpp` 原本已包含 `#include "UiHelper.h"`，无须增删 include；
- 所有用到的类 `QString`、`QColor`、`QFile` 在现有包含链中均具备完整类型定义，不存在隐式依赖断裂风险。
