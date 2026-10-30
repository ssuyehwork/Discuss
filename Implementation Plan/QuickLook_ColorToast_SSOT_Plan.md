# Implementation Plan - QuickLook 颜色标记 Toast 图形化与 SSOT 渲染修正方案

## 1. 方案概述 (Overview)
本方案旨在修正 `PanelMediator.cpp` 中 QuickLook 变更颜色标记时触发的 Toast 提示文案，彻底废除 `"已应用颜色标记"` 等抽象纯文本，严格对齐 `Memories.md` 中的 **颜色标记表达与真理源契约 (Color Display SSOT Contract)**。
修正后，颜色标记 Toast 反馈将通过 `ToolTipOverlay` 增加 `showColorToast` 专属接口，直接绘制并渲染对应颜色的 16x16 矢量实心圆点/色块（若清除颜色则绘制 `no_color` 禁用标识/灰标），在深灰色背景（`#2B2B2B`）与深色边框（`#444444`）的 Toast 气泡中展示，与星级 Toast 的图形化胶囊样式保持 100% 视觉与物理一致。

---

## 2. 问题分析与根因溯源 (Analysis & Root Cause)
1. **抽象纯文本侵入**：`PanelMediator.cpp` 第 966 行使用了抽象文字 `"已应用颜色标记"`，违反了 `Memories.md` 规则 1.3.11：“绝对禁止使用‘已应用颜色标记’等抽象纯文本，必须采用图形化渲染方式，直接绘制并展示对应颜色的实心圆点/色块”。
2. **缺乏颜色图形化渲染接口**：`ToolTipOverlay` 当前仅支持普通文本/HTML Toast，缺少接收颜色值（如 Hex 或颜色 Key）自动渲染圆点/色块资源的 `showColorToast` 专用 API。

---

## 3. 修改文件范围 (Scope of Changes)

### 核心代码文件
- `src/ui/ToolTipOverlay.h` & `src/ui/ToolTipOverlay.cpp`：新增 `showColorToast(const QString& colorHex, QWidget* targetWidget)` 专属接口，根据颜色 Hex 动态绘制 16x16 抗锯齿圆形色块（或 `no_color` 图标）并注册至 `QTextDocument`，于深灰背景（`#2B2B2B`）气泡中展现。
- `src/ui/PanelMediator.cpp`：替换第 966 行对 `showToast(toastMsg)` 的文本调用，改为调用 `ToolTipOverlay::instance()->showColorToast(color, &QuickLookWindow::instance())`。

---

## 4. 架构契约与规范核查 (Architecture Checks)
- **【契约锁】颜色标记表达与真理源契约**：彻底废除 `"已应用颜色标记"` 抽象文本，100% 采用实心颜色圆点/色块图形渲染。
- **【样式锁】暗色视觉一致性**：Toast 背景采用深灰色（`#2B2B2B`），边框采用 `#444444`，与星级 Toast 的深灰胶囊视觉完全对齐。
- **【依赖锁】单向依赖**：`ToolTipOverlay` 使用 `QPainter` 绘制动态色块 Pixmap，保持视图层高内聚。

---

## 5. 详细实现方案 (Implementation Details)

### 5.1 ToolTipOverlay 增加 `showColorToast` 接口
在 `ToolTipOverlay` 中实现 `showColorToast`：
```cpp
void ToolTipOverlay::showColorToast(const QString& colorHex, QWidget* targetWidget) {
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, [this, colorHex, targetWidget]() {
            showColorToast(colorHex, targetWidget);
        });
        return;
    }

    QPixmap colorPix(16, 16);
    colorPix.fill(Qt::transparent);

    if (colorHex.isEmpty() || colorHex == "transparent" || colorHex == "none") {
        colorPix = UiHelper::getPixmap("no_color", QSize(16, 16), QColor("#AAAAAA"));
    } else {
        QPainter painter(&colorPix);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        QColor drawColor(colorHex);
        if (!drawColor.isValid()) {
            drawColor = QColor("#2ecc71");
        }
        painter.setBrush(drawColor);
        painter.drawEllipse(0, 0, 16, 16);
    }

    m_doc.addResource(QTextDocument::ImageResource, QUrl("color_dot.png"), colorPix);

    QString html = "<div style='line-height:16px; text-align:center;'>"
                   "<img src='color_dot.png' style='vertical-align:middle;' />"
                   "</div>";

    showToast(html, 1500, QColor("#444444"), targetWidget);
}
```

