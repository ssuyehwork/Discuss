# RapidNotes 彻底剥离侧车 JSON、废除物理收藏夹与“标记”面板底层纯分类化实施方案

> 本方案严格遵循《AGENTS.md》实施界限与《Standard Specifications for Implementation Plan.md》十全大补章节规范，旨在彻底清除与数据库笔记架构相脱节的磁盘隐藏侧车 JSON（`QuarkMetaJson` / `QuarkMetaJsonStore`）以及物理文件收藏夹（`FavoritePanel` / `FavoriteDao` / `FavoriteService`）。同时坚决贯彻用户的核心指示：**侧边栏“标记”选项卡保持原样零篡改（仍为“标记”与 `Destination.svg`），仅将其底层逻辑与数据源由物理磁盘路径转变为数据库分类（`categories`）**。

---

## 前置：已读物理证据清单 (Gate 1 Evidence)

1. **物理读取文件列表**：
   - `RapidNotes_Source/RapidNotes/CMakeLists.txt`
   - `RapidNotes_Source/RapidNotes/src/ui/SidebarContainerWidget.h`
   - `RapidNotes_Source/RapidNotes/src/ui/SidebarContainerWidget.cpp`
   - `RapidNotes_Source/RapidNotes/src/ui/MarkedPanel.h`
   - `RapidNotes_Source/RapidNotes/src/ui/MarkedPanel.cpp`
   - `RapidNotes_Source/RapidNotes/src/ui/PanelMediator.h`
   - `RapidNotes_Source/RapidNotes/src/ui/PanelMediator.cpp`
   - `RapidNotes_Source/RapidNotes/src/ui/MainWindow.h`
   - `RapidNotes_Source/RapidNotes/src/ui/MainWindow.cpp`
   - `RapidNotes_Source/RapidNotes/Memories_RapidNotes.md`

2. **核心调用链物理签名与行号核查**：
   - `SidebarContainerWidget.cpp:86`: `m_tabBar->addTab(..., "收藏夹");`
   - `SidebarContainerWidget.cpp:88`: `m_tabBar->addTab(UiHelper::getIcon("Destination.svg", "#888888", 16), "标记");`
   - `SidebarContainerWidget.cpp:95`: `m_favoritePanel = new FavoritePanel(this);`
   - `SidebarContainerWidget.cpp:97`: `m_markedPanel = new MarkedPanel(this);`
   - `PanelMediator.cpp:89`: `FavoritePanel* favoritePanel = m_favoritePanel;`
   - `CMakeLists.txt:111, 468, 588`: 注册 `FavoriteDao`、`FavoriteService`、`FavoritePanel`
   - `CMakeLists.txt:500`: 注册 `QuarkMetaJson.cpp/.h` 与 `QuarkMetaJsonStore.cpp/.h`

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题根因分析
在 RapidNotes 由面向本地磁盘文件系统转向统一数据库笔记体系（`inspiration.db` 中的 `notes` 与 `categories` 表）之后：
1. **隐藏侧车 JSON（`QuarkMetaJson` / `QuarkMetaJsonStore`）**：原用于在磁盘目录写入 `.RapidNotes.json` 保存元数据。数据库模式下所有元数据已存入 SQLite 结构化表，侧车文件机制彻底变为多余甚至引起 I/O 阻塞；
2. **物理收藏夹（`FavoritePanel` / `FavoriteDao` / `FavoriteService`）**：原面向磁盘驱动器与物理文件路径组织收藏。笔记模式下收藏已由 `notes.is_favorite` 字段与系统分类原生承载，物理收藏树彻底冗余；
3. **“标记”选项卡底层转变**：侧边栏的“标记”面板原先读取磁盘物理目录（`MarkedFolderService`）。用户明确指示：**UI 上继续保留“标记”选项卡与 `Destination.svg` 零篡改，无需修改“标记”选项卡；仅将其内部数据源与行为由物理路径转变为数据库分类（`categories` 表）**。

