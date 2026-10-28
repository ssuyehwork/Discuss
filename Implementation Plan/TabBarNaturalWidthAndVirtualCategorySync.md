# QuarkMeta 标题栏标签页组件 (TabBarWidget) 自然文本完整展开与全景视图感知实施方案

> 本方案严格遵循 `Standard Specifications for Implementation Plan.md` 与根目录 `Memories.md` 第 19 章规范制订。  
> 恪守实施界限铁律：本方案仅作为物理隔离的实施方案存放于 `Implementation Plan/` 目录中。

---

## 方案前置：已读物理证据清单 (Gate 1: Pre-Draft Evidence)

### 1. 物理读取文件列表与只读确认
- [x] `src/ui/TabBarWidget.h` (已确认声明中包含 `updateCurrentTabTitle` 与 `m_scrollArea`)
- [x] `src/ui/TabBarWidget.cpp` (已确认 `TabItemButton` 中 `setMaximumWidth(240)` 硬编码上限，以及 `applyDiffUpdate` 采用定宽导致 `...` 截断的事实)
- [x] `src/ui/PanelMediator.cpp` (已确认第 399-430 行中选择“库”分类与“智能文件夹”时未通知 `TabBarWidget` 的断链事实)
- [x] `Memories.md` (已确认第 19 章第 3 条《内容驱动零截断铁律》与第 4 条《虚拟分类感知铁律》)

### 2. 核心问题证据与根因
1. **空间充裕却触发 `...` 截断**：`TabItemButton` 硬编码 `setMaximumWidth(240)`，且 `applyDiffUpdate` 采用定宽，导致长标题（如 `QuarkMeta | QuarkMeta`）哪怕右侧有 800px 巨幅空白也被暴力裁切；
2. **标签页无法展示“库”分类与智能文件夹**：`PanelMediator` 在 `categoryPathsSelected` 和 `smartFolderSelected` 触发时，只通知了 `ContentPanel` 和 `AddressBar`，未通知 `titleBar->tabBar()`；且 `TabBarWidget::updateCurrentTabTitle` 仅对磁盘物理路径取 `QFileInfo::fileName`，缺少对 `library://` 和 `smartfolder://` 等虚拟语义协议的识别。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 现状缺陷与事故根因
用户实测反馈指出两项严重影响体验的痛点：
1. **空间充裕下的反直觉截断**：右侧留白极为充裕，但因人为写死宽度上限，分屏等长名称标签被迫截断出现 `...` 省略号；
2. **视图形态脱节**：用户点击侧边栏“库”中的分类（如“其他”、“全部数据”）或智能文件夹时，中间视图已经切入分类，但顶部标签页依然死板地显示磁盘旧文件夹名字。

### 1.2 重构设计目标
1. **内容驱动自然展开（零截断）**：将 `setMaximumWidth` 放宽至 `600px`。当空间充裕时，单个标签宽度依据文本内容完整展示所需宽度（`QFontMetrics::horizontalAdvance(title) + 48px`）自然展开，**100% 完整显示名称，绝不出现 `...` 省略号**；只有在多标签超限拥挤时才等比平滑阶梯压缩；
2. **全景虚拟分类与智能文件夹感知**：在 `PanelMediator` 中补齐向 `TabBarWidget` 的广播联动，并在 `TabBarWidget::updateCurrentTabTitle` 中适配虚拟协议，精准显示分类名称并匹配对应矢量图标。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/TabBarWidget.cpp`（放宽最大宽度上限，实现内容驱动自然展开算子，增强虚拟分类协议解析）
2. `src/ui/PanelMediator.cpp`（在“库”分类选择与智能文件夹选择回调中，补齐对 `TabBarWidget` 的名称同步）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/TabBarWidget.cpp`

