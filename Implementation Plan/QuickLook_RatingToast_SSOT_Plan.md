# Implementation Plan - QuickLook 星级 Toast 规范化与 SSOT 渲染修正方案

## 1. 方案概述 (Overview)
本方案旨在修正 `PanelMediator.cpp` 中 QuickLook 变更星级时触发的 Toast 提示文案，彻底消除违规的 `"已设为 %1 星"` 阿拉伯数字与文字组合，严格对齐 `Memories.md` 中的 **星级表达与真理源契约 (Rating Display SSOT Contract)**。
修正后，星级 Toast 反馈将通过 `ToolTipOverlay` 注册 SVG 矢量星级 `star_filled` 图标资源，在深灰色背景（`#2B2B2B`）与深色边框（`#444444`）的 Toast 气泡中直接渲染 5 颗矢量星级（实心灰色 `#AAAAAA` 星与底色 `#444444` 暗星），做到 0 文字/数字侵入、0 Unicode 字符，保持全软件矢量化与图形 SSOT 通道 100% 物理一致。

---

## 2. 问题分析与根因溯源 (Analysis & Root Cause)
1. **违规文案侵入**：`PanelMediator.cpp` 第 950 行使用了 `QString("已设为 %1 星").arg(rating)`，违反了 `Memories.md` 规则 1.3.9：“绝对禁止使用‘1星’、‘2星’等阿拉伯数字与文字组合，必须统一使用 SVG 矢量图标（`star_filled`）进行渲染展示”。
2. **样式与背景不统一**：用户明确补充样式要求：“星级底部背景必须是深灰色，而星级则是灰色”。现有的字符串 Toast 缺乏星级图形渲染支持与深灰背景匹配机制。

---

## 3. 修改文件范围 (Scope of Changes)

### 核心代码文件
- `src/ui/ToolTipOverlay.h` & `src/ui/ToolTipOverlay.cpp`：新增 `showRatingToast(int rating, QWidget* targetWidget)` 专属星级 Toast 接口，向 `QTextDocument` 注册 `#AAAAAA` 与 `#444444` 的 `star_filled` 矢量 Pixmap 资源，生成 HTML `<img>` 标签组并渲染于深灰背景（`#2B2B2B`）气泡中。
- `src/ui/PanelMediator.cpp`：替换对 `showToast("已设为 %1 星")` 的违规调用，改为调用 `ToolTipOverlay::instance()->showRatingToast(rating, &QuickLookWindow::instance())`。

---

## 4. 架构契约与规范核查 (Architecture Checks)
- **【契约锁】星级表达与真理源契约**：彻底废除 `"已设为 %1 星"` 文本，100% 采用 `star_filled` SVG 矢量图标渲染 1~5 星图形。
- **【样式锁】暗色视觉一致性**：Toast 背景采用深灰色（`#2B2B2B`），边框采用 `#444444`，实心星填充灰色（`#AAAAAA`），空心星填充暗色（`#444444`）。
- **【依赖锁】单向依赖**：`ToolTipOverlay` 利用 `UiHelper::getPixmap` 渲染 SVG 图标，保持视图层高内聚。

---

## 5. 详细实现方案 (Implementation Details)

### 5.1 ToolTipOverlay 增加 `showRatingToast` 接口
在 `ToolTipOverlay` 中实现 `showRatingToast`：
```cpp
void ToolTipOverlay::showRatingToast(int rating, QWidget* targetWidget) {
    QPixmap starOn = UiHelper::getPixmap("star_filled", QSize(14, 14), QColor("#AAAAAA"));
    QPixmap starOff = UiHelper::getPixmap("star_filled", QSize(14, 14), QColor("#444444"));

    m_doc.addResource(QTextDocument::ImageResource, QUrl("star_on.png"), starOn);
    m_doc.addResource(QTextDocument::ImageResource, QUrl("star_off.png"), starOff);

    QString html = "<div style='line-height: 14px;'>";
    if (rating <= 0) {
        html += "<span style='color:#AAAAAA; font-size:12px;'>已清除星级</span>";
    } else {
        for (int i = 1; i <= 5; ++i) {
            if (i <= rating) {
                html += "<img src='star_on.png' style='vertical-align: middle; margin-right: 2px;' />";
            } else {
                html += "<img src='star_off.png' style='vertical-align: middle; margin-right: 2px;' />";
            }
        }
    }
    html += "</div>";

    showToast(html, 1500, QColor("#444444"), targetWidget);
}
```

