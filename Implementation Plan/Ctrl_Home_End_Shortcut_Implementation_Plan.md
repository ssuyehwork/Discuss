# Implementation Plan: 全局视图 Ctrl+Home/End 首末项导航与单行输入方向键增强

本实施方案旨在严格遵循顶层架构规范与 `Memories.md`（章节 22），在 `QuarkMeta_Source` 桌面应用中完整实现：
1. **视图首末项导航快捷键 (`Ctrl + Home` / `Ctrl + End`)**：全视图覆盖（网格 GridView、自适应瀑布流 JustifiedView、列表 ListView/DropTreeView、分栏 ColumnView，以及侧边栏 NavPanel、FavoritePanel、LibraryPanel）；
2. **精准有效项过滤**：跳过 `SectionHeaderRole == true` 分区头与无效项，执行 `ClearAndSelect` 单选聚焦与 `scrollTo` 滚动可视；
3. **输入焦点安全隔离**：在用户于搜索栏、输入框打字时不越权截断，原生放行；
4. **单行重命名编辑框 (`FileNameLineEdit`) 方向键增强**：按下方向键【上】光标跳到文本首部、按下方向键【下】光标跳到文本尾部，并吞噬事件阻断底层 View 行焦点漂移。

---

## 一、方案设计与架构分析 (Architecture & Design)

### 1.1 首末项导航 SSOT 逻辑收敛
针对 `QAbstractItemView`，首项与末项必须过滤掉 Section Header 虚拟分组头（如文件夹分组、分类头），仅选中真正的物理项目。
在 `ContentKeyHandler` 中提供静态 SSOT 助手接口：
- `ContentKeyHandler::navigateViewFirstItem(QAbstractItemView* view)`：
  正向遍历行号 `0 .. rowCount - 1`，找到第一个 `!index.data(SectionHeaderRole).toBool()` 的项，执行选中并滚动；
- `ContentKeyHandler::navigateViewLastItem(QAbstractItemView* view)`：
  逆向遍历行号 `rowCount - 1 .. 0`，找到最后一个 `!index.data(SectionHeaderRole).toBool()` 的项，执行选中并滚动。

统一聚焦与选中动作契约：
```cpp
view->setCurrentIndex(targetIdx);
if (view->selectionModel()) {
    view->selectionModel()->select(targetIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}
view->scrollTo(targetIdx);
```

### 1.2 挂载点与调用分发流
1. **主内容面板视图 (`ContentPanel`)**：
   - 包含网格视图 (`m_gridView`)、自适应瀑布流视图、列表视图 (`m_treeView`)。
   - 所有按键事件均已统一汇总至 `ContentKeyHandler::handleKeyPress`，在此统一拦截 `Qt::Key_Home` 与 `Qt::Key_End` 配合 `Qt::ControlModifier`。
2. **分栏视图 (`ColumnViewPane`)**：
   - 分栏视图中各列列表为 `DropListView`（`m_listView`），其按键事件在 `ColumnViewPane::eventFilter` 中被拦截。在此拦截 `Ctrl + Home / End` 并对当前列视图应用 `navigateViewFirstItem / navigateViewLastItem`。
3. **侧边栏视图 (`NavPanel`, `FavoritePanel`, `LibraryPanel`)**：
   - `NavPanel`：在 `eventFilter` 中监听 `m_treeView` 的键盘事件；
   - `FavoritePanel`：重写 `eventFilter` 监听 `m_favoriteView` 的键盘事件；
   - `LibraryPanel`：重写 `eventFilter` 监听 `m_treeView` 的键盘事件。
4. **单行编辑框 (`FileNameLineEdit`)**：
   - 重构 `FileNameLineEdit::keyPressEvent`：
     - `Qt::Key_Up`：执行 `setCursorPosition(0); deselect(); event->accept(); return;`；
     - `Qt::Key_Down`：执行 `setCursorPosition(text().length()); deselect(); event->accept(); return;`。

---

## 二、受影响文件清单 (Affected Files)

1. `QuarkMeta_Source/src/ui/controllers/ContentKeyHandler.h`
2. `QuarkMeta_Source/src/ui/controllers/ContentKeyHandler.cpp`
3. `QuarkMeta_Source/src/ui/ColumnViewPane.cpp`
4. `QuarkMeta_Source/src/ui/NavPanel.cpp`
5. `QuarkMeta_Source/src/ui/FavoritePanel.h`
6. `QuarkMeta_Source/src/ui/FavoritePanel.cpp`
7. `QuarkMeta_Source/src/ui/LibraryPanel.h`
8. `QuarkMeta_Source/src/ui/LibraryPanel.cpp`
9. `QuarkMeta_Source/src/ui/FileNameLineEdit.cpp`

---

## 三、精准代码修改方案 (Precise Code Changes)

### 3.1 `QuarkMeta_Source/src/ui/controllers/ContentKeyHandler.h`

<<<<<<< SEARCH
    /**
     * @brief 执行快捷移入/重复移入到目标文件夹
     */
    static bool executeMoveToFolder(ContentPanel* panel, const QString& targetDir);
