# 绘图代理（Delegates）有状态交互逻辑上移与 MVC 彻底分离 —— Modification_Plan-22.md

## 1. 任务背景
根据《ArchitectureComplianceAudit.md》架构合规性审计报告中判定为 FAIL 的第 12 项与第 13 项结论：`ThumbnailDelegate` 与 `TreeItemDelegate` 在职责上存在着严重的MVC“越界行为”。它们不仅负责单元格/缩略图的高效绘制（无状态），还在 `editorEvent`（事件拦截与处理）中深度承担了“局部坐标物理 Hitbox 碰撞检测”、“视图选中状态校验（`isSelected`）”、“选区感知多项批量更新”以及“直接调用 `model->setData(...)` 更改评分元数据”的有状态业务逻辑。由于渲染器代理层越权充当了交互控制器，导致在视口高频滚动或后台 MFT 并发加载时，容易发生由于临时选中行状态不对齐或无效索引引起的并发内存安全隐患。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | Delegates 必须成为“哑渲染器” | 彻底废除 `ThumbnailDelegate` 与 `TreeItemDelegate` 内部的 `editorEvent` 重载，使其仅拥有 pure `paint` 职责 | ✅ 一致 |
| 2    | 交互与评分判定逻辑上移 | 将禁止图标和星级区域的精准 Hitbox 碰撞检测及批量修改逻辑移至父级视图（`ContentPanel` 等）中 | ✅ 一致 |
| 3    | 批量评分状态维护 | 在 View 层实现对选区内行数据的感知，并在触发点击时整体调派 `model->setData`，实现完美的 MVC 纯粹分流 | ✅ 一致 |

## 2. 问题定位
- **定位模块 1（`ThumbnailDelegate::editorEvent` 有状态逻辑堆积）**：
  在 `src/ui/ThumbnailDelegate.cpp` 中，`editorEvent`（第 436~496 行）承载了复杂的卡片物理尺寸度量（`calculateMetrics`）、禁止图标区域检测 `m.banRect` 以及 5 颗星区域碰撞遍历：
  ```cpp
  for (int i = 0; i < 5; ++i) {
      if (m.starRect(i).contains(mEvent->pos())) { ... }
  }
  ```
  在成功命中后，代理类还强行调用 SelectionModel 获取所有选区行并通过 `model->setData` 进行大批量持久化修改，逻辑过于臃肿、越权。
- **定位模块 2（`TreeItemDelegate` 内部重复的事件碰撞）**：
  在 `src/ui/TreeItemDelegate.h`（第 179~216 行）中，在 `editorEvent` 中同样写死了针对 TreeView 列 (column == 2) 的 `banHitbox` 和星级间距物理尺寸 Hitbox 算子，代码极度分散且严重降低了表格树在滚动期间的局部性能。

## 4. 详细解决方案

### 4.1 重塑 Delegates 定位（纯粹的“哑渲染器”）
1. **接口抹除**：
   - 彻底删除 `ThumbnailDelegate.h/cpp` 中的 `editorEvent` 重载声明与具体实现。
   - 彻底删除 `TreeItemDelegate.h` 中的 `editorEvent` 拦截重载（第 179~216 行）。
2. **专注于 Painter 级绘制**：
   - 代理类仅保留 `paint` 和 `sizeHint` 核心绘图算子，只负责从 `index.data` 提取元数据（星级、置顶、标记颜色），然后直接利用 `QPainter` 渲染，退化为 100% 的无状态绘图处理器，获得极佳的渲染速度。

### 4.2 交互逻辑上移：View 级事件捕获（交互层与渲染层解耦）
将所有的“鼠标点击坐标碰撞检测 -> 判定星级 -> 批量调用 model 修改数据”逻辑，**整体上刷至父级 View**（`QAbstractItemView` / `ContentPanel` / `NavPanel`）或其专职事件过滤器 `EventFilter` 中。
- **ContentPanel 实现方案（卡片网格模式）**：
  在 `ContentPanel`（或其具体的视图容器，如 `GridResultView`）的 `mousePressEvent`（或通用事件捕获）中：
  ```cpp
  void ContentPanel::handleViewportMousePress(QMouseEvent* event) {
      QPoint pos = event->pos();
      QModelIndex index = indexAt(pos);
      if (!index.isValid()) return;

      // 1. 获取该项绘制尺寸样式 (StyleOption)
      QStyleOptionViewItem option = viewOptions();
      option.rect = visualRect(index);

      // 2. 利用相同的 Metrics 计算公式，判定点击坐标 (pos) 落在哪个星级或禁止符号上
      Metrics m = calculateMetrics(option);
      if (m.banRect.contains(pos)) {
          applyBatchRating(index, 0); // 批量设为 0 星
      } else {
          for (int i = 0; i < 5; ++i) {
              if (m.starRect(i).contains(pos)) {
                  applyBatchRating(index, i + 1); // 批量设为 i+1 星
                  break;
              }
          }
      }
  }
  ```
- **优势**：
  - View 层天然拥有最权威、最实时的选中行（SelectionModel）和数据模型状态（Model），可进行精准的权限校验与批量落盘。
  - Delegates 内部无需再获取 View 指针和 selectionModel，彻底消除了相互强引用（Circular Reference）。

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：
  - `src/ui/ThumbnailDelegate.h` / `.cpp` （抹除 `editorEvent` 重载）
  - `src/ui/TreeItemDelegate.h` （抹除 `editorEvent` 重载）
  - `src/ui/ContentPanel.h` / `.cpp` （上移并封装 View 鼠标事件及批量评分调用）

**明确禁止越界修改的范围：**
- [ ] 严禁修改任何 QPainter 圆角裁切几何与 SvgIcons 的品牌颜色。
- [ ] 严禁在除 View 表示层外的新写逻辑中直接处理 MouseEvent 物理碰撞。

## 6. 实现准则与预警【核心】
1. **视觉精确Hitbox对齐**：事件上移到 View 捕获后，View 获取的点击坐标是相对于**整个视口（Viewport）**的，而 Delegates 的 `calculateMetrics` 通常是基于单元格局部矩形 `option.rect`。
   **核心红线**：在 View 层的碰撞判定计算时，必须将视口坐标转化为对应单元格局部坐标，或者在 calculateMetrics 之前，将 `option.rect` 作为平移偏置加在计算结果中，保证点击精度与原有的星级位置 100% 重合，绝不发生偏移偏移。
2. **物理权限防御**：在 View 触发批量评分前，必须复用原有的受管边界判定（如果是在库外导航等物理源模式下，应拦截元数据评分操作），确保业务逻辑的严肃性。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 `Modification_Plan-22.md` |
| 考古原则 | 重构代码必须基于现有实现保持高度的代码整齐度与风格一致性 | ✅ 符合，碰撞参数和计算逻辑完全沿用 `calculateMetrics` |
| 输入框清除 | 一律使用 Qt 原生 `setClearButtonEnabled(true)` | ✅ 符合，不涉及清除按钮改动 |
