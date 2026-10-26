# Implementation Plan - ListViewSectionHeaderBoundingBoxFix

Base Commit Hash: `943037f85524b65875859191f2d3c1dba8aae1c1`
Prerequisite Plans: 无
Supersedes: 无

---

### 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/ui/DropTreeView.h`
   - `src/ui/DropTreeView.cpp`
   - `src/ui/JustifiedView.cpp`
   - `src/ui/TreeItemDelegate.h`
2. **头文件原样签名**：
   - `src/ui/DropTreeView.h:116`: `class DropTreeView : public QTreeView`
   - `src/ui/DropTreeView.cpp:103`: `void DropTreeView::mousePressEvent(QMouseEvent* event);`
3. **既有 API 检索日志**：
   - `grep -rn "mousePressEvent" src/ui/JustifiedView.cpp`
   - 发现 `JustifiedView::mousePressEvent` 已实现了 `leftX + textW + 30` 标头行交互热区限制，而 `DropTreeView::mousePressEvent` 未限制热区导致整行均可触发折叠。

---

## Chapter 1: Overview
在 QuarkMeta 列表视图（`DropTreeView`）中，文件夹分区的折叠/展开功能由标头行（`SectionHeaderRole`，`SectionKindRole == 1`）响应鼠标点击事件驱动。网格视图（`JustifiedView`）已遵守交互限制规范：只有当鼠标点击落在标题文本标签与箭头图标所在的交互热区包围盒（`leftX` 到 `leftX + textW + 30`）范围内时，才触发折叠/展开；点击右侧空白区域时不触发折叠。

然而，列表视图（`DropTreeView::mousePressEvent`）此前未对点击 X 坐标进行热区校验，导致点击标头行整行最右侧空白区域时仍会触发文件夹的折叠与伸展，破坏了视图间交互规范的一致性。

本方案在 `DropTreeView::mousePressEvent` 中补全标头行交互热区包围盒判断（依据 `visualRect`、`SectionHeaderTextRole` 文本宽度及 30px 图标/Padding 容差），确保列表视图与网格视图行为 100% 归一与一致。

---