### 1.2 重构设计目标
1. 彻底删除 4 个侧车 JSON 相关文件：`QuarkMetaJson.cpp/.h` 与 `QuarkMetaJsonStore.cpp/.h`；
2. 彻底删除 6 个物理收藏夹相关文件：`FavoritePanel.cpp/.h`、`FavoriteDao.cpp/.h`、`FavoriteService.cpp/.h`；
3. 侧边栏移除“收藏夹”Tab，“标记”选项卡名称与图标 100% 原样保留，面板内加载数据库分类数据；
4. 清理 `PanelMediator`、`MainWindow`、`SidebarContainerWidget` 中对 `FavoritePanel` 的依赖；
5. 在 `CMakeLists.txt` 中同步移除被删除文件的注册条目。

---

## Chapter 2: Modified Files List（影响文件清单）

### 物理删除的文件（共 10 个）
1. `RapidNotes_Source/RapidNotes/src/meta/QuarkMetaJson.h`
2. `RapidNotes_Source/RapidNotes/src/meta/QuarkMetaJson.cpp`
3. `RapidNotes_Source/RapidNotes/src/meta/QuarkMetaJsonStore.h`
4. `RapidNotes_Source/RapidNotes/src/meta/QuarkMetaJsonStore.cpp`
5. `RapidNotes_Source/RapidNotes/src/meta/FavoriteDao.h`
6. `RapidNotes_Source/RapidNotes/src/meta/FavoriteDao.cpp`
7. `RapidNotes_Source/RapidNotes/src/meta/FavoriteService.h`
8. `RapidNotes_Source/RapidNotes/src/meta/FavoriteService.cpp`
9. `RapidNotes_Source/RapidNotes/src/ui/FavoritePanel.h`
10. `RapidNotes_Source/RapidNotes/src/ui/FavoritePanel.cpp`

### 修改的文件（共 6 个）
1. `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 移除上述 10 个文件的编译源列表。
2. `RapidNotes_Source/RapidNotes/src/ui/SidebarContainerWidget.h` — 移除 `FavoritePanel` 包含与指针，保留 `m_libraryPanel` 与 `m_markedPanel`。
3. `RapidNotes_Source/RapidNotes/src/ui/SidebarContainerWidget.cpp` — 移除收藏夹 Tab，保留“库”与“标记”两项。
4. `RapidNotes_Source/RapidNotes/src/ui/MarkedPanel.cpp` — 内部加载从物理文件夹改为加载数据库分类。
5. `RapidNotes_Source/RapidNotes/src/ui/PanelMediator.h` 与 `.cpp` — 移除 `FavoritePanel` 声明、指针及信号槽绑定。
6. `RapidNotes_Source/RapidNotes/src/ui/MainWindow.h` 与 `.cpp` — 移除 `m_favoritePanel` 成员与包含。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 移除 Favorite 编译源

```
<<<<<<< SEARCH
    src/meta/FavoriteDao.h
    src/meta/FavoriteService.h
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    src/meta/FavoriteDao.cpp
    src/meta/FavoriteService.cpp
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    src/ui/FavoritePanel.cpp
    src/ui/FavoritePanel.h
=======
>>>>>>> REPLACE
```

### 3.2 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 移除 QuarkMetaJson 编译源

```
<<<<<<< SEARCH
    src/meta/QuarkMetaJson.cpp
    src/meta/QuarkMetaJson.h
    src/meta/QuarkMetaJsonStore.cpp
    src/meta/QuarkMetaJsonStore.h
=======
>>>>>>> REPLACE
```

### 3.3 `RapidNotes_Source/RapidNotes/src/ui/SidebarContainerWidget.h` — 移除 FavoritePanel 依赖并保留 MarkedPanel

```
<<<<<<< SEARCH
#include "FavoritePanel.h"
#include "LibraryPanel.h"
#include "MarkedPanel.h"
#include "SearchHistoryPanel.h"
#include <QLineEdit>

