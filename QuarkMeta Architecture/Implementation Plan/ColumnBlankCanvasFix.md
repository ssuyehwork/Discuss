# ColumnBlankCanvasFix Implementation Plan

## 1. Overview
修正在 `ColumnViewWidget::updatePaneWidths()` 中计算最右侧刻意留白画布宽度 `blankWidth` 时的算法缺陷。原代码在 `totalPanesWidth < viewportW` 时直接取 `viewportW - totalPanesWidth`，导致在分栏或窄视口下（例如视口宽 300px，1列宽 230px），留白画布宽度被计算为 `300 - 230 = 70px`，严重违背了“最右侧刻意留白画布最小宽度必须锁定在 230 像素（`kColumnPaneWidth`）”的刚性设计红线。本方案将其修正为 `qMax(viewportW - totalPanesWidth, kColumnPaneWidth)`。

---

## 2. Modified Files List
1. `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Changes to `src/ui/ColumnViewWidget.cpp`

<<<<<<< SEARCH
    // 【架构与设计理念刚性红线】最右侧刻意留白画布（ColumnBlankCanvasWidget）：
    // 1. 当列总宽未占满视口时：留白宽度拉伸自适应填补视口剩余所有空间（viewportW - totalPanesWidth），避免多余横向滚动条；
    // 2. 当列总宽超出视口时：最右侧始终保持至少 230px 刻意留白画布，确保最后一列右侧有充裕空白区域可供双击回退及拖放投递。
    int blankWidth = (totalPanesWidth < viewportW)
        ? (viewportW - totalPanesWidth)
        : kColumnPaneWidth;
=======
    // 【架构与设计理念刚性红线】最右侧刻意留白画布（ColumnBlankCanvasWidget）：
    // 1. 当列总宽未占满视口时：留白宽度拉伸自适应填补视口剩余所有空间（viewportW - totalPanesWidth）；
    // 2. 物理刚性下限：最右侧留白画布宽度绝对不低于 230px（kColumnPaneWidth），保证双分栏拉窄时留白列不受打折挤压。
    int blankWidth = qMax(viewportW - totalPanesWidth, kColumnPaneWidth);
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### 4.1 编译指令
```cmd
cmake --build build --config Release
```

### 4.2 功能验证步骤
1. 在双分栏状态下，将某个窗格切换至列视图（ColumnView）。
2. 在包含 1 列数据列（宽 230px）时，拖动分栏 Splitter 将窗格拉窄至 300px，检查最右侧留白画布（`ColumnBlankCanvasWidget`）宽度，确认其保持为 230px 宽度（容器总宽 230+230=460px），横向滚动条可平滑向右滚动完整展示 230px 留白列。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 使用 `qMax` 算法，无冗余分支。

---

## 6. Header API Signature Verification
- 未改变函数签名。

---

## 7. Header Inclusion Chain & Type Completeness Check
- 未改变头文件包含链。
