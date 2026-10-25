# 文件夹分组标题折叠组件点击响应区域精准约束重构实施方案

- **Base Commit Hash**: `3e304b14270588e6d40fe9c9cbf79a617f73a932`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

### 已读物理证据清单
1. **物理读取文件列表**：
   - `src/ui/JustifiedView.h`
   - `src/ui/JustifiedView.cpp`
2. **既有逻辑与绘图坐标核查日志**：
   - `JustifiedView::paintEvent` line 374-386：
     - `int leftX = geo.rect.left() + 10;`
     - `int textW = fm.horizontalAdvance(geo.headerText);`
     - `int iconX = leftX + textW + 6;`
     - `int iconSize = 14;`
     - 可折叠组件实际绘制区域 X 轴为 `[leftX, leftX + textW + 6 + 14]`（即 `[leftX, leftX + textW + 20]`），增加 10px 容差热区后有效范围为 `[leftX, leftX + textW + 30]`。
   - `JustifiedView::mousePressEvent` line 238-250：
     - 目前逻辑仅判断 `pressIdx.data(SectionHeaderRole).toBool()`，点击整行标题包围盒（全宽 `containerWidth`）即触发折叠/展开，导致点击右侧空白处（红框区域）也误触发折叠。

---

## Chapter 1: Overview（概述与根因分析）

在 QuarkMeta 的网格/自适应视图（`JustifiedView`）中，顶部的文件夹分组标题栏（例如“文件夹 (6) v”）具备折叠/展开功能。在当前的代码架构中，标题行的交互包围盒为容器的全宽度（`QRect(margin, currentY, containerWidth, headerH)`），导致用户在点击与文件夹标题同行的右侧大片空白区域时，也会触发文件夹的折叠/展开。

为了解决这一不符合直觉的误触问题，本方案精准约束文件夹标题栏折叠组件的点击响应区域：只有当鼠标点击落在**“文件夹 (N)”文本与折叠箭头图标**所占据的视觉热区（`[leftX, leftX + textW + 30]`）范围内时，才触发折叠/展开；点击超出该范围的同行右侧空白处时，将其降级为标准空白区域点击（可触发框选或清除选中状态），绝不误触发折叠。

---

## Chapter 2: Modified Files List（影响文件清单）