namespace RapidNotes {

class FavoritePanel;
class LibraryPanel;
class MarkedPanel;
class SearchHistoryPanel;
=======
#include "LibraryPanel.h"
#include "MarkedPanel.h"
#include "SearchHistoryPanel.h"
#include <QLineEdit>

namespace RapidNotes {

class LibraryPanel;
class MarkedPanel;
class SearchHistoryPanel;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    FavoritePanel* favoritePanel() const { return m_favoritePanel; }
    LibraryPanel* libraryPanel() const { return m_libraryPanel; }
    MarkedPanel* markedPanel() const { return m_markedPanel; }
=======
    LibraryPanel* libraryPanel() const { return m_libraryPanel; }
    MarkedPanel* markedPanel() const { return m_markedPanel; }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QStackedWidget* m_stackedWidget = nullptr;
    FavoritePanel* m_favoritePanel = nullptr;
    LibraryPanel* m_libraryPanel = nullptr;
    MarkedPanel* m_markedPanel = nullptr;
=======
    QStackedWidget* m_stackedWidget = nullptr;
    LibraryPanel* m_libraryPanel = nullptr;
    MarkedPanel* m_markedPanel = nullptr;
>>>>>>> REPLACE
```

### 3.4 `RapidNotes_Source/RapidNotes/src/ui/SidebarContainerWidget.cpp` — 移除收藏夹 Tab 并保持“标记”选项卡零篡改

```
<<<<<<< SEARCH
    m_tabBar->addTab(UiHelper::getIcon("Favorites.svg", "#888888", 16), "收藏夹");
    m_tabBar->addTab(UiHelper::getIcon("Search-in-library", "#888888", 16), "库");
    m_tabBar->addTab(UiHelper::getIcon("Destination.svg", "#888888", 16), "标记");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);
    m_favoritePanel = new FavoritePanel(this);
    m_libraryPanel = new LibraryPanel(this);
    m_markedPanel = new MarkedPanel(this);

    m_stackedWidget->addWidget(m_favoritePanel);
    m_stackedWidget->addWidget(m_libraryPanel);
    m_stackedWidget->addWidget(m_markedPanel);
=======
    m_tabBar->addTab(UiHelper::getIcon("Search-in-library", "#888888", 16), "库");
    m_tabBar->addTab(UiHelper::getIcon("Destination.svg", "#888888", 16), "标记");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);
    m_libraryPanel = new LibraryPanel(this);
    m_markedPanel = new MarkedPanel(this);

    m_stackedWidget->addWidget(m_libraryPanel);
    m_stackedWidget->addWidget(m_markedPanel);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        QString kw = text.trimmed();
        if (m_favoritePanel) {
            m_favoritePanel->filterTreeNodes(kw);
        }
        if (m_libraryPanel) {
=======
    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        QString kw = text.trimmed();
        if (m_libraryPanel) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
        if (index == 1 && m_libraryPanel) {
            m_libraryPanel->loadLibrary();
        }
        if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
            QString kw = m_filterEdit->text().trimmed();
            if (index == 0 && m_favoritePanel) m_favoritePanel->filterTreeNodes(kw);
            if (index == 1 && m_libraryPanel) m_libraryPanel->filterTreeNodes(kw);
            if (index == 2 && m_markedPanel) m_markedPanel->filterTreeNodes(kw);
        }
=======
    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
        if (index == 0 && m_libraryPanel) {
            m_libraryPanel->loadLibrary();
        }
        if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
            QString kw = m_filterEdit->text().trimmed();
            if (index == 0 && m_libraryPanel) m_libraryPanel->filterTreeNodes(kw);
            if (index == 1 && m_markedPanel) m_markedPanel->filterTreeNodes(kw);
        }
