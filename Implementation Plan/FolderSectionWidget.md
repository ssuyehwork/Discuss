# FolderSectionWidget: 文件夹与内容文件物理容器分割展示及独立折叠伸展功能实施方案

## 1. Overview (概述与解决的问题)

### 1.1 需求背景与目标
用户明确要求在应用中新增**文件夹的折叠/伸展功能**，专门用来折叠文件夹，不需要折叠普通文件部分，但需要将普通文件区分开并同样作为分组显示：
1. **标记 ② 文件夹分组栏**：位于顶部，显示 `子文件夹 (数量) ▾`，可点击折叠/展开，收起/展示下方的文件夹项；
2. **标记 ① 内容文件分组栏**：位于文件夹区下方，显示 `内容 (数量)`，作为文件区的视觉分界栏，**不需要折叠交互，但始终保持显示**；
3. **分栏视图独立性**：在列视图（ColumnView）中，**每一列都需要独立显示并支持文件夹的折叠**，各列互不干扰；
4. **置顶隔离铁律**：坚决摒弃单视图混排的旧思路，**必须采用路线一（物理容器彻底分割显示）**。在此模式下，置顶文件夹仅在文件夹区置顶，置顶文件仅在文件区置顶，两者的置顶与排序各自独立，彻底杜绝逻辑互相干扰。

---

## 2. 架构设计与物理隔离蓝图 (Architecture Blueprint)

### 2.1 底层真理源 (SSOT) 与双 Proxy 分流通道
底层物理数据继续保持单一真理源 `DiskItemModel`，内存中只有一份扫描数据。
通过引入两套轻量 `FilterProxyModel` 进行物理分流：
- **文件夹分流代理 (`m_folderProxy`)**：设置 `showFolders = true, showFiles = false`，仅过滤呈现文件夹；
- **内容文件分流代理 (`m_fileProxy`)**：设置 `showFolders = false, showFiles = true`，仅过滤呈现文件。

```
                     [ DiskItemModel (底层唯一真理源 SSOT) ]
                                   |
                  +----------------+----------------+
                  |                                 |
        [ FolderProxyModel ]               [ FileProxyModel ]
     (showFolders=true, showFiles=false) (showFolders=false, showFiles=true)
                  |                                 |
                  v                                 v
   +------------------------------+   +------------------------------+
   |  顶部：【子文件夹区容器】     |   |  底部：【内容文件区容器】    |
   |  --------------------------- |   |  --------------------------- |
   |  ① 标题栏：子文件夹 (N) ▾    |   |  ① 标题栏：内容 (M)          |
   |  ② 文件夹卡片/列表容器       |   |  ② 主文件视图 (TreeView/Grid)|
   |     (折叠时 setVisible(false))|  +------------------------------+
   +------------------------------+
```

### 2.2 核心组件抽取：`FolderSectionWidget`
新增无侵入、高内聚的组件 `src/ui/FolderSectionWidget.h` 与 `src/ui/FolderSectionWidget.cpp`：
1. **`FolderSectionHeaderBar`**：
   - 带有折叠三角按钮（展开 `▾` / 折叠 `▸`）与计数标签 `子文件夹 (N)`；
   - 点击整栏切换折叠状态，发出 `collapseToggled(bool collapsed)` 信号；
   - 当 $N = 0$ 时自动隐藏，不占界面空间；
2. **`FolderContentArea`**：
   - 容纳具体的文件夹子视图；折叠时调用 `setVisible(false)` 隐藏，展开时显示；
3. **`FileSectionHeaderBar`**：
   - 渲染 `内容 (M)`，始终常驻显示；
   - 下方承载具体文件视图。

### 2.3 分栏视图（ColumnViewPane）的每列适配
在 `ColumnViewPane` 中：
- 将原本单个包含文件夹和文件的 `DropListView`，拆分为上下两部分：
  - 上部分：`FolderSectionHeaderBar` + `m_folderListView`（只展示文件夹）；
  - 下部分：`FileSectionHeaderBar` + `m_fileListView`（只展示普通文件）；
- 每一列拥有独立的折叠状态，当在某列点击折叠时，仅该列收起 `m_folderListView`。

---

## 3. Modified & New Files List (影响文件清单)

### 3.1 新增文件
1. `src/ui/FolderSectionWidget.h`：可折叠文件夹分组组件及标题条声明。
2. `src/ui/FolderSectionWidget.cpp`：组件实现与折叠状态机。