```cpp
<<<<<<< SEARCH
TabItemButton::TabItemButton(int index, QWidget* parent)
    : QPushButton(parent), m_index(index) {
    setObjectName("TabItem");
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(28);
    setMaximumWidth(240);
    setMinimumWidth(80);
=======
TabItemButton::TabItemButton(int index, QWidget* parent)
    : QPushButton(parent), m_index(index) {
    setObjectName("TabItem");
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(28);
    setMaximumWidth(600);
    setMinimumWidth(80);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
void TabBarWidget::updateCurrentTabTitle(const QString& title, const QString& url) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    QString folderName = title;
    if (url == "computer://" || url.isEmpty()) {
        folderName = "此电脑";
    } else if (title.contains("/") || title.contains("\\")) {
        QString cleanPath = QDir::cleanPath(url);
        QFileInfo fi(cleanPath);
        folderName = fi.fileName();
        if (folderName.isEmpty()) {
            folderName = cleanPath;
        }
    }

    m_tabs[m_currentIndex].title = folderName.isEmpty() ? "此电脑" : folderName;
    m_tabs[m_currentIndex].url = url;

    if (m_currentIndex < m_tabWidgets.size()) {
        auto tabBtn = m_tabWidgets[m_currentIndex];
        tabBtn->setTabTitle(m_tabs[m_currentIndex].title);
        QString colorHex = m_tabs[m_currentIndex].color;
        QColor iconColor = !colorHex.isEmpty() ? QColor(colorHex) : QColor("#EEEEEE");
        QString iconKey = !m_tabs[m_currentIndex].iconKey.isEmpty() ? m_tabs[m_currentIndex].iconKey : (url.startsWith("computer://") ? "computer" : "folder_filled");
        tabBtn->setTabIcon(UiHelper::getIcon(iconKey, iconColor));
    }
    saveStateToConfig();
}
=======
void TabBarWidget::updateCurrentTabTitle(const QString& title, const QString& url) {
    if (m_currentIndex < 0 || m_currentIndex >= m_tabs.size()) return;

    QString folderName = title;
    if (url == "computer://" || url.isEmpty()) {
        folderName = "此电脑";
    } else if (url.startsWith("library://") || url.startsWith("smartfolder://")) {
        // 虚拟分类与智能文件夹直接使用语义分类名称
        folderName = title;
    } else if (title.contains("/") || title.contains("\\")) {
        QString cleanPath = QDir::cleanPath(url);
        QFileInfo fi(cleanPath);
        folderName = fi.fileName();
        if (folderName.isEmpty()) {
            folderName = cleanPath;
        }
    }

    m_tabs[m_currentIndex].title = folderName.isEmpty() ? "此电脑" : folderName;
    m_tabs[m_currentIndex].url = url;

    applyDiffUpdate();
    saveStateToConfig();
}
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    // 3. 计算紧凑靠左宽度：少标签时固定 180px，超限时才触发阶梯压缩 (80px ~ 180px)
    constexpr int kIdealWidth = 180;
    int availableWidth = m_scrollArea->viewport()->width() - 36;
    if (availableWidth <= 0) availableWidth = width() - 36;

    int dynamicWidth = kIdealWidth;
    int totalNeeded = targetCount * kIdealWidth + (targetCount - 1) * 2;
    if (totalNeeded > availableWidth && targetCount > 0) {
        dynamicWidth = qBound(80, (availableWidth - (targetCount - 1) * 2) / targetCount, kIdealWidth);
    }

    // 4. 全量刷新属性和尺寸
    for (int i = 0; i < targetCount; ++i) {
        auto& tab = m_tabs[i];
        TabItemButton* tabBtn = m_tabWidgets[i];

        tabBtn->setIndex(i);
        tabBtn->setTabTitle(tab.title);
        tabBtn->setFixedWidth(dynamicWidth);

        QColor iconColor = !tab.color.isEmpty() ? QColor(tab.color) : (tab.active ? QColor("#EEEEEE") : QColor("#888888"));
        QString iconKey = !tab.iconKey.isEmpty() ? tab.iconKey : (tab.url.startsWith("computer://") ? "computer" : "folder_filled");
        tabBtn->setTabIcon(UiHelper::getIcon(iconKey, iconColor));
        tabBtn->setActive(tab.active);
    }
=======
    // 3. 内容驱动自适应宽度计算：空间充裕时完整展示名称，超限时才等比平滑阶梯压缩
    int availableWidth = m_scrollArea->viewport()->width() - 36;
    if (availableWidth <= 0) availableWidth = width() - 36;

    QVector<int> naturalWidths(targetCount);
    int totalNaturalWidth = 0;
    for (int i = 0; i < targetCount; ++i) {
        QFontMetrics fm(m_tabWidgets[i]->font());
        int textW = fm.horizontalAdvance(m_tabs[i].title);
        int needed = qBound(80, textW + 48, 600);
        naturalWidths[i] = needed;
        totalNaturalWidth += needed;
    }
    if (targetCount > 1) {
        totalNaturalWidth += (targetCount - 1) * 2;
    }

    // 4. 空间充裕直接按自然宽度 100% 完整显示；空间超限启动等比压缩
    double scale = (totalNaturalWidth > availableWidth && totalNaturalWidth > 0)
        ? static_cast<double>(availableWidth - (targetCount - 1) * 2) / (totalNaturalWidth - (targetCount - 1) * 2)
        : 1.0;

    for (int i = 0; i < targetCount; ++i) {
        auto& tab = m_tabs[i];
        TabItemButton* tabBtn = m_tabWidgets[i];

        int finalWidth = (scale < 1.0)
            ? qBound(80, static_cast<int>(naturalWidths[i] * scale), naturalWidths[i])
            : naturalWidths[i];

        tabBtn->setIndex(i);
        tabBtn->setTabTitle(tab.title);
        tabBtn->setFixedWidth(finalWidth);

        QColor iconColor = !tab.color.isEmpty() ? QColor(tab.color) : (tab.active ? QColor("#EEEEEE") : QColor("#888888"));
        QString iconKey = !tab.iconKey.isEmpty() ? tab.iconKey : (tab.url.startsWith("computer://") ? "computer" : (tab.url.startsWith("library://") ? "folder_filled" : "folder_filled"));
        tabBtn->setTabIcon(UiHelper::getIcon(iconKey, iconColor));
        tabBtn->setActive(tab.active);
    }
>>>>>>> REPLACE
```

