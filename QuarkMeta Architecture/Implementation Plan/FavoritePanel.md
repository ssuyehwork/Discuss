# FavoritePanel Implementation Plan - Unified Row Height Restoration

## Overview
恢复收藏栏（`FavoritePanel`）的行高，使其与 `Version-Old-6` 以及系统其他树状/列表视图（如目录导航 `NavPanel` 与栏视图 `ColumnItemDelegate`）保持统一的标准行高（28px）。

通过在 `FavoriteItemDelegate` 中添加 `sizeHint` 虚函数重载，强制指定每一行物理高度为 `28px`，并确保 `paint` 绘制中的图标和文字垂直居中排版。

---

## Modified Files List
- `src/ui/FavoritePanel.h`
- `src/ui/FavoritePanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/FavoritePanel.h`

```
<<<<<<< SEARCH
class FavoriteItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit FavoriteItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
=======
class FavoriteItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit FavoriteItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
>>>>>>> REPLACE
```

---

### 2. `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
namespace QuarkMeta {

void FavoriteItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
=======
namespace QuarkMeta {

QSize FavoriteItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    sz.setHeight(28);
    return sz;
}

void FavoriteItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. 执行 CMake 构建：
   ```bash
   cmake --build --preset x64-Debug --target QuarkMeta
   ```
2. 运行 QuarkMeta 程序验证 UI：
   - 查看左侧收藏栏与目录导航栏，确认收藏项与目录导航项行高完全统一（均为 28px）。
   - 验证图标与文本在 28px 行高下完美垂直居中，选中与悬停背景块完整覆盖。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- 本方案未新增重复逻辑，通过 Qt 规范的 `QStyledItemDelegate::sizeHint` 统一行高，符合全软件 28px 列表与树节点行高标准。

---

## Header API Signature Verification
- `QStyledItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override` 为 Qt 标准 API 物理签名，绝无编译错位。
