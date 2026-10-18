# MetaPanel-2.md Implementation Plan

## 0. Pre-Draft Evidence (起草前核查)

### 0.1 已读文件清单
- `src/ui/MetaPanel.h` (已物理读取)
- `src/ui/MetaPanel.cpp` (已物理读取)
- `src/ui/components/ElasticEdit.h` (已物理读取)
- `resources/style.qss` (已物理读取)
- `Memories.md` (已物理读取)

### 0.2 签名原文摘录
- `void MetaPanel::adjustFlowHeights();`
- `void MetaPanel::setImagePreview(const QPixmap& pixmap);`
- `explicit ElasticEdit::ElasticEdit(QWidget* parent = nullptr);`
- `void ElasticEdit::adjustHeight();`
- `void QTextEdit::setReadOnly(bool ro);`
- `void QTextEdit::setPlainText(const QString& text);`

### 0.3 既有 API 检索记录
- 检索 `m_pathEdit`：在 `MetaPanel.h` 第 111 行定义为 `QLineEdit* m_pathEdit = nullptr;`，在 `MetaPanel.h` 第 21 行已包含 `#include "components/ElasticEdit.h"`。
- 检索 `m_pathEdit->setReadOnly(true)`：在 `MetaPanel.cpp` 第 348 行调用，替换为 `ElasticEdit` 后需通过 `m_pathEdit->setReadOnly(true)` 继续保持只读属性不变。

### 0.4 Memories.md 与 AGENTS.md 相关条款要点
- 只读不可二次修改铁律（3.1-3）：既有 `MetaPanel.md` 和 `MetaPanel-1.md` 已提交只读，本次重构迭代新建递增后缀文件 `MetaPanel-2.md`。
- 精准 1:1 类名与递增版本命名铁律（3.1-2 & 3.1-3）：实施方案文件命名为 `MetaPanel-2.md`。
- 重构属性参数绝对照抄与零篡改铁律（4.4）：保持原本左右 8 像素容器内边距（`8px`）、只读属性及 preview 200px 尺寸真理源不变。
- 物理实施方案隔离（3.1）：实施方案保存在 `QuarkMeta Architecture/Implementation Plan/` 物理隔离目录中。

---

## 1. Overview (概述与解决的问题)

本实施方案旨在从**代码架构与控件选型**层面彻底治理 `MetaPanel` 元数据面板的深层病灶：
1. **`m_pathEdit` 升级为 `ElasticEdit` 并保持只读**：将物理路径输入框由单行 `QLineEdit` 替换为具备自动换行与弹性长高能力的 `ElasticEdit`，同时严格保持 `setReadOnly(true)` 只读属性不变，彻底解决长路径尾部截断问题。
2. **彻底消灭 `adjustFlowHeights` 中的手算高度与硬算固定死锁**：
   - 将 `m_topPreviewBox` 与 `m_lblImagePreview` 的 `setFixedWidth`/`setFixedSize(200, 200)` 锁替换为 `setMaximumSize(200, 200)` 弹性自适应上限；
   - 移除 `adjustFlowHeights` 中对 `m_paletteContainer` 与 `m_tagContainer` 的 `setFixedHeight` 手工设置，交由 `FlowLayout` 自适应布局引擎管理；
   - 彻底清除散落在多处的 `m_container->adjustSize()` 冲突唤起。

---

## 2. Modified Files List (影响文件清单)

1. `src/ui/MetaPanel.h`
2. `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes (精准替换块)

### 3.1 `src/ui/MetaPanel.h` (将 `m_pathEdit` 类型替换为 `ElasticEdit*`)

```
<<<<<<< SEARCH
    QLineEdit* m_pathEdit = nullptr;
=======
    ElasticEdit* m_pathEdit = nullptr;
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/MetaPanel.cpp` (实例化 `m_pathEdit` 为只读 `ElasticEdit` 并解除 200px 预览固定死锁)

```
<<<<<<< SEARCH
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

```
<<<<<<< SEARCH
    m_pathEdit = new QLineEdit(pathBox);
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setObjectName("MetaPathEdit");
    pathL->addWidget(m_pathEdit);
=======
    m_pathEdit = new ElasticEdit(pathBox);
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setObjectName("MetaPathEdit");
    pathL->addWidget(m_pathEdit);
>>>>>>> REPLACE
```

---

### 3.3 `src/ui/MetaPanel.cpp` (清除 `createCollapsibleSection` 中的 `m_container->adjustSize()` 冲突唤起)

```
<<<<<<< SEARCH
    connect(btnHeader, &QPushButton::clicked, this, [contentWidget, updateHeaderText, btnHeader, this]() {
        bool nowVisible = !contentWidget->isVisible();
        contentWidget->setVisible(nowVisible);
        updateHeaderText(nowVisible);
        adjustFlowHeights();
        if (m_container) m_container->adjustSize();
    });
=======
    connect(btnHeader, &QPushButton::clicked, this, [contentWidget, updateHeaderText, btnHeader, this]() {
        bool nowVisible = !contentWidget->isVisible();
        contentWidget->setVisible(nowVisible);
        updateHeaderText(nowVisible);
        adjustFlowHeights();
    });
>>>>>>> REPLACE
```

---

### 3.4 `src/ui/MetaPanel.cpp` (彻底清理 `adjustFlowHeights` 中的手算高度与 `adjustSize()`)