### 5.2 PanelMediator 信号连接更新
在 `PanelMediator.cpp` 中将:
```cpp
        QString toastMsg = color.isEmpty() ? "已清除颜色标记" : "已应用颜色标记";
        ToolTipOverlay::instance()->showToast(
            toastMsg, 1500, QColor("#2ecc71"), &QuickLookWindow::instance()
        );
```
替换为:
```cpp
        ToolTipOverlay::instance()->showColorToast(
            color, &QuickLookWindow::instance()
        );
```

---

## 6. 代码修改 (Search & Replace Blocks)

### Block 1: `src/ui/ToolTipOverlay.h`
<<<<<<< SEARCH
    void showRatingToast(int rating, QWidget* targetWidget = nullptr);
=======
    void showRatingToast(int rating, QWidget* targetWidget = nullptr);

    /**
     * @brief 显示符合 Color Display SSOT 契约的图形化颜色标记 Toast 气泡 (深灰背景 + 实心颜色圆点)
     */
    void showColorToast(const QString& colorHex, QWidget* targetWidget = nullptr);
>>>>>>> REPLACE

---

### Block 2: `src/ui/ToolTipOverlay.cpp`
<<<<<<< SEARCH
void ToolTipOverlay::showRatingToast(int rating, QWidget* targetWidget) {
=======
void ToolTipOverlay::showColorToast(const QString& colorHex, QWidget* targetWidget) {
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, [this, colorHex, targetWidget]() {
            showColorToast(colorHex, targetWidget);
        });
        return;
    }

    QPixmap colorPix(16, 16);
    colorPix.fill(Qt::transparent);

    if (colorHex.isEmpty() || colorHex == "transparent" || colorHex == "none") {
        colorPix = UiHelper::getPixmap("no_color", QSize(16, 16), QColor("#AAAAAA"));
    } else {
        QPainter painter(&colorPix);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        QColor drawColor(colorHex);
        if (!drawColor.isValid()) {
            drawColor = QColor("#2ecc71");
        }
        painter.setBrush(drawColor);
        painter.drawEllipse(0, 0, 16, 16);
    }

    m_doc.addResource(QTextDocument::ImageResource, QUrl("color_dot.png"), colorPix);

    QString html = "<div style='line-height:16px; text-align:center;'>"
                   "<img src='color_dot.png' style='vertical-align:middle;' />"
                   "</div>";

    showToast(html, 1500, QColor("#444444"), targetWidget);
}

void ToolTipOverlay::showRatingToast(int rating, QWidget* targetWidget) {
>>>>>>> REPLACE

---

### Block 3: `src/ui/PanelMediator.cpp`
<<<<<<< SEARCH
        QString toastMsg = color.isEmpty() ? "已清除颜色标记" : "已应用颜色标记";
        ToolTipOverlay::instance()->showToast(
            toastMsg, 1500, QColor("#2ecc71"), &QuickLookWindow::instance()
        );
=======
        ToolTipOverlay::instance()->showColorToast(
            color, &QuickLookWindow::instance()
        );
>>>>>>> REPLACE

---

## 7. 三道关自检与验证步骤 (Verification)

### 1. 第一道关：静态编译核查
通过自检校验，确保 `showColorToast` 声明与定义无缝对接，`UiHelper::getPixmap("no_color")` 与 `QPainter` 调用编译无误。

### 2. 第二道关：自检脚本验证
运行方案合规性校验工具：
```bash
python3 tools/plan_verify/verify_plan.py "Implementation Plan/QuickLook_ColorToast_SSOT_Plan.md"
```

---

## 8. 总结 (Summary)
本实施方案彻底废除了 QuickLook 颜色标记变更中的抽象文字提示，将其全面升级为深灰背景（`#2B2B2B`）中的实心颜色圆点/色块图形化渲染，完全实现了与星级 Toast 的统一与图形 SSOT 闭环。