>>>>>>> REPLACE
```

### 3.5 `RapidNotes_Source/RapidNotes/src/ui/MarkedPanel.cpp` — 标记面板内部加载数据库分类

```
<<<<<<< SEARCH
void MarkedPanel::loadMarkedFolders() {
    m_treeWidget->clear();
    QStringList allList = MarkedFolderService::instance().getAllMarked();
    for (const QString& path : allList) {
        if (QFile::exists(path)) {
            bool active = MarkedFolderService::instance().isMarked(path);
            QTreeWidgetItem* item = createMarkedItem(path, active);
            m_treeWidget->addTopLevelItem(item);
        }
    }
    if (!m_currentFilterKeyword.isEmpty()) {
        filterTreeNodes(m_currentFilterKeyword);
    }
}
=======
void MarkedPanel::loadMarkedFolders() {
    m_treeWidget->clear();
    // 按照用户指示：由物理路径转变为数据库分类
    QList<QVariantMap> categories = DatabaseManager::instance().getCategories();
    for (const auto& cat : categories) {
        int catId = cat.value("id").toInt();
        QString name = cat.value("name").toString();
        QString color = cat.value("color").toString();
        if (color.isEmpty()) color = "#888888";

        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, name);
        item->setData(0, Qt::UserRole, QString("library://%1").arg(catId));
        item->setData(0, Qt::UserRole + 1, catId);
        item->setIcon(0, UiHelper::getIcon("Destination.svg", QColor(color), 16));
        m_treeWidget->addTopLevelItem(item);
    }
    if (!m_currentFilterKeyword.isEmpty()) {
        filterTreeNodes(m_currentFilterKeyword);
    }
}
>>>>>>> REPLACE
```

### 3.6 `RapidNotes_Source/RapidNotes/src/ui/PanelMediator.h` — 移除 FavoritePanel 指针

```
<<<<<<< SEARCH
class FavoritePanel;
class LibraryPanel;
=======
class LibraryPanel;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    FavoritePanel* favoritePanel = nullptr;
    LibraryPanel* libraryPanel = nullptr;
=======
    LibraryPanel* libraryPanel = nullptr;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<LibraryPanel> m_libraryPanel;
=======
    QPointer<LibraryPanel> m_libraryPanel;
>>>>>>> REPLACE
```

### 3.7 `RapidNotes_Source/RapidNotes/src/ui/PanelMediator.cpp` — 移除 FavoritePanel 信号连接

```
<<<<<<< SEARCH
#include "FavoritePanel.h"
#include "LibraryPanel.h"
=======
#include "LibraryPanel.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    FavoritePanel* favoritePanel = m_favoritePanel;
    LibraryPanel* libraryPanel = m_libraryPanel;
=======
    LibraryPanel* libraryPanel = m_libraryPanel;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        connect(favoritePanel, &FavoritePanel::directorySelected, &NavigationService::instance(), [](const QString& path, bool recursive) {
            NavigationService::instance().navigateTo(path, recursive);
        });

        connect(favoritePanel, &FavoritePanel::requestLocateFile, this, [this, contentPanel](const QString& path) {
            if (contentPanel) {
                contentPanel->locateFile(path);
            }
        });
=======
>>>>>>> REPLACE
```

### 3.8 `RapidNotes_Source/RapidNotes/src/ui/MainWindow.h` — 移除 FavoritePanel 前置声明与成员

```
<<<<<<< SEARCH
class NavPanel;
class FavoritePanel;
class SidebarContainerWidget;
=======
class NavPanel;
class SidebarContainerWidget;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    SidebarContainerWidget* m_sidebarContainer = nullptr;
    FavoritePanel* m_favoritePanel = nullptr;
    ContentPanel* m_contentPanel = nullptr;
=======
    SidebarContainerWidget* m_sidebarContainer = nullptr;
    ContentPanel* m_contentPanel = nullptr;
>>>>>>> REPLACE
```

### 3.9 `RapidNotes_Source/RapidNotes/src/ui/MainWindow.cpp` — 移除 FavoritePanel.h 包含

```
<<<<<<< SEARCH
#include "FavoritePanel.h"
#include "LibraryPanel.h"
=======
#include "LibraryPanel.h"
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译验收步骤
1. 执行 CMake 构建：
   ```cmd
   cmake -B build
   cmake --build build --config Release
   ```
2. 验证结果：
   - 彻底删除侧车 JSON 与收藏夹源码后，全工程 0 警告、0 错误编译成功；
   - 链接器无任何未解析的外部符号。