```
<<<<<<< SEARCH
void MetaPanel::adjustFlowHeights() {
    if (m_topPreviewBox) {
        bool hasPreview = (m_lblImagePreview && !m_lblImagePreview->pixmap().isNull());
        if (hasPreview) {
            m_topPreviewBox->show();
            m_topPreviewBox->setFixedSize(200, 200);
        } else {
            m_topPreviewBox->hide();
            m_topPreviewBox->setFixedHeight(0);
        }
    }

    if (m_paletteContainer) {
        bool hasPalette = (m_paletteFlowLayout && m_paletteFlowLayout->count() > 0);
        if (hasPalette) {
            m_paletteContainer->show();
            int h = m_paletteFlowLayout->heightForWidth(m_paletteContainer->width());
            if (h <= 0) h = 24;
            m_paletteContainer->setFixedHeight(h);
        } else {
            m_paletteContainer->hide();
            m_paletteContainer->setFixedHeight(0);
        }
    }

    if (m_tagContainer) {
        int h = m_tagFlowLayout->heightForWidth(m_tagContainer->width());
        if (h <= 0) h = 28;
        m_tagContainer->setFixedHeight(h);
        m_tagFlowLayout->activate();
    }

    if (m_container) {
        m_container->adjustSize();
    }
}
=======
void MetaPanel::adjustFlowHeights() {
    if (m_topPreviewBox) {
        bool hasPreview = (m_lblImagePreview && !m_lblImagePreview->pixmap().isNull());
        if (hasPreview) {
            m_topPreviewBox->show();
        } else {
            m_topPreviewBox->hide();
        }
    }

    if (m_paletteContainer) {
        bool hasPalette = (m_paletteFlowLayout && m_paletteFlowLayout->count() > 0);
        if (hasPalette) {
            m_paletteContainer->show();
        } else {
            m_paletteContainer->hide();
        }
    }

    if (m_tagContainer) {
        m_tagFlowLayout->activate();
    }
}
>>>>>>> REPLACE
```

---

### 3.5 `src/ui/MetaPanel.cpp` (更新 `updateInfo` 中对 `m_pathEdit` 的赋值并调用 `adjustHeight()`)

```
<<<<<<< SEARCH
        m_pathEdit->setText(p);
        m_pathEdit->setCursorPosition(0);
=======
        m_pathEdit->setPlainText(p);
        m_pathEdit->adjustHeight();
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
1. **验证路径框弹性折行与只读**：选中长物理路径的文件，确认 `m_pathEdit` 为只读且能够优雅地弹性换行展示全长路径，无尾部截断。
2. **验证 8px 布局留白**：拖动 `MetaPanel` 宽度，确认折叠块内部控件与顶部文件名输入框左右两侧 100% 精准保持对齐的 8 像素留白，无贴边裁切或布局拉扯。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **SSOT 复用说明**：继续使用 `m_containerLayout->setContentsMargins(8, 8, 8, 8)` 作为元数据面板布局内边距的唯一真理源。
- **防另起炉灶检查**：路径框复用通用的 `ElasticEdit` 控件；尺寸计算彻底归流至 Qt 原生布局机制。

---

## 6. Header API Signature Verification (头文件 API 签名核查)

| 类 / 结构体 | 函数签名 (源头 `.h` 声明) | 是否 100% 一致 |
|---|---|---|
| `MetaPanel` | `void adjustFlowHeights();` | 是 |
| `ElasticEdit` | `explicit ElasticEdit(QWidget* parent = nullptr);` | 是 |
| `ElasticEdit` | `void adjustHeight();` | 是 |
| `QTextEdit` | `void setReadOnly(bool ro);` | 是 |
| `QTextEdit` | `void setPlainText(const QString& text);` | 是 |

---

## 7. Header Inclusion Chain & Type Completeness Check (头文件包含链核查)

- `src/ui/MetaPanel.h` 第 21 行已包含 `#include "components/ElasticEdit.h"`，头文件包含链完整闭合。

---

## 8. Zero-Value-Alteration Check (数值零篡改核对)

| 属性名称 | 原数值 / 位置 | 新数值 / 位置 | 核对结论 |
|---|---|---|---|
| 容器内边距 | 8px (`MetaPanel.cpp:164`) | 8px (`MetaPanel.cpp:164`) | 100% 照抄零篡改 |
| 路径框只读属性 | `setReadOnly(true)` (`MetaPanel.cpp:348`) | `setReadOnly(true)` (`MetaPanel.cpp:348`) | 100% 照抄零篡改 |
| 预览图上限尺寸 | 200x200px (`MetaPanel.cpp`) | 200x200px (`MetaPanel.cpp`) | 100% 照抄零篡改 (仅由 Fixed 变为 Maximum) |

---

## 9. Compliance Self-Check Table (合规核对表 - 第 2 关)

| 检查项 | 来源条款 | 自检结果 | 对应章节/行号 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射或明确模块映射 | 3.1-2 | 满足 | 文件名 MetaPanel-2.md 精准 1:1 映射核心类 MetaPanel 的递增版本 |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | QuarkMeta Architecture/Implementation Plan/ |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 3.1~3.5 节 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 本次无新增 Q_OBJECT 类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 第 0~9 章全齐 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 第 6 章 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 第 7 章（MetaPanel.h 已包含 components/ElasticEdit.h） |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 清除手工算高与重复 adjustSize，统一走 Qt 原生布局 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 满足 | 无内联样式更改 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 无 Hack 补丁 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 本方案不涉及 UI 批量广播 |
| 迁移的所有数值原样照抄 | 4.4 | 满足 | 8 章：8px 与 200px 尺寸及只读属性照抄零篡改 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 保持原有 8px 布局设计 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅影响 `MetaPanel.h` 与 `MetaPanel.cpp` |

---
*方案编写完成，已通过第 2 关自检。准备提交供审查。*