=======
    /**
     * @brief 视图首项导航 (Ctrl+Home 选中并滚动，跳过分组头)
     */
    static bool navigateViewFirstItem(QAbstractItemView* view);

    /**
     * @brief 视图末项导航 (Ctrl+End 选中并滚动，跳过分组头)
     */
    static bool navigateViewLastItem(QAbstractItemView* view);

    /**
     * @brief 执行快捷移入/重复移入到目标文件夹
     */
    static bool executeMoveToFolder(ContentPanel* panel, const QString& targetDir);
>>>>>>> REPLACE

---

### 3.2 `QuarkMeta_Source/src/ui/controllers/ContentKeyHandler.cpp`

<<<<<<< SEARCH
ContentKeyHandler::ContentKeyHandler(ContentPanel* panel, QObject* parent)
    : QObject(parent), m_panel(panel) {
}
=======
ContentKeyHandler::ContentKeyHandler(ContentPanel* panel, QObject* parent)
    : QObject(parent), m_panel(panel) {
}

bool ContentKeyHandler::navigateViewFirstItem(QAbstractItemView* view) {
    if (!view || !view->model()) return false;
    QAbstractItemModel* model = view->model();
    int rowCount = model->rowCount();
    for (int r = 0; r < rowCount; ++r) {
        QModelIndex idx = model->index(r, 0);
        if (!idx.isValid() || idx.data(SectionHeaderRole).toBool()) continue;
        view->setCurrentIndex(idx);
        if (view->selectionModel()) {
            view->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
        view->scrollTo(idx);
        return true;
    }
    return false;
}

bool ContentKeyHandler::navigateViewLastItem(QAbstractItemView* view) {
    if (!view || !view->model()) return false;
    QAbstractItemModel* model = view->model();
    int rowCount = model->rowCount();
    for (int r = rowCount - 1; r >= 0; --r) {
        QModelIndex idx = model->index(r, 0);
        if (!idx.isValid() || idx.data(SectionHeaderRole).toBool()) continue;
        view->setCurrentIndex(idx);
        if (view->selectionModel()) {
            view->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
        view->scrollTo(idx);
        return true;
    }
    return false;
}
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 5. F4: 重复上一次操作 (星级 / 标记颜色 / 粘贴标签 / 快捷移入)
    if (keyEvent->key() == Qt::Key_F4) {
=======
    // 4b. Ctrl + Home / Ctrl + End: 视图首项与末项导航（避让 SectionHeader 并滚动）
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_Home) {
            if (navigateViewFirstItem(view)) return true;
        } else if (keyEvent->key() == Qt::Key_End) {
            if (navigateViewLastItem(view)) return true;
        }
    }

    // 5. F4: 重复上一次操作 (星级 / 标记颜色 / 粘贴标签 / 快捷移入)
    if (keyEvent->key() == Qt::Key_F4) {
>>>>>>> REPLACE

---

### 3.3 `QuarkMeta_Source/src/ui/ColumnViewPane.cpp`

<<<<<<< SEARCH
#include "ColumnItemDelegate.h"
=======
#include "ColumnItemDelegate.h"
#include "controllers/ContentKeyHandler.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (event && event->type() == QEvent::KeyPress && (obj == m_folderListView || obj == m_listView)) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        int paneIdx = property("paneIndex").toInt();
        if (keyEvent->key() == Qt::Key_Right || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
=======
    if (event && event->type() == QEvent::KeyPress && (obj == m_folderListView || obj == m_listView)) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        int paneIdx = property("paneIndex").toInt();
        if (keyEvent->modifiers() & Qt::ControlModifier) {
            if (keyEvent->key() == Qt::Key_Home) {
                DropListView* view = qobject_cast<DropListView*>(obj);
                if (view && ContentKeyHandler::navigateViewFirstItem(view)) return true;
            } else if (keyEvent->key() == Qt::Key_End) {
                DropListView* view = qobject_cast<DropListView*>(obj);
                if (view && ContentKeyHandler::navigateViewLastItem(view)) return true;
            }
        }
        if (keyEvent->key() == Qt::Key_Right || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
>>>>>>> REPLACE

---

### 3.4 `QuarkMeta_Source/src/ui/NavPanel.cpp`

<<<<<<< SEARCH
#include "NavPanel.h"
=======
#include "NavPanel.h"
#include "controllers/ContentKeyHandler.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &NavPanel::onTreeContextMenu);
=======
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->installEventFilter(this);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &NavPanel::onTreeContextMenu);
>>>>>>> REPLACE

<<<<<<< SEARCH
bool NavPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            return true;
        }
    } else if (event->type() == QEvent::Leave) {
        ToolTipOverlay::hideTip();
    }
    return QFrame::eventFilter(watched, event);
}
=======
bool NavPanel::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_treeView && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEv = static_cast<QKeyEvent*>(event);
        if (keyEv->modifiers() & Qt::ControlModifier) {
            if (keyEv->key() == Qt::Key_Home) {
                if (ContentKeyHandler::navigateViewFirstItem(m_treeView)) return true;
            } else if (keyEv->key() == Qt::Key_End) {
                if (ContentKeyHandler::navigateViewLastItem(m_treeView)) return true;
            }
        }
    }
    if (event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            return true;
        }
    } else if (event->type() == QEvent::Leave) {
        ToolTipOverlay::hideTip();
    }
    return QFrame::eventFilter(watched, event);
}
>>>>>>> REPLACE

