# ColumnItemDelegate-1.md Implementation Plan

## Overview
本实施方案旨在彻底解决列视图 (`ColumnViewWidget` / `ColumnItemDelegate`) 中显示的图标在视觉上比左侧目录导航 (`NavPanel`) 图标显著偏小的问题。

### 根因与修复策略
1. **解除 16px 降级陷阱**：此前 `ColumnItemDelegate.cpp` 中硬编码 `int iconSize = 18;`，导致 Windows Shell 图标在绘制到 18x18 矩形时自动降级选取 16x16 的低分辨率位图，视觉效果严重缩水；将其统一升级为 **20px**，上下各保留 4px 对称呼吸间距（在 28px 行高下垂直居中），促使外壳图标正确调用高质量缩放，视觉饱满度与目录导航完全一致。
2. **几何布局对称同步**：将文本区域左侧起点从 `32px` 平移至 **`34px`**（即 `左外边距 8px + 图标 20px + 图文间距 6px = 34px`），使图文层次呼吸感与导航树达到 100% 视觉对称。
3. **行内重命名编辑器几何同步**：在 `ColumnItemDelegate` 中显式实现 `updateEditorGeometry`，将其定位在左侧 `34px` 与右侧 `22px` 之间，保障重命名编辑框与文本显示区域严丝合缝。

---

## Modified Files List
- `src/ui/ColumnItemDelegate.h`
- `src/ui/ColumnItemDelegate.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnItemDelegate.h`
```diff
<<<<<<< SEARCH
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
=======
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
>>>>>>> REPLACE
```

### 2. `src/ui/ColumnItemDelegate.cpp`
```diff
<<<<<<< SEARCH
    int iconSize = 18;
    QRect iconRect(rect.left(), rect.top() + (rect.height() - iconSize) / 2, iconSize, iconSize);

    if (deco.canConvert<QIcon>() && !deco.value<QIcon>().isNull()) {
        deco.value<QIcon>().paint(painter, iconRect, Qt::AlignCenter);
    } else if (deco.canConvert<QPixmap>() && !deco.value<QPixmap>().isNull()) {
        QPixmap pix = deco.value<QPixmap>();
        painter->drawPixmap(iconRect, pix.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        QIcon fallbackIcon = UiHelper::getIcon(isFolder ? "folder" : "file", QColor("#888888"), 18);
        fallbackIcon.paint(painter, iconRect, Qt::AlignCenter);
    }
=======
    int iconSize = 20;
    QRect iconRect(rect.left(), rect.top() + (rect.height() - iconSize) / 2, iconSize, iconSize);

    if (deco.canConvert<QIcon>() && !deco.value<QIcon>().isNull()) {
        deco.value<QIcon>().paint(painter, iconRect, Qt::AlignCenter);
    } else if (deco.canConvert<QPixmap>() && !deco.value<QPixmap>().isNull()) {
        QPixmap pix = deco.value<QPixmap>();
        painter->drawPixmap(iconRect, pix.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        QIcon fallbackIcon = UiHelper::getIcon(isFolder ? "folder" : "file", QColor("#888888"), 20);
        fallbackIcon.paint(painter, iconRect, Qt::AlignCenter);
    }
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    QString name = index.data(Qt::DisplayRole).toString();
    QRect textRect = option.rect.adjusted(32, 0, -rightMargin, 0);
=======
    QString name = index.data(Qt::DisplayRole).toString();
    QRect textRect = option.rect.adjusted(34, 0, -rightMargin, 0);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
}

} // namespace QuarkMeta
=======
}

void ColumnItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(index);
    QRect r = option.rect;
    r.adjust(34, 1, -22, -1);
    editor->setGeometry(r);
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. 运行 CMake 构建：
   ```powershell
   cmake --build build --config Release
   ```
2. 启动应用：
   ```powershell
   ./build/Release/QuarkMeta.exe
   ```
3. 切换至“列视图”，对比左侧“目录导航”与“列视图”中同名文件夹的图标视觉大小，确认两者大小一致、饱满协调，不存在视觉落差。
4. 双击或按 F2 测试列视图内的行内重命名，确认编辑框光标起点与文件名文本左侧严格对齐（34px）。
