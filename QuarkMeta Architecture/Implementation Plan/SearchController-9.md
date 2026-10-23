# SearchController-9.md - 搜索控制器指针安全与重入崩溃根治实施方案

- **Base Commit Hash**: `a55d3d63d38a44b4f4a112d0797b1c00e88ad571`
- **Prerequisite Plans**: 无
- **Supersedes**: `superseded/SearchController-8.md`

---

## 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `src/ui/SearchController.h`
   - `src/ui/SearchController.cpp`
2. **头文件原样签名核查**：
   - `void setActiveContentPanel(ContentPanel* panel)` (`src/ui/SearchController.h:38`)
   - `void bindContentPanel(ContentPanel* contentPanel)` (`src/ui/SearchController.h:36`)
3. **既有 API 检索日志**：
   - `grep -rn "m_contentPanel" src/ui/SearchController.h`：发现 `ContentPanel* m_contentPanel = nullptr;`

---

## Chapter 1: Overview（概述与根因分析）

### 1. 根因分析
在软件运行过程中（尤其是切换标签页、多窗格分屏或关闭窗格时），既有的 `ContentPanel` 实例可能会被销毁。
然而，在 `SearchController` 中，关联的目标面板指针被声明为 C++ 裸指针 `ContentPanel* m_contentPanel`。当 `ContentPanel` 被销毁后，`m_contentPanel` 未能被清空或更新，成为**悬垂野指针（Dangling Wild Pointer）**。
在搜索框中进行搜索时，执行 `m_contentPanel->search(keyword)` 访问已被释放的废弃内存，直接引发 Windows `0xc0000005` 内存访问违规闪退（Access Violation Crash）。

同时，`SearchHistoryPanel::historyItemClicked` 点击事件触发时，原同步回调在同一个事件循环栈深处直接执行搜索并重构历史面板控件，导致点击的列表项被同步 `deleteLater()` 销毁，`const QString& keyword` 参数变为失效内存（Use-After-Free）。

### 2. 重构设计目标
1. 将 `SearchController` 中的 `m_contentPanel` 升级为 Qt 安全智能指针 `QPointer<ContentPanel>`。当引用的 `ContentPanel` 被物理销毁时，`QPointer` 会被 Qt 自动重置为 `nullptr`，彻底杜绝悬垂野指针访问。
2. 将点击历史记录词条的回调解耦为**传值拷贝（`QString keyword`） + `Qt::QueuedConnection` 队列异步调度**，彻底切断事件流重入自毁与失效内存引用链。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/SearchController.h` （修改）
2. `src/ui/SearchController.cpp` （修改）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/SearchController.h`
```
<<<<<<< SEARCH
#include <QEvent>

namespace QuarkMeta {
=======
#include <QEvent>
#include <QPointer>

namespace QuarkMeta {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    SearchHistoryPanel* m_searchHistoryPanel = nullptr;
    ContentPanel* m_contentPanel = nullptr;
    SearchScope m_searchScope = SearchScope::CurrentFolder;
=======
    SearchHistoryPanel* m_searchHistoryPanel = nullptr;
    QPointer<ContentPanel> m_contentPanel;
    SearchScope m_searchScope = SearchScope::CurrentFolder;
>>>>>>> REPLACE
```

### 2. `src/ui/SearchController.cpp`
```
<<<<<<< SEARCH
    connect(m_searchHistoryPanel, &SearchHistoryPanel::historyItemClicked, this, [this](const QString& keyword) {
        m_searchEdit->setText(keyword);
        doSearch(keyword);
    });
=======
    connect(m_searchHistoryPanel, &SearchHistoryPanel::historyItemClicked, this, [this](QString keyword) {
        if (m_searchHistoryPanel) {
            m_searchHistoryPanel->hide();
        }
        m_searchEdit->setText(keyword);
        QMetaObject::invokeMethod(this, [this, keyword]() {
            doSearch(keyword);
        }, Qt::QueuedConnection);
    });
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 构建命令
```bash
cmake --build build --config Debug
```

### 2. 运行时验收标准
1. **标签页/窗格切换搜索安全性**：
   - 频繁切换标签页或关闭多窗格后，在搜索框中输入搜索词条。
   - 验证：系统不会发生闪退，搜索能安全定位到当前活跃的 `ContentPanel` 窗格。
2. **搜索历史点击安全性**：
   - 双击搜索框唤出历史搜索记录弹窗，点击任意历史词条。
   - 验证：搜索框内容正确填充，历史弹窗及时隐藏，搜索结果正常刷出且无任何程序崩溃。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用 Qt 官方 `QPointer` 机制自动追踪 `QObject` 对象生命周期，无须手写复杂的析构通知监听逻辑。
- 复用 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 切断同步事件深层重入，消除悬垂引用。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 活跃 ContentPanel 指针 | `PanelMediator::m_activeContentPanel` / `MainWindow` 布局 | `SearchController::m_contentPanel` (QPointer) | `setActiveContentPanel` 单向更新，销毁时自动归零 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 外部调用接口 / 成员变量 | 物理 .h 中的声明行 | 访问级别 |
| :--- | :--- | :--- |
| `setActiveContentPanel(ContentPanel* panel)` | `src/ui/SearchController.h:38` | `public` |
| `m_contentPanel` | `src/ui/SearchController.h:58` | `private` (`QPointer<ContentPanel>`) |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `src/ui/SearchController.h` 显式引入 `<QPointer>` 头文件，保障 `QPointer<ContentPanel>` 类型完整定义。
- `src/ui/SearchController.cpp` 中已正常引入 `ContentPanel.h`。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 关联位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 方案位于物理隔离目录 `QuarkMeta Architecture/Implementation Plan/` | 满足 | `SearchController-9.md` |
| 2 | SEARCH 块原文存在且唯一 | 满足 | 精准校验 `SearchController.h` 和 `SearchController.cpp` |
| 3 | 10 大章节完整 | 满足 | Chapter 1 ~ Chapter 10 全齐 |
| 4 | 无 Win32 / Qt::Popup 等平台级 Hack | 满足 | 无任何非法 Hack 补丁 |
| 5 | 无主线程/后台线程 UI 交叉非法调用 | 满足 | 始终运行于 GUI 主线程 |
| 6 | 样式与数值 100% 保持一致 | 满足 | 不涉及 UI 样式修改 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

- **交互行为**：双击搜索框唤出历史搜索记录、点击历史记录填入搜索框并执行搜索的既有交互逻辑 100% 原样保留。
- **外观数值**：不涉及任何 UI 尺寸与颜色的修改，数值 100% 保持不变。
