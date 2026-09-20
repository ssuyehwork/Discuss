# ColumnViewContextMenu-1.md — 列视图空白处右键菜单修复实施方案

---

## 🔒 架构三问 (Architecture 3-Questions Answer)

1. **第一问（真理源溯源 SSOT）**：
   列视图空白区域右键事件触发的目标路径与视图上下文的唯一真理源是谁？
   - 答：唯一真理源为该列所对应的 `ColumnViewPane`（持有 `currentPath`）以及 `ContentPanel`（持有全局导航与上下文状态）。本次修改不增加任何第二份状态，严格通过 `ColumnViewPane::currentPath()` 和既有的 `ContentPanel::onCustomContextMenuRequested` 传递与处理。

2. **第二问（黑盒完整性 Black-box Integrity）**：
   本次修复是否破坏了控件或视图层的封装？
   - 答：完全没有。`ColumnViewPane` 内部的 `m_canvasWidget` 与 `m_paneScrollArea` 作为视图层私有组件，捕获空白处右键事件后，仅将其物理坐标转换为 `listView` 的局部视角坐标并转发给既有 `ContentPanel` 信号槽通道，没有暴露任何私有内部实现，不使用 `friend`，不破坏封装。

3. **第三问（根因 vs 症状 Root Cause Analysis）**：
   为什么此前列视图空白处单击右键不会弹出菜单（或弹出错位）？
   - 答：根本原因有二：
     1. `m_listView` 在 `ColumnViewPane` 中被硬性裁剪高度 (`setFixedHeight`)，导致列内绝大部分空白区域暴露为底层的 `m_canvasWidget` / `m_paneScrollArea`，而这两个底层容器未开启 `Qt::CustomContextMenu`，右键事件被吞掉；当列为空（0 项目）时，列表隐藏，整列空白无响应。
     2. 最右侧延伸空白区 `ColumnBlankCanvasWidget` 传递坐标时将 `pos` 误做了 `mapToGlobal`，而下游 `ContentContextMenu::showMenu` 会对传入坐标再次执行 `mapToGlobal`，导致坐标二次映射而偏离屏幕。

---

## 1. Overview（概述与解决的问题）

本方案旨在解决列视图（ColumnView）中“点击列内空白处、空列内任意位置或最右侧延伸空白处时，无法正确弹出右键上下文菜单”的缺陷。

### 修复策略：
1. **`ColumnBlankCanvasWidget` 坐标修正**：修正最右侧延伸空白区右键菜单坐标转换逻辑，将全局坐标正确映射至当前激活列表视图（`activeItemView`）的 viewport 视口坐标系，避免二次 `mapToGlobal` 导致的偏离。
2. **`ColumnViewPane` 全空白区右键响应**：为 `ColumnViewPane` 内部的 `m_paneScrollArea` 和 `m_canvasWidget` 开启 `Qt::CustomContextMenu`，并在捕获右键信号后，将其转换为对应 `ColumnViewPane` 的 `listView` 视口坐标，统一交给 `ContentPanel::onCustomContextMenuRequested` 处理。

---

## 2. Modified Files List（影响文件清单）

- `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 修改点 1：`src/ui/ColumnViewWidget.cpp`
修复 `ColumnBlankCanvasWidget::onContextMenuRequested` 坐标二次 `mapToGlobal` 的问题，以及为 `ColumnViewPane` 的 `m_paneScrollArea` 与 `m_canvasWidget` 挂载空白区右键响应。

```
<<<<<<< SEARCH
private:
    void onContextMenuRequested(const QPoint& pos) {
        if (m_contentPanel) {
            m_contentPanel->onCustomContextMenuRequested(mapToGlobal(pos));
        }
    }

    ColumnViewWidget* m_columnView = nullptr;
    ContentPanel* m_contentPanel = nullptr;
    bool m_isDragHover = false;
};
=======
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

    ColumnViewWidget* m_columnView = nullptr;
    ContentPanel* m_contentPanel = nullptr;
    bool m_isDragHover = false;
};
>>>>>>> REPLACE
```

### 修改点 2：`src/ui/ColumnViewWidget.cpp`
在 `ColumnViewPane` 构造函数中，为 `m_paneScrollArea` 和 `m_canvasWidget` 开启右键菜单策略并绑定信号槽。

```
<<<<<<< SEARCH
    m_paneScrollArea = new QScrollArea(this);
    m_paneScrollArea->setObjectName("ColumnPaneScrollArea");
    m_paneScrollArea->setWidgetResizable(true);
    m_paneScrollArea->setFrameShape(QFrame::NoFrame);
    m_paneScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_paneScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_canvasWidget = new QWidget(m_paneScrollArea);
    m_canvasWidget->setObjectName("ColumnPaneCanvasWidget");
    QVBoxLayout* canvasLayout = new QVBoxLayout(m_canvasWidget);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->setSpacing(0);
