# QuarkMeta 架构重构方案：TreeItemDelegate 行高上限释放

## 一、 Overview
在 `TreeItemDelegate.h` 的 `sizeHint` 函数中，行高动态范围被写死为了 `qBound(28, iconH + 8, 120)`，导致顶部缩放滑块拉至 230px 时，列表视图行高被硬性截断在 120px。
本方案将 `sizeHint` 的上限从 120 修正为 230，下限规范为 30，实现 30px ~ 230px 全区间无缝响应滑块缩放。

## 二、 Modified Files List
- `src/ui/TreeItemDelegate.h`

## 三、 Detailed Line-by-Line Changes

### 1. `src/ui/TreeItemDelegate.h`
```git
<<<<<<< SEARCH
        int h = qBound(28, iconH + 8, 120);
=======
        int h = qBound(30, iconH + 8, 230);
>>>>>>> REPLACE
```

## 四、 Build & Verification Steps
1. 确认 `src/ui/TreeItemDelegate.h` 修改无误。
2. 验证 `sizeHint` 返回高度上限达到 230px。
