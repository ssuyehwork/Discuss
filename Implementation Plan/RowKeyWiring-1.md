# RowKeyWiring Implementation Plan

## 1. Overview
给 6 个已拆出的分组类（`TagStatusGroup`、`LinkStatusGroup`、`NoteStatusGroup`、`AspectRatioGroup`、`DuplicateStatusGroup`、`ThumbnailStatusGroup`）接入行键机制，固定内部标识识别，不再依赖界面文字。

## 2. Modified Files List
- `src/ui/TagStatusGroup.h`
- `src/ui/TagStatusGroup.cpp`
- `src/ui/LinkStatusGroup.h`
- `src/ui/LinkStatusGroup.cpp`
- `src/ui/NoteStatusGroup.h`
- `src/ui/NoteStatusGroup.cpp`
- `src/ui/AspectRatioGroup.h`
- `src/ui/AspectRatioGroup.cpp`
- `src/ui/DuplicateStatusGroup.h`
- `src/ui/DuplicateStatusGroup.cpp`
- `src/ui/ThumbnailStatusGroup.h`
- `src/ui/ThumbnailStatusGroup.cpp`
- `src/ui/FilterPanel.cpp`

## 3. Detailed Line-by-Line Changes

各分组类 `AddFilterRowFunc` 统一调整为 4 参数形式 `(QVBoxLayout*, const QString&, int, const QString&)`，传入行键；在 `FilterPanel.cpp` 调用处回调 `addFilterRow(layout, label, count, QColor(), rowKey)`。
