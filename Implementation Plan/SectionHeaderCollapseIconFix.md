# Implementation Plan - SectionHeaderCollapseIconFix.md

## Overview
排查与修复内容面板（`ContentPanel`）中的分类标头（`FolderHeader` / `FileHeader`）未显示展开/折叠指示图标（`scroll-008` / `scroll-010` 箭羽）的问题。

### 根因深度分析
在内容面板的瀑布流/网格视图模式（`JustifiedView` / `GridView`）下，内容面板使用 `ThumbnailDelegate` 作为视图代理。
查阅 `ThumbnailDelegate.cpp` 代码：
```cpp
void ThumbnailDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) return;

    if (index.data(SectionHeaderRole).toBool()) {
        return; // Headers are drawn directly in JustifiedView::paintEvent
    }
    ...
}
```
`ThumbnailDelegate` 遇到分类标头行（`SectionHeaderRole` 为 `true`）时直接 `return`，注释声明由 `JustifiedView::paintEvent` 负责绘制。
但在 `JustifiedView.cpp` 的 `paintEvent` 绘制逻辑中：
```cpp
// JustifiedView.cpp (L459-L465)
if (idx.data(SectionHeaderRole).toBool()) {
    m_geometries[i] = { QRect(margin, currentY, containerWidth, headerH), i, true, idx.data(SectionHeaderTextRole).toString(), idx.data(SectionCollapsedRole).toBool() };
    currentY += headerH + vSpacing;
    continue;
}
```
`JustifiedView::paintEvent` 仅仅在 `m_geometries` 数组里记录了标头的位置 Rect，但在真正的 `QPainter` 渲染绘制阶段，**漏掉了绘制标头背景、标头文本以及展开/折叠指示图标 (`scroll-008` / `scroll-010`) 的代码**，导致网格/瀑布流视图下的分类标头（图 1 处 `文件夹 (27)`）只有留白而没有显示折叠箭头图标。

而在树状/列表视图下，`TreeItemDelegate.h` 包含了正确的标头绘制逻辑：
```cpp
if (index.data(SectionKindRole).toInt() == 1) { // 文件夹标头
    bool collapsed = index.data(SectionCollapsedRole).toBool();
    QIcon arrowIcon = UiHelper::getIcon(collapsed ? "scroll-008" : "scroll-010", QColor("#3498db"), 12);
    QRect iconRect(rect.right() - 25, rect.top() + (rect.height() - 12) / 2, 12, 12);
    arrowIcon.paint(painter, iconRect);
}
```

因此，必须在 `JustifiedView.cpp` 与 `ThumbnailDelegate.cpp` 中补充对 `SectionHeaderRole` 的完整标头绘制（包含背景色 `#1E1E1E`、标头文本 `文件夹 (27)` 以及右侧箭头图标 `scroll-008` / `scroll-010`），实现全视图视觉与交互的绝对统一。

---

## Modified Files List
- `src/ui/JustifiedView.cpp`
- `src/ui/ThumbnailDelegate.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/JustifiedView.cpp`
在 `JustifiedView::paintEvent` 中，当绘制索引属于 `SectionHeaderRole` 时，补齐完整的标头背景、文本与右侧折叠指示图标绘制逻辑：