* **修改的文件**：
  - `src/ui/JustifiedView.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 修改 `src/ui/JustifiedView.cpp` 中的 `mousePressEvent`，加入文件夹折叠组件点击 X 轴包围盒计算

```
<<<<<<< SEARCH
void JustifiedView::mousePressEvent(QMouseEvent* event) {
    QModelIndex pressIdx = indexAt(event->pos());
    if (pressIdx.isValid() && pressIdx.data(SectionHeaderRole).toBool()) {
        if (event->button() == Qt::LeftButton) {
            if (pressIdx.data(SectionKindRole).toInt() == 1) {
                SectionProxyModel* secModel = m_sectionModel ? m_sectionModel : qobject_cast<SectionProxyModel*>(model());
                if (secModel) {
                    secModel->setFolderCollapsed(!secModel->isFolderCollapsed());
                }
            }
        }
        event->accept();
        return;
    }
=======
void JustifiedView::mousePressEvent(QMouseEvent* event) {
    QModelIndex pressIdx = indexAt(event->pos());
    if (pressIdx.isValid() && pressIdx.data(SectionHeaderRole).toBool()) {
        if (event->button() == Qt::LeftButton) {
            bool inInteractiveBounds = true;
            if (pressIdx.data(SectionKindRole).toInt() == 1) {
                int visualIdx = pressIdx.row();
                if (visualIdx >= 0 && visualIdx < static_cast<int>(m_geometries.size())) {
                    const auto& geo = m_geometries[visualIdx];
                    QFont font("Microsoft YaHei", 10, QFont::Bold);
                    QFontMetrics fm(font);
                    int leftX = geo.rect.left() + 10;
                    int textW = fm.horizontalAdvance(geo.headerText);
                    int interactiveMaxX = leftX + textW + 30; // 文本宽度 + 箭头图标 + 容差 Padding
                    if (event->pos().x() < leftX || event->pos().x() > interactiveMaxX) {
                        inInteractiveBounds = false;
                    }
                }
            }

            if (inInteractiveBounds) {
                if (pressIdx.data(SectionKindRole).toInt() == 1) {
                    SectionProxyModel* secModel = m_sectionModel ? m_sectionModel : qobject_cast<SectionProxyModel*>(model());
                    if (secModel) {
                        secModel->setFolderCollapsed(!secModel->isFolderCollapsed());
                    }
                }
                event->accept();
                return;
            } else {
                pressIdx = QModelIndex(); // 超出折叠组件热区，视为点击同行空白区域，放行给后续框选/取消选中逻辑
            }
        }
    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 分步构建命令
```bash
cmake --build build --config Release -j 8
```

### 运行时验收标准
1. **折叠组件精准点击验证**：在含有文件夹和文件的主视口中，鼠标点击“文件夹 (N)”文字或紧随其后的箭头图标，文件夹分组顺畅进行折叠与展开。
2. **同行空白区域防误触验证**：鼠标点击“文件夹 (N)”同行右侧大片空白区域（截图红框区域），**绝对不触发折叠/展开**，且开始框选拉框或清除项目选中状态。
3. **不同屏幕缩放/长文本自适应验证**：文件夹名称或数量发生变化（如“文件夹 (128)”）时，热区根据 `QFontMetrics::horizontalAdvance` 动态调整，始终完美覆盖组件文本与箭头范围。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **精准几何计算**：复用 `JustifiedView::paintEvent` 中完全一致的 `QFontMetrics` 文本宽度测量逻辑（`fm.horizontalAdvance`），保证点击响应热区与视觉渲染 1:1 完美契合。
2. **既有数据结构直通**：直接利用 `m_geometries` 中的 `ItemGeometry` 缓存，无需冗余再次请求 Model 进行数据解包。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 观察者 / 视图 | 刷新方向 |
| :--- | :--- | :--- | :--- |
| **折叠组件绘制与响应几何体** | `JustifiedView::m_geometries` | `paintEvent` / `mousePressEvent` | 同步几何尺寸计算与坐标拦截 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- 无任何新增或修改的 `.h` 接口签名，所有逻辑内聚于 `JustifiedView.cpp` 私有事件处理函数中，向后兼容性 100%。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 未修改任何 `#include` 依赖关系，类型完整性与头文件包含链保持不变。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 详细说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否未对源代码进行直接修改 | **满足** | 本方案为 Markdown 实施方案，存放于 `Implementation Plan/` 目录 |
| 2 | 是否回应了架构三问 | **满足** | Chapter 1 阐明了热区精准裁剪与空白点击降级机制 |
| 3 | 是否无 C++ 硬编码内联 QSS | **满足** | 零 QSS 修改 |
| 4 | 是否未动 Freeze 的公共接口 | **满足** | 无任何 `.h` 接口修改 |
| 5 | 是否包含 SEARCH 块真实性核查 | **满足** | 所有 SEARCH 块均与目标 Commit 物理源码 1:1 匹配 |
| 6 | 是否包含构建与运行时验收标准 | **满足** | Chapter 4 提供编译命令与运行时测试标准 |
| 7 | 是否包含数值零篡改表 | **满足** | Chapter 10 提供完整对照表 |
| 8 | 语言规范是否合规 | **满足** | 正文及注释严格遵守中文规范 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 属性 / 交互点 | 原始配置 | 重构后配置 | 一致性 / 变化确认 |
| :--- | :--- | :--- | :--- |
| **点击折叠文本/箭头图标** | 触发折叠/展开 | 触发折叠/展开 | **100% 保持一致** |
| **点击同行右侧空白区域** | 误触发折叠/展开 | **不触发折叠，降级为空白区域点击（框选/取消选中）** | **解决误触缺陷，符合用户期望** |
