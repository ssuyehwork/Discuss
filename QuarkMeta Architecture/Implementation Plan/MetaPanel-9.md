# MetaPanel 实施方案 (MetaPanel-9.md)

### 【第 1 关：起草前核查（Pre-Draft Evidence）】
1. **已读文件清单**：
   - `src/ui/MetaPanel.h`（已用工具 `read_file` 物理读取）
   - `src/ui/MetaPanel.cpp`（已用工具 `read_file` 物理读取）
   - `resources/style.qss`（已用工具 `run_in_bash_session` / `grep` 物理读取）
2. **签名原文摘录**：
   - `void setImagePreview(const QPixmap& pixmap);`（`MetaPanel.h` 第 47 行）
   - `void setMultiSelectionPreview(const QList<MetaPreviewItem>& items);`（`MetaPanel.h` 第 49 行）
   - `void setDefaultIconPreview(const QIcon& icon);`（`MetaPanel.h` 第 48 行）
   - `QPixmap renderThumbnailLayer(const QPixmap& pixmap)`（`MetaPanel.cpp` 匿名命名空间第 37 行）
   - `QPixmap renderIconLayer(const QIcon& icon, qreal dpr)`（`MetaPanel.cpp` 匿名命名空间第 61 行）
3. **既有 API 检索记录**：
   - 检索关键词：`m_topPreviewBox` / `m_lblImagePreview` / `m_pathEdit` / `pathL`
   - 结果：`m_topPreviewBox` 和 `m_lblImagePreview` 控制顶部预览图的大小及呈现；`pathL` 控制物理路径区域组件的垂直布局间距。
4. **Memories.md 相关条款**：
   - 重构属性参数绝对照抄与零篡改铁律（除了用户明确指定的微调数值外，其余所有控件、间距、字体及颜色参数原样保持一致）。

---

## 1. Overview（概述与解决的问题）

本实施方案用于对元数据面板（`MetaPanel`）的以下两项具体的属性设计参数进行精细化微调，以满足最新的 UI 渲染与布局规范：
1. **顶部预览区画幅限制**：`m_topPreviewBox` 保持 `200px x 200px` 物理基座尺寸不变，但缩略图、文件图标及多选卡片堆叠的最大显示范围严格限制在 **`190px x 190px`** 像素之内（更新 `renderThumbnailLayer`、`renderIconLayer` 画布生成尺寸以及 `m_lblImagePreview` 的 `maximumSize`）。
2. **物理路径区域垂直间距锁定**：修改 `m_pathEdit` 所在的垂直布局器 `pathL` 的 `spacing` 属性为固定的 **`5px`**，确保无论路径长度如何自适应换行变长，`MetaPathEdit` 与下方按钮组 (`BtnCopyPath` / `BtnOpenLocation`) 之间的切缝间距恒定保持为 `5px`。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 修改点 1：`src/ui/MetaPanel.cpp` - 调整画布渲染函数中的尺寸限制 (200px -> 190px)

<<<<<<< SEARCH
QPixmap renderThumbnailLayer(const QPixmap& pixmap) {
    QSize canvasSize(200, 200);
    QPixmap canvas(canvasSize);
    canvas.fill(Qt::transparent);

    QSize targetSize(200, 200);
    QPixmap scaled = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    {
        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        int x = (200 - scaled.width()) / 2;
        int y = (200 - scaled.height()) / 2;

        QPainterPath path;
        path.addRoundedRect(QRectF(x, y, scaled.width(), scaled.height()), 4.0, 4.0);
        painter.setClipPath(path);
        painter.drawPixmap(x, y, scaled);
    }
    return canvas;
}

QPixmap renderIconLayer(const QIcon& icon, qreal dpr) {
    QSize logicalCanvasSize(200, 200);
    QSize physicalCanvasSize = logicalCanvasSize * dpr;

    QPixmap canvas(physicalCanvasSize);
    canvas.fill(Qt::transparent);
    canvas.setDevicePixelRatio(dpr);

    QPixmap iconPx = CardPainterHelper::renderDefaultIcon(icon, logicalCanvasSize, dpr);
    if (!iconPx.isNull()) {
        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QSizeF iconLogicalSize = iconPx.deviceIndependentSize();
        int x = qRound((200.0 - iconLogicalSize.width()) / 2.0);
        int y = qRound((200.0 - iconLogicalSize.height()) / 2.0);

        painter.drawPixmap(x, y, iconPx);
    }
    return canvas;
}
=======
QPixmap renderThumbnailLayer(const QPixmap& pixmap) {
    QSize canvasSize(190, 190);
    QPixmap canvas(canvasSize);
    canvas.fill(Qt::transparent);

    QSize targetSize(190, 190);
    QPixmap scaled = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    {
        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        int x = (190 - scaled.width()) / 2;
        int y = (190 - scaled.height()) / 2;

        QPainterPath path;
        path.addRoundedRect(QRectF(x, y, scaled.width(), scaled.height()), 4.0, 4.0);
        painter.setClipPath(path);
        painter.drawPixmap(x, y, scaled);
    }
    return canvas;
}

