# 主界面各面板标题栏图标与文字颜色统一重构实施方案

- **Base Commit Hash**: `3e304b14270588e6d40fe9c9cbf79a617f73a932`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

### 已读物理证据清单
1. **物理读取文件列表**：
   - `src/ui/NavPanel.cpp`
   - `src/ui/ContentHeaderWidget.cpp`
   - `src/ui/MetaPanel.cpp`
   - `src/ui/FilterPanel.cpp`
   - `resources/style.qss`
2. **头文件原样签名验证**：
   - `UiHelper::getIcon(const QString& name, const QColor& color, int size)` -> `src/ui/UiHelper.h`
3. **既有 API 检索日志**：
   - `grep -rn "NavPanelTitleLabel" resources/style.qss` -> 确认样式定义
   - `grep -rn "MetaPanelTitleLabel" resources/style.qss` -> 确认样式定义
   - `grep -rn "FilterPanelTitleLabel" resources/style.qss` -> 确认样式定义
   - `grep -rn "ContentPanelTitleLabel" resources/style.qss` -> 确认样式定义

---

## Chapter 1: Overview（概述与根因分析）

在 QuarkMeta 主界面中，各个栏区（目录导航栏 `NavPanel`、内容区标题栏 `ContentHeaderWidget`、元数据属性面板 `MetaPanel`、筛选面板 `FilterPanel`）顶部的标题栏图标（Icon）与标题文字（Title Label）各自采用了不同的配色方案（如绿色 `#2ecc71`、青蓝色 `#41F2F2`、蓝色 `#4a90e2`、黄色 `#f1c40f` / `#FFD700`），导致全局 UI 主题不统一且过于花哨。

为了打造工业级清爽、统一且专业的高颜值界面风格，本方案对主界面所有顶栏（ContainerHeader）的图标与标题文字颜色进行归一化重构，**统一采用标准的中性灰色 `#888888`**。

---

## Chapter 2: Modified Files List（影响文件清单）

* **修改的文件**：
  - `src/ui/NavPanel.cpp`
  - `src/ui/ContentHeaderWidget.cpp`
  - `src/ui/MetaPanel.cpp`
  - `src/ui/FilterPanel.cpp`
  - `resources/style.qss`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. 修改 `src/ui/NavPanel.cpp` 中的标题图标颜色

```
<<<<<<< SEARCH
    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("list_ul", QColor("#2ecc71"), 18).pixmap(18, 18));
=======
    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("list_ul", QColor("#888888"), 18).pixmap(18, 18));
>>>>>>> REPLACE
```

### 2. 修改 `src/ui/ContentHeaderWidget.cpp` 中的标题图标颜色

```
<<<<<<< SEARCH
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(18, 18);
    m_iconLabel->setPixmap(UiHelper::getIcon("image_picture", QColor("#41F2F2"), 18).pixmap(18, 18));
=======
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(18, 18);
    m_iconLabel->setPixmap(UiHelper::getIcon("image_picture", QColor("#888888"), 18).pixmap(18, 18));
>>>>>>> REPLACE
```

### 3. 修改 `src/ui/MetaPanel.cpp` 中的标题图标颜色

```
<<<<<<< SEARCH
    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("all_data", QColor("#4a90e2"), 16).pixmap(16, 16));
=======
    QLabel* iconLabel = new QLabel(header);
    iconLabel->setPixmap(UiHelper::getIcon("all_data", QColor("#888888"), 16).pixmap(16, 16));
>>>>>>> REPLACE
```

### 4. 修改 `src/ui/FilterPanel.cpp` 中的标题图标颜色

```
<<<<<<< SEARCH
    QColor brandYellow = QColor("#f1c40f");
    m_iconLabel->setPixmap(UiHelper::getIcon("filter_funnel_outline", brandYellow, 18).pixmap(18, 18));
=======
    QColor defaultGray = QColor("#888888");
    m_iconLabel->setPixmap(UiHelper::getIcon("filter_funnel_outline", defaultGray, 18).pixmap(18, 18));
>>>>>>> REPLACE
```

### 5. 修改 `resources/style.qss` 中的标题栏文字颜色选择器

