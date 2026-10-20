# 实施方案：ColorWheel 三点递减比例归一化重构 (ColorWheel-1.md)

## 0. 起草前核查（Pre-Draft Evidence）

根据 AGENTS.md 3.6 第 1 关规范，实施本方案前物理核查证据如下：

### 0.1 已读文件清单
- `src/ui/components/ColorWheel.h`：已物理阅读，确认类定义与成员接口完整，公共接口完全只读冻结。
- `src/ui/components/ColorWheel.cpp`：已物理阅读（行 1~270），定位到手柄绘制（行 110~168）、鼠标点击命中（行 170~195）及悬停判定（行 255~270）。

### 0.2 签名原文摘录
本方案在绘制和交互中调用的外部 Qt 成员函数声明如下：
- `void QPainter::setPen(const QPen &pen);`
- `void QPainter::setBrush(const QBrush &brush);`
- `void QPainter::drawLine(const QPointF &p1, const QPointF &p2);`
- `void QPainter::drawEllipse(const QPointF &center, qreal rx, qreal ry);`
- `static QColor QColor::fromHsv(int h, int s, int v, int a = 255);`
- `qreal QLineF::length() const;`

### 0.3 既有 API 检索记录
- 检索关键词：`getHandleAPosition`、`getHandleCPosition`、`getCenterPoint`
- 检索结果：三点坐标计算函数已完备封装，绘制完全由 `paintEvent` 驱动，无多余冗余实现。

### 0.4 Memories.md 相关条款
- 遵守【契约锁】：对外公开的 `.h` 接口签名一律只读冻结，不破坏既有调用契约。
- 遵守【交互体验保障与视觉一致性】：三点比例和谐，保持充足的交互点击热区。

---

## 1. Overview（概述与解决的问题）

### 1.1 背景与解决的问题
此前手柄 A 缩至 3px 后，中间滑块圆圈（24px）与中心圆圈（8px）尺寸过大，导致比例严重失衡。
本方案按照全新定义的**“从外到内 A（外缘点） $\to$ B（滑杆点） $\to$ C（中心点）”**递减逻辑，按严格的 **`1.5 : 1.0 : 0.5`** 相对比例重构三点圆圈尺寸：
1. **A（外缘点）**：色相调节手柄，半径 **`9.0 px`**（直径 18px）；
2. **B（滑杆点）**：纯度调节滑块，半径 **`6.0 px`**（直径 12px）；
3. **C（中心点）**：色盘中心锚点，半径 **`3.0 px`**（直径 6px）；
4. **连线与线宽统一**：白色实线及各圆环描边线宽统一优化为 **`1.5 px`**；
5. **交互热区适配**：按新的几何半径调整点击与光标悬停容差，确保顺滑操作。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/components/ColorWheel.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### 修改文件：`src/ui/components/ColorWheel.cpp`

#### 修改点 1：连线与三点绘制逻辑（半径 9.0 : 6.0 : 3.0，线宽 1.5px）

