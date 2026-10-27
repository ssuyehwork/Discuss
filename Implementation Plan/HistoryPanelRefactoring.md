# HistoryPanel Selection Highlight & ToolTip Overlay Refactoring

**Base Commit Hash**: `21871189bf5a34f6ffbe0829c2cefe9a654975b7`
**Prerequisite Plans**: None
**Supersedes**: None

---

## Chapter 1: Overview

### 根因分析
在历史访问记录面板 `HistoryPanel`（位于 `src/ui/HistoryPanel.cpp`）中存在两个违反界面一致性与架构规范的问题：
1. **选中项未显示高亮**：`HistoryPanel` 使用了 `QTreeWidget` (`m_treeWidget`)，但并未给 `m_treeWidget` 设置特定 `objectName`，且在 `resources/style.qss` 中缺乏针对 `QTreeWidget` 的选择项视觉样式描述，导致用户在历史记录列表中单击项时无法呈现背景高亮（`#094771`）。
2. **违规调用原生 `setToolTip`**：在 `HistoryPanel::createHistoryItem` 中直接调用了原生 Qt API `item->setToolTip(0, QDir::toNativeSeparators(path));`。根据 `Memories.md` 规范，全应用严禁使用原生 `setToolTip`，必须且只能使用 `ToolTipOverlay` 控件，导致悬浮提示呈现为原生的半透明白色气泡框而非全局一致的主题悬浮提示。

### 重构方案与目标
1. 为 `m_treeWidget` 设置对象名称 `HistoryTreeWidget`，并在 `resources/style.qss` 中补充对应的选中高亮（`background-color: #094771;`）及悬停样式，保持侧边栏视觉语言统一。
2. 物理彻底扫除 `item->setToolTip(0, ...)` 调用，并在 `HistoryPanel::eventFilter` 中拦截 `m_treeWidget->viewport()` 的 `QEvent::MouseMove` / `QEvent::HoverMove` 与 `QEvent::Leave` 事件，通过 `ToolTipOverlay::instance()->showText(...)` 和 `ToolTipOverlay::hideTip()` 托管悬浮气泡显示。

---

## Chapter 2: Modified Files List

1. `resources/style.qss`
2. `src/ui/HistoryPanel.h`
3. `src/ui/HistoryPanel.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes

### 3.1 `resources/style.qss`

```
<<<<<<< SEARCH
/* 统一 QTreeView 列表视图暗色斑马纹与高亮 */
QTreeView {
=======
/* 历史记录面板 QTreeWidget 选中高亮与悬停样式 */
QTreeWidget#HistoryTreeWidget {
    background-color: #1E1E1E;
    color: #EEEEEE;
    border: none;
    outline: 0;
}
QTreeWidget#HistoryTreeWidget::item {
    height: 28px;
    border: none;
    padding-left: 4px;
}
QTreeWidget#HistoryTreeWidget::item:hover {
    background-color: #2A2D2E;
}
QTreeWidget#HistoryTreeWidget::item:selected {
    background-color: #094771;
    color: #FFFFFF;
}

/* 统一 QTreeView 列表视图暗色斑马纹与高亮 */
QTreeView {
>>>>>>> REPLACE
```

### 3.2 `src/ui/HistoryPanel.h`

```
<<<<<<< SEARCH
    QVBoxLayout* m_mainLayout = nullptr;
    QTreeWidget* m_treeWidget = nullptr;
    bool m_isLoaded = false;
};
=======
    QVBoxLayout* m_mainLayout = nullptr;
    QTreeWidget* m_treeWidget = nullptr;
    bool m_isLoaded = false;
    QString m_lastHoveredPath;
};
>>>>>>> REPLACE
```

### 3.3 `src/ui/HistoryPanel.cpp`

```
<<<<<<< SEARCH
#include "HistoryPanel.h"
#include "UiHelper.h"
=======
#include "HistoryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(0);
    m_treeWidget->setIconSize(QSize(16, 16));
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeWidget->setDragEnabled(true);
=======
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setObjectName("HistoryTreeWidget");
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(0);
    m_treeWidget->setIconSize(QSize(16, 16));
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeWidget->setDragEnabled(true);
    m_treeWidget->setMouseTracking(true);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QTreeWidgetItem* HistoryPanel::createHistoryItem(const QString& path) {
    QFileInfo fi(path);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, fi.fileName());
    item->setData(0, Qt::UserRole, path);
    item->setToolTip(0, QDir::toNativeSeparators(path));
