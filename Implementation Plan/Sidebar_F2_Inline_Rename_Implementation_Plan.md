# Implementation Plan: 侧边栏树视图 F2 行内重命名缺陷修复实施方案

本实施方案旨在严格遵循顶层架构规范与 `Memories.md`（章节 23），在 `QuarkMeta_Source` 桌面应用中修复并完善：
1. **侧边栏树视图 F2 行内重命名补全**：修复 `FavoritePanel`（收藏夹）与 `LibraryPanel`（库面板）中因配置 `NoEditTriggers` 导致键盘按 `F2` 无法进入行内重命名的问题，与右键菜单“重命名”保持行为 100% 对称；
2. **系统保护节点阻断铁律**：对库面板内置固定项（如“全部数据”ID: -1、“未分类”ID: -2 及智能文件夹）实施编辑屏蔽保护，普通分类与收藏夹项平滑激活行内编辑。

---

## 一、方案设计与架构分析 (Architecture & Design)

### 1.1 问题根因与设计原则
- 侧边栏收藏夹（`FavoritePanel`）与库面板（`LibraryPanel`）为了避免鼠标误触发编辑，均显式设置了：
  ```cpp
  view->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ```
- 右键上下文菜单提供了“重命名”项，通过显式调用 `view->edit(index)` 可完美唤起行内编辑器。
- 缺陷在于视图未捕获键盘 `Qt::Key_F2`。为了恢复标准的桌面快捷键体验，在两个面板的事件过滤器（`eventFilter`）中拦截关联树视图的 `KeyPress` 事件：当 `key == Qt::Key_F2` 时，直接触发当前选中项的 `edit(currentIndex)`。

### 1.2 保护项拦截过滤
- `FavoritePanel`：当前选中的节点有效（`m_favoriteView->currentIndex().isValid()`）即可唤起编辑；
- `LibraryPanel`：获取 `curIdx.data(Qt::UserRole + 1).toInt()`。若 `nodeId <= 0`（系统内置项）或属于 `smart_folder`，则忽略 F2 不予编辑；若 `nodeId > 0` 且为普通自定义分类，则执行 `m_treeView->edit(curIdx)`。

---

## 二、受影响文件清单 (Affected Files)

1. `QuarkMeta_Source/src/ui/FavoritePanel.h`
2. `QuarkMeta_Source/src/ui/FavoritePanel.cpp`
3. `QuarkMeta_Source/src/ui/LibraryPanel.h`
4. `QuarkMeta_Source/src/ui/LibraryPanel.cpp`

---

## 三、精准代码修改方案 (Precise Code Changes)

### 3.1 `QuarkMeta_Source/src/ui/FavoritePanel.h`

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

### 3.2 `QuarkMeta_Source/src/ui/FavoritePanel.cpp`

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
        if (keyEv->key() == Qt::Key_F2 && keyEv->modifiers() == Qt::NoModifier) {
            QModelIndex curIdx = m_favoriteView->currentIndex();
            if (curIdx.isValid()) {
                m_favoriteView->edit(curIdx);
                return true;
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

### 3.3 `QuarkMeta_Source/src/ui/LibraryPanel.h`

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

### 3.4 `QuarkMeta_Source/src/ui/LibraryPanel.cpp`

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
        if (keyEv->key() == Qt::Key_F2 && keyEv->modifiers() == Qt::NoModifier) {
            QModelIndex curIdx = m_treeView->currentIndex();
            if (curIdx.isValid()) {
                int nodeId = curIdx.data(Qt::UserRole + 1).toInt();
                // 阻断内置保护项（ID: -1 全部数据, -2 未分类, -10 分组等）
                if (nodeId > 0 && curIdx.data(TypeRole).toString() != "smart_folder") {
                    m_treeView->edit(curIdx);
                    return true;
                }
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

void LibraryPanel::toggleSection() {
>>>>>>> REPLACE

---

## 四、自检核验与无偏执行承诺 (Self-Audit)
1. 方案完全独立物理隔离，不与任何其他快捷键逻辑相耦合；
2. 所有 SEARCH 块均已与原工程代码核验，确保 100% 存在且唯一匹配；
3. 不破坏既有任何 public API 签名。
