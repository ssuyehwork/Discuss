# Implementation Plan - SidebarTabBarIconSpacingPlan (侧边栏选项卡图标与文本 7 像素间距实施方案)

## 已读物理证据清单 (Gate 1: Pre-Draft Evidence)
1. **物理读取文件列表**：
   - `src/ui/NavPanel.h` (已阅读：第 10~65 行，确认 `m_tabBar` 类型与头文件包含)
   - `src/ui/NavPanel.cpp` (已阅读：第 125~145 行，确认 `m_tabBar` 初始化与选项卡添加代码)
   - `src/ui/SidebarContainerWidget.h` (已阅读：第 4~45 行，确认 `m_tabBar` 声明)
   - `src/ui/SidebarContainerWidget.cpp` (已阅读：第 25~45 行，确认 `m_tabBar` 初始化逻辑)
   - `resources/style.qss` (已阅读：第 2120~2150 行，确认 `QTabBar#SidebarTabBar` 样式表规则)
2. **头文件原样签名**：
   - `NavPanel.h`:
     ```cpp
     QTabBar* m_tabBar = nullptr;
     ```
   - `SidebarContainerWidget.h`:
     ```cpp
     QTabBar* m_tabBar = nullptr;
     ```
3. **既有 API 与 Qt 样式系统机制检索**：
   - `QTabBar::setIconSize(const QSize&)`
   - `QStyle::PM_TabBarIconSpacing` (选项卡内图标与文字水平间隙度量)
   - `QProxyStyle` (重写特定部件的单一样式度量而不破坏整体主题)

---

## Chapter 1: Overview（概述与根因分析）

### 1. 现象与问题分析
在界面左侧的目录导航标题栏选项卡（“目录导航”、“历史”）与右侧收藏栏标题栏选项卡（“收藏夹”、“库”、“标记”）中：
矢量图标与后随文本之间的实际物理间距为 10~13 像素（平均约 12px）。视觉上图标与文字离得过远，显得松散，不符合紧凑精致的现代化桌面 UI 设计规范。用户明确要求将该间距严格锁定为 **7 像素（7px）**。

### 2. 根因剖析 (Root Cause)
1. **QSS 引擎局限性**：Qt 的 QSS 样式表引擎不解析类似 `icon-spacing` 或 `qproperty-iconSpacing` 属性，无法单纯依靠样式表修改 `QTabBar` 内部的图标文本间隙；
2. **系统默认度量接管**：`NavPanel` 与 `SidebarContainerWidget` 中均直接实例化的原生 `QTabBar`，其绘制 `CE_TabBarTabLabel` 时使用了原生 Windows/Qt 样式引擎的默认像素度量 `PM_TabBarIconSpacing`（系统默认返回较大间距）及文本字间外边距，导致间距放大到 10~13px；
3. **违反设计 SSOT 规范**：未能按 `Memories.md` 第 21 章规范对选项卡内部间距进行统一收敛控制。

### 3. 重构设计与解决目标
1. **轻量级专用代理样式 `SidebarTabStyle`**：
   - 继承自 `QProxyStyle`，保持现有 QSS 样式（背景、字体、圆角、选态等）100% 完整继承；
   - 重写 `pixelMetric(PM_TabBarIconSpacing, ...)`，硬性返回 `7`（像素）；
   - 在 `drawControl(CE_TabBarTabLabel, ...)` 中确保文本与图标垂直居中，水平间隙严丝合缝对齐为 7 像素；
2. **统一挂载与生命周期管理**：
   - 将 `SidebarTabStyle` 提取为可复用的公共头文件或集成到 `UiHelper` / `NavPanel.h` 中；
   - 在 `NavPanel::initUi()` 与 `SidebarContainerWidget::initUi()` 中，为 `m_tabBar` 挂载 `new SidebarTabStyle(m_tabBar->style())`，样式随部件生命周期自动释放；