=======
QTreeWidgetItem* HistoryPanel::createHistoryItem(const QString& path) {
    QFileInfo fi(path);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, fi.fileName());
    item->setData(0, Qt::UserRole, path);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool HistoryPanel::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_treeWidget->viewport() && event->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->buttons() & Qt::LeftButton) {
            QTreeWidgetItem* item = m_treeWidget->itemAt(mouseEvent->pos());
            if (item) {
                QString path = item->data(0, Qt::UserRole).toString();
                if (!path.isEmpty()) {
                    QDrag* drag = new QDrag(m_treeWidget);
                    QMimeData* mimeData = new QMimeData();
                    mimeData->setUrls({QUrl::fromLocalFile(path)});
                    drag->setMimeData(mimeData);
                    drag->exec(Qt::CopyAction);
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
=======
bool HistoryPanel::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_treeWidget->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->buttons() & Qt::LeftButton) {
                QTreeWidgetItem* item = m_treeWidget->itemAt(mouseEvent->pos());
                if (item) {
                    QString path = item->data(0, Qt::UserRole).toString();
                    if (!path.isEmpty()) {
                        ToolTipOverlay::hideTip();
                        QDrag* drag = new QDrag(m_treeWidget);
                        QMimeData* mimeData = new QMimeData();
                        mimeData->setUrls({QUrl::fromLocalFile(path)});
                        drag->setMimeData(mimeData);
                        drag->exec(Qt::CopyAction);
                        return true;
                    }
                }
            } else {
                QTreeWidgetItem* item = m_treeWidget->itemAt(mouseEvent->pos());
                if (item) {
                    QString path = item->data(0, Qt::UserRole).toString();
                    if (!path.isEmpty()) {
                        QString nativePath = QDir::toNativeSeparators(path);
                        if (m_lastHoveredPath != nativePath) {
                            m_lastHoveredPath = nativePath;
                            ToolTipOverlay::instance()->showText(QCursor::pos(), nativePath, 0);
                        }
                    } else {
                        m_lastHoveredPath.clear();
                        ToolTipOverlay::hideTip();
                    }
                } else {
                    m_lastHoveredPath.clear();
                    ToolTipOverlay::hideTip();
                }
            }
        } else if (event->type() == QEvent::Leave) {
            m_lastHoveredPath.clear();
            ToolTipOverlay::hideTip();
        }
    }
    return QWidget::eventFilter(watched, event);
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps

### 构建命令
```bash
cmake -B build -G Ninja
cmake --build build --config Release
```

### 运行时验收标准
1. **选中高亮表现**：打开历史记录面板（`HistoryPanel`），在历史列表中单击任何项目，该项背景立刻高亮显示为蓝底白字（`#094771`）。
2. **悬浮提示表现**：将鼠标悬停在历史记录项目上，弹出的提示框为全局统一的暗色深灰框（由 `ToolTipOverlay` 绘制），原生半透明白框提示彻底消失。
3. **鼠标移出清理**：鼠标移出历史记录控件后，`ToolTipOverlay` 提示气泡自动隐去，无任何视觉残留。

---

## Chapter 5: Official API Reuse & Anti-Redundancy

- **`ToolTipOverlay::instance()`**：完全复用 QuarkMeta 的全局统一 Tooltip 控件 `ToolTipOverlay`，禁止手写局部 QLabel 或弹窗，符合防另起炉灶与样式 SSOT 契约。
- **`QDir::toNativeSeparators()`**：复用 Qt 标准 API 规范路径展示形态。

---

## Chapter 6: Data SSOT Declaration Table

