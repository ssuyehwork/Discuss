# ThumbnailDelegate 职责过载审计与模块化拆分规划 —— Modification_Plan-40.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在 ArcMeta 系统的 UI 架构中，`ThumbnailDelegate` 是作为结果视图中各个单元格数据渲染及交互控制的专属委托类。随着产品功能迭代，该类逐渐引入了多维度的状态信息、自定义重命名控制、评级星级计算、角标渲染、虚线边框绘制等逻辑。
本分析旨在排查该类是否存在“职责过载”问题，并结合其现状提供一套面向高内聚、低耦合的模块化重构与单一职责拆分规划方案。

---

## 2. 问题定位

### 2.1 代码职责现状分析
对 `src/ui/ThumbnailDelegate.h` 与 `src/ui/ThumbnailDelegate.cpp` 进行代码审计后，可以将其承载的职责划分为以下 5 个主要维度：

1. **核心骨架与布局几何计算（职责点 1）**：
   - `calculateMetrics` 物理计算了整个卡片外框、图片区、文本区、星级区和徽标区的尺寸定位，并直接将尺寸与视图的物理缩放大小（`decorationSize()`）相绑定。
2. **底层卡片及缩略图 Cover 物理绘制（职责点 2）**：
   - 自行控制卡片底色（区分图形类文件的占位底色 `#3A3A3A` 与通用底色 `#2d2d2d` 的分配逻辑）。
   - 实现圆角裁剪、缩略图物理拉伸或降级至普通图标的 60% 缩小比例绘制逻辑。
   - 物理绘制选中态（3px 品牌蓝）与非选中态（1px #4a4a4a）的外边框。
3. **元数据状态徽标与多色角标物理绘制（职责点 3）**：
   - **右上角状态图标叠加层**：同时处理置顶 (`pin_vertical`)、已录入 (`check_circle`) 标志，以及手绘计算底环与弧线进度条（通过 `drawArc` 计算进度环角度、起止弧度并渲染进度）。
   - **扩展名徽标层**：通过识别路径文件扩展名，调用工具方法解析自适应背景色，绘制扩展名徽标（如分类强制显示 `DIR` 徽标、文件显示大写后缀徽标），并处理半透明对比度。
   - **评分与彩色胶囊层**：自行解析 `colorRole` 彩色背景色、通过感知亮度公式（Luminance）计算文字对比色（黑白），同时绘制五颗星的评级图标（实心与空心）。
   - **空文件夹特殊标记**：绘制透明的 `#41F2F2` 虚线边框。
4. **截断与折行排版算法（职责点 4）**：
   - 在主函数内部硬编码实现了局域 lambda 闭包 `elidedName` 文本算法，逐字符检测横向尺寸计算第一行换行断点，并处理第二行超长截断并追加 `...`。
   - 针对未录入状态的文件名文本设置半透明遮罩。
5. **重命名编辑器生命周期与交互控制（职责点 5）**：
   - **重命名编辑器微调**：在 `updateEditorGeometry` 中直接微调几何像素对齐；在 `setEditorData` 中运用延迟 `QTimer::singleShot` 应对 Qt 默认焦点机制，解析文件类型决定全选还是主名称高亮。
   - **交互事件和悬浮拦截**：在 `helpEvent` 中直接拦截计算是否悬停在状态进度环之上，手动绕过 View 机制直接调用 `ToolTipOverlay::instance()->showText(...)` 显示提示文本。在 `eventFilter` 拦截左右上下及 Home/End 物理按键进行分流。
   - **选择联动**：在 `setModelData` 重命名成功后，循环向上遍历 Parent 指针，直到定位到 `ContentPanel`，触发其 `onSelectionChanged()`。

### 2.2 结论：是否存在职责过载？
**是的，ThumbnailDelegate 存在明显的职责过载问题（Single Responsibility Principle Violation）。**

其不仅负责继承自 `QStyledItemDelegate` 的标准视图重命名和辅助布局功能，还充当了 **“超级画家（Super Painter）”**（同时绘制背景图、进度环、多色格式角标、彩虹背景胶囊、感知对比评级星）以及 **“半个控制器（Controller）”**（进行视图联动、ToolTip 触发与重命名逻辑转发）。这种万能 Delegate 的设计导致该文件极度膨胀、修改极易引发连锁视觉缺陷（回归灾难），同时也不利于在其他视图中复用上述子组件的绘制代码。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | “ThumbnailDelegate.cpp”是否存在职责过载？ | 第 2.2 节 明确确立存在职责过载的定位与结论 | ✅ |
| 2    | 该如何规划将其拆分成职责单一模块化？ | 第 4 节 提供具体的解耦模块化规划与重构落地步骤 | ✅ |

