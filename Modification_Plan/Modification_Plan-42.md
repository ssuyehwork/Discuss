# 列表视图最左侧微型缩略图卡片自适应显示与大图渲染优化 —— Modification_Plan-42.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在当前版本的列表视图中，当名称列（第 0 列，对应用户原话：“列表视图最左侧缩略图卡片”）开启微型卡片预览模式（`m_drawMiniCards == true`）时，图形和 SVG 等文件的缩略图（对应用户原话：“图片的缩略图，如psd、jpg等”）无法自适应填满圆角矩形，而是缩小成常规的小图标显示，在卡片内部留下了大量无用的灰色背景，视觉效果差（对应用户原话：“采取图标显示的方式，不是我期望的”）。用户期望列表视图中的缩略图能够对齐 “FERREX-META” 版本的圆角卡片渲染效果，平滑、清晰且自适应填满地在卡片内部展现（对应用户原话：“缩略图卡片显示的样子，也是我期望的”）。

## 2. 问题定位
当前版本（`src/`）与 `FERREX-META` 版本中列表视图在卡片里显示缩略图的代码对比结果如下：
1. **渲染条件和类型检测限制**：
   在 `src/ui/TreeItemDelegate.h` 中，自绘 Column 0（对应用户原话：“列表视图最左侧缩略图卡片”）的代码中，使用了如下类型检测逻辑：
   ```cpp
   QVariant decoData = index.data(Qt::DecorationRole);
   if (decoData.canConvert<QPixmap>()) {
       QPixmap thumb = decoData.value<QPixmap>();
       // ... 平滑缩放并绘制缩略图大图 ...
   } else {
       QIcon icon = qvariant_cast<QIcon>(decoData);
       if (!icon.isNull()) {
           int iconSize = qRound(side * 0.6); // 普通小图标强制缩小到 0.6 比例
           // ... 绘制小图标 ...
       }
   }
   ```
2. **数据源类型不一致**：
   在当前版本中，数据模型层 `FerrexVirtualDbModel::data`（`src/ui/ContentPanel.cpp`）返回缓存好的缩略图时，其数据类型为 **`QIcon`**：
   ```cpp
   mutableThis->m_iconCache.insert(cacheKey, new QIcon(icon));
   // ...
   if (cached) return *cached; // 返回一个 QIcon 类型
   ```
   这导致 `decoData.canConvert<QPixmap>()` 永远为 `false`。
3. **退化结果**：
   因为条件判定失败，本来已经获取到的高精度图形缩略图直接坠入 `else` 分支，被作为常规图标强行缩小到了 `0.6` 的比例（对应用户原话：“采取图标显示的方式”），并局限在暗灰色圆角矩形内部一小块区域，没有展开绘制。
4. **修复方向**：
   在 `TreeItemDelegate::paint` 中增强检测手段：如果 `decoData` 能转换为 `QPixmap` **或者**能转换为 `QIcon` 且对应的 `QIcon` 非空，则都尝试将其转换为 `QPixmap` 进行满幅缩放绘制。如果是普通文件类型（非图形、非SVG等不具备缩略图的文件），则依然降级绘制小图标。通过这种方式兼容模型层返回的 `QIcon` 缩略图缓存。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 图一 是FERREX-META版本列表缩略图卡片显示的样子，也是我期望的；图二 是当前版本列表缩略图在卡片里显示的样子，显示的方式好像采取图标显示的方式，不是我期望的 | 修改 `src/ui/TreeItemDelegate.h` 自绘最左侧微型卡片时，确保其不仅能识别 `QPixmap` 还能识别并还原模型层返回的 `QIcon` 缩略图大图（不进行 0.6 倍的强行缩小），使其自适应卡片居中并平滑缩放填满。 | ✅ 一致 |

## 4. 详细解决方案
在 `src/ui/TreeItemDelegate.h` 的名称列绘制逻辑中，对 QVariant `decoData` 进行以下重构：

