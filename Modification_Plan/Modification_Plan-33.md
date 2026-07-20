# 修复网格视图模式下 gridMode 属性丢失导致缩略图拉伸变形 —— Modification_Plan-33.md

## 1. 任务背景
在之前根据 `Modification_Plan-14.md` 实现的“网格视图不拉伸、等比完整呈现”的改造中，`ThumbnailDelegate::paint` 会动态获取当前视图的 `gridMode` 属性。然而，当前版本的 `JustifiedView` 控件重新布局、实现透明背景和双击优化后，由于全量生成覆盖漏掉了 `gridMode` 属性的注入，导致其丢失。这就使得 `property("gridMode")` 结果永远为 `false`，网格视图下本应不拉伸的缩略图发生了等比拉伸裁切（再次被破坏）。用户明确要求对该网格视图二次破坏问题进行全面修复。

## 2. 问题定位
- **定位模块**：`JustifiedView` 自定义布局视图类。
- **物理路径**：`src/ui/JustifiedView.cpp`
- **函数名称**：`JustifiedView::JustifiedView` (构造函数)、`JustifiedView::setLayoutMode` (布局切换函数)
- **根因分析**：
  在 `src/ui/JustifiedView.cpp` 中，没有向该类（其继承自 `QAbstractItemView` / `QWidget`）注入 `"gridMode"` 属性。
  - 构造函数内缺失属性的默认初始化：`setProperty("gridMode", false);`
  - 在 `setLayoutMode` 切换排版模式函数中，缺失了根据当前的模式动态更新该属性值的逻辑：
    ```cpp
    setProperty("gridMode", m_layoutMode == GridMode);
    ```
  这直接导致 `ThumbnailDelegate::paint`（第 118 行）中通过 `option.widget->property("gridMode").toBool()` 无法识别真实的网格状态。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 之前已经按照Modification_Plan-14.md修复好了网格视图模式 | 针对 Modification-14 中提出的 `gridMode` 动态属性逻辑进行链路完整性修复 | ✅ 一致 |
| 2    | 排查当前版本是不是又再次被破坏了 | 查明原因为 `JustifiedView.cpp` 在版本演进中全量改写丢失了此属性注入 | ✅ 一致 |
| 3    | 去创建详细的修改方案 | 产出此 `Modification_Plan-33.md` 详细修改方案，不擅自修改任何功能代码 | ✅ 一致 |

## 4. 详细解决方案

在 `src/ui/JustifiedView.cpp` 文件中进行以下两处修复，精准补回 `"gridMode"` 动态属性：

1. **构造函数中默认初始化属性**：
   在 `JustifiedView::JustifiedView(QWidget* parent)` 的末尾，补充初始化：
   ```cpp
   setProperty("gridMode", false);
   ```

2. **切换布局时动态同步属性状态**：
   在 `JustifiedView::setLayoutMode(LayoutMode mode)` 内部，在更新模式后，同步写入最新的动态属性状态：
   ```cpp
   void JustifiedView::setLayoutMode(LayoutMode mode) {
       if (m_layoutMode != mode) {
           m_layoutMode = mode;
           setProperty("gridMode", m_layoutMode == GridMode);
           scheduleLayout();
       }
   }
   ```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/JustifiedView.cpp` （仅限于构造函数内部，以及 `setLayoutMode` 函数体内新增对 `setProperty` 的属性配置）

**明确禁止越界修改的范围：**
- [ ] 严禁修改 `ThumbnailDelegate::paint` 中的缩略图绘制逻辑。
- [ ] 严禁修改 `JustifiedView::doLayout` 中的坐标计算与排版逻辑。
- [ ] 严禁修改任何底层 `Model` 与数据库逻辑。

## 6. 实现准则与预警【核心】
1. **防止命名空间及上下文冲突**：本次修改在 `src/ui/JustifiedView.cpp` 既有上下文的局部做微调，确保 `GridMode` 枚举在此处可用（当前定义在 `JustifiedView::GridMode`）。
2. **QObject 属性系统依赖**：使用 `setProperty` 动态属性注入时，需要确保视图在 `ThumbnailDelegate::paint` 阶段能通过 `option.widget` 指针（即 `JustifiedView` 实例）完整读取该属性。这完全符合 Qt 固有的 `QObject` 元对象系统（Meta-Object System）设计。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 侧边栏与磁盘模式 | 搜索过滤等作用域要与 Focus Line 对齐 | ✅ 符合，不触碰过滤和检索逻辑 |
| 纯分析师模式 | 未收到明确“批准执行”前，禁止修改任何代码文件 | ✅ 符合，当前仅创建方案文件，绝对不修改任何代码 |

## 8. 待确认事项（可选）
- 无。该属性注入逻辑与基线版本 `FERREX-META` 中的属性对齐方式完全保持一致。