=======
    m_paneScrollArea = new QScrollArea(this);
    m_paneScrollArea->setObjectName("ColumnPaneScrollArea");
    m_paneScrollArea->setWidgetResizable(true);
    m_paneScrollArea->setFrameShape(QFrame::NoFrame);
    m_paneScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_paneScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_paneScrollArea->setContextMenuPolicy(Qt::CustomContextMenu);

    m_canvasWidget = new QWidget(m_paneScrollArea);
    m_canvasWidget->setObjectName("ColumnPaneCanvasWidget");
    m_canvasWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    QVBoxLayout* canvasLayout = new QVBoxLayout(m_canvasWidget);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->setSpacing(0);

    auto handlePaneBlankContextMenu = [this](const QPoint& pos, QWidget* sourceWidget) {
        if (!m_contentPanel) return;
        QPoint globalPos = sourceWidget ? sourceWidget->mapToGlobal(pos) : QCursor::pos();
        DropListView* targetView = m_listView ? m_listView : m_folderListView;
        if (targetView && targetView->viewport()) {
            QPoint viewPos = targetView->viewport()->mapFromGlobal(globalPos);
            m_contentPanel->onCustomContextMenuRequested(viewPos);
        }
    };

    connect(m_paneScrollArea, &QWidget::customContextMenuRequested, this, [this, handlePaneBlankContextMenu](const QPoint& pos) {
        handlePaneBlankContextMenu(pos, m_paneScrollArea);
    });
    connect(m_canvasWidget, &QWidget::customContextMenuRequested, this, [this, handlePaneBlankContextMenu](const QPoint& pos) {
        handlePaneBlankContextMenu(pos, m_canvasWidget);
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
1. **验证列内文件下方空白处右键**：
   - 打开 QuarkMeta，切换至列视图（Column View）。
   - 进入包含少量文件的文件夹，在文件列表下方的空白区域右键单击。
   - 预期结果：正常弹出空白处右键上下文菜单（包含“新建...”、“粘贴”、“在资源管理器中显示”、“刷新”等项）。
2. **验证空列（0 项目）右键**：
   - 在列视图中创建一个空文件夹并进入。
   - 在该空列的任意空白位置右键单击。
   - 预期结果：正确弹出该空列对应的空白处上下文菜单。
3. **验证最右侧延伸空白区右键**：
   - 在列视图最右侧空白延伸区域（`ColumnBlankCanvasWidget`）右键单击。
   - 预期结果：右键菜单在鼠标光标位置精确弹出，无偏移或失效现象。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- **SSOT 通道复用**：本次修改完全复用了既有 `ContentPanel::onCustomContextMenuRequested(pos)` 接口以及 `ContentContextMenu::showMenu` 状态机逻辑，未另起炉灶创建任何新的菜单构建逻辑。
- **物理死代码清理**：清理并纠正了此前误将全局坐标直接传入 `onCustomContextMenuRequested` 的旧调用。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / API | 物理声明文件 | 精确物理签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `ContentPanel::onCustomContextMenuRequested` | `src/ui/ContentPanel.h` | `void onCustomContextMenuRequested(const QPoint& pos)` | ✅ 匹配一致 |
| `ContentPanel::activeItemView` | `src/ui/ContentPanel.h` | `QAbstractItemView* activeItemView() const` | ✅ 匹配一致 |
| `ColumnViewPane::listView` | `src/ui/ColumnViewWidget.h` | `DropListView* listView() const` | ✅ 匹配一致 |
| `ColumnViewPane::folderListView` | `src/ui/ColumnViewWidget.h` | `DropListView* folderListView() const` | ✅ 匹配一致 |
| `QWidget::mapToGlobal` | Qt Header | `QPoint mapToGlobal(const QPoint &pos) const` | ✅ 匹配一致 |
| `QWidget::mapFromGlobal` | Qt Header | `QPoint mapFromGlobal(const QPoint &pos) const` | ✅ 匹配一致 |