```cpp
<<<<<<< SEARCH
            // 2. 图像/图标平滑居中绘制（最左侧看片核心逻辑）
            QVariant decoData = index.data(Qt::DecorationRole);
            if (decoData.canConvert<QPixmap>()) {
                QPixmap thumb = decoData.value<QPixmap>();
                if (!thumb.isNull()) {
                    QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int x = squareRect.center().x() - scaled.width() / 2;
                    int y = squareRect.center().y() - scaled.height() / 2;
                    painter->drawPixmap(x, y, scaled);
                }
            } else {
                QIcon icon = qvariant_cast<QIcon>(decoData);
                if (!icon.isNull()) {
                    int iconSize = qRound(side * 0.6);
                    QRect iconRect(squareRect.center().x() - iconSize / 2,
                                   squareRect.center().y() - iconSize / 2,
                                   iconSize, iconSize);
                    icon.paint(painter, iconRect);
                }
            }
=======
            // 2. 图像/图标平滑居中绘制（最左侧看片核心逻辑，对应用户原话：“最左侧缩略图卡片在卡片里是如何显示的”）
            QVariant decoData = index.data(Qt::DecorationRole);
            bool isThumbnailDrawn = false;

            // 优先判定是否为有效的 QPixmap 格式缩略图 (对应用户原话：“缩略图卡片显示的样子”)
            if (decoData.canConvert<QPixmap>()) {
                QPixmap thumb = decoData.value<QPixmap>();
                if (!thumb.isNull()) {
                    QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int x = squareRect.center().x() - scaled.width() / 2; // (对应用户原话：“在卡片里是如何显示的”)
                    int y = squareRect.center().y() - scaled.height() / 2;
                    painter->drawPixmap(x, y, scaled);
                    isThumbnailDrawn = true;
                }
            }

            // 如果不能直接转换，或者 Pixmap 为空，尝试从 QIcon 转换（打通模型缓存 QIcon 到 Pixmap 平滑大图绘制链路）
            if (!isThumbnailDrawn && decoData.canConvert<QIcon>()) {
                QIcon icon = qvariant_cast<QIcon>(decoData);
                if (!icon.isNull()) {
                    // 判断当前文件是否属于图形、视频或 SVG 格式 (对应用户原话：“图片的缩略图，如psd、jpg等”)
                    QModelIndex idx0 = index.model()->index(index.row(), 0);
                    QString ext = index.model()->data(idx0, TypeRole).toString(); // 读取文件或后缀类型
                    // 为保证最左侧微卡片圆角预览大图的兼容性，直接提取该 QIcon 最大分辨率的 pixmap 作为缩略图资产
                    // 并且只要它是图形/SVG文件或者该 icon 的实际物理分辨率大于普通小图标尺寸，就使用大卡片满幅自适应绘制
                    QSize actualSize = icon.actualSize(QSize(128, 128));
                    bool isGraphicFile = false;
                    QString path = index.model()->data(idx0, PathRole).toString();
                    if (!path.isEmpty()) {
                        QString suffix = QFileInfo(path).suffix().toLower();
                        isGraphicFile = UiHelper::isGraphicsFile(suffix) || suffix == "svg";
                    }

                    if (isGraphicFile || actualSize.width() >= 48) {
                        QPixmap thumb = icon.pixmap(squareRect.size()); // (对应用户原话：“缩略图卡片显示的样子，也是我期望的”)
                        if (!thumb.isNull()) {
                            QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                            int x = squareRect.center().x() - scaled.width() / 2;
                            int y = squareRect.center().y() - scaled.height() / 2;
                            painter->drawPixmap(x, y, scaled);
                            isThumbnailDrawn = true;
                        }
                    }
                }
            }

            // 退化兜底：如果不是图像缩略图（如纯文本、二进制等），则绘制常规默认系统图标，强行按 0.6 比例缩小
            if (!isThumbnailDrawn) {
                QIcon icon = qvariant_cast<QIcon>(decoData);
                if (!icon.isNull()) {
                    int iconSize = qRound(side * 0.6); // (对应用户原话：“显示的方式好像采取图标显示的方式，不是我期望的”)
                    QRect iconRect(squareRect.center().x() - iconSize / 2,
                                   squareRect.center().y() - iconSize / 2,
                                   iconSize, iconSize);
                    icon.paint(painter, iconRect);
                }
            }
>>>>>>> REPLACE
```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/TreeItemDelegate.h`（第 75 行至 105 行左右，负责微型卡片自绘逻辑）

**明确禁止越界修改的范围：**
- [ ] 模块/文件：`src/ui/ContentPanel.cpp` 中的 `FerrexVirtualDbModel` —— 不修改其 QCache 结构与后台线程的缩略图加载及保存逻辑。
- [ ] 模块/文件：`src/ui/ListResultView.cpp` —— 不修改其结构与视图生命期控制链。

## 6. 实现准则与预警【核心】
1. **依赖的头文件**：必须确保 `src/ui/TreeItemDelegate.h` 中已经包含 `#include "UiHelper.h"`。
2. **作用域与命名空间**：修改必须限定在 `ArcMeta` 命名空间内部，使用 `UiHelper::isGraphicsFile` 进行后缀名安全匹配，防止引入外部未定义的标识符。
3. **性能开销**：从 `QIcon` 获取 `QPixmap` 时，传入 `squareRect.size()`（一般为 24px 至 30px）进行按需提取，杜绝重复调用大图导致主线程卡顿，以实现渲染零开销、开箱即用。
4. **安全保护**：添加空值检测 `if (!thumb.isNull())` 避免由于图像解析失败导致的空画笔异常。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 缩略图平滑加载 | 针对图形文件（图像、SVG等），在异步加载缩略图期间，`data()` 接口必须返回空图标 (`QIcon()`)。`ThumbnailDelegate` 必须通过检测空图标状态，在单元格区域绘制轻量的灰色圆角矩形 (`#3A3A3A`) 作为占位背景，确保过渡平滑。 | ✅ 符合（本次在列表模式 `TreeItemDelegate` 中自绘圆角容器，当加载完成并返回 `QIcon` 时，我们安全提取并平滑拉伸，未破环异步加载空图标的占位规则） |

## 8. 待确认事项
无。