QPixmap renderIconLayer(const QIcon& icon, qreal dpr) {
    QSize logicalCanvasSize(190, 190);
    QSize physicalCanvasSize = logicalCanvasSize * dpr;

    QPixmap canvas(physicalCanvasSize);
    canvas.fill(Qt::transparent);
    canvas.setDevicePixelRatio(dpr);

    QPixmap iconPx = CardPainterHelper::renderDefaultIcon(icon, logicalCanvasSize, dpr);
    if (!iconPx.isNull()) {
        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QSizeF iconLogicalSize = iconPx.deviceIndependentSize();
        int x = qRound((190.0 - iconLogicalSize.width()) / 2.0);
        int y = qRound((190.0 - iconLogicalSize.height()) / 2.0);

        painter.drawPixmap(x, y, iconPx);
    }
    return canvas;
}
>>>>>>> REPLACE

### 修改点 2：`src/ui/MetaPanel.cpp` - 调整 initUi() 中预览图控件尺寸与路径区域布局间距

<<<<<<< SEARCH
    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setMaximumSize(200, 200);
    m_lblImagePreview->setObjectName("MetaImagePreview");
=======
    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setMaximumSize(190, 190);
    m_lblImagePreview->setObjectName("MetaImagePreview");
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 8. 物理路径区
    QWidget* pathBox = new QWidget(m_container);
    QVBoxLayout* pathL = new QVBoxLayout(pathBox);
    pathL->setContentsMargins(0, 0, 0, 0);
    pathL->setSpacing(6);
    pathL->setSizeConstraint(QLayout::SetMinimumSize);
=======
    // 8. 物理路径区
    QWidget* pathBox = new QWidget(m_container);
    QVBoxLayout* pathL = new QVBoxLayout(pathBox);
    pathL->setContentsMargins(0, 0, 0, 0);
    pathL->setSpacing(5);
    pathL->setSizeConstraint(QLayout::SetMinimumSize);
>>>>>>> REPLACE

### 修改点 3：`src/ui/MetaPanel.cpp` - 调整预览图更新槽函数中的控件最大尺寸设置 (200px -> 190px)

<<<<<<< SEARCH
void MetaPanel::setImagePreview(const QPixmap& pixmap) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        QPixmap canvas = renderThumbnailLayer(pixmap);
        m_lblImagePreview->setPixmap(canvas);
        m_lblImagePreview->setMaximumSize(QSize(200, 200));

        m_lblImagePreview->show();
        if (m_topPreviewBox) {
            m_topPreviewBox->show();
        }
    }
    m_adjustTimer->start();
}
=======
void MetaPanel::setImagePreview(const QPixmap& pixmap) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        QPixmap canvas = renderThumbnailLayer(pixmap);
        m_lblImagePreview->setPixmap(canvas);
        m_lblImagePreview->setMaximumSize(QSize(190, 190));

        m_lblImagePreview->show();
        if (m_topPreviewBox) {
            m_topPreviewBox->show();
        }
    }
    m_adjustTimer->start();
}
>>>>>>> REPLACE

<<<<<<< SEARCH
    std::reverse(validLayers.begin(), validLayers.end());

    qreal dpr = devicePixelRatioF();
    QSize logicalCanvasSize(200, 200);
    QSize physicalCanvasSize = logicalCanvasSize * dpr;

    QPixmap canvas(physicalCanvasSize);
    canvas.fill(Qt::transparent);
    canvas.setDevicePixelRatio(dpr);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    for (int k = validLayers.size() - 1; k >= 0; --k) {
        const QPixmap& layer = validLayers[k];

        // 🎯 核心修复：多选时所有卡片（包含最顶层 k=0 卡片）全部参与交替倾斜，确保顶层卡片本身倾斜，一眼直观辨识多选状态
        double stepMultiplier = std::ceil((k + 1) / 2.0);
        double baseAngle = kMultiPreviewAngleStep * stepMultiplier;
        double angle = (k % 2 == 0) ? -baseAngle : baseAngle;
        double scale = 1.0 - (0.04 * k); // 逐层微缩 4%，露出下层边角
        double offsetY = -3.0 * k;      // 逐层向上微平移 3px

        painter.save();
        painter.translate(100.0, 100.0 + offsetY);
        painter.rotate(angle);
        if (scale < 1.0) {
            painter.scale(scale, scale);
        }
        painter.drawPixmap(QPointF(-100.0, -100.0), layer);
        painter.restore();
    }