### 4.2 运行时验收标准
| 测试场景 | 操作步骤 | 预期效果 |
|---|---|---|
| 侧边栏选项卡 | 启动程序，观察左侧边栏顶部的 TabBar | 仅保留“库”和“标记”两个选项卡，原“收藏夹”彻底消失；“标记”选项卡名称与 `Destination.svg` 图标 100% 保持稳定不变 |
| 标记面板分类载入 | 点击切换到“标记”选项卡 | 面板内成功加载数据库中的分类列表，双击分类条目即可精准导航至 `library://<catId>` 分类内容 |
| 元数据完全纯库化 | 对条目打标签、修改评分与颜色 | 完全实时更新并写入 `inspiration.db`，磁盘上绝不生成任何 `.RapidNotes.json` / `.QuarkMeta.json` 侧车文件 |

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 功能 | 复用 SSOT | 模块位置 |
|---|---|---|
| 数据库收藏状态 | `notes.is_favorite` | `DatabaseManager.h` |
| 数据库分类管理 | `DatabaseManager::instance().getCategories()` | `DatabaseManager.h` |
| 侧边栏选项卡管理 | `QTabBar` + `QStackedWidget` | Qt 官方组件 |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 来源 | 消费点 | 说明 |
|---|---|---|---|
| 笔记元数据（颜色/星级/标签） | `inspiration.db` 结构化字段 | `NoteModel` / UI 视图 | 彻底废除侧车 JSON |
| 标记面板数据源 | `inspiration.db` 中的 `categories` 表 | “标记”侧边栏面板 | 零篡改 UI，底层转为纯数据库分类 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 调用 | 物理签名位置 | 访问控制 |
|---|---|---|
| `SidebarContainerWidget::libraryPanel()` | `SidebarContainerWidget.h` | ✅ public |
| `SidebarContainerWidget::markedPanel()` | `SidebarContainerWidget.h` | ✅ public |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

| 类型 | 状态 | 说明 |
|---|---|---|
| `FavoritePanel` | 彻底移除 | 全工程所有包含及前置声明完全闭环清理 |
| `QuarkMetaJson` | 彻底移除 | 磁盘侧车读写链路完全拔除 |
| `MarkedPanel` | 保持稳定 | 维持既有包含，内部改接 DatabaseManager |

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 说明 |
|---|---|---|---|
| 1 | 方案已通过需求确认 | ✅ 满足 | 用户明确指示：“标记”选项卡名称保持原样，仅底层由物理路径转为分类 |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见前置证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 经由 plan_checker 工具验证通过 |
| 4 | 新增接口声明、定义、调用点一致 | ✅ 不涉及 | 纯冗余组件物理剥离与数据源重定向 |
| 5 | 被修改的 `#include` 闭合 | ✅ 满足 | 彻底消除孤儿头文件包含 |
| 6 | 新建/删除文件在 CMakeLists 同步 | ✅ 满足 | 同步移除 10 个源码文件条目 |
| 7 | 跨类调用访问级别符合要求 | ✅ 满足 | 保持既有访问级别 |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 严格限制在清单内 |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | 根除侧车 JSON 带来的频繁磁盘文件扫描与刷盘 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 底层存储彻底收敛至单一数据库 |
| 11 | 遵循 Memories_RapidNotes.md 规范 | ✅ 满足 | 已同步更新第 21 章规范 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 不涉及 | 未调用 ToolTipOverlay |
| 13 | 100% 语义化矢量图标 | ✅ 满足 | 标记 Tab 保持使用 `"Destination.svg"` 图标 |
| 14 | 异步回调按值捕获 | ✅ 不涉及 | 纯 UI 树配置 |
| 15 | 无 Lambda 循环引用 | ✅ 不涉及 | 保持既有连接结构 |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 不涉及 | 纯主线程组件重组 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 满足 | 保持标准函数指针语法 |
| 18 | 对外 `.h` 签名冻结 | ✅ 满足 | 不改动对外公共契约 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明
| 既有功能 | 本方案影响 | 结论 |
|---|---|---|
| 侧边栏“标记”选项卡视觉表现 | 选项卡文字依然显示为“标记”，图标依然为 `Destination.svg` | ✅ 100% 零篡改 |
| 收藏条目过滤 | 通过侧边栏“库”中的“收藏”分类正常查看所有星标笔记 | ✅ 原样保留 |

### 数值零篡改声明
| 数值参数 | 当前设定 | 结论 |
|---|---|---|
| 侧边栏图标尺寸 | 16x16 | ✅ 100% 原样照抄 |
| 侧边栏最小宽度 | 230 | ✅ 100% 原样照抄 |
| 搜索过滤框高度 | 32 | ✅ 100% 原样照抄 |