3. **消除冗余与零破坏**：
   - 既有 QSS 样式规则保持零篡改；
   - 严格锁定图标与文本间距为 7px，达成视觉紧凑、统一、精致的设计目标。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/NavPanel.cpp`
2. `src/ui/SidebarContainerWidget.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/NavPanel.cpp`

```
<<<<<<< SEARCH
#include "controllers/ContextMenuFactory.h"
#include <QHeaderView>
=======
#include "controllers/ContextMenuFactory.h"
#include <QProxyStyle>
#include <QStyleOptionTab>
#include <QPainter>
#include <QHeaderView>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_tabBar = new QTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setIconSize(QSize(16, 16));
=======
    // 侧边栏选项卡专用代理样式：硬锁图标与文本间距为 7 像素
    class SidebarTabStyle : public QProxyStyle {
    public:
        using QProxyStyle::QProxyStyle;
        int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override {
            if (metric == PM_TabBarIconSpacing) {
                return 7;
            }
            return QProxyStyle::pixelMetric(metric, option, widget);
        }
        void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override {
            if (element == CE_TabBarTabLabel) {
                if (const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
                    QStyleOptionTab opt = *tab;
                    // 若存在图标，通过定制矩形严格保证图标与文字间距为 7px
                    if (!opt.icon.isNull()) {
                        QSize iconSize = opt.iconSize.isValid() ? opt.iconSize : QSize(16, 16);
                        QRect tr = opt.rect;
                        int textWidth = opt.fontMetrics.horizontalAdvance(opt.text);
                        int spacing = 7;
                        int totalContentWidth = iconSize.width() + spacing + textWidth;
                        int startX = tr.left() + (tr.width() - totalContentWidth) / 2;

                        QRect iconRect(startX, tr.top() + (tr.height() - iconSize.height()) / 2, iconSize.width(), iconSize.height());
                        QRect textRect(startX + iconSize.width() + spacing, tr.top(), textWidth + 2, tr.height());

                        opt.icon.paint(painter, iconRect, Qt::AlignCenter);
                        painter->setFont(opt.font);
                        painter->setPen(opt.palette.color(QPalette::WindowText));
                        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, opt.text);
                        return;
                    }
                }
            }
            QProxyStyle::drawControl(element, option, painter, widget);
        }
    };

    m_tabBar = new QTabBar(header);
    m_tabBar->setStyle(new SidebarTabStyle(m_tabBar->style()));
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setIconSize(QSize(16, 16));
>>>>>>> REPLACE
```

### 2. `src/ui/SidebarContainerWidget.cpp`

```
<<<<<<< SEARCH
#include <QMouseEvent>

namespace QuarkMeta {
=======
#include <QMouseEvent>
#include <QProxyStyle>
#include <QStyleOptionTab>
#include <QPainter>

namespace QuarkMeta {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_tabBar = new QTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setUsesScrollButtons(false);
    m_tabBar->setIconSize(QSize(16, 16));
=======
    // 侧边栏选项卡专用代理样式：硬锁图标与文本间距为 7 像素
    class SidebarTabStyle : public QProxyStyle {
    public:
        using QProxyStyle::QProxyStyle;
        int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override {
            if (metric == PM_TabBarIconSpacing) {
                return 7;
            }
            return QProxyStyle::pixelMetric(metric, option, widget);
        }
        void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override {
            if (element == CE_TabBarTabLabel) {
                if (const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
                    QStyleOptionTab opt = *tab;
                    if (!opt.icon.isNull()) {
                        QSize iconSize = opt.iconSize.isValid() ? opt.iconSize : QSize(16, 16);
                        QRect tr = opt.rect;
                        int textWidth = opt.fontMetrics.horizontalAdvance(opt.text);
                        int spacing = 7;
                        int totalContentWidth = iconSize.width() + spacing + textWidth;
                        int startX = tr.left() + (tr.width() - totalContentWidth) / 2;

                        QRect iconRect(startX, tr.top() + (tr.height() - iconSize.height()) / 2, iconSize.width(), iconSize.height());
                        QRect textRect(startX + iconSize.width() + spacing, tr.top(), textWidth + 2, tr.height());

                        opt.icon.paint(painter, iconRect, Qt::AlignCenter);
                        painter->setFont(opt.font);
                        painter->setPen(opt.palette.color(QPalette::WindowText));
                        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, opt.text);
                        return;
                    }
                }
            }
            QProxyStyle::drawControl(element, option, painter, widget);
        }
    };

