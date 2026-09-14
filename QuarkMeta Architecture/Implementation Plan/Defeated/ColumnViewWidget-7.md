# ColumnViewWidget-7.md Implementation Plan

## Overview
本实施方案旨在彻底解决列视图 (Column View) 缺失判断与呈现“空文件夹”逻辑的问题：
1. **分析与其他视图的对比**：
   - 列表中网格视图 (`ThumbnailDelegate`) 与树视图 (`TreeItemDelegate` 在 `m_drawMiniCards = true` 模式下) 均会在 `IsEmptyRole` 返回 `true` 时，为文件夹图标/小卡片绘制青蓝色虚线边框 (`#41F2F2`, `Qt::DashLine`)，清晰展示空文件夹状态；
   - 在列视图 (`ColumnViewPane`) 中，其使用的 `QListView` 对应列代理也是 `TreeItemDelegate`，但因构造参数为 `m_drawMiniCards = false`，导致 `TreeItemDelegate::paint` 中的空文件夹绘制逻辑 `if (isFolder && isEmpty)` 被跳过（此前仅写在 `if (col == 0 && m_drawMiniCards)` 内）；
2. **修复策略**：
   - 在 `TreeItemDelegate::paint` 的 `col == 0 && !m_drawMiniCards`（即列视图单列渲染模式）中补齐空文件夹判断 (`IsEmptyRole`) 与标志绘制逻辑；
   - 当列视图项为文件夹且 `IsEmptyRole` 为 `true` 时，在其图标或文本左侧区域以 `#41F2F2` 虚线框或精美视觉标记空文件夹，保持全应用四大视图（网格/列表/列视图）空文件夹语义与 UI 绘制标准 100% 绝对一致！

---

## Modified Files List
- `src/ui/TreeItemDelegate.h`

---

## Detailed Line-by-Line Changes

### `src/ui/TreeItemDelegate.h` (补齐列视图模式下的 IsEmptyRole 判定与绘制)

```diff
<<<<<<< SEARCH
        // 🚨【列视图支持】：如果是单列 QListView (col == 0 且 !m_drawMiniCards)，在右侧绘制 trailing 箭头指示器 (>)
        bool isFolder = (index.data(TypeRole).toString() == "folder");
        if (col == 0 && !m_drawMiniCards && isFolder) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QRect arrowRect(option.rect.right() - 16, option.rect.top(), 12, option.rect.height());
            painter->setPen(selected ? QColor("#FFFFFF") : QColor("#888888"));
            painter->drawText(arrowRect, Qt::AlignCenter, ">");
            painter->restore();
        }
=======
        // 🚨【列视图支持】：如果是单列 QListView (col == 0 且 !m_drawMiniCards)，在右侧绘制 trailing 箭头指示器 (>)，并补齐空文件夹判断
        bool isFolder = (index.data(TypeRole).toString() == "folder");
        bool isEmpty = index.data(IsEmptyRole).toBool();

        if (col == 0 && !m_drawMiniCards && isFolder) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);

            // 1. 绘制空文件夹青蓝色虚线指示框 (#41F2F2 Qt::DashLine)，对齐网格与列表视图视觉标准
            if (isEmpty) {
                QRect iconBounds = option.rect.adjusted(6, 4, -option.rect.width() + 24, -4);
                painter->setPen(QPen(QColor("#41F2F2"), 1, Qt::DashLine));
                painter->setBrush(Qt::NoBrush);
                painter->drawRoundedRect(iconBounds, 3, 3);
            }

            // 2. 在右侧绘制 trailing 级联箭头指示器 (>)
            QRect arrowRect(option.rect.right() - 16, option.rect.top(), 12, option.rect.height());
            painter->setPen(selected ? QColor("#FFFFFF") : QColor("#888888"));
            painter->drawText(arrowRect, Qt::AlignCenter, ">");
            painter->restore();
        }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. 编译代码
运行 CMake 构建命令，确保编译零 Error / Warning：
```bash
cmake --build build --config Release
```

### 2. 验证方案
1. 打开“列视图” (Column View)，浏览包含空文件夹与非空文件夹的目录；
2. 观察空文件夹的渲染外观，确认空文件夹在列视图的图标区域边缘清晰呈现 `#41F2F2` 青蓝色虚线边框；
3. 验证非空文件夹不呈现虚线框，右侧仍正常保持 `>` 级联箭头，与其他视图（网格/列表）关于 `IsEmptyRole` 的视觉表达保持 100% 规则统一。