| 数据项 | 唯一 SSOT 源 | 派生/视图副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 历史纪录列表路径 | `HistoryService` (`global.db` / `AppConfig`) | `QTreeWidget` / `QTreeWidgetItem::data` | `HistoryService` 信号广播 ➔ `HistoryPanel` 刷新节点 |
| 悬停路径文案 | `QTreeWidgetItem::data(0, Qt::UserRole)` | `m_lastHoveredPath` | 鼠标 Hover 事件触发 ➔ `ToolTipOverlay` 渲染 |

---

## Chapter 7: Interface & Visibility Consistency Check

1. **外部调用接口物理签名**：
   - `HistoryPanel::HistoryPanel(QWidget* parent = nullptr)`声明在 `src/ui/HistoryPanel.h` 第 14 行。
2. **新增私有变量**：
   - `QString m_lastHoveredPath;` 声明在 `src/ui/HistoryPanel.h` 私有区域，无跨类暴露隐患。
3. **访问控制验证**：
   - `eventFilter` 重写自 `QWidget::eventFilter`，修饰符为 `protected`，访问级别完全合规。

---

## Chapter 8: Header Chain & Inclusion Closure Check

- `src/ui/HistoryPanel.cpp` 显式包含 `"ToolTipOverlay.h"`，确保 `ToolTipOverlay::instance()` 和 `ToolTipOverlay::hideTip()` 的完整类型定义可被正确解析。
- `src/ui/HistoryPanel.h` 内部已有 `<QTreeWidget>`, `<QVBoxLayout>`, `<QMenu>`, `<QIcon>`，包含闭合性完全满足需求。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 序号 | 检查项 | 结果 | 位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 严禁盲改对外签名 (Public API Freezing) | 满足 | `HistoryPanel.h` 公有 API 未做任何变更 |
| 2 | 严格单向依赖流动 | 满足 | UI 层依赖自身 `ToolTipOverlay` 与 `UiHelper` |
| 3 | 架构三问先答后写 | 满足 | 第一章已分析 SSOT、黑盒与根因 |
| 4 | 静态红线扫描 | 满足 | 无 Win32 强抢焦点或原生 `setToolTip` |
| 5 | 黑盒隔离锁 | 满足 | 模块私有状态内聚在 `HistoryPanel` 内部 |
| 6 | 绝对禁止修改业务逻辑碰运气 | 满足 | 手写最小化事件过滤修复 |
| 7 | 单一变量隔离验证 | 满足 | 仅修改 `HistoryPanel` 控件样式与 Tooltip 逻辑 |
| 8 | 物理路径 SSOT 入口校验 | 满足 | 统一走 `ToolTipOverlay` 入口 |
| 9 | 物理彻底清除死代码 | 满足 | 删除原生 `setToolTip` |
| 10 | 界面样式无 C++ 硬编码内联 | 满足 | QSS 样式写在 `resources/style.qss` |
| 11 | 数值与交互零篡改 | 满足 | 高亮色与暗色主题一致，交互不变 |
| 12 | CMakeLists.txt 全注册 | 满足 | 无新增 `.cpp`/`.h` 文件，不涉及注册变化 |
| 13 | 10 大章节结构完整 | 满足 | 包含 Chapter 1 至 10 |
| 14 | SEARCH 块唯一匹配 | 满足 | 代码在源文件中唯一匹配 |
| 15 | 图标/颜色语义化复用 | 满足 | 复用 `ToolTipOverlay` |
| 16 | 线程安全的主线程 UI 操作 | 满足 | 全部事件响应与绘制在 GUI 主线程执行 |
| 17 | 语言规范纯中文表达 | 满足 | 方案正文全中文表述 |
| 18 | 物理隔离目录定位 | 满足 | 存放于 `Implementation Plan/HistoryPanelRefactoring.md` |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table

### 行为与交互零篡改
- 双击历史记录项导航至文件、右键上下文菜单移除/清空历史、拖拽历史项至外部等交互行为 100% 保持不变。

### 样式数值零篡改
- 选中背景色 `#094771` 严照暗色主题 `QListView#ColumnViewPaneListView::item:selected` 的高亮蓝标准；
- 悬停背景色 `#2A2D2E` 严格参照 `QTreeView` 项目 Hover 标准；
- 行高 `28px` 与 `QTreeView::item` 标准行高完全一致。
