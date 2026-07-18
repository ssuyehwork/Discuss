# 网格视图缩略图自适应不拉伸改造 —— Modification_Plan-14.md

## 1. 任务背景
在当前版本的卡片网格视图（`GridResultView`）中，图形与多媒体文件的缩略图默认采用等比拉伸、强制填满卡片容器（Cover 模式）的绘制规则。这种拉伸铺满方式会导致长图、宽图的边缘（上下或左右区域）被外边缘圆角裁剪区裁切掉，无法完整呈现全图。用户明确提出需要将网格视图下的缩略图呈现规则改为不拉伸、不裁切、完整容纳（Contain 模式）的显示方式。

## 2. 问题定位
- **定位模块**：`ThumbnailDelegate` 绘图代理类。
- **物理路径**：`src/ui/ThumbnailDelegate.cpp`
- **函数名称**：`ThumbnailDelegate::paint`
- **根因分析**：
  在 `paint` 函数（原第 115~122 行）中，绘制缩略图时无差别地采用了 `Qt::KeepAspectRatioByExpanding` 策略。该策略会强制等比扩展，直到宽和高全部填满 `m.cardRect` 卡片容器，导致比例不一致时发生边缘裁剪：
  ```cpp
  QPixmap scaled = thumb.scaled(m.cardRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
  ```
  为了实现不拉伸、完整容纳（Contain 模式），必须读取视图控件的 `gridMode` 属性（网格模式）。当处于网格模式（`isGrid == true`）时，应当将缩放规则切换为 `Qt::KeepAspectRatio`；在其他非网格拼图状态下保持原逻辑。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 当前版本的网格视图缩略图会拉伸占满整个卡片，而“FERREX-META”却没有拉伸占满整个卡片 | 通过在卡片渲染阶段读取 `gridMode` 属性来感知当前处于网格视图下 | ✅ 一致 |
| 2    | 我期望的是不拉伸的方式 | 在网格模式下将缩放策略调整为 `Qt::KeepAspectRatio`（等比缩放完整容纳） | ✅ 一致 |

## 4. 详细解决方案

在 `src/ui/ThumbnailDelegate.cpp` 文件的 `ThumbnailDelegate::paint` 成员函数中，对缩略图缩放规则进行自适应改造：

1. **引入视图模式感知变量**：
   通过选项参数 `option.widget` 读取当前的属性 `gridMode`。
   
2. **应用互斥缩放机制**：
   - 当 `isGrid` 属性为 `true` 时，调用 `thumb.scaled` 时传入 `Qt::KeepAspectRatio`（不拉伸方式 / 对应用户原话："我期望的是不拉伸的方式"）。
   - 当 `isGrid` 属性为 `false` 时，维持 `Qt::KeepAspectRatioByExpanding` 以兼容拼图视图模式。

**方案说明片段（非执行代码文件）**：
```cpp
// 1. 获取视图网格模式状态
bool isGrid = option.widget ? option.widget->property("gridMode").toBool() : false;

// 2. 动态自适应缩放（对应用户原话："我期望的是不拉伸的方式"）
QPixmap scaled = thumb.scaled(m.cardRect.size(), 
                              isGrid ? Qt::KeepAspectRatio : Qt::KeepAspectRatioByExpanding, 
                              Qt::SmoothTransformation);
```

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/ThumbnailDelegate.cpp` （仅限 `paint` 绘制分支内部的 `scaled` 参数替换）

**明确禁止越界修改的范围：**
- [ ] 严禁修改任何数据加载、模型缓存（`m_cache`）、倒排索引以及数据库 WAL 同步等底层逻辑。
- [ ] 严禁修改除了 `ThumbnailDelegate.cpp` 以外的任何 UI 控件文件。

## 6. 实现准则与预警【核心】
1. **防空指针崩溃**：在获取 `option.widget` 属性前，必须使用三目运算符或空指针防御机制，防止某些临时状态下 widget 为空引起的空指针闪退。
2. **绘图上下文对称**：自适应缩放计算的 `scaled` 的宽和高将不再恒等等于 `m.cardRect.size()`。后续的 `x` 和 `y` 位移居中计算必须继续复用 `m.cardRect.center()` 进行动态偏移：
   ```cpp
   int x = m.cardRect.center().x() - scaled.width() / 2;
   int y = m.cardRect.center().y() - scaled.height() / 2;
   ```
   这可确保缩略图在卡片内完全居中展示。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 缩略图平滑加载 | 异步提取缩略图未命中时必须显示轻量灰色占位背景 `#3A3A3A` | ✅ 符合，不改动已有占位渲染逻辑 |
| 品牌规范 | 严禁自行脑补颜色或混用置顶色 | ✅ 符合，不涉及颜色常量修改 |
| 清除按钮 | 输入框采用原生 `setClearButtonEnabled(true)` | ✅ 符合，本方案不涉及输入框修改 |
| 纯分析师模式 | 禁止 Jules 在本次任务中直接提交并修改代码文件 | ✅ 符合，本方案仅产出方案说明文档，没有任何直接修改行为 |
