# MetaPanelPaddingFix-1.md Implementation Plan

## 0. Pre-Draft Evidence (起草前核查)

### 0.1 已读文件清单
- `src/ui/MetaPanel.h` (已物理读取)
- `src/ui/MetaPanel.cpp` (已物理读取)
- `resources/style.qss` (已物理读取)
- `Memories.md` (已物理读取)

### 0.2 签名原文摘录
- `void setImagePreview(const QPixmap& pixmap);`
- `void setMultiSelectionPreview(const QList<MetaPreviewItem>& items);`
- `void setDefaultIconPreview(const QIcon& icon);`

### 0.3 既有 API 检索记录
- 检索 `setFixedSize(200, 200)` 与 `setFixedWidth(200)`：在 `MetaPanel.cpp` 的 `initUi`、`setImagePreview`、`setMultiSelectionPreview` 与 `setDefaultIconPreview` 中硬编码设置了固定尺寸，导致面板收窄时撑爆 8px 容器内边距。

### 0.4 Memories.md 与 AGENTS.md 相关条款要点
- 只读不可二次修改铁律（3.1-3）：由于既有 `MetaPanelPaddingFix.md` 已提交只读，本次治本迭代采用递增后缀形式新建 `MetaPanelPaddingFix-1.md` 方案文件。
- 重构属性参数绝对照抄与零篡改铁律（4.4）：保持原本左右 8 像素容器内边距（`8px`）设计红线不变，通过消除硬编码宽度解决贴边溢出。
- 物理实施方案隔离（3.1）：实施方案保存在 `QuarkMeta Architecture/Implementation Plan/` 物理隔离目录中。

---

## 1. Overview (概述与解决的问题)

本实施方案旨在从**代码架构与控件尺寸策略**层面彻底治理元数据面板（`MetaPanel`）在小宽度下输入框与容器“0 边距贴边顶破”的根源病因：
1. **根因归因**：`MetaPanel.cpp` 中多处对 `m_topPreviewBox` 与 `m_lblImagePreview` 执行了 `setFixedWidth(200)` 和 `setFixedSize(200, 200)` 硬性固定尺寸约束。当面板宽度由于 `QSplitter` 调整或垂直滚动条出现而缩窄至小于 `216px`（200px + 左右 8px 内边距）时，固定尺寸的控件强制撑卡了 `m_container` 的物理最小宽度，导致 `m_containerLayout` 的右侧 8px 内边距被强行挤出视口裁剪区外，造成界面右侧彻底 0 边距贴边。
2. **治本解法**：彻底解除 `m_topPreviewBox` 与 `m_lblImagePreview` 的 `FixedWidth` / `FixedSize` 物理锁，改为弹性最大尺寸约束 `setMaximumWidth(200)` / `setMaximumSize(QSize(200, 200))`，配合 `m_scrollArea` 的透明边框设置，保证无论面板总宽度被拉窄至多少，`m_containerLayout->setContentsMargins(8, 8, 8, 8)` 的左右 8 像素安全红线空间在数学与渲染上 100% 始终被硬性保证。

---

## 2. Modified Files List (影响文件清单)

1. `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes (精准替换块)

### 3.1 `src/ui/MetaPanel.cpp` (解除 `m_topPreviewBox` 与 `m_scrollArea` 的硬编码卡死宽度)

```
<<<<<<< SEARCH
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setObjectName("MetaScrollArea");

    m_container = new QWidget(m_scrollArea);
    m_containerLayout = new QVBoxLayout(m_container);
    m_containerLayout->setContentsMargins(8, 8, 8, 8);
    m_containerLayout->setSpacing(8);

    // 1. 顶部预览区
    m_topPreviewBox = new QWidget(m_container);
    m_topPreviewBox->setObjectName("TopPreviewBox");
    m_topPreviewBox->setFixedWidth(200);
    // TopPreviewBox style in style.qss
    QVBoxLayout* previewLayout = new QVBoxLayout(m_topPreviewBox);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(0);

    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setFixedSize(200, 200);