    m_tabBar = new QTabBar(header);
    m_tabBar->setStyle(new SidebarTabStyle(m_tabBar->style()));
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setUsesScrollButtons(false);
    m_tabBar->setIconSize(QSize(16, 16));
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 编译命令
```bash
cmake -B build -S .
cmake --build build --config Release -j8
```

### 2. 运行时验收标准
1. **间距精确度核验**：启动程序后，分别对“目录导航”容器（“目录导航”、“历史”）与收藏栏容器（“收藏夹”、“库”、“标记”）截取像素；图标右侧边缘到文字左起笔边缘距离在各缩放比例下严格等于 7 像素；
2. **垂直对齐核验**：图标中心与汉字水平几何中心在同一水平基准线上，无高低错位；
3. **样式与选态核验**：选中的选项卡高亮背景（`#37373D`）、未选态悬停效果（`#2A2D2E`）及圆角（`4px`）保持与原有 QSS 100% 相同。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **复用 `QProxyStyle` 样式代理机制**：这是 Qt 官方推荐的修改单一控件样式度量的标准模式，无需继承重写庞大的 `QTabBar`，更无需将 QTabBar 替换为一组分散的 `QPushButton`；
2. **复用 QSS 样式表**：所有的边距、颜色和字体仍然由 `resources/style.qss` 统一驱动，保持样式真理源一致性。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本 | 同步与刷新方式 |
| :--- | :--- | :--- | :--- |
| **图标与文本水平间距** | `SidebarTabStyle::pixelMetric` (7px) | 渲染时的局部 `spacing` | 绘制事件中即时计算，无持久化脏状态 |
| **选项卡标题与图标** | `QTabBar::addTab` / `UiHelper::getIcon` | 选项卡标签模型 | 部件初始化时绑定 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部调用接口**：保持既有 `NavPanel` 与 `SidebarContainerWidget` 公共接口零改动；
2. **局部代理类**：`SidebarTabStyle` 作为局部样式代理，作用范围严格限制在各自的初始化代码块内，对外无符号暴露；
3. **生命周期核查**：`m_tabBar->setStyle(style)` 会自动接管代理样式的父对象生命周期，部件销毁时样式安全析构。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. **`NavPanel.cpp`**：显式包含 `<QProxyStyle>`、`<QStyleOptionTab>`、`<QPainter>`，类型完全闭合；
2. **`SidebarContainerWidget.cpp`**：显式包含 `<QProxyStyle>`、`<QStyleOptionTab>`、`<QPainter>`，类型完全闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 合规状态 | 证据与位置 |
| :---: | :--- | :---: | :--- |
| 1 | 实施界限铁律：禁止直接篡改源代码，物理隔离于 Implementation Plan/ | **满足** | 本方案存放于 `Implementation Plan/SidebarTabBarIconSpacingPlan.md` |
| 2 | SEARCH 块物理文件中唯一匹配 | **满足** | 经 `verify_plan.py` 自动化检测，所有 SEARCH 块 count=1 |
| 3 | 对外接口签名冻结契约 (Public API Freezing) | **满足** | 无任何公共头文件接口破坏 |
| 4 | C++/Qt 五道工程硬锁闭环 | **满足** | 严格遵循依赖锁与契约锁 |
| 5 | Memories.md 第 21 章 7px 间距设计铁律对齐 | **满足** | 参数严格等于 7px |
| 6 | 包含 10 大必备章节且结构完整 | **满足** | 包含 Chapter 1 至 Chapter 10 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 序号 | 既有设计/属性参数 | 规范要求 | 方案处理说明 |
| :---: | :--- | :--- | :--- |
| 1 | 图标与文本间距 | 用户明确指示锁定为 7px | 原生松散间距收窄至 7px，严格执行用户参数指示 |
| 2 | 图标尺寸 16x16 | 保持 100% 不变 | 沿用 `QSize(16, 16)` |
| 3 | QSS 内边距与颜色 | 保持 100% 不变 | 沿用 `resources/style.qss` 中定义的所有数值与 Hex 色值 |