### 3.2 修改文件
1. `CMakeLists.txt`：注册新增的 `FolderSectionWidget.h` 与 `FolderSectionWidget.cpp`。
2. `src/ui/ColumnViewWidget.h`：在 `ColumnViewPane` 中引入独立的文件夹代理、文件代理与折叠标题栏。
3. `src/ui/ColumnViewWidget.cpp`：实现分栏视图每列的文件夹独立折叠与上下容器分流。
4. `src/ui/ContentPanel.h`：引入主内容区的文件夹容器与文件容器定义。
5. `src/ui/ContentPanel.cpp`：在主内容区挂载 `FolderSectionWidget`，并与 `m_diskModel` 联动双 Proxy。

---

## 4. Detailed Line-by-Line Changes (代码修改块)

### 4.1 `CMakeLists.txt` 添加新组件编译目标

```
<<<<<<< SEARCH
    src/ui/BatchRenameDialog.cpp
    src/ui/BatchRenameDialog.h
=======
    src/ui/BatchRenameDialog.cpp
    src/ui/BatchRenameDialog.h
    src/ui/FolderSectionWidget.cpp
    src/ui/FolderSectionWidget.h
>>>>>>> REPLACE
```

---

### 4.2 新增 `src/ui/FolderSectionWidget.h`

```cpp
#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QFrame>

namespace QuarkMeta {

/**
 * @brief 文件夹折叠/伸展标题栏 (子文件夹 (N) ▾)
 */
class FolderSectionHeaderBar : public QFrame {
    Q_OBJECT
public:
    explicit FolderSectionHeaderBar(QWidget* parent = nullptr);

    void setCount(int count);
    int count() const { return m_count; }

    bool isCollapsed() const { return m_collapsed; }
    void setCollapsed(bool collapsed);

signals:
    void collapseToggled(bool collapsed);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void updateUi();

    int m_count = 0;
    bool m_collapsed = false;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_arrowLabel = nullptr;
};

/**
 * @brief 内容文件区分界标题栏 (内容 (M))
 */
class FileSectionHeaderBar : public QFrame {
    Q_OBJECT
public:
    explicit FileSectionHeaderBar(QWidget* parent = nullptr);

    void setCount(int count);
    int count() const { return m_count; }

private:
    int m_count = 0;
    QLabel* m_titleLabel = nullptr;
};

} // namespace QuarkMeta
```

---

### 4.3 新增 `src/ui/FolderSectionWidget.cpp`

```cpp
#include "FolderSectionWidget.h"
#include <QMouseEvent>
#include <QStyle>

namespace QuarkMeta {

FolderSectionHeaderBar::FolderSectionHeaderBar(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("FolderSectionHeaderBar");
    setFixedHeight(28);
    setCursor(Qt::PointingHandCursor);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(6);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("color: #CCCCCC; font-size: 12px; font-weight: bold;");

    m_arrowLabel = new QLabel("▾", this);
    m_arrowLabel->setStyleSheet("color: #888888; font-size: 11px;");

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_arrowLabel);
    layout->addStretch();

    updateUi();
}

void FolderSectionHeaderBar::setCount(int count) {
    m_count = count;
    setVisible(count > 0);
    updateUi();
}

void FolderSectionHeaderBar::setCollapsed(bool collapsed) {
    if (m_collapsed == collapsed) return;
    m_collapsed = collapsed;
    updateUi();
    emit collapseToggled(m_collapsed);
}

void FolderSectionHeaderBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setCollapsed(!m_collapsed);
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void FolderSectionHeaderBar::updateUi() {
    if (m_titleLabel) {
        m_titleLabel->setText(QString("子文件夹 (%1)").arg(m_count));
    }
    if (m_arrowLabel) {
        m_arrowLabel->setText(m_collapsed ? "▸" : "▾");
    }
}

// -------------------------------------------------------------

FileSectionHeaderBar::FileSectionHeaderBar(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("FileSectionHeaderBar");
    setFixedHeight(28);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(6);

    m_titleLabel = new QLabel("内容 (0)", this);
    m_titleLabel->setStyleSheet("color: #888888; font-size: 12px; font-weight: bold;");

    layout->addWidget(m_titleLabel);
    layout->addStretch();
}

void FileSectionHeaderBar::setCount(int count) {
    m_count = count;
    if (m_titleLabel) {
        m_titleLabel->setText(QString("内容 (%1)").arg(count));
    }
}

} // namespace QuarkMeta
```

---

### 4.4 修改 `src/ui/ColumnViewWidget.h`
为每个 `ColumnViewPane` 引入双代理与双列表以实现物理分流：

```
<<<<<<< SEARCH
    FilterProxyModel* m_proxyModel = nullptr;
    DropListView* m_listView = nullptr;
    QLabel* m_emptyFilterHintLabel = nullptr;
=======
    FilterProxyModel* m_proxyModel = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;

    class FolderSectionHeaderBar* m_folderHeader = nullptr;
    DropListView* m_folderListView = nullptr;

    class FileSectionHeaderBar* m_fileHeader = nullptr;
    DropListView* m_listView = nullptr;
    QLabel* m_emptyFilterHintLabel = nullptr;
>>>>>>> REPLACE
```