---

## 4. 详细解决方案

为了将 `ThumbnailDelegate` 解耦为高内聚、单一职责的模块化结构，规划采取 **“绘制逻辑原子化 + 重命名控制器独立化 + 辅助排版静态化”** 的渐进式重构路线，具体拆分架构设计如下：

### 4.1 模块化拆分架构设计

```
[原万能委托：ThumbnailDelegate]
           │
           ├──► [重命名与交互控制器：ThumbnailDelegate] (主骨架，仅负责状态控制、事件分流、Editor管理)
           │
           ├──► [原子化渲染辅助：CardPainterHelper] (静态/原子类，专职视觉绘制)
           │     ├─── 绘制卡片背景与圆角 Cover 图 (drawCardBackgroundAndCover)
           │     ├─── 绘制互斥状态指示器与进度弧 (drawStatusIndicators)
           │     ├─── 绘制多色后缀角标与胶囊 (drawExtensionBadge)
           │     └─── 绘制评级星级与感知对比底色 (drawRatingStars)
           │
           ├──► [文本自适应处理：ElidedTextUtility] (公共静态工具方法)
           │     └─── 双行截断排版逻辑 (elideTwoLinesText)
           │
           └──► [几何量规计算器：ThumbnailDelegate::Metrics] (保持结构体紧凑性，只计算坐标)
```

### 4.2 规划拆分步骤与伪代码说明

#### 第一步：提取文本截断工具类 `ElidedTextUtility`
将 `elidedName` 移至公共辅助工具中：
```cpp
namespace ArcMeta {
class ElidedTextUtility {
public:
    static QString elideTwoLinesText(const QString& text, const QFontMetrics& fm, int width) {
        QString line1 = fm.elidedText(text, Qt::ElideRight, width);
        if (line1 == text) return text;
        int breakPos = 0;
        for (int i = 1; i <= text.length(); ++i) {
            if (fm.horizontalAdvance(text.left(i)) > width) {
                breakPos = i - 1;
                break;
            }
        }
        if (breakPos <= 0) return line1;
        QString remaining = text.mid(breakPos);
        QString line2 = fm.elidedText(remaining, Qt::ElideRight, width);
        return text.left(breakPos) + "\n" + line2;
    }
};
}
```

#### 第二步：创建原子化渲染辅助类 `CardPainterHelper`
创建单独的辅助文件（如 `CardPainterHelper.h` / `.cpp`），将底层的 `QPainter` 物理绘制逻辑拆分为若干原子级的静态或轻量化函数：
```cpp
namespace ArcMeta {
class CardPainterHelper {
public:
    // 1. 绘制主体卡片及缩略图 Cover
    static void drawCardCover(QPainter* painter, const QRect& cardRect, bool isSelected,
                             bool hasThumb, const QPixmap& thumb, const QIcon& defaultIcon,
                             bool isGridMode, bool isWaitingThumb);

    // 2. 绘制状态互斥标记及进度环
    static void drawStatusIndicators(QPainter* painter, const QRect& cardRect,
                                     bool isPinned, bool isManaged, bool isDir, double progress);

    // 3. 绘制自适应扩展名徽章
    static void drawExtensionBadge(QPainter* painter, const QRect& cardRect,
                                   const QString& ext, bool hasThumb);

    // 4. 绘制评级星级与彩色胶囊底色
    static void drawRatingStars(QPainter* painter, const QRect& banRect, const QRect& lastStarRect,
                                int rating, const QString& colorStr, bool isSelected);

    // 5. 绘制空文件夹特异虚线边框
    static void drawEmptyFolderBorder(QPainter* painter, const QRect& cardRect);
};
}
```