```
<<<<<<< SEARCH
        if (g.isHeader) {
            // Draw section header
            painter.save();
            painter.fillRect(g.rect, QColor("#222222"));
            painter.setPen(QColor("#AAAAAA"));
            painter.setFont(headerFont);
            painter.drawText(g.rect.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft, g.headerText);
            painter.restore();
            continue;
        }
=======
        if (g.isHeader) {
            // Draw section header with collapse icon
            painter.save();
            painter.fillRect(g.rect, QColor("#1E1E1E"));

            painter.setPen(QColor("#3498db"));
            painter.setFont(headerFont);
            painter.drawText(g.rect.adjusted(10, 0, -35, 0), Qt::AlignVCenter | Qt::AlignLeft, g.headerText);

            // 仅对文件夹标头（Kind == 1）绘制折叠/展开指示箭羽图标
            QModelIndex idx = model()->index(g.itemIndex, 0);
            if (idx.isValid() && idx.data(SectionKindRole).toInt() == 1) {
                bool collapsed = idx.data(SectionCollapsedRole).toBool();
                QIcon arrowIcon = UiHelper::getIcon(collapsed ? "scroll-008" : "scroll-010", QColor("#3498db"), 12);
                QRect iconRect(g.rect.right() - 25, g.rect.top() + (g.rect.height() - 12) / 2, 12, 12);
                arrowIcon.paint(&painter, iconRect);
            }

            painter.restore();
            continue;
        }
>>>>>>> REPLACE
```

### 2. `src/ui/ThumbnailDelegate.cpp`
在 `ThumbnailDelegate::paint` 中，确保对 `SectionHeaderRole` 绘制友好响应：

```
<<<<<<< SEARCH
    if (index.data(SectionHeaderRole).toBool()) {
        return; // Headers are drawn directly in JustifiedView::paintEvent
    }
=======
    if (index.data(SectionHeaderRole).toBool()) {
        painter->save();
        painter->fillRect(option.rect, QColor("#1E1E1E"));
        QFont font = painter->font();
        font.setBold(true);
        font.setPixelSize(12);
        painter->setFont(font);
        painter->setPen(QColor("#3498db"));

        QString text = index.data(SectionHeaderTextRole).toString();
        QRect textRect = option.rect.adjusted(10, 0, -35, 0);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

        if (index.data(SectionKindRole).toInt() == 1) {
            bool collapsed = index.data(SectionCollapsedRole).toBool();
            QIcon arrowIcon = UiHelper::getIcon(collapsed ? "scroll-008" : "scroll-010", QColor("#3498db"), 12);
            QRect iconRect(option.rect.right() - 25, option.rect.top() + (option.rect.height() - 12) / 2, 12, 12);
            arrowIcon.paint(painter, iconRect);
        }
        painter->restore();
        return;
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 编译步骤
```bash
cmake -B build
cmake --build build --config Release
```

### 验证方法
1. **分类标头图标验证**：运行 QuarkMeta，打开包含文件夹和文件的目录（如 `src` 或 `QuarkMeta`）。在网格视图/瀑布流视图下，观察 `文件夹 (27)` 标头右侧，验证成功显示蓝色折叠指示图标 `scroll-010` (展开状态) 或 `scroll-008` (折叠状态)。
2. **交互切换验证**：点击 `文件夹 (27)` 标头或右侧箭头，验证图标在展开（向下箭头）与折叠（向右箭头）之间平滑切换，文件夹网格区域联动隐匿或展开。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- 矢量图标获取严格复用既有官方 API `UiHelper::getIcon(collapsed ? "scroll-008" : "scroll-010", QColor("#3498db"), 12)`。
- 标头数据模型字段严格复用 `SectionHeaderRole`、`SectionHeaderTextRole`、`SectionKindRole` 与 `SectionCollapsedRole`（定义于 `ModelContract.h`），未手写私有字段。

---

## Header API Signature Verification
| 类名 / 接口 | 物理签名核查 (.h 源头) | 结论 |
| :--- | :--- | :--- |
| `UiHelper::getIcon` | `static QIcon getIcon(const QString& name, const QColor& color = QColor(), int size = 0);` | 100% 物理匹配 |
| `SectionKindRole` | `constexpr int SectionKindRole = Qt::UserRole + 201;` | 100% 物理匹配 |
| `SectionCollapsedRole` | `constexpr int SectionCollapsedRole = Qt::UserRole + 203;` | 100% 物理匹配 |

---

## Header Inclusion Chain & Type Completeness Check
- `JustifiedView.cpp` 中已包含或需补齐 `#include "UiHelper.h"` 与 `#include "../core/ModelContract.h"`，类型包含闭合完整。