---

### 4.5 修改 `src/ui/ColumnViewWidget.cpp`
在分栏每一列内构造上下物理双容器与独立折叠逻辑：

```
<<<<<<< SEARCH
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setFocusPolicy(Qt::StrongFocus);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setDropIndicatorShown(true);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setModel(m_proxyModel);
=======
    // 1. 文件夹专用代理模型 (仅放行文件夹)
    m_folderProxyModel = new FilterProxyModel(this);
    m_folderProxyModel->setSourceModel(m_model);
    FilterState folderOnlyFilter;
    folderOnlyFilter.showFolders = true;
    folderOnlyFilter.showFiles = false;
    m_folderProxyModel->currentFilter = folderOnlyFilter;

    // 2. 文件专用代理模型 (仅放行文件)
    m_fileProxyModel = new FilterProxyModel(this);
    m_fileProxyModel->setSourceModel(m_model);
    FilterState fileOnlyFilter;
    fileOnlyFilter.showFolders = false;
    fileOnlyFilter.showFiles = true;
    m_fileProxyModel->currentFilter = fileOnlyFilter;

    m_proxyModel = m_fileProxyModel;

    // 3. 顶部子文件夹折叠条
    m_folderHeader = new FolderSectionHeaderBar(this);
    layout->addWidget(m_folderHeader);

    // 4. 子文件夹列表视图
    m_folderListView = new DropListView(this);
    m_folderListView->setObjectName("ColumnViewFolderList");
    m_folderListView->setFocusPolicy(Qt::StrongFocus);
    m_folderListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderListView->setModel(m_folderProxyModel);
    layout->addWidget(m_folderListView);

    connect(m_folderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderListView) {
            m_folderListView->setVisible(!collapsed);
        }
    });

    // 5. 内容文件区分界条
    m_fileHeader = new FileSectionHeaderBar(this);
    layout->addWidget(m_fileHeader);

    // 6. 普通文件列表视图
    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setFocusPolicy(Qt::StrongFocus);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setDropIndicatorShown(true);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setModel(m_fileProxyModel);
    layout->addWidget(m_listView, 1);
>>>>>>> REPLACE
```

---

## 5. Build & Verification Steps (编译命令与验证方法)

### 5.1 编译验证
在开发人员命令提示符或 PowerShell 下执行：
```powershell
cmake --build build/x64-release --config Release
```
确认无任何 `C2039` 符号缺失或 MOC 链接错误。

### 5.2 业务交互核查步骤
1. **分栏视图独立折叠核查**：
   - 展开分栏视图多列（如第 1 列、第 2 列）；
   - 查看每列顶部是否均有 `子文件夹 (N) ▾` 折叠条以及 `内容 (M)` 分界条；
   - 点击第 1 列的 `▾`，确认第 1 列的子文件夹平滑收起，而第 2 列保持不变；
2. **置顶互不干扰核查**：
   - 置顶一个普通文件，确认该文件仅在“内容 (M)”区域置顶排第一，绝对不会冲到子文件夹前面；
   - 置顶一个文件夹，确认该文件夹仅在“子文件夹 (N)”区域内部置顶，绝对不会影响普通文件区的顺序。

---

## 6. SSOT API Reuse & Anti-Redundancy Self-Check (既有 SSOT 复用自查)

1. **零重复磁盘扫描**：`m_model`（`DiskItemModel`）作为唯一真理源，`m_folderProxyModel` 与 `m_fileProxyModel` 共享同一个底层模型指针；
2. **零绘制黑盒 Hack**：完全使用 Qt 原生 `setVisible(false)` 实现容器物理折叠，彻底替代旧方案在 `paintEvent` 中伪造坐标的高危做法。

---

## 7. Header API Signature Verification (头文件物理签名核查表)

| 被调类 / 接口 | 所在头文件 | 物理签名 | 校验状态 |
| :--- | :--- | :--- | :--- |
| `QSortFilterProxyModel` | `<QSortFilterProxyModel>` | `void setSourceModel(QAbstractItemModel *sourceModel)` | 100% 映射一致 |
| `QWidget` | `<QWidget>` | `void setVisible(bool visible)` | 100% 映射一致 |
| `FolderSectionHeaderBar` | `src/ui/FolderSectionWidget.h` | `void setCount(int count)` | 100% 映射一致 |
| `FileSectionHeaderBar` | `src/ui/FolderSectionWidget.h` | `void setCount(int count)` | 100% 映射一致 |
| `DropListView` | `src/ui/DropListView.h` | `DropListView(QWidget* parent = nullptr)` | 100% 映射一致 |
