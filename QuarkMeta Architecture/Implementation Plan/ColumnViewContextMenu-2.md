# ColumnViewContextMenu-2.md — 列视图“位置与视图目标脱节”缺陷彻底修复实施方案

---

## 🔒 架构三问 (Architecture 3-Questions Answer)

1. **第一问（真理源溯源 SSOT）**：
   上下文菜单（ContextMenu）响应的目标视图（`QAbstractItemView*`）与点击坐标（`QPoint`）的绑定关系，唯一权威源是谁？
   - 答：唯一权威源为发出右键信号或事件的**当前具体列视图组件本身**（如第 N 列的 `m_listView` 或 `ColumnViewPane`）。绝不能依赖全局猜测（如 `activeItemView()`）作为位置绑定的权威源。本次修复在接口传递中显式强绑定视图对象，保持状态一致。

2. **第二问（黑盒完整性 Black-box Integrity）**：
   本次修复是否符合【契约锁】规范？
   - 答：完全符合。既有的 `ContentPanel::onCustomContextMenuRequested(const QPoint& pos)` 签名保持不变且 100% 向后兼容；通过重载函数扩展 `ContentPanel::onCustomContextMenuRequested(QAbstractItemView* view, const QPoint& pos)` 签名，符合《SYSTEM_PROMPT.md》契约锁平滑扩展规则，不使用友元，不破坏封装。

3. **第三问（根因 vs 症状 Root Cause Analysis）**：
   为什么在第二列空白处右键单击，会误选中第三列的项目并弹出第三列的右键菜单？
   - 答：根本原因为**“位置来自 A 视图、但视图目标靠全局猜测”**（位置与视图目标脱节）。
     此前代码在转发空白区右键信号时仅传递了局部坐标 `viewPos`，未传递属于哪个 `view`。下游 `onCustomContextMenuRequested(pos)` 在 `sender()` 为空时回退到 `activeItemView()`，误拿了此前聚焦的第三列视图 `view3`。`view3->indexAt(viewPos)` 将第二列的坐标错套在第三列上计算，偶然命中第三列某一行，导致了跨列误选中与弹窗。

---

## 1. Overview（概述与解决的问题）

本方案物理消除列视图（Column View）中由于“位置坐标与目标视图脱节”导致的右键菜单跨列误触 Bug。

### 修复策略：
1. **`ContentPanel` 契约扩展重载**：在 `ContentPanel` 中新增 `onCustomContextMenuRequested(QAbstractItemView* view, const QPoint& pos)` 重载槽函数，允许调用方显式指定目标视图。
2. **`ColumnViewPane` 空白区精确强绑定**：在 `ColumnViewPane` 的 `handlePaneBlankContextMenu` 闭包中，将算得的 `viewPos` 与本列的 `targetView`（如 `m_listView` / `m_folderListView`）一同显式传给 `ContentPanel`。
3. **`ColumnBlankCanvasWidget` 视口强绑定**：最右侧延伸空白区明确传入 `activeItemView()` 及其视口局部坐标。
4. **子视图信号显示绑定**：将 `m_folderListView` 和 `m_listView` 的 `customContextMenuRequested` 信号改为显式带上视图指针的 lambda 转发，杜绝依赖 `sender()` 转换失败引发的隐患。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 修改点 1：`src/ui/ContentPanel.h`
在 `public slots` 区域添加 `onCustomContextMenuRequested` 重载槽函数声明。

```
<<<<<<< SEARCH
public slots:
    void setZoomLevel(int level);
    void onSelectionChanged();
    void onCustomContextMenuRequested(const QPoint& pos);
    void onDoubleClicked(const QModelIndex& index);
=======
public slots:
    void setZoomLevel(int level);
    void onSelectionChanged();
    void onCustomContextMenuRequested(const QPoint& pos);
    void onCustomContextMenuRequested(QAbstractItemView* view, const QPoint& pos);
    void onDoubleClicked(const QModelIndex& index);
>>>>>>> REPLACE
```

### 修改点 2：`src/ui/ContentPanel.cpp`
实现 `onCustomContextMenuRequested(QAbstractItemView* view, const QPoint& pos)` 重载函数，原单参数版本平滑转发给重载版本。

```
<<<<<<< SEARCH
void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
    if (!view) view = activeItemView();
    if (!view) return;
    ContentContextMenu menuHandler(this);
    menuHandler.showMenu(view, pos);
}
=======
void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
    onCustomContextMenuRequested(view, pos);
}

void ContentPanel::onCustomContextMenuRequested(QAbstractItemView* view, const QPoint& pos) {
    if (!view) view = activeItemView();
    if (!view) return;
    ContentContextMenu menuHandler(this);
    menuHandler.showMenu(view, pos);
}
>>>>>>> REPLACE
```