#### 第三步：极简重构 `ThumbnailDelegate::paint` 逻辑
重构后的 `ThumbnailDelegate::paint` 变成轻量级的控制器骨架。它只负责从 `index` 中索取底层 Model 数据、调用 Metrics 几何定位，随后将绘制动作分发给 `CardPainterHelper`：
```cpp
void ThumbnailDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Metrics m = calculateMetrics(option);
    bool isSelected = (option.state & QStyle::State_Selected);

    // 1. 绘制卡片底色、Cover 图与边框
    bool hasThumb = index.data(m_hasThumbnailRole).toBool();
    QPixmap thumb = getPixmapFromIndex(index);
    bool isWaitingThumb = checkIsWaitingThumb(index);
    bool isGrid = option.widget ? option.widget->property("gridMode").toBool() : false;

    CardPainterHelper::drawCardCover(painter, m.cardRect, isSelected, hasThumb, thumb,
                                     qvariant_cast<QIcon>(index.data(Qt::DecorationRole)),
                                     isGrid, isWaitingThumb);

    // 2. 绘制右上角状态位 (Pin / Progress Ring / Checked)
    if (m_pinnedRole != -1 && m_managedRole != -1) {
        bool isPinned = index.data(m_pinnedRole).toBool();
        bool isManaged = index.data(m_managedRole).toBool();
        bool isDir = index.data(m_typeRole).toString() == "folder";
        double progress = (m_registrationProgressRole != -1) ? index.data(m_registrationProgressRole).toDouble() : -1.0;

        CardPainterHelper::drawStatusIndicators(painter, m.cardRect, isPinned, isManaged, isDir, progress);
    }

    // 3. 绘制扩展名徽章
    if (m_pathRole != -1) {
        QString ext = getExtensionString(index);
        CardPainterHelper::drawExtensionBadge(painter, m.cardRect, ext, hasThumb);
    }

    // 4. 绘制评级和彩色背景
    if (m_ratingRole != -1) {
        int rating = index.data(m_ratingRole).toInt();
        QString colorStr = (m_colorRole != -1) ? index.data(m_colorRole).toString() : "";

        CardPainterHelper::drawRatingStars(painter, m.banRect, m.starRect(4), rating, colorStr, isSelected);
    }

    // 5. 绘制空文件夹提示
    if (!isSelected && m_isEmptyRole != -1 && m_typeRole != -1) {
        if (index.data(m_typeRole).toString() == "folder" && index.data(m_isEmptyRole).toBool()) {
            CardPainterHelper::drawEmptyFolderBorder(painter, m.cardRect);
        }
    }

    // 6. 绘制截断文字
    drawFileNameText(painter, m.textRect, isSelected, index);
}
```

---

## 5. 修改边界声明【范围】

**本次方案涉及范围（只读分析）：**
- [ ] 模块/文件：`src/ui/ThumbnailDelegate.h` —— 职责梳理与重构方案制定。
- [ ] 模块/文件：`src/ui/ThumbnailDelegate.cpp` —— 职责梳理与重构方案制定。

**明确禁止越界修改的范围：**
- [ ] 物理扫描及底层数据库：`src/core/` / `src/meta/` —— 不涉及。

---

## 6. 实现准则与预警【核心】
1. **防止 QPainter 状态泄漏**：在委托拆分子函数（如 `CardPainterHelper::drawCardCover`）绘制时，必须严格遵守 `painter->save()` 和 `painter->restore()` 的成对匹配准则，防止画笔、笔刷或反锯齿等状态污染相邻单元格渲染。
2. **零开销与避免对象频繁构造**：由于视图在虚拟滚动时，`paint` 函数会被高频触发，原子化辅助类 `CardPainterHelper` 的函数应全部采用 `const` 引用传递（例如 `const QRect&`），严禁在内部生成临时大对象，必须确保纯计算和高频无状态极速绘制。
3. **保持重命名选区的稳定联动**：在拆分编辑器数据处理职责时，`QTimer::singleShot` 的延迟选中逻辑是应对 Qt 底层抢夺焦点的必要防线，必须予以保留，确保在拆分后依然具备稳定的文件名称首半截高亮交互体验。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 输入框清除功能 | 一律使用 Qt 原生 `setClearButtonEnabled(true)`。严禁通过 `addAction`、手动绘图或自定义按钮模拟清除逻辑。 | ✅ 本方案不涉及新增输入框；重命名编辑器使用的基类 QLineEdit 亦不对此做越权改动。 |
| 标题栏按钮样式 | 悬停：`#3E3E42`（`Style::HoverBackground`），按下：`#4E4E52`（`Style::PressedBackground`），严禁使用 rgba 蒙版。 | ✅ 本方案不涉及新按钮的样式定义。 |
| 实现前强制考古规则 | 在实现任何新 UI 组件前，必须在现有代码中搜索同类已实现的案例，并以该案例为唯一参考标准进行实现。 | ✅ 模块化重构旨在拆分现有 `ThumbnailDelegate`，所涉及的各项绘制均为将现有原子化提取，完全维持现有视觉考古对齐。 |

---

## 8. 待确认事项（可选）
- **性能评估确认**：将绘制方法提取到静态类后，编译器可通过 inline 进一步优化。用户是否需要将 `CardPainterHelper` 逻辑与 `CategoryDelegate`（分类面板委托）的部分通用星级/彩色背景绘制进行打通复用？
