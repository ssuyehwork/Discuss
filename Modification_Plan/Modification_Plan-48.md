# 列表视图去分割线并启用斑马纹 —— Modification_Plan-48.md

> 状态：已批准，执行中 / 已执行完成

## 1. 任务背景
用户反馈在列表视图中，自定义的底部分割线具有冗余感，且界面整体缺少斑马纹高亮以辅助行对齐。
本方案将彻底移除列表视图自定义代理中的水平底部分割线，并对列表视图启用标准的 QTreeView 斑马纹（交替行背景色）以提升整体工业感和可读性。

## 2. 问题定位
- **定位模块 1（Delegate 绘制自绘线逻辑）**：
  在 `src/ui/TreeItemDelegate.h` 的第 142 行左右（`col == 0 && m_drawMiniCards` 分支下）：
  原底部分割线（对应用户原话：“1. 列表视图这些分割线都是冗余的”）的物理自绘逻辑，已在主线分支最新提交中完全移除。

- **定位模块 2（QTreeView 初始化与 QSS 斑马纹设置）**：
  在 `src/ui/ContentPanel.cpp` 中的 `ContentPanel::setupViews()` 方法里，对 `m_treeView` 开启了交替行背景色（对应用户原话：“2. 应该采用斑马纹才对”）：
  ```cpp
  m_treeView->setAlternatingRowColors(true);
  ```
  并在初始化样式表及动态缩放更新样式表（`ContentPanel::updateGridSize()`）两处，均已成功合并交替行背景 `QTreeView::item:alternate`、`:selected` 以及 `:hover` 的 QSS 规则。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 列表视图这些分割线都是冗余的 | 彻底移除 `TreeItemDelegate.h` 中的 `painter->drawLine` 自绘横线逻辑。 | ✅ 一致 |
| 2    | 应该采用斑马纹才对 | 开启 `m_treeView->setAlternatingRowColors(true)` 并补充 QSS 规则。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 移除底部分割线
修改 `src/ui/TreeItemDelegate.h` 中的 `paint` 方法。
移除 `col == 0 && m_drawMiniCards` 分支下的 `painter->drawLine(...)` 分割线绘制逻辑。该修改已在主线中落实合并。

### 4.2 开启并设置斑马纹
1. 修改 `src/ui/ContentPanel.cpp` 里的 `ContentPanel::setupViews()`：
   在 `m_treeView = new DropTreeView(this);` 之后的配置部分，加入 `m_treeView->setAlternatingRowColors(true);` 启用斑马纹。
2. 调整 `setupViews()` 内的 QSS 初始化设置及 `ContentPanel::updateGridSize()` 中缩放级别变化时的 QSS 刷新逻辑，确保斑马纹和悬停高亮样式在主线上完全一致并且正确加载。

## 5. 修改边界声明【范围】
**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/TreeItemDelegate.h`
- [ ] 模块/文件：`src/ui/ContentPanel.cpp`

**明确禁止越界修改的范围：**
- [ ] 侧边栏导航面板 `NavPanel` 中的树形视图风格及代理——不修改。
- [ ] 网格或自适应等非列表视图的代理逻辑及单元格元素绘制——不修改。

## 6. 实现准则与预警【核心】
1. **QSS 覆盖预警**：
   在 Qt 中，若通过 `setStyleSheet` 重新设置了控件的样式表，此前设置的所有 QSS 会被彻底覆盖。必须在 `updateGridSize` 动态计算行高时，完整载入交替行背景色（`:alternate`）样式和悬停高亮样式，确保缩放及切视图后斑马纹不丢失。
2. **底部分割线越界预防**：
   在 Column 0 以外（Column 1, 2, 3 等自绘列中），从未手动绘制过任何底部分割线，因此只需物理清除 Column 0 的 `drawLine` 段。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 输入框清除功能 | 一律使用 Qt 原生 `setClearButtonEnabled(true)`。 | ✅ 符合（本方案未添加新输入框，不涉及） |
| 窗口置顶 | 一律使用 Win32 原生 `SetWindowPos`（`HWND_TOPMOST`/`HWND_NOTOPMOST`），必须配合 `SWP_NOSENDCHANGING` | ✅ 符合（本方案未重构置顶逻辑，不涉及） |
| 标题栏按钮样式 | 悬停：`#3E3E42`；按下：`#4E4E52`。禁用 rgba 蒙版。 | ✅ 符合（本方案不修改标题栏按钮，不涉及） |

## 8. 待确认事项（可选）
- **斑马纹背景色选值**：
  本次交替行背景色采用 `#252526`，该颜色略微亮于暗色背景 `#1E1E1E`，与主标题栏/分割线色块等工业色系完全统一，效果极佳，已被主线采纳落实。