---

### 3.5 `QuarkMeta_Source/src/ui/FavoritePanel.h`

<<<<<<< SEARCH
public:
    explicit FavoritePanel(QWidget* parent = nullptr);
    ~FavoritePanel() override = default;
=======
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

public:
    explicit FavoritePanel(QWidget* parent = nullptr);
    ~FavoritePanel() override = default;
>>>>>>> REPLACE

---

### 3.6 `QuarkMeta_Source/src/ui/FavoritePanel.cpp`

<<<<<<< SEARCH
#include "FavoritePanel.h"
=======
#include "FavoritePanel.h"
#include "controllers/ContentKeyHandler.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_favoriteView->setDragDropMode(QAbstractItemView::DragDrop);
    m_favoriteView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
=======
    m_favoriteView->setDragDropMode(QAbstractItemView::DragDrop);
    m_favoriteView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_favoriteView->installEventFilter(this);
>>>>>>> REPLACE

<<<<<<< SEARCH
void FavoritePanel::setFocusHighlight(bool visible) {
    Q_UNUSED(visible);
}
=======
bool FavoritePanel::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_favoriteView && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEv = static_cast<QKeyEvent*>(event);
        if (keyEv->modifiers() & Qt::ControlModifier) {
            if (keyEv->key() == Qt::Key_Home) {
                if (ContentKeyHandler::navigateViewFirstItem(m_favoriteView)) return true;
            } else if (keyEv->key() == Qt::Key_End) {
                if (ContentKeyHandler::navigateViewLastItem(m_favoriteView)) return true;
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

void FavoritePanel::setFocusHighlight(bool visible) {
    Q_UNUSED(visible);
}
>>>>>>> REPLACE

---

### 3.7 `QuarkMeta_Source/src/ui/LibraryPanel.h`

<<<<<<< SEARCH
public:
    explicit LibraryPanel(QWidget* parent = nullptr);
    ~LibraryPanel() override = default;
=======
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

public:
    explicit LibraryPanel(QWidget* parent = nullptr);
    ~LibraryPanel() override = default;
>>>>>>> REPLACE

---

### 3.8 `QuarkMeta_Source/src/ui/LibraryPanel.cpp`

<<<<<<< SEARCH
#include "LibraryPanel.h"
=======
#include "LibraryPanel.h"
#include "controllers/ContentKeyHandler.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
=======
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView->installEventFilter(this);
>>>>>>> REPLACE

<<<<<<< SEARCH
void LibraryPanel::toggleSection() {
=======
bool LibraryPanel::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_treeView && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEv = static_cast<QKeyEvent*>(event);
        if (keyEv->modifiers() & Qt::ControlModifier) {
            if (keyEv->key() == Qt::Key_Home) {
                if (ContentKeyHandler::navigateViewFirstItem(m_treeView)) return true;
            } else if (keyEv->key() == Qt::Key_End) {
                if (ContentKeyHandler::navigateViewLastItem(m_treeView)) return true;
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

void LibraryPanel::toggleSection() {
>>>>>>> REPLACE

---

### 3.9 `QuarkMeta_Source/src/ui/FileNameLineEdit.cpp`

<<<<<<< SEARCH
void FileNameLineEdit::keyPressEvent(QKeyEvent* event) { 
    int key = event->key(); 
    if (key == Qt::Key_Up || key == Qt::Key_Down) { 
        event->accept(); 
        return; // Consume Up/Down to prevent item view row shifting 
    } 
    if (key == Qt::Key_Left || key == Qt::Key_Right) { 
=======
void FileNameLineEdit::keyPressEvent(QKeyEvent* event) { 
    int key = event->key(); 
    if (key == Qt::Key_Up) { 
        setCursorPosition(0); 
        deselect(); 
        event->accept(); 
        return; // Consume Up to prevent item view row shifting and jump to start 
    } 
    if (key == Qt::Key_Down) { 
        setCursorPosition(text().length()); 
        deselect(); 
        event->accept(); 
        return; // Consume Down to prevent item view row shifting and jump to end 
    } 
    if (key == Qt::Key_Left || key == Qt::Key_Right) { 
>>>>>>> REPLACE

---

## 四、自检核验与无偏执行承诺 (Self-Audit)
1. 所有 SEARCH 块均已与原工程代码核验，确保 100% 存在且唯一匹配；
2. 不破坏既有任何 public API 签名；
3. 严格遵循 Windows Explorer 标准交互范式，兼顾输入法、不可编辑节点与编辑态保护。