<<<<<<< SEARCH
    // 2. 绘制连线：从 B 到 A 的白色连线 (宽 2px)
    p.save();
    QPen linePen(Qt::white, 2.0);
    linePen.setCapStyle(Qt::RoundCap);
    p.setPen(linePen);
    p.drawLine(posB, posA);
    p.restore();

    // 3. 绘制手柄 B (圆心，小空心圆，直径 8px，白色描边)
    p.save();
    p.setPen(QPen(Qt::white, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(posB, 4.0, 4.0);
    p.restore();

    // 辅助闭包：绘制手柄 A
    auto drawHandleA = [&]() {
        p.save();
        // 蓝框外环 (直径 6px，半径 3px，线宽 1.0px)
        p.setPen(QPen(QColor("#378ADD"), 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(posA, 3.0, 3.0);

        // 主体实心圆 (直径 4px，半径 2px，无描边，纯色色相填充)
        QColor pureHueColor = QColor::fromHsv(m_hue, 255, 255);
        p.setPen(Qt::NoPen);
        p.setBrush(pureHueColor);
        p.drawEllipse(posA, 2.0, 2.0);

        // 中心小白点 (直径 2px，半径 1.0px，纯白填充)
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawEllipse(posA, 1.0, 1.0);
        p.restore();
    };

    // 辅助闭包：绘制手柄 C (空心白圈，直径 24px，半径 12px，白色描边，无填充)
    auto drawHandleC = [&]() {
        p.save();
        p.setPen(QPen(Qt::white, 2.0));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(posC, 12.0, 12.0);
        p.restore();
    };
=======
    // 2. 绘制连线：从 C 到 A 的白色连线 (宽 1.5px)
    p.save();
    QPen linePen(Qt::white, 1.5);
    linePen.setCapStyle(Qt::RoundCap);
    p.setPen(linePen);
    p.drawLine(posB, posA);
    p.restore();

    // 3. 绘制点 C (中心点，空心小圆，半径 3.0px，白色描边 1.5px)
    p.save();
    p.setPen(QPen(Qt::white, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(posB, 3.0, 3.0);
    p.restore();

    // 辅助闭包：绘制点 A (外缘点，半径 9.0px，从外到内比例 1.5)
    auto drawHandleA = [&]() {
        p.save();
        // 蓝框外环 (直径 18px，半径 9.0px，线宽 1.5px)
        p.setPen(QPen(QColor("#378ADD"), 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(posA, 9.0, 9.0);

        // 主体实心圆 (直径 14px，半径 7.0px，纯色色相填充，细白描边 1.0px)
        QColor pureHueColor = QColor::fromHsv(m_hue, 255, 255);
        p.setPen(QPen(Qt::white, 1.0));
        p.setBrush(pureHueColor);
        p.drawEllipse(posA, 7.0, 7.0);

        // 中心小白点 (直径 3px，半径 1.5px，纯白实心)
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawEllipse(posA, 1.5, 1.5);
        p.restore();
    };

    // 辅助闭包：绘制点 B (滑杆点，半径 6.0px，从外到内比例 1.0，白色描边 1.5px，无填充)
    auto drawHandleC = [&]() {
        p.save();
        p.setPen(QPen(Qt::white, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(posC, 6.0, 6.0);
        p.restore();
    };
>>>>>>> REPLACE

#### 修改点 2：点击命中容差与光标判定适配

<<<<<<< SEARCH
        // 判定手柄点击命中 (手柄 A 保留 10px 交互容差热区，手柄 C 保持 18px)
        bool hitA = (distToA <= 10.0);
        bool hitC = (distToC <= 12.0 + 6.0);
=======
        // 判定手柄点击命中 (点 A 半径 9px + 5px 容差；点 B 半径 6px + 6px 容差)
        bool hitA = (distToA <= 9.0 + 5.0);
        bool hitC = (distToC <= 6.0 + 6.0);
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (distA <= 10.0 || distC <= 12.0 + 4.0) {
        setCursor(Qt::PointingHandCursor);
    } else {
=======
    if (distA <= 14.0 || distC <= 12.0) {
        setCursor(Qt::PointingHandCursor);
    } else {
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证
在 Visual Studio 开发者终端中运行：
```powershell
cmake --build "G:\C++\QuarkMeta\QuarkMeta_Build\x64-release"
```
确保 `ColorWheel.cpp.obj` 编译通过。

### 4.2 视觉与交互运行验证
1. 打开调色面板，查看色轮控件：
   - 确认外缘点 A 半径为 9px，滑杆点 B 半径为 6px，中心点 C 半径为 3px；
   - 视觉比例呈现严格的 `1.5 : 1.0 : 0.5` 层次递减；
   - 连线粗细与空心圆描边均匀一致（均为 1.5px）；
2. 拖拽点 A 能够精准顺畅旋转色相；
3. 拖拽点 B 能够平滑沿着连线滑动改变纯度；
4. 鼠标悬停在两点附近时能精准触发手型光标。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- 本修改纯粹调整现有 `ColorWheel.cpp` 内部的绘图几何参数；
- 不改变既有的数据属性、事件分发及数据流动路径，未另起炉灶。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用成员 / API | 所在头文件 | 物理精确签名 | 核查状态 |
|---|---|---|---|
| `drawLine` | `<QPainter>` | `void QPainter::drawLine(const QPointF &p1, const QPointF &p2);` | 100% 吻合 |
| `drawEllipse` | `<QPainter>` | `void QPainter::drawEllipse(const QPointF &center, qreal rx, qreal ry);` | 100% 吻合 |
| `fromHsv` | `<QColor>` | `static QColor QColor::fromHsv(int h, int s, int v, int a = 255);` | 100% 吻合 |
| `length` | `<QLineF>` | `qreal QLineF::length() const;` | 100% 吻合 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- 本方案不新增、修改或删除任何 `#include` 头文件。
- 所使用的 `<QPainter>`、`<QColor>`、`<QLineF>` 均在 `ColorWheel.cpp` 原有头文件包含链中具备完整类型定义，不存在隐式包含断裂风险。

---

## 8. Compliance Self-Check Table（合规核对表）

根据 AGENTS.md 3.6 第 2 关规范，交付前自检结果如下：

| 检查项 | 来源条款 | 自检结果 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | `ColorWheel-1.md` 映射 `ColorWheel` 类（递增后缀） |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/ColorWheel-1.md` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 第 3 节 SEARCH 块与 `ColorWheel.cpp` 现有代码 100% 吻合 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建类，`ColorWheel` 已在 CMakeLists 中注册 |
| 第 1~9 章齐全 | 3.4 | 满足 | 方案包含 0~9 所有必备章节 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 见第 0.2 节与第 6 节核查表 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未变动任何 `#include` 行，见第 7 节 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 见第 5 节 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 纯 QPainter 矢量几何绘制，无内联 QSS |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 纯标准 Qt 控件重载绘制与标准鼠标事件处理 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 单控件单值局部拖拽更新 |
| 迁移的所有数值原样照抄 | 4.4 | 满足 | 严格按共识的 1.5 : 1.0 : 0.5 比例配置 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 交互逻辑、坐标变换均原样保留 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅影响 `ColorWheel.cpp`，无多余文件触碰 |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

针对三点递减比例设定核对如下：

| 点位角色 | 比例要求 | 对应物理半径 | 对应物理直径 | 核对状态 |
|---|---|---|---|---|
| **A（外缘点）** | `1.5` | **`9.0 px`** | `18.0 px` | 严格吻合 |
| **B（滑杆点）** | `1.0` | **`6.0 px`** | `12.0 px` | 严格吻合 |
| **C（中心点）** | `0.5` | **`3.0 px`** | `6.0 px` | 严格吻合 |
| **连线与描边** | 统一标准 | **`1.5 px`** | - | 结构一致 |
