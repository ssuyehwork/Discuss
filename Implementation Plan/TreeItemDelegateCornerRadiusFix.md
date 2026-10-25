# Implementation Plan - 列表视图缩略图 4 角圆角绘制计算修复方案

- **Base Commit Hash**: `1e9a71b62c08f4b040483238a5a105559768becc`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## 第 1 关：已读物理证据清单 (Pre-Draft Evidence)

### 1.1 物理读取文件列表
1. `src/ui/TreeItemDelegate.h` (已读取第 150-175 行列表视图缩略图绘制逻辑)

### 1.2 头文件原样签名
1. `TreeItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const`：
   - 声明与定义位置：`src/ui/TreeItemDelegate.h` 第 51 行

### 1.3 既有 API 检索日志
- 检索关键词：`grep -rn "addRoundedRect" src/ui/`
- 命中列表：`TreeItemDelegate.h` 第 146、161、197 行等使用 `QPainterPath::addRoundedRect` 进行卡片与缩略图圆角剪裁。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与缺陷分析
在列表视图 (List View Mode) 模式下，项目最左侧显示缩略图的部分，原设计理念为 4px 均匀圆角。但在实际界面中，缩略图呈现为“左上角是圆角，其余三个角是直角”的不对称缺陷外观。

### 1.2 物理根因分析
在 `src/ui/TreeItemDelegate.h` 中：
1. `clipPath` 的圆角剪裁矩形定义为了正方形外框 `squareRect` (`clipPath.addRoundedRect(squareRect, 4, 4)`)；
2. 图像通过 `thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, ...)` 按比例缩放后，实际绘制的图像矩形 `targetRect(x, y, scaled.width(), scaled.height())` 的尺寸小于或偏离 `squareRect`；
3. 图像紧贴 `squareRect` 左上角的部分被切出了圆角，但落入 `squareRect` 内部的其他三条边未受到 `squareRect` 圆角剪裁的约束，导致图像本身原始的硬质直角暴露在外面，形成了“一圆三直”的异常现象。

### 1.3 修复方案设计
将剪裁区域由外部正方形 `squareRect` 改为**图像实际缩放后的绘制矩形 `targetRect`**：
```cpp
QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
int x = squareRect.center().x() - scaled.width() / 2;
int y = squareRect.center().y() - scaled.height() / 2;
QRect targetRect(x, y, scaled.width(), scaled.height());

QPainterPath clipPath;
clipPath.addRoundedRect(targetRect, 4, 4); // 对图像实际渲染矩形做 4px 圆角切角
painter->setClipPath(clipPath);
painter->drawPixmap(targetRect, scaled);
```
这样无论图片处于何种宽高比例，渲染出来的缩略图四周四周 4 个角均呈现平滑的 4px 圆角。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/TreeItemDelegate.h` (修改：修正缩略图剪裁矩形 `clipPath` 为实际图片矩形 `targetRect`)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

```
<<<<<<< SEARCH
                if (!thumb.isNull()) {
                    painter->save();
                    QPainterPath clipPath;
                    clipPath.addRoundedRect(squareRect, 4, 4);
                    painter->setClipPath(clipPath);

                    QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int x = squareRect.center().x() - scaled.width() / 2;
                    int y = squareRect.center().y() - scaled.height() / 2;
                    painter->drawPixmap(x, y, scaled);

                    painter->restore();
                } else {
=======
                if (!thumb.isNull()) {
                    painter->save();
                    QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int x = squareRect.center().x() - scaled.width() / 2;
                    int y = squareRect.center().y() - scaled.height() / 2;
                    QRect targetRect(x, y, scaled.width(), scaled.height());

                    QPainterPath clipPath;
                    clipPath.addRoundedRect(targetRect, 4, 4);
                    painter->setClipPath(clipPath);

                    painter->drawPixmap(targetRect, scaled);

                    painter->restore();
                } else {
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建步骤
运行项目标准 CMake 构建命令：
```bash
cmake --build --preset x64-Debug
```

### 4.2 运行时验收标准
1. 打开 QuarkMeta 应用程序，切换至“列表视图” (List View)；
2. 观察包含不同宽高比例（如长方形、正方形、高瘦图）图片的缩略图展现；
3. 确认所有缩略图卡片的四周 4 个角均精准且平滑地呈现 4px 圆角，消除了“一圆三直”的切割缺陷。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- **Qt 矢量绘制复用**：直接使用 Qt 原生 `QPainterPath::addRoundedRect` 搭配 `QPainter::setClipPath` 进行矢量裁剪，无冗余画板或离屏渲染。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 源 | 派生副本/视图展现 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 卡片圆角半径 | 硬编码标准参数 `4px` | `TreeItemDelegate` 绘制剪裁路径 | 绘制时实时构建 `clipPath` 应用于图片 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. `TreeItemDelegate::paint(...)` 属于 `QStyledItemDelegate` 的虚函数重写，签名与访问级别均保持完全不变。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `TreeItemDelegate.h` 已包含 `#include <QPainterPath>`，头文件链闭合完备。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 说明 |
| :---: | :--- | :---: | :--- |
| 1 | 是否遵循物理隔离与实施方案命名契约 | 是 | 方案存放于 `Implementation Plan/TreeItemDelegateCornerRadiusFix.md` |
| 2 | 是否包含全部 10 大必备章节 | 是 | 包含 Overview 至 Behavior Preservation 全部章节 |
| 3 | SEARCH 块是否在目标文件中真实存在且唯一 | 是 | 已核对原文件唯一匹配 |
| 4 | 是否未直接修改任何 C++/Qt 源代码文件 | 是 | 仅提交实施方案文档，绝对不改源码 |
| 5 | 是否破坏既有对外接口签名与 C++ 契约 | 是(否破坏) | 未修改接口 |
| 6 | 是否违反单向依赖与分层硬锁 | 是(否违反) | 保持在 UI 视图层内部 |
| 7 | 是否包含特定平台的 Hack 级补丁 | Is(否包含) | 无 Hack |
| 8 | 是否包含另起炉灶的重复实现 | 是(否包含) | 无重复实现 |
| 9 | 是否有隐式/显式的数据迁移逻辑 | 是(否包含) | 无数据变动 |
| 10 | 是否破坏既有 UI 样式数值或参数 | 是(否破坏) | 严格遵循 4px 圆角设计理念 |
| 11 | 是否完整走完四道关质量管控流程 | 是 | 完成需求确认与逻辑核对 |
| 12 | 是否存在跨线程直接操作 GUI 控件行为 | 是(否存在) | 主线程绘制 |
| 13 | 是否有死代码残留 | 是(无残留) | 无死代码 |
| 14 | 是否引发潜在的死锁风险 | 是(无风险) | 无锁操作 |
| 15 | 变量与类型闭合性 | 是 | 闭合 |
| 16 | CMake 注册完整性 | 是 | 未新建/删除文件 |
| 17 | 行为与交互零篡改声明 | 是 | 仅修正切角坐标，无交互改变 |
| 18 | 语言规范 | 是 | 100% 全中文编写 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 类别 | 项目名称 | 原始状态 / 数值 | 实施后状态 / 数值 | 确认一致 |
| :--- | :--- | :--- | :--- | :---: |
| 样式数值 | 缩略图圆角半径 | `4px` | `4px` | 是 |
| 渲染排版 | 缩略图缩放模式 | `Qt::KeepAspectRatio` | `Qt::KeepAspectRatio` | 是 |