=======
    std::reverse(validLayers.begin(), validLayers.end());

    qreal dpr = devicePixelRatioF();
    QSize logicalCanvasSize(190, 190);
    QSize physicalCanvasSize = logicalCanvasSize * dpr;

    QPixmap canvas(physicalCanvasSize);
    canvas.fill(Qt::transparent);
    canvas.setDevicePixelRatio(dpr);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    for (int k = validLayers.size() - 1; k >= 0; --k) {
        const QPixmap& layer = validLayers[k];

        // 🎯 核心修复：多选时所有卡片（包含最顶层 k=0 卡片）全部参与交替倾斜，确保顶层卡片本身倾斜，一眼直观辨识多选状态
        double stepMultiplier = std::ceil((k + 1) / 2.0);
        double baseAngle = kMultiPreviewAngleStep * stepMultiplier;
        double angle = (k % 2 == 0) ? -baseAngle : baseAngle;
        double scale = 1.0 - (0.04 * k); // 逐层微缩 4%，露出下层边角
        double offsetY = -3.0 * k;      // 逐层向上微平移 3px

        painter.save();
        painter.translate(95.0, 95.0 + offsetY);
        painter.rotate(angle);
        if (scale < 1.0) {
            painter.scale(scale, scale);
        }
        painter.drawPixmap(QPointF(-95.0, -95.0), layer);
        painter.restore();
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
void MetaPanel::setDefaultIconPreview(const QIcon& icon) {
    if (!m_lblImagePreview) return;
    if (icon.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        QPixmap canvas = renderIconLayer(icon, devicePixelRatioF());
        m_lblImagePreview->setPixmap(canvas);
        m_lblImagePreview->setMaximumSize(QSize(200, 200));

        m_lblImagePreview->show();
        if (m_topPreviewBox) {
            m_topPreviewBox->show();
        }
    }
    m_adjustTimer->start();
}
=======
void MetaPanel::setDefaultIconPreview(const QIcon& icon) {
    if (!m_lblImagePreview) return;
    if (icon.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        QPixmap canvas = renderIconLayer(icon, devicePixelRatioF());
        m_lblImagePreview->setPixmap(canvas);
        m_lblImagePreview->setMaximumSize(QSize(190, 190));

        m_lblImagePreview->show();
        if (m_topPreviewBox) {
            m_topPreviewBox->show();
        }
    }
    m_adjustTimer->start();
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译验证**：
   在终端或构建系统中执行常规 CMake / MSVC 编译。
2. **UI 细节核对**：
   - 选中任何文件或文件夹，观察元数据面板顶部的预览图片/图标，确认其最大渲染画幅已被限制在 `190px x 190px` 范围之内，而外围的 `m_topPreviewBox` 保持居中支撑。
   - 检查物理路径区，调整路径长度（使其自适应折行为多行），确认 `MetaPathEdit` 与 "复制路径" / "打开位置" 按钮之间的间距始终为刚性 `5px`。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

本次修改仅涉及元数据面板内部视图控件的呈现尺寸与布局 spacing 微调，完全复用既有的 `MetaPanel` 自适应绘制逻辑与布局关系，未修改任何通用行为接口，不存在另起炉灶行为。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

- `void setImagePreview(const QPixmap& pixmap);`（`MetaPanel.h` 第 47 行）
- `void setMultiSelectionPreview(const QList<MetaPreviewItem>& items);`（`MetaPanel.h` 第 49 行）
- `void setDefaultIconPreview(const QIcon& icon);`（`MetaPanel.h` 第 48 行）

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

本次修改仅修改 `src/ui/MetaPanel.cpp` 中的局部物理数值与布局间距，不增加、删除或修改任何 `#include` 头文件包含行。

---

## 8. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文档标题 (MetaPanel-9.md) |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 第 3 章全部 SEARCH 块 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建或修改 Q_OBJECT 声明 |
| 第 1~9 章齐全 | 3.4 | 满足 | 第 1~9 章完整覆盖 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 第 6 章 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未修改头文件包含 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 第 5 章 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 满足 | 布局用 `setSpacing(5)`，样式走 QSS |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 均未采用 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 不涉及文件批量 IO |
| 迁移的所有数值原样照抄 | 4.4 | 满足 | 第 9 章 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 仅精确落实用户指定的微调数值 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅影响 `src/ui/MetaPanel.cpp` |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

| 参数/数值描述 | 原始数值 | 新数值 | 变更理由/依据 |
|---|---|---|---|
| 缩略图画布物理尺寸 (`canvasSize` / `targetSize`) | `200 x 200` | `190 x 190` | 用户明确指示：显示图标/缩略图范围不可大于 190x190px |
| 图标画布逻辑尺寸 (`logicalCanvasSize`) | `200 x 200` | `190 x 190` | 用户明确指示：显示图标/缩略图范围不可大于 190x190px |
| `m_lblImagePreview` 最大物理尺寸 (`maximumSize`) | `200 x 200` | `190 x 190` | 用户明确指示：显示图标/缩略图范围不可大于 190x190px |
| 多选卡片堆叠中心平移坐标 (`translate`) | `(100.0, 100.0)` | `(95.0, 95.0)` | 适配 `190x190` 画布的几何中心点 (`190 / 2 = 95`) |
| 物理路径区域垂直布局间距 (`pathL->setSpacing`) | `6px` | `5px` | 用户明确指示：BtnCopyPath / BtnOpenLocation 与 MetaPathEdit 保持 5px 间距 |