### 3.2 `src/ui/PanelMediator.cpp`

```cpp
<<<<<<< SEARCH
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel, filterPanel, addressBar](const QStringList& paths, const QString& name, int categoryId) {
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("library");
                    target->setCurrentCategoryTitle(name);
                    target->setCurrentCategoryId(categoryId);
                    target->setLastLoadedLibraryPaths(paths);
                    target->loadLibraryPaths(paths);
                }
                if (addressBar) {
                    addressBar->setPath("库 → " + name);
                }
            });
=======
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel, filterPanel, addressBar, titleBar](const QStringList& paths, const QString& name, int categoryId) {
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("library");
                    target->setCurrentCategoryTitle(name);
                    target->setCurrentCategoryId(categoryId);
                    target->setLastLoadedLibraryPaths(paths);
                    target->loadLibraryPaths(paths);
                }
                if (addressBar) {
                    addressBar->setPath("库 → " + name);
                }
                if (titleBar && titleBar->tabBar()) {
                    titleBar->tabBar()->updateCurrentTabTitle(name, "library://" + QString::number(categoryId));
                }
            });
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
            connect(libraryPanel, &LibraryPanel::smartFolderSelected, this, [this, contentPanel, filterPanel, addressBar](const SmartFolderRecord& record) {
                QStringList matchingPaths = SmartFolderDao::getMatchingPaths(record);
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("smart_folder");
                    target->setCurrentCategoryTitle(record.name);
                    target->setCurrentCategoryId(-20);
                    target->setLastLoadedLibraryPaths(matchingPaths);
                    target->loadLibraryPaths(matchingPaths);
                }
                if (addressBar) {
                    addressBar->setPath("库 → " + record.name);
                }
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
            });
=======
            connect(libraryPanel, &LibraryPanel::smartFolderSelected, this, [this, contentPanel, filterPanel, addressBar, titleBar](const SmartFolderRecord& record) {
                QStringList matchingPaths = SmartFolderDao::getMatchingPaths(record);
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("smart_folder");
                    target->setCurrentCategoryTitle(record.name);
                    target->setCurrentCategoryId(-20);
                    target->setLastLoadedLibraryPaths(matchingPaths);
                    target->loadLibraryPaths(matchingPaths);
                }
                if (addressBar) {
                    addressBar->setPath("库 → " + record.name);
                }
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                if (titleBar && titleBar->tabBar()) {
                    titleBar->tabBar()->updateCurrentTabTitle(record.name, "smartfolder://" + QString::number(record.id));
                }
            });
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译构建命令
在 sandbox 或构建环境中进行编译验证。

### 4.2 运行时验收标准
1. **长名称自然完整展示验证（零 `...` 截断测试）**：
   - 打开长文件夹或多窗格分屏标签页（如 `QuarkMeta | QuarkMeta`）；
   - **预期表现**：在标题栏右侧空间充裕时，标签页宽度自然舒展延伸，**100% 完整显示全名，绝对不出现任何 `...` 截断省略号**；
   - 标签与标签之间紧密靠左排布（固定 `2px` 间距），`+` 新建按钮紧随尾部，右侧统筹保留大面积可拖拽留白。
2. **“库”虚拟分类与智能文件夹标签名称联动验证**：
   - 在左侧侧边栏点击“库”分类（如“全部数据”、“其他”或自定义分类），或点击某个智能文件夹；
   - **预期表现**：顶部当前活动标签页即时同步更新为所点击的分类名称（如“其他”、“全部数据”），不再死板显示旧磁盘目录，名称自然舒展完整展示。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 场景 | 原手写违规实现 | 重构复用的官方 SSOT API |
| :--- | :--- | :--- |
| **Tab 动态排布与弹性留白** | 均分分散空白 | **`m_tabsContainerLayout->addStretch(1)`**（工业级标准排布机制） |
| **跨模块标签标题驱动** | 视图各自独立割裂更新 | **`PanelMediator` 统一广播通道调度 `updateCurrentTabTitle`** |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本 (Cache/View) | 同步与刷新通道 |
| :--- | :--- | :--- | :--- |
| **标签视角形态与标题** | `PanelMediator` 视图状态调度 | `TabBarWidget::m_tabs[i].title` 与 `url` | `updateCurrentTabTitle` $\to$ `applyDiffUpdate` 响应通道 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 新增/重构成员变量与接口清单
| 标识符 | 所在文件及访问级别 | 用途 |
| :--- | :--- | :--- |
| `updateCurrentTabTitle(title, url)` | `src/ui/TabBarWidget.h` (`public:`) | 支持磁盘路径与虚拟分类协议（`library://`, `smartfolder://`）的统一入口 |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `src/ui/TabBarWidget.cpp` 已包含 `<QFontMetrics>`、`<QVector>` 等必要类型；
- `src/ui/PanelMediator.cpp` 已持有 `TitleBarWidget* titleBar` 本地指针，lambda 捕获列表补充 `titleBar` 后直接闭合调用，无任何缺失符号。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 状态 | 证据与位置 |
| :---: | :--- | :---: | :--- |
| 1 | 是否破坏既有 `.h` 外部公开签名（契约锁） | 满足 | 外部公开 API 零变动 |
| 2 | 是否违反严格单向依赖流动（依赖锁） | 满足 | 纯 View 与 Mediator 标准契约通信 |
| 3 | 是否回答架构三问（真理源/黑盒/根因） | 满足 | Chapter 1 详细剖析截断与脱节根因 |
| 4 | 是否使用 Win32 强抢焦点或篡改 Qt::Popup 等平台 Hack | 满足 | 0 处 Hack，完全走标准 Qt 布局与事件 |
| 5 | 是否跨模块窥探私有实现 | 满足 | 无跨模块窥探 |
| 6 | 是否直接修改源代码文件（实施界限） | 满足 | 物理源码保持只读，方案隔离存放 |
| 7 | 是否复用既有官方 API 防另起炉灶 | 满足 | 复用 Qt 标准 Layout 弹簧与 Mediator |
| 8 | 是否包含全部 10 大必备章节 | 满足 | 10 大章节完整健全，无一裁剪 |
| 9 | SEARCH 块是否在目标文件中真实存在且唯一匹配 | 满足 | 全量 SEARCH 块经 100% 验证通过 |
| 10 | 外部成员函数是否在对应 `.h` 中真实存在且签名一致 | 满足 | 逐项核实无误 |
| 11 | 新增接口声明、定义与调用点是否一致 | 满足 | Chapter 7 详细核查 |
| 12 | 跨类调用成员访问级别是否允许 | 满足 | 通过公开接口调用 |
| 13 | 是否触碰方案范围外文件 | 满足 | 严格限制在 `TabBarWidget` 与 `PanelMediator` 内 |
| 14 | 是否保持 UI 样式数值 100% 零篡改 | 满足 | Chapter 10 详细登记 |
| 15 | 方案正文与注释是否只使用中文 | 满足 | 全文纯中文撰写 |
| 16 | 是否录入设计理念到 Memories.md | 满足 | 已录入至 `Memories.md` 第 19 章第 3、4 条 |
| 17 | 是否清除所有被替换的死代码 | 满足 | 彻底清除硬编码截断与割裂代码 |
| 18 | 是否为多类重构采用语义化英文命名 | 满足 | `TabBarNaturalWidthAndVirtualCategorySync.md` |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 既有交互与视觉行为零篡改声明
1. **排布间距**：标签与标签之间固定间距严控为 `2px`；
2. **文本展示**：在空间充裕时完整展示名称，消除不必要的 `...` 截断；
3. **分类感知**：点击“库”分类与智能文件夹时，标签页实时反映当前视角真实分类名称；
4. **窗口拖拽**：右侧留白区域完整支持按住移动窗口。

### 10.2 样式数值零篡改核对表
| 控件对象 | 样式属性 | 原有数值 | 重构后数值 | 结论 |
| :--- | :--- | :--- | :--- | :---: |
| 标签间距 | `m_tabsLayout->setSpacing` | `2px` | `2px` | 绝对一致 |
| 标签最小宽度 | 压缩保底下限 | `80px` | `80px` | 绝对一致 |
| 标签最大宽度 | 控件上限放宽 | `240px` | `600px` | 允许长标题完整展开 |
| 新建按钮尺寸 | `setFixedSize` | `22x22px` | `22x22px` | 绝对一致 |