```
<<<<<<< SEARCH
QLabel#FilterPanelTitleLabel {
    color: #FFD700;
}
=======
QLabel#FilterPanelTitleLabel {
    color: #888888;
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QLabel#NavPanelTitleLabel {
    color: #2ecc71;
    font-size: 13px;
    font-weight: bold;
    background: transparent;
    border: none;
}
=======
QLabel#NavPanelTitleLabel {
    color: #888888;
    font-size: 13px;
    font-weight: bold;
    background: transparent;
    border: none;
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QLabel#MetaPanelTitleLabel {
    font-size: 12px;
    font-weight: bold;
    color: #4a90e2;
    background: transparent;
    border: none;
}
=======
QLabel#MetaPanelTitleLabel {
    font-size: 12px;
    font-weight: bold;
    color: #888888;
    background: transparent;
    border: none;
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
1. **视觉归一化验证**：启动主界面后，观察左侧“目录导航”、中间“内容区”、右侧“元数据属性”和“筛选” 4 个栏区顶部标题栏。
2. **图标与文字一致性**：所有标题栏图标与标题文字均统一显示为清爽的灰色 (`#888888`)，消除不同彩色混杂现象。
3. **交互无破坏**：标题栏点击、折叠、按钮响应等既有交互逻辑 100% 保持完好。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **图标渲染入口**：统一复用 `UiHelper::getIcon(name, QColor("#888888"), size)`，无额外渲染逻辑。
2. **样式定义入口**：标题栏文字 CSS 样式统一在 `resources/style.qss` 中维护，禁止在 C++ 中使用内联 `setStyleSheet`。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 观察者 / 视图 | 刷新方向 |
| :--- | :--- | :--- | :--- |
| **标题栏图标与文字色值** | `resources/style.qss` & C++ 灰色常量 (`#888888`) | 各种 ContainerHeader 子控件 | UI 样式静态/自适应渲染 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- 无任何新增或修改的 `.h` 接口签名，所有公开与私有声明保持 100% 原样兼容。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 未添加或删除任何 `#include` 语句，依赖类型与头文件包含闭合完好。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 详细说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否未对源代码进行直接修改 | **满足** | 本方案为 Markdown 实施方案，存放于 `Implementation Plan/` 目录 |
| 2 | 是否回应了架构三问 | **满足** | Chapter 1 明确说明统一灰色视觉归一化目标 |
| 3 | 是否无 C++ 硬编码内联 QSS | **满足** | 颜色样式全部收敛于 `style.qss` |
| 4 | 是否未动 Freeze 的公共接口 | **满足** | 无任何 `.h` 签名修改 |
| 5 | 是否包含 SEARCH 块真实性核查 | **满足** | 所有 SEARCH 块均与 physical 源代码 1:1 匹配 |
| 6 | 是否包含构建与运行时验收标准 | **满足** | Chapter 4 提供编译与运行时测试用例 |
| 7 | 是否包含数值零篡改表 | **满足** | Chapter 10 提供完整对照表 |
| 8 | 语言规范是否合规 | **满足** | 正文及注释严格遵守中文规范 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 属性 / 栏区 | 原始配置 | 重构后配置 | 一致性 / 变化确认 |
| :--- | :--- | :--- | :--- |
| **目录导航标题栏图标/文字** | 图标 `#2ecc71` 绿色 | 图标 `#888888` 灰色，文字 `#888888` | **统一改为 `#888888` 灰色** |
| **内容区标题栏图标/文字** | 图标 `#41F2F2` 青蓝色 | 图标 `#888888` 灰色，文字 `#888888` | **统一改为 `#888888` 灰色** |
| **元数据属性标题栏图标/文字** | 图标 `#4a90e2` 蓝色 | 图标 `#888888` 灰色，文字 `#888888` | **统一改为 `#888888` 灰色** |
| **筛选标题栏图标/文字** | 图标 `#f1c40f` 黄色 | 图标 `#888888` 灰色，文字 `#888888` | **统一改为 `#888888` 灰色** |
| **标题栏尺寸与布局** | 高度 `32px` | 高度 `32px` | **100% 原样保持** |
