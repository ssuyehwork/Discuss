# Implementation Plan - ColumnViewWidget Fixed 230px Column Width & Blank Canvas Restoration

## 1. Overview（概述与解决的问题）

### 1.1 问题背景与根因定位
在当前版本的列视图（Miller Columns 架构）中，[`ColumnViewWidget.cpp`](file:///g:/C++/QuarkMeta/QuarkMeta/src/ui/ColumnViewWidget.cpp) 的列宽分配算法被篡改为“等比例均分”逻辑（`targetWidth = count > 0 ? (viewportW / count) : minWidth;`）：
1. **破坏各列固定 230px 宽度设计理念**：当列数较少（1~3 列）时，算法强行将视口宽度均分，把每一列拉伸至 400px~500px 以上强行撑满屏幕；只有当列数极多撑破视口时，才会被挤压至下限；
2. **彻底抹杀最右侧刻意留白画布**：留白画布宽度被设定为 `(totalPanesWidth < viewportW) ? ... : 0`。由于列少时各列被均分拉伸填满了视口（`totalPanesWidth == viewportW`），留白被算为 0；列多超出视口时，条件不满足同样被算为 0。导致无论列多还是列少，最右侧的刻意留白区域（`ColumnBlankCanvasWidget`）宽度永久为 0，使最右侧列直接贴死在元数据栏边框上，双击留白回退、拖拽放置等核心交互全部失效。

### 1.2 重构目标
彻底废除“等比例均分”算法，回归纯粹的物理设计理念：
- **各列列宽绝对固定为 230 像素**（`setFixedWidth(230)`），无论当前有 1 列、3 列还是 7 列，列宽永恒固定为 230px；
- **最右侧刻意留白画布（`ColumnBlankCanvasWidget`）弹性填充与保底机制**：
  - 当列总宽未铺满视口时（`totalPanesWidth < viewportW`）：留白画布宽度自适应吸收视口剩余所有空间（`viewportW - totalPanesWidth`），保持平整留白且不产生水平滚动条；
  - 当列总宽超出视口时（`totalPanesWidth >= viewportW`）：最右侧始终保持至少 230px 的刻意留白画布（`blankWidth = 230`），确保水平滚动条滑到最右端时，最后一列右侧永远保留一块 230px 宽度的空白区域供双击回退上一级与拖拽放置。

### 1.3 架构三问回答 (Architecture 3-Question Answers)
1. **SSOT 真理源溯源**：列布局与几何尺寸由 `ColumnViewWidget::updatePaneWidths()` 统一全权管理并驱动 `m_panes` 与 `m_blankCanvasWidget`，不存在任何跨层逻辑散落；
2. **黑盒完整性**：仅在 `ColumnViewWidget.cpp` 的 `updatePaneWidths()` 私有方法内部修正几何尺寸计算，对外暴露的公有/信号接口（`ColumnViewWidget.h`）保持 100% 冻结与只读；
3. **根因溯源**：彻底清除 Gemini 擅自引入的 `viewportW / count` 伪均分算法，物理还原 `230px` 刚性列宽与留白保底公式。

---

## 2. Modified Files List（影响文件清单）

| 文件路径 | 修改类型 | 说明 |
| :--- | :--- | :--- |
| `src/ui/ColumnViewWidget.cpp` | 逻辑修正 | 废除均分拉伸，锁定各列固定 230px 宽度，恢复刻意留白画布弹性填充与保底计算 |

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ColumnViewWidget.cpp` — 还原各列固定 230px 宽度与刻意留白画布机制

```
<<<<<<< SEARCH
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
=======
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;

    // 【架构与设计理念刚性红线】列视图各列列宽严格、永恒固定为 230 像素，严禁任何形式的等比例均分拉伸！
    constexpr int kColumnPaneWidth = 230;
    const int count = m_panes.size();
    const int viewportW = viewport()->width();

    int totalPanesWidth = 0;
    for (int i = 0; i < count; ++i) {
        m_panes[i]->setFixedWidth(kColumnPaneWidth);
        totalPanesWidth += kColumnPaneWidth;
    }

    // 【架构与设计理念刚性红线】最右侧刻意留白画布（ColumnBlankCanvasWidget）：
    // 1. 当列总宽未占满视口时：留白宽度拉伸自适应填补视口剩余所有空间（viewportW - totalPanesWidth），避免多余横向滚动条；
    // 2. 当列总宽超出视口时：最右侧始终保持至少 230px 刻意留白画布，确保最后一列右侧有充裕空白区域可供双击回退及拖放投递。
    int blankWidth = (totalPanesWidth < viewportW)
        ? (viewportW - totalPanesWidth)
        : kColumnPaneWidth;

    int containerHeight = m_container ? m_container->height() : viewport()->height();

    if (m_blankCanvasWidget) {
        m_blankCanvasWidget->setGeometry(totalPanesWidth, 0, blankWidth, qMax(containerHeight, viewport()->height()));
    }
    if (m_container) {
        m_container->setMinimumWidth(totalPanesWidth + blankWidth);
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译命令
```bash
cmake --build build --config Release
```

### 4.2 验证步骤
1. **少列场景验证（1~2 列）**：
   - 打开根目录或浅层目录（仅显示 1 或 2 列）；
   - 验证每一列的宽度是否严格为 `230px`，绝无拉伸；
   - 验证最右侧是否呈现宽阔的刻意留白区域，右键/双击留白区域能够正常触发回退上一级或上下文菜单；
   - 验证无横向滚动条。
2. **多列溢出场景验证（7 列以上）**：
   - 逐级深入目录展开 7 列以上（如截图所示路径）；
   - 验证所有列的宽度保持一致的 `230px`；
   - 水平滚动条滚动至最右端，验证最后一列右侧是否存在一块明确的 `230px` 刻意留白画布，绝不再贴死在元数据栏边框上；
   - 双击最右侧留白区域，验证能够平滑向上回退一级目录。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（防另起炉灶自查）

| 检查项 | 结论 |
| :--- | :--- |
| 是否复用既有 SSOT 通道 | ✅ 严格复用既有 `ColumnBlankCanvasWidget`，不新建任何类或冗余结构 |
| 是否彻底清除伪均分代码 | ✅ 彻底物理清除 `viewportW / count` 与 `paneW` 吸收余数等死代码 |
| 是否私自改动外部公开接口 | ❌ 严守【契约锁】，`ColumnViewWidget.h` 100% 保持只读未改 |
| 是否引入平台 Hack | ❌ 纯基于标准 Qt 布局与 `setGeometry` 坐标换算，零平台级侵入 |

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 类名 | 函数调用签名 | 核查状态 | 头文件物理位置 |
| :--- | :--- | :--- | :--- |
| `QScrollArea` | `QWidget* viewport() const` | ✅ 已验证 | Qt 原生接口 |
| `QWidget` | `int width() const` | ✅ 已验证 | Qt 原生接口 |
| `QWidget` | `int height() const` | ✅ 已验证 | Qt 原生接口 |
| `QWidget` | `void setFixedWidth(int w)` | ✅ 已验证 | Qt 原生接口 |
| `QWidget` | `void setMinimumWidth(int minw)` | ✅ 已验证 | Qt 原生接口 |
| `QWidget` | `void setGeometry(int x, int y, int w, int h)` | ✅ 已验证 | Qt 原生接口 |
| `ColumnViewWidget` | `void updatePaneWidths()` | ✅ 已验证 | `src/ui/ColumnViewWidget.h` L61 |
