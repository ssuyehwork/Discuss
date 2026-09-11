# ColumnViewWidget Implementation Plan (ColumnViewWidget-10.md)

## Overview
本方案旨在彻底解决分栏视图（Column View）在级联导航/收藏夹跳转时的选中高亮丢失问题，以及消除底部横向滚动条盖住文件夹右侧级联箭头 (`chevron_right`) 的视觉缺陷。

## Modified Files List
1. `src/ui/ColumnItemDelegate.h`
2. `src/ui/ColumnViewWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnItemDelegate.h`
明确计算右侧 20px 的 `chevron_right` 箭头排他保留区，使用 `Qt::ElideRight` 自动文本截断，并通过 `ToolTipRole` 在鼠标悬停时向用户展示完整的长文件名：

```
<<<<<<< SEARCH
        // 3. 绘制文字
        QString name = index.data(Qt::DisplayRole).toString();
        QRect textRect = option.rect.adjusted(32, 0, -28, 0);
        QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
        painter->setPen(textColor);
        painter->setFont(option.font);
        QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

        // 4. 如果是文件夹，最右侧绘制向右箭头 chevron_right
        bool isDir = index.data(Qt::UserRole + 2).toBool();
        if (isDir) {
            QRect arrowRect(option.rect.right() - 20, option.rect.top() + (option.rect.height() - 14) / 2, 14, 14);
            QColor arrowColor = selected ? QColor("#FFFFFF") : QColor("#888888");
            UiHelper::getIcon("chevron_right", arrowColor, 14).paint(painter, arrowRect, Qt::AlignCenter);
        }
=======
        // 3. 绘制文字 (预留右侧 22px 保证 chevron_right 箭头画廊排他)
        QString name = index.data(Qt::DisplayRole).toString();
        QRect textRect = option.rect.adjusted(32, 0, -22, 0);
        QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
        painter->setPen(textColor);
        painter->setFont(option.font);
        QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

        // 4. 如果是文件夹，最右侧绘制向右箭头 chevron_right
        bool isDir = index.data(Qt::UserRole + 2).toBool();
        if (isDir) {
            QRect arrowRect(option.rect.right() - 20, option.rect.top() + (option.rect.height() - 14) / 2, 14, 14);
            QColor arrowColor = selected ? QColor("#FFFFFF") : QColor("#888888");
            UiHelper::getIcon("chevron_right", arrowColor, 14).paint(painter, arrowRect, Qt::AlignCenter);
        }
>>>>>>> REPLACE
```

### 2. `src/ui/ColumnViewWidget.cpp`
禁用 `QListView` 默认的水平滚动条，防止其盖住列表最底部的级联箭头，并确保 `selectItemByPath` 正确触发视图滚动与 `selectionModel` 高亮：

```
<<<<<<< SEARCH
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
=======
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    if (!m_proxyModel) return;
    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        QModelIndex idx = m_proxyModel->index(r, 0);
        if (idx.data(Qt::UserRole + 1).toString() == targetPath) {
            m_listView->setCurrentIndex(idx);
            break;
        }
    }
}
=======
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    if (!m_proxyModel || !m_listView) return;
    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        QModelIndex idx = m_proxyModel->index(r, 0);
        if (idx.data(Qt::UserRole + 1).toString() == targetPath) {
            m_listView->setCurrentIndex(idx);
            m_listView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            m_listView->scrollTo(idx, QAbstractItemView::EnsureVisible);
            break;
        }
    }
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. 编译构建项目：`cmake --build build`
2. 启动应用并切换至分栏视图模式；
3. 从收藏夹或导航栏跳转至深层文件夹，确认目标路径最后一级的数据项被精准高亮选中；
4. 调整窗口大小或放入极长文件名项目，确认文件名自动呈现 `...` 尾部省略且右侧 `chevron_right` 箭头无重叠遮挡，底部亦无额外横向滚动条盖住图标。