### 5.2 PanelMediator 信号连接更新
在 `PanelMediator.cpp` 中将:
```cpp
ToolTipOverlay::instance()->showToast(
    QString("已设为 %1 星").arg(rating),
    1500, QColor("#3498db"), &QuickLookWindow::instance()
);
```
替换为:
```cpp
ToolTipOverlay::instance()->showRatingToast(
    rating, &QuickLookWindow::instance()
);
```

---

## 6. 代码修改 (Search & Replace Blocks)

### Block 1: `src/ui/ToolTipOverlay.h`
<<<<<<< SEARCH
    void showToast(const QString& text, int timeout = 1500, const QColor& borderColor = QColor("#2ecc71"), QWidget* targetWidget = nullptr);
=======
    void showToast(const QString& text, int timeout = 1500, const QColor& borderColor = QColor("#2ecc71"), QWidget* targetWidget = nullptr);

    /**
     * @brief 显示符合 Rating Display SSOT 契约的 1~5 矢量星级 Toast 气泡 (深灰背景 + 灰色 SVG 星级)
     */
    void showRatingToast(int rating, QWidget* targetWidget = nullptr);
>>>>>>> REPLACE

---

### Block 2: `src/ui/ToolTipOverlay.cpp` (头文件包含)
<<<<<<< SEARCH
#include "ToolTipOverlay.h"

#include <QTimer>
=======
#include "ToolTipOverlay.h"
#include "UiHelper.h"

#include <QTimer>
>>>>>>> REPLACE

---

### Block 3: `src/ui/ToolTipOverlay.cpp`
<<<<<<< SEARCH
void ToolTipOverlay::showToast(const QString& text, int timeout, const QColor& borderColor, QWidget* targetWidget) {
=======
void ToolTipOverlay::showRatingToast(int rating, QWidget* targetWidget) {
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, [this, rating, targetWidget]() {
            showRatingToast(rating, targetWidget);
        });
        return;
    }

    QPixmap starOn = UiHelper::getPixmap("star_filled", QSize(14, 14), QColor("#AAAAAA"));
    QPixmap starOff = UiHelper::getPixmap("star_filled", QSize(14, 14), QColor("#444444"));

    m_doc.addResource(QTextDocument::ImageResource, QUrl("star_on.png"), starOn);
    m_doc.addResource(QTextDocument::ImageResource, QUrl("star_off.png"), starOff);

    QString html = "<div style='line-height:14px;'>";
    if (rating <= 0) {
        html += "<span style='color:#AAAAAA; font-size:12px;'>已清除星级</span>";
    } else {
        for (int i = 1; i <= 5; ++i) {
            if (i <= rating) {
                html += "<img src='star_on.png' style='vertical-align:middle; margin-right:2px;' />";
            } else {
                html += "<img src='star_off.png' style='vertical-align:middle; margin-right:2px;' />";
            }
        }
    }
    html += "</div>";

    showToast(html, 1500, QColor("#444444"), targetWidget);
}

void ToolTipOverlay::showToast(const QString& text, int timeout, const QColor& borderColor, QWidget* targetWidget) {
>>>>>>> REPLACE

---

### Block 4: `src/ui/PanelMediator.cpp`
<<<<<<< SEARCH
        ToolTipOverlay::instance()->showToast(
            QString("已设为 %1 星").arg(rating),
            1500, QColor("#3498db"), &QuickLookWindow::instance()
        );
=======
        ToolTipOverlay::instance()->showRatingToast(
            rating, &QuickLookWindow::instance()
        );
>>>>>>> REPLACE

---

## 7. 三道关自检与验证步骤 (Verification)

### 1. 第一道关：静态编译核查
通过自检校验，确保 `UiHelper.h` 资源与 `showRatingToast` 声明/定义 100% 匹配无误。

### 2. 第二道关：自检脚本验证
运行方案合规性校验工具：
```bash
python3 tools/plan_verify/verify_plan.py "Implementation Plan/QuickLook_RatingToast_SSOT_Plan.md"
```

---

## 8. 总结 (Summary)
本实施方案彻底清除了 QuickLook 星级变更提示中的违规阿拉伯数字文本，将其全面升级为深灰背景（`#2B2B2B`）与灰色 SVG 矢量星级（`#AAAAAA`）渲染，完美对齐星级表达与真理源契约。