### 修改点 3：`src/ui/ColumnViewWidget.cpp`
更新 `ColumnBlankCanvasWidget::onContextMenuRequested`，显式传递 `view` 对象。

```
<<<<<<< SEARCH
private:
    void onContextMenuRequested(const QPoint& pos) {
        if (m_contentPanel) {
            QPoint globalPos = mapToGlobal(pos);
            QAbstractItemView* view = m_contentPanel->activeItemView();
            if (view && view->viewport()) {
                QPoint viewPos = view->viewport()->mapFromGlobal(globalPos);
                m_contentPanel->onCustomContextMenuRequested(viewPos);
            } else {
                m_contentPanel->onCustomContextMenuRequested(globalPos);
            }
        }
    }
=======
private:
    void onContextMenuRequested(const QPoint& pos) {
        if (m_contentPanel) {
            QPoint globalPos = mapToGlobal(pos);
            QAbstractItemView* view = m_contentPanel->activeItemView();
            if (view && view->viewport()) {
                QPoint viewPos = view->viewport()->mapFromGlobal(globalPos);
                m_contentPanel->onCustomContextMenuRequested(view, viewPos);
            } else {
                m_contentPanel->onCustomContextMenuRequested(nullptr, globalPos);
            }
        }
    }
>>>>>>> REPLACE
```

### 修改点 4：`src/ui/ColumnViewWidget.cpp`
更新 `ColumnViewPane` 中的 `handlePaneBlankContextMenu` 闭包与列表视图信号连接，显式绑定目标 `targetView` / `m_listView` / `m_folderListView`。

```
<<<<<<< SEARCH
    auto handlePaneBlankContextMenu = [this](const QPoint& pos, QWidget* sourceWidget) {
        if (!m_contentPanel) return;
        QPoint globalPos = sourceWidget ? sourceWidget->mapToGlobal(pos) : QCursor::pos();
        DropListView* targetView = m_listView ? m_listView : m_folderListView;
        if (targetView && targetView->viewport()) {
            QPoint viewPos = targetView->viewport()->mapFromGlobal(globalPos);
            m_contentPanel->onCustomContextMenuRequested(viewPos);
        }
    };
=======
    auto handlePaneBlankContextMenu = [this](const QPoint& pos, QWidget* sourceWidget) {
        if (!m_contentPanel) return;
        QPoint globalPos = sourceWidget ? sourceWidget->mapToGlobal(pos) : QCursor::pos();
        DropListView* targetView = m_listView ? m_listView : m_folderListView;
        if (targetView && targetView->viewport()) {
            QPoint viewPos = targetView->viewport()->mapFromGlobal(globalPos);
            m_contentPanel->onCustomContextMenuRequested(targetView, viewPos);
        }
    };
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        connect(m_folderListView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
        connect(m_listView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
=======
        connect(m_folderListView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_folderListView, pos);
            }
        });
        connect(m_listView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_listView, pos);
            }
        });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 编译步骤：
在软件根目录下执行 CMake 构建命令：
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### 验证步骤：
1. **跨列点击复现验证**：
   - 展开三列（第一列、第二列、第三列），先在第三列任意点击某个文件使其处于选中/活跃状态。
   - 鼠标移至**第二列的空白处**右键单击（即用户截图中的场景）。
   - **预期结果**：菜单在第二列鼠标点击位置精确弹出，且**绝对不会**误选中第三列的项目，也不会弹出第三列文件的右键菜单。
2. **常规列表项右键验证**：
   - 在任意列的具体文件或文件夹上右键单击。
   - **预期结果**：正常选中该项并弹出对象右键菜单。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- **SSOT 通道复用**：彻底消除了跨模块调用时靠 `activeItemView()` 盲猜视图的死穴，统一通过重载接口 `onCustomContextMenuRequested(view, pos)` 进行明确物理绑定。
- **物理死代码清理**：彻底替换掉了依靠单坐标隐式猜测上下文视图的漏洞代码。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / API | 物理声明文件 | 精确物理签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `ContentPanel::onCustomContextMenuRequested` (单参数) | `src/ui/ContentPanel.h` | `void onCustomContextMenuRequested(const QPoint& pos)` | ✅ 向后兼容保留 |
| `ContentPanel::onCustomContextMenuRequested` (双参数) | `src/ui/ContentPanel.h` | `void onCustomContextMenuRequested(QAbstractItemView* view, const QPoint& pos)` | ✅ 重载新增 |
| `ContentPanel::activeItemView` | `src/ui/ContentPanel.h` | `QAbstractItemView* activeItemView() const` | ✅ 匹配一致 |
| `ColumnViewPane::listView` | `src/ui/ColumnViewWidget.h` | `DropListView* listView() const` | ✅ 匹配一致 |
| `ColumnViewPane::folderListView` | `src/ui/ColumnViewWidget.h` | `DropListView* folderListView() const` | ✅ 匹配一致 |