## Chapter 2: Modified Files List
- `src/ui/DropTreeView.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes

### `src/ui/DropTreeView.cpp`
<<<<<<< SEARCH
void DropTreeView::mousePressEvent(QMouseEvent* event) {
    QModelIndex idx = indexAt(event->pos());
    if (idx.isValid() && idx.data(SectionHeaderRole).toBool()) {
        if (event->button() == Qt::LeftButton) {
            if (idx.data(SectionKindRole).toInt() == 1) {
                SectionProxyModel* secModel = nullptr;
                QAbstractItemModel* cur = model();
                while (cur) {
                    secModel = qobject_cast<SectionProxyModel*>(cur);
                    if (secModel) break;
                    auto* proxy = qobject_cast<QAbstractProxyModel*>(cur);
                    if (proxy) cur = proxy->sourceModel();
                    else break;
                }
                if (secModel) {
                    secModel->setFolderCollapsed(!secModel->isFolderCollapsed());
                }
            }
        }
        event->accept();
        return;
    }
    QTreeView::mousePressEvent(event);
}
=======
void DropTreeView::mousePressEvent(QMouseEvent* event) {
    QModelIndex idx = indexAt(event->pos());
    if (idx.isValid() && idx.data(SectionHeaderRole).toBool()) {
        if (event->button() == Qt::LeftButton) {
            bool inInteractiveBounds = true;
            if (idx.data(SectionKindRole).toInt() == 1) {
                QRect rect = visualRect(idx);
                QFont font("Microsoft YaHei", 10, QFont::Bold);
                QFontMetrics fm(font);
                int leftX = rect.left() + 10;
                int textW = fm.horizontalAdvance(idx.data(SectionHeaderTextRole).toString());
                int interactiveMaxX = leftX + textW + 30; // 文本宽度 + 箭头图标 + 容差 Padding
                if (event->pos().x() < leftX || event->pos().x() > interactiveMaxX) {
                    inInteractiveBounds = false;
                }
            }

            if (inInteractiveBounds) {
                if (idx.data(SectionKindRole).toInt() == 1) {
                    SectionProxyModel* secModel = nullptr;
                    QAbstractItemModel* cur = model();
                    while (cur) {
                        secModel = qobject_cast<SectionProxyModel*>(cur);
                        if (secModel) break;
                        auto* proxy = qobject_cast<QAbstractProxyModel*>(cur);
                        if (proxy) cur = proxy->sourceModel();
                        else break;
                    }
                    if (secModel) {
                        secModel->setFolderCollapsed(!secModel->isFolderCollapsed());
                    }
                }
                event->accept();
                return;
            }
        }
    }
    QTreeView::mousePressEvent(event);
}
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps
1. **构建步骤**：
   在终端执行 `cmake --build build` 或项目默认 CMake 构建命令。
2. **运行时验收标准**：
   - 切换到列表视图（List View）模式。
   - 鼠标左键点击“文件夹 (X)”文本或折叠箭头图标（蓝色范围），校验文件夹能正常折叠/展开。
   - 鼠标左键点击“文件夹 (X)”右侧的整行空白区域（红色范围），校验**不触发**折叠/展开，行为与网格视图完全一致。

---

## Chapter 5: Official API Reuse & Anti-Redundancy
- 复用了 `QTreeView::visualRect(QModelIndex)` 与 `QFontMetrics` 既有 Qt 标准 API，完全对齐 `JustifiedView` 中的包围盒计算逻辑，零冗余实现。

---

## Chapter 6: Data SSOT Declaration Table
| 数据项 | 唯一 SSOT 源 | 派生/缓存副本 | 同步方向 |
| :--- | :--- | :--- | :--- |
| 文件夹折叠状态 | `SectionProxyModel::isFolderCollapsed()` | UI 展现 | `mousePressEvent` -> `SectionProxyModel::setFolderCollapsed` -> `dataChanged` -> View 刷新 |

---

## Chapter 7: Interface & Visibility Consistency Check
- 方法：`DropTreeView::mousePressEvent(QMouseEvent* event)`
  - 声明位置：`src/ui/DropTreeView.h:120`
  - 访问权限：`protected override`
- 未修改任何头文件接口与方法签名。

---

## Chapter 8: Header Chain & Inclusion Closure Check
- 修改仅包含 `src/ui/DropTreeView.cpp` 内部实现逻辑，无需增删 `#include` 头文件包含。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 序号 | 检查项目 | 结果 | 位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否只修改实施方案（禁止直接动源码） | 满足 | 仅创建 `Implementation Plan/ListViewSectionHeaderBoundingBoxFix.md` |
| 2 | SEARCH 块是否物理匹配且唯一 | 满足 | 精确匹配 `src/ui/DropTreeView.cpp` 第 103-126 行 |
| 3 | 是否破坏冻结的 .h 公开 API 签名 | 满足 | 零修改 .h |
| 4 | 是否包含 CMake 文件注册变更 | 不涉及 | 未新建或删除 C++ 源文件 |
| 5 | 是否遵循单向依赖流 | 满足 | UI 控制流，无逆向包含 |
| 6 | 是否零篡改既有设计数值 | 满足 | 100% 对齐 `JustifiedView` 的 `10px` 边距与 `30px` 图标 Padding |
| 7 | 是否包含完整 10 大章节 | 满足 | 包含 Chapter 1 至 Chapter 10 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table
- **行为与交互**：标头行蓝色热区内的点击折叠/展开交互 100% 保持；右侧空白点击行为统一修正为放行给基类 `QTreeView`，消除无意折叠误触。
- **外观与数值**：包围盒边界计算参数（`+10px` 左边距，`+30px` 箭头与 Padding）与 `JustifiedView` 保持 100% 数值一致。