=======
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setObjectName("MetaScrollArea");

    m_container = new QWidget(m_scrollArea);
    m_containerLayout = new QVBoxLayout(m_container);
    m_containerLayout->setContentsMargins(8, 8, 8, 8);
    m_containerLayout->setSpacing(8);

    // 1. 顶部预览区
    m_topPreviewBox = new QWidget(m_container);
    m_topPreviewBox->setObjectName("TopPreviewBox");
    m_topPreviewBox->setMaximumWidth(200);
    // TopPreviewBox style in style.qss
    QVBoxLayout* previewLayout = new QVBoxLayout(m_topPreviewBox);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(0);

    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setMaximumSize(200, 200);
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/MetaPanel.cpp` (解除 `setImagePreview` 中的固定 200x200 限制)

```
<<<<<<< SEARCH
    } else {
        QPixmap canvas = renderThumbnailLayer(pixmap);
        m_lblImagePreview->setPixmap(canvas);
        m_lblImagePreview->setFixedSize(QSize(200, 200));

        m_lblImagePreview->show();
=======
    } else {
        QPixmap canvas = renderThumbnailLayer(pixmap);
        m_lblImagePreview->setPixmap(canvas);
        m_lblImagePreview->setMaximumSize(QSize(200, 200));

        m_lblImagePreview->show();
>>>>>>> REPLACE
```

---

### 3.3 `src/ui/MetaPanel.cpp` (解除 `setMultiSelectionPreview` 中的固定 200x200 限制)

```
<<<<<<< SEARCH
    m_lblImagePreview->setPixmap(canvas);
    m_lblImagePreview->setFixedSize(logicalCanvasSize);

    m_lblImagePreview->show();
=======
    m_lblImagePreview->setPixmap(canvas);
    m_lblImagePreview->setMaximumSize(logicalCanvasSize);

    m_lblImagePreview->show();
>>>>>>> REPLACE
```

---

### 3.4 `src/ui/MetaPanel.cpp` (解除 `setDefaultIconPreview` 中的固定 200x200 限制)

```
<<<<<<< SEARCH
    } else {
        QPixmap canvas = renderIconLayer(icon, devicePixelRatioF());
        m_lblImagePreview->setPixmap(canvas);
        m_lblImagePreview->setFixedSize(QSize(200, 200));

        m_lblImagePreview->show();
=======
    } else {
        QPixmap canvas = renderIconLayer(icon, devicePixelRatioF());
        m_lblImagePreview->setPixmap(canvas);
        m_lblImagePreview->setMaximumSize(QSize(200, 200));

        m_lblImagePreview->show();
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps (编译与验证)

### 编译步骤
1. 配置 CMake：
   `cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug`
2. 编译项目：
   `cmake --build build --config Debug`

### 验证方法
1. **测试窄宽度渲染**：启动 QuarkMeta，拖动 QSplitter 句柄将元数据面板（`MetaPanel`）收窄至 200px 左右。
2. **核对 8px 内边距**：检查“备注说明”、“关联网址”、“物理路径”输入框及标签框，确认左右两侧与面板边缘始终精准保持 8 像素留白，不再发生 0 边距贴边或顶破裁剪现象。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **SSOT 复用说明**：继续使用 `m_containerLayout->setContentsMargins(8, 8, 8, 8)` 作为元数据面板布局内边距的唯一真理源，消除硬编码尺寸卡死造成的破坏。
- **防另起炉灶检查**：未新增冗余布局和属性设置，仅将卡死的固定尺寸优化为弹性自适应最大尺寸（`MaximumSize`）。

---

## 6. Header API Signature Verification (头文件 API 签名核查)

| 类 / 结构体 | 函数签名 (源头 `.h` 声明) | 是否 100% 一致 |
|---|---|---|
| `MetaPanel` | `void setImagePreview(const QPixmap& pixmap);` | 是 |
| `MetaPanel` | `void setMultiSelectionPreview(const QList<MetaPreviewItem>& items);` | 是 |
| `MetaPanel` | `void setDefaultIconPreview(const QIcon& icon);` | 是 |

---

## 7. Header Inclusion Chain & Type Completeness Check (头文件包含链核查)

- `src/ui/MetaPanel.cpp` 包含 `MetaPanel.h`, `<QScrollArea>`, `<QVBoxLayout>` 等。类型定义完整。

---

## 8. Zero-Value-Alteration Check (数值零篡改核对)

| 属性名称 | 原数值 / 位置 | 新数值 / 位置 | 核对结论 |
|---|---|---|---|
| 容器内边距 | 8px (`MetaPanel.cpp:164`) | 8px (`MetaPanel.cpp:164`) | 100% 照抄零篡改 |
| 预览图上限尺寸 | 200x200px (`MetaPanel.cpp`) | 200x200px (`MetaPanel.cpp`) | 100% 照抄零篡改 (仅由 Fixed 变为 Maximum) |

---

## 9. Compliance Self-Check Table (合规核对表 - 第 2 关)

| 检查项 | 来源条款 | 自检结果 | 对应章节/行号 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射或明确模块映射 | 3.1-2 | 满足 | 文件名 MetaPanelPaddingFix-1.md 精准映射 MetaPanel 面板边距修复的递增版本 |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | QuarkMeta Architecture/Implementation Plan/ |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 3.1~3.4 节 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 本次无新增 Q_OBJECT 类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 第 0~9 章全齐 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 第 6 章 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 第 7 章 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 修复固定尺寸矛盾，复用 `m_containerLayout` 内边距 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 满足 | 无内联样式更改 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 无 Hack 补丁 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 本方案不涉及 UI 批量广播 |
| 迁移的所有数值原样照抄 | 4.4 | 满足 | 8 章：8px 与 200px 尺寸照抄零篡改 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 保持原有 8px 布局设计 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅影响 `MetaPanel.cpp` |

---
*方案编写完成，已通过第 2 关自检。准备提交供审查。*
