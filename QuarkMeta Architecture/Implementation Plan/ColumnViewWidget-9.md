# ColumnViewWidget 与 ContentPanel 窗格智能均分及 230px 最小宽度硬约束实施方案

## 1. Overview（概述与解决的问题）

在 QuarkMeta 的分栏视图（Column View）以及双窗格拆分模式（Split Pane）下，当前存在以下布局计算局限：
1. **分栏视图旧版硬编码固定宽度**：`ColumnViewWidget::updatePaneWidths()` 之前将每一个列窗格（`ColumnViewPane`）强制写死固定为 `fixedWidth(230)`，当视口宽度较宽（如 1200px）且列数较少（如 2 列）时，无法智能均分并铺满视口，导致右侧留下大面积空白区域。
2. **缺乏 230px 动态下限保底**：当新增列或视口缩小使得 `列数 × 230px > 视口宽度` 时，缺乏平滑开启水平滚动与锁定 230px 最小宽度的精确联动。

**重构与改进策略**：
1. 在 `ColumnViewWidget::updatePaneWidths()` 中重构宽度分配算法：
   - 动态计算视口宽度 `viewportW = viewport()->width()` 及当前列数 `count = m_panes.size()`。
   - 基准列宽为 `targetWidth = viewportW / count`，并强制施加 `calculatedWidth = qMax(230, targetWidth)` 的硬性下限保底。
   - 当 `calculatedWidth > 230`（即视口足够大）时，将所有列均分视口宽度；若存在像素整除余数，末尾列自动吸收余数像素，做到 100% 铺满视口且无缝无隙。
   - 当 `calculatedWidth == 230`（即列数多或视口小）时，每列刚性锁定 230px 宽度，`QScrollArea` 自动提供平滑水平滚动能力，防止内容被挤压变形。
2. 在 `ContentPanel::splitPane` 中为双窗格容器 (`m_primaryPaneContainer` / `m_secondaryPaneContainer`) 明确设置 `setMinimumWidth(230)` 约束，确保 `QSplitter` 手柄拖拽或窗口缩小时无法将任一主副窗格压缩至 230px 以下。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ColumnViewWidget.cpp` 替换

```left
<<<<<<< SEARCH
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = 230;
    for (auto* pane : m_panes) {
        pane->setFixedWidth(defaultWidth);
        pane->setMinimumWidth(defaultWidth);
        pane->setMaximumWidth(defaultWidth);
    }

    int totalPanesWidth = m_panes.size() * defaultWidth;
    int containerHeight = m_container ? m_container->height() : viewport()->height();
    int blankWidth = qMax(230, viewport()->width() - totalPanesWidth);
    if (m_blankCanvasWidget) {
        m_blankCanvasWidget->setGeometry(totalPanesWidth, 0, blankWidth, qMax(containerHeight, viewport()->height()));
    }
    if (m_container) {
        m_container->setMinimumWidth(totalPanesWidth + blankWidth);
    }
}
=======
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;

    const int minWidth = 230;
    const int count = m_panes.size();
    const int viewportW = viewport()->width();

    // 1. 智能等比例均分算法：计算各列基准宽度并施加 230px 刚性最小下限
    int targetWidth = count > 0 ? (viewportW / count) : minWidth;
    int calculatedWidth = qMax(minWidth, targetWidth);

    int totalPanesWidth = 0;
    for (int i = 0; i < count; ++i) {
        // 当视口足够大时，最后一列吸收除法余数像素，做到 100% 铺满视口
        int paneW = (calculatedWidth > minWidth && i == count - 1)
            ? qMax(minWidth, viewportW - calculatedWidth * (count - 1))
            : calculatedWidth;

        m_panes[i]->setMinimumWidth(minWidth);
        m_panes[i]->setMaximumWidth(QWIDGETSIZE_MAX);
        m_panes[i]->setFixedWidth(paneW);
        totalPanesWidth += paneW;
    }

    // 2. 空白画布与滚动容器联动 (视口充满时留白设为 0，避免弹出额外水平滚动条)
    int containerHeight = m_container ? m_container->height() : viewport()->height();
    int blankWidth = (totalPanesWidth < viewportW) ? qMax(230, viewportW - totalPanesWidth) : 0;

    if (m_blankCanvasWidget) {
        m_blankCanvasWidget->setGeometry(totalPanesWidth, 0, blankWidth, qMax(containerHeight, viewport()->height()));
    }
    if (m_container) {
        m_container->setMinimumWidth(totalPanesWidth + blankWidth);
    }
}
>>>>>>> REPLACE
```

### 3.2 `src/ui/ContentPanel.cpp` 替换

```left
<<<<<<< SEARCH
        // 1. Primary pane container
        m_primaryPaneContainer = new QFrame(m_paneSplitter);
        m_primaryPaneContainer->setObjectName("EditorContainer");
        m_primaryPaneContainer->setAttribute(Qt::WA_StyledBackground, true);
        QVBoxLayout* primLayout = new QVBoxLayout(m_primaryPaneContainer);
        primLayout->setContentsMargins(0, 0, 0, 0);
        primLayout->setSpacing(0);

        if (m_headerWidget) {
            m_mainLayout->removeWidget(m_headerWidget);
            primLayout->addWidget(m_headerWidget);
        }
        if (m_viewStack) {
            m_mainLayout->removeWidget(m_viewStack);
            primLayout->addWidget(m_viewStack, 1);
        }

        m_paneSplitter->addWidget(m_primaryPaneContainer);

        // 2. Secondary pane container
        m_secondaryPaneContainer = new QWidget(m_paneSplitter);
        QVBoxLayout* secLayout = new QVBoxLayout(m_secondaryPaneContainer);
        secLayout->setContentsMargins(0, 0, 0, 0);
        secLayout->setSpacing(0);
=======
        // 1. Primary pane container (锁定最小宽度 230px，杜绝被 QSplitter 压缩)
        m_primaryPaneContainer = new QFrame(m_paneSplitter);
        m_primaryPaneContainer->setObjectName("EditorContainer");
        m_primaryPaneContainer->setAttribute(Qt::WA_StyledBackground, true);
        m_primaryPaneContainer->setMinimumWidth(230);
        QVBoxLayout* primLayout = new QVBoxLayout(m_primaryPaneContainer);
        primLayout->setContentsMargins(0, 0, 0, 0);
        primLayout->setSpacing(0);

        if (m_headerWidget) {
            m_mainLayout->removeWidget(m_headerWidget);
            primLayout->addWidget(m_headerWidget);
        }
        if (m_viewStack) {
            m_mainLayout->removeWidget(m_viewStack);
            primLayout->addWidget(m_viewStack, 1);
        }

        m_paneSplitter->addWidget(m_primaryPaneContainer);

        // 2. Secondary pane container (锁定最小宽度 230px，杜绝被 QSplitter 压缩)
        m_secondaryPaneContainer = new QWidget(m_paneSplitter);
        m_secondaryPaneContainer->setMinimumWidth(230);
        QVBoxLayout* secLayout = new QVBoxLayout(m_secondaryPaneContainer);
        secLayout->setContentsMargins(0, 0, 0, 0);
        secLayout->setSpacing(0);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **项目构建与编译**：
   - 使用 CMake 构建系统编译 QuarkMeta 目标项目。
2. **功能验证路径**：
   - **均分验证**：打开分栏视图，当仅有 1 或 2 列且视口较宽时，检查列宽度是否智能按比例铺满整个视口空间，无多余未利用留白。
   - **230px 硬下限验证**：逐级展开子文件夹增加列数（例如展开至 5 列），验证每一列宽度均不低于 230px，同时水平滚动条正常出现，滚动无卡顿。
   - **双窗格拆分验证**：点击顶部拆分按钮开启双窗格，拖拽中间 QSplitter 手柄，验证主/副窗格均无法被拖压缩至 230px 以下。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- **既有布局通道复用**：完全在 `ColumnViewWidget::updatePaneWidths()` 既有方法中扩展智能计算，无新增另起炉灶的布局重算函数。
- **既有事件与尺寸响应**：复用 `ColumnViewWidget::resizeEvent` 及 `QScrollArea` 原生水平滚动机制，符合物理真实机制。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 类名 | 物理头文件 | 调用的成员函数 / 字段签名 | 存在性核实结果 |
| :--- | :--- | :--- | :--- |
| `ColumnViewWidget` | `src/ui/ColumnViewWidget.h` | `void updatePaneWidths();` | 物理存在 (已核实) |
| `ColumnViewWidget` | `src/ui/ColumnViewWidget.h` | `QList<ColumnViewPane*> m_panes;` | 物理存在 (已核实) |
| `ColumnViewWidget` | `src/ui/ColumnViewWidget.h` | `QWidget* m_container;` | 物理存在 (已核实) |
| `ColumnViewWidget` | `src/ui/ColumnViewWidget.h` | `QWidget* m_blankCanvasWidget;` | 物理存在 (已核实) |
| `ContentPanel` | `src/ui/ContentPanel.h` | `QSplitter* m_paneSplitter;` | 物理存在 (已核实) |
| `ContentPanel` | `src/ui/ContentPanel.h` | `QWidget* m_primaryPaneContainer;` | 物理存在 (已核实) |
| `ContentPanel` | `src/ui/ContentPanel.h` | `QWidget* m_secondaryPaneContainer;` | 物理存在 (已核实) |
